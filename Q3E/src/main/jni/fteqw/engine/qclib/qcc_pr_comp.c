#if !defined(MINIMAL) && !defined(OMIT_QCC)

#include "qcc.h"
#include <math.h>

#if defined(_WIN32) || defined(__DJGPP__)
	#include <malloc.h>
#elif defined(__unix__) && !defined(__linux__) // quick hack for the bsds and other unix systems
    #include<stdlib.h>
#elif !defined(alloca)	//alloca.h isn't present on bsd (stdlib.h should define it to __builtin_alloca, and we can check for that here).
	#include <alloca.h>
#endif

#ifdef _MSC_VER
#define longlong __int64
#define LL(x) x##i64
#else
#define longlong long long
#define LL(x) x##ll
#endif

/*
TODO:
*foo++ = 5;
is currently store foo->tmp; store 5->*tmp; add tmp,1->foo
should be just store 5->*foo; add foo,1->foo
the dereference does the post-inc. needs to be delayed somehow. could build a list of post-inc terms after the current expression, but might be really messy.
*/

#define WARN_IMPLICITVARIANTCAST 0

//FIXME: #define IAMNOTLAZY

#define SUPPORTINLINE

void QCC_PR_ParseAsm(void);

#define MEMBERFIELDNAME "__m%s"

#define STRCMP(s1,s2) (((*s1)!=(*s2)) || strcmp(s1,s2))	//saves about 2-6 out of 120 - expansion of idea from fastqcc
#define STRNCMP(s1,s2,l) (((*s1)!=(*s2)) || strncmp(s1,s2,l))	//pathetic saving here.

extern char *compilingfile;

int conditional;

//standard qc keywords
#define keyword_do		1
#define keyword_return	1
#define keyword_if		1
#define keyword_else	1
#define keyword_while	1

//extended keywords.
pbool keyword_switch;	//hexen2/c
pbool keyword_case;		//hexen2/c
pbool keyword_default;	//hexen2/c
pbool keyword_break;	//hexen2/c
pbool keyword_continue;	//hexen2/c
pbool keyword_loop;		//hexen2
pbool keyword_until;	//hexen2
pbool keyword_thinktime;//hexen2
pbool keyword_asm;
pbool keyword_class;
pbool keyword_accessor;
pbool keyword_inout;
pbool keyword_optional;
pbool keyword_const;	//fixme
pbool keyword_entity;	//for skipping the local
pbool keyword_float;	//for skipping the local
pbool keyword_double;
pbool keyword_for;
pbool keyword_goto;
pbool keyword_char;
pbool keyword_short;
pbool keyword_int;		//for skipping the local
pbool keyword_integer;	//for skipping the local
pbool keyword_long;
pbool keyword_signed;
pbool keyword_unsigned;
pbool keyword_register;
pbool keyword_volatile;
pbool keyword_state;
pbool keyword_string;	//for skipping the local
pbool keyword_struct;
pbool keyword_var;		//allow it to be initialised and set around the place.
pbool keyword_vector;	//for skipping the local
pbool keyword_local;
pbool keyword_static;
pbool keyword_auto;
pbool keyword_nonstatic;
pbool keyword_used;
pbool keyword_unused;


pbool keyword_enum;	//kinda like in c, but typedef not supported.
pbool keyword_enumflags;	//like enum, but doubles instead of adds 1.
pbool keyword_typedef;	//fixme
#define keyword_codesys		flag_acc	//reacc needs this (forces the resultant crc)
#define keyword_function	flag_acc	//reacc needs this (reacc has this on all functions, wierd eh?)
#define keyword_objdata		flag_acc	//reacc needs this (following defs are fields rather than globals, use var to disable)
#define keyword_object		flag_acc	//reacc needs this (an entity)
#define keyword_pfunc		flag_acc	//reacc needs this (pointer to function)
#define keyword_system		flag_acc	//reacc needs this (potatos)
#define keyword_real		flag_acc	//reacc needs this (a float)
#define keyword_exit		flag_acc	//emits an OP_DONE opcode.
#define keyword_external	flag_acc	//reacc needs this (a builtin)
pbool keyword_extern;	//function is external, don't error or warn if the body was not found
pbool keyword_shared;	//mark global to be copied over when progs changes (part of FTE_MULTIPROGS)
pbool keyword_noref;	//nowhere else references this, don't strip it.
pbool keyword_nosave;	//don't write the def to the output.
pbool keyword_inline;
pbool keyword_strip;
pbool keyword_ignore;
pbool keyword_union;	//you surly know what a union is!
pbool keyword_weak;
pbool keyword_wrap;
pbool keyword_accumulate;
pbool keyword_using;

#define keyword_not			1	//hexenc support needs this, and fteqcc can optimise without it, but it adds an extra token after the if, so it can cause no namespace conflicts

pbool keywords_coexist;		//don't disable a keyword simply because a var was made with the same name.
pbool output_parms;			//emit some PARMX fields. confuses decompilers.
pbool autoprototype;		//take two passes over the source code. First time round doesn't enter and functions or initialise variables.
pbool autoprototyped;		//previously autoprototyped. no longer allowed to enable autoproto, but don't warn about it.
pbool parseonly;			//parse defs and stuff, but don't bother compiling any actual code.
pbool pr_subscopedlocals;	//causes locals to be valid ONLY within their statement block. (they simply can't be referenced by name outside of it)
pbool flag_nullemptystr;	//null immediates are 0, not 1.
pbool flag_brokenifstring;	//break strings even more
pbool flag_ifstring;		//makes if (blah) equivelent to if (blah != "") which resolves some issues in multiprogs situations.
pbool flag_iffloat;			//use an op_if_f instruction instead of op_if so if(-0) evaluates to false.
pbool flag_ifvector;		//use an op_not_v instruction instead of testing only _x.
pbool flag_vectorlogic;		//flag_ifvector but for && and ||
pbool flag_acc;				//reacc like behaviour of src files (finds *.qc in start dir and compiles all in alphabetical order)
pbool flag_caseinsensitive;	//symbols will be matched to an insensitive case if the specified case doesn't exist. This should b usable for any mod
pbool flag_laxcasts;		//Allow lax casting. This'll produce loadsa warnings of course. But allows compilation of certain dodgy code.
pbool flag_hashonly;		//Allows use of only #constant for precompiler constants, allows certain preqcc using mods to compile
pbool flag_macroinstrings;	//preqcc's support macro expansion in strings (blind expansion).
pbool flag_fasttrackarrays;	//Faster arrays, dynamically detected, activated only in supporting engines.
pbool flag_msvcstyle;		//MSVC style warnings, so msvc's ide works properly
pbool flag_debugmacros;		//Print out #defines as they are expanded, for debugging.
pbool flag_assume_integer;	//5 - is that an integer or a float? qcc says float. but we support int too, so maybe we want that instead?
pbool flag_assume_double;	//5.0 - is that single or double precision? QC says float, but C says double. should probably only be used with assume-int enabled too.
pbool flag_filetimes;		//only rebuild if files were modified.
pbool flag_typeexplicit;	//no implicit type conversions, you must do the casts yourself.
pbool flag_boundchecks;		//Disable generation of bound check instructions.
pbool flag_guiannotate;		//spit out lots of extra text that the gui can interpret to display some inline asm
pbool flag_brokenarrays;	//return array; returns array[0] instead of &array;
pbool flag_rootconstructor;	//if true, class constructors are ordered to call the super constructor first, rather than the child constructor
pbool flag_qccx;			//accept qccx syntax. you may wish to disable warnings separately.
pbool flag_attributes;		//gmqcc-style attributes
pbool flag_assumevar;		//initialised globals will no longer be considered constant
pbool flag_dblstarexp;		// a**b is pow(a,b) instead of a*(*b)
pbool flag_cpriority;		//operator precidence should adhere to C standards, instead of QC compatibility.
pbool flag_qcfuncs;			//void() is a function type, and not a syntax error.
pbool flag_allowuninit;		//ignore uninitialised locals, avoiding all private locals.
pbool flag_embedsrc;		//embed all source files inside the .dat (can be opened with any zip program)
pbool flag_noreflection;	//no reflection stuff, for smaller unsavable binaries.
pbool flag_nopragmafileline;//ignore #pragma file and #pragma line, so that I can actually read+debug xonotic's code.
pbool flag_utf8strings;		//strings default to u8"" string rules.
pbool flag_reciprocalmaths;	//unsafe maths optimisations
pbool flag_ILP32;			//restrict long to 32 bits. long long still exists.
pbool flag_undefwordsize;	//pointers are NOT always multiples of 4. sizeof becomes an error foo.length will still work though. pointer maths will resort to OP_ADD_PIW in order to still work (or just break). char and short types are unusable. casting between strings and pointers is an error (though you can still contrive casts past it). for compat with DP and its potential 64bit qcvm.
pbool flag_pointerrelocs;	//engine accepts ev_pointer globaldefs and biases them by the in-memory globals.

pbool opt_overlaptemps;		//reduce numpr_globals by reuse of temps. When they are not needed they are freed for reuse. The way this is implemented is better than frikqcc's. (This is the single most important optimisation)
pbool opt_assignments;		//STORE_F isn't used if an operation wrote to a temp.
pbool opt_shortenifnots;		//if(!var) is made an IF rather than NOT IFNOT
pbool opt_noduplicatestrings;	//brute force string check. time consuming but more effective than the equivelent in frikqcc.
pbool opt_constantarithmatic;	//3*5 appears as 15 instead of the extra statement.
pbool opt_nonvec_parms;			//store_f instead of store_v on function calls, where possible.
pbool opt_constant_names;		//take out the defs and name strings of constants.
pbool opt_constant_names_strings;//removes the defs of strings too. plays havok with multiprogs.
pbool opt_precache_file;			//remove the call, the parameters, everything.
pbool opt_filenames;				//strip filenames. hinders older decompilers.
pbool opt_unreferenced;			//strip defs that are not referenced.
pbool opt_function_names;		//strip out the names of builtin functions.
pbool opt_locals;				//strip out the names of locals and immediates.
pbool opt_dupconstdefs;			//float X = 5; and float Y = 5; occupy the same global with this.
pbool opt_return_only;			//RETURN; DONE; at the end of a function strips out the done statement if there is no way to get to it.
pbool opt_compound_jumps;		//jumps to jump statements jump to the final point.
pbool opt_stripfunctions;		//if a functions is only ever called directly or by exe, don't emit the def.
pbool opt_locals_overlapping;	//make the local vars of all functions occupy the same globals.
pbool opt_logicops;				//don't make conditions enter functions if the return value will be discarded due to a previous value. (C style if statements)
pbool opt_vectorcalls;			//vectors can be packed into 3 floats, which can yield lower numpr_globals, but cost two more statements per call (only works for q1 calling conventions).
pbool opt_classfields;
pbool opt_simplifiedifs;		//if (f != 0) -> if_f (f). if (f == 0) -> ifnot_f (f)
//bool opt_comexprremoval;

//these are the results of the opt_. The values are printed out when compilation is compleate, showing effectivness.
int optres_shortenifnots;
int optres_assignments;
int optres_overlaptemps;
int optres_noduplicatestrings;
int optres_constantarithmatic;
int optres_nonvec_parms;
int optres_constant_names;
int optres_constant_names_strings;
int optres_precache_file;
int optres_filenames;
int optres_unreferenced;
int optres_function_names;
int optres_locals;
int optres_dupconstdefs;
int optres_return_only;
int optres_compound_jumps;
//int optres_comexprremoval;
int optres_stripfunctions;
int optres_locals_overlapping;
int optres_logicops;
int optres_inlines;

int optres_test1;
int optres_test2;

void *(*pHash_Get)(hashtable_t *table, const char *name);
void *(*pHash_GetNext)(hashtable_t *table, const char *name, void *old);
void *(*pHash_Add)(hashtable_t *table, const char *name, void *data, bucket_t *);
void (*pHash_RemoveData)(hashtable_t *table, const char *name, void *data);

QCC_type_t *QCC_PR_FindType (QCC_type_t *type);
QCC_type_t *QCC_PR_PointerType (QCC_type_t *pointsto);
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto);
QCC_sref_t	QCC_PR_Term (unsigned int exprflags);
QCC_sref_t	QCC_PR_ParseValue (QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers);
void		QCC_Marshal_Locals(int firststatement, int laststatement);
QCC_sref_t	QCC_PR_ParseArrayPointer (QCC_sref_t d, pbool allowarrayassign, pbool makestructpointers);
QCC_sref_t	QCC_LoadFromArray(QCC_sref_t base, QCC_sref_t index, QCC_type_t *t, pbool preserve);
void		 QCC_PR_ParseInitializerDef(QCC_def_t *def, unsigned int flags);

static pbool QCC_RefNeedsCalls(QCC_ref_t *ref);
QCC_ref_t *QCC_DefToRef(QCC_ref_t *ref, QCC_sref_t def);	//ref is a buffer to write into, to avoid excessive allocs
QCC_sref_t	QCC_RefToDef(QCC_ref_t *ref, pbool freetemps);
QCC_ref_t *QCC_PR_RefExpression (QCC_ref_t *retbuf, int priority, int exprflags);
QCC_ref_t *QCC_PR_ParseRefValue (QCC_ref_t *refbuf, QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers);
QCC_ref_t *QCC_PR_ParseRefArrayPointer (QCC_ref_t *refbuf, QCC_ref_t *d, pbool allowarrayassign, pbool makestructpointers);
QCC_ref_t *QCC_PR_BuildRef(QCC_ref_t *retbuf, unsigned int reftype, QCC_sref_t base, QCC_sref_t index, QCC_type_t *cast, pbool	readonly, unsigned int bitofs);
QCC_ref_t *QCC_PR_BuildAccessorRef(QCC_ref_t *retbuf, QCC_sref_t base, QCC_sref_t index, struct accessor_s *accessor, pbool readonly);
QCC_sref_t	QCC_StoreSRefToRef(QCC_ref_t *dest, QCC_sref_t source, pbool readable, pbool preservedest);
QCC_sref_t	QCC_StoreRefToRef(QCC_ref_t *dest, QCC_ref_t *source, pbool readable, pbool preservedest);
void QCC_PR_DiscardRef(QCC_ref_t *ref);
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type, pbool dowrap);
const char *QCC_VarAtOffset(QCC_sref_t ref);
QCC_sref_t QCC_EvaluateCast(QCC_sref_t src, QCC_type_t *cast, pbool implicit);
QCC_sref_t QCC_PR_ParseInitializerTemp(QCC_type_t *type);
pbool QCC_PR_ParseInitializerType(int arraysize, QCC_def_t *basedef, QCC_sref_t def, unsigned bitofs, unsigned int flags);
#define PIF_WRAP		1	//new initialisation is meant to wrap an existing one.
#define PIF_STRONGER	2	//previous initialisation was weak.
#define PIF_ACCUMULATE	4	//glue them together...
#define PIF_AUTOWRAP	8	//accumulate without wrap must autowrap if the function was not previously defined as accumulate...

QCC_statement_t *QCC_Generate_OP_IFNOT(QCC_sref_t e, pbool preserve);
QCC_statement_t *QCC_Generate_OP_IF(QCC_sref_t e, pbool preserve);
QCC_statement_t *QCC_Generate_OP_GOTO(void);
QCC_sref_t QCC_PR_GenerateLogicalNot(QCC_sref_t e, const char *errormessage);
static QCC_function_t *QCC_PR_GenerateQCFunction (QCC_def_t *def, QCC_type_t *type, unsigned int *pif_flags);
static void QCC_StoreToSRef(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type, pbool preservesource, pbool preservedest);
void QCC_PR_ParseStatement (void);

//NOTE: prints may use from the func argument's symbol, which can be awkward if its a temp.
QCC_sref_t	QCC_PR_GenerateFunctionCallSref (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t *arglist, int argcount);
QCC_sref_t QCC_PR_GenerateFunctionCallRef (QCC_sref_t newself, QCC_sref_t func, QCC_ref_t **arglist, unsigned int argcount);
QCC_sref_t QCC_PR_GenerateFunctionCall1 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a);
QCC_sref_t QCC_PR_GenerateFunctionCall2 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a, QCC_sref_t b, QCC_type_t *type_b);
QCC_sref_t QCC_PR_GenerateFunctionCall3 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a, QCC_sref_t b, QCC_type_t *type_b, QCC_sref_t c, QCC_type_t *type_c);

QCC_sref_t QCC_MakeTranslateStringConst(const char *value);
QCC_sref_t QCC_MakeStringConst(const char *value);
QCC_sref_t QCC_MakeStringConstLength(const char *value, int length);
QCC_sref_t QCC_MakeFloatConst(pvec_t value);
QCC_sref_t QCC_MakeDoubleConst(double value);
QCC_sref_t QCC_MakeFloatConstFromInt(longlong llvalue);
QCC_sref_t QCC_MakeIntConst(longlong llvalue);	//longlongs for warnings
QCC_sref_t QCC_MakeUIntConst(unsigned longlong llvalue);
QCC_sref_t QCC_MakeInt64Const(longlong llvalue);
QCC_sref_t QCC_MakeUInt64Const(unsigned longlong llvalue);
static QCC_sref_t QCC_MakeUniqueConst(QCC_type_t *type, void *data);
QCC_sref_t QCC_MakeVectorConst(pvec_t a, pvec_t b, pvec_t c);
static QCC_sref_t QCC_MakeGAddress(QCC_type_t *type, QCC_def_t *relocof, int idx, int bitofs);

enum
{
	STFL_PRESERVEA=1u<<0,	//if a temp is released as part of the statement, it can be reused for the result. Which is bad if the temp is needed for something else, like e.e.f += 4;
	STFL_CONVERTA=1u<<1,		//convert to/from ints/floats to match the operand types required by the opcode
	STFL_PRESERVEB=1u<<2,
	STFL_CONVERTB=1u<<3,
	STFL_DISCARDRESULT=1u<<4,
	STFL_NOEMULATE=1u<<5,	//don't emulate unsupported opcode with other opcodes.
};
#define QCC_PR_Statement(op,a,b,st) QCC_PR_StatementFlags(op,a,b,st,STFL_CONVERTA|STFL_CONVERTB)
QCC_sref_t QCC_PR_StatementFlags ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_statement_t **outstatement, unsigned int flags);
#ifdef __GNUC__
void QCC_PR_StatementAnnotation(const char *fmt, ...) __attribute__((deprecated("don't forget about me")))LIKEPRINTF(1);
#endif

void QCC_PR_ParseState (void);
pbool expandedemptymacro;	//just inhibits warnings about hanging semicolons

QCC_pr_info_t	pr;
//QCC_def_t		**pr_global_defs/*[MAX_REGS]*/;	// to find def for a global variable

//keeps track of how many funcs are called while parsing a statement
//int qcc_functioncalled;

//========================================

const QCC_sref_t nullsref = {0};

struct QCC_function_s		*pr_scope;		// the function being parsed, or NULL
QCC_type_t		*pr_classtype;	// the class that the current function is part of.
QCC_type_t		*pr_assumetermtype;	//undefined things get this time, with no warning about being undeclared (used for the state function, so prototypes are not needed)
QCC_function_t	*pr_assumetermscope;
unsigned int	pr_assumetermflags; //GDF_
pbool			pr_ignoredeprecation;
pbool	pr_dumpasm;
const char		*s_unitn;
const char		*s_filen;
QCC_string_t	s_filed;			// filename for function definition

unsigned int			locals_marshalled;	// largest local block size that needs to be allocated for locals overlapping.

jmp_buf		pr_parse_abort;		// longjump with this on parse error

pbool qcc_usefulstatement;

pbool debug_armour_defined;

int max_breaks;
int max_continues;
int max_cases;
int num_continues;
int num_breaks;
int num_cases;
int *pr_breaks;
int *pr_continues;
int *pr_cases;
QCC_ref_t *pr_casesref;
QCC_ref_t *pr_casesref2;

#define MAX_LABEL_LENGTH 256
typedef struct {
	int statementno;
	int lineno;
	char name[MAX_LABEL_LENGTH];
} gotooperator_t;

int max_labels;
int max_gotos;
gotooperator_t *pr_labels;
gotooperator_t *pr_gotos;
int num_gotos;
int num_labels;

QCC_sref_t extra_parms[MAX_EXTRA_PARMS];

static QCC_statement_t *initstatements;
static size_t numinitstatements, maxinitstatements;

//#define ASSOC_RIGHT_RESULT ASSOC_RIGHT

//========================================

#undef PC_NONE
enum
{
	PC_NONE,
	PC_STORE,	//stores are handled specially, but its still nice to mark them
	PC_UNARY,	//these happen elsewhere
	PC_MEMBER,	//these happen elsewhere
	PC_TERNARY,
	PC_UNARYNOT,

	PC_MULDIV,
	PC_ADDSUB,
	PC_SHIFT,
	PC_RELATION,
	PC_EQUALITY,
	PC_BITAND,
	PC_BITXOR,
	PC_BITOR,
	PC_LOGICAND,
	PC_LOGICOR,
	MAX_PRIORITY_CLASSES
};
static int priority_class[MAX_PRIORITY_CLASSES+1];	//to simplify implementation slightly

/*static char *reftypename[] =
{
	"REF_GLOBAL",
	"REF_ARRAY",
	"REF_ARRAYHEAD",
	"REF_POINTER",
	"REF_POINTERARRAY",
	"REF_FIELD",
	"REF_STRING",
	"REF_NONVIRTUAL",
	"REF_THISCALL",
	"REF_ACCESSOR"
};*/

//FIXME: modifiy list so most common GROUPS are first
//use look up table for value of first char and sort by first char and most common...?

//if true, effectivly {b=a; return a;}
QCC_opcode_t pr_opcodes[] =
{
 {6, "<DONE>", "DONE",		PC_NONE,	ASSOC_LEFT,		&type_void,		&type_void,		&type_void},

 {6, "*", "MUL_F",			PC_MULDIV,	ASSOC_LEFT,		&type_float,	&type_float,	&type_float,	OPF_STD},
 {6, "*", "MUL_V",			PC_MULDIV,	ASSOC_LEFT,		&type_vector,	&type_vector,	&type_float,	OPF_STD},
 {6, "*", "MUL_FV",			PC_MULDIV,	ASSOC_LEFT,		&type_float,	&type_vector,	&type_vector,	OPF_STD},
 {6, "*", "MUL_VF",			PC_MULDIV,	ASSOC_LEFT,		&type_vector,	&type_float,	&type_vector,	OPF_STD},

 {6, "/", "DIV_F",			PC_MULDIV,	ASSOC_LEFT,		&type_float,	&type_float,	&type_float,	OPF_STD},

 {6, "+", "ADD_F",			PC_ADDSUB,	ASSOC_LEFT,		&type_float,	&type_float,	&type_float,	OPF_STD},
 {6, "+", "ADD_V",			PC_ADDSUB,	ASSOC_LEFT,		&type_vector,	&type_vector,	&type_vector,	OPF_STD},

 {6, "-", "SUB_F",			PC_ADDSUB,	ASSOC_LEFT,		&type_float,	&type_float,	&type_float,	OPF_STD},
 {6, "-", "SUB_V",			PC_ADDSUB,	ASSOC_LEFT,		&type_vector,	&type_vector,	&type_vector,	OPF_STD},

 {6, "==", "EQ_F",			PC_EQUALITY, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, "==", "EQ_V",			PC_EQUALITY, ASSOC_LEFT,	&type_vector,	&type_vector,	&type_bfloat,	OPF_STD},
 {6, "==", "EQ_S",			PC_EQUALITY, ASSOC_LEFT,	&type_string,	&type_string,	&type_bfloat,	OPF_STD},
 {6, "==", "EQ_E",			PC_EQUALITY, ASSOC_LEFT,	&type_entity,	&type_entity,	&type_bfloat,	OPF_STD},
 {6, "==", "EQ_FNC",		PC_EQUALITY, ASSOC_LEFT,	&type_function,	&type_function,	&type_bfloat,	OPF_STD},

 {6, "!=", "NE_F",			PC_EQUALITY, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, "!=", "NE_V",			PC_EQUALITY, ASSOC_LEFT,	&type_vector,	&type_vector,	&type_bfloat,	OPF_STD},
 {6, "!=", "NE_S",			PC_EQUALITY, ASSOC_LEFT,	&type_string,	&type_string,	&type_bfloat,	OPF_STD},
 {6, "!=", "NE_E",			PC_EQUALITY, ASSOC_LEFT,	&type_entity,	&type_entity,	&type_bfloat,	OPF_STD},
 {6, "!=", "NE_FNC",		PC_EQUALITY, ASSOC_LEFT,	&type_function,	&type_function,	&type_bfloat,	OPF_STD},

 {6, "<=", "LE_F",			PC_RELATION, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, ">=", "GE_F",			PC_RELATION, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, "<", "LT_F",			PC_RELATION, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, ">", "GT_F",			PC_RELATION, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},

 {6, ".", "LOADF_F",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_float},
 {6, ".", "LOADF_V",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_vector},
 {6, ".", "LOADF_S",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_string},
 {6, ".", "LOADF_ENT",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_entity},
 {6, ".", "LOADF_FLD",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_field},
 {6, ".", "LOADF_FNC",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_function},

 {6, ".", "FLDADDRESS",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field,	&type_pointer},

 {6, "=", "STORE_F",		PC_STORE, ASSOC_RIGHT,		&type_float,	&type_float,	&type_float,	OPF_STORE},
 {6, "=", "STORE_V",		PC_STORE, ASSOC_RIGHT,		&type_vector,	&type_vector,	&type_vector,	OPF_STORE},
 {6, "=", "STORE_S",		PC_STORE, ASSOC_RIGHT,		&type_string,	&type_string,	&type_string,	OPF_STORE},
 {6, "=", "STORE_ENT",		PC_STORE, ASSOC_RIGHT,		&type_entity,	&type_entity,	&type_entity,	OPF_STORE},
 {6, "=", "STORE_FLD",		PC_STORE, ASSOC_RIGHT,		&type_field,	&type_field,	&type_field,	OPF_STORE},
 {6, "=", "STORE_FNC",		PC_STORE, ASSOC_RIGHT,		&type_function,	&type_function,	&type_function,	OPF_STORE},

 {6, "=", "STOREP_F",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_float,	&type_float,	OPF_STOREPTROFS},
 {6, "=", "STOREP_V",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_vector,	&type_vector,	OPF_STOREPTROFS},
 {6, "=", "STOREP_S",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_string,	&type_string,	OPF_STOREPTROFS},
 {6, "=", "STOREP_ENT",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_entity,	&type_entity,	OPF_STOREPTROFS},
 {6, "=", "STOREP_FLD",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_field,	&type_field,	OPF_STOREPTROFS},
 {6, "=", "STOREP_FNC",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_function,	&type_function,	OPF_STOREPTROFS},

 {6, "<RETURN>", "RETURN",	PC_NONE, ASSOC_LEFT,		&type_vector,	&type_void,		&type_void},

 {6, "!", "NOT_F",			PC_UNARY, ASSOC_LEFT,		&type_float,	&type_void,		&type_bfloat,	OPF_STDUNARY},
 {6, "!", "NOT_V",			PC_UNARY, ASSOC_LEFT,		&type_vector,	&type_void,		&type_bfloat,	OPF_STDUNARY},
 {6, "!", "NOT_S",			PC_UNARY, ASSOC_LEFT,		&type_vector,	&type_void,		&type_bfloat,	OPF_STDUNARY},
 {6, "!", "NOT_ENT",		PC_UNARY, ASSOC_LEFT,		&type_entity,	&type_void,		&type_bfloat,	OPF_STDUNARY},
 {6, "!", "NOT_FNC",		PC_UNARY, ASSOC_LEFT,		&type_function,	&type_void,		&type_bfloat,	OPF_STDUNARY},

  {6, "<IF>", "IF",			PC_NONE, ASSOC_RIGHT,		&type_float,	NULL,			&type_void},
  {6, "<IFNOT>", "IFNOT",	PC_NONE, ASSOC_RIGHT,		&type_float,	NULL,			&type_void},

// calls returns REG_RETURN
 {6, "<CALL0>", "CALL0",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL1>", "CALL1",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL2>", "CALL2",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL3>", "CALL3",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL4>", "CALL4",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL5>", "CALL5",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL6>", "CALL6",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL7>", "CALL7",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},
 {6, "<CALL8>", "CALL8",	PC_NONE, ASSOC_LEFT,		&type_function,	&type_void,		&type_void},

 {6, "<STATE>", "STATE",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_void},

 {6, "<GOTO>", "GOTO",		PC_NONE, ASSOC_RIGHT,		NULL,			&type_void,		&type_void},

 {6, "&&", "AND_F",			PC_LOGICAND, ASSOC_LEFT,	&type_float,	&type_float,	&type_bfloat,	OPF_STD},
 {6, "||", "OR_F",			PC_LOGICOR, ASSOC_LEFT,		&type_float,	&type_float,	&type_bfloat,	OPF_STD},

 {6, "&", "BITAND",			PC_BITAND, ASSOC_LEFT,		&type_float,	&type_float,	&type_float,		OPF_STD},
 {6, "|", "BITOR",			PC_BITOR, ASSOC_LEFT,		&type_float,	&type_float,	&type_float,		OPF_STD},

 //version 6 are in normal progs.



//these are hexen2
 {7, "*=", "MULSTORE_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float,		OPF_STORE},
 {7, "*=", "MULSTORE_VF",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_vector, &type_float, &type_vector,	OPF_STORE},
 {7, "*=", "MULSTOREP_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "*=", "MULSTOREP_VF",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_vector,	OPF_STOREPTR},

 {7, "/=", "DIVSTORE_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float,		OPF_STORE},
 {7, "/=", "DIVSTOREP_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float,	OPF_STOREPTR},

 {7, "+=", "ADDSTORE_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float,		OPF_STORE},
 {7, "+=", "ADDSTORE_V",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_vector, &type_vector, &type_vector,	OPF_STORE},
 {7, "+=", "ADDSTOREP_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "+=", "ADDSTOREP_V",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_vector, &type_vector,	OPF_STOREPTR},

 {7, "-=", "SUBSTORE_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float,		OPF_STORE},
 {7, "-=", "SUBSTORE_V",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_vector, &type_vector, &type_vector,	OPF_STORE},
 {7, "-=", "SUBSTOREP_F",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "-=", "SUBSTOREP_V",	PC_STORE, ASSOC_RIGHT_RESULT,				&type_pointer, &type_vector, &type_vector,	OPF_STOREPTR},

 {7, "<FETCH_GBL_F>", "FETCH_GBL_F",		PC_NONE, ASSOC_LEFT,	&type_float, &type_float, &type_float},
 {7, "<FETCH_GBL_V>", "FETCH_GBL_V",		PC_NONE, ASSOC_LEFT,	&type_vector, &type_float, &type_vector},
 {7, "<FETCH_GBL_S>", "FETCH_GBL_S",		PC_NONE, ASSOC_LEFT,	&type_string, &type_float, &type_string},
 {7, "<FETCH_GBL_E>", "FETCH_GBL_E",		PC_NONE, ASSOC_LEFT,	&type_entity, &type_float, &type_entity},
 {7, "<FETCH_GBL_FNC>", "FETCH_GBL_FNC",	PC_NONE, ASSOC_LEFT,	&type_function, &type_float, &type_function},

 {7, "<CSTATE>", "CSTATE",					PC_NONE, ASSOC_LEFT,	&type_float, &type_float, &type_void},

 {7, "<CWSTATE>", "CWSTATE",				PC_NONE, ASSOC_LEFT,	&type_float, &type_float, &type_void},

 {7, "<THINKTIME>", "THINKTIME",			PC_NONE, ASSOC_LEFT,	&type_entity, &type_float, &type_void},

 {7, "|=", "BITSETSTORE_F",					PC_STORE,	ASSOC_RIGHT,	&type_float, &type_float, &type_float,	OPF_STORE},
 {7, "|=", "BITSETSTOREP_F",				PC_STORE,	ASSOC_RIGHT,	&type_pointer, &type_float, &type_float,OPF_STOREPTR},
 {7, "&~=", "BITCLRSTORE_F",				PC_STORE,	ASSOC_RIGHT,	&type_float, &type_float, &type_float,	OPF_STORE},
 {7, "&~=", "BITCLRSTOREP_F",				PC_STORE,	ASSOC_RIGHT,	&type_pointer, &type_float, &type_float,OPF_STOREPTR},

 {7, "<RAND0>", "RAND0",					PC_NONE, ASSOC_LEFT,	&type_void, &type_void, &type_float},
 {7, "<RAND1>", "RAND1",					PC_NONE, ASSOC_LEFT,	&type_float, &type_void, &type_float},
 {7, "<RAND2>", "RAND2",					PC_NONE, ASSOC_LEFT,	&type_float, &type_float, &type_float},
 {7, "<RANDV0>", "RANDV0",					PC_NONE, ASSOC_LEFT,	&type_void, &type_void, &type_vector},
 {7, "<RANDV1>", "RANDV1",					PC_NONE, ASSOC_LEFT,	&type_vector, &type_void, &type_vector},
 {7, "<RANDV2>", "RANDV2",					PC_NONE, ASSOC_LEFT,	&type_vector, &type_vector, &type_vector},

 {7, "<SWITCH_F>", "SWITCH_F",				PC_NONE, ASSOC_RIGHT,	&type_float, NULL, &type_void},
 {7, "<SWITCH_V>", "SWITCH_V",				PC_NONE, ASSOC_RIGHT,	&type_vector, NULL, &type_void},
 {7, "<SWITCH_S>", "SWITCH_S",				PC_NONE, ASSOC_RIGHT,	&type_string, NULL, &type_void},
 {7, "<SWITCH_E>", "SWITCH_E",				PC_NONE, ASSOC_RIGHT,	&type_entity, NULL, &type_void},
 {7, "<SWITCH_FNC>", "SWITCH_FNC",			PC_NONE, ASSOC_RIGHT,	&type_function, NULL, &type_void},

 {7, "<CASE>", "CASE",						PC_NONE, ASSOC_RIGHT,	&type_variant, NULL, &type_void},
 {7, "<CASERANGE>", "CASERANGE",			PC_NONE, ASSOC_RIGHT,	&type_float, &type_float, NULL},


//Later are additions by DMW.

 {7, "<CALL1H>", "CALL1H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_void},
 {7, "<CALL2H>", "CALL2H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL3H>", "CALL3H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL4H>", "CALL4H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL5H>", "CALL5H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL6H>", "CALL6H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL7H>", "CALL7H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},
 {7, "<CALL8H>", "CALL8H",	PC_NONE, ASSOC_RIGHT,		&type_function, &type_variant, &type_variant},

 {7, "=",	"STORE_I",		PC_STORE, ASSOC_RIGHT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},
 {7, "=",	"STORE_IF",		PC_STORE, ASSOC_RIGHT,		&type_float, &type_integer, &type_integer,	OPF_STORE},
 {7, "=",	"STORE_FI",		PC_STORE, ASSOC_RIGHT,		&type_integer, &type_float, &type_float,	OPF_STORE},

 {7, "+", "ADD_I",			PC_ADDSUB, ASSOC_LEFT,		&type_integer, &type_integer, &type_integer,OPF_STD},
 {7, "+", "ADD_FI",			PC_ADDSUB, ASSOC_LEFT,		&type_float, &type_integer, &type_float,	OPF_STD},
 {7, "+", "ADD_IF",			PC_ADDSUB, ASSOC_LEFT,		&type_integer, &type_float, &type_float,	OPF_STD},

 {7, "-", "SUB_I",			PC_ADDSUB, ASSOC_LEFT,		&type_integer, &type_integer, &type_integer,OPF_STD},
 {7, "-", "SUB_FI",			PC_ADDSUB, ASSOC_LEFT,		&type_float, &type_integer, &type_float,	OPF_STD},
 {7, "-", "SUB_IF",			PC_ADDSUB, ASSOC_LEFT,		&type_integer, &type_float, &type_float,	OPF_STD},

 {7, "<CIF>", "CONV_IF",	PC_STORE, ASSOC_LEFT,		&type_integer, &type_void, &type_float},
 {7, "<CFI>", "CONV_FI",	PC_STORE, ASSOC_LEFT,		&type_float, &type_void, &type_integer},
 {7, "<CPIF>", "CONVP_IF",	PC_STORE, ASSOC_LEFT,		&type_pointer, &type_integer, &type_float},
 {7, "<CPFI>", "CONVP_FI",	PC_STORE, ASSOC_LEFT,		&type_pointer, &type_float, &type_integer},

 {7, ".", "LOADF_I",	PC_MEMBER, ASSOC_LEFT,			&type_entity,	&type_field, &type_integer},
 {7, "=", "STOREP_I",	PC_STORE, ASSOC_RIGHT,			&type_pointer,	&type_integer, &type_integer,	OPF_STOREPTROFS},
 {7, "=", "STOREP_IF",	PC_STORE, ASSOC_RIGHT,			&type_pointer,	&type_float, &type_integer,	OPF_STOREPTROFS},
 {7, "=", "STOREP_FI",	PC_STORE, ASSOC_RIGHT,			&type_pointer,	&type_integer, &type_float,	OPF_STOREPTROFS},

 {7, "&", "BITAND_I",	PC_BITAND, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer,OPF_STD},
 {7, "|", "BITOR_I",	PC_BITOR, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer,OPF_STD},

 {7, "*", "MUL_I",		PC_MULDIV, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer,OPF_STD},
 {7, "/", "DIV_I",		PC_MULDIV, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer,OPF_STD},
 {7, "==", "EQ_I",		PC_EQUALITY, ASSOC_LEFT,		&type_integer,	&type_integer, &type_bint	,OPF_STD},
 {7, "!=", "NE_I",		PC_EQUALITY, ASSOC_LEFT,		&type_integer,	&type_integer, &type_bint	,OPF_STD},

 {7, "<IFNOTS>", "IFNOTS",	PC_NONE, ASSOC_RIGHT,		&type_string,	NULL, &type_void},
 {7, "<IFS>", "IFS",		PC_NONE, ASSOC_RIGHT,		&type_string,	NULL, &type_void},

 {7, "!", "NOT_I",			PC_UNARY, ASSOC_LEFT,		&type_integer,	&type_void,		&type_bint,	OPF_STDUNARY},

 {7, "/", "DIV_VF",			PC_MULDIV, ASSOC_LEFT,		&type_vector,	&type_float, &type_vector,	OPF_STD},

 {7, "^", "BITXOR_I",		PC_BITXOR, ASSOC_LEFT,		&type_integer,	&type_integer, &type_integer,OPF_STD},
 {7, ">>", "RSHIFT_I",		PC_SHIFT, ASSOC_LEFT,		&type_integer,	&type_integer, &type_integer,OPF_STD},
 {7, "<<", "LSHIFT_I",		PC_SHIFT, ASSOC_LEFT,		&type_integer,	&type_integer, &type_integer,OPF_STD},

										//var,		offset			return
 {7, "<ARRAY>", "GLOBALADDRESS", PC_NONE, ASSOC_LEFT,	&type_float,		&type_integer, &type_pointer},
 {7, "<ARRAY>", "ADD_PIW", PC_NONE, ASSOC_LEFT,			&type_pointer,	&type_integer, &type_pointer},

 {7, "=", "LOADA_F",		PC_STORE, ASSOC_LEFT,		&type_float,	&type_integer, &type_float},
 {7, "=", "LOADA_V",		PC_STORE, ASSOC_LEFT,		&type_vector,	&type_integer, &type_vector},
 {7, "=", "LOADA_S",		PC_STORE, ASSOC_LEFT,		&type_string,	&type_integer, &type_string},
 {7, "=", "LOADA_ENT",		PC_STORE, ASSOC_LEFT,		&type_entity,	&type_integer, &type_entity},
 {7, "=", "LOADA_FLD",		PC_STORE, ASSOC_LEFT,		&type_field,	&type_integer, &type_field},
 {7, "=", "LOADA_FNC",		PC_STORE, ASSOC_LEFT,		&type_function,	&type_integer, &type_function},
 {7, "=", "LOADA_I",		PC_STORE, ASSOC_LEFT,		&type_integer,	&type_integer, &type_integer},

 {7, "=", "STORE_P",		PC_STORE, ASSOC_RIGHT,		&type_pointer,	&type_pointer, &type_void,	OPF_STORE},
 {7, ".", "LOADF_P",		PC_MEMBER, ASSOC_LEFT,		&type_entity,	&type_field, &type_pointer},

 {7, "=", "LOADP_F",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_float,	OPF_LOADPTR},
 {7, "=", "LOADP_V",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_vector,	OPF_LOADPTR},
 {7, "=", "LOADP_S",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_string,	OPF_LOADPTR},
 {7, "=", "LOADP_ENT",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_entity,	OPF_LOADPTR},
 {7, "=", "LOADP_FLD",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_field,	OPF_LOADPTR},
 {7, "=", "LOADP_FNC",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_function,	OPF_LOADPTR},
 {7, "=", "LOADP_I",		PC_STORE, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_integer,	OPF_LOADPTR},


 {7, "<=",	"LE_I",			PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_integer,	&type_bint,	OPF_STD},
 {7, ">=",	"GE_I",			PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_integer,	&type_bint,	OPF_STD},
 {7, "<",	"LT_I",			PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_integer,	&type_bint,	OPF_STD},
 {7, ">",	"GT_I",			PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_integer,	&type_bint,	OPF_STD},

 {7, "<=",	"LE_IF",		PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_float,	&type_bint,	OPF_STD},
 {7, ">=",	"GE_IF",		PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_float,	&type_bint,	OPF_STD},
 {7, "<",	"LT_IF",		PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_float,	&type_bint,	OPF_STD},
 {7, ">",	"GT_IF",		PC_RELATION, ASSOC_LEFT,	&type_integer,	&type_float,	&type_bint,	OPF_STD},

 {7, "<=",	"LE_FI",		PC_RELATION, ASSOC_LEFT,	&type_float,	&type_integer,	&type_bint,	OPF_STD},
 {7, ">=",	"GE_FI",		PC_RELATION, ASSOC_LEFT,	&type_float,	&type_integer,	&type_bint,	OPF_STD},
 {7, "<",	"LT_FI",		PC_RELATION, ASSOC_LEFT,	&type_float,	&type_integer,	&type_bint,	OPF_STD},
 {7, ">",	"GT_FI",		PC_RELATION, ASSOC_LEFT,	&type_float,	&type_integer,	&type_bint,	OPF_STD},

 {7, "==",	"EQ_IF",		PC_EQUALITY, ASSOC_LEFT,	&type_integer,	&type_float,	&type_bint,	OPF_STD},
 {7, "==",	"EQ_FI",		PC_EQUALITY, ASSOC_LEFT,	&type_float,	&type_integer,	&type_bint,	OPF_STD},

 	//-------------------------------------
	//string manipulation.
 {7, "+", "ADD_SF",	PC_ADDSUB, ASSOC_LEFT,				&type_string,	&type_float,	&type_string,	OPF_STD},
 {7, "-", "SUB_S",	PC_ADDSUB, ASSOC_LEFT,				&type_string,	&type_string,	&type_float,	OPF_STD},
 {7, "<STOREP_C>", "STOREP_C",	PC_STORE, ASSOC_RIGHT,	&type_string,	&type_float,	&type_float,	OPF_STOREPTROFS},
 {7, "<LOADP_C>", "LOADP_C",	PC_STORE, ASSOC_LEFT,	&type_string,	&type_float,	&type_float,	OPF_LOADPTR},
	//-------------------------------------



{7, "*", "MUL_IF",	PC_MULDIV, ASSOC_LEFT,				&type_integer,	&type_float,	&type_float,	OPF_STD},
{7, "*", "MUL_FI",	PC_MULDIV, ASSOC_LEFT,				&type_float,	&type_integer,	&type_float,	OPF_STD},
{7, "*", "MUL_VI",	PC_MULDIV, ASSOC_LEFT,				&type_vector,	&type_integer,	&type_vector,	OPF_STD},
{7, "*", "MUL_IV",	PC_MULDIV, ASSOC_LEFT,				&type_integer,	&type_vector,	&type_vector,	OPF_STD},

{7, "/", "DIV_IF",	PC_MULDIV, ASSOC_LEFT,				&type_integer,	&type_float,	&type_float,	OPF_STD},
{7, "/", "DIV_FI",	PC_MULDIV, ASSOC_LEFT,				&type_float,	&type_integer,	&type_float,	OPF_STD},

{7, "&", "BITAND_IF",	PC_BITAND, ASSOC_LEFT,			&type_integer,	&type_float,	&type_integer,	OPF_STD},
{7, "|", "BITOR_IF",	PC_BITOR, ASSOC_LEFT,			&type_integer,	&type_float,	&type_integer,	OPF_STD},
{7, "&", "BITAND_FI",	PC_BITAND, ASSOC_LEFT,			&type_float,	&type_integer,	&type_integer,	OPF_STD},
{7, "|", "BITOR_FI",	PC_BITOR, ASSOC_LEFT,			&type_float,	&type_integer,	&type_integer,	OPF_STD},

{7, "&&", "AND_I",	PC_LOGICAND, ASSOC_LEFT,			&type_integer,	&type_integer,	&type_bint,		OPF_STD},
{7, "||", "OR_I",	PC_LOGICOR, ASSOC_LEFT,				&type_integer,	&type_integer,	&type_bint,		OPF_STD},
{7, "&&", "AND_IF",	PC_LOGICAND, ASSOC_LEFT,			&type_integer,	&type_float,	&type_bint,		OPF_STD},
{7, "||", "OR_IF",	PC_LOGICOR, ASSOC_LEFT,				&type_integer,	&type_float,	&type_bint,		OPF_STD},
{7, "&&", "AND_FI",	PC_LOGICAND, ASSOC_LEFT,			&type_float,	&type_integer,	&type_bint,		OPF_STD},
{7, "||", "OR_FI",	PC_LOGICOR, ASSOC_LEFT,				&type_float,	&type_integer,	&type_bint,		OPF_STD},
{7, "!=", "NE_IF",	PC_EQUALITY, ASSOC_LEFT,			&type_integer,	&type_float,	&type_bint,		OPF_STD},
{7, "!=", "NE_FI",	PC_EQUALITY, ASSOC_LEFT,			&type_float,	&type_integer,	&type_bint,		OPF_STD},






{7, "<>",	"GSTOREP_I",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_F",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_ENT",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_FLD",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_S",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_FNC",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_V",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},

{7, "<>",	"GADDRESS",		PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},

{7, "<>",	"GLOAD_I",		PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_F",		PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_FLD",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_ENT",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_S",		PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_FNC",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},

{7, "<>",	"BOUNDCHECK",	PC_NONE, ASSOC_LEFT,		&type_integer,	NULL,	NULL},

{7, "<UNUSED>",	"UNUSED",	PC_NONE, ASSOC_RIGHT,		&type_void,	&type_void,	&type_void},
{7, "<PUSH>",	"PUSH",		PC_NONE, ASSOC_RIGHT,		&type_float,	&type_void,		&type_pointer},
{7, "<POP>",	"POP",		PC_NONE, ASSOC_RIGHT,		&type_float,	&type_void,		&type_void},

{7, "<SWITCH_I>", "SWITCH_I",PC_NONE, ASSOC_LEFT,		&type_void, NULL, &type_void},
{7, "<>",	"GLOAD_S",		PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_float},

{7, "<IF_F>",	"IF_F",		PC_NONE, ASSOC_RIGHT,		&type_float, NULL, &type_void},
{7, "<IFNOT_F>","IFNOT_F",	PC_NONE, ASSOC_RIGHT,		&type_float, NULL, &type_void},

{7, "=",	"STOREF_V",		PC_NONE,	ASSOC_RIGHT,			&type_entity, &type_field, &type_vector,		OPF_STOREFLD},	//ent.fld=c
{7, "=",	"STOREF_F",		PC_NONE,	ASSOC_RIGHT,			&type_entity, &type_field, &type_float,			OPF_STOREFLD},
{7, "=",	"STOREF_S",		PC_NONE,	ASSOC_RIGHT,			&type_entity, &type_field, &type_string,		OPF_STOREFLD},
{7, "=",	"STOREF_I",		PC_NONE,	ASSOC_RIGHT,			&type_entity, &type_field, &type_integer,		OPF_STOREFLD},

{7, "<STOREP_B>", "STOREP_I8",	PC_STORE, ASSOC_RIGHT,	&type_string,	&type_integer, &type_integer,			OPF_STOREPTROFS},
{7, "<LOADP_B>", "LOADP_U8",	PC_STORE, ASSOC_LEFT,	&type_string,	&type_integer, &type_integer,			OPF_LOADPTR},

//uint opcodes (not many, they're shared with ints for the most part)
{7, "<=",	"LE_U",			PC_RELATION, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint,				OPF_LOADPTR},
{7, "<",	"LT_U",			PC_RELATION, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint,				OPF_LOADPTR},
{7, "/",	"DIV_U",		PC_MULDIV, ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,				OPF_LOADPTR},
{7, ">>",	"RSHIFT_U",		PC_SHIFT, ASSOC_LEFT,		&type_uint,		&type_integer,	&type_uint,				OPF_LOADPTR},

//[u]int64+double opcodes
{7, "+",	"ADD_I64",		PC_ADDSUB,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "-",	"SUB_I64",		PC_ADDSUB,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "*",	"MUL_I64",		PC_MULDIV,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "/",	"DIV_I64",		PC_MULDIV,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "&",	"BITAND_I64",	PC_BITAND,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "|",	"BITOR_I64",	PC_BITOR,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "^",	"BITXOR_I64",	PC_BITXOR,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64,	OPF_STD},
{7, "<<",	"LSHIFT_I64I",	PC_SHIFT,	ASSOC_LEFT,		&type_int64,		&type_integer,		&type_int64,	OPF_STD},
{7, ">>",	"RSHIFT_I64I",	PC_SHIFT,	ASSOC_LEFT,		&type_int64,		&type_integer,		&type_int64,	OPF_STD},
{7, "<=",	"LE_I64",		PC_RELATION, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},
{7, "<",	"LT_I64",		PC_RELATION, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},
{7, "==",	"EQ_I64",		PC_EQUALITY, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},
{7, "!=",	"NE_I64",		PC_EQUALITY, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},
{7, "<",	"LE_U64",		PC_RELATION, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},
{7, "<",	"LT_U64",		PC_RELATION, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},
{7, "%",	"DIV_U64",		PC_MULDIV,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, ">>",	"RSHIFT_U64I",	PC_SHIFT,	ASSOC_LEFT,		&type_uint64,		&type_uint,			&type_uint64,	OPF_STD},
{7, "=",	"STORE_I64",	PC_STORE, ASSOC_RIGHT,		&type_int64,		&type_int64,		&type_int64,	OPF_STORE},
{7, "=",	"STOREP_I64",	PC_STORE, ASSOC_RIGHT,		&type_pointer,		&type_int64,		&type_int64,	OPF_STOREPTROFS},
{7, "<=>",	"STOREF_I64",	PC_NONE,	ASSOC_RIGHT,	&type_entity,		&type_field,		&type_int64,	OPF_STOREFLD},
{7, ".",	"LOADF_I64",	PC_MEMBER, ASSOC_LEFT,		&type_entity,		&type_field,		&type_pointer},
{7, "=",	"LOADA_I64",	PC_STORE, ASSOC_LEFT,		&type_int64,		&type_integer,		&type_int64},
{7, "=",	"LOADP_I64",	PC_STORE, ASSOC_LEFT,		&type_pointer,		&type_int64,		&type_int64,	OPF_LOADPTR},
{7, "=",	"CONV_UI64",	PC_STORE, ASSOC_LEFT,		&type_uint,			&type_void,			&type_int64},
{7, "=",	"CONV_II64",	PC_STORE, ASSOC_LEFT,		&type_integer,		&type_void,			&type_int64},
{7, "=",	"CONV_I64I",	PC_STORE, ASSOC_LEFT,		&type_int64,		&type_void,			&type_integer},
{7, "=",	"CONV_FD",		PC_STORE, ASSOC_LEFT,		&type_float,		&type_void,			&type_double},
{7, "=",	"CONV_DF",		PC_STORE, ASSOC_LEFT,		&type_double,		&type_void,			&type_float},
{7, "=",	"CONV_I64F",	PC_STORE, ASSOC_LEFT,		&type_int64,		&type_void,			&type_float},
{7, "=",	"CONV_FI64",	PC_STORE, ASSOC_LEFT,		&type_float,		&type_void,			&type_int64},
{7, "=",	"CONV_I64D",	PC_STORE, ASSOC_LEFT,		&type_int64,		&type_void,			&type_double},
{7, "=",	"CONV_DI64",	PC_STORE, ASSOC_LEFT,		&type_double,		&type_void,			&type_int64},
{7, "+",	"ADD_D",		PC_ADDSUB,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "-",	"SUB_D",		PC_ADDSUB,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "*",	"MUL_D",		PC_MULDIV,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "/",	"DIV_D",		PC_MULDIV,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "<=",	"LE_D",			PC_RELATION, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},
{7, "<",	"LT_D",			PC_RELATION, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},
{7, "==",	"EQ_D",			PC_EQUALITY, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},
{7, "!=",	"NE_D",			PC_EQUALITY, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},

{7, "<STOREP_I16>", "STOREP_I16",	PC_STORE, ASSOC_RIGHT,	&type_pointer,	&type_integer, &type_integer,			OPF_STOREPTROFS},
{7, "<LOADP_I16>", "LOADP_I16",		PC_STORE, ASSOC_LEFT,	&type_pointer,	&type_integer, &type_integer,			OPF_LOADPTR},
{7, "<LOADP_U16>", "LOADP_U16",		PC_STORE, ASSOC_LEFT,	&type_pointer,	&type_integer, &type_integer,			OPF_LOADPTR},
{7, "<LOADP_I8>", "LOADP_I8",		PC_STORE, ASSOC_LEFT,	&type_pointer,	&type_integer, &type_integer,			OPF_LOADPTR},
{7, "<BITEXTEND_I>", "BITEXTEND_I",	PC_NONE, ASSOC_LEFT,	&type_integer,	&type_integer, &type_integer,			OPF_STD},
{7, "<BITEXTEND_U>", "BITEXTEND_U",	PC_NONE, ASSOC_LEFT,	&type_integer,	&type_integer, &type_integer,			OPF_STD},
{7, "<BITCOPY_I>", "BITCOPY_I",	PC_NONE, ASSOC_LEFT,	&type_integer,	&type_integer, &type_integer,			OPF_STD},
{7, "=",	"CONV_UF",		PC_STORE, ASSOC_LEFT,		&type_uint,		&type_void,				&type_float},
{7, "=",	"CONV_FU",		PC_STORE, ASSOC_LEFT,		&type_float,		&type_void,			&type_uint},
{7, "=",	"CONV_U64D",		PC_STORE, ASSOC_LEFT,		&type_uint64,		&type_void,			&type_double},
{7, "=",	"CONV_DU64",		PC_STORE, ASSOC_LEFT,		&type_double,		&type_void,			&type_uint64},
{7, "=",	"CONV_U64F",		PC_STORE, ASSOC_LEFT,		&type_uint64,		&type_void,			&type_float},
{7, "=",	"CONV_FU64",		PC_STORE, ASSOC_LEFT,		&type_float,		&type_void,			&type_uint64},

/* emulated ops begin here */
 {7, "<>",	"OP_EMULATED",	PC_NONE, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},

 {7, "|=", "BITSET_I",		PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},
 {7, "|=", "BITSETP_I",		PC_STORE,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer,	OPF_STOREPTR},
 {7, "&~=", "BITCLR_I",		PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},


 {7, "*=", "MULSTORE_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},
 {7, "/=", "DIVSTORE_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},
 {7, "+=", "ADDSTORE_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},
 {7, "-=", "SUBSTORE_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer,	OPF_STORE},

 {7, "*=", "MULSTOREP_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer,	OPF_STOREPTR},
 {7, "/=", "DIVSTOREP_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer,	OPF_STOREPTR},
 {7, "+=", "ADDSTOREP_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer,	OPF_STOREPTR},
 {7, "-=", "SUBSTOREP_I",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer,	OPF_STOREPTR},

 {7, "*=", "MULSTORE_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float,	OPF_STORE},
 {7, "*=", "MULSTOREP_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "/=", "DIVSTORE_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float,	OPF_STORE},
 {7, "/=", "DIVSTOREP_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "+=", "ADDSTORE_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float,	OPF_STORE},
 {7, "+=", "ADDSTOREP_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float,	OPF_STOREPTR},
 {7, "-=", "SUBSTORE_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float,	OPF_STORE},
 {7, "-=", "SUBSTOREP_IF",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float,	OPF_STOREPTR},

 {7, "*=", "MULSTORE_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float,	OPF_STORE},
 {7, "*=", "MULSTOREP_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float,	OPF_STOREPTR},
 {7, "/=", "DIVSTORE_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float,	OPF_STORE},
 {7, "/=", "DIVSTOREP_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float,	OPF_STOREPTR},
 {7, "+=", "ADDSTORE_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float,	OPF_STORE},
 {7, "+=", "ADDSTOREP_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float,	OPF_STOREPTR},
 {7, "-=", "SUBSTORE_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float,	OPF_STORE},
 {7, "-=", "SUBSTOREP_FI",	PC_STORE,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float,	OPF_STOREPTR},

 {7, "*=", "MULSTORE_VI",	PC_STORE, ASSOC_RIGHT_RESULT,		&type_vector, &type_integer, &type_vector,	OPF_STORE},
 {7, "*=", "MULSTOREP_VI",	PC_STORE, ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_vector,	OPF_STOREPTR},

 {7, "=", "LOADA_STRUCT",	PC_STORE, ASSOC_LEFT,				&type_float,	&type_integer, &type_float},

 {7, "=",	"LOADP_P",		PC_STORE, ASSOC_LEFT,				&type_pointer,	&type_integer, &type_pointer,	OPF_LOADPTR},
 {7, "=",	"STOREP_P",		PC_STORE,	ASSOC_RIGHT,			&type_pointer,	&type_pointer, &type_pointer,	OPF_STOREPTROFS},
 {7, "~",	"BITNOT_F",		PC_UNARY, ASSOC_LEFT,				&type_float,	&type_void, &type_float,		OPF_STDUNARY},
 {7, "~",	"BITNOT_I",		PC_UNARY, ASSOC_LEFT,				&type_integer,	&type_void, &type_integer,		OPF_STDUNARY},

 {7, "==", "EQ_P",			PC_EQUALITY, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},
 {7, "!=", "NE_P",			PC_EQUALITY, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},
 {7, "<=", "LE_P",			PC_RELATION, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},
 {7, ">=", "GE_P",			PC_RELATION, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},
 {7, "<", "LT_P",			PC_RELATION, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},
 {7, ">", "GT_P",			PC_RELATION, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_bfloat, OPF_STD},

 {7, "&=", "ANDSTORE_F",	PC_STORE, ASSOC_RIGHT_RESULT,		&type_float, &type_float, &type_float, OPF_STORE},
 {7, "&~=", "BITCLR_F",		PC_STORE, ASSOC_LEFT,					&type_float, &type_float, &type_float, OPF_STORE},
 {7, "&~=", "BITCLR_I",		PC_STORE, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer, OPF_STORE},
 {7, "&~=", "BITCLR_V",		PC_STORE, ASSOC_LEFT,					&type_vector, &type_vector, &type_vector, OPF_STORE},

 {7, "+", "ADD_SI",	PC_ADDSUB, ASSOC_LEFT,						&type_string,	&type_integer,	&type_string,	OPF_STD},
 {7, "+", "ADD_IS",	PC_ADDSUB, ASSOC_LEFT,						&type_integer,	&type_string,	&type_string,	OPF_STD},
 {7, "+", "ADD_PF",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_float,	&type_pointer,	OPF_STD},//var_a.cast->auxtype matters
 {7, "+", "ADD_FP",	PC_ADDSUB, ASSOC_LEFT,						&type_float,	&type_pointer,	&type_pointer,	OPF_STD},//var_b.cast->auxtype matters
 {7, "+", "ADD_PI",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_integer,	&type_pointer,	OPF_STD},//var_a.cast->auxtype matters
 {7, "+", "ADD_IP",	PC_ADDSUB, ASSOC_LEFT,						&type_integer,	&type_pointer,	&type_pointer,	OPF_STD},//var_b.cast->auxtype matters
 {7, "+", "ADD_PU",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_uint,		&type_pointer,	OPF_STD},//var_a.cast->auxtype matters
 {7, "+", "ADD_UP",	PC_ADDSUB, ASSOC_LEFT,						&type_uint,		&type_pointer,	&type_pointer,	OPF_STD},//var_b.cast->auxtype matters
 {7, "-", "SUB_SI",	PC_ADDSUB, ASSOC_LEFT,						&type_string,	&type_integer,	&type_string,	OPF_STD},
 {7, "-", "SUB_PF",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_float,	&type_pointer,	OPF_STD},
 {7, "-", "SUB_PI",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_integer,	&type_pointer,	OPF_STD},
 {7, "-", "SUB_PU",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_uint,		&type_pointer,	OPF_STD},
 {7, "-", "SUB_PP",	PC_ADDSUB, ASSOC_LEFT,						&type_pointer,	&type_pointer,	&type_integer,	OPF_STD},

 {7, "%", "MOD_F",	PC_MULDIV, ASSOC_LEFT,						&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, "%", "MOD_I",	PC_MULDIV, ASSOC_LEFT,						&type_integer,	&type_integer,	&type_integer,	OPF_STD},
 {7, "%", "MOD_FI",	PC_MULDIV, ASSOC_LEFT,						&type_float,	&type_integer,	&type_integer,	OPF_STD},
 {7, "%", "MOD_IF",	PC_MULDIV, ASSOC_LEFT,						&type_integer,	&type_float,	&type_integer,	OPF_STD},
 {7, "%", "MOD_V",	PC_MULDIV, ASSOC_LEFT,						&type_vector,	&type_vector,	&type_vector,	OPF_STD},

 {7, "^", "BITXOR_F",	PC_BITXOR, ASSOC_LEFT,					&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, ">>", "RSHIFT_F",	PC_SHIFT, ASSOC_LEFT,					&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, "<<", "LSHIFT_F",	PC_SHIFT, ASSOC_LEFT,					&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, ">>", "RSHIFT_IF",	PC_SHIFT, ASSOC_LEFT,					&type_integer,	&type_float,	&type_integer,	OPF_STD},
 {7, "<<", "LSHIFT_IF",	PC_SHIFT, ASSOC_LEFT,					&type_integer,	&type_float,	&type_integer,	OPF_STD},
 {7, ">>", "RSHIFT_FI",	PC_SHIFT, ASSOC_LEFT,					&type_float,	&type_integer,	&type_integer,	OPF_STD},
 {7, "<<", "LSHIFT_FI",	PC_SHIFT, ASSOC_LEFT,					&type_float,	&type_integer,	&type_integer,	OPF_STD},

 {7, "&&", "AND_ANY",	PC_LOGICAND, ASSOC_LEFT,					&type_variant,	&type_variant,	&type_bfloat,	OPF_STD},
 {7, "||", "OR_ANY",	PC_LOGICOR, ASSOC_LEFT,					&type_variant,	&type_variant,	&type_bfloat,	OPF_STD},

 {7, "+", "ADD_EI",		PC_ADDSUB, ASSOC_LEFT,						&type_entity,	&type_integer,	&type_entity,	OPF_STD},
 {7, "+", "ADD_EF",		PC_ADDSUB, ASSOC_LEFT,						&type_entity,	&type_float,	&type_entity,	OPF_STD},
 {7, "-", "SUB_EI",		PC_ADDSUB, ASSOC_LEFT,						&type_entity,	&type_integer,	&type_entity,	OPF_STD},
 {7, "-", "SUB_EF",		PC_ADDSUB, ASSOC_LEFT,						&type_entity,	&type_float,	&type_entity,	OPF_STD},

 {7, "&", "BITAND_V",	PC_BITAND, ASSOC_LEFT,					&type_vector,	&type_vector,	&type_vector,	OPF_STD},
 {7, "|", "BITOR_V",	PC_BITOR, ASSOC_LEFT,						&type_vector,	&type_vector,	&type_vector,	OPF_STD},
 {7, "~", "BITNOT_V",	PC_UNARY, ASSOC_LEFT,					&type_vector,	&type_void,		&type_vector,	OPF_STD},
 {7, "^", "BITXOR_V",	PC_BITXOR, ASSOC_LEFT,					&type_vector,	&type_vector,	&type_vector,	OPF_STD},

 {7, "*^", "POW_F",		PC_MULDIV, ASSOC_LEFT,					&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, "*^", "POW_I",		PC_MULDIV, ASSOC_LEFT,					&type_integer,	&type_integer,	&type_integer,	OPF_STD},
 {7, "*^", "POW_FI",	PC_MULDIV, ASSOC_LEFT,					&type_float,	&type_integer,	&type_float,	OPF_STD},
 {7, "*^", "POW_IF",	PC_MULDIV, ASSOC_LEFT,					&type_integer,	&type_float,	&type_float,	OPF_STD},
 {7, "><", "CROSS_V",	PC_MULDIV, ASSOC_LEFT,					&type_vector,	&type_vector,	&type_vector,	OPF_STD},

 {7, "==", "EQ_FLD",	PC_EQUALITY, ASSOC_LEFT,				&type_field,	&type_field,	&type_bfloat,	OPF_STD},
 {7, "!=", "NE_FLD",	PC_EQUALITY, ASSOC_LEFT,				&type_field,	&type_field,	&type_bfloat,	OPF_STD},

 {7, "<=>", "SPACESHIP_F",	PC_EQUALITY, ASSOC_LEFT,				&type_float,	&type_float,	&type_float,	OPF_STD},
 {7, "<=>", "SPACESHIP_S",	PC_EQUALITY, ASSOC_LEFT,				&type_string,	&type_string,	&type_float,	OPF_STD},

 	//uint32 opcodes. they match the int32 ones so emulation is basically swapping them over.
{7, "+",	"ADD_U",		PC_ADDSUB,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "-",	"SUB_U",		PC_ADDSUB,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "*",	"MUL_U",		PC_MULDIV,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "%",	"MOD_U",		PC_MULDIV,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "&",	"BITAND_U",		PC_BITAND,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "|",	"BITOR_U",		PC_BITOR,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "^",	"BITXOR_U",		PC_BITXOR,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, "~",	"BITNOT_U",		PC_UNARY,	ASSOC_LEFT,		&type_uint,		&type_void,		&type_uint,	OPF_STDUNARY},
{7, "&~=",	"BITCLR_U",		PC_STORE,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STORE},
{7, "<<",	"LSHIFT_U",		PC_SHIFT,	ASSOC_LEFT,		&type_uint,		&type_uint,		&type_uint,	OPF_STD},
{7, ">=",	"GE_U",			PC_RELATION, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint,	OPF_STD},
{7, ">",	"GT_U",			PC_RELATION, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint,	OPF_STD},
{7, "==",	"EQ_U",			PC_EQUALITY, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint, OPF_STD},
{7, "!=",	"NE_U",			PC_EQUALITY, ASSOC_LEFT,	&type_uint,		&type_uint,		&type_bint, OPF_STD},

//64bit ones that we can emulate cheaply or are rare.
{7, "~",	"BITNOT_I64",	PC_UNARY,	ASSOC_LEFT,		&type_int64,		&type_void,			&type_int64,	OPF_STDUNARY},
{7, "&~=",	"BITCLR_I64",	PC_STORE,	ASSOC_LEFT,		&type_int64,		&type_int64,		&type_int64, OPF_STORE},
{7, ">=",	"GE_I64",		PC_RELATION, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},
{7, ">",	"GT_I64",		PC_RELATION, ASSOC_LEFT,	&type_int64,		&type_int64,		&type_bint,		OPF_STD},

//unsigned versions (emulated via signed int64 ones, just with different types).
{7, "+",	"ADD_U64",		PC_ADDSUB,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "-",	"SUB_U64",		PC_ADDSUB,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "*",	"MUL_U64",		PC_MULDIV,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "%",	"MOD_U64",		PC_MULDIV,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "&",	"BITAND_U64",	PC_BITAND,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "|",	"BITOR_U64",	PC_BITOR,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "^",	"BITXOR_U64",	PC_BITXOR,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STD},
{7, "~",	"BITNOT_U64",	PC_UNARY,	ASSOC_LEFT,		&type_uint64,		&type_void,			&type_uint64,	OPF_STDUNARY},
{7, "&~=",	"BITCLR_U64",	PC_STORE,	ASSOC_LEFT,		&type_uint64,		&type_uint64,		&type_uint64,	OPF_STORE},
{7, "<<",	"LSHIFT_U64I",	PC_SHIFT,	ASSOC_LEFT,		&type_uint64,		&type_integer,		&type_uint64,	OPF_STD},
{7, ">=",	"GE_U64",		PC_RELATION, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},
{7, ">",	"GT_U64",		PC_RELATION, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},
{7, "==",	"EQ_U64",		PC_EQUALITY, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},
{7, "!=",	"NE_U64",		PC_EQUALITY, ASSOC_LEFT,	&type_uint64,		&type_uint64,		&type_bint,		OPF_STD},


{7, "&",	"BITAND_D",		PC_BITAND,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "|",	"BITOR_D",		PC_BITOR,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "^",	"BITXOR_D",		PC_BITXOR,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STD},
{7, "~",	"BITNOT_D",		PC_UNARY,	ASSOC_LEFT,		&type_double,		&type_void,			&type_bint,		OPF_STDUNARY},
{7, "&~=",	"BITCLR_D",		PC_STORE,	ASSOC_LEFT,		&type_double,		&type_double,		&type_double,	OPF_STORE},
{7, "<<",	"LSHIFT_DI",	PC_SHIFT,	ASSOC_LEFT,		&type_double,		&type_integer,		&type_double,	OPF_STD},
{7, ">>",	"RSHIFT_DI",	PC_SHIFT,	ASSOC_LEFT,		&type_double,		&type_integer,		&type_double,	OPF_STD},
{7, ">=",	"GE_D",			PC_RELATION, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},
{7, ">",	"GT_D",			PC_RELATION, ASSOC_LEFT,	&type_double,		&type_double,		&type_bint,		OPF_STD},

{7, "<WSTATE>", "WSTATE",	PC_NONE, ASSOC_LEFT,		&type_float,	&type_float,	&type_void},

 {0, NULL, "OPD_GOTO_FORSTART"},
 {0, NULL, "OPD_GOTO_WHILE1"},
 {0, NULL, "OPD_GOTO_BREAK"},
 {0, NULL, "OPD_GOTO_DEFAULT"},

 {0, NULL}
};


static pbool OpAssignsToC(unsigned int op)
{
	// calls, switches and cases DON'T
	if(pr_opcodes[op].type_c == &type_void)
		return false;
	if(op >= OP_SWITCH_F && op <= OP_CALL8H)
		return false;
//	if(op >= OP_RAND0 && op <= OP_RANDV2)
//		return false;
	// they use a and b, but have 3 types
	// safety
	if(op >= OP_BITSETSTORE_F && op <= OP_BITCLRSTOREP_F)
		return false;
	/*if(op >= OP_STORE_I && op <= OP_STORE_FI)
	  return false; <- add STOREP_*?*/
	if(op == OP_STOREP_C || op == OP_STOREP_I8 || op == OP_STOREP_I16)
		return false;
	if((op >= OP_STORE_F && op <= OP_STOREP_FNC) || op == OP_STOREP_P || op == OP_STORE_P || op == OP_STORE_I64)
		return false;
	if(op >= OP_MULSTORE_F && op <= OP_SUBSTOREP_V)
		return false;
	if (op >= OP_STORE_I && op <= OP_STORE_FI)
		return false;
	if ((op >= OP_STOREF_V && op <= OP_STOREF_I) || op == OP_STOREF_I64)
		return false;	//reads it, doesn't write.
	if (op == OP_BOUNDCHECK || op == OP_UNUSED || op == OP_POP)
		return false;
	return true;
}
static pbool OpAssignsToB(unsigned int op)
{
	if(op >= OP_BITSETSTORE_F && op <= OP_BITCLRSTOREP_F)
		return true;
	if(op >= OP_STORE_I && op <= OP_STORE_FI)
		return true;
	if(op == OP_STOREP_C || op == OP_STOREP_I8 || op == OP_STOREP_I16)
		return true;
	if(op >= OP_MULSTORE_F && op <= OP_SUBSTOREP_V)
		return true;
	if((op >= OP_STORE_F && op <= OP_STOREP_FNC) || op == OP_STOREP_P || op == OP_STORE_P || op == OP_STORE_I64)
		return true;
	return false;
}
#define OpAssignsToA(op) false
#ifdef _DEBUG
static int OpAssignsCount(unsigned int op)
{
	switch(op)
	{
	case OP_DONE:
	case OP_RETURN:
		return 0;	//eep
	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
	case OP_CALL8:
	case OP_CALL1H:
	case OP_CALL2H:
	case OP_CALL3H:
	case OP_CALL4H:
	case OP_CALL5H:
	case OP_CALL6H:
	case OP_CALL7H:
	case OP_CALL8H:
		return 0;	//also, eep.
	case OP_STATE:
	case OP_WSTATE:
	case OP_CSTATE:
	case OP_CWSTATE:
	case OP_THINKTIME:
		return 0;	//egads
	case OP_RAND0:
	case OP_RAND1:
	case OP_RAND2:
	case OP_RANDV0:
	case OP_RANDV1:
	case OP_RANDV2:
		return 1;	//writes C, even when there's no A or B arg specified.
	case OP_UNUSED:
	case OP_POP:
		return 0;	//FIXME
	//branches have no side effects, other than the next instruction (or runaway loop)
	case OP_SWITCH_F:
	case OP_SWITCH_V:
	case OP_SWITCH_S:
	case OP_SWITCH_E:
	case OP_SWITCH_FNC:
	case OP_SWITCH_I:
	case OP_GOTO:
	case OP_IF_I:
	case OP_IFNOT_I:
	case OP_IF_S:
	case OP_IFNOT_S:
	case OP_IF_F:
	case OP_IFNOT_F:
	case OP_CASE:
	case OP_CASERANGE:
		return 0;
	case OP_STOREF_V:
	case OP_STOREF_F:
	case OP_STOREF_S:
	case OP_STOREF_I:
	case OP_STOREF_I64:
		return 0;	//stores to a.b rather than any direct value...
	case OP_BOUNDCHECK:
		return 0;
	default:	//the majority will write c
		return 1;
	}
}
static void OpAssignsTo_Debug(void)
{
	int i;
	for (i = 0; i < OP_NUMREALOPS; i++)
	{
		if (OpAssignsToA(i) + OpAssignsToB(i) + OpAssignsToC(i) != OpAssignsCount(i))
		{
			//we don't know what it assigns to. bug.
			QCC_PR_ParseError(0, "opcode %s metadata is bugged", pr_opcodes[i].opname);
		}
	}
}
#endif
/*pbool OpAssignedTo(QCC_def_t *v, unsigned int op)
{
	if(OpAssignsToC(op))
	{
	}
	else if(OpAssignsToB(op))
	{
	}
	return false;
}
*/
#undef ASSOC_RIGHT_RESULT

//#define TERM_PRIORITY		0
#define FUNC_PRIORITY		1
#define UNARY_PRIORITY		1	//~ !
//#define MULDIV_PRIORITY		priority_class[PC_MULDIV]	//* / %
//#define ADDSUB_PRIORITY		priority_class[PC_ADDSUB]	//+ -
//#define BITSHIFT_PRIORITY	priority_class[PC_BITSHIFT]	//<< >>
//#define COMPARISON_PRIORITY	priority_class[PC_COMPARISON]	//< <= > >=
//#define EQUALITY_PRIORITY	priority_class[PC_EQUALITY]	//== !=
//#define BITAND_PRIORITY		priority_class[PC_BITAND]	//&
//#define BITXOR_PRIORITY		priority_class[PC_BITXOR]	//^
//#define BITOR_PRIORITY		priority_class[PC_BITOR]	//|
//#define LOGICAND_PRIORITY	priority_class[PC_LOGICAND]	//&&
//#define LOGICOR_PRIORITY	priority_class[PC_LOGICOR]	//||
#define TERNARY_PRIORITY	priority_class[PC_TERNARY]	//?: (in C: (a||b)?(c):(d||e) )
#define	ASSIGN_PRIORITY		priority_class[PC_STORE]	//QC is WRONG compared to C
//#define COMMA_PRIORITY

#define	TOP_PRIORITY		priority_class[MAX_PRIORITY_CLASSES]
#define	NOT_PRIORITY		priority_class[PC_UNARYNOT]

QCC_opcode_t *opcodes_store[] =
{
	NULL
};
QCC_opcode_t *opcodes_addstore[] =
{
	&pr_opcodes[OP_ADD_F],
	&pr_opcodes[OP_ADD_V],
	&pr_opcodes[OP_ADD_I],
	&pr_opcodes[OP_ADD_U],
	&pr_opcodes[OP_ADD_I64],
	&pr_opcodes[OP_ADD_U64],
	&pr_opcodes[OP_ADD_D],
	&pr_opcodes[OP_ADD_FI],
	&pr_opcodes[OP_ADD_IF],
	&pr_opcodes[OP_ADD_SF],
	&pr_opcodes[OP_ADD_PI],
	&pr_opcodes[OP_ADD_IP],
	&pr_opcodes[OP_ADD_PU],
	&pr_opcodes[OP_ADD_UP],
	&pr_opcodes[OP_ADD_PF],
	&pr_opcodes[OP_ADD_FP],
	&pr_opcodes[OP_ADD_SI],
	&pr_opcodes[OP_ADD_IS],
	&pr_opcodes[OP_ADD_EI],
	&pr_opcodes[OP_ADD_EF],
	NULL
};
QCC_opcode_t *opcodes_addstorep[] =
{
	&pr_opcodes[OP_ADDSTOREP_F],
	&pr_opcodes[OP_ADDSTOREP_V],
	&pr_opcodes[OP_ADDSTOREP_I],
	&pr_opcodes[OP_ADDSTOREP_IF],
	&pr_opcodes[OP_ADDSTOREP_FI],
	NULL
};
QCC_opcode_t *opcodes_substore[] =
{
	&pr_opcodes[OP_SUB_F],
	&pr_opcodes[OP_SUB_V],
	&pr_opcodes[OP_SUB_I],
	&pr_opcodes[OP_SUB_U],
	&pr_opcodes[OP_SUB_FI],
	&pr_opcodes[OP_SUB_IF],
	&pr_opcodes[OP_SUB_S],
	&pr_opcodes[OP_SUB_PP],
	&pr_opcodes[OP_SUB_PI],
	&pr_opcodes[OP_SUB_PU],
	&pr_opcodes[OP_SUB_PF],
	&pr_opcodes[OP_SUB_SI],
	&pr_opcodes[OP_SUB_EI],
	&pr_opcodes[OP_SUB_EF],
	&pr_opcodes[OP_SUB_I64],
	&pr_opcodes[OP_SUB_U64],
	&pr_opcodes[OP_SUB_D],
	NULL
};
QCC_opcode_t *opcodes_substorep[] =
{
	&pr_opcodes[OP_SUBSTOREP_F],
	&pr_opcodes[OP_SUBSTOREP_V],
	&pr_opcodes[OP_SUBSTOREP_I],
	&pr_opcodes[OP_SUBSTOREP_IF],
	&pr_opcodes[OP_SUBSTOREP_FI],
	NULL
};
QCC_opcode_t *opcodes_mulstore[] =
{
	&pr_opcodes[OP_MUL_F],
	&pr_opcodes[OP_MUL_V],
	&pr_opcodes[OP_MUL_FV],
	&pr_opcodes[OP_MUL_IV],
	&pr_opcodes[OP_MUL_VF],
	&pr_opcodes[OP_MUL_VI],
	&pr_opcodes[OP_MUL_I],
	&pr_opcodes[OP_MUL_U],
	&pr_opcodes[OP_MUL_FI],
	&pr_opcodes[OP_MUL_IF],
	&pr_opcodes[OP_MUL_I64],
	&pr_opcodes[OP_MUL_U64],
	&pr_opcodes[OP_MUL_D],
	NULL
};
QCC_opcode_t *opcodes_mulstorep[] =
{
	&pr_opcodes[OP_MULSTOREP_F],
	&pr_opcodes[OP_MULSTOREP_VF],
	&pr_opcodes[OP_MULSTOREP_VI],
	&pr_opcodes[OP_MULSTOREP_I],
	&pr_opcodes[OP_MULSTOREP_IF],
	&pr_opcodes[OP_MULSTOREP_FI],
	NULL
};
QCC_opcode_t *opcodes_divstore[] =
{
	&pr_opcodes[OP_DIV_F],
	&pr_opcodes[OP_DIV_I],
	&pr_opcodes[OP_DIV_U],
	&pr_opcodes[OP_DIV_FI],
	&pr_opcodes[OP_DIV_IF],
	&pr_opcodes[OP_DIV_VF],
	&pr_opcodes[OP_DIV_I64],
	&pr_opcodes[OP_DIV_U64],
	&pr_opcodes[OP_DIV_D],
	NULL
};
QCC_opcode_t *opcodes_divstorep[] =
{
	&pr_opcodes[OP_DIVSTOREP_F],
	NULL
};
QCC_opcode_t *opcodes_orstore[] =
{
	&pr_opcodes[OP_BITOR_F],
	&pr_opcodes[OP_BITOR_I],
	&pr_opcodes[OP_BITOR_U],
	&pr_opcodes[OP_BITOR_IF],
	&pr_opcodes[OP_BITOR_FI],
	&pr_opcodes[OP_BITOR_V],
	&pr_opcodes[OP_BITOR_I64],
	&pr_opcodes[OP_BITOR_U64],
	&pr_opcodes[OP_BITOR_D],
	NULL
};
QCC_opcode_t *opcodes_orstorep[] =
{
	&pr_opcodes[OP_BITSETSTOREP_F],
	NULL
};
QCC_opcode_t *opcodes_xorstore[] =
{
	&pr_opcodes[OP_BITXOR_I],
	&pr_opcodes[OP_BITXOR_U],
	&pr_opcodes[OP_BITXOR_F],
	&pr_opcodes[OP_BITXOR_V],
	&pr_opcodes[OP_BITXOR_I64],
	&pr_opcodes[OP_BITXOR_U64],
	&pr_opcodes[OP_BITXOR_D],
	NULL
};
QCC_opcode_t *opcodes_andstore[] =
{
	&pr_opcodes[OP_BITAND_F],
	&pr_opcodes[OP_BITAND_I],
	&pr_opcodes[OP_BITAND_U],
	&pr_opcodes[OP_BITAND_IF],
	&pr_opcodes[OP_BITAND_FI],
	&pr_opcodes[OP_BITAND_V],
	&pr_opcodes[OP_BITAND_I64],
	&pr_opcodes[OP_BITAND_U64],
	&pr_opcodes[OP_BITAND_D],
	NULL
};
QCC_opcode_t *opcodes_clearstore[] =
{
	&pr_opcodes[OP_BITCLR_F],
	&pr_opcodes[OP_BITCLR_I],
	&pr_opcodes[OP_BITCLR_U],
	&pr_opcodes[OP_BITCLR_V],
	&pr_opcodes[OP_BITCLR_I64],
	&pr_opcodes[OP_BITCLR_U64],
	&pr_opcodes[OP_BITCLR_D],
	NULL
};
QCC_opcode_t *opcodes_clearstorep[] =
{
	&pr_opcodes[OP_BITCLRSTOREP_F],
	NULL
};
QCC_opcode_t *opcodes_shlstore[] =
{
	&pr_opcodes[OP_LSHIFT_F],
	&pr_opcodes[OP_LSHIFT_I],
	&pr_opcodes[OP_LSHIFT_U],
	&pr_opcodes[OP_LSHIFT_IF],
	&pr_opcodes[OP_LSHIFT_FI],
	&pr_opcodes[OP_LSHIFT_I64I],
	&pr_opcodes[OP_LSHIFT_U64I],
	&pr_opcodes[OP_LSHIFT_DI],
	NULL
};
QCC_opcode_t *opcodes_shrstore[] =
{
	&pr_opcodes[OP_RSHIFT_F],
	&pr_opcodes[OP_RSHIFT_I],
	&pr_opcodes[OP_RSHIFT_U],
	&pr_opcodes[OP_RSHIFT_IF],
	&pr_opcodes[OP_RSHIFT_FI],
	&pr_opcodes[OP_RSHIFT_I64I],
	&pr_opcodes[OP_RSHIFT_U64I],
	&pr_opcodes[OP_RSHIFT_DI],
	NULL
};

QCC_opcode_t *opcodes_spaceship[] =
{
	&pr_opcodes[OP_SPACESHIP_F],
	&pr_opcodes[OP_SPACESHIP_S],
//	&pr_opcodes[OP_SPACESHIP_I],
//	&pr_opcodes[OP_SPACESHIP_U],
//	&pr_opcodes[OP_SPACESHIP_I64],
//	&pr_opcodes[OP_SPACESHIP_U64],
//	&pr_opcodes[OP_SPACESHIP_D],
	NULL
};

QCC_opcode_t *opcodes_none[] =
{
	NULL
};

//these evaluate as top first.
static QCC_opcode_t *opcodeprioritized[13+1][128];
static int
#ifdef _MSC_VER
__cdecl
#endif
sort_opcodenames(const void*a,const void*b)
{
	const QCC_opcode_t *opa = *(QCC_opcode_t*const*)a;
	const QCC_opcode_t *opb = *(QCC_opcode_t*const*)b;
	if (opa == NULL)
		return opb?1:0;
	if (opb == NULL)
		return -1;
	return strcmp(opa->name, opb->name);
}
void QCC_PrioritiseOpcodes(void)
{
	int pcount[MAX_PRIORITY_CLASSES];
	int i, j;
	QCC_opcode_t *op;

	priority_class[PC_NONE] = 0;
	priority_class[PC_UNARY] = 0;
	priority_class[PC_MEMBER] = 0;

	if (flag_cpriority)
	{
		priority_class[PC_UNARYNOT] = 1;
		priority_class[PC_MULDIV] = 2;
		priority_class[PC_ADDSUB] = 3;
		priority_class[PC_SHIFT] = 4;
		priority_class[PC_RELATION] = 5;
		priority_class[PC_EQUALITY] = 6;
		priority_class[PC_BITAND] = 7;
		priority_class[PC_BITXOR] = 8;
		priority_class[PC_BITOR] = 9;
		priority_class[PC_LOGICAND] = 10;
		priority_class[PC_LOGICOR] = 11;
		priority_class[PC_TERNARY] = 12;
		priority_class[PC_STORE] = 13;
	}
	else
	{
		priority_class[PC_UNARYNOT] = 5;
		priority_class[PC_MULDIV] = 3;
		priority_class[PC_ADDSUB] = 4;
		priority_class[PC_SHIFT] = 3;
		priority_class[PC_RELATION] = 5;
		priority_class[PC_EQUALITY] = 5;
		priority_class[PC_BITAND] = 3;
		priority_class[PC_BITXOR] = 3;
		priority_class[PC_BITOR] = 3;
		priority_class[PC_LOGICAND] = 7;
		priority_class[PC_LOGICOR] = 7;
		priority_class[PC_TERNARY] = 6;
		priority_class[PC_STORE] = 6;
	}
	priority_class[MAX_PRIORITY_CLASSES] = 0;
	for (j = 0; j < MAX_PRIORITY_CLASSES; j++)
		if (priority_class[MAX_PRIORITY_CLASSES] < priority_class[j])
			priority_class[MAX_PRIORITY_CLASSES] = priority_class[j];

	memset(pcount, 0, sizeof(pcount));
	memset(opcodeprioritized, 0, sizeof(opcodeprioritized));
	for (i = 0; pr_opcodes[i].name; i++)
	{
		op = &pr_opcodes[i];
		j = priority_class[op->priorityclass];
		if (j <= 0 || j > priority_class[MAX_PRIORITY_CLASSES])
			continue;	//class doesn't need prioritising
		opcodeprioritized[j][pcount[j]++] = op;
	}

	//operators need to be sorted so we don't have to scan through all of them. should probably have some better table for that.
	for (j = 0; j <= TOP_PRIORITY; j++)
		qsort (&opcodeprioritized[j][0], pcount[j], sizeof(opcodeprioritized[j][0]), sort_opcodenames);
}

static pbool QCC_OPCodeValidForTarget(qcc_targetformat_t targfmt, unsigned int qcc_targetversion, QCC_opcode_t *op)
{
	int num;
	num = op - pr_opcodes;

	//never any emulated opcodes
	if (num >= OP_NUMREALOPS)
		return false;

	switch(targfmt)
	{
	case QCF_STANDARD:
	case QCF_KK7:
	case QCF_QTEST:
		if (num < OP_MULSTORE_F)
			return true;
		return false;
	case QCF_UHEXEN2:
	case QCF_HEXEN2:
		if (num >= OP_SWITCH_V && num <= OP_SWITCH_FNC)	//these were assigned numbers but were never actually implemtented in standard h2.
			return false;
//		if (num >= OP_MULSTORE_F && num <= OP_SUBSTOREP_V)
//			return false;
		if (num <= OP_CALL8H)	//CALLXH are fixed up. This is to provide more dynamic switching...
			return true;
		return false;
	case QCF_FTEH2:
	case QCF_FTE:
	case QCF_FTEDEBUG:
#define QCTARGVER_FTE_DEF			5768//5529
#define QCTARGVER_FTE_MAX			   QCTARGVER_FTE_PRELOCS
		if (num == OP_PUSH || num >= OP_STOREP_I16)
#define QCTARGVER_FTE_PRELOCS		6614//FIXME
			return (qcc_targetversion>=QCTARGVER_FTE_PRELOCS);	//OP_PUSH was buggy before this.
		if (num >= OP_LT_U)		//uint+double+int64+uint64 ops
			return (qcc_targetversion>=5768);
		if (num >= OP_STOREP_I8)	//byte
			return (qcc_targetversion>=5744);
#define QCTARGVER_FTE_STOREP_IDX	5712//added support for the argc arg for storep_* opcodes.
		//	return (qcc_targetversion>=5744);
		if (num >= OP_STOREF_V)	//field stores
			return (qcc_targetversion>=5698);
		if (num >= OP_IF_F)		//iffloat fixes
			return (qcc_targetversion>=3349);
		return true;
	case QCF_QSS:
#define QCTARGVER_QSS_MAX	0
		if (num < OP_MULSTORE_F)
			return true;
		switch(num)
		{
		//int operations.
		case OP_ADD_I:
		case OP_ADD_IF:
		case OP_ADD_FI:
		case OP_SUB_I:
		case OP_SUB_IF:
		case OP_SUB_FI:
		case OP_MUL_I:
		case OP_MUL_IF:
		case OP_MUL_FI:
		case OP_MUL_VI:
		case OP_DIV_VF:
		case OP_DIV_I:
		case OP_DIV_IF:
		case OP_DIV_FI:
		case OP_BITAND_I:
		case OP_BITOR_I:
		case OP_BITAND_IF:
		case OP_BITOR_IF:
		case OP_BITAND_FI:
		case OP_BITOR_FI:
		case OP_GE_I:
		case OP_LE_I:
		case OP_GT_I:
		case OP_LT_I:
		case OP_AND_I:
		case OP_OR_I:
		case OP_GE_IF:
		case OP_LE_IF:
		case OP_GT_IF:
		case OP_LT_IF:
		case OP_AND_IF:
		case OP_OR_IF:
		case OP_GE_FI:
		case OP_LE_FI:
		case OP_GT_FI:
		case OP_LT_FI:
		case OP_AND_FI:
		case OP_OR_FI:
		case OP_NOT_I:
		case OP_EQ_I:
		case OP_EQ_IF:
		case OP_EQ_FI:
		case OP_NE_I:
		case OP_NE_IF:
		case OP_NE_FI:
		case OP_CONV_ITOF:
		case OP_CONV_FTOI:
		case OP_BITXOR_I:
		case OP_RSHIFT_I:
		case OP_LSHIFT_I:
		case OP_STORE_I:
		case OP_STORE_IF:
		case OP_STORE_FI:
			return true;

		//bugfixes...
		case OP_IF_F:
		case OP_IFNOT_F:
		case OP_IF_S:
		case OP_IFNOT_S:
			return true;

		//faster field access
		case OP_STOREF_V:	//3 elements...
		case OP_STOREF_F:	//1 fpu element...
		case OP_STOREF_S:	//1 string reference
		case OP_STOREF_I:	//1 non-string reference/int
			return true;

		//dp-style arrays
		case OP_GLOAD_I:
		case OP_GLOAD_F:
		case OP_GLOAD_FLD:
		case OP_GLOAD_ENT:
		case OP_GLOAD_S:
		case OP_GLOAD_FNC:
		case OP_GLOAD_V:
		case OP_GSTOREP_I:
		case OP_GSTOREP_F:
		case OP_GSTOREP_ENT:
		case OP_GSTOREP_FLD:
		case OP_GSTOREP_S:
		case OP_GSTOREP_FNC:
		case OP_GSTOREP_V:
			return true;

		case OP_BOUNDCHECK:
			return true;

		//fte-style arrays
		case OP_LOADA_F:
		case OP_LOADA_V:
		case OP_LOADA_S:
		case OP_LOADA_ENT:
		case OP_LOADA_FLD:
		case OP_LOADA_FNC:
		case OP_LOADA_I:
		case OP_GLOBALADDRESS:
			return true;

		//pointer-to-global
		//other useful pointer stuff
//		case OP_LOADP_F:
//		case OP_LOADP_V:
//		case OP_LOADP_S:
//		case OP_LOADP_ENT:
//		case OP_LOADP_FLD:
//		case OP_LOADP_FNC:
//		case OP_LOADP_I:
//		case OP_STOREP_I:
//			return true;

		//hexen2-style arrays.
		case OP_FETCH_GBL_F:
		case OP_FETCH_GBL_S:
		case OP_FETCH_GBL_E:
		case OP_FETCH_GBL_FNC:
		case OP_FETCH_GBL_V:
			return true;
		//hexen2's calling convention.
		case OP_CALL1H:
		case OP_CALL2H:
		case OP_CALL3H:
		case OP_CALL4H:
		case OP_CALL5H:
		case OP_CALL6H:
		case OP_CALL7H:
		case OP_CALL8H:
			return true;

		default:
			return false;
		}
		return false;
	case QCF_DARKPLACES:
#define QCTARGVER_DP_DEF		12901
#define QCTARGVER_DP_MAX		20250104	//https://github.com/DarkPlacesEngine/DarkPlaces/pull/237
		//all id opcodes.
		if (num < OP_MULSTORE_F)
			return true;

		//extended opcodes.
		switch(num)
		{
		case OP_RSHIFT_I:
		case OP_LSHIFT_I:
		case OP_LE_U:
		case OP_LT_U:
		case OP_DIV_U:
		case OP_RSHIFT_U:
			return (qcc_targetversion>=20250104);	//https://github.com/DarkPlacesEngine/DarkPlaces/pull/237

		case OP_ADD_PIW:
		case OP_GLOBALADDRESS:
		case OP_LOADA_F:
		case OP_LOADA_V:
		case OP_LOADA_S:
		case OP_LOADA_ENT:
		case OP_LOADA_FLD:
		case OP_LOADA_FNC:
		case OP_LOADA_I:
		case OP_LOAD_P:
		case OP_LOADP_F:
		case OP_LOADP_V:
		case OP_LOADP_S:
		case OP_LOADP_ENT:
		case OP_LOADP_FLD:
		case OP_LOADP_FNC:
		case OP_LOADP_I:
#define QCTARGVER_DP_STOREP_IDX	20241108	//FIXME: set properly once https://github.com/DarkPlacesEngine/DarkPlaces/pull/215 is merged.
			return (qcc_targetversion>=QCTARGVER_DP_STOREP_IDX);	//https://github.com/DarkPlacesEngine/DarkPlaces/pull/215
		//opcodes that were buggy in DP.
		case OP_ADD_IF:		//dp wrote these to ints, which doesn't match our defined opcodes. not really a problem.
		case OP_SUB_IF:		//dp wrote these to ints, which doesn't match our defined opcodes. revert to _F.
		case OP_MUL_IF:		//dp wrote these to ints, which doesn't match our defined opcodes. revert to _F
		case OP_DIV_IF:		//dp wrote these to ints, which doesn't match our defined opcodes. revert to _F
		case OP_BITAND_FI:	//dp outputs floats, which doesn't match our defined opcodes.
		case OP_BITOR_FI:	//dp outputs floats, which doesn't match our defined opcodes.
		case OP_STORE_I:	//was omitted.
		case OP_STORE_P:	//was omitted.
			return (qcc_targetversion>=12901);

		case OP_ADD_I:
		case OP_ADD_FI:
		case OP_SUB_I:
		case OP_SUB_FI:
		case OP_CONV_ITOF:
		case OP_CONV_FTOI:
		case OP_LOAD_I:		//no worse than the other OP_LOAD_X functions.
		case OP_STOREP_I:	//no worse than the other OP_STOREP_X functions
		case OP_BITAND_I:
		case OP_BITOR_I:
		case OP_MUL_I:
		case OP_DIV_I:
		case OP_EQ_I:
		case OP_NE_I:
		case OP_NOT_I:
		case OP_DIV_VF:
		case OP_LE_I:
		case OP_GE_I:
		case OP_LT_I:
		case OP_GT_I:
		case OP_LE_IF:
		case OP_GE_IF:
		case OP_LT_IF:
		case OP_GT_IF:
		case OP_LE_FI:
		case OP_GE_FI:
		case OP_LT_FI:
		case OP_GT_FI:
		case OP_EQ_IF:
		case OP_EQ_FI:
		case OP_MUL_FI:
		case OP_MUL_VI:
		case OP_DIV_FI:
		case OP_BITAND_IF:
		case OP_BITOR_IF:
		case OP_AND_I:
		case OP_OR_I:
		case OP_AND_IF:
		case OP_OR_IF:
		case OP_AND_FI:
		case OP_OR_FI:
		case OP_NE_IF:
		case OP_NE_FI:
		case OP_GSTOREP_I:		//stores into the globals array, they can change any global dynamically, but thats supposedly no real security risk.
		case OP_GSTOREP_F:
		case OP_GSTOREP_ENT:
		case OP_GSTOREP_FLD:
		case OP_GSTOREP_S:
		case OP_GSTOREP_FNC:
		case OP_GSTOREP_V:
//		case OP_GADDRESS:
		case OP_GLOAD_I://c = globals[inta]
		case OP_GLOAD_F://note: fte does not support these
		case OP_GLOAD_FLD:
		case OP_GLOAD_ENT:
		case OP_GLOAD_S:
		case OP_GLOAD_FNC:
		case OP_BOUNDCHECK:
		case OP_GLOAD_V:
			return true;


		//this opcode looks weird
		case OP_GADDRESS://floatc = globals[inta + floatb] (fte does not support)
			return false;

		default:			//anything I forgot to mention is new, and doesn't work in DP that I'm aware of.
			return false;
		}
	}
	return false;
}

static pbool QCC_OPCodeValid(QCC_opcode_t *op)
{
	return op->flags & OPF_VALID;
}
static pbool QCC_OPCode_StorePOffset(void)
{	//5712
	switch(qcc_targetformat)
	{
	case QCF_FTE:
	case QCF_FTEH2:
	case QCF_FTEDEBUG:
		return (qcc_targetversion>=QCTARGVER_FTE_STOREP_IDX);
	case QCF_DARKPLACES:
		return (qcc_targetversion>=QCTARGVER_DP_STOREP_IDX);
	case QCF_QSS:
		return true;
	default:
		return true;
	}
}

void QCC_OPCodeSetTarget(qcc_targetformat_t targfmt, unsigned int targver)
{
	size_t i;
	qcc_targetformat = targfmt;
	qcc_targetversion = targver;
	flag_undefwordsize = false;
	flag_pointerrelocs = false;

	switch(qcc_targetformat)
	{
	case QCF_FTE:
	case QCF_FTEH2:
	case QCF_FTEDEBUG:
		if (qcc_targetversion > QCTARGVER_FTE_MAX)
		{
			if (qcc_targetversion != ~0u)
				QCC_PR_ParseWarning(WARN_BADTARGET, "target revision %u is unknown, assuming revision %u", qcc_targetversion, QCTARGVER_FTE_MAX);
			qcc_targetversion = QCTARGVER_FTE_MAX;
		}

		flag_pointerrelocs = qcc_targetversion >= QCTARGVER_FTE_PRELOCS;
		break;
	case QCF_DARKPLACES:
		flag_undefwordsize = true;	//DP insists on using word-aligned pointers instead of byte-aligned ones. This breaks both pointer maths and string<>pointer casts.
		if (qcc_targetversion > QCTARGVER_DP_MAX)
		{
			if (qcc_targetversion != ~0u)
				QCC_PR_ParseWarning(WARN_BADTARGET, "target revision %u is unknown, assuming revision %u", qcc_targetversion, QCTARGVER_DP_MAX);
			qcc_targetversion = QCTARGVER_DP_MAX;
		}
		break;
	case QCF_QSS:
		if (qcc_targetversion > QCTARGVER_QSS_MAX)
		{
			if (qcc_targetversion != ~0u)
				QCC_PR_ParseWarning(WARN_BADTARGET, "target revision %u is unknown, assuming revision %u", qcc_targetversion, QCTARGVER_QSS_MAX);
			qcc_targetversion = QCTARGVER_QSS_MAX;
		}
		break;
	default:
		if (qcc_targetversion > 0)
		{
			if (qcc_targetversion != ~0u)
				QCC_PR_ParseWarning(WARN_BADTARGET, "target revision %u is unknown, assuming revision %u", qcc_targetversion, 0);
			qcc_targetversion = 0;
		}
		break;
	}

	for (i = 0; i < OP_NUMOPS; i++)
	{
		QCC_opcode_t *op = &pr_opcodes[i];
		if (QCC_OPCodeValidForTarget(targfmt, qcc_targetversion, op))
			op->flags |= OPF_VALID;
		else
			op->flags &= ~OPF_VALID;
	}
}

static struct {
	qcc_targetformat_t target;
	const char *name;
	unsigned int defaultrev;
} targets[] = {
	{QCF_STANDARD,	"standard",		0},
	{QCF_STANDARD,	"vanilla",		0},
	{QCF_STANDARD,	"q1",			0},
	{QCF_STANDARD,	"id",			0},
	{QCF_STANDARD,	"quakec",		0},

	{QCF_STANDARD,	"qs",			0},
	{QCF_QSS,		"qss",			QCTARGVER_QSS_MAX},

	{QCF_HEXEN2,	"hexen2",		0},
	{QCF_HEXEN2,	"h2",			0},
	{QCF_UHEXEN2,	"uhexen2",		0},

	{QCF_KK7,		"kkqwsv",		0},
	{QCF_KK7,		"kk7",			0},
	{QCF_KK7,		"version7",		0},

	{QCF_FTE,		"fte",			QCTARGVER_FTE_DEF},	//'latest' stable revision.
	{QCF_FTEH2,		"fteh2",		QCTARGVER_FTE_DEF},
	{QCF_FTEDEBUG,	"ftedebug",		QCTARGVER_FTE_DEF},
	{QCF_FTEDEBUG,	"debug",		QCTARGVER_FTE_DEF},

	{QCF_FTE,		"quake2c",		5744},	//an alias for Paril's project, which does various pointer stuff. the revision should be high enough for str[int] ops.

	{QCF_DARKPLACES,"darkplaces",	QCTARGVER_DP_DEF},
	{QCF_DARKPLACES,"dp",			QCTARGVER_DP_DEF},

	{QCF_QTEST,		"qtest",		0},
	{0,				NULL}
};
pbool QCC_OPCodeSetTargetName(const char *targ)
{	//fte_24 -> fmt=QCF_FTE, ver=24
	const char *ver;
	size_t i, tlen;
	ver = strchr(targ, '_');
	if (ver)
	{
		tlen = ver-targ;
		ver++;
	}
	else
	{
		tlen = strlen(targ);
		ver = NULL;
	}

	for (i = 0; targets[i].name; i++)
		if (!strnicmp(targ, targets[i].name, tlen) && strlen(targets[i].name)==tlen)
		{
			qcc_targetformat_t newtype = targets[i].target;

			if (numstatements > 1)
			{
#define ish2(fmt) ((fmt) == QCF_HEXEN2 || (fmt) == QCF_UHEXEN2 || (fmt) == QCF_FTEH2)
				pbool nowh2  = ish2(newtype);
				pbool wash2 = ish2(qcc_targetformat);

				if (nowh2 != wash2)
				{	//hexen2 involves bulk renaming of OP_CALL->OP_CALLH opcodes on load time, which breaks any previously compiled code, rather than just extra statements.
					QCC_PR_ParseWarning(WARN_BADTARGET, "Cannot switch to %shexen2 target \'%s\' after the first statement. Ignored.", targ, wash2?"non-":"");
					return true;
				}
			}

			QCC_OPCodeSetTarget(targets[i].target, ver?atoi(ver):targets[i].defaultrev);
			return true;
		}
	return false;
}

#define EXPR_WARN_ABOVE_1 2
#define EXPR_DISALLOW_COMMA 4
#define EXPR_DISALLOW_ARRAYASSIGN 8
QCC_sref_t QCC_PR_Expression (int priority, int exprflags);
int QCC_AStatementJumpsTo(int targ, int first, int last);
pbool QCC_StatementIsAJump(int stnum, int notifdest);

//===========================================================================

//typically used for debugging. Also used to determine function names for intrinsics.
static const char *QCC_GetSRefName(QCC_sref_t ref)
{
	if (ref.sym && ref.sym->name/* && !ref.ofs*/)
	{
		if (ref.sym->temp)
			return ref.cast->name;
		return ref.sym->name;
	}
	return "TEMP";
}

qc_inlinestatic QCC_eval_t *QCC_SRef_Data(QCC_sref_t ref)
{
	return (QCC_eval_t*)&ref.sym->symboldata[ref.ofs];
}
qc_inlinestatic QCC_eval_t *QCC_SRef_DataWord(QCC_sref_t ref, int word)
{
	return (QCC_eval_t*)&ref.sym->symboldata[ref.ofs + word];
}


//retrieves the data associated with the reference if its constant and thus readable at compile time.
qc_inlinestatic const QCC_eval_t *QCC_SRef_EvalConst(QCC_sref_t ref)
{
	if (ref.sym && ref.sym->initialized && ref.sym->constant && !ref.sym->reloc)
	{
		ref.sym->referenced = true;
		return QCC_SRef_Data(ref);
	}
	return NULL;
}
//retrieves the data associated with the reference if its constant and thus readable at compile time.
qc_inlinestatic const char *QCC_SRef_EvalStringConst(QCC_sref_t ref)
{
	if (ref.cast->type == ev_string)
	{
		const QCC_eval_t *c = QCC_SRef_EvalConst(ref);
		if (c)
			return &strings[c->string];
	}
	return NULL;
}
//NULL is defined as the immediate 0 or 0i (aka __NULL__).
//named constants that merely have the value 0 are NOT meant to count.
qc_inlinestatic pbool QCC_SRef_IsNull(QCC_sref_t ref)
{
	if (ref.cast->type == ev_integer || ref.cast->type == ev_uint || ref.cast->type == ev_float || ref.cast->type == ev_pointer)
	{
		const QCC_eval_t *c = QCC_SRef_EvalConst(ref);
		if (c)
			return !c->_int;
	}
	return false;
}

//retrieves the int value associated with the reference if its constant and thus readable at compile time.
static pint64_t QCC_Eval_Int(const QCC_eval_t *eval, QCC_type_t *type)
{
	if (!eval)
	{
		QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Not an initialised constant");
		return 0;
	}
	switch(type->type)
	{
	case ev_float:		return eval->_float;
	case ev_double:		return eval->_double;
	case ev_integer:	return eval->_int;
//	case ev_function:
//	case ev_entity:
//	case ev_field:
//	case ev_pointer:
	case ev_uint:		return eval->_uint;
	case ev_boolean:	return QCC_Eval_Int(eval, type->parentclass);	//bools are weird.
	case ev_int64:		return eval->i64;
	case ev_uint64:		return eval->u64;
//	case ev_string:
//	case ev_vector:
	case ev_struct:
	case ev_union:
	default:
		QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Unable to evaluate to numeric constant");
		return 0;
	}
}

//retrieves the int value associated with the reference if its constant and thus readable at compile time.
static float QCC_Eval_Float(const QCC_eval_t *eval, QCC_type_t *type)
{
	if (!eval)
	{
		QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Not an initialised constant");
		return 0;
	}
	switch(type->type)
	{
	case ev_float:		return eval->_float;
	case ev_double:		return eval->_double;
	case ev_integer:	return eval->_int;
//	case ev_function:
//	case ev_entity:
//	case ev_field:
//	case ev_pointer:
	case ev_uint:		return eval->_uint;
	case ev_boolean:	return QCC_Eval_Float(eval, type->parentclass);	//bools are weird.
	case ev_int64:		return eval->i64;
	case ev_uint64:		return eval->u64;
//	case ev_string:
//	case ev_vector:
	case ev_struct:
	case ev_union:
	default:
		QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Unable to evaluate to numeric constant");
		return 0;
	}
}

//retrieves the data associated with the reference if its constant and thus readable at compile time.
static pbool QCC_Eval_Truth(const QCC_eval_t *eval, QCC_type_t *type, pbool assume)
{
	pbool istrue = false;
	int i;
	if (!eval)
		return assume;
	switch(type->type)
	{
	case ev_float:
		istrue = (eval->_float != 0);
		break;
	case ev_double:
		istrue = (eval->_double != 0);
		break;
	case ev_integer:
	case ev_uint:
	case ev_function:
	case ev_entity:
	case ev_field:
	case ev_pointer:
	case ev_boolean:
		istrue = (eval->_int != 0);
		break;
	case ev_int64:
	case ev_uint64:
		istrue = (eval->i64 != 0);
		break;
	case ev_string:
		if (flag_ifstring)
			istrue = !!strings[eval->_int];	//value compare.
		else
			istrue = (eval->_int != 0);		//offset compare
		break;
	case ev_vector:
		if (flag_ifvector)
			istrue = eval->vector[0] || eval->vector[1] || eval->vector[2];
		else
			istrue = (eval->_float != 0);		//legacy buggy behaviour
		break;

	case ev_struct:
	case ev_union:
		QCC_PR_ParseWarning(WARN_CONDITIONALTYPEMISMATCH, "conditional type mismatch: %s", basictypenames[type->type]);
		//fall through anyway.
	default:
		for (i = 0; i < type->size; i++)
			if ((&eval->_int)[i])
				break;
		istrue = i!=type->size;
		break;
	}
	return istrue;
}

static const char *QCC_GetRefName(QCC_ref_t *ref, char *buffer, size_t buffersize)
{
	switch(ref->type)
	{
	case REF_FIELD:
	case REF_NONVIRTUAL:
		QC_snprintfz(buffer, buffersize, "%s.%s", QCC_GetSRefName(ref->base), QCC_GetSRefName(ref->index));
		return buffer;
	case REF_ARRAY:
	case REF_STRING:
		QC_snprintfz(buffer, buffersize, "%s[%s]", QCC_GetSRefName(ref->base), QCC_GetSRefName(ref->index));
		return buffer;
	case REF_POINTER:
		QC_snprintfz(buffer, buffersize, "%s->%s", QCC_GetSRefName(ref->base), QCC_GetSRefName(ref->index));
		return buffer;
	case REF_ACCESSOR:
		if (*ref->accessor->fieldname)
		{	//not an anonymous field
			if (ref->index.sym)
				QC_snprintfz(buffer, buffersize, "%s.%s[%s]", QCC_GetSRefName(ref->base), ref->accessor->fieldname, QCC_GetSRefName(ref->index));
			else
				QC_snprintfz(buffer, buffersize, "%s.%s", QCC_GetSRefName(ref->base), ref->accessor->fieldname);
		}
		else
		{
			if (ref->index.sym)
				QC_snprintfz(buffer, buffersize, "%s[%s]", QCC_GetSRefName(ref->base), QCC_GetSRefName(ref->index));
			else
				QC_snprintfz(buffer, buffersize, "*%s", QCC_GetSRefName(ref->base));
		}
		break;
	case REF_ARRAYHEAD:
	case REF_GLOBAL:
	default:
		break;
	}
	return QCC_GetSRefName(ref->base);
}

/*
============
PR_Statement

Emits a primitive statement, returning the var it places it's value in
============
*/
static int QCC_ShouldConvert(QCC_type_t *from, etype_t wanted)
{
	if (from->type == ev_boolean && wanted != ev_boolean)
		from = from->parentclass;

	/*no conversion needed*/
	if (from->type == wanted)
		return 0;
	if (from->type == ev_integer && wanted == ev_function)
		return 0;
	if (from->type == ev_integer && wanted == ev_pointer)
		return 0;
	/*stuff needs converting*/
	if (from->type == ev_pointer && from->aux_type)
	{
		if (from->aux_type->type == ev_float && wanted == ev_integer)
			return OP_LOADP_FTOI;

		if (from->aux_type->type == ev_integer && wanted == ev_float)
			return OP_LOADP_ITOF;
	}
	else
	{
		if (from->type == ev_float && wanted == ev_integer)
			return OP_CONV_FTOI;
		if (from->type == ev_integer && wanted == ev_float)
			return OP_CONV_ITOF;

		if (from->type == ev_float && wanted == ev_uint)
			return OP_CONV_FU;
		if (from->type == ev_uint && wanted == ev_float)
			return OP_CONV_UF;

		if ((from->type == ev_integer||from->type == ev_uint) && (wanted == ev_integer||wanted == ev_uint))
			return 0;
		if ((from->type == ev_int64||from->type == ev_uint64) && (wanted == ev_int64||wanted == ev_uint64))
			return 0;
		if ((from->type == ev_int64||from->type == ev_uint64) && (wanted == ev_integer||wanted == ev_uint))
			return OP_CONV_I64I;
		if ((from->type == ev_integer) && (wanted == ev_int64 || wanted == ev_uint64))
			return OP_CONV_II64;
		if (from->type == ev_uint && (wanted == ev_int64 || wanted == ev_uint64))
			return OP_CONV_UI64;

		if (from->type == ev_float && wanted == ev_double)
			return OP_CONV_FD;
		if (from->type == ev_double && wanted == ev_float)
			return OP_CONV_DF;

		if (from->type == ev_int64 && wanted == ev_float)
			return OP_CONV_I64F;
		if (from->type == ev_float && wanted == ev_int64)
			return OP_CONV_FI64;
		if (from->type == ev_int64 && wanted == ev_double)
			return OP_CONV_I64D;
		if (from->type == ev_double && wanted == ev_int64)
			return OP_CONV_DI64;
		if (from->type == ev_uint64 && wanted == ev_double)
			return OP_CONV_U64D;
		if (from->type == ev_double && wanted == ev_uint64)
			return OP_CONV_DU64;
		if (from->type == ev_uint64 && wanted == ev_float)
			return OP_CONV_U64F;
		if (from->type == ev_float && wanted == ev_uint64)
			return OP_CONV_FU64;
	
		if (from->type == ev_float && wanted == ev_vector)
			return OP_MUL_FV;
	}

	/*impossible*/
	return -1;
}
static QCC_sref_t QCC_TryEvaluateCast(QCC_sref_t src, QCC_type_t *cast, pbool implicit);
static QCC_sref_t QCC_SupplyConversionForAssignment(QCC_ref_t *to, QCC_sref_t from, QCC_type_t *wanted, pbool fatal)
{
	int o;
	QCC_sref_t rhs;

	rhs = QCC_TryEvaluateCast(from, to->cast, !fatal);
	if (rhs.cast)
		return rhs;

	if (wanted->type == ev_accessor && wanted->parentclass && from.cast->type != ev_accessor)
		wanted = wanted->parentclass;
	if (from.cast->type == ev_accessor && from.cast->parentclass && wanted->type != ev_accessor)
		from.cast = from.cast->parentclass;

	o = QCC_ShouldConvert(from.cast, wanted->type);

	if (o == 0) //type already matches
		return from;
	if (flag_typeexplicit)
	{
		char totypename[256], fromtypename[256], destname[256];
		TypeName(wanted, totypename, sizeof(totypename));
		TypeName(from.cast, fromtypename, sizeof(fromtypename));
		QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, from, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetRefName(to, destname, sizeof(destname)), totypename, fromtypename);
	}
	if (o < 0)
	{
		if (fatal && wanted->type != ev_variant && from.cast->type != ev_variant)
		{
			char totypename[256], fromtypename[256], destname[256];
			TypeName(wanted, totypename, sizeof(totypename));
			TypeName(from.cast, fromtypename, sizeof(fromtypename));
			if (flag_laxcasts)
			{
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetRefName(to, destname, sizeof(destname)), totypename, fromtypename);
				QCC_PR_ParsePrintSRef(WARN_LAXCAST, from);
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, from, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetRefName(to, destname, sizeof(destname)), totypename, fromtypename);
		}
		return from;
	}

	if (o == OP_MUL_FV)
		rhs = QCC_MakeVectorConst(1,1,1);
	else
		rhs = nullsref;
	return QCC_PR_Statement(&pr_opcodes[o], from, rhs, NULL);	//conversion return value
}
static QCC_sref_t QCC_SupplyConversion(QCC_sref_t  var, etype_t wanted, pbool fatal)
{
	extern char *basictypenames[];
	int o;
	QCC_sref_t rhs;

	o = QCC_ShouldConvert(var.cast, wanted);

	if (o == 0) //type already matches
		return var;
	if (flag_typeexplicit)// && !QCC_SRef_IsNull(var))
		QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, var, "Automatic type conversions disabled. Needed %s, got %s.", basictypenames[wanted], basictypenames[var.cast->type]);
	if (o < 0)
	{
		if (fatal && wanted != ev_variant && var.cast->type != ev_variant)
		{
			if (flag_laxcasts)
			{
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit type mismatch. Needed %s%s%s, got %s%s%s.", col_type,basictypenames[wanted],col_none, col_type,basictypenames[var.cast->type],col_none);
				QCC_PR_ParsePrintSRef(WARN_LAXCAST, var);
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, var, "Implicit type mismatch. Needed %s%s%s, got %s%s%s.", col_type,basictypenames[wanted],col_none, col_type,basictypenames[var.cast->type],col_none);
		}
		return var;
	}

	if (o == OP_MUL_FV)
		rhs = QCC_MakeVectorConst(1,1,1);
	else
		rhs = nullsref;
	return QCC_PR_Statement(&pr_opcodes[o], var, rhs, NULL);	//conversion return value
}

size_t tempslocked;	//stats
size_t tempsused;
size_t tempsmax;
temp_t *tempsinfo;

QCC_def_t *aliases;
QCC_def_t *allaliases;

static QCC_sref_t QCC_GetTemp(QCC_type_t *type);
void QCC_FreeTemp(QCC_sref_t t);
void QCC_FreeDef(QCC_def_t *def);
QCC_sref_t QCC_MakeSRefForce(QCC_def_t *def, unsigned int ofs, QCC_type_t *type);
QCC_sref_t QCC_MakeSRef(QCC_def_t *def, unsigned int ofs, QCC_type_t *type);

//we're about to overwrite the given def, so if there's any aliases to it, we need to clear them out.
//if def == NULL, then clobber all.
//def should never be a temp...
static void QCC_ClobberDef(QCC_def_t *def)
{
	QCC_def_t *a, **link;
	for (link = &aliases; *link;)
	{
		a = *link;
		if (!def || a->generatedfor == def)
		{
			//okay, we have a live alias. bum.
			//create a new temp for it. update previous statements to refer to the original location instead of the alias.
			//copy the source into the temp, and then update the alias's def to be a sub-symbol of the temp's def instead of a sub-symbol of the original location.
			//yes. all this just to make mods like xonotic not an insane sea of copies.
			QCC_sref_t tmp, from;
			int st;
			*link = a->nextlocal;
			a->nextlocal = NULL;
			if (a->refcount)
			{
				tmp = QCC_GetTemp(a->type->type==ev_variant?type_vector:a->type);
				for (st = a->fromstatement; st < numstatements; st++)
				{
					if (statements[st].a.sym == a)
						statements[st].a.sym = a->generatedfor;
					if (statements[st].b.sym == a)
						statements[st].b.sym = a->generatedfor;
					if (statements[st].c.sym == a)
						statements[st].c.sym = a->generatedfor;
				}
				tmp.sym->refcount = a->symbolheader->refcount;
				a->symbolheader = tmp.sym;	//the alias now refers to a temp
				from = QCC_MakeSRefForce(a->generatedfor, 0, a->type);
				a->generatedfor = tmp.sym;
				a->name = tmp.sym->name;
				a->temp = tmp.sym->temp;
				a->ofs = tmp.sym->ofs;
				tmp.sym = a;
				if (a->type->type==ev_variant || a->type->type == ev_vector)
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], from, tmp, NULL, STFL_PRESERVEB));
				else if (a->type->type==ev_int64 || a->type->type == ev_uint64 || a->type->type == ev_double)
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], from, tmp, NULL, STFL_PRESERVEB));
				else
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], from, tmp, NULL, STFL_PRESERVEB));
			}
		}
		else
			link = &(*link)->nextlocal;
	}
}
//return an alias to a reference. typically the return value.
static QCC_sref_t QCC_GetAliasTemp(QCC_sref_t ref)
{
	QCC_def_t *def;
	def = qccHunkAlloc(sizeof(QCC_def_t));
	def->type = ref.cast;
	def->generatedfor = ref.sym;
	def->symbolheader = def;
	def->symbolsize = ref.sym->symbolsize;
	def->name = ref.sym->name;
	def->referenced = true;
	def->fromstatement = numstatements;
	def->scope = pr_scope;

	//allaliases allows them to be finalized correctly
	def->strip = true;
	def->next = allaliases;
	allaliases = def;

	//and active aliases are the ones that are currently live in their original location
	def->nextlocal = aliases;
	aliases = def;

	QCC_FreeTemp(ref);

	return QCC_MakeSRefForce(def, 0, ref.cast);
}

static QCC_sref_t QCC_GetTemp(QCC_type_t *type)
{
	QCC_sref_t var_c = nullsref;
	size_t u;

	var_c.cast = type;

	if (opt_overlaptemps)	//don't exceed. This lets us allocate a huge block, and still be able to compile smegging big funcs.
	{
		for (u = 0; u < tempsused; u += tempsinfo[u].size)
		{
			if (!tempsinfo[u].def->refcount && tempsinfo[u].size == type->size)
				break;
		}
	}
	else
		u = tempsused;

	if (u == tempsused)
	{
		char buffer[32];
		gofs_t ofs = tempsused;
		unsigned int i;
		unsigned int size = type->size;
		if (type->type == ev_accessor || type->type == ev_bitfld)
			size = type->parentclass->size;
		tempsused += size;

		if (tempsused > tempsmax)
		{
			size_t newmax = (tempsused + 64) & ~63;
			tempsinfo = realloc(tempsinfo, newmax*sizeof(*tempsinfo));
			memset(tempsinfo+ofs, 0, (newmax-ofs)*sizeof(*tempsinfo));
			tempsmax = newmax;
		}
		for(i = u; size > 0; i++, size--)
		{
//			tempsinfo[i].used = (i==ofs)?0:2;	//2 is an 'padded' temp. if encountered, scan down for one that doesn't have a 2 there.
			tempsinfo[i].size = (i==ofs)?size:0;
			tempsinfo[i].def = qccHunkAlloc(sizeof(QCC_def_t));
			tempsinfo[i].def->symbolheader = tempsinfo[i].def;
			tempsinfo[i].def->symbolsize = tempsinfo[i].size;
		}
		for (i = 0; i < tempsused; i++)
			tempsinfo[i].def->temp = &tempsinfo[i];
		tempsinfo[u].def->ofs = u;
		tempsinfo[u].def->type = type;

		sprintf(buffer, "temp_%u", (unsigned)u);
		tempsinfo[u].def->name = qccHunkAlloc(strlen(buffer)+1);
		strcpy(tempsinfo[u].def->name, buffer);
	}
	else
		optres_overlaptemps+=type->size;

	var_c.sym = tempsinfo[u].def;
	var_c.ofs = 0;//u;
	tempsinfo[u].def->refcount+=1;

	tempsinfo[u].lastfunc = pr_scope;
	tempsinfo[u].lastline = pr_source_line;
	tempsinfo[u].laststatement = numstatements;

	var_c.sym->referenced = true;
	return var_c;
}

void QCC_FinaliseTemps(void)
{
	unsigned int i;
	for (i = 0; i < tempsused; )
	{
		tempsinfo[i].def->ofs = numpr_globals;
		numpr_globals += tempsinfo[i].size;
		i += tempsinfo[i].size;
	}

	if (numpr_globals >= MAX_REGS)
	{
		if (!opt_overlaptemps || !opt_locals_overlapping)
			QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS - you'll need to use more optimisations");
		else
			QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS of %u. Increase with eg: -max_regs %u", MAX_REGS, MAX_REGS*2);
	}

	//finalize alises so they map correctly.
	while(allaliases)
	{
		allaliases->symbolheader = allaliases->generatedfor->symbolheader;
		allaliases->ofs = allaliases->generatedfor->ofs;
		allaliases = allaliases->next;
	}
}

void QCC_FreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
	{
		if (--def->symbolheader->refcount < 0)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: over-freed refcount to %s", def->name);
	}
}
//nothing else references this temp.
void QCC_FreeTemp(QCC_sref_t t)
{
	if (t.sym && t.sym->symbolheader)
	{
		if (--t.sym->symbolheader->refcount < 0)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: over-freed refcount to %s", QCC_VarAtOffset(t));
	}
}

static void QCC_ForceUnFreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
		def->symbolheader->refcount++;
}
/*
static void QCC_UnFreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
	{
		if (!def->symbolheader->refcount++)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %s was already fully freed.", def->name);
	}
}
*/
static void QCC_UnFreeTemp(QCC_sref_t t)
{
	if (t.sym && t.sym->symbolheader)
	{
		if (!t.sym->symbolheader->refcount++)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %s+%i@%i was already fully freed.", QCC_VarAtOffset(t), t.ofs, t.sym->ofs);
	}
}

//We've just parsed a statement.
//We can gaurentee that any used temps are now not used.
#ifdef _DEBUG
static void QCC_FreeTemps(void)
{
}
#else
#define QCC_FreeTemps()
#endif
void QCC_PurgeTemps(void)
{
	free(tempsinfo);
	tempsinfo = NULL;
	tempsmax = 0;
	tempsused = 0;
	aliases = NULL;
	allaliases = NULL;


	max_labels = 0;
	max_gotos = 0;
	free(pr_labels);
	pr_labels = NULL;
	free(pr_gotos);
	pr_gotos = NULL;
	num_gotos = num_labels = 0;

	free(initstatements);
	initstatements = NULL;
	numinitstatements = maxinitstatements = 0;
}

//temps that are still in use over a function call can be considered dodgy.
//we need to remap these to locally defined temps, on return from the function so we know we got them all.
static void QCC_LockActiveTemps(QCC_sref_t exclude)
{
	size_t u;
	size_t excludeofs = ~0;

	QCC_ClobberDef(NULL);

	if (exclude.sym && exclude.sym->temp)
		excludeofs = exclude.sym->temp - tempsinfo;

	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (tempsinfo[u].def->refcount && u != excludeofs)	//don't print this after an error jump out.
			tempsinfo[u].locked = true;
	}
}

/*
static void QCC_ForceLockTempForOffset(int ofs)
{
	tempsinfo[ofs].locked = true;
}
*/

static QCC_def_t *QCC_MakeLocked(gofs_t tofs, gofs_t tsize, QCC_def_t *tmp)
{
#ifdef WRITEASM
	char buffer[128];
#endif

	QCC_def_t *def = NULL;
	QCC_def_t *a, **link;
	tempslocked+=tsize;

	def = QCC_PR_DummyDef(type_float, NULL, pr_scope, tsize==1?0:tsize, NULL, 0, false, GDF_STRIP);
	def->arraylengthprefix = false;	//don't waste space with temps.
#ifdef WRITEASM
	sprintf(buffer, "locked_%i", tofs);
	def->name = qccHunkAlloc(strlen(buffer)+1);
	strcpy(def->name, buffer);
#endif
	def->referenced = true;


	//aliases might refer to this temp.
	//make sure they point to the local instead.
	for (link = &allaliases; *link;)
	{
		a = *link;
		if (a->generatedfor == tmp && a->scope == pr_scope)
		{
//			*link = a->next;
//			a->next = NULL;
			
			a->generatedfor = def;
			a->name = def->name;
		}
		else
			link = &(*link)->next;
	}
	return def;
}
static void QCC_RemapLockedTemp(gofs_t tofs, gofs_t tsize, int firststatement, int laststatement)
{
	QCC_def_t *def = NULL;
	QCC_statement_t *st;
	int i;

	for (i = firststatement, st = &statements[i]; i < laststatement; i++, st++)
	{
		if (pr_opcodes[st->op].type_a && st->a.sym && st->a.sym->temp && st->a.sym->ofs >= tofs && st->a.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->a.sym);
			st->a.sym = def;
//			st->a.ofs = st->a.ofs - tofs;
		}
		if (pr_opcodes[st->op].type_b && st->b.sym && st->b.sym->temp && st->b.sym->ofs >= tofs && st->b.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->b.sym);
			st->b.sym = def;
//			st->b.ofs = st->b.ofs - tofs;
		}
		if (pr_opcodes[st->op].type_c && st->c.sym && st->c.sym->temp && st->c.sym->ofs >= tofs && st->c.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->c.sym);
			st->c.sym = def;
//			st->c.ofs = st->c.ofs - tofs;
		}
	}
}

//called for function calls to avoid wasting copies
static pbool QCC_RemapTemp(int firststatement, int laststatement, QCC_sref_t temp, QCC_sref_t targ)
{
	QCC_def_t *def = NULL;
	QCC_statement_t *st;
	int i;
	pbool remapped = false;

	//if its not a temp, we'll need to do a copy.
	if (!temp.sym->temp)
		return false;
	if (temp.sym->refcount != 1)
		return false;	//should only have one reference... something weird is happening if it doesn't.

	//make sure the target is not already used in the interim, because that gets messy.
	def = targ.sym->symbolheader;
	for (i = firststatement, st = &statements[i]; i < laststatement; i++, st++)
	{
		if (pr_opcodes[st->op].type_a && st->a.sym && st->a.sym->symbolheader == def)
			return false;
		if (pr_opcodes[st->op].type_b && st->b.sym && st->b.sym->symbolheader == def)
			return false;
		if (pr_opcodes[st->op].type_c && st->c.sym && st->c.sym->symbolheader == def)
			return false;
	}

	def = temp.sym->symbolheader;
	//make sure things actually look okay. reads from it imply something weird
	for (i = firststatement, st = &statements[i]; i < laststatement; i++, st++)
	{
		if (pr_opcodes[st->op].type_a && st->a.sym && st->a.sym->temp == def->temp)
			if (!OpAssignsToA(st->op))
				return false;
		if (pr_opcodes[st->op].type_b && st->b.sym && st->b.sym->temp == def->temp)
			if (!OpAssignsToB(st->op))
				return false;
		if (pr_opcodes[st->op].type_c && st->c.sym && st->c.sym->temp == def->temp)
			if (!OpAssignsToC(st->op))
				return false;
	}

	//okay, go ahead and remap it
	for (i = firststatement, st = &statements[i]; i < laststatement; i++, st++)
	{
		if (pr_opcodes[st->op].type_a && st->a.sym && st->a.sym->temp == def->temp)
		{
			st->a.sym = targ.sym;
			remapped = true;
		}
		if (pr_opcodes[st->op].type_b && st->b.sym && st->b.sym->temp == def->temp)
		{
			st->b.sym = targ.sym;
			remapped = true;
		}
		if (pr_opcodes[st->op].type_c && st->c.sym && st->c.sym->temp == def->temp)
		{
			st->c.sym = targ.sym;
			remapped = true;
		}
	}
	return remapped;
}

static void QCC_RemapLockedTemps(int firststatement, int laststatement)
{
	size_t u;
	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (tempsinfo[u].locked)
		{
			QCC_RemapLockedTemp(u, tempsinfo[u].size, firststatement, laststatement);
			tempsinfo[u].locked = false;
		}
	}
}

static void QCC_fprintfLocals(FILE *f, QCC_def_t *locals)
{
	QCC_def_t	*var;
	char typebuf[1024];
	size_t u;

	for (var = locals; var; var = var->nextlocal)
	{
		if (var->arraysize)
			fprintf(f, "local %s %s[%i];\n", TypeName(var->type, typebuf, sizeof(typebuf)), var->name, var->arraysize);
		else
			fprintf(f, "local %s %s;\n", TypeName(var->type, typebuf, sizeof(typebuf)), var->name);
	}

	if (opt_overlaptemps)	//don't spam.
	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (!tempsinfo[u].locked)
//		if (tempsinfo[u].lastfunc == pr_scope)
		{
			fprintf(f, "local %s temp_%u; //%i\n", (tempsinfo[u].size == 1)?"float":"vector", (unsigned)u, tempsinfo[u].lastline);
		}
	}
}

#ifdef WRITEASM
void QCC_WriteAsmFunction(QCC_function_t	*sc, unsigned int firststatement, QCC_def_t *firstlocal);
const char *QCC_VarAtOffset(QCC_sref_t ref)
{	//for debugging, we don't need to preserve the cast.
	static char message[1024];
	//check the temps

	if (ref.sym)
	{
		if (ref.sym && ref.sym->temp)
		{
			if (!ref.ofs)
				QC_snprintfz(message, sizeof(message), "temp_%i", ref.sym->ofs-tempsinfo[0].def->ofs);
			else
				QC_snprintfz(message, sizeof(message), "temp_%i+%i", ref.sym->ofs-tempsinfo[0].def->ofs, ref.ofs);
			return message;
		}
		else if (ref.sym->name && !STRCMP(ref.sym->name, "IMMEDIATE"))
		{
			int type;
			const QCC_eval_t *val = QCC_SRef_EvalConst(ref);
			type = !val?-1:ref.cast->type;
			if (type == ev_variant)
				type = ref.sym->type->type;
			if (ref.sym->reloc)// && ref.cast->type == ev_pointer)
			{
				QC_snprintfz(message, sizeof(message), "&(%s+%i)", ref.sym->reloc->name, ref.sym->symboldata[ref.ofs]._int);
				return message;
			}
			switch(type)
			{
			case ev_string:
				{
					char *in = &strings[val->string], *out=message;
					char *end = out+sizeof(message)-3;
					*out++ = '\"';
					for(; out < end && *in; in++)
					{
						if (*in == '\n')
						{
							*out++ = '\\';
							*out++ = 'n';
						}
						else if (*in == '\t')
						{
							*out++ = '\\';
							*out++ = 't';
						}
						else if (*in == '\r')
						{
							*out++ = '\\';
							*out++ = 'r';
						}
						else if (*in == '\"')
						{
							*out++ = '\\';
							*out++ = '"';
						}
						else if (*in == '\'')
						{
							*out++ = '\\';
							*out++ = '\'';
						}
						else
							*out++ = *in;
					}
					*out++ = '\"';
					*out++ = 0;
				}
				return message;
			case ev_function:
				if (val->_int>0 && val->_int < numfunctions && *functions[val->_int].name)
					QC_snprintfz(message, sizeof(message), "%s", functions[val->_int].name);
				else
					QC_snprintfz(message, sizeof(message), "%"pPRIi"i", val->_int);
				return message;
			case ev_field:
			case ev_integer:
				QC_snprintfz(message, sizeof(message), "%#"pPRIx"i", val->_int);
				return message;
			case ev_uint:
				QC_snprintfz(message, sizeof(message), "%#"pPRIx"u", val->_uint);
				return message;
			case ev_int64:
				//QC_snprintfz(message, sizeof(message), "%"pPRIu64"ill", val->i64);
				QC_snprintfz(message, sizeof(message), "%#"pPRIx64"ill", val->i64);
				return message;
			case ev_uint64:
				//QC_snprintfz(message, sizeof(message), "%"pPRIu64"ull", val->u64);
				QC_snprintfz(message, sizeof(message), "%#"pPRIx64"ull", val->u64);
				return message;
			case ev_entity:
				QC_snprintfz(message, sizeof(message), "%"pPRIi"e", val->_int);
				return message;
			case ev_float:
				if (!val->_float || val->_int & 0x7f800000)
					QC_snprintfz(message, sizeof(message), "%gf", val->_float);
				else
					QC_snprintfz(message, sizeof(message), "%%%"pPRIi, val->_int);
				return message;
			case ev_double:
				QC_snprintfz(message, sizeof(message), "%gd", val->_double);
				return message;
			case ev_vector:
				QC_snprintfz(message, sizeof(message), "'%g %g %g'", val->vector[0], val->vector[1], val->vector[2]);
				return message;
			default:
				if (!ref.ofs)
					QC_snprintfz(message, sizeof(message), "IMMEDIATE");
				else
					QC_snprintfz(message, sizeof(message), "IMMEDIATE+%i", ref.ofs);
				return message;
			}
		}
		else if (ref.ofs || ref.cast != ref.sym->type)
		{
			if (!ref.ofs)
				QC_snprintfz(message, sizeof(message), "%s", ref.sym->name);
			else
				QC_snprintfz(message, sizeof(message), "%s+%i", ref.sym->name, ref.ofs);
			return message;
		}

		return ref.sym->name;
	}

	QC_snprintfz(message, sizeof(message), "offset_%i", ref.ofs);
	return message;
}
#endif

#if IAMNOTLAZY
//need_lock is set if it crossed a function call.
static int QCC_PR_FindSourceForTemp(QCC_def_t *tempdef, int op, pbool *need_lock)
{
	int st = -1;
	*need_lock = false;
	if (tempdef->temp)
	{
		for (st = numstatements-1; st>=0; st--)
		{
			if (statements[st].c == tempdef->ofs)
			{
				if (statements[st].op == op)
					return st;
				return -1;
			}

			if ((statements[st].op >= OP_CALL0 && statements[st].op <= OP_CALL8) || (statements[st].op >= OP_CALL1H && statements[st].op <= OP_CALL8H))
				*need_lock = true;
		}
	}
	return st;
}
#endif

static int QCC_PR_FindSourceForAssignedOffset(QCC_def_t *sym, int firstst)
{
	int st = -1;
	for (st = numstatements-1; st>=firstst; st--)
	{
		if (statements[st].c.sym == sym && OpAssignsToC(statements[st].op))
			return st;
		if (statements[st].b.sym == sym && OpAssignsToB(statements[st].op))
			return st;
	}
	return -1;
}

pbool QCC_Temp_Describe(QCC_def_t *def, char *buffer, int buffersize)
{
	QCC_statement_t	*s;
	int st;
	temp_t *t = def->temp;
	if (!t)
		return false;
	if (t->lastfunc != pr_scope)
		return false;
	st = QCC_PR_FindSourceForAssignedOffset(t->def, t->laststatement);
	if (st == -1)
		return false;
	s = &statements[st];
	switch(s->op)
	{
	default:
		QC_snprintfz(buffer, buffersize, "%s %s %s", QCC_VarAtOffset(s->a), pr_opcodes[s->op].name, QCC_VarAtOffset(s->b));
		break;
	}
	return true;
}

static int QCC_PR_RoundFloatConst(const QCC_eval_t *eval)
{
	float val = eval->_float;
	int ival = val;
	if (val != (float)ival)
		QCC_PR_ParseWarning(WARN_OVERFLOW, "Constant float operand %f will be truncated to %i", val, ival);
	return ival;
}

QCC_statement_t *QCC_PR_SimpleStatement ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_sref_t var_c, int force);

QCC_sref_t QCC_PR_StatementFlags ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_statement_t **outstatement, unsigned int flags)
{
	char typea[256], typeb[256];
	QCC_statement_t	*statement;
	QCC_sref_t			var_c=nullsref;
	pbool nan_eq_cond, sym_cmp;

	if (var_a.sym)
	{
		var_a.sym->referenced = true;
		if (flags&STFL_PRESERVEA)
			QCC_UnFreeTemp(var_a);
	}
	if (var_b.sym)
	{
		var_b.sym->referenced = true;
		if (flags&STFL_PRESERVEB)
			QCC_UnFreeTemp(var_b);
	}

/*	if (op->priority != -1 && op->priority != CONDITION_PRIORITY)
	{
		if (op->associative!=ASSOC_LEFT)
		{
			if (op->type_a != &type_pointer && (flags&STFL_CONVERTB))
				var_b = QCC_SupplyConversion(var_b, (*op->type_a)->type, false);
		}
		else
		{
			if (var_a.cast && (flags&STFL_CONVERTA))
				var_a = QCC_SupplyConversion(var_a, (*op->type_a)->type, false);
			if (var_b.cast && (flags&STFL_CONVERTB))
				var_b = QCC_SupplyConversion(var_b, (*op->type_b)->type, false);
		}
	}
*/
	//maths operators
	if (opt_constantarithmatic || !pr_scope)
	{
		const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
		const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
		if (eval_a)
		{
			if (outstatement)
				*outstatement = NULL;
			if (eval_b)
			{
				//both are constants
				switch (op - pr_opcodes)	//improve some of the maths.
				{
//				case OP_GLOBALADDRESS:
//					if (flag_pointerrelocs)
//						return QCC_MakeGAddress(type_pointer, var_a.sym, var_a.ofs + QCC_Eval_Int(QCC_SRef_EvalConst(var_b), var_b.cast));
//					break;
				case OP_AND_ANY:
//				case OP_AND_F:
				case OP_AND_FI:
//				case OP_AND_I:
				case OP_AND_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?b:a);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
						else
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
					}*/
					optres_constantarithmatic++;
					QCC_FreeTemp(var_a);
					QCC_FreeTemp(var_b);
					if (QCC_Eval_Truth(eval_a, var_a.cast, false) && QCC_Eval_Truth(eval_b, var_b.cast, false))
						return flag_assume_integer?QCC_MakeIntConst(1):QCC_MakeFloatConst(1);
					else
						return flag_assume_integer?QCC_MakeIntConst(0):QCC_MakeFloatConst(0);
					break;

				case OP_OR_ANY:
//				case OP_OR_F:
				case OP_OR_FI:
//				case OP_OR_I:
				case OP_OR_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?a:b);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
						else
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
					}*/
					optres_constantarithmatic++;
					QCC_FreeTemp(var_a);
					QCC_FreeTemp(var_b);
					if (QCC_Eval_Truth(eval_a, var_a.cast, false) || QCC_Eval_Truth(eval_b, var_b.cast, false))
						return flag_assume_integer?QCC_MakeIntConst(1):QCC_MakeFloatConst(1);
					else
						return flag_assume_integer?QCC_MakeIntConst(0):QCC_MakeFloatConst(0);
					break;

				case OP_LOADA_F:
				case OP_LOADA_V:
				case OP_LOADA_S:
				case OP_LOADA_ENT:
				case OP_LOADA_FLD:
				case OP_LOADA_FNC:
				case OP_LOADA_I:
					{
						QCC_sref_t nd = var_a;
						QCC_FreeTemp(var_b);
						nd.ofs += eval_b->_int;
						//FIXME: case away the array...
						return nd;
					}
					break;
				case OP_BITXOR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int ^ eval_b->_int);
				case OP_RSHIFT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int >> eval_b->_int);
				case OP_LSHIFT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int << eval_b->_int);
				case OP_BITXOR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) ^ QCC_PR_RoundFloatConst(eval_b));
				case OP_BITXOR_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(
						(int)eval_a->vector[0] ^ (int)eval_b->vector[0],
						(int)eval_a->vector[1] ^ (int)eval_b->vector[1],
						(int)eval_a->vector[2] ^ (int)eval_b->vector[2]);
				case OP_RSHIFT_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) >> QCC_PR_RoundFloatConst(eval_b));
				case OP_RSHIFT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int >> QCC_PR_RoundFloatConst(eval_b));
				case OP_RSHIFT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) >> eval_b->_int);
				case OP_LSHIFT_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) << QCC_PR_RoundFloatConst(eval_b));
				case OP_LSHIFT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int << QCC_PR_RoundFloatConst(eval_b));
				case OP_LSHIFT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) << eval_b->_int);
				case OP_BITOR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) | QCC_PR_RoundFloatConst(eval_b));
				case OP_BITOR_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(
						(int)eval_a->vector[0] | (int)eval_b->vector[0],
						(int)eval_a->vector[1] | (int)eval_b->vector[1],
						(int)eval_a->vector[2] | (int)eval_b->vector[2]);
				case OP_BITAND_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) & QCC_PR_RoundFloatConst(eval_b));
				case OP_BITAND_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(
						(int)eval_a->vector[0] & (int)eval_b->vector[0],
						(int)eval_a->vector[1] & (int)eval_b->vector[1],
						(int)eval_a->vector[2] & (int)eval_b->vector[2]);
				case OP_MUL_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float * eval_b->_float);
				case OP_DIV_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					if (!eval_b->_float)
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division of %g by 0\n", eval_a->_float);
					return QCC_MakeFloatConst(eval_a->_float / eval_b->_float);
				case OP_ADD_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float + eval_b->_float);
				case OP_SUB_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float - eval_b->_float);

				case OP_BITOR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int | eval_b->_int);
				case OP_BITAND_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int & eval_b->_int);
				case OP_MUL_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int * eval_b->_int);
				case OP_MUL_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int * eval_b->_float);
				case OP_MUL_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float * eval_b->_int);
				case OP_DIV_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					if (eval_b->_int == 0)
					{
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by constant 0");
						return QCC_MakeIntConst(0);
					}
					else if (eval_b->_int == -1 && eval_a->_int == (int)0x80000000)
					{
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Signed overflow on division by -1");
						return QCC_MakeIntConst(0x7fffffff);
					}
					else
						return QCC_MakeIntConst(eval_a->_int / eval_b->_int);
				case OP_ADD_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int + eval_b->_int);
				case OP_SUB_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int - eval_b->_int);

				case OP_MOD_I:
					if (!eval_b->_int)
						break;
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int % eval_b->_int);
				case OP_MOD_U:
					if (!eval_b->_uint)
						break;
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeUIntConst(eval_a->_uint % eval_b->_uint);
				case OP_MOD_F:
					{
						float a = eval_a->_float,n=eval_b->_float;
						if (!n)
							break;
						QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
						optres_constantarithmatic++;
						return QCC_MakeFloatConst(a - (n * (int)(a/n)));
					}

				case OP_ADD_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int + eval_b->_float);
				case OP_ADD_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float + eval_b->_int);

				case OP_ADD_PIW:
					if (flag_undefwordsize)
						break;	//don't make assumptions.
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int + eval_b->_int*VMWORDSIZE);

				case OP_SUB_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int - eval_b->_float);
				case OP_SUB_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float - eval_b->_int);

				case OP_AND_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int && eval_b->_int);
				case OP_OR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int || eval_b->_int);
				case OP_AND_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float && eval_b->_float);
				case OP_OR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float || eval_b->_float);
				case OP_MUL_V:	//mul_v is actually a dot-product
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(	eval_a->vector[0] * eval_b->vector[0] +
												eval_a->vector[1] * eval_b->vector[1] +
												eval_a->vector[2] * eval_b->vector[2]);
				case OP_MUL_FV:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->_float * eval_b->vector[0],
												eval_a->_float * eval_b->vector[1],
												eval_a->_float * eval_b->vector[2]);
				case OP_MUL_VF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] * eval_b->_float,
												eval_a->vector[1] * eval_b->_float,
												eval_a->vector[2] * eval_b->_float);
				case OP_ADD_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] + eval_b->vector[0],
												eval_a->vector[1] + eval_b->vector[1],
												eval_a->vector[2] + eval_b->vector[2]);
				case OP_SUB_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] - eval_b->vector[0],
												eval_a->vector[1] - eval_b->vector[1],
												eval_a->vector[2] - eval_b->vector[2]);


					
				case OP_LE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int <= eval_b->_int);
				case OP_GE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int >= eval_b->_int);
				case OP_LT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int < eval_b->_int);
				case OP_GT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int > eval_b->_int);

				case OP_LE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int <= eval_b->_float);
				case OP_GE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int >= eval_b->_float);
				case OP_LT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int < eval_b->_float);
				case OP_GT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int > eval_b->_float);

				case OP_LE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float <= eval_b->_int);
				case OP_GE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float >= eval_b->_int);
				case OP_LT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float < eval_b->_int);
				case OP_GT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float > eval_b->_int);

				case OP_EQ_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_float == eval_b->_float);
				case OP_EQ_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int == eval_b->_int);
				case OP_EQ_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int == eval_b->_float);
				case OP_EQ_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float == eval_b->_int);

				case OP_MUL_VI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeVectorConst(	eval_a->vector[0] * eval_b->_int,
												eval_a->vector[1] * eval_b->_int,
												eval_a->vector[2] * eval_b->_int);
				case OP_MUL_IV:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeVectorConst(	eval_a->_int * eval_b->vector[0],
												eval_a->_int * eval_b->vector[1],
												eval_a->_int * eval_b->vector[2]);
				case OP_DIV_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					if (!eval_b->_float)
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division of %d by 0\n", eval_a->_int);
					return QCC_MakeFloatConst(eval_a->_int / eval_b->_float);
				case OP_DIV_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					if (!eval_b->_int)
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division of %g by 0\n", eval_a->_float);
					return QCC_MakeFloatConst(eval_a->_float / eval_b->_int);
				case OP_BITAND_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int & QCC_PR_RoundFloatConst(eval_b));
				case OP_BITOR_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int | QCC_PR_RoundFloatConst(eval_b));
				case OP_BITAND_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) & eval_b->_int);
				case OP_BITOR_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) | eval_b->_int);
				case OP_NE_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_float != eval_b->_float);
				case OP_NE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int != eval_b->_int);
				case OP_NE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int != eval_b->_float);
				case OP_NE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float != eval_b->_int);


				case OP_LT_U:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUIntConst(eval_a->_uint < eval_b->_uint);
				case OP_LE_U:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUIntConst(eval_a->_uint <= eval_b->_uint);
				case OP_DIV_U:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUIntConst(eval_a->_uint / eval_b->_uint);
				case OP_RSHIFT_U:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUIntConst(eval_a->_uint >> eval_b->_int);

				case OP_ADD_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 + eval_b->i64);
				case OP_SUB_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 - eval_b->i64);
				case OP_MUL_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 * eval_b->i64);
				case OP_DIV_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					if (eval_b->i64 == 0)
					{
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by constant 0");
						return QCC_MakeInt64Const(0);
					}
					else if (eval_b->i64 == -1 && eval_a->i64 == INT64_MIN)
					{
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Signed overflow on division by -1");
						return QCC_MakeInt64Const(INT64_MAX);
					}
					else
						return QCC_MakeInt64Const(eval_a->i64 / eval_b->i64);
				case OP_LSHIFT_I64I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 << eval_b->_int);
				case OP_RSHIFT_I64I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 >> eval_b->_int);
				case OP_BITAND_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 & eval_b->i64);
				case OP_BITOR_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 | eval_b->i64);
				case OP_BITXOR_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeInt64Const(eval_a->i64 ^ eval_b->i64);
				case OP_LE_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->i64 <= eval_b->i64);
				case OP_LT_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->i64 < eval_b->i64);
				case OP_EQ_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->i64 == eval_b->i64);
				case OP_NE_I64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->i64 != eval_b->i64);
				case OP_LT_U64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->u64 < eval_b->u64);
				case OP_LE_U64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->u64 <= eval_b->u64);
				case OP_DIV_U64:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					if (eval_b->u64 == 0)
					{
						QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by constant 0");
						return QCC_MakeUIntConst(0);
					}
					else
						return QCC_MakeUInt64Const(eval_a->u64 / eval_b->u64);
				case OP_RSHIFT_U64I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUInt64Const(eval_a->u64 >> eval_b->_int);

				case OP_ADD_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst(eval_a->_double + eval_b->_double);
				case OP_SUB_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst(eval_a->_double - eval_b->_double);
				case OP_MUL_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst(eval_a->_double * eval_b->_double);
				case OP_DIV_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst(eval_a->_double / eval_b->_double);

				case OP_LE_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_double <= eval_b->_double);
				case OP_LT_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_double < eval_b->_double);
				case OP_EQ_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_double == eval_b->_double);
				case OP_NE_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_double != eval_b->_double);


				case OP_LSHIFT_DI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst((pint64_t)eval_a->_double << eval_b->_int);
				case OP_RSHIFT_DI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst((pint64_t)eval_a->_double >> eval_b->_int);
				case OP_BITAND_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst((pint64_t)eval_a->_double & (pint64_t)eval_b->_double);
				case OP_BITOR_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst((pint64_t)eval_a->_double | (pint64_t)eval_b->_double);
				case OP_BITXOR_D:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeDoubleConst((pint64_t)eval_a->_double ^ (pint64_t)eval_b->_double);

				case OP_BITEXTEND_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst((eval_a->_int << (32-(eval_b->_uint&0xff)-(eval_b->_uint>>8))) >> (32-(eval_b->_uint&0xff)));
				case OP_BITEXTEND_U:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeUIntConst((eval_a->_uint << (32u-(eval_b->_uint&0xff)-(eval_b->_uint>>8))) >> (32u-(eval_b->_uint&0xff)));
				//case OP_BITCOPY_I: //reads var_c too.
				//	break;
				}
			}
			else
			{
				//a is const, b is not
				switch (op - pr_opcodes)
				{
					//OP_NOT_S needs to do a string comparison
				case OP_NOT_F:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->_float);
				case OP_NOT_V:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->vector[0] && !eval_a->vector[1] && !eval_a->vector[2]);
				case OP_NOT_ENT: // o.O
				case OP_NOT_FNC: // o.O
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->_int);
				case OP_NOT_I:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(!eval_a->_int);
				case OP_BITNOT_F:
					QCC_FreeTemp(var_a);
					return QCC_MakeFloatConst(~QCC_PR_RoundFloatConst(eval_a));
				case OP_BITNOT_I:
					QCC_FreeTemp(var_a);
					return QCC_MakeIntConst(~eval_a->_int);
				case OP_BITNOT_V:
					QCC_FreeTemp(var_a);
					return QCC_MakeVectorConst(~(int)eval_a->vector[0], ~(int)eval_a->vector[1], ~(int)eval_a->vector[2]);
				case OP_CONV_FTOI:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((int)eval_a->_float != eval_a->_float)
					{
						if (eval_a->_int & 0x7f800000)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %i", eval_a->_float, (int)eval_a->_float);
						else
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Denormalized float %g will be rounded to %i. This is probably not what you want.", eval_a->_float, (int)eval_a->_float);
					}
					return QCC_MakeIntConst(eval_a->_float);
				case OP_CONV_ITOF:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						float fl = eval_a->_int;
						if ((int)fl != eval_a->_int)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#x to %#x.", eval_a->_int, (int)fl);
					}
					return QCC_MakeFloatConst(eval_a->_int);
				case OP_CONV_FU:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((unsigned int)eval_a->_float != eval_a->_float)
					{
						if (eval_a->_int & 0x7f800000)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %i", eval_a->_float, (unsigned int)eval_a->_float);
						else
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Denormalized float %g will be rounded to %i. This is probably not what you want.", eval_a->_float, (unsigned int)eval_a->_float);
					}
					return QCC_MakeUIntConst(eval_a->_float);
				case OP_CONV_UF:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						float fl = eval_a->_uint;
						if ((unsigned int)fl != eval_a->_uint)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#x to %#x.", eval_a->_uint, (unsigned int)fl);
					}
					return QCC_MakeFloatConst(eval_a->_uint);

				case OP_CONV_FD:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeDoubleConst(eval_a->_float);
				case OP_CONV_DF:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						float fl = eval_a->_double;
//						if ((double)fl != eval_a->_double)
//							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %g to %g.", eval_a->_double, (float)fl);
						return QCC_MakeFloatConst(fl);
					}

				case OP_CONV_DI64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((pint64_t)eval_a->_double != eval_a->_double)
						QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %"pPRIi64"", eval_a->_double, (pint64_t)eval_a->_double);
					return QCC_MakeInt64Const(eval_a->_double);
				case OP_CONV_I64D:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						double d = eval_a->i64;
						if ((pint64_t)d != eval_a->i64)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#"pPRIx64" to %#"pPRIx64".", eval_a->i64, (pint64_t)d);
						return QCC_MakeDoubleConst(d);
					}
				case OP_CONV_DU64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((pint64_t)eval_a->_double != eval_a->_double)
						QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %"pPRIi64"", eval_a->_double, (puint64_t)eval_a->_double);
					return QCC_MakeUInt64Const(eval_a->_double);
				case OP_CONV_U64D:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						double d = eval_a->u64;
						if ((pint64_t)d != eval_a->u64)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#"pPRIx64" to %#"pPRIx64".", eval_a->u64, (puint64_t)d);
						return QCC_MakeDoubleConst(d);
					}
				case OP_CONV_FI64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((pint64_t)eval_a->_float != eval_a->_float)
						QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %"pPRIi64"", eval_a->_float, (pint64_t)eval_a->_float);
					return QCC_MakeInt64Const(eval_a->_float);
				case OP_CONV_I64F:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						float d = eval_a->i64;
						if ((pint64_t)d != eval_a->i64)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#"pPRIx64" to %#"pPRIx64".", eval_a->i64, (pint64_t)d);
						return QCC_MakeFloatConst(d);
					}
				case OP_CONV_FU64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					if ((puint64_t)eval_a->_float != eval_a->_float)
						QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical overflow. %f will be rounded to %"pPRIu64"", eval_a->_float, (puint64_t)eval_a->_float);
					return QCC_MakeUInt64Const(eval_a->_float);
				case OP_CONV_U64F:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						float d = eval_a->u64;
						if ((puint64_t)d != eval_a->u64)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %#"pPRIx64" to %#"pPRIx64".", eval_a->u64, (puint64_t)d);
						return QCC_MakeFloatConst(d);
					}
				case OP_CONV_II64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						pint64_t d = eval_a->_int;
						if ((pint_t)d != eval_a->_int)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %"pPRIi" to %#"pPRIx64".", eval_a->_int, (pint64_t)d);
						return QCC_MakeInt64Const(d);
					}
				case OP_CONV_UI64:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						puint64_t d = eval_a->_uint;
						if ((puint_t)d != eval_a->_uint)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %"pPRIu" to %#"pPRIx64".", eval_a->_uint, (pint64_t)d);
						return QCC_MakeUInt64Const(d);
					}
				case OP_CONV_I64I:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					{
						pint_t d = eval_a->i64;
						if ((pint64_t)d != eval_a->i64)
							QCC_PR_ParseWarning(WARN_OVERFLOW, "Numerical truncation of %"pPRIx64" to %#"pPRIx".", eval_a->i64, (pint_t)d);
						return QCC_MakeIntConst(d);
					}

				case OP_BITOR_F:
				case OP_ADD_F:
					if (eval_a->_float == 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_MUL_F:
					if (eval_a->_float == 1)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_BITAND_F:
				case OP_BITAND_FI:
					if (QCC_PR_RoundFloatConst(eval_a) == 0)
					{
						QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
						return QCC_MakeFloatConst(0);
					}
					break;
				case OP_LSHIFT_U:
				case OP_RSHIFT_U:
					if (eval_a->_uint == 0)
					{
						QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
						return QCC_MakeUIntConst(0);
					}
					break;
				case OP_LSHIFT_I:
				case OP_RSHIFT_I:
				case OP_BITAND_I:
				case OP_BITAND_IF:
					if (eval_a->_int == 0)
					{
						QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
						return QCC_MakeIntConst(0);
					}
					break;
				case OP_AND_ANY:
				case OP_AND_F:
				case OP_AND_FI:
				case OP_AND_I:
				case OP_AND_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?b:a);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
						else
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
					}*/
					if (!QCC_Eval_Truth(eval_a, var_a.cast, false))
					{	//one side is false, thus return false.
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						QCC_FreeTemp(var_b);
						return QCC_MakeFloatConst(0);
					}
					break;

				case OP_OR_ANY:
				case OP_OR_F:
				case OP_OR_FI:
				case OP_OR_I:
				case OP_OR_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?a:b);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
						else
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
					}*/
					if (QCC_Eval_Truth(eval_a, var_a.cast, false))
					{	//one side is true, thus return true.
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						QCC_FreeTemp(var_b);
						return QCC_MakeFloatConst(1);
					}
					break;
				case OP_BITOR_I:
				case OP_ADD_I:
					if (eval_a->_int == 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_MUL_I:
					if (eval_a->_int == 1)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				}
			}
		}
		else if (eval_b)
		{
			if (outstatement)
				*outstatement = NULL;
			//b is const, a is not
			switch (op - pr_opcodes)
			{
				case OP_GLOBALADDRESS:
					if (flag_pointerrelocs && !pr_scope)
						return QCC_MakeGAddress(type_pointer, var_a.sym, var_a.ofs + QCC_Eval_Int(QCC_SRef_EvalConst(var_b), var_b.cast), 0);
					break;
				case OP_AND_ANY:
				case OP_AND_F:
				case OP_AND_FI:
				case OP_AND_I:
				case OP_AND_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?b:a);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
						else
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
					} else*/
					if (!QCC_Eval_Truth(eval_b, var_b.cast, false))
					{	//one side is false, thus return false.
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						QCC_FreeTemp(var_b);
						return QCC_MakeFloatConst(0);
					}
					break;

				case OP_OR_ANY:
				case OP_OR_F:
				case OP_OR_FI:
				case OP_OR_I:
				case OP_OR_IF:
					/*if (flag_pythonlogic)
					{	//c=(a?a:b);
						optres_constantarithmatic++;
						if (QCC_Eval_Truth(eval_a, var_a.cast))
						{
							QCC_FreeTemp(var_b);
							return var_a;
						}
						else
						{
							QCC_FreeTemp(var_a);
							return var_b;
						}
					}*/
					if (QCC_Eval_Truth(eval_a, var_a.cast, false))
					{	//one side is true, thus return true.
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						QCC_FreeTemp(var_b);
						return QCC_MakeFloatConst(1);
					}
					break;


#if IAMNOTLAZY
			case OP_LOADA_F:
			case OP_LOADA_V:
			case OP_LOADA_S:
			case OP_LOADA_ENT:
			case OP_LOADA_FLD:
			case OP_LOADA_FNC:
			case OP_LOADA_I:
				{
					QCC_def_t *nd;
					nd = (void *)qccHunkAlloc (sizeof(QCC_def_t));
					memset (nd, 0, sizeof(QCC_def_t));
					nd->type = var_a->type;
					nd->ofs = var_a->ofs + G_INT(var_b->ofs);
					nd->temp = var_a->temp;
					nd->constant = false;
					nd->name = var_a->name;
					return nd;
				}
				break;
#endif
			case OP_BITOR_F:
			case OP_SUB_F:
			case OP_ADD_F:
				if (eval_b->_float == 0)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_DIV_VF:
				if (!eval_b->_float)
					QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by 0\n");
				else if (flag_reciprocalmaths && eval_b->_int&(0xff<<23))
				{
					QCC_FreeTemp(var_b);
					var_b = QCC_MakeFloatConst(1.0 / eval_b->_float);
					return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_VF], var_a, var_b, outstatement, 0);
				}
				if (eval_b->_float == 1)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;

			case OP_DIV_F:
			case OP_DIV_IF:
				if (!eval_b->_float)
					QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by 0\n");
				else if (flag_reciprocalmaths && (eval_b->_int&(0xff<<23)) && eval_b->_int != 3/*paranoid about vector indexing. don't ruin precision*/)
				{
					QCC_FreeTemp(var_b);
					var_b = QCC_MakeFloatConst(1.0 / eval_b->_float);
					return QCC_PR_StatementFlags((op == &pr_opcodes[OP_DIV_F])?&pr_opcodes[OP_MUL_F]:&pr_opcodes[OP_MUL_IF], var_a, var_b, outstatement, 0);
				}
				//fallthrough
			case OP_MUL_F:
			case OP_MUL_IF:
				if (eval_b->_float == 1)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_BITOR_I:
			case OP_SUB_I:
			case OP_ADD_I:
			case OP_ADD_PIW:
				if (eval_b->_int == 0)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_DIV_I:
			case OP_DIV_U:
			case OP_DIV_FI:
				if (!eval_b->_int)
					QCC_PR_ParseWarning(WARN_DIVISIONBY0, "Division by 0\n");
			case OP_MUL_I:
			case OP_MUL_FI:
				if (eval_b->_int == 1)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_BITAND_I:
				if (eval_b->_int == ~0)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;

			case OP_BITEXTEND_I:
				if (eval_b->_uint == 32)
				{
					QCC_FreeTemp(var_b);
					return var_a;	//wtf?
				}
				if ((eval_b->_uint&0xff) == 0)
				{
					QCC_FreeTemp(var_b);
					QCC_FreeTemp(var_a);
					return QCC_MakeIntConst(0);	//wtf?
				}
				break;
			case OP_BITEXTEND_U:
				if (eval_b->_uint == 32)
				{
					QCC_FreeTemp(var_b);
					return var_a;	//wtf?
				}
				if ((eval_b->_uint&0xff) == 0)
				{
					QCC_FreeTemp(var_b);
					QCC_FreeTemp(var_a);
					return QCC_MakeUIntConst(0);	//wtf?
				}
				break;
			}
		}
	}

	// self-comparison that is impacted when NaN
	// e.g. NaN == NaN, NaN != NaN, [NaN, 0, 0] == [NaN, 0, 0], etc.
	nan_eq_cond = false;

	switch (op - pr_opcodes)
	{
	case OP_STATE:
		if (qcc_framerate>0 && qcc_framerate != (qcc_targetformat_ishexen2()?20:10))
		{	//can't use the normal opcode.
			QCC_ref_t tempref;
			QCC_sref_t self = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
			QCC_sref_t time = QCC_PR_GetSRef(type_float, "time", NULL, true, 0, false);
			QCC_sref_t fldframe = QCC_PR_GetSRef(type_floatfield, "frame", NULL, true, 0, false);
			QCC_sref_t fldthink = QCC_PR_GetSRef(QCC_PR_FieldType(type_function), "think", NULL, true, 0, false);
			QCC_sref_t fldnextthink = QCC_PR_GetSRef(type_floatfield, "nextthink", NULL, true, 0, false);

			QCC_UnFreeTemp(self);
			QCC_UnFreeTemp(self);

			//self.frame = var_a;
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD,	self,
				fldframe, fldframe.cast->aux_type,
				false, 0), var_a, false, false);

			//self.think = var_b;
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD, self,
				fldthink, fldthink.cast->aux_type,
				false, 0), var_b, false, false);

			//self.frame = time + interval;
			time = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], time, QCC_MakeFloatConst(1/qcc_framerate), NULL);
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD, self,
				fldnextthink, fldnextthink.cast->aux_type,
				false, 0), time, false, false);
			return nullsref;
		}
		break;
	case OP_WSTATE:
		{	//there is no normal opcode.
			QCC_ref_t tempref;
			QCC_sref_t self = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
			QCC_sref_t time = QCC_PR_GetSRef(type_float, "time", NULL, true, 0, false);
			QCC_sref_t fldframe = QCC_PR_GetSRef(type_floatfield, "weaponframe", NULL, true, 0, false);
			QCC_sref_t fldthink = QCC_PR_GetSRef(QCC_PR_FieldType(type_function), "think", NULL, true, 0, false);
			QCC_sref_t fldnextthink = QCC_PR_GetSRef(type_floatfield, "nextthink", NULL, true, 0, false);

			float framerate = (qcc_framerate>0)?qcc_framerate:(qcc_targetformat_ishexen2()?20:10);

			QCC_UnFreeTemp(self);
			QCC_UnFreeTemp(self);

			//self.frame = var_a;
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD,	self,
				fldframe, fldframe.cast->aux_type,
				false, 0), var_a, false, false);

			//self.think = var_b;
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD, self,
				fldthink, fldthink.cast->aux_type,
				false, 0), var_b, false, false);

			//self.frame = time + interval;
			time = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], time, QCC_MakeFloatConst(1/framerate), NULL);
			QCC_StoreSRefToRef(QCC_PR_BuildRef(&tempref, REF_FIELD, self,
				fldnextthink, fldnextthink.cast->aux_type,
				false, 0), time, false, false);
			return nullsref;
		}
		break;

	case OP_STORE_F:
	case OP_STORE_V:
	case OP_STORE_FLD:
	case OP_STORE_P:
	case OP_STORE_I:
	case OP_STORE_ENT:
	case OP_STORE_FNC:
	case OP_STORE_I64:
//	case OP_STORE_D:
		{
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(var_a);
			if (idxeval && (var_a.cast->type == ev_integer || var_a.cast->type == ev_float) && !idxeval->_int)
			{
				//you're allowed to assign 0i to anything
				if (op - pr_opcodes == OP_STORE_V)	//make sure vectors get set properly.
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeVectorConst(0, 0, 0);
				}
				if (op - pr_opcodes == OP_STORE_I64)	//make sure vectors get set properly.
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeInt64Const(0);
				}
			}
			/*else
			{
				QCC_type_t *t = var_a->type;
				while(t)
				{
					if (!typecmp_lax(t, var_b->type))
						break;
					t = t->parentclass;
				}
				if (!t)
				{
					TypeName(var_a->type, typea, sizeof(typea));
					TypeName(var_b->type, typeb, sizeof(typeb));
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "Implicit assignment from %s to %s %s", typea, typeb, var_b->name);
				}
			}*/
		}
		break;

	case OP_STOREP_F:
	case OP_STOREP_V:
	case OP_STOREP_FLD:
	case OP_STOREP_P:
	case OP_STOREP_I:
	case OP_STOREP_ENT:
	case OP_STOREP_FNC:
		{
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(var_a);
			if (idxeval && (var_a.cast->type == ev_integer || var_a.cast->type == ev_float) && !idxeval->_int)
			{
				//you're allowed to assign 0i to anything
				if (op - pr_opcodes == OP_STOREP_V)	//make sure vectors get set properly.
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeVectorConst(0, 0, 0);
				}
			}
			/*else
			{
				QCC_type_t *t = var_a->type;
				while(t)
				{
					if (!typecmp_lax(t, var_b->type->aux_type))
						break;
					t = t->parentclass;
				}
				if (!t)
				{
					TypeName(var_a->type, typea, sizeof(typea));
					TypeName(var_b->type->aux_type, typeb, sizeof(typeb));
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "Implicit field assignment from %s to %s", typea, typeb);
				}
			}*/ 
		}
		break;

	case OP_LOADA_F:
	case OP_LOADA_V:
	case OP_LOADA_S:
	case OP_LOADA_ENT:
	case OP_LOADA_FLD:
	case OP_LOADA_FNC:
	case OP_LOADA_I:
		break;
	case OP_AND_F:
		if (var_a.sym == var_b.sym && var_a.ofs == var_b.ofs)
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Parameter offsets for && are the same");
		if (var_a.sym && var_b.sym && (var_a.sym->constant && var_b.sym->constant))
		{
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_a.sym);
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_b.sym);
		}
		break;
	case OP_OR_F:
		if (var_a.sym == var_b.sym && var_a.ofs == var_b.ofs)
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Parameters for || are the same");
		if (var_a.sym && var_b.sym && (var_a.sym->constant || var_b.sym->constant))
		{
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_a.sym);
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_b.sym);
		}
		break;

	case OP_EQ_F:
	case OP_NE_F:
	case OP_EQ_V:
	case OP_NE_V:
	case OP_LE_F:
	case OP_GE_F:
		nan_eq_cond = true;
	case OP_EQ_S:
	case OP_EQ_E:
	case OP_EQ_FNC:
//		if (opt_shortenifnots)
//			if (var_b->constant && ((int*)qcc_pr_globals)[var_b->ofs]==0)	// (a == 0) becomes (!a)
//				op = &pr_opcodes[(op - pr_opcodes) - OP_EQ_F + OP_NOT_F];

	case OP_NE_S:
	case OP_NE_E:
	case OP_NE_FNC:

	case OP_LT_F:
	case OP_GT_F:
		if (typecmp_lax(var_a.cast, var_b.cast))
		{
			QCC_type_t *t;
			//simplify a, see if we can get an inherited comparison
			for (t = var_a.cast; t; t = t->parentclass)
			{
				if (typecmp_lax(t, var_b.cast))
					break;
			}
			if (t)
				break;
			//now try with b simplified
			for (t = var_b.cast; t; t = t->parentclass)
			{
				if (typecmp_lax(var_a.cast, t))
					break;
			}
			if (t)
				break;
			//if both need to simplify then the classes are too diverse
			TypeName(var_a.cast, typea, sizeof(typea));
			TypeName(var_b.cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "'%s' type mismatch: %s with %s", op->name, typea, typeb);
		}

		sym_cmp = !nan_eq_cond && var_a.sym == var_b.sym && var_a.ofs == var_b.ofs;

		if ((var_a.sym->constant && var_b.sym->constant && !var_a.sym->temp && !var_b.sym->temp) || sym_cmp)
		{
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_a.sym);
			QCC_PR_ParsePrintDef(WARN_CONSTANTCOMPARISON, var_b.sym);
			//Note: EQ_S and NE_S compares the pointed-to data, rather than the pointers themselves. it would be too unsafe to optimise these
			//fixme: fold other comparisons.
		}
		break;
	case OP_IF_S:
	case OP_IFNOT_S:
	case OP_IF_F:
	case OP_IFNOT_F:
	case OP_IF_I:
	case OP_IFNOT_I:
//		if (var_a.cast->type == ev_function && !var_a.sym->temp)
//			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
//		if (!var_a.sym || var_a.sym->constant && !var_a.sym->temp)
//			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
		break;
	default:
		break;
	}

	if (numstatements && !(statements[numstatements-1].flags&STF_NOFOLD))
	{	//optimise based on last statement.
		if (op - pr_opcodes == OP_IFNOT_I)
		{
			if (opt_shortenifnots && var_a.cast && var_a.sym->temp && var_a.sym->refcount == 1 && (statements[numstatements-1].op == OP_NOT_F || statements[numstatements-1].op == OP_NOT_FNC || statements[numstatements-1].op == OP_NOT_ENT))
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					if (statements[numstatements-1].op == OP_NOT_F && QCC_OPCodeValid(&pr_opcodes[OP_IF_F]))
						op = &pr_opcodes[OP_IF_F];
					else
						op = &pr_opcodes[OP_IF_I];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (op - pr_opcodes == OP_IFNOT_F)
		{
			if (opt_shortenifnots && var_a.cast && var_a.sym->temp && var_a.sym->refcount == 1 && statements[numstatements-1].op == OP_NOT_F)
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					op = &pr_opcodes[OP_IF_F];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (op - pr_opcodes == OP_IFNOT_S)
		{
			if (opt_shortenifnots && var_a.cast && var_a.sym->temp && var_a.sym->refcount == 1 && statements[numstatements-1].op == OP_NOT_S)
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					op = &pr_opcodes[OP_IF_S];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (((unsigned) ((op - pr_opcodes) - OP_STORE_F) < 6) || (op-pr_opcodes) == OP_STORE_P || (op-pr_opcodes) == OP_STORE_I || (op-pr_opcodes) == OP_STORE_I64)
		{
			// remove assignments if what should be assigned is the 3rd operand of the previous statement?
			// don't if it's a call, callH, switch or case
			// && var_a->ofs >RESERVED_OFS)
			if (OpAssignsToC(statements[numstatements-1].op) &&
			    opt_assignments && var_a.cast && var_a.sym == statements[numstatements-1].c.sym && var_a.ofs == statements[numstatements-1].c.ofs)
			{
				if (var_a.cast->type == var_b.cast->type)
				{
					if (var_a.sym && var_b.sym && var_a.sym->temp && var_a.sym->refcount==1)
					{
						statement = &statements[numstatements-1];
						statement->c = var_b;

						if (var_a.cast->type != var_b.cast->type)
							QCC_PR_ParseWarning(0, "store type mismatch");
						var_b.sym->referenced=true;
						var_a.sym->referenced=true;
						QCC_FreeTemp(var_a);
						optres_assignments++;

						if (flags&STFL_DISCARDRESULT)
						{
							QCC_FreeTemp(var_b);
							var_b = nullsref;
						}
						return var_b;
					}
				}
			}
		}
		else if (op - pr_opcodes == OP_ADD_I && statements[numstatements-1].op == OP_MUL_I && opt_assignments && !flag_undefwordsize)
		{
			//mul_i idx 4i tmp
			//add_i tmp 2i out
			//becomes add_piw 2i idx out

//			const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
			const QCC_eval_t *eval_b = QCC_SRef_EvalConst(statements[numstatements-1].b);

			if (eval_b && eval_b->_int == VMWORDSIZE)
			{
				if (var_a.cast && var_a.sym == statements[numstatements-1].c.sym && var_a.ofs == statements[numstatements-1].c.ofs)
					if (var_a.sym && var_b.sym && var_a.sym->temp && var_a.sym->refcount==1)
					{
						op = &pr_opcodes[OP_ADD_PIW];
						QCC_FreeDef(var_a.sym);
						var_a = var_b;
						var_b = statements[numstatements-1].a;
						numstatements--;
						QCC_ForceUnFreeDef(var_b.sym);
					}
			}
		}
	}

	if (!QCC_OPCodeValid(op) && !(flags&STFL_NOEMULATE))
	{
#define QCC_PR_EmulationFunc(n) QCC_PR_GetSRef(NULL, #n, pr_scope, false, 0, 0)
		QCC_sref_t tmp;
//FIXME: add support for flags so we don't corrupt temps
		switch(op - pr_opcodes)
		{
		case OP_LOADA_STRUCT:
			/*emit this anyway. if it reaches runtime then you messed up.
			this is valid only if you do &foo[0]*/
//			QCC_PR_ParseWarning(0, "OP_LOADA_STRUCT: cannot emulate");
			break;

		case OP_ADD_SF:
			var_c = QCC_PR_EmulationFunc(AddStringFloat);
			if (var_c.cast)
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, var_c, var_a, type_string, var_b, type_float);
			else
			{
				if (!var_a.sym->constant)
					QCC_PR_ParseWarning(WARN_STRINGOFFSET, "OP_ADD_SF: string+float may be unsafe");
				var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], var_a, var_b, NULL, 0);
			}
			var_c.cast = type_string;
			return var_c;

		case OP_ADD_EF:
		case OP_SUB_EF:
			if (flag_qccx)	//no implicit cast. qccx always uses denormalised floats everywhere.
				QCC_PR_ParseWarning(WARN_DENORMAL, "OP_ADD_EF: qccx entity offsets are unsafe, and denormals are unsafe");	
			else if (1)
			{	//slightly better defined.
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				return QCC_PR_StatementFlags(&pr_opcodes[(op==&pr_opcodes[OP_ADD_EF])?OP_ADD_EI:OP_SUB_EI], var_a, var_b, NULL, flags&STFL_PRESERVEA);
			}
			else
			{
				var_c = QCC_PR_EmulationFunc(nextent);
				if (!var_c.cast)
				{
					QCC_PR_ParseWarning(0, "the nextent builtin is not defined");
					goto badopcode;
				}
				QCC_PR_ParseWarning(WARN_DENORMAL, "OP_ADD_EF: denormals are unsafe");
				var_c = QCC_PR_GenerateFunctionCall1 (nullsref, var_c, QCC_MakeIntConst(0), type_entity);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, flags&STFL_PRESERVEB);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_c, var_b, NULL, 0);
				flags&=~STFL_PRESERVEB;
			}
			var_c = QCC_PR_StatementFlags(&pr_opcodes[(op==&pr_opcodes[OP_ADD_EF])?OP_ADD_IF:OP_SUB_F], var_a, var_b, NULL, flags);
			var_c.cast = type_entity;
			return var_c;

		case OP_ADD_EI:
		case OP_SUB_EI:
			if (flag_qccx)
				QCC_PR_ParseWarning(0, "qccx entity offsets are unsafe");	
			else
			{
				var_c = QCC_PR_EmulationFunc(nextent);
				if (!var_c.cast)
				{
					QCC_PR_ParseWarning(0, "the nextent builtin is not defined");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall1 (nullsref, var_c, QCC_MakeIntConst(0), type_entity);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_c, var_b, NULL, flags&STFL_PRESERVEB);
				flags&=~STFL_PRESERVEB;
			}
			var_c = QCC_PR_StatementFlags(&pr_opcodes[(op==&pr_opcodes[OP_ADD_EF])?OP_ADD_I:OP_SUB_I], var_a, var_b, NULL, flags);
			var_c.cast = type_entity;
			return var_c;

		case OP_ADD_SI:
		case OP_ADD_IS:
			QCC_PR_ParseWarning(WARN_STRINGOFFSET, "OP_ADD_SI: string+int may be unsafe");
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], var_a, var_b, NULL, 0);
			var_c.cast = type_string;
			return var_c;
		case OP_ADD_PF:
		case OP_ADD_FP:
		case OP_ADD_PI:
		case OP_ADD_IP:
		case OP_ADD_PU:
		case OP_ADD_UP:
			{
				QCC_type_t *t;
				var_c = (op == &pr_opcodes[OP_ADD_PF] || op == &pr_opcodes[OP_ADD_PI] || op == &pr_opcodes[OP_ADD_PU])?var_a:var_b;	//ptr
				var_b = (op == &pr_opcodes[OP_ADD_PF] || op == &pr_opcodes[OP_ADD_PI] || op == &pr_opcodes[OP_ADD_PU])?var_b:var_a;	//idx
				t = var_c.cast;
				if (op == &pr_opcodes[OP_ADD_FP] || op == &pr_opcodes[OP_ADD_PF])
					var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
				if (t->aux_type->type == ev_void)	//void* is treated as a byte type.
				{
					if (flag_undefwordsize)
						QCC_PR_ParseWarning(ERR_BADEXTENSION, "void* maths was disabled.");
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], var_c, var_b, NULL, 0);
				}
				else if (t->aux_type->bits)	//awkward bittyness
				{
					if (flag_undefwordsize)
						QCC_PR_ParseWarning(ERR_BADEXTENSION, "pointer* maths was disabled on this type.");
					if (t->aux_type->bits != 8)
						var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(t->aux_type->bits/8), NULL, 0);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], var_c, var_b, NULL, 0); //PIW doesn't make sense here.
				}
				else
				{
					var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(t->aux_type->size), NULL, 0);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], var_c, var_b, NULL, 0);
				}
				var_c.cast = t;
			}
			return var_c;
		case OP_SUB_PF:
		case OP_SUB_PI:
		case OP_SUB_PU:
			var_c = var_a;
			if (op == &pr_opcodes[OP_SUB_PF])
				var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
			if (var_c.cast->aux_type->type == ev_void)	//void* is treated as a byte type.
			{
				if (flag_undefwordsize)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "void* maths was disabled.");
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_c, var_b, NULL, 0);
			}
			else if (var_c.cast->aux_type->bits)	//awkward bittyness
			{
				if (flag_undefwordsize)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "pointer* maths was disabled on this type.");
				if (var_c.cast->aux_type->bits!=8)
					var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(var_c.cast->aux_type->bits/8), NULL, 0);	//negated. this is a subtract after all.
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_c, var_b, NULL, 0); //PIW doesn't make sense here.
			}
			else if (1)
			{
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(-var_c.cast->aux_type->size), NULL, 0);
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], var_c, var_b/*negative, making this a subtraction*/, NULL, 0);
			}
			else
			{
				QCC_sref_t idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], QCC_MakeIntConst(0), QCC_MakeIntConst(var_c.cast->aux_type->size), NULL);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, idx, NULL, 0);
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_c, var_b, NULL, 0);
			}
			var_c.cast = var_a.cast;
			return var_c;
		case OP_SUB_PP:
			if (typecmp(var_a.cast, var_b.cast))
				QCC_PR_ParseError(ERR_BADEXTENSION, "incompatible pointer types");
			//determine byte offset
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_a, var_b, NULL, 0);
			if (var_a.cast->aux_type->type == ev_void)
			{
				if (flag_undefwordsize)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "void* maths was disabled.");
				return var_c;	//we're done if we're using void/bytes
			}
			//determine divisor
			if (var_a.cast->aux_type->bits)
			{
				if (flag_undefwordsize)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "pointer sizes were marked as undefined.");
				var_b = QCC_MakeIntConst(var_a.cast->aux_type->bits/8);
			}
			else
				var_b = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], QCC_MakeIntConst(0), QCC_MakeIntConst(var_a.cast->aux_type->size), NULL);
			//divide the result
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], var_c, var_b, NULL, 0);

		case OP_BITAND_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(BitandInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "BitandInt function not defined: cannot emulate int&int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_BITOR_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(BitorInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "BitorInt function not defined: cannot emulate int|int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_BITAND_V:
		case OP_BITOR_V:
			op = &pr_opcodes[((op - pr_opcodes)==OP_BITAND_V)?OP_BITAND_F:OP_BITOR_F];
			var_c = QCC_GetTemp(type_vector);
			var_a.cast = type_float;
			var_b.cast = type_float;
			var_c.cast = type_float;
			QCC_PR_SimpleStatement(op, var_a, var_b, var_c, true);
			var_a.ofs++; var_b.ofs++; var_c.ofs++;
			QCC_PR_SimpleStatement(op, var_a, var_b, var_c, true);
			var_a.ofs++; var_b.ofs++; var_c.ofs++;
			QCC_PR_SimpleStatement(op, var_a, var_b, var_c, true);
			var_a.ofs++; var_b.ofs++; var_c.ofs++;
			var_c.cast = type_vector;
			var_c.ofs -= 3;
			return var_c;
		case OP_ADD_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(AddInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "AddInt function not defined: cannot emulate int+int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MOD_F:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(mod);
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_F], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate float%%float");
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_MOD_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(ModInt);
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
					//var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_I], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate int%%int");
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MOD_U:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(ModInt);
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_U], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
					//var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_U], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_U], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_U], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate int%%int");
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MOD_FI:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(mod);
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_FI], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_IF], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_F], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate float%%int");
				}
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_MOD_IF:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(mod);
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_IF], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_IF], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate int%%float");
				}
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_MOD_V:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(ModVec);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "ModVec function not defined: cannot emulate vector%%vector");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_vector, var_b, type_vector);
				var_c.cast = type_vector;
				return var_c;
			}
			break;
		case OP_SUB_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(SubInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "SubInt function not defined: cannot emulate int-int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MUL_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(MulInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "MulInt function not defined: cannot emulate int*int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_DIV_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(DivInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "DivInt function not defined: cannot emulate int/int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;

		case OP_DIV_VF:
			//v/f === v*(1/f)
			op = &pr_opcodes[OP_MUL_VF];
			var_b = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], QCC_MakeFloatConst(1), var_b, NULL);
			var_b.sym->referenced = true;
			break;

		case OP_POW_F:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "pow function not defined: cannot emulate float*^float");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_POW_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "pow function not defined: cannot emulate int*^int");
					goto badopcode;
				}
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_c, nullsref, NULL, 0);
				return var_c;
			}
			break;
		case OP_POW_FI:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "pow function not defined: cannot emulate float*^int");
					goto badopcode;
				}
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_POW_IF:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "pow function not defined: cannot emulate int*^float");
					goto badopcode;
				}
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_CROSS_V:
			{
				QCC_sref_t t;
				var_c = QCC_GetTemp(type_vector);

				t = QCC_PR_Statement(&pr_opcodes[OP_SUB_F], QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+1, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+2, type_float), NULL), QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+2, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+1, type_float), NULL), NULL);
				QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], t, QCC_MakeSRef(var_c.sym, var_c.ofs+0, type_float), NULL, flags&STFL_DISCARDRESULT);

				t = QCC_PR_Statement(&pr_opcodes[OP_SUB_F], QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+2, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+0, type_float), NULL), QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+0, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+2, type_float), NULL), NULL);
				QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], t, QCC_MakeSRef(var_c.sym, var_c.ofs+1, type_float), NULL, flags&STFL_DISCARDRESULT);

				t = QCC_PR_Statement(&pr_opcodes[OP_SUB_F], QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+0, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+1, type_float), NULL), QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_MakeSRef(var_a.sym, var_a.ofs+1, type_float), QCC_MakeSRef(var_b.sym, var_b.ofs+0, type_float), NULL), NULL);
				QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], t, QCC_MakeSRef(var_c.sym, var_c.ofs+2, type_float), NULL, flags&STFL_DISCARDRESULT);

				return var_c;
			}
			break;

		case OP_SPACESHIP_F:
			{	//basically just a subtraction-with-fsign.
				const QCC_eval_t *eval;
				QCC_statement_t *patch1;
				var_a = QCC_PR_Statement(&pr_opcodes[OP_SUB_F], var_a, var_b, NULL);
				eval = QCC_SRef_EvalConst(var_c);
				if (eval)
				{
					if (eval->_float < 0)
						return QCC_MakeFloatConst(-1);
					else
						return QCC_MakeFloatConst(eval->_float > 0);
				}

				//hack: make local, not temp. this prevents assignment/temp folding...
				var_c = QCC_MakeSRefForce(QCC_PR_DummyDef(type_float, "ternary", pr_scope, 0, NULL, 0, false, GDF_STRIP), 0, type_float);

				//var_c = a>0;
				QCC_PR_SimpleStatement(&pr_opcodes[OP_GT_F], var_a, QCC_MakeFloatConst(0), var_c, true);
				patch1 = QCC_Generate_OP_IFNOT(QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, QCC_MakeFloatConst(0), NULL, 0), false);
				QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(-1), var_c, nullsref, true);
				patch1->b.jumpofs = &statements[numstatements] - patch1;
				return var_c;
			}
			break;
		case OP_SPACESHIP_S:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(strcmp);
				if (!fnc.cast)
					QCC_PR_ParseError(0, "strcmp function not defined: cannot emulate string<=>string");
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_string, var_b, type_string);
				var_c.cast = type_float;
				return var_c;
			}
			break;

		case OP_CONV_UF:
			var_c = QCC_PR_EmulationFunc(utof);
			if (!var_c.cast)
				var_c = QCC_PR_EmulationFunc(quake_utof);
			if (var_c.cast)
			{
				var_a = QCC_PR_GenerateFunctionCall1(nullsref, var_c, var_a, type_uint);
				var_a.cast = type_float;
				return var_a;
			}
			var_c = QCC_PR_EmulationFunc(itof);
			if (!var_c.cast)
				var_c = QCC_PR_EmulationFunc(quake_itof);
			if (var_c.cast)
			{	//there's some optional args that actually have it treated as unsigned
				var_a = QCC_PR_GenerateFunctionCall3(nullsref, var_c, var_a,type_integer, QCC_MakeFloatConst(0),type_float, QCC_MakeFloatConst(32),type_float);
				var_a.cast = type_float;
				return var_a;
			}

			//urgh...
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			/*statement = QCC_Generate_OP_IFNOT(QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_c, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), false);
			if (statement)
			{
				QCC_PR_SimpleStatement(&pr_opcodes[OP_ADD_F], var_c, QCC_MakeFloatConst(0x100000000), var_c, false);	//biiig number, to overpower the negative part.
				statement->b.jumpofs = &statements[numstatements] - statement;
			}
			else*/
				QCC_PR_ParseWarning(WARN_NOTSTANDARDBEHAVIOUR, "utof emulation: will break if %s is big", var_a.sym->name);
			return var_c;

		case OP_CONV_FU:
			var_c = QCC_PR_EmulationFunc(ftou);
			if (!var_c.cast)
				return QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, var_b, NULL, flags&STFL_PRESERVEA);
			var_a = QCC_PR_GenerateFunctionCall1(nullsref, var_c, var_a, type_float);
			var_a.cast = type_uint;
			return var_a;
		case OP_CONV_ITOF:
		case OP_STORE_IF:
			{
				const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
				if (eval_a)
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeFloatConst(eval_a->_int);
				}
				else
				{
					var_c = QCC_PR_EmulationFunc(itof);
					if (!var_c.cast)
					{
						//with denormals, 5.0 * 1i -> 5i, and 5i / 1i = 5.0
						QCC_PR_ParseWarning(WARN_DENORMAL, "itof emulation: denormals are unsafe %s", var_a.sym->name);
						var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, QCC_MakeIntConst(1), NULL, 0);
					}
					else
						var_a = QCC_PR_GenerateFunctionCall1(nullsref, var_c, var_a, type_integer);
					var_a.cast = type_float;
				}
				if (var_b.cast)
					return QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], var_a, var_b, NULL, flags&STFL_DISCARDRESULT);
			}
			return var_a;
		case OP_CONV_FTOI:
		case OP_STORE_FI:
			{
				const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
				if (eval_a)
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeIntConst(eval_a->_float);
				}
				else
				{
					var_c = QCC_PR_EmulationFunc(ftoi);
					if (!var_c.cast)
					{
						//with denormals, 5 * 1i -> 5i
						QCC_PR_ParseWarning(WARN_DENORMAL, "ftoi emulation: denormals are unsafe");
						var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, QCC_MakeIntConst(1), NULL, 0);
					}
					else
						var_a = QCC_PR_GenerateFunctionCall1(nullsref, var_c, var_a, type_float);
					var_a.cast = type_integer;
				}
				if (var_b.cast)
					return QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], var_a, var_b, NULL, flags&STFL_DISCARDRESULT);
			}
			return var_a;
		case OP_STORE_P:
		case OP_STORE_I:
			op = pr_opcodes+OP_STORE_F;
			break;

		case OP_BITXOR_F:
//			r = (a & ~b) | (b & ~a);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_F], var_b, nullsref, NULL, STFL_PRESERVEA);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_c, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_F], var_a, nullsref, NULL, 0);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_b, var_a, NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_a, NULL, 0);

		case OP_BITXOR_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(BitxorInt);
				if (!fnc.cast)
				{
/*					QCC_PR_ParseWarning(0, "BitxorInt function not defined: cannot emulate int^int");
					goto badopcode;
*/
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I], var_b, nullsref, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_a, var_c, NULL, STFL_PRESERVEA);
					var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I], var_a, nullsref, NULL, 0);
					var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, var_a, NULL, 0);
					return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I], var_c, var_a, NULL, 0);
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;

		case OP_BITXOR_V:
//			r = (a & ~b) | (b & ~a);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_V], var_b, nullsref, NULL, STFL_PRESERVEA);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_V], var_a, var_c, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_V], var_a, nullsref, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_V], var_b, var_a, NULL, STFL_PRESERVEB);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_V], var_c, var_a, NULL, 0);

		case OP_BITXOR_D:
//			r = (a & ~b) | (b & ~a);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_D], var_b, nullsref, NULL, STFL_PRESERVEA);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_D], var_a, var_c, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_D], var_a, nullsref, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_D], var_b, var_a, NULL, STFL_PRESERVEB);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_D], var_c, var_a, NULL, 0);

		case OP_IF_S:
			tmp = QCC_MakeFloatConst(0);
			tmp.cast = type_string;
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_S], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IF_I];
			break;

		case OP_IFNOT_S:
			tmp = QCC_MakeFloatConst(0);
			tmp.cast = type_string;
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_S], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IFNOT_I];
			break;

		case OP_IF_F:
			tmp = QCC_MakeFloatConst(0);
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_F], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IF_I];
			break;

		case OP_IFNOT_F:
			tmp = QCC_MakeFloatConst(0);
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_F], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IFNOT_I];
			break;

		case OP_ADDSTORE_F:
			op = &pr_opcodes[OP_ADD_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_ADDSTORE_I:
			op = &pr_opcodes[OP_ADD_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_ADDSTORE_FI:
			op = &pr_opcodes[OP_ADD_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_ADDSTORE_IF:
//			fixme: result is a float but needs to be an int
//			op = &pr_opcodes[OP_ADD_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;

		case OP_SUBSTORE_F:
			op = &pr_opcodes[OP_SUB_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_SUBSTORE_FI:
			op = &pr_opcodes[OP_SUB_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_SUBSTORE_IF:
//			fixme: result is a float but needs to be an int
//			op = &pr_opcodes[OP_SUB_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;
		case OP_SUBSTORE_I:
			op = &pr_opcodes[OP_SUB_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_BITNOT_I:
			op = &pr_opcodes[OP_SUB_I];
			if (QCC_OPCodeValid(op))
			{
				op = &pr_opcodes[OP_SUB_I];
				var_b = var_a;
				var_a = QCC_MakeIntConst(~0);
				var_a.sym->referenced = true;
			}
			else
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(SubInt);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "SubInt function not defined: cannot emulate ~int");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, QCC_MakeIntConst(~0), type_integer, var_a, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_BITNOT_I64:
			op = &pr_opcodes[OP_SUB_I64];
			var_b = var_a;
			var_a = QCC_MakeInt64Const(~(longlong)0);
			var_a.sym->referenced = true;
			break;
		case OP_BITNOT_F:
			op = &pr_opcodes[OP_SUB_F];
			var_b = var_a;
			var_a = QCC_MakeFloatConst(-1);	//divVerent says -1 is safe, even with floats. I guess I'm just too paranoid.
			var_a.sym->referenced = true;
			break;
		case OP_BITNOT_V:
			op = &pr_opcodes[OP_SUB_V];
			var_b = var_a;
			var_a = QCC_MakeVectorConst(-1, -1, -1);
			var_a.sym->referenced=true;
			break;

		case OP_DIVSTORE_F:
			op = &pr_opcodes[OP_DIV_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_DIVSTORE_FI:
			op = &pr_opcodes[OP_DIV_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_DIVSTORE_IF:
//			fixme: result is a float, but needs to be an int
//			op = &pr_opcodes[OP_DIV_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;
		case OP_DIVSTORE_I:
			op = &pr_opcodes[OP_DIV_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_MULSTORE_F:
			op = &pr_opcodes[OP_MUL_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_MULSTORE_IF:
//			fixme: result is a float, but needs to be an int
//			op = &pr_opcodes[OP_MUL_IF];
//			var_c = var_b;
//			var_b = var_a;
//			var_a = var_c;
//			break;
		case OP_MULSTORE_FI:
			op = &pr_opcodes[OP_MUL_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_ADDSTORE_V:
			op = &pr_opcodes[OP_ADD_V];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_SUBSTORE_V:
			op = &pr_opcodes[OP_SUB_V];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_MULSTORE_VF:
			op = &pr_opcodes[OP_MUL_VF];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_MULSTORE_VI:
			op = &pr_opcodes[OP_MUL_VI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_BITSETSTORE_I:
			op = &pr_opcodes[OP_BITOR_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_BITSETSTORE_F:
			op = &pr_opcodes[OP_BITOR_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_LOAD_P:
		case OP_LOAD_I:
			op = &pr_opcodes[OP_LOAD_F];
			break;
		case OP_STOREP_P:
			op = &pr_opcodes[OP_STOREP_I];
			break;

		case OP_EQ_P:
			op = &pr_opcodes[OP_EQ_E];
			break;
		case OP_NE_P:
			op = &pr_opcodes[OP_NE_E];
			break;

		case OP_GT_P:
			op = &pr_opcodes[OP_GT_I];
			break;
		case OP_GE_P:
			op = &pr_opcodes[OP_GE_I];
			break;
		case OP_LE_P:
			op = &pr_opcodes[OP_LE_I];
			break;
		case OP_LT_P:
			op = &pr_opcodes[OP_LT_I];
			break;

		case OP_BITCLR_I64:
			//b = var, a = bit field.
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I64], var_a, var_b, NULL, STFL_PRESERVEA);
			var_c = nullsref;
			op = &pr_opcodes[OP_SUB_I64];
			break;

		case OP_BITCLR_I:
			if (QCC_OPCodeValid(&pr_opcodes[OP_BITCLRSTORE_I]))
			{
				op = &pr_opcodes[OP_BITCLRSTORE_I];
				break;
			}
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			//fallthrough
		case OP_BITCLRSTORE_I:
			//b = var, a = bit field.
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, var_a, NULL, STFL_PRESERVEA);

			var_a = var_b;
			var_b = var_c;
			var_c = ((op - pr_opcodes)==OP_BITCLRSTORE_I)?var_a:nullsref;
			op = &pr_opcodes[OP_SUB_I];
			break;
		case OP_BITCLR_V:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_V], var_a, var_b, NULL, STFL_PRESERVEA);
			op = &pr_opcodes[OP_SUB_V];
			var_c = nullsref;
			break;
		case OP_BITCLR_D:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_D], var_a, var_b, NULL, STFL_PRESERVEA);
			op = &pr_opcodes[OP_SUB_D];
			var_c = nullsref;
			break;

		case OP_BITCLR_F:
			var_c = var_b;
			var_b = var_a;
			var_a = var_c;
			if (QCC_OPCodeValid(&pr_opcodes[OP_BITCLRSTORE_F]))
			{	//its all okay, just use it
				op = &pr_opcodes[OP_BITCLRSTORE_F];
				break;
			}
			//fallthrough
		case OP_BITCLRSTORE_F:
			//b = var, a = bit field.

			//b - (b&a)

			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_b, var_a, NULL, STFL_PRESERVEA);

			var_a = var_b;
			var_b = var_c;
			var_c = ((op - pr_opcodes)==OP_BITCLRSTORE_F)?var_a:nullsref;
			op = &pr_opcodes[OP_SUB_F];
			break;
#if IAMNOTLAZY
		case OP_SUBSTOREP_FI:
		case OP_SUBSTOREP_IF:
		case OP_ADDSTOREP_FI:
		case OP_ADDSTOREP_IF:
		case OP_MULSTOREP_FI:
		case OP_MULSTOREP_IF:
		case OP_DIVSTOREP_FI:
		case OP_DIVSTOREP_IF:

		case OP_MULSTOREP_VF:
		case OP_MULSTOREP_VI:
		case OP_SUBSTOREP_V:
		case OP_ADDSTOREP_V:

		case OP_SUBSTOREP_F:
		case OP_SUBSTOREP_I:
		case OP_ADDSTOREP_I:
		case OP_ADDSTOREP_F:
		case OP_MULSTOREP_F:
		case OP_DIVSTOREP_F:
		case OP_BITSETSTOREP_F:
		case OP_BITSETSTOREP_I:
		case OP_BITCLRSTOREP_F:
//			QCC_PR_ParseWarning(0, "XSTOREP_F emulation is still experimental");
			QCC_UnFreeTemp(var_a);
			QCC_UnFreeTemp(var_b);

			statement = &statements[numstatements++];

			//don't chain these... this expansion is not the same.
			{
				int st;
				pbool need_lock;
				st = QCC_PR_FindSourceForTemp(var_b, OP_ADDRESS, &need_lock);

				var_c = QCC_GetTemp(*op->type_c);
				if (st < 0)
				{
					/*generate new OP_LOADP instruction*/
					statement->op = ((*op->type_c)->type==ev_vector)?OP_LOADP_V:OP_LOADP_F;
					statement->a = var_b->ofs;
					statement->b = var_c->ofs;
					statement->c = 0;
				}
				else
				{
					/*it came from an OP_ADDRESS - st says the instruction*/
					if (need_lock)
					{
						QCC_ForceLockTempForOffset(statements[st].a);
						QCC_ForceLockTempForOffset(statements[st].b);
//						QCC_LockTemp(var_c); /*that temp needs to be preserved over calls*/
					}

					/*generate new OP_ADDRESS instruction - FIXME: the arguments may have changed since the original instruction*/
					statement->op = OP_ADDRESS;
					statement->flags = 0;
					statement->a = statements[st].a;
					statement->b = statements[st].b;
					statement->c = var_c->ofs;
					statement->linenum = statements[st].linenum;

					/*convert old one to an OP_LOAD*/
					statements[st].op = ((*op->type_c)->type==ev_vector)?OP_LOAD_V:OP_LOAD_F;
					statement->flags = 0;
//					statements[st].a = statements[st].a;
//					statements[st].b = statements[st].b;
//					statements[st].c = statements[st].c;
					statements[st].linenum = pr_token_line_last;
				}
			}

			statement = &statements[numstatements++];

			statement->linenum = pr_token_line_last;
			switch(op - pr_opcodes)
			{
			case OP_SUBSTOREP_V:
				statement->op = OP_SUB_V;
				break;
			case OP_ADDSTOREP_V:
				statement->op = OP_ADD_V;
				break;
			case OP_MULSTOREP_VF:
				statement->op = OP_MUL_VF;
				break;
			case OP_MULSTOREP_VI:
				statement->op = OP_MUL_VI;
				break;
			case OP_SUBSTOREP_F:
				statement->op = OP_SUB_F;
				break;
			case OP_SUBSTOREP_I:
				statement->op = OP_SUB_I;
				break;
			case OP_SUBSTOREP_IF:
				statement->op = OP_SUB_IF;
				break;
			case OP_SUBSTOREP_FI:
				statement->op = OP_SUB_FI;
				break;
			case OP_ADDSTOREP_IF:
				statement->op = OP_ADD_IF;
				break;
			case OP_ADDSTOREP_FI:
				statement->op = OP_ADD_FI;
				break;
			case OP_MULSTOREP_IF:
				statement->op = OP_MUL_IF;
				break;
			case OP_MULSTOREP_FI:
				statement->op = OP_MUL_FI;
				break;
			case OP_DIVSTOREP_IF:
				statement->op = OP_DIV_IF;
				break;
			case OP_DIVSTOREP_FI:
				statement->op = OP_DIV_FI;
				break;
			case OP_ADDSTOREP_F:
				statement->op = OP_ADD_F;
				break;
			case OP_ADDSTOREP_I:
				statement->op = OP_ADD_I;
				break;
			case OP_MULSTOREP_F:
				statement->op = OP_MUL_F;
				break;
			case OP_DIVSTOREP_F:
				statement->op = OP_DIV_F;
				break;
			case OP_BITSETSTOREP_F:
				statement->op = OP_BITOR_F;
				break;
			case OP_BITSETSTOREP_I:
				statement->op = OP_BITOR_I;
				break;
			case OP_BITCLRSTOREP_F:
				//float pointer float
				temp = QCC_GetTemp(type_float);
				statement->op = OP_BITAND_F;
				statement->flags = 0;
				statement->a = var_c ? var_c->ofs : 0;
				statement->b = var_a ? var_a->ofs : 0;
				statement->c = temp->ofs;

				statement = &statements[numstatements];
				numstatements++;

				statement->linenum = pr_token_line_last;
				statement->op = OP_SUB_F;
				statement->flags = 0;

				//t = c & i
				//c = c - t
				break;
			default:	//no way will this be hit...
				QCC_PR_ParseError(ERR_INTERNAL, "opcode invalid 3 times %i", op - pr_opcodes);
			}
			if (op - pr_opcodes == OP_BITCLRSTOREP_F)
			{
				statement->a = var_b ? var_b->ofs : 0;
				statement->b = temp ? temp->ofs : 0;
				statement->c = var_b->ofs;
				QCC_FreeTemp(temp);
				QCC_FreeTemp(var_a);
				var_a = var_b;	//this is the value.
				var_b = var_c;	//this is the ptr.
			}
			else
			{
				statement->a = var_b ? var_b->ofs : 0;
				statement->b = var_a ? var_a->ofs : 0;
				statement->c = var_b->ofs;
				QCC_FreeTemp(var_a);
				var_a = var_b;	//this is the value.
				var_b = var_c;	//this is the ptr.
			}

			op = &pr_opcodes[((*op->type_c)->type==ev_vector)?OP_STOREP_V:OP_STOREP_F];
			QCC_FreeTemp(var_c);
			var_c = NULL;
			QCC_FreeTemp(var_b);
			break;
#endif

		case OP_LSHIFT_IF:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_LSHIFT_FI:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_RSHIFT_IF:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_RSHIFT_FI:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, var_b, NULL, flags&STFL_PRESERVEB);

		case OP_LSHIFT_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(LShiftInt);
				if (!fnc.cast)
				{
					const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
					if (eval_b)
						var_b = QCC_MakeIntConst(1<<eval_b->_int);
					else
						var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_POW_I], QCC_MakeIntConst(2), var_b, NULL, flags&STFL_PRESERVEB);
					return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_a, var_b, NULL, flags&STFL_PRESERVEA);
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_RSHIFT_I:	//C leaves it undefined whether OP_RSHIFT_I is _I or actually _U
			{
				const QCC_eval_t *eval_b;
				QCC_sref_t fnc = QCC_PR_EmulationFunc(RShiftInt);
				if (fnc.cast && strcmp(fnc.sym->name, pr_scope->name))
				{
					var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_integer, var_b, type_integer);
					var_c.cast = type_integer;
					return var_c;
				}
				//fix up the rhs of the shift to something we can actually use.
				eval_b = QCC_SRef_EvalConst(var_b);
				if (eval_b)
				{
					if (!(flags&STFL_PRESERVEB))
						QCC_FreeTemp(var_b);	//done with it now
					var_b = QCC_MakeIntConst(1<<eval_b->_int);
				}
				else
					var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_POW_I], QCC_MakeIntConst(2), var_b, NULL, flags&STFL_PRESERVEB);
				if (QCC_OPCodeValid(&pr_opcodes[OP_DIV_U]))
					op = &pr_opcodes[OP_DIV_U];
				else
					op = &pr_opcodes[OP_DIV_I];	//might be quirky when the high bit is set in the divisor
				//jump-if->=0
				statement = QCC_Generate_OP_IFNOT(QCC_PR_StatementFlags(&pr_opcodes[OP_LT_I], var_a, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), false);
				//negative. we need to do this explicitly to avoid rounding
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I], var_a, nullsref, NULL, STFL_PRESERVEA);	//flip it
				var_c = QCC_PR_StatementFlags(op, var_c, var_b, NULL, STFL_PRESERVEB);		//shift it (zero-extend)...
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I], var_c, nullsref, NULL, 0);				//now flip that.
				//else
				statement->b.jumpofs = &statements[numstatements+1] - statement;//jump to after the following goto
				statement = QCC_Generate_OP_GOTO();
				//positive
				QCC_PR_SimpleStatement(op, var_a, var_b, var_c, false)->flags|=STF_NOFOLD;	//can just shift into the same temp the negative form would have produced..
				//end
				statement->a.jumpofs = &statements[numstatements] - statement;
				QCC_FreeTemp(var_b);	//done with it now
				if (!(flags&STFL_PRESERVEA))
					QCC_FreeTemp(var_a);	//done with it now
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_RSHIFT_U:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(RShiftUInt);
				if (!fnc.cast)
				{
					const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
					if (eval_b)
						var_b = QCC_MakeUIntConst(1<<eval_b->_uint);
					else
						var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_POW_I], QCC_MakeUIntConst(2), var_b, NULL, flags&STFL_PRESERVEB);
					return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_U], var_a, var_b, NULL, flags&STFL_PRESERVEA);
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_uint, var_b, type_uint);
				var_c.cast = type_uint;
				return var_c;
			}
			break;

		case OP_LSHIFT_DI:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I64I], var_a, var_b, NULL, 0);
		case OP_RSHIFT_DI:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I64I], var_a, var_b, NULL, 0);

		//convert both to ints
		case OP_LSHIFT_F:
			if (QCC_OPCodeValid(&pr_opcodes[OP_LSHIFT_I]))
			{
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				return QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, var_b, NULL, 0);
			}
			else
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (fnc.cast)
				{	//a<<b === floor(a*pow(2,b))
					QCC_sref_t fnc2 = QCC_PR_EmulationFunc(floor);
					if (fnc2.cast)
					{
						var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, QCC_MakeFloatConst(2), type_float, var_b, type_float);
						var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, var_c, NULL, flags&STFL_PRESERVEA);
						var_c = QCC_PR_GenerateFunctionCall1(nullsref, fnc2, var_c, type_float);
						return var_c;
					}
				}
				fnc = QCC_PR_EmulationFunc(bitshift);
				if (fnc.cast)
				{	//warning: DP's implementation of bitshift is fucked, hence why we favour the more expensibe floor+pow above.
					var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
					var_c.cast = *op->type_c;
					return var_c;
				}
				QCC_PR_ParseWarning(0, "bitshift function not defined: cannot emulate OP_LSHIFT_F*");
				goto badopcode;
			}
		case OP_RSHIFT_F:
			if (QCC_OPCodeValid(&pr_opcodes[OP_RSHIFT_I]))
			{
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
				return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, var_b, NULL, 0);
			}
			else
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(powf);
				if (!fnc.cast)
					fnc = QCC_PR_EmulationFunc(pow);
				if (fnc.cast)
				{	//a<<b === floor(a/pow(2,b))
					QCC_sref_t fnc2 = QCC_PR_EmulationFunc(floor);
					if (fnc2.cast)
					{
						var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, QCC_MakeFloatConst(2), type_float, var_b, type_float);
						var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_c, NULL, flags&STFL_PRESERVEA);
						var_c = QCC_PR_GenerateFunctionCall1(nullsref, fnc2, var_c, type_float);
						return var_c;
					}
				}
				fnc = QCC_PR_EmulationFunc(bitshift);
				if (fnc.cast)
				{	//warning: DP's implementation of bitshift is fucked, hence why we favour the more expensibe floor+pow above
					var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], QCC_MakeFloatConst(0), var_b, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEB:0);
					var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_float, var_b, type_float);
					var_c.cast = *op->type_c;
					return var_c;
				}
				QCC_PR_ParseWarning(0, "bitshift function not defined: cannot emulate OP_RSHIFT_F*");
				goto badopcode;
			}

		case OP_BITEXTEND_I:
			{
				const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
				if (eval_b)
				{
					int bits = eval_b->_uint&0xff;
					int bitofs = eval_b->_uint>>8;
					var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, QCC_MakeIntConst(32-bits-bitofs), NULL, flags&STFL_PRESERVEA);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, QCC_MakeIntConst(32-bits), NULL, flags&STFL_PRESERVEA);
					return var_c;
				}
			}
			goto badopcode;
		case OP_BITEXTEND_U:
			{
				const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
				if (eval_b)
				{
					int bits = eval_b->_uint&0xff;
					int bitofs = eval_b->_uint>>8;
					var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_U], var_a, QCC_MakeIntConst(32-bits-bitofs), NULL, flags&STFL_PRESERVEA);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_U], var_a, QCC_MakeIntConst(32-bits), NULL, flags&STFL_PRESERVEA);
					return var_c;
				}
			}
			goto badopcode;

		case OP_BITAND_D:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I64], var_a, var_b, NULL, 0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_I64D], var_c, nullsref, NULL, 0);	//grr
			return var_c;
		case OP_BITOR_D:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_DI64], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I64], var_a, var_b, NULL, 0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_I64D], var_c, nullsref, NULL, 0);	//grr
			return var_c;

		case OP_LOAD_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_GetTemp(type_int64);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_LOAD_FLD], var_a, var_b, var_c, false);
			var_b.ofs++;
			var_c.ofs++;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_LOAD_FLD], var_a, var_b, var_c, false);
			var_c.ofs--;
			return var_c;
		case OP_BITAND_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_GetTemp(type_int64);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITAND_I], var_a, var_b, var_c, false);
			var_a.ofs++;
			var_b.ofs++;
			var_c.ofs++;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITAND_I], var_a, var_b, var_c, false);
			var_c.ofs--;
			return var_c;
		case OP_BITOR_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_GetTemp(type_int64);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITOR_I], var_a, var_b, var_c, false);
			var_a.ofs++;
			var_b.ofs++;
			var_c.ofs++;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITOR_I], var_a, var_b, var_c, false);
			var_c.ofs--;
			return var_c;
		case OP_BITXOR_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_GetTemp(type_int64);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITXOR_I], var_a, var_b, var_c, false);
			var_a.ofs++;
			var_b.ofs++;
			var_c.ofs++;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_BITXOR_I], var_a, var_b, var_c, false);
			var_c.ofs--;
			return var_c;
		case OP_EQ_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_I], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
			var_a.ofs++;
			var_b.ofs++;
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_I], var_a, var_b, NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_AND_I], var_c, var_a, NULL, 0);
		case OP_NE_I64:
			var_a.cast = type_integer;
			var_b.cast = type_integer;
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_I], var_a, var_b, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
			var_a.ofs++;
			var_b.ofs++;
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_I], var_a, var_b, NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_OR_I], var_c, var_a, NULL, 0);

		//statements where the rhs is an input int and can be swapped with a float
		case OP_ADD_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_SUB_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_BITAND_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_BITOR_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_LT_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_LE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_GT_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_GE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_EQ_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_NE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_DIV_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_MUL_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_MUL_VI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_VF], var_a, var_b, NULL, flags&STFL_PRESERVEA);

		//statements where the lhs is an input int and can be swapped with a float
		case OP_ADD_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_SUB_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_BITAND_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_BITOR_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LT_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GT_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_EQ_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_NE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_DIV_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_MUL_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_MUL_IV:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_FV], var_a, var_b, NULL, flags&STFL_PRESERVEB);

		//statements where both sides will need to be converted to floats to work

		case OP_LE_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GE_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LT_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GT_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_EQ_FLD:
		case OP_EQ_I:
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_FNC], var_a, var_b, NULL, flags&(STFL_PRESERVEA|STFL_PRESERVEB));
		case OP_NE_FLD:
		case OP_NE_I:
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_FNC], var_a, var_b, NULL, flags&(STFL_PRESERVEA|STFL_PRESERVEB));
		case OP_NOT_I:
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_FNC], var_a, QCC_MakeIntConst(0), NULL, flags&(STFL_PRESERVEA));

		case OP_AND_I:
		case OP_AND_FI:
		case OP_AND_IF:
		case OP_AND_ANY:
			if (var_a.cast->type == ev_vector && flag_vectorlogic)	//we can do a dot-product to test if a vector has a value, instead of a double-not
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_a, var_a, NULL, STFL_PRESERVEA | (flags&STFL_PRESERVEA?STFL_PRESERVEB:0));
			if (var_b.cast->type == ev_vector && flag_vectorlogic)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_b, var_b, NULL, STFL_PRESERVEA | (flags&STFL_PRESERVEB?STFL_PRESERVEB:0));

			if (((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring)) && ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring)))
			{	//just 3 extra instructions instead of 4.
				var_a = QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value");
				var_b = QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value");
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_OR_ANY], var_a, var_b, NULL, 0);
				return QCC_PR_GenerateLogicalNot(var_a, "%s isn't a float...");
			}
			if (var_a.cast->type == ev_string && flag_ifstring)
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_S], var_a, QCC_MakeStringConst(""), NULL, (flags&STFL_PRESERVEA?STFL_PRESERVEA:0));
			else if ((var_a.cast->type == ev_string && flag_ifstring))
				var_a = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value"), "%s isn't a float...");
			if (var_b.cast->type == ev_string && flag_ifstring)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_S], QCC_MakeStringConst(""), var_b, NULL, (flags&STFL_PRESERVEB?STFL_PRESERVEB:0));
			else if (var_b.cast->size != 1 && flag_vectorlogic)
				var_b = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value"), "%s isn't a float...");

			if (var_a.cast->type != ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_I]))
				op = &pr_opcodes[OP_AND_I];	//negative 0 as a float is considered zero by the fpu, which makes 0x80000000 tricky. avoid that if we can.
			else if (var_a.cast->type != ev_float && var_b.cast->type == ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_IF]))
				op = &pr_opcodes[OP_AND_IF];
			else if (var_a.cast->type == ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_FI]))
				op = &pr_opcodes[OP_AND_FI];
			else
				op = &pr_opcodes[OP_AND_F];	//generally works. if there's no other choice then meh.
			break;
		case OP_OR_I:
		case OP_OR_FI:
		case OP_OR_IF:
		case OP_OR_ANY:
			if (var_a.cast->type == ev_vector && flag_vectorlogic)	//we can do a dot-product to test if a vector has a value, instead of a double-not
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_a, var_a, NULL, STFL_PRESERVEA | (flags&STFL_PRESERVEA?STFL_PRESERVEB:0));
			if (var_b.cast->type == ev_vector && flag_vectorlogic)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_b, var_b, NULL, STFL_PRESERVEA | (flags&STFL_PRESERVEB?STFL_PRESERVEB:0));

			if (((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring)) && ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring)))
			{	//just 3 extra instructions instead of 4.
				var_a = QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value");
				var_b = QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value");
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_AND_ANY], var_a, var_b, NULL, 0);
				return QCC_PR_GenerateLogicalNot(var_a, "%s isn't a float...");
			}
			if (var_a.cast->type == ev_string && flag_ifstring)
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_S], var_a, QCC_MakeStringConst(""), NULL, (flags&STFL_PRESERVEA?STFL_PRESERVEA:0));
			else if (var_a.cast->size != 1 && flag_vectorlogic)
				var_a = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value"), "%s isn't a float...");
			if (var_b.cast->type == ev_string && flag_ifstring)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_S], QCC_MakeStringConst(""), var_b, NULL, (flags&STFL_PRESERVEB?STFL_PRESERVEB:0));
			else if (var_b.cast->size != 1 && flag_vectorlogic)
				var_b = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value"), "%s isn't a float...");
			if (var_a.cast->type != ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_I]))
				op = &pr_opcodes[OP_OR_I];	//negative 0 as a float is considered zero by the fpu, which makes 0x80000000 tricky. avoid that if we can.
			else if (var_a.cast->type != ev_float && var_b.cast->type == ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_IF]))
				op = &pr_opcodes[OP_OR_IF];
			else if (var_a.cast->type == ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_FI]))
				op = &pr_opcodes[OP_OR_FI];
			else
				op = &pr_opcodes[OP_OR_F];	//generally works. if there's no other choice then meh.
			break;


		case OP_LOADP_U8:
			if (!var_b.cast) var_b = QCC_MakeUIntConst(0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, QCC_MakeUIntConst(3), NULL, STFL_PRESERVEA);	//read byte index for later
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_b, QCC_MakeUIntConst(2), NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);	//go from byte to word precision
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_I], var_a, var_b, NULL, flags&STFL_PRESERVEA); //read it as a[b], WARNING: var_a may be misaligned
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], QCC_MakeUIntConst(8), var_c, NULL, 0); //bytes to bits
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], QCC_MakeUIntConst(32-8), var_c, NULL, 0); //32-bits-bitofs
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_U], var_a, var_c, NULL, 0); //shift the part we want to the high bit
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_U], var_a, QCC_MakeIntConst(32-8), NULL, 0); //and shift down to the bits we actually want
		case OP_LOADP_I8:
			if (!var_b.cast) var_b = QCC_MakeUIntConst(0);
			if (QCC_OPCodeValid(&pr_opcodes[OP_LOADP_U8]))
			{	//can just reextend it in combination with the older instruction set, for 2 or 3 instructions instead of 7.
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_U8], var_a, var_b, NULL, flags&(STFL_PRESERVEA|STFL_PRESERVEB));
				return QCC_PR_StatementFlags(&pr_opcodes[OP_BITEXTEND_I], var_c, QCC_MakeUIntConst(8), NULL, 0);
			}
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, QCC_MakeUIntConst(3), NULL, STFL_PRESERVEA);	//read byte index for later
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_b, QCC_MakeUIntConst(2), NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);	//go from byte to word precision
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_I], var_a, var_b, NULL, flags&STFL_PRESERVEA); //read it as a[b], WARNING: var_a may be misaligned
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], QCC_MakeUIntConst(8), var_c, NULL, 0); //bytes to bits
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], QCC_MakeUIntConst(32-8), var_c, NULL, 0); //32-bits-bitofs
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, var_c, NULL, 0); //shift the part we want to the high bit
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, QCC_MakeIntConst(32-8), NULL, 0); //and shift down to the bits we actually want
		case OP_LOADP_U16:
			if (!var_b.cast) var_b = QCC_MakeUIntConst(0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, QCC_MakeUIntConst(1), NULL, STFL_PRESERVEA);	//read byte index for later
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_b, QCC_MakeUIntConst(1), NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);	//go from short to word precision
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_I], var_a, var_b, NULL, flags&STFL_PRESERVEA); //read it as a[b], WARNING: var_a may be misaligned
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], QCC_MakeUIntConst(16), var_c, NULL, 0); //short to bits
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], QCC_MakeUIntConst(32-16), var_c, NULL, 0); //32-bits-bitofs
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_U], var_a, var_c, NULL, 0); //shift the part we want to the high bit
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_U], var_a, QCC_MakeIntConst(32-16), NULL, 0); //and shift down to the bits we actually want
		case OP_LOADP_I16:
			if (!var_b.cast) var_b = QCC_MakeUIntConst(0);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, QCC_MakeUIntConst(1), NULL, STFL_PRESERVEA);	//read byte index for later
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_b, QCC_MakeUIntConst(1), NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);	//go from short to word precision
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_I], var_a, var_b, NULL, flags&STFL_PRESERVEA); //read it as a[b], WARNING: var_a may be misaligned
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], QCC_MakeUIntConst(16), var_c, NULL, 0); //short to bits
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], QCC_MakeUIntConst(32-16), var_c, NULL, 0); //32-bits-bitofs
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, var_c, NULL, 0); //shift the part we want to the high bit
			return QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], var_a, QCC_MakeIntConst(32-16), NULL, 0); //and shift down to the bits we actually want

//		case OP_LOADP_V:
//			break;
		case OP_LOADP_F:
		case OP_LOADP_S:
		case OP_LOADP_ENT:
		case OP_LOADP_FLD:
		case OP_LOADP_FNC:
		case OP_LOADP_I:
			{
				QCC_sref_t fnc = QCC_PR_EmulationFunc(memgetval);
				if (!fnc.cast)
				{
					QCC_PR_ParseWarning(0, "memgetval function not defined: cannot emulate OP_LOADP_*");
					goto badopcode;
				}
				var_c = QCC_PR_GenerateFunctionCall2(nullsref, fnc, var_a, type_pointer, QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0), type_float);
				var_c.cast = *op->type_c;
				return var_c;
			}
			break;
		case OP_ADD_PIW:
			if (flag_undefwordsize)	//fine if its the engine's doing it, not so fine if we're assuming it in the qcc.
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "Making assumptions about pointer sizes was blocked.");
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], QCC_MakeIntConst(VMWORDSIZE), var_b, NULL, flags&STFL_PRESERVEB);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], var_a, var_b, NULL, flags&~STFL_PRESERVEB);
			var_c.cast = var_a.cast;
			return var_c;

		case OP_LT_U:
		case OP_LE_U:
			if (!QCC_OPCodeValid(&pr_opcodes[OP_LT_I]))
			{	//we're kinda fucked. go straight for floats instead. with any luck we might avoid precision issues.
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_UF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_UF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB?STFL_PRESERVEA:0));
				if (op == &pr_opcodes[OP_LT_U])
					return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, var_b, NULL, flags&~(STFL_PRESERVEA|STFL_PRESERVEB));
				else if (op == &pr_opcodes[OP_LE_U])
					return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_F], var_a, var_b, NULL, flags&~(STFL_PRESERVEA|STFL_PRESERVEB));
				goto badopcode;
			}
			else
			{
				//bias down, forcing it to wrap. then use the regular compares
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_U], var_a, QCC_MakeUIntConst(0x80000000), NULL, flags&STFL_PRESERVEA);
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_U], var_b, QCC_MakeUIntConst(0x80000000), NULL, (flags&STFL_PRESERVEB?STFL_PRESERVEA:0));
				var_a.cast = type_integer;
				var_b.cast = type_integer;
				if (op == &pr_opcodes[OP_LT_U])
					return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_I], var_a, var_b, NULL, flags&~(STFL_PRESERVEA|STFL_PRESERVEB));
				else if (op == &pr_opcodes[OP_LE_U])
					return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_I], var_a, var_b, NULL, flags&~(STFL_PRESERVEA|STFL_PRESERVEB));
				goto badopcode;
			}

		//FIXME: assume the high/sign bit is not set
		case OP_DIV_U:
			QCC_PR_ParseWarning(0, "OP_DIV_U emulation: assuming the high bit is not set...");
			var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint;
			return var_c;

		//other uint ops have the same bit pattern
		case OP_ADD_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_SUB_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_MUL_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c; //carry/overflow differ, but we don't provide such feedback.
//		case OP_MOD_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MOD_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_BITAND_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_BITOR_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_BITXOR_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITXOR_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_BITNOT_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_BITCLR_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITCLR_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_LSHIFT_U:	var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I],	var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_GE_U:																return  QCC_PR_StatementFlags(&pr_opcodes[OP_LE_U],		var_b, var_a, NULL, flags);	//just swap the args, fewer opcodes needed.
		case OP_GT_U:																return  QCC_PR_StatementFlags(&pr_opcodes[OP_LT_U],		var_b, var_a, NULL, flags);	//just swap the args, fewer opcodes needed.
		case OP_EQ_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_I],		var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;
		case OP_NE_U:		var_a.cast = type_integer;	var_b.cast = type_integer;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_I],		var_a, var_b, NULL, flags); var_c.cast = type_uint; return var_c;

		case OP_ADD_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_SUB_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_MUL_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
//		case OP_MOD_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_MOD_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_BITAND_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I64],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_BITOR_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I64],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_BITXOR_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITXOR_I64],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_BITNOT_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I64],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_BITCLR_U64:		var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITCLR_I64],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_LSHIFT_U64I:	var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I64I],	var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_GE_U64:																	return  QCC_PR_StatementFlags(&pr_opcodes[OP_LE_U64],		var_b, var_a, NULL, flags);
		case OP_GT_U64:																	return  QCC_PR_StatementFlags(&pr_opcodes[OP_LT_U64],		var_b, var_a, NULL, flags);
		case OP_EQ_U64:			var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_NE_U64:			var_a.cast = type_int64;	var_b.cast = type_int64;	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_NE_I64],		var_a, var_b, NULL, flags); var_c.cast = type_uint64; return var_c;
		case OP_GE_I64:			var_a.cast = type_int64;	var_b.cast = type_int64;	return  QCC_PR_StatementFlags(&pr_opcodes[OP_LE_I64],		var_b, var_a, NULL, flags);
		case OP_GT_I64:			var_a.cast = type_int64;	var_b.cast = type_int64;	return  QCC_PR_StatementFlags(&pr_opcodes[OP_LT_I64],		var_b, var_a, NULL, flags);
		case OP_GE_D:			return  QCC_PR_StatementFlags(&pr_opcodes[OP_LE_D],		var_b, var_a, NULL, flags);
		case OP_GT_D:			return  QCC_PR_StatementFlags(&pr_opcodes[OP_LT_D],		var_b, var_a, NULL, flags);

		case OP_STORE_I64:
			var_c = var_b;
			var_c.cast = type_integer;
			QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], var_a, var_c, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
			var_a.ofs++;
			var_c.ofs++;
			QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], var_a, var_c, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
			return var_b;

		default:
		badopcode:
			if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_DEF, op))
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte", op->name, op->opname);
			else if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_MAX, op))
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte_%u", op->name, op->opname, QCTARGVER_FTE_MAX);
			else
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "Opcode \"%s|%s\" is not supported.", op->name, op->opname);
			break;
		}
	}

	if (!pr_scope)
	{
		switch(op - pr_opcodes)
		{
		case OP_GLOBALADDRESS:
			if (flag_pointerrelocs)
				return QCC_MakeGAddress(type_pointer, var_a.sym, var_a.ofs + (var_b.cast?QCC_Eval_Int(QCC_SRef_EvalConst(var_b), var_b.cast):0), 0);
			break;
		}

		QCC_PR_ParseWarning(ERR_BADEXTENSION, "Unable to generate statements at global scope (%s).", op->opname);
	}

	if (op->type_c == &type_void || op->associative==ASSOC_RIGHT || op->type_c == NULL)
	{
		QCC_FreeTemp(var_b);	//returns a instead of some result/temp
		if (flags&STFL_DISCARDRESULT)
			QCC_FreeTemp(var_a);
	}
	else
	{
		QCC_FreeTemp(var_a);
		QCC_FreeTemp(var_b);
	}

	if (outstatement)
		QCC_ClobberDef(NULL);

	statement = &statements[numstatements++];

	if (outstatement)
		*outstatement = statement;

	statement->linenum = pr_token_line_last;
	statement->op = op - pr_opcodes;
	statement->flags = 0;
	statement->a = var_a;
	statement->b = var_b;

	if (var_c.cast && var_c.sym && !var_c.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_c was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_c);
	}
	if (var_b.cast && var_b.sym && !var_b.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_b was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_b);
	}
	if (var_a.cast && var_a.sym && !var_a.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_a was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_a);
	}

	if (var_c.cast)
		statement->c = var_c;
	else if (op->type_c == &type_void || op->associative==ASSOC_RIGHT || op->type_c == NULL)
	{
		var_c = nullsref;
		statement->c = nullsref;			// ifs, gotos, and assignments
										// don't need vars allocated
		if (flags&STFL_DISCARDRESULT)
			return nullsref;
		return var_a;
	}
	else if (op-pr_opcodes == OP_ADD_PIW)
	{
		var_c = QCC_GetTemp(var_a.cast);
		statement->c = var_c;
	}
	else
	{	// allocate result space
		var_c = QCC_GetTemp(*op->type_c);
		statement->c = var_c;
		if (op->type_b == &type_field)
		{
			//&(a.b) returns a pointer to b, so that pointer's auxtype should have the same type as b's auxtype
			if (var_b.cast->type == ev_variant)
				var_c.cast = type_variant;
			else if (var_c.cast->type == ev_pointer)
				var_c.cast = QCC_PR_PointerType(var_b.cast->aux_type);
			else if (var_c.cast->type == ev_field)
				var_c.cast = QCC_PR_FieldType(var_b.cast->aux_type);
			else
				var_c.cast = var_b.cast->aux_type;
		}
	}

	if (flags&STFL_DISCARDRESULT)
	{
		QCC_FreeTemp(var_c);
		var_c = nullsref;
	}
	return var_c;
}

/*
============
QCC_PR_SimpleStatement

Emits a primitive statement
============
*/
QCC_statement_t *QCC_PR_SimpleStatement ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_sref_t var_c, int force)
{
	QCC_statement_t	*statement;

	if (!force && !QCC_OPCodeValid(op))
	{
//		outputversion = op->extension;
//		if (noextensions)
		if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_DEF, op))
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte", op->name, op->opname);
		else if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_MAX, op))
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte_%u", op->name, op->opname, QCTARGVER_FTE_MAX);
		else
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" is not supported.", op->name, op->opname);
	}

	statement = &statements[numstatements];
	numstatements++;

	statement->op = op - pr_opcodes;
	statement->flags = 0;
	statement->a = var_a;
	statement->b = var_b;
	statement->c = var_c;
	statement->linenum = pr_token_line_last;

	return statement;
}

QCC_statement_t *QCC_PR_SimpleInitStatement ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_sref_t var_c)
{	//statements execed at global scope. will get inserted into an appropriate function at some later point.
	QCC_statement_t	*statement;

	if (!QCC_OPCodeValid(op))
	{
		if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_DEF, op))
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte", op->name, op->opname);
		else if (QCC_OPCodeValidForTarget(QCF_FTE, QCTARGVER_FTE_MAX, op))
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte_%u", op->name, op->opname, QCTARGVER_FTE_MAX);
		else
			QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" is not supported.", op->name, op->opname);
	}

	if (numinitstatements+1 > maxinitstatements)
	{
		maxinitstatements+=64;
		initstatements = realloc(initstatements, sizeof(*initstatements)*maxinitstatements);
	}

	statement = &initstatements[numinitstatements];
	numinitstatements++;

	statement->op = op - pr_opcodes;
	statement->flags = 0;
	statement->a = var_a;
	statement->b = var_b;
	statement->c = var_c;
	statement->linenum = 0;//pr_token_line_last; //will come from all over. the file won't be meaningful so nor will the line.

	return statement;
}

//this opcode updates its dest, so can't use QCC_PR_StatementFlags because that's only two args. We still want to be able to emulate it though.
//NOTE: emulation might require an extra copy after.
QCC_sref_t QCC_PR_Statement_BitCopy(QCC_sref_t var_a, unsigned int bitofs, unsigned int bits, QCC_sref_t var_c)
{
	if (QCC_OPCodeValid(&pr_opcodes[OP_BITCOPY_I]))
	{	//can do it in a single opcode.
		QCC_sref_t var_b = QCC_MakeUIntConst((bitofs<<8) | bits);
		var_b.sym->referenced = true;
		QCC_PR_SimpleStatement(&pr_opcodes[OP_BITCOPY_I], var_a, var_b, var_c, false);
		QCC_FreeTemp(var_a);
		QCC_FreeTemp(var_b);
		return var_c;
	}

	//this will take effort. we might still be able to fold part of it at least.
	var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_c, QCC_MakeUIntConst(~(((1u<<bits)-1u)<<bitofs)), NULL, 0);	//clear the (cache of the) dest
	var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_a, QCC_MakeUIntConst(((1u<<bits)-1u)), NULL, 0);	//clear the (cache of the) dest
	var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], var_a, QCC_MakeUIntConst(bitofs), NULL, 0);	//shift the source...
	return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I], var_c, var_a, NULL, 0);	//and pack it together.
}
void QCC_PR_Statement_SmallStore(int opcode, QCC_sref_t val, QCC_sref_t ptr, QCC_sref_t idx)
{
	QCC_sref_t tmp, byteidx;
	int idxshift;
	int bits;

	if (QCC_OPCodeValid(&pr_opcodes[opcode]))
	{	//no need to emulate it, just spit out the instruction directly.
		QCC_PR_SimpleStatement(&pr_opcodes[opcode], val,ptr,idx, false);
		return;
	}

	QCC_UnFreeTemp(ptr);
	QCC_UnFreeTemp(val);
	QCC_UnFreeTemp(idx);
	if (!idx.cast) idx = QCC_MakeUIntConst(0);

	if (opcode == OP_STOREP_I8)
	{
		bits = 8;
		idxshift=2;
	}
	else if (opcode == OP_STOREP_C)
	{
		bits = 8;
		idxshift=2;
		val = QCC_SupplyConversion(val, ev_uint, true);
	}
	else if (opcode == OP_STOREP_I16)
	{
		bits = 16;
		idxshift=1;
	}
	else
		QCC_PR_ParseError(ERR_BADEXTENSION, "QCC_PR_Statement_SmallStore: Opcode \"%s\" is not supported.", pr_opcodes[opcode].opname);

	byteidx = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], idx, QCC_MakeUIntConst((32/bits)-1), NULL, STFL_PRESERVEA);	//drop the extra precision...

	idx = QCC_PR_StatementFlags(&pr_opcodes[OP_RSHIFT_I], idx, QCC_MakeUIntConst(idxshift), NULL, 0);	//drop the extra precision...
	tmp = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_I], ptr, idx, NULL, STFL_PRESERVEA|STFL_PRESERVEB);	//read the input

	/*if (QCC_OPCodeValid(&pr_opcodes[opcode]))
	{	//can do it in a single opcode.
		QCC_sref_t shiftinfo = QCC_MakeUIntConst((bitofs<<8) | bits);
		QCC_PR_SimpleStatement(&pr_opcodes[OP_BITCOPY_I], val, shiftinfo, tmp, false);
		val = tmp;
	}
	else*/
	{
		QCC_sref_t bitidx = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], byteidx, QCC_MakeUIntConst(bits), NULL, 0);
		QCC_sref_t bitmask = QCC_MakeUIntConst((1u<<bits)-1);

		val = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], val, bitmask, NULL, STFL_PRESERVEB);
		val = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], val, bitidx, NULL, STFL_PRESERVEB);

		bitmask = QCC_PR_StatementFlags(&pr_opcodes[OP_LSHIFT_I], bitmask, bitidx, NULL, 0);

		bitmask = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_I], bitmask, nullsref, NULL, 0);
		tmp = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], tmp, bitmask, NULL, 0);
		val = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_I], tmp, val, NULL, 0);
	}

	if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I]))
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I], val,ptr,idx, false);
	else
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], val,ptr,idx, false);

	QCC_FreeTemp(ptr);
	QCC_FreeTemp(val);
	QCC_FreeTemp(idx);
}
//For qcc debugging, inserts a no-op statement with a string argument
void QCC_PR_StatementAnnotation(const char *fmt, ...)
{
	char buf[1024];
	va_list va;
	QCC_sref_t s, d;
	va_start(va, fmt);
	QC_vsnprintf(buf,sizeof(buf), fmt, va);
	va_end(va);
	s = QCC_MakeStringConst(buf);
	d = QCC_PR_GetSRef(type_string, "#note", NULL, true, 0, false);
	QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_S], s,d,nullsref, true);
}

/*
	Removes trailing statements, rewinding back to a known-safe position.
*/
void QCC_UngenerateStatements(int newstatementcount)
{
	int i;

	//forget any indexes to statements if those statements are going to go away...
	for (i = 0; i < num_gotos; )
	{
		if (pr_gotos[i].statementno >= newstatementcount)
		{
			memmove(&pr_gotos[i], &pr_gotos[i+1], sizeof(*pr_gotos)*(num_gotos-(i+1)));
			num_gotos--;
		}
		else
			i++;
	}
	for (i = 0; i < num_labels; )
	{	//FIXME: stripping a label? erk?
		if (pr_labels[i].statementno >= newstatementcount)
		{
			memmove(&pr_labels[i], &pr_labels[i+1], sizeof(*pr_labels)*(num_labels-(i+1)));
			num_labels--;
		}
		else
			i++;
	}
	for (i = 0; i < num_breaks; )
	{
		if (pr_breaks[i] >= newstatementcount)
		{
			memmove(&pr_breaks[i], &pr_breaks[i+1], sizeof(*pr_breaks)*(num_breaks-(i+1)));
			num_breaks--;
		}
		else
			i++;
	}
	for (i = 0; i < num_continues; )
	{
		if (pr_continues[i] >= newstatementcount)
		{
			memmove(&pr_continues[i], &pr_continues[i+1], sizeof(*pr_continues)*(num_continues-(i+1)));
			num_continues--;
		}
		else
			i++;
	}
	for (i = 0; i < num_cases; )
	{
		if (pr_cases[i] >= newstatementcount)
		{
			memmove(&pr_cases[i], &pr_cases[i+1], sizeof(*pr_cases)*(num_cases-(i+1)));
			memmove(&pr_casesref[i], &pr_casesref[i+1], sizeof(*pr_casesref)*(num_cases-(i+1)));
			memmove(&pr_casesref2[i], &pr_casesref2[i+1], sizeof(*pr_casesref2)*(num_cases-(i+1)));
			num_cases--;
		}
		else
			i++;
	}

	numstatements = newstatementcount;
}

/*
============
PR_ParseImmediate

Looks for a preexisting constant
============
*/
static QCC_sref_t	QCC_PR_ParseImmediate (void)
{
	QCC_sref_t	cn;

	switch(pr_immediate_type->type)
	{
	case ev_float:
		cn = QCC_MakeFloatConst(pr_immediate._float);
		QCC_PR_Lex ();
		return cn;

	case ev_integer:
		cn = QCC_MakeIntConst(pr_immediate._int);
		QCC_PR_Lex ();
		return cn;
	case ev_uint:
		cn = QCC_MakeUIntConst(pr_immediate._uint);
		QCC_PR_Lex ();
		return cn;

	case ev_double:
		cn = QCC_MakeDoubleConst(pr_immediate._double);
		QCC_PR_Lex ();
		return cn;
	case ev_int64:
		cn = QCC_MakeInt64Const(pr_immediate.i64);
		QCC_PR_Lex ();
		return cn;
	case ev_uint64:
		cn = QCC_MakeUInt64Const(pr_immediate.u64);
		QCC_PR_Lex ();
		return cn;

	case ev_vector:
		cn = QCC_MakeVectorConst(pr_immediate.vector[0], pr_immediate.vector[1], pr_immediate.vector[2]);
		QCC_PR_Lex ();
		return cn;

	case ev_string:
		{
			int t=0,l;
			char tmp[8192];
			do
			{
				l = pr_immediate_strlen;
				if (t+l+1 > sizeof(tmp))
					QCC_PR_ParseError (ERR_NAMETOOLONG, "string immediate is too long");
				memcpy(tmp+t, pr_immediate_string, l);
				t+=l;
				
				QCC_PR_Lex ();
			} while(pr_token_type == tt_immediate && pr_immediate_type == type_string);
			tmp[t++] = 0;
			cn = QCC_MakeStringConstLength(tmp, t);
		}
		return cn;
	default:
		QCC_PR_ParseError (ERR_BADIMMEDIATETYPE, "weird immediate type");
		return nullsref;
	}
}

static QCC_ref_t *QCC_PR_GenerateAddressOf(QCC_ref_t *retbuf, QCC_ref_t *operand)
{
//	QCC_def_t *e2;
	if (operand->type == REF_FIELD)
	{
		if (operand->bitofs)
			QCC_PR_ParseWarning (ERR_BADEXTENSION, "Address-of operator on bitfield");
		//&e.f should generate a pointer def
		//as opposed to a ref
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				QCC_PR_Statement(&pr_opcodes[OP_ADDRESS], operand->base, operand->index, NULL),
				nullsref,
				QCC_PR_PointerType((operand->index.cast->type == ev_field)?operand->index.cast->aux_type:type_variant),
				true, 0);
	}
	if (operand->type == REF_ARRAYHEAD || operand->type == REF_GLOBAL || operand->type == REF_ARRAY)
	{
		QCC_sref_t ptr;
		const QCC_eval_t *eval;
		QCC_type_t *basetype = operand->cast;
		if (operand->type == REF_ARRAYHEAD)
			basetype = basetype->aux_type;

		if (!QCC_OPCodeValid(&pr_opcodes[OP_GLOBALADDRESS]))
		{
			if (operand->type == REF_ARRAYHEAD)
				QCC_PR_ParseError (ERR_BADEXTENSION, "Address-of operator is not supported in this form without extensions. Consider the use of either '#pragma target fte' or '#pragma flag enable brokenarray'");
			else
				QCC_PR_ParseError (ERR_BADEXTENSION, "Address-of operator is not supported in this form without extensions. Consider the use of: #pragma target fte");
		}
		if (operand->base.sym->scope && !operand->base.sym->addressedwarned && !operand->base.sym->isstatic)
		{
			char type[128];
			QCC_PR_ParseWarning (WARN_UNSAFELOCALPOINTER, "Address-of operator on local %s %s is unsafe if recursing (and still bloated when not). Consider use of 'static' or 'auto'.", TypeName(operand->base.cast, type,sizeof(type)), operand->base.sym->name);
			operand->base.sym->addressedwarned = true;
			operand->base.sym->scope->privatelocals = true;	//just in case.
		}
		if (operand->base.sym->temp)
			QCC_PR_ParseWarning (WARN_NOTSTANDARDBEHAVIOUR, "Address-of operator on temp from line %i", operand->base.sym->temp->lastline);

		eval = QCC_SRef_EvalConst(operand->index);
		if (eval && flag_pointerrelocs)
		{
			int ofs = QCC_Eval_Int(eval, operand->index.cast);
			ptr = QCC_MakeGAddress(type_pointer, operand->base.sym, operand->base.ofs, basetype->align * ofs + operand->bitofs);
		}
		else
		{
			if (basetype->align && basetype->align!=32)
			{	//urgh. OP_GLOBALADDRESS is in terms of words, while pointers are often not (eg __int16, __int8, or even certain structs which lack 32bit members.)
				ptr = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], operand->base, nullsref, NULL);
				if (operand->index.cast)
				{
					QCC_sref_t idx = QCC_SupplyConversion(operand->index, ev_integer, true);
					if (basetype->align!=8)
						idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], idx, QCC_MakeIntConst(basetype->align/8), NULL);
					ptr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], ptr, idx, NULL);
				}
			}
			else
				ptr = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], operand->base, operand->index.cast?QCC_SupplyConversion(operand->index, ev_integer, true):nullsref, NULL);
			if (operand->bitofs)
				ptr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], ptr, QCC_MakeIntConst(operand->bitofs/8), NULL);
		}

		//&foo (or &((&foo)[5]), which is basically an array). the result is a temp and thus cannot be assigned to (but should be possible to dereference further).
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				ptr,
				nullsref,
				QCC_PR_PointerType(basetype),
				true, 0);
	}
	if (operand->type == REF_POINTER || operand->type == REF_POINTERARRAY /*technically already meant to be an address, but hey, silently allow taking the address of it anyway*/)
	{
		//&(p[5]) just reverts back to p+5. it cannot be assigned to.
		QCC_sref_t addr, idx;
		const QCC_eval_t *eval;
		QCC_type_t *resulttype = operand->cast;	//'int' *
		QCC_type_t *basetype = operand->cast;	//'int'
		if (operand->type != REF_POINTERARRAY)
			resulttype = QCC_PR_PointerType(resulttype);
		else
			basetype = basetype->aux_type;	//already a pointer.

		if (operand->index.cast)
		{
			if (basetype->align != 32)
			{	//index is bytes or shorts or something small.
				if (!flag_undefwordsize && (eval=QCC_SRef_EvalConst(operand->index)))
				{
					int byteofs = QCC_Eval_Int(eval, operand->index.cast)*(basetype->align/8) + operand->bitofs/8;
					addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], operand->base, QCC_MakeIntConst(byteofs), NULL);
				}
				else
				{
					idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], QCC_MakeIntConst(basetype->align/8), QCC_SupplyConversion(operand->index, ev_integer, true), NULL);
					if (operand->bitofs)
						addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], idx, QCC_MakeUInt64Const(operand->bitofs/8), NULL);
					addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], operand->base, idx, NULL);
				}
			}
			else
			{	//index is words
//				if (!QCC_OPCodeValid(&pr_opcodes[OP_ADD_PIW]))
//					QCC_PR_ParseError (ERR_BADEXTENSION, "Address-of operator is not supported in this form without extensions. Consider the use of: #pragma target fte");
				addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], operand->base, QCC_SupplyConversion(operand->index, ev_integer, true), NULL);
				if (operand->bitofs)
					addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], addr, QCC_MakeUInt64Const(operand->bitofs/8), NULL);
			}
		}
		else
		{
			addr = operand->base;
			if (operand->bitofs)
				addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], addr, QCC_MakeUInt64Const(operand->bitofs/8), NULL);
		}
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				addr, 
				nullsref,
				resulttype,
				true, 0);
	}
	QCC_PR_ParseError (ERR_BADEXTENSION, "Cannot use addressof operator ('&') on a global. Please use the FTE target.");
	return operand;
}
QCC_sref_t QCC_DP_GlobalAddress(QCC_sref_t base, QCC_sref_t index, unsigned int flags)
{
	QCC_sref_t ptr = QCC_MakeGAddress(type_integer, base.sym, 0, 0);
	base.sym->used = true;
	if (!(flags&STFL_PRESERVEA))
		QCC_FreeTemp(base);
	if (base.ofs)
		ptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], ptr, QCC_MakeIntConst(base.ofs), NULL, 0);
	if (index.cast)
		ptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], ptr, index, NULL, flags);
	return ptr;
}

static void QCC_PrecacheSound (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	if (ch >= '1'  && ch <= '9')
		ch -= '0';
	else
		ch = 1;

	for (i=0 ; i<numsounds ; i++)
		if (!STRCMP(n, precache_sound[i].name))
		{
			if (!precache_sound[i].block || precache_sound[i].block > ch)
				precache_sound[i].block = ch;
			return;
		}
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (numsounds == QCC_MAX_SOUNDS)
		return;
//		QCC_Error ("PrecacheSound: numsounds == MAX_SOUNDS");
	strcpy (precache_sound[i].name, n);
	precache_sound[i].block = ch;
	precache_sound[i].filename = s_filen;
	precache_sound[i].fileline = pr_source_line;
	numsounds++;
}

static void QCC_PrecacheModel (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<nummodels ; i++)
		if (!STRCMP(n, precache_model[i].name))
		{
			if (!precache_model[i].block)
			{
				if (ch >= '1'  && ch <= '9')
					precache_model[i].block = ch - '0';
				else
					precache_model[i].block = 1;
			}
			return;
		}
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (nummodels == QCC_MAX_MODELS)
		return;
//		QCC_Error ("PrecacheModels: nummodels == MAX_MODELS");
	strcpy (precache_model[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_model[i].block = ch - '0';
	else
		precache_model[i].block = 1;
	precache_model[i].filename = s_filen;
	precache_model[i].fileline = pr_source_line;
	nummodels++;
}

static void QCC_SetModel (const char *n)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<nummodels ; i++)
		if (!STRCMP(n, precache_model[i].name))
		{
			precache_model[i].used++;
			return;
		}
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (nummodels == QCC_MAX_MODELS)
		return;
	strcpy (precache_model[i].name, n);
	precache_model[i].block = 0;
	precache_model[i].used=1;

	precache_model[i].filename = s_filen;
	precache_model[i].fileline = pr_source_line;
	nummodels++;
}
static void QCC_SoundUsed (const char *n)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numsounds ; i++)
		if (!STRCMP(n, precache_sound[i].name))
		{
			precache_sound[i].used++;
			return;
		}
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (numsounds == QCC_MAX_SOUNDS)
		return;
	strcpy (precache_sound[i].name, n);
	precache_sound[i].block = 0;
	precache_sound[i].used=1;

	precache_sound[i].filename = s_filen;
	precache_sound[i].fileline = pr_source_line;
	numsounds++;
}

static void QCC_PrecacheTexture (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numtextures ; i++)
		if (!STRCMP(n, precache_texture[i].name))
			return;
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (nummodels == QCC_MAX_MODELS)
		return;
//		QCC_Error ("PrecacheTextures: numtextures == MAX_TEXTURES");
	strcpy (precache_texture[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_texture[i].block = ch - '0';
	else
		precache_texture[i].block = 1;
	numtextures++;
}

static void QCC_PrecacheFile (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numfiles ; i++)
		if (!STRCMP(n, precache_file[i].name))
			return;
	if (strchr(n, '\\'))
		QCC_PR_ParseWarning(WARN_NONPORTABLEFILENAME, "backslashes in path names are non-portable - %s", n);
	if (numfiles == QCC_MAX_FILES)
		return;
//		QCC_Error ("PrecacheFile: numfiles == MAX_FILES");
	strcpy (precache_file[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_file[i].block = ch - '0';
	else
		precache_file[i].block = 1;
	numfiles++;
}


static void QCC_VerifyFormatString (const char *funcname, QCC_ref_t **arglist, unsigned int argcount)
{
	const char *s = "%s", *reqtype;
	int firstarg = 1;
	const char *s0;
	char *err;
	int width, thisarg, arg;
	char formatbuf[16];
	char temp[256];
	int argpos = firstarg, argn_last = firstarg;
	int isfloat, is64bit;
	const QCC_eval_t *formatstring = QCC_SRef_EvalConst(arglist[0]->base);
	if (!formatstring)	//can't check variables.
		return;
	if (!qccwarningaction[WARN_FORMATSTRING])
		return;	//don't bother if its not relevant anyway.
	s = strings + formatstring->string;

#define ARGTYPE(a) (((a)>=firstarg && (a)<argcount) ? ((arglist[a]->cast->type==ev_boolean)?arglist[a]->cast->parentclass->type:arglist[a]->cast->type) : ev_void)
#define ARGCTYPE(a) (((a)>=firstarg && (a)<argcount) ? (arglist[a]->cast) : type_void)

	for(;;)
	{
		s0 = s;
		switch(*s)
		{
		case 0:
			if (argpos < argcount && argn_last < argcount)
				QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: surplus trailing %s%s%s argument(s) for format %s\"%s\"%s", funcname, col_type, TypeName(ARGCTYPE(argpos), temp, sizeof(temp)), col_none, col_name, strings + formatstring->string, col_none);
			return;
		case '%':
			if(*++s == '%')
			{
				s++;
				break;
			}

			// complete directive format:
			// %3$*1$.*2$ld
			
			width = -1;
			thisarg = -1;
			isfloat = -1;
			is64bit = 0;

			// is number following?
			if(*s >= '0' && *s <= '9')
			{
				width = strtol(s, &err, 10);
				if(!err)
				{
					QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: bad format string: %s%s%s", funcname, col_name, s0, col_none);
					return;
				}
				if(*err == '$')
				{
					thisarg = width + (firstarg-1);
					width = -1;
					s = err + 1;
				}
				else
				{
					if(*s == '0')
					{
						if(width == 0)
							width = -1; // it was just a flag
					}
					s = err;
				}
			}

			if(width < 0)
			{
				for(;;)
				{
					switch(*s)
					{
						case '#': //alternate
						case '0': //zero-pad
						case '-': //left-align
						case ' ': //space-positive
						case '+': //sign-positive
							break;
						default:
							goto noflags;
					}
					++s;
				}
noflags:
				if(*s == '*')
				{
					++s;
					if(*s >= '0' && *s <= '9')
					{
						arg = strtol(s, &err, 10);
						if(!err || *err != '$')
						{
							QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
							return;
						}
						s = err + 1;
					}
					else
						arg = argpos++;
					if (ARGTYPE(arg) != ev_float)
						QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: width modifier requires float at arg %i", funcname, arg+1);
				}
				else if(*s >= '0' && *s <= '9')
				{
					strtol(s, &err, 10);
					if(!err)
					{
						QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
						return;
					}
					s = err;
				}
				// otherwise width stays -1
			}

			//precision modifiers
			if(*s == '.')
			{
				++s;
				if(*s == '*')
				{
					++s;
					if(*s >= '0' && *s <= '9')
					{
						arg = strtol(s, &err, 10);
						if(!err || *err != '$')
						{
							QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
							return;
						}
						s = err + 1;
					}
					else
						arg = argpos++;

					if (ARGTYPE(arg) != ev_float)
						QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: precision modifier requires float at arg %i", funcname, arg+1);
				}
				else if(*s >= '0' && *s <= '9')
				{
					strtol(s, &err, 10);
					if(!err)
					{
						QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
						return;
					}
					s = err;
				}
				else
				{
					QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
					return;
				}
			}

			//length modifiers
			for(;;)
			{
				switch(*s)
				{
					//case 'hh':	//char
					case 'h': isfloat = 1; break;	//short
					//case 'll':	//long long
					case 'l': isfloat = 0; break;	//long
					case 'L': isfloat = 0; break;	//long double
					case 'q': is64bit = 1; break; //BSD's int64
					case 'j':	//[u]intmax_t
					case 'z':	//size_t
					case 't':	//ptrdiff_t
						QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: length modifier %s%c%s is a placeholder", funcname, col_type, *s, col_none);
						break;
					default:
						goto nolength;
				}
				++s;
			}
nolength:

			// now s points to the final directive char and is no longer changed
			if (*s == 'p' || *s == 'P')
			{
				//%p is slightly different from %x.
				//always 8-bytes wide with 0 padding, always ints.
				if (isfloat < 0) isfloat = 0;
			}
			else if (*s == 'i' || *s == 'I')
			{
				//%i defaults to ints, not floats.
				if(isfloat < 0) isfloat = 0;
			}

			//assume floats, not ints.
			if(isfloat < 0)
				isfloat = 1;

			if(thisarg < 0)
				thisarg = argpos++;
			if (argn_last < thisarg+1)
				argn_last = thisarg+1;

			memcpy(formatbuf, s0, s+1-s0);
			formatbuf[s+1-s0] = 0;

			reqtype = NULL;
			switch(*s)
			{
			//fixme: should we validate char ranges?
			case 'd': case 'i': case 'I': case 'c':
			case 'o': case 'u': case 'x': case 'X': case 'p': case 'P':
			case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
				if (isfloat)
				{
					if (is64bit)
					{
						switch(ARGTYPE(thisarg))
						{
						case ev_double:
						case ev_variant:
							break;
						default:
							reqtype = "double";
							break;
						}
					}
					else
					{
						switch(ARGTYPE(thisarg))
						{
						case ev_float:
						case ev_variant:
							break;
						default:
							reqtype = "float";
							break;
						}
					}
				}
				else
				{
					if (*s == 'p' || *s == 'P')
					{
						if (is64bit)
							reqtype = "some kind of double-size pointer! oh noes!";
						else switch(ARGTYPE(thisarg))
						{
						case ev_pointer:
						case ev_variant:
							break;
						default:
							reqtype = "pointer";
							break;
						}
					}
					else
					{
						if (is64bit)
						{
							switch(ARGTYPE(thisarg))
							{
							case ev_int64:
							case ev_uint64:
							case ev_variant:
								break;
							default:
								reqtype = "__int64";
								QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: %s%s%s requires __int64 at arg %i (got %s%s%s)", funcname, col_name, formatbuf, col_none, thisarg+1, col_type, TypeName(ARGCTYPE(thisarg), temp, sizeof(temp)), col_none);
								break;
							}
						}
						else
						{
							switch(ARGTYPE(thisarg))
							{
							case ev_integer:
							case ev_uint:
							case ev_variant:
								break;
							case ev_entity:	//accept ents ONLY for %i
								if (*s == 'i')
									break;
								//fallthrough
							default:
								reqtype = "int";
								QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: %s%s%s requires int at arg %i (got %s%s%s)", funcname, col_name, formatbuf, col_none, thisarg+1, col_type, TypeName(ARGCTYPE(thisarg), temp, sizeof(temp)), col_none);
								break;
							}
						}
					}
				}
				break;
			case 'v': case 'V':
				if (isfloat)
				{
					switch(ARGTYPE(thisarg))
					{
					case ev_vector:
					case ev_variant:
						break;
					default:
						reqtype = "vector";
						break;
					}
				}
				else
					reqtype = "intvector";
				break;
			case 's':
			case 'S':
				switch(ARGTYPE(thisarg))
				{
				case ev_string:
				case ev_variant:
					break;
				default:
					reqtype = "string";
					break;
				}
				break;
			default:
				QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: invalid format string: %s%s%s", funcname, col_name, s0, col_none);
				return;
			}

			if (reqtype)
			{
				QCC_PR_ParseWarning(WARN_FORMATSTRING, "%s: %s%s%s requires %s%s%s at arg %i (got %s%s%s)", funcname, col_name,formatbuf,col_none, col_type,reqtype,col_none, thisarg+1, col_type,TypeName(ARGCTYPE(thisarg), temp, sizeof(temp)),col_none);
				switch(ARGCTYPE(thisarg)->type)
				{
				case ev_string:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %s or %S for strings");
					break;
				case ev_float:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %g or %f or %hx for floats");
					break;
				case ev_vector:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %v for vectors");
					break;
				case ev_entity:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %i for entities");
					break;
				case ev_pointer:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %p for pointer types");
					break;
				case ev_bitfld:
				case ev_integer:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %i or %lx for 32bit ints");
					break;
				case ev_uint:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %lu or or %lx for 32bit ints");
					break;
				case ev_int64:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %qi or %lqx for 64bit ints");
					break;
				case ev_uint64:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %lqu or or %lqx for 64bit ints");
					break;
				case ev_double:
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "use %qg or %qf or %hqx for doubles");
					break;
				case ev_void:		//coder's problem
				case ev_field:		//cast to int
				case ev_function:	//cast to int
				case ev_variant:	//should be accepted by anything...
				case ev_struct:		//coder's problem
				case ev_union:		//coder's problem
				case ev_accessor:	//should be unreachable
				case ev_enum:		//should be unreachable
				case ev_typedef:	//should be unreachable
				case ev_boolean:	//should be unreachable
					QCC_PR_Note(WARN_FORMATSTRING, s_filen, pr_source_line, "%s", "cast to something else");
					break;
				}
			}

			s++;
			break;
		default:
			s++;
			break;
		}
	}
}

static void QCC_VerifyArgs_sendevent (const char *funcname, QCC_ref_t **arglist, unsigned int argcount)
{
	//arg0 is the event name, and not meaningful
	//arg1 denotes the arg types passed and defines the types used for all other args.
	const QCC_eval_t *eval = (argcount >= 2 && arglist[1]->type == REF_GLOBAL)?QCC_SRef_EvalConst(arglist[1]->base):NULL;
	const char *argtypes = eval?strings + eval->string:NULL;
	size_t arg = 2;
	QCC_type_t *t;
	char temp[256];
	if (!argtypes)
		return;

	for (; *argtypes; argtypes++, arg++)
	{
		switch(*argtypes)
		{
		case 's':	t = type_string;	break;
		case 'f':	t = type_float;		break;
		case 'F':	t = type_double;	break;
		case 'i':	t = type_integer;	break;
		case 'I':	t = type_int64;		break;
		case 'u':	t = type_uint;		break;
		case 'U':	t = type_uint64;	break;
		case 'v':	t = type_vector;	break;
		case 'e':	t = type_entity;	break;
		default:	t = NULL;			break;
		}
		if (arg >= argcount)
		{
			QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: arg type list longer than args", funcname);
			break;
		}
		else if (!t)
			QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: '%s%c%s' is not a recognised arg type", funcname, col_name, *argtypes, col_none);
		else if (typecmp_lax(arglist[arg]->cast, t))
			QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: '%s%c%s' specified, but was passed %s%s%s", funcname, col_name, *argtypes, col_none, col_type, TypeName(arglist[arg]->cast, temp, sizeof(temp)), col_none);
	}

	if (arg > 8)
		QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: cannot pass more than 8 args to a builtin.", funcname);
}

static void QCC_VerifyArgs_setviewprop (const char *funcname, QCC_ref_t **arglist, unsigned int argcount)
{
	static struct
	{
		const char *name;
		int n;
		etype_t t1;
		etype_t t2;
		etype_t t3;
	} argtypes[] =
	{
		{"VF_MIN",				1, ev_vector},
		{"VF_MIN_X",			2, ev_float},
		{"VF_MIN_Y",			3, ev_float},
		{"VF_SIZE",				4, ev_vector},
		{"VF_SIZE_X",			5, ev_float},
		{"VF_SIZE_Y",			6, ev_float},
		{"VF_VIEWPORT",			7, ev_vector, ev_vector},
		{"VF_FOV",				8, ev_vector},
		{"VF_FOVX",				9, ev_float},
		{"VF_FOVY",				10, ev_float},
		{"VF_ORIGIN",			11, ev_vector},
		{"VF_ORIGIN_X",			12, ev_float},
		{"VF_ORIGIN_Y",			13, ev_float},
		{"VF_ORIGIN_Z",			14, ev_float},
		{"VF_ANGLES",			15, ev_vector},
		{"VF_ANGLES_X",			16, ev_float},
		{"VF_ANGLES_Y",			17, ev_float},
		{"VF_ANGLES_Z",			18, ev_float},
		{"VF_DRAWWORLD",		19, ev_float},
		{"VF_ENGINESBAR",		20, ev_float},
		{"VF_DRAWCROSSHAIR",	21, ev_float},
//		{"VF_CARTESIAN_ANGLES",	22, ev_vector},
		{"VF_MINDIST",			23, ev_float},
		{"VF_MAXDIST",			24, ev_float},

		{"VF_CL_VIEWANGLES_V",	33, ev_vector},
		{"VF_CL_VIEWANGLES_X",	34, ev_float},
		{"VF_CL_VIEWANGLES_X",	35, ev_float},
		{"VF_CL_VIEWANGLES_X",	36, ev_float},
		{"VF_PERSPECTIVE",		200, ev_float},
//		{"VF_DP_CLEARSCENE",	201, ev_float},
		{"VF_ACTIVESEAT",		202, ev_float, ev_float},
		{"VF_AFOV",				203, ev_float},
//		{"VF_SCREENVSIZE",		204, ev_vector},
//		{"VF_SCREENPSIZE",		205, ev_vector},
		{"VF_VIEWENTITY",		206, ev_float},
//		{"VF_STATSENTITY",		207, ev_float},
//		{"VF_SCREENVOFFSET",	208, ev_float},
		{"VF_RT_SOURCECOLOUR",	209, ev_string},
		{"VF_RT_DEPTH",			210, ev_string, ev_float, ev_vector},
		{"VF_RT_RIPPLE",		211, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR0",	212, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR1",	213, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR2",	214, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR3",	215, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR4",	216, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR5",	217, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR6",	218, ev_string, ev_float, ev_vector},
		{"VF_RT_DESTCOLOUR7",	219, ev_string, ev_float, ev_vector},
		{"VF_ENVMAP",			220, ev_string},
		{"VF_USERDATA",			221, ev_pointer, ev_uint},
		{"VF_SKYROOM_CAMERA",	222, ev_vector},
//		{"VF_PIXELPSCALE",		223, ev_vector},
		{"VF_PROJECTIONOFFSET",	224, ev_vector},
		{"VF_VRBASEORIENTATION",225, ev_vector, ev_vector},

		{"VF_DP_MAINVIEW",			400, ev_float},
//		{"VF_DP_MINFPS_QUALITY",	401, ev_float},
	};

	char temp[256];
	const QCC_eval_t *ev;
	int i, vf;
	if (!argcount)
		return; // o.O
	ev = QCC_SRef_EvalConst(arglist[0]->base);
	if (!ev)	//can't check variables.
		return;
	vf = ev->_float;
	if (!qccwarningaction[WARN_ARGUMENTCHECK])
		return;	//don't bother if its not relevant anyway.

	for (i = 0; i < sizeof(argtypes)/sizeof(argtypes[0]); i++)
	{
		if (argtypes[i].n == vf)
		{
			if (argcount >= 2 && argtypes[i].t1 != ((arglist[1]->cast->type==ev_boolean)?arglist[1]->cast->parentclass->type:arglist[1]->cast->type))
			{
				QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s(%s, ...): expected %s, got %s", funcname, argtypes[i].name, basictypenames[argtypes[i].t1], TypeName(arglist[1]->cast, temp, sizeof(temp)));
				return;
			}
			if (argcount >= 3 && argtypes[i].t2 != ((arglist[2]->cast->type==ev_boolean)?arglist[2]->cast->parentclass->type:arglist[2]->cast->type))
			{
				QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s(%s, X, ...): expected %s, got %s", funcname, argtypes[i].name, basictypenames[argtypes[i].t2], TypeName(arglist[2]->cast, temp, sizeof(temp)));
				return;
			}
			if (argcount >= 4 && argtypes[i].t3 != ((arglist[3]->cast->type==ev_boolean)?arglist[3]->cast->parentclass->type:arglist[3]->cast->type))
			{
				QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s(%s, X, Y, ...): expected %s, got %s", funcname, argtypes[i].name, basictypenames[argtypes[i].t3], TypeName(arglist[3]->cast, temp, sizeof(temp)));
				return;
			}
			return;
		}
	}
	QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: unknown argument %i", funcname, vf);
}

void QCC_VerifyArgs_cvar(const char *funcname, QCC_ref_t *cvarname)
{
	if (cvarname->type == REF_GLOBAL && cvarname->cast->type == ev_string)
	{
		const QCC_eval_t *a = QCC_SRef_EvalConst(cvarname->base);
		const char *str;
		if (!a)
			return;
		str = strings + a->string;
		if (!strcmp(str, "vid_conwidth") || !strcmp(str, "vid_conheight"))
			QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: cvar(\"%s\") is deprecated and is likely to give some hacky value to work around old API usage which does not necessarily reflect the actual cvar value. Use getviewprop(VF_SCREENVSIZE) for the screen's virtual use, or use ftos+cvar_string to read the actual value of the cvar, or cast to variant to mute this warning.", funcname, str);
	}
}

void QCC_VerifyArgs_MatchingFieldType(const char *funcname, QCC_ref_t *type, QCC_ref_t *fldref)
{
	if (type->type == REF_GLOBAL && fldref->cast->type == ev_field)
	{
		char temp[256];
		const QCC_eval_t *a = QCC_SRef_EvalConst(type->base);
		if (a)
		{
			int i;
			if (type->cast->type == ev_integer || type->cast->type == ev_uint)
				i = a->_int;
			else if (type->cast->type == ev_float)
				i = a->_float;
			else
				return;
			if (i != fldref->cast->aux_type->type && fldref->cast->aux_type->type != ev_variant)
			{
				if (i >= 0 && i < ev_variant)
					QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: indicated type ev_%s does not match passed field type .%s", funcname, basictypenames[i], TypeName(fldref->cast->aux_type, temp, sizeof(temp)));
				else
					QCC_PR_ParseWarning(WARN_ARGUMENTCHECK, "%s: indicated type %i is not a basic type", funcname, i);
			}
		}
	}
}

#ifdef SUPPORTINLINE
struct inlinectx_s
{
	QCC_def_t		*fdef;
	QCC_function_t	*func;
	QCC_sref_t arglist[8];
	pbool argisout[8];

	QCC_sref_t		result;

	struct
	{
		QCC_def_t *def;
		QCC_def_t *srcsym;
		int bias;
	} locals[64];
	int numlocals;

	const char *error;
};
static pbool QCC_PR_InlinePushResult(struct inlinectx_s *ctx, QCC_sref_t src/*original statement's symbol*/, QCC_sref_t mappedto/*effective value*/)
{
	QCC_def_t *local;
	int i, p;
	for (i = 0; i < ctx->numlocals; i++)
	{
		if (ctx->locals[i].srcsym == src.sym)
			break;
	}
	if (i == ctx->numlocals)
	{
		if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
		{
			ctx->error = "too many temps";
			return false;
		}

		for (local = ctx->func->firstlocal, p = 0; local && p < MAX_PARMS && (unsigned int)p < ctx->func->type->num_parms; local = local->deftail->nextlocal, p++)
		{
			if (src.sym->symbolheader == local)
			{
				if (ctx->argisout[p])
				{
					/*if (ctx->arglist[p].sym->symbolheader != mappedto.sym || ctx->arglist[p].ofs != mappedto.ofs)
					{
//						ctx->error = "assignment wrote to variable other than intended output.";
						return false;
					}*/
					return true;
				}
			}
		}

		ctx->locals[i].srcsym = src.sym;
		ctx->numlocals++;
	}
	else if (ctx->locals[i].def)
		QCC_FreeDef(ctx->locals[i].def);

	ctx->locals[i].def = mappedto.sym;
	ctx->locals[i].bias = mappedto.ofs - src.ofs;	//FIXME: this feels unsafe (needed for array[immediate] fixups)
	return true;
}

static QCC_sref_t QCC_PR_InlineFindDef(struct inlinectx_s *ctx, QCC_sref_t src, pbool assign)
{
	QCC_def_t *d;
	int p;
//	int pstart = ctx->func->parm_start;

	//aliases are weird and annoying
	if (src.sym && src.sym->generatedfor)
		src.sym = src.sym->generatedfor;

	for (p = 0; p < ctx->numlocals; p++)
	{
		if (ctx->locals[p].srcsym == src.sym && ctx->locals[p].def)
		{
			d = ctx->locals[p].def;
			if (assign && src.sym)
			{
				if (!(src.sym->localscope || src.sym->temp))
				{	//update the symbol to refer to its original value...
//					QCC_FreeDef(ctx->locals[p].def);
					ctx->locals[p].def = ctx->locals[p].srcsym;
					ctx->locals[p].bias = 0;
					return QCC_MakeSRefForce(src.sym, src.ofs, src.cast);
				}
				//substitute the assignment with a new temp
//				QCC_FreeDef(ctx->locals[p].def);
				ctx->locals[p].srcsym = src.sym;
				ctx->locals[p].def = QCC_GetTemp(src.sym->type).sym;
				ctx->locals[p].bias = 0;
				return QCC_MakeSRefForce(ctx->locals[p].def, src.ofs, src.cast);
			}
			return QCC_MakeSRefForce(d, src.ofs + ctx->locals[p].bias, src.cast);
		}
	}

	if (src.sym && (src.sym->localscope || src.sym->temp))
	{
		//if its a parm, use that
		QCC_def_t *local;
		for (local = ctx->func->firstlocal, p = 0; local && p < MAX_PARMS && (unsigned int)p < ctx->func->type->num_parms; local = local->deftail->nextlocal, p++)
		{
			if (src.sym->symbolheader == local)
			{
				if (assign && !ctx->argisout[p])
				{
//					QCC_FreeDef(ctx->locals[p].def);
					ctx->locals[p].srcsym = src.sym;
					ctx->locals[p].def = QCC_GetTemp(src.sym->type).sym;
					ctx->locals[p].bias = 0;
					return QCC_MakeSRefForce(ctx->locals[p].def, src.ofs, src.cast);
				}
				return QCC_MakeSRefForce(ctx->arglist[p].sym->symbolheader, ctx->arglist[p].ofs+src.ofs, src.cast);
			}
		}

		//otherwise its a local or a temp.
		if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
			return nullsref;
		ctx->locals[ctx->numlocals].srcsym = src.sym;
		ctx->locals[ctx->numlocals].def = QCC_GetTemp(src.sym->type).sym;
		ctx->locals[ctx->numlocals].bias = 0;
		return QCC_MakeSRefForce(ctx->locals[ctx->numlocals++].def, src.ofs, src.cast);
	}
	return QCC_MakeSRefForce(src.sym, src.ofs, src.cast);
/*
	if (ofs < RESERVED_OFS)
	{
		if (ofs == OFS_RETURN)
		{
			def_ret.type = type_void;
			return &def_ret;
		}
		ofs -= OFS_PARM0;
		ofs /= 3;
		def_parms[ofs].type = type_void;
		return &def_parms[ofs];
	}
	if (ofs < pstart || ofs >= pstart+ctx->func->locals)
	{
		for (d = &pr.def_head; d; d = d->next)
		{
			if (d->ofs == ofs)
				return d;
		}
		return NULL;	//not found?
	}

	for (p = 0; p < ctx->func->numparms; pstart += ctx->func->parm_size[p++])
	{
		if (ofs < pstart+ctx->func->parm_size[p])
			return ctx->arglist[p];
	}

	if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
		return NULL;
	ctx->locals[ctx->numlocals].srcofs = ofs;
	ctx->locals[ctx->numlocals].def = QCC_GetTemp(type_float);
	return ctx->locals[ctx->numlocals++].def;
*/
}

//returns a string saying why inlining failed.
static const char *QCC_PR_InlineStatements(struct inlinectx_s *ctx)
{
	/*FIXME: what happens with:
	t = foo;
	foo = 5;
	return t;
	*/
	QCC_sref_t a, b, c;
	QCC_statement_t *st, *est;
	const QCC_eval_t *eval;
//	float af,bf;
//	int i;

	if (ctx->func->statements)
	{
		st = ctx->func->statements;
		est = st + ctx->func->numstatements;
	}
	else
	{
		st = &statements[ctx->func->code];
		est = statements+numstatements;
	}
	while(st < est)
	{
		switch(st->op)
		{
		case OP_IF_F:
		case OP_IFNOT_F:
		case OP_IF_I:
		case OP_IFNOT_I:
		case OP_IF_S:
		case OP_IFNOT_S:
			if (st->b.ofs > 0 && st[st->b.jumpofs].op == OP_DONE)
			{
				//logically, an if statement around the entire function is safe because the locals are safe
			}
		case OP_GOTO:
		case OP_SWITCH_F:
		case OP_SWITCH_I:
		case OP_SWITCH_E:
		case OP_SWITCH_FNC:
		case OP_SWITCH_S:
		case OP_SWITCH_V:
			return "function contains branches";	//conditionals are not supported in any way. this keeps the code linear. each input can only come from a single place.
/*		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
		case OP_CALL1H:
		case OP_CALL2H:
		case OP_CALL3H:
		case OP_CALL4H:
		case OP_CALL5H:
		case OP_CALL6H:
		case OP_CALL7H:
		case OP_CALL8H:
			return "function contains function calls";	//conditionals are not supported in any way. this keeps the code linear. each input can only come from a single place.
*/
		case OP_RETURN:
		case OP_DONE:
			a = QCC_PR_InlineFindDef(ctx, st->a, false);
			ctx->result = a;
			if (!a.cast)
			{
				if (ctx->func->type->aux_type->type == ev_void)
					ctx->result.cast = type_void;
				else
					return "OP_RETURN no a";
			}
			return NULL;
		case OP_BOUNDCHECK:
			a = QCC_PR_InlineFindDef(ctx, st->a, false);
			QCC_PR_InlinePushResult(ctx, st->a, a);
			eval = QCC_SRef_EvalConst(a);
			if (eval)
			{
				if (eval->_int < st->c.jumpofs || eval->_int >= st->b.jumpofs)
					QCC_PR_ParseWarning(0, "constant value exceeds bounds failed bounds check while inlining");
			}
			else
				QCC_PR_SimpleStatement(&pr_opcodes[OP_BOUNDCHECK], a, st->b, st->c, false);
			break;
		default:
			if ((st->op >= OP_CALL0 && st->op <= OP_CALL8) || (st->op >= OP_CALL1H && st->op <= OP_CALL8H))
			{
				//function calls are a little weird in that they have no outputs
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;

				if (st->b.cast)
				{
					b = QCC_PR_InlineFindDef(ctx, st->b, false);
					if (!b.cast)
						return "unable to determine what a was";
				}
				else
					b = nullsref;
				if (st->c.cast)
				{
					c = QCC_PR_InlineFindDef(ctx, st->c, false);
					if (!c.cast)
						return "unable to determine what a was";
				}
				else
					c = nullsref;

				{
					QCC_sref_t r;
					r.sym = &def_ret;
					r.ofs = 0;
					r.cast = a.cast;
					QCC_PR_InlinePushResult(ctx, r, nullsref);
				}

				QCC_ClobberDef(&def_ret);
				QCC_FreeTemp(a);
				QCC_FreeTemp(b);
				QCC_FreeTemp(c);
				QCC_LockActiveTemps(a);
				QCC_PR_SimpleStatement(&pr_opcodes[st->op], a, b, c, false);

				{
					QCC_sref_t r;
					r.sym = &def_ret;
					r.ofs = 0;
					r.cast = a.cast;
					QCC_PR_InlinePushResult(ctx, r, QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, a.cast->aux_type)));
				}
			}
			else if (pr_opcodes[st->op].flags & (OPF_STOREFLD|OPF_STOREPTROFS))
			{	//these forms don't write to any actual globals, we've no real scope for optimising these out.
				a = QCC_PR_InlineFindDef(ctx, st->a, false);
				b = QCC_PR_InlineFindDef(ctx, st->b, false);
				c = QCC_PR_InlineFindDef(ctx, st->c, false);
				QCC_PR_SimpleStatement(&pr_opcodes[st->op], a, b, c, false);
			}
			else if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
			{
				//a->b
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;
				b = QCC_PR_InlineFindDef(ctx, st->b, !(pr_opcodes[st->op].flags & OPF_STOREPTR));
				c = QCC_PR_StatementFlags(&pr_opcodes[st->op], a, b, NULL, 0);

				if (!QCC_PR_InlinePushResult(ctx, st->b, c))
					return ctx->error;
			}
			else if (OpAssignsToC(st->op))
			{
				//a+b->c
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;
				if (st->b.cast)
				{
					b = QCC_PR_InlineFindDef(ctx, st->b, false);
					if (!b.cast)
						return "unable to determine what b was";
				}
				else
					b = nullsref;

				if (pr_opcodes[st->op].associative == ASSOC_LEFT && pr_opcodes[st->op].type_c != &type_void)
				{
					QCC_sref_t r;
					c = QCC_PR_InlineFindDef(ctx, st->c, true);
					r = QCC_PR_StatementFlags(&pr_opcodes[st->op], a, b, NULL, STFL_NOEMULATE);
					if (c.cast && !QCC_SRef_EvalConst(r))
						c = QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], r, c, NULL, STFL_NOEMULATE);
					else
					{
						QCC_FreeTemp(c);
						c = r;
					}

					if (!QCC_PR_InlinePushResult(ctx, st->c, c))
						return ctx->error;
				}
				else
				{
					if (st->c.cast)
					{
						c = QCC_PR_InlineFindDef(ctx, st->c, false);
						if (!c.cast)
							return "unable to determine what c was";
					}
					else
						c = nullsref;
					QCC_PR_SimpleStatement(&pr_opcodes[st->op], a, b, c, false);
				}
			}
			else
			{
				return "nonstandard opcode form";
			}
			break;
		}
		st++;
	}

	a = QCC_PR_InlineFindDef(ctx, nullsref, false);
	ctx->result = a;
	if (!a.cast)
	{
		if (ctx->func->type->aux_type->type == ev_void)
			ctx->result.cast = type_void;
		else
			return "missing return type";
	}
	return NULL;
}
#endif
static QCC_sref_t QCC_PR_Inline(QCC_sref_t fdef, QCC_ref_t **arglist, unsigned int argcount)
{
#ifndef SUPPORTINLINE
	return nullsref;
#else
//	QCC_def_t *dd = NULL;
	struct inlinectx_s ctx;
	const char *error;
	int statements, i;
	unsigned int a;
	const QCC_eval_t *eval = QCC_SRef_EvalConst(fdef);
	//make sure that its a function type and that there's no special weirdness
	if (!eval || eval->function < 0 || argcount > 8 || eval->function >= numfunctions || fdef.sym->arraysize != 0 || fdef.cast->type != ev_function || argcount != fdef.cast->num_parms || fdef.cast->vargs || fdef.cast->vargcount)
	{
		QCC_PR_ParseWarning(0, "Couldn't inline \"%s\": %s", fdef.sym->name, "inconsistent context");
		return nullsref;
	}
	ctx.func = &functions[eval->function];
	if (fdef.cast != ctx.func->type)
	{
		QCC_PR_ParseWarning(0, "Couldn't inline \"%s\": %s", ctx.func->name, "function was cast");
		return nullsref;
	}
	ctx.numlocals = 0;
	for (a = 0; a < argcount; a++)
	{
		ctx.arglist[a] = QCC_RefToDef(arglist[a], true);
		ctx.argisout[a] = ctx.func->type->params[a].out;
	}
	ctx.fdef = fdef.sym;
	ctx.result = nullsref;
	ctx.error = NULL;
	if ((int)ctx.func->code <= 0)
	{
		char *fname = ctx.func->name;
		if (argcount == 1)
		{
			const QCC_eval_t *eval = QCC_SRef_EvalConst(ctx.arglist[0]);
			if (eval && !strcmp(fname, "sin"))
				return QCC_MakeFloatConst(sin(eval->_float));
			if (eval && !strcmp(fname, "cos"))
				return QCC_MakeFloatConst(cos(eval->_float));
			if (eval && !strcmp(fname, "floor"))
				return QCC_MakeFloatConst(floor(eval->_float));
			if (eval && !strcmp(fname, "ceil"))
				return QCC_MakeFloatConst(ceil(eval->_float));
			if (eval && !strcmp(fname, "rint"))
				return QCC_MakeFloatConst((int)((eval->_float>0)?(eval->_float+0.5):(eval->_float-0.5)));
			if (eval && !strcmp(fname, "fabs"))
				return QCC_MakeFloatConst(fabs(eval->_float));
			if (eval && !strcmp(fname, "sqrt"))
				return QCC_MakeFloatConst(sqrt(eval->_float));
			if (eval && !strcmp(fname, "log"))
				return QCC_MakeFloatConst(log(eval->_float));
			if (eval && !strcmp(fname, "log10"))
				return QCC_MakeFloatConst(log10(eval->_float));
			if (eval && !strcmp(fname, "ftoi"))
				return QCC_MakeIntConst(eval->_float);
			if (eval && !strcmp(fname, "itof"))
				return QCC_MakeFloatConst(eval->_int);
		}
		else if (argcount == 2)
		{
			const QCC_eval_t *a1 = QCC_SRef_EvalConst(ctx.arglist[0]);
			const QCC_eval_t *a2 = QCC_SRef_EvalConst(ctx.arglist[1]);
			if (a1 && a2 && !strcmp(fname, "pow"))
				return QCC_MakeFloatConst(pow(a1->_float, a2->_float));
		}

		return nullsref;	//don't try to inline builtins. that simply cannot work.
	}

	//FIXME: inefficient: we can't revert this on failure, so make sure its done early, just in case.
	if (argcount && ctx.arglist[0].sym->generatedfor == &def_ret)
		QCC_ClobberDef(&def_ret);
	QCC_ClobberDef(NULL);

	statements = numstatements;
	error = QCC_PR_InlineStatements(&ctx);
	if (!error)
		error = ctx.error;
	if (error)
	{
		QCC_PR_ParseWarning(0, "Couldn't inline \"%s\": %s", ctx.func->name, error);
		QCC_PR_ParsePrintDef(0, fdef.sym);
	}
	

	for(i = 0; i < ctx.numlocals; i++)
	{
		if (ctx.locals[i].def)
			QCC_FreeDef(ctx.locals[i].def);
	}

	if (!ctx.result.cast)
		QCC_UngenerateStatements(statements);
	else
	{	//on success, make sure the args were freed
		while (argcount-->0)
			QCC_FreeTemp(ctx.arglist[argcount]);
	}

	return ctx.result;
#endif
}
pbool QCC_Intrinsic_strlen(QCC_sref_t *result, const QCC_eval_t *a)
{
	const char *str = &strings[a->string];
	size_t l = 0;
	for (l = 0; str[l]; l++)
	{	//don't shortcut it when its an extended char. we don't know what the engine's strlen function would return.
		if ((unsigned char)str[l] & 0x80)
			return false;
	}
	if (l > 1u<<24)	//wait, what?
		return false;	//too big for a float.
	*result = QCC_MakeFloatConst(l);
	return true;
}
QCC_sref_t QCC_PR_GenerateFunctionCallRef (QCC_sref_t newself, QCC_sref_t func, QCC_ref_t **arglist, unsigned int argcount)	//warning, the func could have no name set if it's a field call.
{
	QCC_sref_t		d, oself, self, retval;
	unsigned int			i;
	QCC_type_t		*t;
//	int np;

	int callconvention;
	QCC_statement_t *st;
	unsigned int parm;
	struct
	{
		QCC_sref_t ref;
		int firststatement;
	} args[MAX_PARMS+MAX_EXTRA_PARMS];
	QCC_sref_t bigret = nullsref;

	const char *funcname = QCC_GetSRefName(func);
	if (opt_constantarithmatic && argcount == 1 && arglist[0]->type == REF_GLOBAL)
	{
		const QCC_eval_t *a = QCC_SRef_EvalConst(arglist[0]->base);
		if (a)
		{
//			if (!strcmp(funcname, "isnan"))
//				return QCC_MakeFloatConst(isnan(a->_float));
//			if (!strcmp(funcname, "isinf"))
//				return QCC_MakeFloatConst(isinf(a->_float));
			if (!strcmp(funcname, "floor"))
				return QCC_MakeFloatConst(floor(a->_float));
			if (!strcmp(funcname, "ceil"))
				return QCC_MakeFloatConst(ceil(a->_float));
			if (!strcmp(funcname, "sin"))
				return QCC_MakeFloatConst(sin(a->_float));
			if (!strcmp(funcname, "cos"))
				return QCC_MakeFloatConst(cos(a->_float));
			if (!strcmp(funcname, "log"))
				return QCC_MakeFloatConst(log(a->_float));
			if (!strcmp(funcname, "sqrt"))
				return QCC_MakeFloatConst(sqrt(a->_float));
//			if (!strcmp(funcname, "ftos"))
//				return QCC_MakeStringConst(ftos(a->_float));	//engines differ too much in their ftos implementation for this to be worthwhile

			if (!strcmp(funcname, "strlen") && QCC_Intrinsic_strlen(&d, a))
				return d;
			if (!strcmp(funcname, "stof"))
				return QCC_MakeFloatConst(atof(&strings[a->string]));
		}
	}
	else if (opt_constantarithmatic && argcount == 2 && arglist[0]->type == REF_GLOBAL && arglist[1]->type == REF_GLOBAL)
	{
		const QCC_eval_t *a = QCC_SRef_EvalConst(arglist[0]->base);
		const QCC_eval_t *b = QCC_SRef_EvalConst(arglist[1]->base);
		if (a && b)
		{
			if (arglist[0]->cast == type_float && arglist[1]->cast == type_float)
			{
				if (!strcmp(funcname, "pow"))
					return QCC_MakeFloatConst(pow(a->_float, b->_float));
				if (!strcmp(funcname, "mod"))
					return QCC_MakeFloatConst(fmodf((int)a->_float, (int)b->_float));
				if (!strcmp(funcname, "min"))
					return QCC_MakeFloatConst(min(a->_float, b->_float));
				if (!strcmp(funcname, "max"))
					return QCC_MakeFloatConst(max(a->_float, b->_float));
				if (!strcmp(funcname, "bitshift"))
				{
					if (b->_float < 0)
						return QCC_MakeFloatConst((int)a->_float >> (int)-b->_float);
					else
						return QCC_MakeFloatConst((int)a->_float << (int)b->_float);
				}
			}
		}
	}

	if (!strcmp(funcname, "sprintf"))
		QCC_VerifyFormatString(funcname, arglist, argcount);
	if (!strcmp(funcname, "cvar") && argcount == 1)
		QCC_VerifyArgs_cvar(funcname, arglist[0]);
	if ((!strcmp(funcname, "clientstat")||!strcmp(funcname, "addstat")) && argcount == 3)
		QCC_VerifyArgs_MatchingFieldType(funcname, arglist[1], arglist[2]);
	if (!strcmp(funcname, "setviewprop") || !strcmp(funcname, "setproperty"))
		QCC_VerifyArgs_setviewprop(funcname, arglist, argcount);
	if (!strcmp(funcname, "sendevent"))	//void(string eventname, string argtypes, ...)
		QCC_VerifyArgs_sendevent(funcname, arglist, argcount);

	func.sym->timescalled++;

	if (!newself.cast && func.sym->constant && func.sym->allowinline)
	{
		d = QCC_PR_Inline(func, arglist, argcount);
		if (d.cast)
		{
			optres_inlines++;
			func.sym->referenced = true;	//not really, but hey, the warning is stupid.
			QCC_FreeTemp(func);
			return d;
		}
	}

	if (QCC_OPCodeValid(&pr_opcodes[OP_CALL1H]))
		callconvention = OP_CALL1H;	//FTE extended
	else
		callconvention = OP_CALL1;	//standard

	t = func.cast;
	if (t->type == ev_function)
	{
		if (t->aux_type->size > type_vector->size)
			bigret = QCC_GetTemp(QCC_PR_PointerType(t->aux_type));
	}
	else if (t->type != ev_variant)
	{	//all varg
		QCC_PR_ParseErrorPrintSRef (ERR_NOTAFUNCTION, func, "not a function");
	}

	self = nullsref;
	oself = nullsref;
	d = nullsref;

	if (newself.cast)
	{
		//we're entering OO code with a different self. make sure self is preserved.
		//eg: other.touch(self)

		self = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
		if (newself.ofs != self.ofs || newself.sym != self.sym)
		{
			oself = QCC_GetTemp(pr_classtype?pr_classtype:type_entity);
			//oself = self
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], self, oself, nullsref, false);
			//self = other
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], newself, self, nullsref, false);

			//if the args refered to self, update them to refer to oself instead
			//(as self is now set to 'other')
			for (i = 0; i < argcount; i++)
			{
				if (arglist[i]->base.ofs == self.ofs && arglist[i]->base.sym && arglist[i]->base.sym->symbolheader == self.sym->symbolheader)
				{
					QCC_FreeTemp(arglist[i]->base);
					arglist[i]->base = oself;
					QCC_UnFreeTemp(arglist[i]->base);
				}
				if (arglist[i]->index.ofs == self.ofs && arglist[i]->index.sym && arglist[i]->index.sym->symbolheader == self.sym->symbolheader)
				{
					QCC_FreeTemp(arglist[i]->index);
					arglist[i]->index = oself;
					QCC_UnFreeTemp(arglist[i]->index);
				}
			}
		}
		else
		{
			QCC_FreeTemp(self);
			self = nullsref;
		}
		QCC_FreeTemp(newself);
	}

//	write the arguments into temps
	parm = 0;
	for (i = 0; i < argcount; i++)
	{
		if (func.cast->type == ev_function && func.cast->params && i < func.cast->num_parms && func.cast->params[i].out == 2)
		{
			args[parm].firststatement = numstatements;
			args[parm++].ref = nullsref;	//__out args do not actually need to pass anything.
		}
		else if (callconvention == OP_CALL1H && parm < 2 && arglist[i]->cast->size <= 3)
		{
			args[parm].firststatement = numstatements;
			args[parm++].ref = QCC_RefToDef(arglist[i], func.cast->type != ev_function || !func.cast->params || i >= func.cast->num_parms || !func.cast->params[i].out);
		}
		else
		{
			int firststatement;
			QCC_sref_t sref = nullsref, copyop_index = nullsref;
			int copyop[3] = {0,0,0}, copyop_idx=0;
			if (arglist[i]->postinc || arglist[i]->cast->align!=32)
			{
				arglist[i]->base = QCC_RefToDef(arglist[i], true);
				arglist[i]->index = nullsref;
				arglist[i]->type = REF_GLOBAL;
				arglist[i]->postinc = false;
				arglist[i]->readonly = true;
			}

			switch(arglist[i]->type)
			{
			case REF_GLOBAL:
			case REF_ARRAY:
				if (!arglist[i]->index.cast || QCC_SRef_EvalConst(arglist[i]->index))
					break;	//no problem
				if (arglist[i]->cast->align != 32)
					QCC_PR_ParseWarning (ERR_INTERNAL, "QCC_PR_GenerateFunctionCallRef: REF_ARRAY: align not 32");
				if (QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]))
				{
					copyop[2] = OP_LOADA_V;
					copyop[1] = OP_LOADA_I64;
					copyop[0] = OP_LOADA_F;
					copyop_idx = -2;	//offset the base ref
					copyop_index = arglist[i]->index;
					copyop_index = QCC_SupplyConversion(copyop_index, ev_integer, true);
					sref = arglist[i]->base;
				}
				else if (arglist[i]->base.sym->arraylengthprefix && QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]))
				{
					copyop[2] = 0;
					copyop[1] = 0;
					copyop[0] = OP_FETCH_GBL_F;
					copyop_idx = OP_ADD_F;
					copyop_index = arglist[i]->index;
					sref = arglist[i]->base;
				}
				break;
			case REF_FIELD:
				if (arglist[i]->cast->align != 32)
					QCC_PR_ParseWarning (ERR_INTERNAL, "QCC_PR_GenerateFunctionCallRef: REF_FIELD: align not 32");
				copyop[2] = OP_LOAD_V;
				copyop[1] = OP_LOAD_I64;
				copyop[0] = OP_LOAD_F;
				copyop_index = arglist[i]->index;
				copyop_idx = -1;
				if (!QCC_SRef_EvalConst(copyop_index))
				{	//if its a variable then its probably not a proper field (which has extra field ref values following it). do the shitty thing and make assumptions about ordering. :(
					copyop_index.cast = type_integer;
					copyop_idx = OP_ADD_I;
				}
				sref = arglist[i]->base;
				break;
			case REF_POINTER:
				if (arglist[i]->cast->align == 8)
				{
					copyop[2] = 0;
					copyop[1] = 0;
					copyop[0] = (arglist[i]->cast->type==ev_bitfld&&arglist[i]->cast->parentclass==type_integer)?OP_LOADP_I8:OP_LOADP_U8;
				}
				else if (arglist[i]->cast->align == 16)
				{
					copyop[2] = 0;
					copyop[1] = 0;
					copyop[0] = (arglist[i]->cast->type==ev_bitfld&&arglist[i]->cast->parentclass==type_integer)?OP_LOADP_I16:OP_LOADP_U16;
				}
				else
				{
					copyop[2] = OP_LOADP_V;
					copyop[1] = OP_LOADP_I64;
					copyop[0] = OP_LOADP_F;
				}
				copyop_idx = OP_ADD_I;
				copyop_index = arglist[i]->index;
				if (!copyop_index.cast)
					copyop_index = QCC_MakeUIntConst(0);	//don't bug out!
				sref = arglist[i]->base;
				break;
			default:
				//warning:reftodef may be doing a copy for various ref types. we're then copying it into the args as an extra step. this is obviously wasteful.
				//reading pointer, field, or struct types needs special code here to avoid that copy.
				break;
			}

			firststatement = numstatements;
			if (!sref.cast)
			{
				sref = QCC_RefToDef(arglist[i], func.cast->type != ev_function || !func.cast->params || i >= func.cast->num_parms || !func.cast->params[i].out);
				copyop[2] = OP_STORE_V;
				copyop[1] = OP_STORE_I64;
				copyop[0] = OP_STORE_F;
				copyop_idx = 0;
			}
			else if (!(func.cast->type != ev_function || !func.cast->params || i >= func.cast->num_parms || !func.cast->params[i].out))
			{
				QCC_UnFreeTemp(sref);
				QCC_UnFreeTemp(copyop_index);
			}

			if (copyop_index.cast || arglist[i]->cast->size > 3)
			{
				unsigned int ofs;
				QCC_sref_t src, fparm, newindex;
				int asz;
				src = sref;
				if (copyop_idx == OP_ADD_I && copyop_index.cast)
					copyop_index = QCC_SupplyConversion(copyop_index, ev_integer, true);
				else if (copyop_idx == OP_ADD_F && copyop_index.cast)
					copyop_index = QCC_SupplyConversion(copyop_index, ev_float, true);
				for (ofs = 0; ofs < arglist[i]->cast->size; )
				{
					if (copyop_idx == -1)
					{	//with this mode, the base reference can just be updated. no mess with the index.
						newindex = copyop_index;
						newindex.ofs += ofs;
						QCC_UnFreeTemp(copyop_index);
					}
					else if (copyop_idx == -2)
					{	//with this mode, the base reference can just be updated. no mess with the index.
						newindex = copyop_index;
						QCC_UnFreeTemp(copyop_index);
					}
					else if (copyop_idx == OP_ADD_I)
					{
						if (!copyop_index.cast)
							newindex = QCC_MakeIntConst(ofs);	//index can be simple constants
						else
						{	//if the index is a constant, then things get a little easier
							const QCC_eval_t *cnst = QCC_SRef_EvalConst(copyop_index);
							if (cnst)
								newindex = QCC_MakeIntConst(ofs + cnst->_int);
							else if (ofs)	//okay, looks like we'll have to actually do some maths then
								newindex = QCC_PR_StatementFlags (&pr_opcodes[OP_ADD_I], copyop_index, QCC_MakeIntConst(ofs), NULL, STFL_PRESERVEA);
							else	//first index needs no offset.
							{
								newindex = copyop_index;
								QCC_UnFreeTemp(copyop_index);
							}
						}
					}
					else if (copyop_idx == OP_ADD_F)
					{
						if (!copyop_index.cast)
							newindex = QCC_MakeFloatConst(ofs);	//index can be simple constants
						else
						{	//if the index is a constant, then things get a little easier
							const QCC_eval_t *cnst = QCC_SRef_EvalConst(copyop_index);
							if (cnst)
								newindex = QCC_MakeFloatConst(ofs + cnst->_float);
							else if (ofs)	//okay, looks like we'll have to actually do some maths then
								newindex = QCC_PR_StatementFlags (&pr_opcodes[OP_ADD_F], copyop_index, QCC_MakeFloatConst(ofs), NULL, STFL_PRESERVEA);
							else	//first index needs no offset.
							{
								newindex = copyop_index;
								QCC_UnFreeTemp(copyop_index);
							}
						}
					}
					else
						newindex = nullsref;

					asz = 3-(ofs%3);
					if (ofs+asz > arglist[i]->cast->size)
						asz = arglist[i]->cast->size-ofs;
					while (asz > 3 || !copyop[asz-1] || (asz>1&&!QCC_OPCodeValid(&pr_opcodes[copyop[asz-1]])))
						asz--;	//can't do that size...

					if (copyop[0] == OP_STORE_F)
					{
						if (ofs+asz != arglist[i]->cast->size)
							QCC_UnFreeTemp(src);

						if (!(ofs%3))
						{
							args[parm].firststatement = numstatements;
							args[parm].ref = QCC_GetTemp(type_vector);
							QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[copyop[asz-1]], src, args[parm].ref, NULL, STFL_PRESERVEB));
							parm++;
						}
						else
						{
							QCC_sref_t t = args[parm-1].ref;
							t.ofs += ofs%3;
							QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[copyop[asz-1]], src, t, NULL, STFL_PRESERVEB));
						}
					}
					else
					{
						if (ofs%3)
							parm--;
						if (parm>=MAX_PARMS)
						{
							fparm = extra_parms[parm - MAX_PARMS];
							if (!fparm.cast)
							{
								char name[128];
								QC_snprintfz(name, sizeof(name), "$parm%u", parm);
								fparm = extra_parms[parm - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
							}
							else
								QCC_ForceUnFreeDef(fparm.sym);
						}
						else
						{
							fparm.sym = &def_parms[parm];
							fparm.cast = type_vector;
							QCC_ForceUnFreeDef(fparm.sym);
						}
						fparm.ofs = ofs%3;
						if (!fparm.ofs)
						{
							args[parm].firststatement = numstatements;
							args[parm].ref = fparm;
						}
						parm++;

						if (ofs+asz == arglist[i]->cast->size)
						{
							QCC_FreeTemp(src);
							QCC_FreeTemp(copyop_index);
						}
						QCC_FreeTemp(newindex);

						if (copyop_idx == -2)
						{	//with this mode, the base reference can just be updated. no mess with the index.
							src.ofs += ofs;
							QCC_PR_SimpleStatement(&pr_opcodes[copyop[asz-1]], src, newindex, fparm, false);
							src.ofs -= ofs;
						}
						else
							QCC_PR_SimpleStatement(&pr_opcodes[copyop[asz-1]], src, newindex, fparm, false);
					}

					ofs += asz;

					if (copyop_idx == 0)
						src.ofs += asz;
				}
			}
			else
			{	//small and simple. yay.
				args[parm].firststatement = firststatement;
				args[parm].ref = sref;
				parm++;
			}
		}
	}

	//remap those temps to the actual parms if the parms were not used in the interim.

	for (i = ((callconvention==OP_CALL1H)?2:0); i < parm; i++)
	{
		if (i>=MAX_PARMS)
		{
			if (i - MAX_PARMS >= MAX_EXTRA_PARMS)
				QCC_PR_ParseErrorPrintSRef (ERR_TOOMANYTOTALPARAMETERS, func, "Function call needs too much paramater storage");

			d = extra_parms[i - MAX_PARMS];
			if (!d.cast)
			{
				char name[128];
				QC_snprintfz(name, sizeof(name), "$parm%u", i);
				d = extra_parms[i - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
				QCC_FreeTemp(d);
			}
		}
		else
		{
			d.sym = &def_parms[i];
			d.ofs = 0;
			d.cast = type_vector;
		}
		d.cast = args[i].ref.cast;
		if (!d.cast)
			continue;

		if (QCC_RemapTemp(args[i].firststatement, numstatements, args[i].ref, d))
		{
			QCC_FreeTemp(args[i].ref);
		}
		else
		{
			if (args[i].ref.sym != d.sym || args[i].ref.ofs != d.ofs)
			{
				QCC_ForceUnFreeDef(d.sym);
#if 0
				QCC_StoreToSRef(d, args[i].ref, d.cast, false, false);
#else
				if (args[i].ref.cast->size == 3)
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_V], args[i].ref, d, NULL, 0));
				else if (args[i].ref.cast->size == 2)
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[QCC_OPCodeValid(&pr_opcodes[OP_STORE_I64])?OP_STORE_I64:OP_STORE_V], args[i].ref, d, NULL, 0));
				else if (args[i].ref.cast->size == 1)
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], args[i].ref, d, NULL, 0));
				else
					QCC_PR_ParseErrorPrintSRef (ERR_BADEXTENSION, func, "arg storage not 1, 2, or 3.");
#endif
			}
			else
				QCC_FreeTemp(args[i].ref);
		}
		args[i].ref = d;
	}

	if (func.cast->vargcount)
	{
		QCC_sref_t va_passcount = QCC_PR_GetSRef(type_float, "__va_count", NULL, true, 0, 0);
		QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(argcount), va_passcount, NULL, 0));
	}

	//free any references to ofs_ret if we know we're not going to make any calls that will clobber it.
	//ie: func(func()); should not try to preserve ofs_return because of it being in use as part of the nested call
	if (callconvention == OP_CALL1H)
	{
		for (i = 0; i < parm && i < 2; i++)
			if (args[i].ref.sym && args[i].ref.sym->generatedfor == &def_ret && args[i].ref.sym->refcount == 1)
			{
				QCC_FreeTemp(args[i].ref);
				args[i].ref.sym = &def_ret;
				QCC_ForceUnFreeDef(args[i].ref.sym);
				break;
			}
	}

	QCC_ClobberDef(&def_ret);

	/*can free temps used for arguments now*/
	if (callconvention == OP_CALL1H)
	{
		for (i = 0; i < parm && i < 2; i++)
		{
			if (!args[i].ref.cast)
				continue;
			args[i].ref.sym->referenced=true;
			QCC_FreeTemp(args[i].ref);
		}
	}

	//we dont need to lock the local containing the function index because its thrown away after the call anyway
	//(if a function is called in the argument list then it'll be locked as part of that call)
	QCC_LockActiveTemps(func);	//any temps before are likly to be used with the return value.

	if (bigret.cast)	//get some storage
	{
		QCC_PR_SimpleStatement(&pr_opcodes[OP_PUSH], QCC_MakeUIntConst(bigret.cast->aux_type->size), nullsref, bigret, false);	//get some cheap/auto storage the child can safely write to.
		QCC_PR_SimpleStatement(QCC_OPCodeValid(&pr_opcodes[OP_STORE_P])?&pr_opcodes[OP_STORE_P]:&pr_opcodes[OP_STORE_F], bigret, QCC_MakeSRefForce(&def_ret, 0, bigret.cast), nullsref, false);	//let the child know where to write.
	}

	//generate the call
	if (parm>MAX_PARMS)
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[callconvention-1+MAX_PARMS], func, nullsref, (QCC_statement_t **)&st));
	else if (parm)
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[callconvention-1+parm], func, nullsref, (QCC_statement_t **)&st));
	else
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CALL0], func, nullsref, (QCC_statement_t **)&st));

	if (callconvention == OP_CALL1H)
	{
		if (parm)
		{
			st->b = args[0].ref;
			if (parm>1)
				st->c = args[1].ref;
		}
	}

	if (bigret.cast)	//get some storage
	{
		QCC_ref_t refbuf, *r;
		r = QCC_PR_BuildRef(&refbuf, REF_POINTER, bigret, nullsref, bigret.cast->aux_type, false, 0);
		retval = QCC_RefToDef(r, true);	//this line sucks perf
		//QCC_PR_SimpleStatement(&pr_opcodes[OP_POP], QCC_MakeUIntConst(bigret.cast->aux_type->size), nullsref, bigret, false); //should really be part of the qcc_ref_t
	}
	else if (t->type == ev_variant)
		retval = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_variant));
	else
		retval = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, t->aux_type));

	//restore the class owner
	if (oself.cast)
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], oself, self, nullsref, false);

	//handle outs.
	if (func.cast->type == ev_function && func.cast->params)
	{
		unsigned int parm = 0;
		for (i = 0; i < argcount && i < func.cast->num_parms; i++)	//fixme: parm offset should be (deftype->size+2)/3
		{
			if (func.cast->params[i].out)
			{
				if (oself.cast)
				{	//gah, this is messy
					if (arglist[i]->base.ofs == oself.ofs && arglist[i]->base.sym == oself.sym)
					{
						QCC_UnFreeTemp(self);
						QCC_FreeTemp(arglist[i]->base);
						arglist[i]->base = self;
					}
					if (arglist[i]->index.ofs == oself.ofs && arglist[i]->index.sym == oself.sym)
					{
						QCC_UnFreeTemp(self);
						QCC_FreeTemp(arglist[i]->index);
						arglist[i]->index = self;
					}
				}


				if (arglist[i]->readonly)
				{
					QCC_PR_ParseWarning(ERR_TYPEMISMATCHPARM, "Unable to write to read-only out argument");
					continue;
				}
				if (parm>=MAX_PARMS)
				{
					d = extra_parms[parm - MAX_PARMS];
					if (!d.cast)
					{
						char name[128];
						QC_snprintfz(name, sizeof(name), "$parm%u", parm);
						d = extra_parms[parm - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
					}
					else
						QCC_ForceUnFreeDef(d.sym);
				}
				else
				{
					d.sym = &def_parms[parm];
					d.ofs = 0;
					d.cast = type_vector;
					QCC_ForceUnFreeDef(d.sym);
				}
				d.cast = arglist[i]->cast;

				//FIXME: this may need to generate function calls, which can potentially clobber parms. This would be bad. we may need to copy them all out first THEN do the assignments.
				//FIXME: this can't cope with splitting return values over different extra_parms.
				QCC_StoreSRefToRef(arglist[i], d, false, false);
			}
			parm += (func.cast->params[i].type->size+2)/3;
		}
	}

	QCC_FreeTemp(oself);
	QCC_FreeTemp(self);

	return retval;
}
QCC_sref_t QCC_PR_GenerateFunctionCallSref (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t *arglist, int argcount)
{
	QCC_ref_t arg[MAX_PARMS];
	QCC_ref_t *outlist[MAX_PARMS];
	int i;
	for (i = 0; i < argcount; i++)
	{
		memset(&arg[i], 0, sizeof(arg[i]));
		arg[i].type = REF_GLOBAL;
		arg[i].base = arglist[i];
		arg[i].cast = arglist[i].cast;
		outlist[i] = &arg[i];
	}
	return QCC_PR_GenerateFunctionCallRef(newself, func, outlist, argcount);
}
QCC_sref_t QCC_PR_GenerateFunctionCall3 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a, QCC_sref_t b, QCC_type_t *type_b, QCC_sref_t c, QCC_type_t *type_c)
{
	QCC_ref_t arg_a = {REF_GLOBAL};
	QCC_ref_t arg_b = {REF_GLOBAL};
	QCC_ref_t arg_c = {REF_GLOBAL};
	QCC_ref_t *arglist[3] = {&arg_a, &arg_b, &arg_c};
	arg_a.base = a;
	arg_a.cast = type_a?type_a:a.cast;
	arg_b.base = b;
	arg_b.cast = type_b?type_b:b.cast;
	arg_c.base = c;
	arg_c.cast = type_c?type_c:c.cast;
	return QCC_PR_GenerateFunctionCallRef(newself, func, arglist, 3);
}
QCC_sref_t QCC_PR_GenerateFunctionCall2 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a, QCC_sref_t b, QCC_type_t *type_b)
{
	QCC_ref_t arg_a = {REF_GLOBAL};
	QCC_ref_t arg_b = {REF_GLOBAL};
	QCC_ref_t *arglist[2] = {&arg_a, &arg_b};
	arg_a.base = a;
	arg_a.cast = type_a?type_a:a.cast;
	arg_b.base = b;
	arg_b.cast = type_b?type_b:b.cast;
	return QCC_PR_GenerateFunctionCallRef(newself, func, arglist, 2);
}
QCC_sref_t QCC_PR_GenerateFunctionCall1 (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t a, QCC_type_t *type_a)
{
	QCC_ref_t arg_a = {REF_GLOBAL};
	QCC_ref_t *arglist[1] = {&arg_a};
	arg_a.base = a;
	arg_a.cast = type_a?type_a:a.cast;
	return QCC_PR_GenerateFunctionCallRef(newself, func, arglist, 1);
}


/*
============
PR_ParseFunctionCall
============
*/
static QCC_sref_t QCC_PR_ParseFunctionCall (QCC_ref_t *funcref)	//warning, the func could have no name set if it's a field call.
{
	QCC_sref_t	newself, func;
	QCC_sref_t	e, d, out;
	unsigned int			arg;
	QCC_type_t		*t, *p;
	int extraparms=false;
	unsigned int np;
	const char *funcname, *value;

	QCC_ref_t *param[MAX_PARMS+MAX_EXTRA_PARMS];
	QCC_ref_t parambuf[MAX_PARMS+MAX_EXTRA_PARMS];

	if (funcref->type == REF_FIELD && strstr(QCC_GetSRefName(funcref->index), "::"))
	{
		newself = funcref->base;
		QCC_UnFreeTemp(newself);
		func = QCC_RefToDef(funcref, true);
	}
	else if (funcref->type == REF_NONVIRTUAL)
	{
		newself = funcref->index;
		QCC_UnFreeTemp(newself);
		func = QCC_RefToDef(funcref, true);
	}
	else
	{
		newself = nullsref;
		func = QCC_RefToDef(funcref, true);
	}

	func.sym->timescalled++;

	t = func.cast;

	if (t->type == ev_variant)
	{
		t->aux_type = type_variant;
	}

	if (t->type != ev_function && t->type != ev_variant)
	{
		QCC_PR_ParseErrorPrintSRef (ERR_NOTAFUNCTION, func, "not a function");
	}

	funcname = QCC_GetSRefName(func);
	if (!newself.cast && !t->num_parms&&t->type != ev_variant)	//intrinsics. These base functions have variable arguments. I would check for (...) args too, but that might be used for extended builtin functionality. (this code wouldn't compile otherwise)
	{
		if (!strcmp(funcname, "alloca"))
		{	//FIXME: half of these functions with known arguments should be handled later or something
			QCC_sref_t sz, ret;

			func.sym->unused = true;
			func.sym->referenced = true;
			QCC_FreeTemp(func);

			sz = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			QCC_PR_Expect(")");
			sz = QCC_SupplyConversion(sz, ev_integer, true);
			//result = push_words((sz+3)/4);
			sz = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], sz, QCC_MakeIntConst(3), NULL);
			sz = QCC_PR_Statement(&pr_opcodes[OP_DIV_I], sz, QCC_MakeIntConst(4), NULL);
			QCC_FreeTemp(sz);
			ret = QCC_GetTemp(QCC_PointerTypeTo(type_void));
			QCC_PR_SimpleStatement(&pr_opcodes[OP_PUSH], sz, nullsref, ret, false);	//push *(int*)&a elements
			return ret;
		}
		if (!strcmp(funcname, "_"))
		{
			char *comment = pr_token_precomment;
			func.sym->unused = true;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_string)
			{
				d = QCC_MakeTranslateStringConst(pr_immediate_string);

				d.sym->comment = comment;
				QCC_PR_Lex();
				if (!d.sym->comment)
					d.sym->comment = pr_token_precomment;
				if (QCC_PR_CheckTokenComment (")", &d.sym->comment))
					return d;
			}
			else
			{
				QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "_() intrinsic accepts only a string immediate");
				d = nullsref;

			}
			QCC_PR_Expect(")");
			return d;
		}

		if (!strcmp(funcname, "va_arg") || !strcmp(funcname, "..."))	//second for compat with gmqcc
		{
			QCC_sref_t va_list;
			QCC_sref_t idx;
			QCC_type_t *type;
			va_list = QCC_PR_GetSRef(type_vector, "__va_list", pr_scope, false, 0, 0);
			idx = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			if (idx.cast->type == ev_float)
				idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], idx, QCC_MakeFloatConst(3), NULL);
			else
				idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], QCC_SupplyConversion(idx, ev_integer, true), QCC_MakeIntConst(3), NULL);
			QCC_PR_Expect(",");
			type = QCC_PR_ParseType(false, false, false);
			QCC_PR_Expect(")");
			if (!va_list.cast || !va_list.sym || !va_list.sym->arraysize)
				QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "va_arg() intrinsic only works inside varadic functions");

			func.sym->unused = true;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			return QCC_LoadFromArray(va_list, idx, type, false);
		}
		if (!strcmp(funcname, "random"))
		{
			func.sym->unused = true;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			if (!QCC_PR_CheckToken(")"))
			{
				e = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				e = QCC_SupplyConversion(e, ev_float, true);
				if (e.cast->type != ev_float)
					QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 1);
				if (!QCC_PR_CheckToken(")"))
				{
					QCC_PR_Expect(",");
					d = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
					d = QCC_SupplyConversion(d, ev_float, true);
					if (d.cast->type != ev_float)
						QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 2);
					QCC_PR_Expect(")");
				}
				else
					d = nullsref;
			}
			else
			{
				e = nullsref;
				d = nullsref;
			}

			if (QCC_OPCodeValid(&pr_opcodes[OP_RAND0]))
			{
				if(qcc_targetformat != QCF_HEXEN2 && qcc_targetformat != QCF_UHEXEN2)
					out = QCC_GetTemp(type_float);
				else
				{	//hexen2 requires the output be def_ret
					QCC_ClobberDef(&def_ret);
					out = nullsref;
				}
				if (e.cast)
				{
					if (d.cast)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND2], e, d, out, false);
						QCC_FreeTemp(d);
					}
					else
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND1], e, nullsref, out, false);
					QCC_FreeTemp(e);
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND0], nullsref, nullsref, out, false);
				if (!out.cast)
					out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_float));
			}
			else
			{
				QCC_ClobberDef(&def_ret);
				//this is normally a builtin, so don't bother locking temps.
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_float));
				if (d.cast)
				{
					QCC_sref_t t;
					//min + (max-min)*random()
					t = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], d, e, NULL, STFL_PRESERVEB);
					out = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], out, t, NULL);
					out = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], out, e, NULL);
				}
				else if (e.cast)
					out = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], out, e, NULL);
			}
			return out;
		}
		if (!strcmp(funcname, "randomv"))
		{
			func.sym->unused = true;
			func.sym->referenced=true;
			QCC_FreeTemp(func);
			if (!QCC_PR_CheckToken(")"))
			{
				e = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				if (e.cast->type != ev_vector)
					QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 1);
				if (!QCC_PR_CheckToken(")"))
				{
					QCC_PR_Expect(",");
					d = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
					if (d.cast->type != ev_vector)
						QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 2);
					QCC_PR_Expect(")");
				}
				else
					d = nullsref;
			}
			else
			{
				e = nullsref;
				d = nullsref;
			}

			if (QCC_OPCodeValid(&pr_opcodes[OP_RANDV0]))
			{
				if(qcc_targetformat != QCF_HEXEN2 && qcc_targetformat != QCF_UHEXEN2)
					out = QCC_GetTemp(type_vector);
				else
				{	//hexen2 requires the output be def_ret
					QCC_ClobberDef(&def_ret);
					out = nullsref;
				}
				if (e.cast)
				{
					if (d.cast)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV2], e, d, out, false);
						QCC_FreeTemp(d);
					}
					else
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV1], e, nullsref, out, false);
					QCC_FreeTemp(e);
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV0], nullsref, nullsref, out, false);
				if (!out.cast)
					out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_vector));
			}
			else
			{
				QCC_sref_t x,y,z;
				QCC_sref_t min = nullsref;
				QCC_sref_t scale = nullsref;
				if (d.cast)
				{
					min = e;
					scale = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_V], d, min, NULL, STFL_PRESERVEB);
				}
				else if (e.cast)
					scale = e;
				QCC_ClobberDef(&def_ret);
				out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_vector));
				x = out;
				x.cast = type_float;
				y = x;
				y.ofs += 1;
				z = y;
				z.ofs += 1;
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					scale.cast = type_float;
					scale.ofs += 2;
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, z, false);
					scale.ofs--;
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], x, z, nullsref, false);
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, y, false);
					scale.ofs--;
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], x, y, nullsref, false);
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, x, false);
					scale.sym->referenced = true;
					scale.ofs--;
					scale.cast = type_vector;
					QCC_FreeTemp(scale);
				}
				if (min.cast)
					out = QCC_PR_Statement(&pr_opcodes[OP_ADD_V], out, min, NULL);
			}

			return out;
		}
		else if (!strcmp(funcname, "spawn"))
		{
			//foo_c e = spawn(foo_c, fld:val, fld:val);	//regular spawn...
			//foo_c e = spawn(existingent, foo_c, fld:val, fld:val);	//placement-spawn...
			QCC_sref_t result = nullsref;
			QCC_type_t *rettype;
			
			/*
			ret = spawn();
			ret.FOO* = FOO*;
			result.(classcall)spawnfunc_foo();
			return result;
			this mechanism means entities can be spawned easily via maps.
			*/

			if (!QCC_PR_CheckToken(")"))
			{
				rettype = QCC_PR_ParseType(false, true, false);
				if (!rettype)
				{
					result = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
					QCC_PR_Expect(",");
					rettype = QCC_PR_ParseType(false, true, false);
				}
				//FIXME: C++'s Placement New syntax: obj *p= new(ptr) obj();
				if (!rettype || rettype->type != ev_entity)
					QCC_PR_ParseError(ERR_NOTANAME, "Spawn operator with undefined class: %s", QCC_PR_ParseName());
			}
			else
				rettype = NULL;	//default, corrected to entity later

			if (!result.cast)
			{
				//ret = spawn()
				result = QCC_PR_GenerateFunctionCallRef(nullsref, func, NULL, 0);
			}

			if (rettype)
			{
				char genfunc[256];
				//do field assignments.
				while(QCC_PR_CheckToken(","))
				{
					QCC_sref_t f, p, v;
					f = QCC_PR_ParseValue(rettype, false, false, true);
					if (f.cast->type != ev_field)
						QCC_PR_ParseError(0, "Named field is not a field.");
					if (QCC_PR_CheckToken("="))							//allow : or = as a separator, but throw a warning for =
						QCC_PR_ParseWarning(0, "That = should be a :");	//rejecting = helps avoid qcc bugs. :P
					else
						QCC_PR_Expect(":");
					v = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);

					p = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], result, f, NULL, STFL_PRESERVEA);
					if (v.cast->size == 3)
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_V], v, p, NULL));
					else
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_F], v, p, NULL));
				}
				QCC_PR_Expect(")");

				QC_snprintfz(genfunc, sizeof(genfunc), "spawnfunc_%s", rettype->name);
				func = QCC_PR_GetSRef(type_function, genfunc, NULL, true, 0, GDF_CONST);
				func.sym->referenced = true;

				QCC_UnFreeTemp(result);
				QCC_FreeTemp(QCC_PR_GenerateFunctionCallRef(result, func, NULL, 0));
				result.cast = rettype;
			}

			return result;
		}
		else if (!strcmp(funcname, "used_sound"))
		{
			e = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			QCC_PR_Expect(")");
			if ((value=QCC_SRef_EvalStringConst(e)))
				QCC_SoundUsed(value);
			else
				QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Argument to used_sound intrinsic was not a string immediate.");
			return e;
		}
		else if (!strcmp(funcname, "used_model"))
		{
			e = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			QCC_PR_Expect(")");
			if ((value=QCC_SRef_EvalStringConst(e)))
				QCC_SetModel(value);
			else
				QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "Argument to used_model intrinsic was not a string immediate.");
			return e;
		}
		else if (!strcmp(funcname, "autocvar") && !QCC_PR_CheckToken(")"))
		{
			char autocvarname[256];
			char *desc = NULL;
			QCC_FreeTemp(func);
			QC_snprintfz(autocvarname, sizeof(autocvarname), "autocvar_%s", QCC_PR_ParseName());
			QCC_PR_Expect(",");
			e = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			if (QCC_PR_CheckToken(","))
			{
				if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_string)
				{	//okay, its a string immediate, consume it for the description, being careful to not generate any string immediate defs (which still require an entry in the string table).
					desc = qccHunkAlloc(strlen(pr_immediate_string)+1);
					strcpy(desc, pr_immediate_string);
					QCC_PR_Lex ();
				}
			}
			QCC_PR_Expect(")");
			d = QCC_PR_GetSRef(e.cast, autocvarname, NULL, true, 0, GDF_USED);
			if (!d.sym->comment)
				d.sym->comment = desc;
			if (!e.sym->constant)
				QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "autocvar default value is not constant");
			
			if (d.sym->initialized)
			{
				if (memcmp(QCC_SRef_Data(d), QCC_SRef_Data(e), d.sym->symbolsize*sizeof(int)))
					QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, d, "autocvar %s was already initialised with another value", autocvarname+9);
			}
			else
			{
				memcpy(QCC_SRef_Data(d), QCC_SRef_Data(e), d.sym->symbolsize*sizeof(int));
				d.sym->initialized = true;
			}
			QCC_FreeTemp(e);
			return d;
		}
		else if (!strcmp(funcname, "entnum") && !QCC_PR_CheckToken(")"))
		{
			//t = (a/%1) / (nextent(world)/%1)
			//a/%1 does a (int)entity to float conversion type thing
			func.sym->unused = true;
			func.sym->referenced = true;
			QCC_FreeTemp(func);

			e = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			QCC_PR_Expect(")");
			e = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], e, QCC_MakeIntConst(1), NULL, 0);
			d = QCC_PR_EmulationFunc(nextent);
			if (!d.cast)
				QCC_PR_ParseError(0, "the nextent builtin is not defined");
			QCC_UnFreeTemp(e);
			d = QCC_PR_GenerateFunctionCall1 (nullsref, d, e, type_entity);
			d = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], d, QCC_MakeIntConst(1), NULL, 0);
			e = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], e, d, NULL, 0);

			return e;
		}
	}	//so it's not an intrinsic.
	else if (!newself.cast && t->num_parms == 1 && t->type == ev_function)
	{
		if (!strcmp(funcname, "checkbuiltin"))
		{
			pr_ignoredeprecation = true;
			param[0] = QCC_PR_RefExpression (&parambuf[0], TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			pr_ignoredeprecation = false;
			QCC_PR_Expect(")");

			if (param[0]->type == REF_GLOBAL && param[0]->cast == param[0]->base.sym->type && !param[0]->base.sym->arraysize)
			{
				e.ofs = param[0]->base.ofs;
				e.cast = param[0]->cast;
				e.sym = QCC_PR_DummyDef(e.cast, param[0]->base.sym->name, pr_scope, 0, param[0]->base.sym, 0, true, GDF_ALIAS|GDF_STRIP);
				e = QCC_PR_GenerateFunctionCallSref(newself, func, &e, 1);
			}
			else
				e = QCC_PR_GenerateFunctionCallRef(newself, func, param, 1);
			return e;
		}
	}

	if (opt_precache_file)	//should we strip out all precache_file calls?
	{
		if (!newself.cast && !strncmp(funcname,"precache_file", 13))
		{
			if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_string && pr_scope && !strcmp(pr_scope->name, "main"))
			{
				optres_precache_file += strlen(pr_immediate_string);
				QCC_PR_Lex();
				QCC_PR_Expect(")");
				QCC_PrecacheFile (pr_immediate_string, funcname[13]);
				QCC_FreeTemp(func);
				QCC_FreeTemp(newself);
				return QCC_MakeFloatConst(0);
			}
		}
	}

// copy the arguments to the global parameter variables
	arg = 0;
	if (t->type == ev_variant)
	{
		extraparms = true;
		np = 0;
	}
	else if (t->vargs)
	{
		extraparms = true;
		np = t->num_parms;
	}
	else
		np = t->num_parms;

	//any temps referenced to build the parameters don't need to be locked.
	if (!QCC_PR_CheckToken(")"))
	{
		QCC_ref_t *e;
		do
		{
			if (arg >= t->num_parms)
				p = NULL;
			else
				p = t->params[arg].type;

			if (arg >= MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseErrorPrintSRef (ERR_TOOMANYTOTALPARAMETERS, func, "More than %i parameters", MAX_PARMS+MAX_EXTRA_PARMS);

			if (QCC_PR_CheckToken("#"))
			{
				QCC_sref_t sr = QCC_MakeSRefForce(&def_parms[arg], 0, p?p:type_variant);
//				sr.sym = &def_parms[arg];
//				sr.ofs = 0;
//				sr.cast = p?p:type_variant;
				e = QCC_PR_BuildRef(&parambuf[arg], REF_GLOBAL, sr, nullsref, p?p:type_variant, true, 0);
			}
			else if (arg < t->num_parms && (QCC_PR_PeekToken (",") || QCC_PR_PeekToken (")")))
			{
				if (!func.cast->params[arg].defltvalue.cast)
					QCC_PR_ParseErrorPrintSRef (ERR_NOTDEFINED, func, "Default value not specified for implicit argument %i", arg+1);
				e = QCC_DefToRef(&parambuf[arg], func.cast->params[arg].defltvalue);
			}
			else
				e = QCC_PR_RefExpression(&parambuf[arg], TOP_PRIORITY, EXPR_DISALLOW_COMMA);

			if (extraparms && arg >= MAX_PARMS && !t->vargcount)
			{
				//vararg builtins cannot accept more than 8 args. they can't tell if they got more, and wouldn't know where to read them.
				QCC_PR_ParseWarning (WARN_TOOMANYPARAMETERSVARARGS, "More than %i parameters on varargs function", MAX_PARMS);
				QCC_PR_ParsePrintSRef(WARN_TOOMANYPARAMETERSVARARGS, func);
			}
			else if (!extraparms && arg >= t->num_parms && !p)
			{
				char buf[256];
				QCC_PR_ParseWarning (WARN_TOOMANYPARAMETERSFORFUNC, "too many parameters on call to %s, argument %s will be ignored", funcname, QCC_GetRefName(e, buf, sizeof(buf)));
				QCC_PR_ParsePrintSRef(WARN_TOOMANYPARAMETERSFORFUNC, func);
			}

			//with vectorcalls, we store the vector into the args as individual floats
			//this allows better reuse of vector constants.
			//the immediate vector def will be discarded while linking, if its still unused.
			if (opt_vectorcalls && e->cast == type_vector && e->type == REF_GLOBAL && !e->postinc && e->readonly)
			{
				const QCC_eval_t *eval = QCC_SRef_EvalConst(e->base);
				if (eval)
				{
					QCC_sref_t t = QCC_GetTemp(type_vector);
					t.cast = type_float;
					t.ofs = 0;
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(eval->vector[0]), t, NULL, STFL_PRESERVEB));
					t.ofs = 1;
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(eval->vector[1]), t, NULL, STFL_PRESERVEB));
					t.ofs = 2;
					QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(eval->vector[2]), t, NULL, STFL_PRESERVEB));

					t.ofs = 0;
					QCC_FreeTemp(e->base);

					e = QCC_PR_BuildRef(&parambuf[arg], REF_GLOBAL, t, nullsref, type_vector, true, 0);
				}
			}

			if (!p && e->cast->type == ev_float && t->vargtodouble)
				e = QCC_PR_BuildRef(&parambuf[arg], REF_GLOBAL, QCC_EvaluateCast(QCC_RefToDef(e, true), type_double, true), nullsref, p, true, 0);//C promotes floats to double on variadic functions, for some reason.
			else if (p && typecmp(e->cast, p))
			{
				e = QCC_PR_BuildRef(&parambuf[arg], REF_GLOBAL, QCC_EvaluateCast(QCC_RefToDef(e, true), p, true), nullsref, p, true, 0);
			}
			else if (QCC_RefNeedsCalls(e))
			{
				e = QCC_PR_BuildRef(&parambuf[arg], REF_GLOBAL, QCC_RefToDef(e, true), nullsref, p, true, 0);
			}
			param[arg] = e;

			if (arg == 0)
			{
			// save information for model and sound caching
				if (!strncmp(funcname,"precache_", 9) && e->cast->type == ev_string && e->base.cast->type == ev_string && e->type == REF_GLOBAL)
				{
					const QCC_eval_t *eval = QCC_SRef_EvalConst(e->base);
					if (eval)
					{
						const char *value = &strings[eval->string];
						if (!strncmp(funcname+9,"sound", 5))
							QCC_PrecacheSound (value, funcname[14]);
						else if (!strncmp(funcname+9,"model", 5))
							QCC_PrecacheModel (value, funcname[14]);
						else if (!strncmp(funcname+9,"texture", 7))
							QCC_PrecacheTexture (value, funcname[16]);
						else if (!strncmp(funcname+9,"file", 4))
							QCC_PrecacheFile (value, funcname[13]);
					}
				}
			}
			if (arg == 1 && e->cast->type == ev_string && e->type == REF_GLOBAL && !STRCMP(funcname, "setmodel") )
			{
				const QCC_eval_t *eval = QCC_SRef_EvalConst(e->base);
				if (eval)
				{
					const char *value = &strings[eval->string];
					QCC_SetModel(value);
				}
			}
			if (arg == 1 && e->cast->type == ev_string && e->type == REF_GLOBAL && !STRCMP(funcname, "localsound") )
			{
				const QCC_eval_t *eval = QCC_SRef_EvalConst(e->base);
				if (eval)
				{
					const char *value = &strings[eval->string];
					QCC_SoundUsed(value);
				}
			}
			if (arg == 2 && e->cast->type == ev_string && e->type == REF_GLOBAL && !STRCMP(funcname, "sound"))
			{
				const QCC_eval_t *eval = QCC_SRef_EvalConst(e->base);
				if (eval)
				{
					const char *value = &strings[eval->string];
					QCC_SoundUsed(value);
				}
			}
			arg++;
		} while (QCC_PR_CheckToken (","));
		QCC_PR_Expect (")");
	}

	//don't warn if we omited optional arguments
	while (arg < np && func.cast->params[arg].defltvalue.cast && !func.cast->params[arg].optional)
	{
		QCC_ForceUnFreeDef(func.cast->params[arg].defltvalue.sym);
		param[arg] = QCC_DefToRef(&parambuf[arg], func.cast->params[arg].defltvalue);
		arg++;
	}
	if (arg < np && func.cast->params[arg].optional)
		np = arg;
	if (arg < np)
	{
		/*if (arg+1==np && !strcmp(QCC_GetSRefName(func), "makestatic"))
		{
			//vanilla QC sucks. I want fteextensions.qc to compile with vanilla, yet also result in errors for when the mod fucks up.
			QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "too few parameters on call to %s. Passing 'self'.", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_COMPATIBILITYHACK, func);

			param[arg] = QCC_PR_GetSRef(NULL, "self", NULL, 0, 0, false);
			arg++;
		}
		else if (arg+1==np && !strcmp(QCC_GetSRefName(func), "ai_charge"))
		{
			//vanilla QC sucks. I want fteextensions.qc to compile with vanilla, yet also result in errors for when the mod fucks up.
			QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "too few parameters on call to %s. Passing 0.", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_COMPATIBILITYHACK, func);

			param[arg] = QCC_MakeFloatConst(0);
			arg++;
		}
		else*/
		{
			if (func.cast->params[arg].paramname)
				QCC_PR_ParseWarning (WARN_TOOFEWPARAMS, "too few parameters on call to %s, %s will be UNDEFINED", QCC_GetSRefName(func), func.cast->params[arg].paramname);
			else
				QCC_PR_ParseWarning (WARN_TOOFEWPARAMS, "too few parameters on call to %s", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_TOOFEWPARAMS, func);
		}
	}

	return QCC_PR_GenerateFunctionCallRef(newself, func, param, arg);
}

//returns a usable sref_t, always increases the def's refcount even if the def is not live yet. should only be used when a term is created/named.
//this special distinction allows temp reuse to be caught/debugged more reliably.
QCC_sref_t QCC_MakeSRefForce(QCC_def_t *def, unsigned int ofs, QCC_type_t *type)
{
	QCC_sref_t sr;
	sr.sym = def;
	sr.ofs = ofs;
	sr.cast = type;
	if (def)
		QCC_ForceUnFreeDef(def);
	return sr;
}
//makes a sref from a def+ofs+type. also increases refcount. considered an error if the specified def is not currently live.
QCC_sref_t QCC_MakeSRef(QCC_def_t *def, unsigned int ofs, QCC_type_t *type)
{
	QCC_sref_t sr;
	sr.sym = def;
	sr.ofs = ofs;
	sr.cast = type;
	if (def)
		QCC_UnFreeTemp(sr);
	return sr;
}
//int constchecks;
//int varchecks;
//int typechecks;
extern hashtable_t floatconstdefstable;
static QCC_sref_t QCC_Make32bitConst(QCC_type_t *type, puint_t value)	//none of these types may be mutilated by the engine (allowing 0s to merge)
{
	QCC_def_t	*cn;
	unsigned int key = value;

	cn = Hash_GetKey(&floatconstdefstable, key);
	while (cn)
	{
		if (cn->type->size == type->size)
			if (((QCC_eval_t*)cn->symboldata)->_uint == value)
				return QCC_MakeSRefForce(cn, 0, type);
		cn = Hash_GetNextKey(&floatconstdefstable, key, cn);
	}

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(value));
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;
	cn->referenced = true;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	((QCC_eval_t*)cn->symboldata)->_uint = value;

	Hash_AddKey(&floatconstdefstable, key, cn, qccHunkAlloc(sizeof(bucket_t)));

	return QCC_MakeSRefForce(cn, 0, type);
}
static QCC_sref_t QCC_Make64bitConst(QCC_type_t *type, puint64_t value)	//all values MUST be word-swapped, for big-endian machines to byteswap their 64bit immediates, or something.
{
	QCC_def_t	*cn;
	unsigned int key = value ^ (value>>32);

	cn = Hash_GetKey(&floatconstdefstable, key);
	while (cn)
	{
		if (cn->type->size == type->size)
			if (((QCC_eval_t*)cn->symboldata)->u64 == value)
				return QCC_MakeSRefForce(cn, 0, type);
		cn = Hash_GetNextKey(&floatconstdefstable, key, cn);
	}

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(value));
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	((QCC_eval_t*)cn->symboldata)->u64 = value;

	Hash_AddKey(&floatconstdefstable, key, cn, qccHunkAlloc(sizeof(bucket_t)));

	return QCC_MakeSRefForce(cn, 0, type);
}
static QCC_sref_t QCC_Make96bitConst(QCC_type_t *type, puint_t *value)	//basically just vectors...
{
	QCC_def_t	*cn;
	unsigned int key = value[0] ^ value[1] ^ value[2];

	cn = Hash_GetKey(&floatconstdefstable, key);
	while (cn)
	{
		if (cn->type->size == type->size)
			if (((puint_t*)cn->symboldata)[0] == value[0] &&
				((puint_t*)cn->symboldata)[1] == value[1] &&
				((puint_t*)cn->symboldata)[2] == value[2])
				return QCC_MakeSRefForce(cn, 0, type);
		cn = Hash_GetNextKey(&floatconstdefstable, key, cn);
	}

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(*value)*3);
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	((puint_t*)cn->symboldata)[0] = value[0];
	((puint_t*)cn->symboldata)[1] = value[1];
	((puint_t*)cn->symboldata)[2] = value[2];

	Hash_AddKey(&floatconstdefstable, key, cn, qccHunkAlloc(sizeof(bucket_t)));

	return QCC_MakeSRefForce(cn, 0, type);
}

QCC_sref_t QCC_MakeFloatConst(float value)
{
	union
	{
		float d;
		puint_t i;
	} u = {value};
	return QCC_Make32bitConst(type_float, u.i);
}
QCC_sref_t QCC_MakeIntConst(longlong llvalue)
{
	pint_t value = llvalue;
	if (value != llvalue)
		QCC_PR_ParseWarning(WARN_OVERFLOW, "Constant int operand %llu will be truncated to %i", llvalue, value);
	return QCC_Make32bitConst(type_integer, value);
}
QCC_sref_t QCC_MakeUIntConst(unsigned longlong llvalue)
{
	puint_t value = llvalue;
	if (value != llvalue)
		QCC_PR_ParseWarning(WARN_OVERFLOW, "Constant int operand %llu will be truncated to %i", llvalue, value);
	return QCC_Make32bitConst(type_uint, value);
}
QCC_sref_t QCC_MakeInt64Const(longlong llvalue)
{
	pint64_t value = llvalue;
	if (value != llvalue)
		QCC_PR_ParseWarning(WARN_OVERFLOW, "Constant int operand %llu will be truncated to %"pPRIi64, llvalue, value);
	return QCC_Make64bitConst(type_int64, value);
}
QCC_sref_t QCC_MakeUInt64Const(unsigned longlong llvalue)
{
	puint64_t value = llvalue;
	if (value != llvalue)
		QCC_PR_ParseWarning(WARN_OVERFLOW, "Constant int operand %llu will be truncated to %"pPRIu64, llvalue, value);
	return QCC_Make64bitConst(type_uint64, value);
}
QCC_sref_t QCC_MakeDoubleConst(double value)
{
	union
	{
		double d;
		puint64_t i;
	} u = {value};
	return QCC_Make64bitConst(type_double, u.i);
}

//immediates with no overlapping. this means aliases can be set up to mark them as strings/fields/functions and the engine can safely remap them as needed
static QCC_sref_t QCC_MakeUniqueConst(QCC_type_t *type, void *data)
{
	QCC_def_t	*cn;

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(pint_t) * type->size);
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	memcpy(cn->symboldata, data, sizeof(pint_t) * type->size);

	return QCC_MakeSRefForce(cn, 0, type);
}

static QCC_sref_t QCC_MakeGAddress(QCC_type_t *type, QCC_def_t *relocof, int idx, int bitofs)
{
	QCC_def_t	*cn;

	if (relocof->temp)
		QCC_PR_ParseWarning(ERR_INTERNAL, "generating reloc of temp");

	idx += bitofs>>5;
	bitofs &= 31;
	if (type->type == ev_pointer)
	{
		if (bitofs&7)
			QCC_PR_ParseWarning(ERR_INTERNAL, "pointer has too fine granularity");
		idx = idx*VMWORDSIZE + (bitofs>>3);	//fix up fte style pointer
	}
	else
	{
		if (bitofs&7)
			QCC_PR_ParseWarning(ERR_INTERNAL, "pointer has too fine granularity");
		//sucky granularity
	}

	if (relocof->gaddress && !idx && relocof->gaddress->type->type == type->type)
		return QCC_MakeSRefForce(relocof->gaddress, 0, type);

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(pint_t) * type->size);
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 0;	//we don't know addresses until the end, which hurts folding. :(
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	cn->reloc = relocof;
	if (!idx && !bitofs)
		relocof->gaddress = cn;

	memset(cn->symboldata, 0, sizeof(pint_t) * type->size);
	cn->symboldata->_int = idx;

	return QCC_MakeSRefForce(cn, 0, type);
}



QCC_sref_t QCC_PR_GenerateVector(QCC_sref_t x, QCC_sref_t y, QCC_sref_t z);
QCC_sref_t QCC_MakeVectorConst(pvec_t a, pvec_t b, pvec_t c)
{
/*	QCC_def_t	*cn;

// check for a constant with the same value
	for (cn=pr.def_head.next ; cn ; cn=cn->next)
	{
		if (!cn->initialized)
			continue;
		if (!cn->constant)
			continue;
		if (cn->type != type_vector)
			continue;
		if (cn->arraysize)
			continue;

		if (cn->symboldata[0].vector[0] == a &&
			cn->symboldata[0].vector[1] == b &&
			cn->symboldata[0].vector[2] == c)
		{
			return QCC_MakeSRefForce(cn, 0, type_vector);
		}
	}*/

	{
		union
		{
			pvec_t f[3];
			pint_t i[3];
		} u = {{a,b,c}};
		return QCC_Make96bitConst(type_vector, u.i);
	}
}

extern hashtable_t stringconstdefstable, stringconstdefstable_trans;
int dotranslate_count;
static QCC_sref_t QCC_MakeStringConstInternal(const char *value, size_t length, pbool translate)
{
	QCC_def_t	*cn;
	int string;
	pbool usehash = (length == strlen(value)+1);	//if there are embedded nulls, our hash code will not be able to cope.

	if (usehash)
	{
		cn = pHash_Get(translate?&stringconstdefstable_trans:&stringconstdefstable, value);
		if (cn)
		{
			return QCC_MakeSRefForce(cn, 0, type_string);
		}
	}

// allocate a new one
	if(translate)
	{
		char buf[64];
		QC_snprintfz(buf, sizeof(buf), "dotranslate_%i", ++dotranslate_count);
		cn = (void *)qccHunkAlloc (sizeof(QCC_def_t)+sizeof(string_t) + strlen(buf)+1);
		cn->name = (char*)((string_t*)(cn+1)+1);
		strcpy(cn->name, buf);
		cn->used = true;	//
		cn->referenced = true;
		cn->nofold = true;
	}
	else
	{
		cn = (void *)qccHunkAlloc (sizeof(QCC_def_t)+sizeof(string_t));
		cn->name = "IMMEDIATE";
	}
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type_string;
	cn->constant = !translate;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;
	cn->localscope = false;

	cn->filen = s_filen;
	cn->s_line = pr_source_line;

// copy the immediate to the global area
	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_basic_t*)(cn+1);

	if (usehash)
	{
		string = QCC_CopyString (value);
		pHash_Add(translate?&stringconstdefstable_trans:&stringconstdefstable, strings+string, cn, qccHunkAlloc(sizeof(bucket_t)));
	}
	else
		string = QCC_CopyStringLength (value, length);

	cn->symboldata[0].string = string;

	return QCC_MakeSRefForce(cn, 0, type_string);
}

QCC_sref_t QCC_MakeStringConstLength(const char *value, int length)
{
	return QCC_MakeStringConstInternal(value, length, false);
}
QCC_sref_t QCC_MakeStringConst(const char *value)
{
	return QCC_MakeStringConstInternal(value, strlen(value)+1, false);
}
QCC_sref_t QCC_MakeTranslateStringConst(const char *value)
{
	return QCC_MakeStringConstInternal(value, strlen(value)+1, true);
}

QCC_type_t *QCC_PointerTypeTo(QCC_type_t *type)
{
	QCC_type_t *newtype;
	newtype = QCC_PR_NewType("ptr", ev_pointer, false);
	newtype->aux_type = type;
	return newtype;
}

QCC_type_t *QCC_GenArrayType(QCC_type_t *type, unsigned int arraysize)
{
	struct QCC_typeparam_s *param = qccHunkAlloc(sizeof(*param));
	param->type = type;
	param->arraysize = arraysize;
	param->paramname = NULL;
	type = QCC_PR_NewType("array", ev_union, false);
	type->params = param;
	type->num_parms = 1;
	if (param->type->bits)
	{
		type->bits = param->type->bits * param->arraysize;
		type->size = (param->type->bits * param->arraysize + 31) & ~31;
	}
	else
		type->size = param->type->size * param->arraysize;
	type->align = param->type->align;
	return type;
}

QCC_type_t **basictypes[] =
{
	&type_void,
	&type_string,
	&type_float,
	&type_vector,
	&type_entity,
	&type_field,
	&type_function,
	&type_pointer,
	&type_integer,
	&type_uint,
	&type_int64,
	&type_uint64,
	&type_double,
	&type_variant,
	NULL,	//type_struct
	NULL,	//type_union
	NULL,	//type_accessor
	NULL,	//type_enum
	NULL,	//type_boolean
};

/*static QCC_def_t *QCC_MemberInParentClass(char *name, QCC_type_t *clas)
{	//if a member exists, return the member field (rather than mapped-to field)
	QCC_def_t *def;
	unsigned int p;
	char membername[2048];

	if (!clas)
	{
		def = QCC_PR_GetDef(NULL, name, NULL, 0, 0, false);
		if (def && def->type->type == ev_field)	//the member existed as a normal entity field.
			return def;
		return NULL;
	}

	for (p = 0; p < clas->num_parms; p++)
	{
		if (strcmp(clas->params[p].paramname, name))
			continue;

		//the parent has it.

		QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, clas->name, clas->params[p].paramname);
		def = QCC_PR_GetDef(NULL, membername, NULL, false, 0, false);
		if (def)
			return def;
		break;
	}

	return QCC_MemberInParentClass(name, clas->parentclass);
}*/

static void QCC_PR_EmitClassFunctionTable(QCC_type_t *clas, QCC_type_t *childclas, QCC_sref_t ed)
{	//go through clas, do the virtual thing only if the child class does not override.

	char membername[2048];
	QCC_type_t *type;
	QCC_type_t *oc;
	unsigned int p;

	QCC_sref_t point, member;
	QCC_sref_t virt;

	if (clas->parentclass)
		QCC_PR_EmitClassFunctionTable(clas->parentclass, childclas, ed);

	for (p = 0; p < clas->num_parms; p++)
	{
		type = clas->params[p].type;
		for (oc = childclas; oc != clas; oc = oc->parentclass)
		{
			QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, oc->name, clas->params[p].paramname);
			if (QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false).cast)
				break;	//a child class overrides.
		}
		if (oc != clas)
			continue;

		if (type->type == ev_function)	//FIXME: inheritance will not install all the member functions.
		{
			member = nullsref;
			for (oc = childclas; oc && !member.cast; oc = oc->parentclass)
			{
				QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, oc->name, clas->params[p].paramname);
				member = QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false);
			}
			if (!member.cast)
			{
				QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, clas->name, clas->params[p].paramname);
				QCC_PR_Warning(ERR_INTERNAL, NULL, 0, "Member function %s was not defined", membername);
				continue;
			}
			QC_snprintfz(membername, sizeof(membername), "%s::%s", clas->name, clas->params[p].paramname);
			virt = QCC_PR_GetSRef(type, membername, NULL, false, 0, false);
			if (!virt.cast)
			{
				QCC_PR_Warning(0, NULL, 0, "Member function %s was not defined", membername);
				continue;
			}
			point = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], ed, member, NULL, STFL_PRESERVEA);
			type_pointer->aux_type = virt.cast;
			QCC_PR_Statement(&pr_opcodes[OP_STOREP_FNC], virt, point, NULL);
		}
	}
}

//take all functions in the type, and parent types, and make sure the links all work properly.
void QCC_PR_EmitClassFromFunction(QCC_def_t *scope, QCC_type_t *basetype)
{
	QCC_type_t *parenttype;

	QCC_sref_t ed;
	QCC_sref_t constructor;
	int basictypefield[ev_union+1];

	int src,dst;

//	int func;

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	pr_scope = NULL;
	memset(basictypefield, 0, sizeof(basictypefield));
//	QCC_PR_EmitFieldsForMembers(basetype, basictypefield);


	pr_source_line = pr_token_line_last = scope->s_line;

	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type, NULL);
	//reset the locals chain
	pr.local_head.nextlocal = NULL;
	pr.local_tail = &pr.local_head;

	scope->initialized = true;
	scope->symboldata[0].function = pr_scope - functions;

	ed = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);

	{
		QCC_sref_t fclassname = QCC_PR_GetSRef(NULL, "classname", NULL, false, 0, false);
		if (fclassname.cast)
		{
			QCC_sref_t point = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], ed, fclassname, NULL, STFL_PRESERVEA);
			type_pointer->aux_type = type_string;
			QCC_PR_Statement(&pr_opcodes[OP_STOREP_FNC], QCC_MakeStringConst(basetype->name), point, NULL);
		}
	}

	QCC_PR_EmitClassFunctionTable(basetype, basetype, ed);

	src = numstatements;
	for (parenttype = basetype; parenttype; parenttype = parenttype->parentclass)
	{
		char membername[2048];
		QC_snprintfz(membername, sizeof(membername), "%s::%s", parenttype->name, parenttype->name);
		constructor = QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false);

		if (constructor.cast)
		{
			constructor.sym->referenced = true;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], constructor, nullsref, nullsref, false);
			QCC_FreeTemp(constructor);
		}
	}

	if (flag_rootconstructor)
	{
		dst = numstatements-1;
		while(src < dst)
		{
			QCC_sref_t t = statements[src].a;
			statements[src].a = statements[dst].a;
			statements[dst].a = t;
			src++;
			dst--;
		}
	}

	QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));



	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);

	QCC_Marshal_Locals(pr_scope->code, numstatements);
}

static QCC_sref_t QCC_PR_ExpandField(QCC_sref_t ent, QCC_sref_t field, QCC_type_t *fieldtype, unsigned int preserveflags)
{
	QCC_type_t *basicfieldtype;
	QCC_sref_t r;
	if (!fieldtype)
	{
		if (field.cast->type == ev_field)
			fieldtype = field.cast->aux_type;
		else
		{
			if (field.cast->type != ev_variant)
				QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, field, "QCC_PR_ExpandField: invalid field type");
			fieldtype = type_variant;
		}
	}
	basicfieldtype = fieldtype;
	while(basicfieldtype->type == ev_accessor || basicfieldtype->type == ev_boolean || basicfieldtype->type == ev_enum)
		basicfieldtype = (basicfieldtype->type == ev_enum)?basicfieldtype->aux_type:basicfieldtype->parentclass;

	//FIXME: class.staticmember should directly read staticmember instead of trying to dereference
	switch(basicfieldtype->type)
	{
	case ev_struct:
	case ev_union:
		{
			int i = 0;
			QCC_type_t *type = fieldtype;
			QCC_sref_t dest = QCC_GetTemp(type);
			QCC_sref_t source = field;
			//don't bother trying to optimise any temps here, its not likely to happen anyway.
			for (; i+2 < type->size; i+=3, dest.ofs += 3, source.ofs += 3)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_LOAD_V], ent, source, dest, false);
			if (QCC_OPCodeValid(&pr_opcodes[OP_LOAD_I64]))
				for (; i+1 < type->size; i+=2, dest.ofs += 2, source.ofs += 2)
					QCC_PR_SimpleStatement(&pr_opcodes[OP_LOAD_I64], ent, source, dest, false);
			for (; i < type->size; i++, dest.ofs++, source.ofs++)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_LOAD_F], ent, source, dest, false);
			source.ofs -= type->size;
			dest.ofs -= type->size;
			if (!(preserveflags & STFL_PRESERVEA))
				QCC_FreeTemp(ent);
			if (!(preserveflags & STFL_PRESERVEB))
				QCC_FreeTemp(field);

			if (type->size > 3)
				QCC_PR_ParseWarning(WARN_UNDESIRABLECONVENTION, "inefficient - copying %u words to a temp", type->size);
			return dest;
		}
		break;
	case ev_void:
	case ev_accessor:
	case ev_boolean:
	case ev_enum:
	default:
		{
			char temp[256];
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, field, "QCC_PR_ExpandField: invalid field type %s%s%s",  col_type,TypeName(fieldtype, temp, sizeof(temp)),col_none);
		}
		r = field;
		break;
	case ev_integer:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I], ent, field, NULL, preserveflags);
		break;
	case ev_uint:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I], ent, field, NULL, preserveflags);
		r.cast = type_uint;
		break;
	case ev_double:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I64], ent, field, NULL, preserveflags);
		r.cast = type_double;
		break;
	case ev_int64:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I64], ent, field, NULL, preserveflags);
		r.cast = type_int64;
		break;
	case ev_uint64:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I64], ent, field, NULL, preserveflags);
		r.cast = type_uint64;
		break;

	case ev_pointer:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_P], ent, field, NULL, preserveflags);
		break;
	case ev_field:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FLD], ent, field, NULL, preserveflags);
		break;
	case ev_variant:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FLD], ent, field, NULL, preserveflags);
		break;
	case ev_float:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_F], ent, field, NULL, preserveflags);
		break;
	case ev_string:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_S], ent, field, NULL, preserveflags);
		break;
	case ev_vector:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_V], ent, field, NULL, preserveflags);
		break;
	case ev_function:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FNC], ent, field, NULL, preserveflags);
		break;
	case ev_entity:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_ENT], ent, field, NULL, preserveflags);
		break;
	}
	r.cast = fieldtype;
	return r;
}

/*checks for <DEF>.foo and expands in a class-aware fashion
normally invoked via QCC_PR_ParseArrayPointer
*/
static QCC_ref_t *QCC_PR_ParseField(QCC_ref_t *refbuf, QCC_ref_t *lhs)
{
	QCC_type_t *t;
	t = lhs->cast;
	if ((t->accessors || t->type == ev_entity) && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
	{
		QCC_ref_t *field;
		QCC_ref_t fieldbuf;

		if (pr_token_type == tt_name)
		{
			QCC_sref_t index = nullsref;
			char *fieldname = pr_token;
			struct accessor_s *acc = NULL, *anon = NULL;
			QCC_type_t *a;

			for (a = t; a && !acc; a = a->parentclass)
				for (acc = a->accessors; acc; acc = acc->next)
				{
					if (!*acc->fieldname && acc->indexertype)
					{
						if (!anon)
							anon = acc;
					}
					else if (!strcmp(acc->fieldname, fieldname))
					{
						fieldname = QCC_PR_ParseName(); //do it for real now.
						if (acc->indexertype)
						{
							if (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->"))
								index = QCC_MakeStringConst(QCC_PR_ParseName());
							else
							{
								QCC_PR_Expect("[");
								index = QCC_PR_Expression (TOP_PRIORITY, 0);
								QCC_PR_Expect("]");
							}
						}
						break;
					}
				}
			if (!acc && anon)
			{
				acc = anon;
				fieldname = QCC_PR_ParseName(); //do it for real now.
				index = QCC_MakeStringConst(fieldname);
			}
			if (acc)
			{
				lhs = QCC_PR_BuildAccessorRef(refbuf, QCC_RefToDef(lhs, true), index, acc, lhs->readonly);
				lhs = QCC_PR_ParseField(refbuf, lhs);
				return lhs;
			}
		}

		if (t->type == ev_entity)
		{
			if (QCC_PR_CheckToken("("))
			{
				field = QCC_PR_RefExpression(&fieldbuf, TOP_PRIORITY, 0);
				QCC_PR_Expect(")");
			}
			else
				field = QCC_PR_ParseRefValue(&fieldbuf, t, false, false, true);
			if (field->type != REF_ARRAYHEAD && (field->cast->type == ev_field || field->cast->type == ev_variant))
			{
				//fields are generally always readonly. that refers to the field def itself, rather than products of said field.
				//entities, like 'world' might also be consts. just ignore that fact. the def itself is not assigned, but the fields of said def.
				//the engine may have a problem with this, but the qcc has no way to referenced locations as readonly separately from the def itself.
				lhs = QCC_PR_BuildRef(refbuf, REF_FIELD, QCC_RefToDef(lhs, true), QCC_RefToDef(field, true), (field->cast->type == ev_field)?field->cast->aux_type:type_variant, false, 0);
			}
			else
			{
				if (field->type == REF_GLOBAL && strstr(QCC_GetSRefName(field->base), "::"))
				{
					QCC_sref_t theent = QCC_RefToDef(lhs, true);
					*refbuf = *field;
					refbuf->type = REF_NONVIRTUAL;
					refbuf->index = theent;
					return refbuf;
				}
				if (t->parentclass)
					QCC_PR_ParseError(ERR_BADMEMBER, "%s is not a field of class %s", QCC_GetSRefName(QCC_RefToDef(field, false)), t->name);
				else
					QCC_PR_ParseError(ERR_BADMEMBER, "%s is not a field", QCC_GetSRefName(QCC_RefToDef(field, false)));
			}

			lhs = QCC_PR_ParseField(refbuf, lhs);

			lhs = QCC_PR_ParseRefArrayPointer (refbuf, lhs, false, false);
		}
		else
		{
			QCC_PR_ParseWarning(ERR_BADMEMBER, "%s is not a member of %s", QCC_PR_ParseName(), t->name);
			if (t->filen)
				QCC_PR_Note(ERR_BADMEMBER, t->filen, t->line, "%s is defined here", t->name);
			return QCC_PR_BuildRef(refbuf, REF_GLOBAL, QCC_MakeIntConst(0), nullsref, type_void, false, 0);
		}
	}
	else if (flag_qccx && t->type == ev_entity && QCC_PR_CheckToken("["))
	{	//p[%0] gives a regular array reference. except that p is probably a float, and we're expecting OP_LOAD_F
		//might also be assigned to, so just create a regular field ref and figure that stuff out later.
		QCC_ref_t *field;
		QCC_ref_t fieldbuf;
		field = QCC_PR_RefExpression(&fieldbuf, TOP_PRIORITY, 0);
		field->cast = type_floatfield;
		QCC_PR_Expect("]");

		lhs = QCC_PR_BuildRef(refbuf, REF_FIELD, QCC_RefToDef(lhs, true), QCC_RefToDef(field, true), type_float, false, 0);

		
		lhs = QCC_PR_ParseField(refbuf, lhs);
		lhs = QCC_PR_ParseRefArrayPointer (refbuf, lhs, false, false);
	}
	else if (flag_qccx && t->type == ev_entity && QCC_PR_CheckToken("^"))
	{	//p^[%0] is evaluated as an OP_LOAD_V (or OP_ADDRESS+OP_STOREP_V)
		QCC_ref_t *field;
		QCC_ref_t fieldbuf;
		QCC_PR_Expect("[");
		field = QCC_PR_RefExpression(&fieldbuf, TOP_PRIORITY, 0);
		field->cast = type_floatfield;
		QCC_PR_Expect("]");

		lhs = QCC_PR_BuildRef(refbuf, REF_FIELD, QCC_RefToDef(lhs, true), QCC_RefToDef(field, true), type_vector, false, 0);

		
		lhs = QCC_PR_ParseField(refbuf, lhs);
		lhs = QCC_PR_ParseRefArrayPointer (refbuf, lhs, false, false);
	}
	return lhs;
}

//this is more complex than it needs to be, in order to ensure that anon unions/structs can be handled.
struct QCC_typeparam_s *QCC_PR_FindStructMember(QCC_type_t *t, const char *membername, unsigned int *out_ofs, unsigned int *out_bitofs)
{
	unsigned int nofs, nbitofs;
	int i;
	struct QCC_typeparam_s *r = NULL, *n;
	for (i = 0; i < t->num_parms; i++)
	{
		if ((!t->params[i].paramname || !*t->params[i].paramname) && (t->params[i].type->type == ev_struct || t->params[i].type->type == ev_union))
		{	//anonymous structs/unions can nest
			n = QCC_PR_FindStructMember(t->params[i].type, membername, &nofs, &nbitofs);
			if (n)
			{
				if (r)
					break;
				r = n;
				*out_ofs = t->params[i].ofs + nofs;
				*out_bitofs = t->params[i].bitofs + nbitofs;
			}
		}
		else if (flag_caseinsensitive?!stricmp (t->params[i].paramname, membername):!STRCMP(t->params[i].paramname, membername))
		{
			if (r)
				break;
			r = t->params+i;
			*out_ofs = r->ofs;
			*out_bitofs = r->bitofs;
		}
	}
	if (i < t->num_parms)
	{
		QCC_PR_ParseError(0, "multiple members found matching %s.%s", t->name, membername);
		return NULL;
	}
	if (!r && t->parentclass)	//chain through the parent struct
		return QCC_PR_FindStructMember(t->parentclass, membername, out_ofs, out_bitofs);

	return r;
}

/*checks for:
<d>[X]
<d>[X].foo
<d>.foo
within types which are a contiguous block, expanding to an array index.

Also calls QCC_PR_ParseField, which does fields too.
*/
QCC_ref_t *QCC_PR_ParseRefArrayPointer (QCC_ref_t *retbuf, QCC_ref_t *r, pbool allowarrayassign, pbool makearraypointers)
{
	QCC_type_t *t, *p;
	QCC_sref_t idx;
	QCC_sref_t tmp;
	pbool allowarray, arraytype;
	unsigned int arraysize;
	unsigned int rewindpoint = numstatements;
	pbool dereference = false;
	const QCC_eval_t *eval;
	unsigned int bitofs = 0;
	pbool dorecurse = false;
	QCC_ref_t addr;

	idx = nullsref;

	t = r->cast;
	if (r->type == REF_ARRAYHEAD || r->type == REF_POINTERARRAY)
	{
		if (r->type == REF_POINTERARRAY)
			dereference = true;

		if (t->type != ev_pointer)
			QCC_PR_ParseWarning(ERR_INTERNAL, "QCC_PR_ParseRefArrayPointer: array reference not a cast to pointer\n");
		t = t->aux_type;
		arraysize = r->arraysize;
	}
	else
	{
		if (r->type == REF_POINTER && r->cast->type != ev_pointer && !r->postinc && (r->cast->type==ev_union || r->cast->type==ev_struct) && QCC_PR_PeekToken("."))
		{	// (*ptr).blah === ptr->blah
			//try to undo the *ptr so we can do it automatically from the ->
//			r = QCC_PR_GenerateAddressOf(&addr, r);
//			return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);

			addr = *r;
			r = &addr;

			r->cast = t = QCC_PR_PointerType(t);
//			dereference = true;
			idx = r->index;
			r->index = nullsref;
			r->type = REF_GLOBAL;

		}
		arraysize = 0;
	}

	while(1)
	{
		allowarray = false;
		arraytype = (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname);	//FIXME
		if (arraytype)
			allowarray = true;
		if (idx.cast)
			allowarray = arraysize>0 ||
						(t->type == ev_vector) ||
						(t->type == ev_field && t->aux_type->type == ev_vector) ||
						(arraytype && !arraysize);
		else if (!idx.cast)
		{
			allowarray = arraysize>0 ||
						(t->type == ev_pointer) ||	//we can dereference pointers
						(t->type == ev_string) ||	//strings are effectively pointers
						(t->type == ev_vector) ||	//vectors are mini arrays
						(t->type == ev_field && t->aux_type->type == ev_vector) ||	//as are field vectors
						(arraytype && !arraysize) ||
						(!arraysize&&t->accessors);	//custom accessors
		}

		if (allowarray && QCC_PR_CheckToken("["))
		{
			p = t;
			tmp = QCC_PR_Expression (TOP_PRIORITY, 0);
			QCC_PR_Expect("]");

			if (!arraysize && t->accessors)
			{
				struct accessor_s *acc;
				for (acc = t->accessors; acc; acc = acc->next)
					if (!*acc->fieldname)
						break;
				if(acc)
				{
					r = QCC_PR_BuildAccessorRef(retbuf, QCC_RefToDef(r, true), tmp, acc, r->readonly);
					return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
				}
			}

			/*if its a pointer that got dereferenced, follow the type*/
			if (!idx.cast && t->type == ev_pointer && !arraysize)
				t = t->aux_type;
			else if (idx.cast && (arraytype && !arraysize))
			{
				arraysize = t->params[0].arraysize;
				bitofs += t->params[0].bitofs;
				t = t->params[0].type;
			}

			if (!idx.cast && p->type == ev_pointer && !arraysize)
			{
				/*no bounds checks on pointer dereferences*/
				if (dereference)
				{	//resolve it now
					r = QCC_PR_BuildRef(retbuf, REF_POINTER, QCC_RefToDef(r, true), idx, t, r->readonly, bitofs);
					idx = nullsref;
				}
				dereference = true;
			}
			else if (!idx.cast && p->type == ev_string && !arraysize)
			{
				if (flag_qccx)
				{
					QCC_sref_t base = QCC_RefToDef(r, true);
					if (tmp.cast && tmp.cast->type == ev_float)
					{
						QCC_PR_ParseWarning(WARN_DENORMAL, "string offsetting emulation: denormals are unsafe");
						idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], base, QCC_SupplyConversion(tmp, ev_float, true), NULL);
					}
					else
						idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], base, QCC_SupplyConversion(tmp, ev_integer, true), NULL);
					return QCC_PR_BuildRef(retbuf, REF_GLOBAL, idx, nullsref, type_string, true, 0);
				}
				else
				{
					/*automatic runtime bounds checks on strings, I'm not going to check this too much...*/
					r = QCC_PR_BuildRef(retbuf, REF_STRING, QCC_RefToDef(r, true), tmp, (tmp.cast->type != ev_float)?type_integer:type_float, r->readonly, 0);
					return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
				}
			}
			else if ((!idx.cast && p->type == ev_vector && !arraysize) || (idx.cast && t->type == ev_vector && !arraysize))
			{
				/*array notation on vector*/
vectorarrayindex:
				if ((eval=QCC_SRef_EvalConst(tmp)))
				{
					unsigned long i = QCC_Eval_Int(eval, tmp.cast);
					if (i >= 3u)
						QCC_PR_ParseErrorPrintSRef(0, r->base, "(vector) array index out of bounds");
				}
				else if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && flag_boundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, 3, NULL), nullsref, false);
				}
				t = type_float;
			}
			else if ((!idx.cast && p->type == ev_field && r->cast->aux_type->type == ev_vector && !arraysize) || (idx.cast && t->type == ev_field && t->aux_type->type && !arraysize))
			{
				/*array notation on vector field*/
fieldarrayindex:
				if ((eval=QCC_SRef_EvalConst(tmp)))
				{
					unsigned long i = QCC_Eval_Int(eval, tmp.cast);
					if (i >= 3u)
						QCC_PR_ParseErrorPrintSRef(0, r->base, "(.vector) array index out of bounds");
				}
				else if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && flag_boundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, 3, NULL), nullsref, false);
				}
				t = type_floatfield;
			}
			else if (!arraysize)
			{
				QCC_PR_ParseErrorPrintSRef(0, r->base, "array index on non-array");
			}
			else if ((eval=QCC_SRef_EvalConst(tmp)))
			{
				unsigned i = QCC_Eval_Int(eval, tmp.cast);
				if (i >= (unsigned)arraysize)
				{
					QCC_PR_ParseWarning(WARN_BOUNDS, "(constant) array index out of bounds (0 <= %i < %i)", i, arraysize);
					QCC_PR_ParsePrintSRef(WARN_BOUNDS, r->base);
				}
			}
			else
			{
				if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && flag_boundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, arraysize, NULL), nullsref, false);
				}
			}
			arraysize = 0;

			if (t->bits)
			{
				//convert it to ints if that makes sense
				if (idx.cast)
					idx = QCC_SupplyConversion(idx, ev_integer, true);
				tmp = QCC_SupplyConversion(tmp, ev_integer, true);
				tmp = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], QCC_SupplyConversion(tmp, ev_integer, true), QCC_MakeIntConst(t->bits/t->align), NULL);
			}
			else if (t->size != 1) /*don't multiply by type size if the instruction/emulation will do that instead*/
			{
				//convert it to ints if that makes sense
				if (QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]) && ((idx.cast && idx.cast->type == ev_integer) || tmp.cast->type == ev_integer))
				{
					if (idx.cast)
						idx = QCC_SupplyConversion(idx, ev_integer, true);
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
				}

				if (tmp.cast->type == ev_float)
					tmp = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], QCC_SupplyConversion(tmp, ev_float, true), QCC_MakeFloatConst(t->size), NULL);
				else
					tmp = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], QCC_SupplyConversion(tmp, ev_integer, true), QCC_MakeIntConst(t->size), NULL);

				//FIXME: we really need some sort of x*stride+offset ref type. it would allow adding offsets more efficiently.
			}

			//legacy opcodes needs to stay using floats even if an int was specified. avoid int immediates.
//			if (!QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]))
//			{
//				if (idx.cast)
//					idx = QCC_SupplyConversion(idx, ev_float, true);
//				tmp = QCC_SupplyConversion(tmp, ev_float, true);
//			}

			/*calc the new index*/
			if (idx.cast && idx.cast->type == ev_float && tmp.cast->type == ev_float)
				idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], QCC_SupplyConversion(idx, ev_float, true), QCC_SupplyConversion(tmp, ev_float, true), NULL);
			else if (idx.cast)
				idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], QCC_SupplyConversion(idx, ev_integer, true), QCC_SupplyConversion(tmp, ev_integer, true), NULL);
			else
				idx = tmp;
		}
		else if (arraysize && (QCC_PR_CheckToken(".") /*|| QCC_PR_CheckToken("->")*/))
		{
			//the only field of an array type is the 'length' property.
			//if we calculated offsets etc, discard those statements.
			//FIXME: if this is an array of pointer-to-struct then '->' will dereference and we just misparsed.
			numstatements = rewindpoint;
			QCC_PR_Expect("length");
			QCC_FreeTemp(r->base);
			QCC_FreeTemp(r->index);
			QCC_FreeTemp(idx);
			return QCC_PR_BuildRef(retbuf, REF_GLOBAL, QCC_MakeIntConst(arraysize), nullsref, type_integer, true, 0);
		}
		else if (arraytype && (QCC_PR_CheckToken(".")/* || QCC_PR_CheckToken("->")*/))
		{
			//the only field of an array type is the 'length' property.
			//if we calculated offsets etc, discard those statements.
			numstatements = rewindpoint;
			QCC_PR_Expect("length");
			QCC_FreeTemp(r->base);
			QCC_FreeTemp(r->index);
			QCC_FreeTemp(idx);
			return QCC_PR_BuildRef(retbuf, REF_GLOBAL, QCC_MakeIntConst(t->params[0].arraysize), nullsref, type_integer, true, 0);
		}
		else if (t->type == ev_string && !idx.cast && (QCC_PR_CheckToken(".")/* || QCC_PR_CheckToken("->")*/))
		{
			if (QCC_PR_CheckName("length"))
			{
				const char *val = QCC_SRef_EvalStringConst(r->base);
				if (val)
				{	//the only field of an array type is the 'length' property.
					//if we calculated offsets etc, discard those statements.
					numstatements = rewindpoint;
					QCC_FreeTemp(r->base);
					QCC_FreeTemp(r->index);
					QCC_FreeTemp(idx);
					return QCC_PR_BuildRef(retbuf, REF_GLOBAL, QCC_MakeIntConst(strlen(val)), nullsref, type_integer, true, 0);
				}
				QCC_PR_ParseError(0, "not a constant");
			}
			else
				QCC_PR_ParseError(0, "unsupported string method %s", pr_token);
		}
		else if (t->type == ev_vector && !arraysize && !t->accessors && QCC_PR_CheckToken("."))
		{
			char *swizzle = QCC_PR_ParseName();
			//single-channel swizzles just result in a float. nice and easy. assignable, too.
			if ((!strcmp(swizzle, "x") || !strcmp(swizzle, "r")) && t->size >= 1)
			{
				tmp = QCC_MakeIntConst(0);
				goto vectorarrayindex;
			}
			else if ((!strcmp(swizzle, "y") || !strcmp(swizzle, "g")) && t->size >= 2)
			{
				tmp = QCC_MakeIntConst(1);
				goto vectorarrayindex;
			}
			else if ((!strcmp(swizzle, "z") || !strcmp(swizzle, "b")) && t->size >= 3)
			{
				tmp = QCC_MakeIntConst(2);
				goto vectorarrayindex;
			}
			else if ((!strcmp(swizzle, "w")  || !strcmp(swizzle, "a")) && t->size >= 4)
			{
				tmp = QCC_MakeIntConst(3);
				goto vectorarrayindex;
			}
			else
				QCC_PR_ParseError(0, "unsupported vector swizzle '.%s'", swizzle);
		}
		else if ((t->type == ev_field && t->aux_type->type == ev_vector) && !arraysize && !t->accessors && QCC_PR_CheckToken("."))
		{
			char *swizzle = QCC_PR_ParseName();
			//single-channel swizzles just result in a float. nice and easy. assignable, too.
			if ((!strcmp(swizzle, "x") || !strcmp(swizzle, "r")) && t->size >= 1)
			{
				tmp = QCC_MakeIntConst(0);
				goto fieldarrayindex;
			}
			else if ((!strcmp(swizzle, "y") || !strcmp(swizzle, "g")) && t->size >= 2)
			{
				tmp = QCC_MakeIntConst(1);
				goto fieldarrayindex;
			}
			else if ((!strcmp(swizzle, "z") || !strcmp(swizzle, "b")) && t->size >= 3)
			{
				tmp = QCC_MakeIntConst(2);
				goto fieldarrayindex;
			}
			else if ((!strcmp(swizzle, "w")  || !strcmp(swizzle, "a")) && t->size >= 4)
			{
				tmp = QCC_MakeIntConst(3);
				goto fieldarrayindex;
			}
			else
				QCC_PR_ParseError(0, "unsupported vector swizzle '.%s'", swizzle);
		}
		else if (((t->type == ev_pointer && !arraysize) || (t->type == ev_field && (t->aux_type->type == ev_struct || t->aux_type->type == ev_union)) || t->type == ev_struct || t->type == ev_union) && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
		{
			const char *tname, *mname;
			unsigned int ofs, mbitofs;
			pbool fld = t->type == ev_field;
			struct QCC_typeparam_s *p;
			if (t->type == ev_field)
				t = t->aux_type;
			else if (t->type == ev_pointer && !arraysize)
			{
				t = t->aux_type;
				if (dereference)
				{
					r = QCC_PR_BuildRef(retbuf, REF_POINTER, QCC_RefToDef(r, true), idx, t, false, bitofs);
					idx = nullsref;
				}
				dereference = true;
			}
			tname = t->name;

			if (t->type == ev_struct || t->type == ev_union)
			{
				if (!t->size)
					QCC_PR_ParseError(0, "%s was not defined yet", tname);
			}
			else
			{
				char typea[256];
				char typeb[256];
				TypeName(t, typea, sizeof(typea));
				if (idx.cast)
				{
					TypeName(idx.cast, typeb, sizeof(typeb));
					QCC_PR_ParseError(0, "indirection in %s [%s %s] - not a struct or union", typea, typeb, idx.sym->name);
				}
				else
					QCC_PR_ParseError(0, "indirection in %s - not a struct or union", typea);
			}

			mname = QCC_PR_ParseName();
			p = QCC_PR_FindStructMember(t, mname, &ofs, &mbitofs);
			if (!p)
			{	//check for static or non-virtual-function
				QCC_type_t *c;
				char membername[2048];
				for (c = t; c; c = c->parentclass)
				{
					QC_snprintfz(membername, sizeof(membername), "%s::%s", c->name, mname);
					tmp = QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false);
					if (tmp.cast)
					{	//static or non-virtual
						QCC_sref_t base = nullsref;	//for static

						r = QCC_PR_BuildRef(retbuf, base.cast?REF_THISCALL:REF_GLOBAL, tmp, base, tmp.cast, tmp.sym->constant, 0);
						return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
					}
				}

				QCC_PR_ParseWarning(ERR_BADMEMBER, "%s is not a member of %s", mname, t->name);
				if (t->filen)
					QCC_PR_Note(ERR_BADMEMBER, t->filen, t->line, "%s is defined here", t->name);
				QCC_PR_ParseError(ERR_BADMEMBER, NULL);
			}

			if (idx.cast && p->type->align != t->align)	//switching alignment requires dealing with what the previous was defined as. should onlly switch to tighter alignment
				idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], QCC_SupplyConversion(idx, ev_integer, true), QCC_MakeIntConst(t->align/p->type->align), NULL);

			if(p->type->align == 8)
			{
				ofs*=(32/p->type->align);
				ofs+=(mbitofs>>3);
				mbitofs &= 7;
			}
			else if(p->type->align == 16)
			{
				ofs*=(32/p->type->align);
				ofs+=(mbitofs>>4);
				mbitofs &= 15;
			}
			else
				ofs *= (32/p->type->align);

			if (!ofs && idx.cast)
				;
			else if (QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]))
			{
				tmp = QCC_MakeIntConst(ofs);
				if (idx.cast)
					idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], QCC_SupplyConversion(idx, ev_integer, true), tmp, NULL);
				else
					idx = tmp;
			}
			else
			{
				tmp = QCC_MakeFloatConst(ofs*(32/p->type->align));
				if (idx.cast)
					idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], QCC_SupplyConversion(idx, ev_float, true), QCC_SupplyConversion(tmp, ev_float, true), NULL);
				else
					idx = tmp;
			}
			arraysize = p->arraysize;
			t = p->type;

			bitofs += mbitofs;

			if (fld)
				t = QCC_PR_FieldType(t);
		}
		else
			break;
		dorecurse = true;

		if (t->type == ev_pointer && !arraysize)
		{
			QCC_sref_t base;
			if (r->type == REF_ARRAYHEAD)
			{
				r->type = REF_ARRAY;
				r->cast = type_void;
				base = QCC_RefToDef(r, true);
				r->type = REF_ARRAYHEAD;
			}
			else
				base = QCC_RefToDef(r, true);

			if (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname)	//FIXME: this destroys type info required for sizeof, and breaks struct lvalues.
			{
				arraysize = t->params[0].arraysize;
				t = t->params[0].type;
			}

//QCC_PR_StatementAnnotation("ParseRefArrayPointer:%i %s->",__LINE__, reftypename[r->type]);
			if (dereference)
				r = QCC_PR_BuildRef(retbuf, REF_POINTER, base, idx, t, r->readonly, bitofs);
			else
				r = QCC_PR_BuildRef(retbuf, REF_ARRAY, base, idx, t, r->readonly, bitofs);
//QCC_PR_StatementAnnotation("->%s", reftypename[r->type]);
			return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
		}

	}

	if (idx.cast)
	{
		QCC_sref_t base;
		if (r->type == REF_ARRAYHEAD)
		{
			r->type = REF_ARRAY;
			r->cast = type_void;
			base = QCC_RefToDef(r, true);
			r->type = REF_ARRAYHEAD;
		}
		else
			base = QCC_RefToDef(r, true);

		if (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname && makearraypointers)	//FIXME: this destroys type info required for sizeof, and breaks struct lvalues.
		{
			arraysize = t->params[0].arraysize;
			t = t->params[0].type;
		}

		//okay, not a pointer, we'll have to read it in somehow
//QCC_PR_StatementAnnotation("ParseRefArrayPointer:%i %s->",__LINE__, reftypename[r->type]);
		if (arraysize && makearraypointers)
		{
			if (dereference)
				r = QCC_PR_BuildRef(retbuf, REF_POINTERARRAY, base, idx, QCC_PR_PointerType(t), r->readonly, bitofs);
			else
				r = QCC_PR_BuildRef(retbuf, REF_ARRAYHEAD, base, idx, QCC_PR_PointerType(t), r->readonly, bitofs);
			r->arraysize = arraysize;
		}
		else
		{
			if (dereference)
				r = QCC_PR_BuildRef(retbuf, REF_POINTER, base, idx, t, r->readonly, bitofs);
			else
				r = QCC_PR_BuildRef(retbuf, REF_ARRAY, base, idx, t, r->readonly, bitofs);
			r->arraysize = arraysize;
		}
//QCC_PR_StatementAnnotation("->%s", reftypename[r->type]);

		//parse recursively
		if (dorecurse)
			r = QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
	}
	else
		r->bitofs += bitofs;

	r = QCC_PR_ParseField(retbuf, r);
	return r;
}

QCC_sref_t QCC_PR_GenerateVector(QCC_sref_t x, QCC_sref_t y, QCC_sref_t z)
{
	QCC_sref_t		d;
	const QCC_eval_t *c[3];

	if (x.cast->type != ev_float && x.cast->type != ev_integer)
		x = QCC_EvaluateCast(x, type_float, true);
	if (y.cast->type != ev_float && y.cast->type != ev_integer)
		y = QCC_EvaluateCast(y, type_float, true);
	if (z.cast->type != ev_float && z.cast->type != ev_integer)
		z = QCC_EvaluateCast(z, type_float, true);

	if ((x.cast->type != ev_float && x.cast->type != ev_integer) ||
		(y.cast->type != ev_float && y.cast->type != ev_integer) ||
		(z.cast->type != ev_float && z.cast->type != ev_integer))
	{
		QCC_PR_ParseError(ERR_TYPEMISMATCH, "Argument not a single numeric value in vector constructor");
		return QCC_MakeVectorConst(0, 0, 0);
	}

	//return a constant if we can.
	c[0] = QCC_SRef_EvalConst(x);
	c[1] = QCC_SRef_EvalConst(y);
	c[2] = QCC_SRef_EvalConst(z);
	if (c[0] && c[1] && c[2])
	{
		d = QCC_MakeVectorConst(
			QCC_Eval_Float(c[0], x.cast),
			QCC_Eval_Float(c[1], y.cast),
			QCC_Eval_Float(c[2], z.cast));
		QCC_FreeTemp(x);
		QCC_FreeTemp(y);
		QCC_FreeTemp(z);
		return d;
	}
	if (QCC_SRef_IsNull(y) && QCC_SRef_IsNull(z))
	{
		QCC_FreeTemp(y);
		QCC_FreeTemp(z);
		return QCC_PR_StatementFlags(pr_opcodes + OP_MUL_VF, QCC_MakeVectorConst(1, 0, 0), QCC_SupplyConversion(x, ev_float, true), NULL, 0);
	}
	if (QCC_SRef_IsNull(x) && QCC_SRef_IsNull(z))
	{
		QCC_FreeTemp(x);
		QCC_FreeTemp(z);
		return QCC_PR_StatementFlags(pr_opcodes + OP_MUL_VF, QCC_MakeVectorConst(0, 1, 0), QCC_SupplyConversion(y, ev_float, true), NULL, 0);
	}
	if (QCC_SRef_IsNull(x) && QCC_SRef_IsNull(y))
	{
		QCC_FreeTemp(x);
		QCC_FreeTemp(y);
		return QCC_PR_StatementFlags(pr_opcodes + OP_MUL_VF, QCC_MakeVectorConst(0, 0, 1), QCC_SupplyConversion(z, ev_float, true), NULL, 0);
	}

	//pack the variables into a vector
	d = QCC_GetTemp(type_vector);
	d.cast = type_float;
	if (x.cast->type == ev_float)
		QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, x, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	else
		QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, x, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	d.ofs++;
	if (y.cast->type == ev_float)
		QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, y, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	else
		QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, y, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	d.ofs++;
	if (z.cast->type == ev_float)
		QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, z, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	else
		QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, z, d, NULL, STFL_PRESERVEB|STFL_DISCARDRESULT);
	d.ofs++;
	d.ofs -= 3;
	d.cast = type_vector;

	return d;
}

/*
============
PR_ParseValue

Returns the global ofs for the current token
============
*/
QCC_ref_t	*QCC_PR_ParseRefValue (QCC_ref_t *refbuf, QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers)
{
	QCC_sref_t		d;
	QCC_type_t		*t;
	char		*name;
	QCC_ref_t *r;

	char membername[2048];

// if the token is an immediate, allocate a constant for it
	if (pr_token_type == tt_immediate)
	{
		d = QCC_PR_ParseImmediate ();
//		d.sym->referenced = true;
//		return QCC_DefToRef(refbuf, d);
		name = NULL;
	}
	else if (QCC_PR_CheckToken("["))
	{
		//originally used for reacc - taking the form of [5 84 2]
		//we redefine it to include statements - [a+b, c, 3+(d*2)]
		//and to not need the 2nd/3rd parts if you're lazy - [5] or [5,6] - FIXME: should we accept 1-d vector? or is that too risky with arrays and weird error messages?
		//note the addition of commas.
		//if we're parsing reacc code, we will still accept [(a+b) c (3+(d*2))], as QCC_PR_Term contains the () handling. We do also allow optional commas.
		QCC_sref_t x,y,z;
		if (flag_acc)
		{
			x = QCC_PR_Term(EXPR_DISALLOW_COMMA);
			QCC_PR_CheckToken(",");
			y = QCC_PR_Term(EXPR_DISALLOW_COMMA);
			QCC_PR_CheckToken(",");
			z = QCC_PR_Term(EXPR_DISALLOW_COMMA);
		}
		else
		{
			x = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);

			if (QCC_PR_CheckToken(","))
				y = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			else
				y = QCC_MakeFloatConst(0);

			if (QCC_PR_CheckToken(","))
				z = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			else
				z = QCC_MakeFloatConst(0);
		}

		QCC_PR_Expect("]");

		d = QCC_PR_GenerateVector(x,y,z);
//		d.sym->referenced = true;
//		return QCC_DefToRef(refbuf, d);
		name = NULL;
	}
	else
	{
		if (QCC_PR_CheckToken("::"))
		{
			assumeclass = NULL;
			expandmemberfields = false;	//::classname is always usable for eg: the find builtin.
		}
		name = QCC_PR_ParseName ();

		//fixme: namespaces should be relative
		if (QCC_PR_CheckToken("::"))
		{
			struct accessor_s *a;
			QCC_type_t *p;
			char membername[1024];
			expandmemberfields = false;	//this::classname should also be available to the find builtin, etc. this won't affect self.classname::member nor classname::staticfunc

			if (assumeclass && !strcmp(name, "super"))
				t = assumeclass->parentclass;
			else if (assumeclass && !strcmp(name, "this"))
				t = assumeclass;
			else
				t = QCC_TypeForName(name);
			if (!t)
			{
				d = QCC_PR_GetSRef (pr_assumetermtype, name, pr_assumetermscope, false, 0, pr_assumetermflags);
				if (d.cast)
				{
					QCC_FreeTemp(d);
					t = d.cast;
				}
				else
					QCC_PR_ParseError (ERR_UNKNOWNVALUE, "\"%s\" is not a type", name);
			}

			name = QCC_PR_ParseName ();
			//walk up the parents if needed, to find one that has that field
			for(d = nullsref, p = t; ; )
			{
				if (!d.cast && p->accessors)
				{
					for (a = t->accessors; a; a = a->next)
					{
						if (!strcmp(a->fieldname, name))
						{
							d = a->staticval;
							QCC_ForceUnFreeDef(d.sym);
							break;
						}
					}
					if (d.cast)
						break;
				}
				if (!d.cast && t->type == ev_entity)
				{
					//use static functions in preference to virtual functions. kinda needed so you can use super::func...
					QC_snprintfz(membername, sizeof(membername), "%s::%s", p->name, name);
					d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
					if (!d.cast)
					{
						QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, p->name, name);
						d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
					}

					p = p->parentclass;
					if (p)
						continue;
				}
				if (!d.cast && t->type == ev_struct)
				{
					//use static functions in preference to virtual functions. kinda needed so you can use super::func...
					QC_snprintfz(membername, sizeof(membername), "%s::%s", p->name, name);
					d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
					p = p->parentclass;
				}
				break;
			}
			if (!d.cast)
			{
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s::%s\"", t->name, name);
			}
		}
		else
		{
			d = nullsref;
			// 'testvar' becomes 'this::testvar'
			if (assumeclass && assumeclass->parentclass)
			{	//try getting a member.
				QCC_type_t *type;
				if (assumeclass->type == ev_struct)
				{
					unsigned int ofs, bitofs;
					struct QCC_typeparam_s *p = QCC_PR_FindStructMember(assumeclass, name, &ofs, &bitofs);
					if (p)
					{
						QCC_sref_t		ths;
						ths = QCC_PR_GetSRef(QCC_PR_PointerType(pr_classtype), "this", pr_scope, false, 0, false);
						if (ths.cast)
						{
							ths.cast = QCC_PR_PointerType(p->type);
							
							if (d.sym->arraysize)
							{
								//FIXME: this should result in a pointer type, and not this->member[0]
							}
							return QCC_PR_ParseRefArrayPointer(refbuf, QCC_PR_BuildRef(refbuf, REF_POINTER, ths, QCC_MakeIntConst(ofs), p->type, false, bitofs), allowarrayassign, makearraypointers);
						}
					}
				}
				else
				{
					for(type = assumeclass; type && !d.cast; type = type->parentclass)
					{

						//look for virtual things
						QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, type->name, name);
						d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
					}
					for(type = assumeclass; type && !d.cast; type = type->parentclass)
					{
						//look for non-virtual things (functions: after virtual stuff, because this will find the actual function def too)
						QC_snprintfz(membername, sizeof(membername), "%s::%s", type->name, name);
						d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
					}
				}
			}
			if (!d.cast)
			{
				// look through the defs
				d = QCC_PR_GetSRef (NULL, name, pr_scope, false, 0, false);
			}
		}

		if (!d.cast)
		{
			if (!strcmp(name, "nil"))
				d = QCC_MakeIntConst(0);
			else if (	(!strcmp(name, "randomv"))	||
					(!strcmp(name, "alloca"))	||
					(!strcmp(name, "entnum"))	||
					(!strcmp(name, "autocvar"))	||
					(!strcmp(name, "used_model"))	||
					(!strcmp(name, "used_sound"))	||
					(!strcmp(name, "va_arg"))	||
					(!strcmp(name, "..."))		||	//for compat. otherwise wtf?
					(!strcmp(name, "_"))		)	//intrinsics, any old function with no args will do.
			{
				d = QCC_PR_GetSRef (type_function, name, NULL, true, 0, false);
	//			d->initialized = 0;
			}
			else if (	(!strcmp(name, "random" ))	)	//intrinsics, any old function with no args will do. returning a float just in case people declare things in the wrong order
			{
				d = QCC_PR_GetSRef (type_floatfunction, name, NULL, true, 0, false);
	//			d.sym->initialized = 0;
			}
			else if (keyword_class && !strcmp(name, "this"))
			{
				if (!pr_classtype)
					QCC_PR_ParseError(ERR_NOTANAME, "Cannot use 'this' outside of an OO function\n");
				d = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
				d.cast = pr_classtype;
			}
			else if (keyword_class && !strcmp(name, "super"))
			{
				if (!assumeclass)
					QCC_PR_ParseError(ERR_NOTANAME, "Cannot use 'super' outside of an OO function\n");
				if (!assumeclass->parentclass)
					QCC_PR_ParseError(ERR_NOTANAME, "class %s has no super\n", pr_classtype->name);
				d = QCC_PR_GetSRef(NULL, "self", NULL, true, 0, false);
				d.cast = assumeclass->parentclass;
			}
			else if (pr_assumetermtype)
			{
				d = QCC_PR_GetSRef (pr_assumetermtype, name, pr_assumetermscope, true, 0, pr_assumetermflags);
				if (!d.cast)
					QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", name);
			}
			else
			{
				d = QCC_PR_GetSRef (type_variant, name, pr_scope, true, 0, false);
				if (!expandmemberfields && assumeclass)
				{
					if (!d.cast)
						QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown field \"%s\" in class \"%s\"", name, assumeclass->name);
					else if (!assumeclass->parentclass && assumeclass != type_entity)
					{
						QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Class \"%s\" is not defined, cannot access member \"%s\"", assumeclass->name, name);
						if (!autoprototype && !autoprototyped)
							QCC_PR_Note(ERR_UNKNOWNVALUE, s_filen, pr_source_line, "Consider using #pragma autoproto");
					}
					else
					{
						QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Unknown field \"%s\" in class \"%s\"", name, assumeclass->name);
					}
				}
				else
				{
					if (!d.cast)
						QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", name);
					else
					{
						QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Unknown value \"%s\".", name);
					}
				}
			}
		}
	}

	d.sym->referenced = true;

	//class code uses self as though it was 'this'. its a hack, but this is QC.
	if (assumeclass && pr_classtype && name && !strcmp(name, "self"))
	{
		//use 'this' instead.
		QCC_sref_t t = QCC_PR_GetSRef(NULL, "this", pr_scope, false, 0, false);
		if (!t.cast)	//shouldn't happen.
		{
			t.sym = QCC_PR_DummyDef(pr_classtype, "this", pr_scope, 0, d.sym, 0, true, GDF_CONST|GDF_STRIP|GDF_ALIAS);
			t.cast = pr_classtype;
			t.ofs = 0;
		}
		else
			QCC_FreeTemp(d);
		d = t;
		QCC_PR_ParseWarning (WARN_SELFNOTTHIS, "'self' used inside OO function, use 'this'.");
	}

	if (!d.cast)
		QCC_PR_ParseError (ERR_INTERNAL, "d.cast == NULL");

	//within class functions, refering to fields should use them directly.
	if (pr_classtype && expandmemberfields && d.cast->type == ev_field)
	{
		QCC_sref_t t;
		if (assumeclass)
		{
			t = QCC_PR_GetSRef(NULL, "this", pr_scope, false, 0, false);
			if (!t.cast)
			{
				t.sym = QCC_PR_DummyDef(pr_classtype, "this", pr_scope, 0, QCC_PR_GetDef(NULL, "self", NULL, true, 0, false), 0, true, GDF_CONST|GDF_STRIP|GDF_ALIAS);	//create a union into it
				t.cast = pr_classtype;
				t.ofs = 0;
			}
		}
		else
			t = QCC_PR_GetSRef(NULL, "self", NULL, true, 0, false);

		if (d.sym->arraysize)
		{
			QCC_DefToRef(refbuf, d);
			refbuf->type = REF_ARRAYHEAD;
			refbuf->arraysize = d.sym->arraysize;
			refbuf->cast = QCC_PointerTypeTo(refbuf->cast);
			d = QCC_RefToDef(QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers), true);
		}
		else
			d = QCC_PR_ParseArrayPointer(d, allowarrayassign, makearraypointers); //opportunistic vecmember[0] handling

		//then return a reference to this.field
		QCC_PR_BuildRef(refbuf, REF_FIELD, t, d, d.cast->aux_type, false, 0);

		//(a.(foo[4]))[2] should still function, and may be common with field vectors
		return QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers); //opportunistic vecmember[0] handling
	}

	t = d.cast;
	if (d.sym->arraysize)
	{
		QCC_DefToRef(refbuf, d);
		refbuf->type = REF_ARRAYHEAD;
		refbuf->arraysize = d.sym->arraysize;
		refbuf->cast = QCC_PointerTypeTo(refbuf->cast);
		r = QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers);
		if (r->type == REF_ARRAYHEAD && flag_brokenarrays)
		{
			r->type = REF_GLOBAL;
			r->cast = r->cast->aux_type;
		}
		/*if (r->type == REF_ARRAYHEAD)
		{
			r->type = REF_GLOBAL;
			return QCC_PR_GenerateAddressOf(refbuf, r);
		}*/
		return r;
	}
	else if (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname)	//FIXME: this destroys type info required for sizeof, and breaks struct lvalues.
	{	//convert it to a proper array, instead of leaving as a simple global.
		QCC_DefToRef(refbuf, d);
		refbuf->type = REF_ARRAYHEAD;
		refbuf->arraysize = t->params[0].arraysize;
		refbuf->cast = QCC_PointerTypeTo(t->params[0].type);
		return QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers);
	}

	else if (d.sym->autoderef)
	{
		t = t->aux_type;
		if (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname)	//FIXME: this destroys type info required for sizeof, and breaks struct lvalues.
		{
			r = QCC_PR_BuildRef(refbuf, REF_POINTERARRAY, d, nullsref, QCC_PointerTypeTo(t->params[0].type), false, 0);	//it points to an array, which resolves to a pointer to its base type...
			r->arraysize = t->params[0].arraysize;	//make sure it has the proper size etc in case someone typedefs it.
		}
		else
			r = QCC_PR_BuildRef(refbuf, REF_POINTER, d, nullsref, t, false, 0); // just dereference it before parsing any array/fields/etc stuff.
	}
	else
		r = QCC_DefToRef(refbuf, d);
	return QCC_PR_ParseRefArrayPointer(refbuf, r, allowarrayassign, makearraypointers);
}

//true if its NOT 0
QCC_sref_t QCC_PR_GenerateLogicalTruth(QCC_sref_t e, const char *errormessage)
{
	etype_t	t;
	QCC_type_t *type = e.cast;
	while(type->type == ev_accessor || type->type == ev_boolean)
		type = type->parentclass;
	t = type->type;
	if (t == ev_float)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_F], e, QCC_MakeFloatConst(0), NULL);
	else if (t == ev_string)
		return QCC_PR_Statement (&pr_opcodes[flag_brokenifstring?OP_NE_E:OP_NE_S], e, QCC_MakeIntConst(0), NULL);
	else if (t == ev_entity)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_E], e, QCC_MakeIntConst(0), NULL);
	else if (t == ev_vector)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_V], e, QCC_MakeVectorConst(0,0,0), NULL);
	else if (t == ev_function)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_FNC], e, QCC_MakeIntConst(0), NULL);
	else if (t == ev_integer || t == ev_uint)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_I], e, QCC_MakeIntConst(0), NULL);	//functions are integer values too.
	else if (t == ev_pointer)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_I], e, QCC_MakeIntConst(0), NULL);	//Pointers are too.
	else if (t == ev_double)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_D], e, QCC_MakeDoubleConst(0), NULL);
	else if (t == ev_int64)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_I64], e, QCC_MakeInt64Const(0), NULL);
	else if (t == ev_uint64)
		return QCC_PR_Statement (&pr_opcodes[OP_NE_U64], e, QCC_MakeUInt64Const(0), NULL);
	else if (t == ev_void && flag_laxcasts)
	{
		QCC_PR_ParseWarning(WARN_LAXCAST, errormessage, "void");
		return QCC_PR_Statement (&pr_opcodes[OP_NE_F], e, QCC_MakeFloatConst(0), NULL);
	}
	else
	{
		char etype[256];
		TypeName(e.cast, etype, sizeof(etype));

		QCC_PR_ParseError (ERR_BADNOTTYPE, errormessage, etype);
		return nullsref;
	}
}

QCC_sref_t QCC_PR_GenerateLogicalNot(QCC_sref_t e, const char *errormessage)
{
	etype_t	t;
	QCC_type_t *type = e.cast;
	while(type->type == ev_accessor || type->type == ev_boolean)
		type = type->parentclass;
	t = type->type;
	if (t == ev_float)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_F], e, nullsref, NULL);
	else if (t == ev_string)
		return QCC_PR_Statement (&pr_opcodes[flag_brokenifstring?OP_NOT_ENT:OP_NOT_S], e, nullsref, NULL);
	else if (t == ev_entity)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_ENT], e, nullsref, NULL);
	else if (t == ev_vector)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_V], e, nullsref, NULL);
	else if (t == ev_function)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_FNC], e, nullsref, NULL);
	else if (t == ev_integer || t == ev_uint)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_I], e, nullsref, NULL);	//functions are integer values too.
	else if (t == ev_pointer)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_I], e, nullsref, NULL);	//Pointers are too.
	else if (t == ev_double)
		return QCC_PR_Statement (&pr_opcodes[OP_EQ_D], e, QCC_MakeDoubleConst(0), NULL);
	else if (t == ev_int64)
		return QCC_PR_Statement (&pr_opcodes[OP_EQ_I64], e, QCC_MakeInt64Const(0), NULL);
	else if (t == ev_uint64)
		return QCC_PR_Statement (&pr_opcodes[OP_EQ_U64], e, QCC_MakeUInt64Const(0), NULL);
	else if (t == ev_void && flag_laxcasts)
	{
		QCC_PR_ParseWarning(WARN_LAXCAST, errormessage, "void");
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_F], e, nullsref, NULL);
	}
	else
	{
		char etype[256];
		TypeName(e.cast, etype, sizeof(etype));

		QCC_PR_ParseError (ERR_BADNOTTYPE, errormessage, etype);
		return nullsref;
	}
}
QCC_sref_t QCC_PR_GenerateBitwiseNot(QCC_sref_t e, const char *errormessage)
{
	etype_t	t;
	QCC_type_t *type = e.cast;
	while(type->type == ev_accessor || type->type == ev_boolean)
		type = type->parentclass;
	t = type->type;
	if (t == ev_float)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_F], e, nullsref, NULL);
	else if (t == ev_string)
		return QCC_PR_Statement (&pr_opcodes[flag_brokenifstring?OP_NOT_ENT:OP_NOT_S], e, nullsref, NULL);
//	else if (t == ev_entity)
//		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_ENT], e, nullsref, NULL);
	else if (t == ev_vector)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_V], e, nullsref, NULL);
//	else if (t == ev_function)
//		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_FNC], e, nullsref, NULL);
	else if (t == ev_integer || t == ev_uint)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_I], e, nullsref, NULL);	//functions are integer values too.
	else if (t == ev_pointer)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_I], e, nullsref, NULL);	//Pointers are too.
	else if (t == ev_double)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_D], e, QCC_MakeDoubleConst(0), NULL);
	else if (t == ev_int64)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_I64], e, QCC_MakeInt64Const(0), NULL);
	else if (t == ev_uint64)
		return QCC_PR_Statement (&pr_opcodes[OP_BITNOT_U64], e, QCC_MakeUInt64Const(0), NULL);
	else if (t == ev_void && flag_laxcasts)
	{
		QCC_PR_ParseWarning(WARN_LAXCAST, errormessage, "void");
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_F], e, nullsref, NULL);
	}
	else
	{
		char etype[256];
		TypeName(e.cast, etype, sizeof(etype));

		QCC_PR_ParseError (ERR_BADNOTTYPE, errormessage, etype);
		return nullsref;
	}
}

//doesn't consider parents
static QCC_sref_t QCC_TryEvaluateCast(QCC_sref_t src, QCC_type_t *cast, pbool implicit)
{
	QCC_type_t *tmp;
	int totype;

	for (tmp = cast; tmp->type == ev_accessor || tmp->type == ev_bitfld; tmp = tmp->parentclass)
		;
	totype = tmp->type;

	while (src.cast->type == ev_boolean)
		src.cast = src.cast->parentclass;

	/*you may cast from a type to itself*/
	if (!typecmp(src.cast, cast))
	{
		//no-op
	}
	/*you may cast from const 0 to any other basic type for free (from either int or float for simplicity). things get messy when its a struct*/
	else if (QCC_SRef_IsNull(src) && totype != ev_struct && totype != ev_union)
	{
		QCC_FreeTemp(src);
		if (cast->size == 3)// || cast->type == ev_variant)
			src = QCC_MakeVectorConst(0,0,0);
		else
			src = QCC_MakeIntConst(0);
		src.cast = cast;
	}
	else if (totype == ev_boolean)
	{
		src = QCC_PR_GenerateLogicalTruth(src, "cast to boolean");
		//src will often be a float(eg:NQ_F) with a bint totype. make sure we evaluate it fully.
		return QCC_TryEvaluateCast(src, tmp->parentclass, implicit);
	}
	else if (totype == ev_float && src.cast->type == ev_uint)	//uint->float
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_UF], src, nullsref, NULL);
	else if (totype == ev_uint && src.cast->type == ev_float)	//float->uint
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FU], src, nullsref, NULL);
	/*cast from int->float will convert*/
	else if (totype == ev_float && (src.cast->type == ev_integer || (src.cast->type == ev_entity && !implicit)))
	{
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_ITOF], src, nullsref, NULL);
		src.cast = cast;
	}
	/*cast from float->int will convert*/
	else if ((totype == ev_integer || (totype == ev_entity && !implicit)) && src.cast->type == ev_float)
	{
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FTOI], src, nullsref, NULL);
		src.cast = cast;
	}
	else if ((totype == ev_integer || totype == ev_uint) && (src.cast->type == ev_integer || src.cast->type == ev_uint))
		src.cast = cast;	//fine, just treat it as-is
	else if ((totype == ev_int64 || totype == ev_uint64) && (src.cast->type == ev_int64 || src.cast->type == ev_uint64))
		src.cast = cast;	//fine, just treat it as-is

	else if (totype == ev_float && src.cast->type == ev_double)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_DF], src, nullsref, NULL);
	else if (totype == ev_double && src.cast->type == ev_float)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FD], src, nullsref, NULL);

	else if (totype == ev_float && src.cast->type == ev_int64)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64F], src, nullsref, NULL);
	else if (totype == ev_int64 && src.cast->type == ev_float)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FI64], src, nullsref, NULL);
	else if (totype == ev_float && src.cast->type == ev_uint64)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_U64F], src, nullsref, NULL);
	else if (totype == ev_uint64 && src.cast->type == ev_float)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FU64], src, nullsref, NULL);

	else if (totype == ev_uint64 && src.cast->type == ev_double)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_DU64], src, nullsref, NULL);
	else if (totype == ev_double && src.cast->type == ev_uint64)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_U64D], src, nullsref, NULL);
	else if (totype == ev_int64 && src.cast->type == ev_double)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_DI64], src, nullsref, NULL);
	else if (totype == ev_double && src.cast->type == ev_int64)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64D], src, nullsref, NULL);

	else if (totype == ev_uint && src.cast->type == ev_double)
	{
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_DU64], src, nullsref, NULL);
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64I], src, nullsref, NULL);
		src.cast = cast;
	}
	else if ((totype == ev_integer||totype == ev_uint) && src.cast->type == ev_double)
	{
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_DI64], src, nullsref, NULL);
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64I], src, nullsref, NULL);
		src.cast = cast;
	}
	else if (totype == ev_double && (src.cast->type == ev_integer || src.cast->type == ev_uint))
	{
		if (src.cast->type == ev_uint)
			src = QCC_PR_Statement (&pr_opcodes[OP_CONV_UI64], src, nullsref, NULL);
		else
			src = QCC_PR_Statement (&pr_opcodes[OP_CONV_II64], src, nullsref, NULL);
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64D], src, nullsref, NULL);
		src.cast = cast;
	}


	else if ((totype == ev_int64||totype == ev_uint64) && src.cast->type == ev_uint)
	{	//zero extends. we could emulate this but that means endian issues.
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_UI64], src, nullsref, NULL);
		src.cast = cast;
	}
	else if ((totype == ev_int64||totype == ev_uint64) && src.cast->type == ev_integer)
	{	//sign extends
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_II64], src, nullsref, NULL);
		src.cast = cast;
	}
	else if ((totype == ev_integer||totype == ev_uint) && (src.cast->type == ev_int64||src.cast->type == ev_uint64))
	{	//truncates. we could emulate this but that means endian issues.
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_I64I], src, nullsref, NULL);
		src.cast = cast;
	}
	else if (totype == ev_integer && (src.cast->type == ev_bitfld && src.cast->parentclass->type == ev_integer))
	{	//just read out the relevant bits, sign extending.
		src = QCC_PR_Statement (&pr_opcodes[OP_BITEXTEND_I], src, QCC_MakeUIntConst(src.cast->bits), NULL);
		src.cast = cast;
	}
	else if (totype == ev_uint && (src.cast->type == ev_bitfld && src.cast->parentclass->type == ev_uint))
	{	//just read out the relevant bits, sign extending.
		src = QCC_PR_Statement (&pr_opcodes[OP_BITAND_I], src, QCC_MakeUIntConst(~((1<<src.cast->bits)-1)), NULL);
		src.cast = cast;
	}

	else if (totype == ev_vector && src.cast->type == ev_float)
	{
		if (implicit)
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
		}
		src = QCC_PR_Statement (&pr_opcodes[OP_MUL_FV], src, QCC_MakeVectorConst(1,1,1), NULL);
	}
	else if (totype == ev_vector && src.cast->type == ev_integer)
	{
		if (implicit)
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
		}
		src = QCC_PR_Statement (&pr_opcodes[OP_MUL_IV], src, QCC_MakeVectorConst(1,1,1), NULL);
	}
	else if (totype == ev_entity && src.cast->type == ev_entity)
	{
		if (implicit)
		{	//this is safe if the source inherits from the dest type
			//but we should warn if the other way around
			QCC_type_t *t = src.cast;
			while(t)
			{
				if (!typecmp_lax(t, cast))
					break;
				t = t->parentclass;
			}
			if (!t)
			{
				char typea[256];
				char typeb[256];
				TypeName(src.cast, typea, sizeof(typea));
				TypeName(cast, typeb, sizeof(typeb));
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
			}
		}
		src.cast = cast;
	}
	else if (flag_undefwordsize && ((totype == ev_pointer && src.cast->type == ev_string) || (totype == ev_pointer && src.cast->type == ev_string)))
	{	//pointer<->string is blocked when pointers are not bytes. if words are 8 bytes then everything is screwed, or if pointers cannot store byte offsets...
		QCC_PR_ParseWarning(ERR_BADEXTENSION, "pointer<->string casts were disabled.");
		src.cast = cast;
	}
	//string and char* are the same type. don't care if its signed or unsigned.
	else if (((totype == ev_pointer && tmp->aux_type->type == ev_bitfld && tmp->aux_type->bits == 8) && src.cast->type == ev_string) ||
			 (totype == ev_string && (src.cast->type == ev_pointer && src.cast->aux_type->type == ev_bitfld && src.cast->aux_type->bits == 8)))
		src.cast = cast;
	//can cast any pointer type to void*
	else if (totype == ev_pointer && tmp->aux_type->type == ev_void && (src.cast->type == ev_pointer || src.cast->type == ev_function || src.cast->type == ev_string))
		src.cast = cast;
	//can cast void* to any pointer type.
	else if (src.cast->type == ev_pointer && src.cast->aux_type->type == ev_void && (totype == ev_pointer || totype == ev_function || totype == ev_string))
		src.cast = cast;
	else if (totype == ev_pointer && src.cast->type == ev_pointer)
	{
		if (implicit)
		{	//this is safe if the source inherits from the dest type
			//but we should warn if the other way around
			QCC_type_t *t = src.cast->aux_type;
			while(t)
			{
				if (!typecmp_lax(t, cast->aux_type))
					break;
				t = t->parentclass;
			}
			if (!t)
			{
				char typea[256];
				char typeb[256];
				TypeName(src.cast, typea, sizeof(typea));
				TypeName(cast, typeb, sizeof(typeb));
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
			}
		}
		src.cast = cast;
	}
	/*variants can be cast from/to anything without warning, even implicitly. FIXME: size issues*/
	else if (totype == ev_variant || src.cast->type == ev_variant
		|| (totype == ev_field && totype == src.cast->type && (tmp->aux_type->type == ev_variant || src.cast->aux_type->type == ev_variant)))
	{
		src.cast = cast;

		if (implicit && typecmp_lax(src.cast, cast))
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
		}
	}
	/*these casts are acceptable but probably an error (so warn when implicit)*/
	else if (
		/*you may explicitly cast between pointers and ints (strings count as pointers - WARNING: some strings may not be expressable as pointers)*/
		((totype == ev_pointer || totype == ev_string || totype == ev_integer || totype == ev_uint || totype == ev_function) && (src.cast->type == ev_pointer || src.cast->type == ev_string || src.cast->type == ev_integer || src.cast->type == ev_uint || src.cast->type == ev_function))
		//ents->ints || ints->ents. WARNING: the integer value of ent types is engine specific.
		|| (totype == ev_entity && src.cast->type == ev_integer)
		|| (totype == ev_entity && src.cast->type == ev_float && flag_qccx)
		|| (totype == ev_integer && src.cast->type == ev_entity)
		|| (totype == ev_float && src.cast->type == ev_entity && flag_qccx)
		|| (totype == ev_string && src.cast->type == ev_float && flag_qccx)
		|| (totype == ev_float && src.cast->type == ev_string && flag_qccx)
		)
	{
		//direct cast
		if (implicit && typecmp_lax(src.cast, cast))
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
		}
		src.cast = cast;
	}
	else if (!implicit && !typecmp_lax(src.cast, cast))
		src.cast = cast;
	else if (!implicit && cast->type == ev_void)
		src.cast = type_void;	//anything can be cast to void, but only do it explicitly.
	else	//failed
		return nullsref;
	return src;
}

QCC_sref_t QCC_EvaluateCast(QCC_sref_t src, QCC_type_t *cast, pbool implicit)
{
	QCC_sref_t r;
	if (	(cast->type == ev_accessor && cast->parentclass == src.cast)
		||	(src.cast->type == ev_accessor && src.cast->parentclass == cast))
	{
		if (implicit)
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(0, "Implicit cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
		}
		src.cast = cast;
		return src;
	}

//casting from an accessor uses the base type of that accessor (this allows us to properly read void* accessors)
	for(;;)
	{
		r = QCC_TryEvaluateCast(src, cast, implicit);
		if (r.cast)
			return r;	//success

		if (src.cast->type == ev_accessor || src.cast->type == ev_bitfld)
			src.cast = src.cast->parentclass;
		else if (flag_laxcasts)
		{
			if (implicit)
			{
				char typea[256];
				char typeb[256];
				TypeName(src.cast, typea, sizeof(typea));
				TypeName(cast, typeb, sizeof(typeb));
				QCC_PR_ParseWarning(0, "Implicit lax cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type,typeb,col_none);
			}
			r = src;
			r.cast = cast;		//decompilers suck
			return r;
		}
		else
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseError(0, "Cannot cast from %s%s%s to %s%s%s", col_type,typea,col_none, col_type, typeb,col_none);
		}
	}
}

/*
============
PR_Term
============
*/
static QCC_ref_t *QCC_PR_RefTerm (QCC_ref_t *retbuf, unsigned int exprflags)
{
	QCC_ref_t	*r;
	QCC_sref_t	e, e2;
	if (pr_token_type == tt_punct)	//a little extra speed...
	{
		int preinc;
		if (QCC_PR_CheckToken("++"))
			preinc = 1;
		else if (QCC_PR_CheckToken("--"))
			preinc = -1;
		else
			preinc = 0;

		if (preinc)
		{
			QCC_ref_t tmp;
			qcc_usefulstatement=true;
			r = QCC_PR_RefTerm (&tmp, 0);	//parse the term, as if we were going to return it
			if (r->readonly)
				QCC_PR_ParseError(ERR_BADPLUSPLUSOPERATOR, "%s operator on read-only value", (preinc>0)?"++":"--");
			e = QCC_RefToDef(r, false);	//read it as needed
			if (e.sym->constant)
			{
				QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to constant %s", QCC_GetSRefName(e));
				QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, e);
			}
//			if (e.sym->temp && r->type == REF_GLOBAL)
//				QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Hey! That's a temp! ++ operators cannot work on temps!");
			switch (e.cast->type)
			{
			case ev_integer:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], e, QCC_MakeIntConst(preinc), NULL);
				break;
			case ev_uint:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_U], e, QCC_MakeIntConst(preinc), NULL);
				break;
			case ev_pointer:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], e, QCC_MakeIntConst(preinc * e.cast->aux_type->size), NULL);
				break;
			case ev_float:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], e, QCC_MakeFloatConst(preinc), NULL);
				break;
			case ev_double:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_D], e, QCC_MakeDoubleConst(preinc), NULL);
				break;
			case ev_int64:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_I64], e, QCC_MakeInt64Const(preinc), NULL);
				break;
			case ev_uint64:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_U64], e, QCC_MakeInt64Const(preinc), NULL);
				break;
			default:
				QCC_PR_ParseError(ERR_BADPLUSPLUSOPERATOR, "++ operator on unsupported type");
				break;
			}
			//return the 'result' of the store. its read-only now.
			return QCC_DefToRef(retbuf, QCC_StoreSRefToRef(r, e, true, false));
		}
		if (QCC_PR_CheckToken ("!"))
		{
			e = QCC_PR_Expression (NOT_PRIORITY, EXPR_DISALLOW_COMMA|EXPR_WARN_ABOVE_1);
			e = QCC_PR_GenerateLogicalNot(e, "Type mismatch: !%s");
			return QCC_DefToRef(retbuf, e);
		}

		if (QCC_PR_CheckToken ("~"))
		{
			e = QCC_PR_Expression (NOT_PRIORITY, EXPR_DISALLOW_COMMA|EXPR_WARN_ABOVE_1);
			e = QCC_PR_GenerateBitwiseNot(e, "Type mismatch: ~%s");
			return QCC_DefToRef(retbuf, e);
		}
		if (QCC_PR_CheckToken ("&"))
		{
			r = QCC_PR_RefExpression (retbuf, UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
			if (flag_qccx && r->cast->type == ev_float)
			{	//&%342 casts it to a (pre-dereferenced) pointer.
				r = QCC_PR_BuildRef(retbuf, REF_POINTER, QCC_RefToDef(r, true), nullsref, type_float, false, 0);
			}
			else if (flag_qccx && (r->cast->type == ev_string || r->cast->type == ev_field || r->cast->type == ev_entity || r->cast->type == ev_function))
			{	//&string casts it to a float. does not dereference it
				r->cast = type_float;
			}
			else
			{
				if (r->base.sym->temp && r->type != REF_POINTER)
					QCC_PR_ParseWarning(ERR_INTERNAL, "Address-of temp on line %s:%i", s_filen, pr_source_line);
				else
					r = QCC_PR_GenerateAddressOf(retbuf, r);
			}
			return r;
		}
		if (QCC_PR_CheckToken ("*"))
		{
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
			if (flag_qccx && (e.cast->type == ev_float || e.cast->type == ev_integer))
			{	//just an evil cast. note that qccx assumes offsets rather than indexes, so these are often quite large and typically refer to some index into the world entity.
				return QCC_PR_BuildRef(retbuf, REF_GLOBAL, e, nullsref, type_entity, false, 0);
			}
			else if (e.cast->type == ev_pointer)	//FIXME: arrays
				return QCC_PR_BuildRef(retbuf, REF_POINTER, e, nullsref, e.cast->aux_type, false, 0);
			else if (e.cast->type == ev_string)	//FIXME: arrays
				return QCC_PR_BuildRef(retbuf, REF_STRING, e, nullsref, type_integer, false, 0);
			else if (e.cast->type == ev_function)
			{	// (*funcptr)(args); is one way to call a function pointer in C, but the * is actually irrelevant.
				if (flag_qcfuncs)
					QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCH, e, "attempt to dereference function type.");
				return QCC_PR_BuildRef(retbuf, REF_GLOBAL, e, nullsref, e.cast, false, 0);
			}
			else if (e.cast->accessors)
			{
				struct accessor_s *acc;
				for (acc = e.cast->accessors; acc; acc = acc->next)
					if (!strcmp(acc->fieldname, ""))
						return QCC_PR_BuildAccessorRef(retbuf, e, nullsref, acc, e.sym->constant);
			}
			QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCH, e, "Unable to dereference non-pointer type.");
		}
		if (flag_qccx && QCC_PR_CheckToken ("@"))
		{	//@foo is equivelent to (string)*(int*)&foo
			r = QCC_PR_RefExpression (retbuf, UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
			if (r->cast->type == ev_float || r->cast->type == ev_integer)
			{
				r->cast = type_string;
				return r;
			}
			QCC_PR_ParseErrorPrintSRef (ERR_BADEXTENSION, QCC_RefToDef(r, true), "'@' operator only functions on floats and ints. go figure.");
		}
		if (QCC_PR_CheckToken ("-"))
		{
			QCC_type_t *type;
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);

			type = e.cast;
			while (type->type == ev_boolean)
				type = type->parentclass;
			switch(type->type)
			{
			case ev_float:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_F], QCC_MakeFloatConst(0), e, NULL);
				break;
			case ev_double:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_D], QCC_MakeDoubleConst(0), e, NULL);
				break;
			case ev_vector:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_V], QCC_MakeVectorConst(0, 0, 0), e, NULL);
				break;
			case ev_integer:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_I], QCC_MakeIntConst(0), e, NULL);
				break;
			case ev_uint:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_U], QCC_MakeIntConst(0), e, NULL);
				break;
			case ev_int64:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_I64], QCC_MakeInt64Const(0), e, NULL);
				break;
			case ev_uint64:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_U64], QCC_MakeInt64Const(0), e, NULL);
				break;
			default:
				QCC_PR_ParseError (ERR_BADNOTTYPE, "type mismatch for -");
				e2 = nullsref;
				break;
			}
			return QCC_DefToRef(retbuf, e2);
		}
		if (QCC_PR_CheckToken ("+"))
		{
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);

			switch(e.cast->type)
			{
			case ev_float:
			case ev_double:
			case ev_vector:
			case ev_integer:
			case ev_uint:
			case ev_int64:
			case ev_uint64:
				e2 = e;
				break;
			default:
				QCC_PR_ParseError (ERR_BADNOTTYPE, "type mismatch for +");
				e2 = nullsref;
				break;
			}
			return QCC_DefToRef(retbuf, e2);
		}
		if (QCC_PR_CheckToken ("("))
		{
			QCC_type_t *newtype;
			newtype = QCC_PR_ParseType(false, true, false);
			if (newtype)
			{
				/*if (QCC_PR_Check ("["))
				{
					QCC_PR_Expect ("]");
					QCC_PR_Expect(")");
					QCC_PR_Expect("{");

					QCC_PR_Expect ("}");
					return array;
				}*/
				QCC_PR_Expect (")");
				if (QCC_PR_PeekToken("{"))
				{
					if (newtype->type == ev_vector)
					{
						QCC_sref_t x,y,z;
						QCC_PR_Expect("{");
						x = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
						QCC_PR_Expect(",");
						y = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
						QCC_PR_Expect(",");
						z = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
						QCC_PR_Expect ("}");
						return QCC_DefToRef(retbuf, QCC_PR_GenerateVector(x,y,z));
					}
					else
					{
						e = QCC_PR_ParseInitializerTemp(newtype);
						if (newtype->type == ev_function && QCC_PR_PeekToken("("))
							e.sym->allowinline = true;
					}
				}
				else
				{
					//not a single term, so we can cast the result of function calls. just make sure its not too high a priority
					//and yeah, okay, use defs not refs. whatever.
					e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
					e = QCC_EvaluateCast(e, newtype, false);
				}
				return QCC_DefToRef(retbuf, e);
			}
			else
			{
				pbool oldcond = conditional;
				conditional = conditional?2:0;
				r =	QCC_PR_RefExpression(retbuf, TOP_PRIORITY, 0);
				QCC_PR_Expect (")");
				conditional = oldcond;

//				QCC_PR_ParseArrayPointer(r, true, true);

				r = QCC_PR_ParseRefArrayPointer(retbuf, r, true, true);
			}
			return r;
		}
	}
	if (pr_token_type == tt_name)	//a little extra speed...
	{
		if (QCC_PR_CheckKeyword(true, "sizeof"))	//'returns size_t', which we interpret as uintptr_t aka uint32_t for us.
		{
			QCC_type_t *t;
			pbool bracket = QCC_PR_CheckToken("(");
			t = QCC_PR_ParseType(false, true, false);
			if (t)
			{
				if (flag_undefwordsize)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "sizeof was disabled, use array.length instead.");
				if (bracket)
					QCC_PR_Expect(")");
				if (t->bits&31)
					return QCC_DefToRef(retbuf, QCC_MakeUIntConst(t->bits/8));
				else	//word-aligned types should use PIW to get byte counts, but its not necessarily meaningful, so be efficient instead.
					return QCC_DefToRef(retbuf, QCC_MakeUIntConst(t->size*VMWORDSIZE));
					//return QCC_DefToRef(retbuf, QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], QCC_MakeIntConst(0), QCC_MakeIntConst(t->size), NULL));
			}
			else
			{
				int sz;
				int oldstcount = numstatements;
				QCC_ref_t refbuf, *r;
				r = QCC_PR_RefExpression(&refbuf, TOP_PRIORITY, 0);	//formulas are accepted weirdly enough, its not just terms.
				if (r->type == REF_GLOBAL && r->base.sym->type == type_string && !strcmp(r->base.sym->name, "IMMEDIATE"))
					sz = strlen(&strings[QCC_SRef_EvalConst(r->base)->string]) + 1;	//sizeof("hello") includes the null, and is bytes not codepoints
				else
				{
					if ((r->cast->align&7))
						QCC_PR_ParseWarning(ERR_BADEXTENSION, "sizeof on bit-field is invalid");
					else if (flag_undefwordsize)
						QCC_PR_ParseWarning(ERR_BADEXTENSION, "sizeof was disabled.");

					t = r->cast;
					if (r->type == REF_ARRAYHEAD || r->type == REF_POINTERARRAY)
						t = t->aux_type;
//					else if (r->cast->type == ev_pointer)
//						QCC_PR_ParseWarning(WARN_IGNOREDKEYWORD, "sizeof on pointer? (%i)", r->type);
					if (t->bits)
						sz = t->bits/8;
					else
						sz = VMWORDSIZE*t->size;	//4 bytes per word. we don't support char/short (our string type is logically char*)
					if (r->type == REF_ARRAYHEAD || r->type == REF_POINTERARRAY)
						sz *= r->arraysize;
				}
				QCC_FreeTemp(r->base);
				if (r->index.cast)
					QCC_FreeTemp(r->index);
				//the term should not have side effects, or generate any actual statements.
				numstatements = oldstcount;
				if (bracket)
					QCC_PR_Expect(")");
				return QCC_DefToRef(retbuf, QCC_MakeUIntConst(sz));
			}
		}
		/*if (QCC_PR_CheckKeyword(keyword_new, "new"))
		{	//note: C++ requires struct-based classes rather than entity ones.
			const char *cname = QCC_PR_ParseName();
			QCC_type_t *rettype = QCC_TypeForName(cname);
			QCC_sref_t	result, func;
			if (!rettype || rettype->type != ev_entity)
				QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "new %s() unsupported argument type for intrinsic", cname);
			e = QCC_PR_GetSRef(NULL, "spawn", NULL, 0, 0, 0);
			if (!e.cast)
				QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "new %s() spawn builtin not defined", cname);
			result = QCC_PR_GenerateFunctionCallRef(nullsref, e, NULL,0);
			result.cast = rettype;
			if (QCC_PR_CheckToken("("))
			{	//arglist is optional, apparently
				char genfunc[256];
				//do field assignments.
				while(QCC_PR_CheckToken(","))
				{
					QCC_sref_t f, p, v;
					f = QCC_PR_ParseValue(rettype, false, false, true);
					if (f.cast->type != ev_field)
						QCC_PR_ParseError(0, "Named field is not a field.");
					if (QCC_PR_CheckToken("="))							//allow : or = as a separator, but throw a warning for =
						QCC_PR_ParseWarning(0, "That = should be a :");	//rejecting = helps avoid qcc bugs. :P
					else
						QCC_PR_Expect(":");
					v = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);

					p = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], result, f, NULL, STFL_PRESERVEA);
					if (v.cast->size == 3)
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_V], v, p, NULL));
					else
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_F], v, p, NULL));
				}
				QCC_PR_Expect(")");

				QC_snprintfz(genfunc, sizeof(genfunc), "spawnfunc_%s", rettype->name);
				func = QCC_PR_GetSRef(type_function, genfunc, NULL, true, 0, GDF_CONST);
				func.sym->referenced = true;

				QCC_UnFreeTemp(result);
				QCC_FreeTemp(QCC_PR_GenerateFunctionCallRef(result, func, NULL, 0));
				result.cast = rettype;
			}
			return QCC_DefToRef(retbuf, result);
		}*/
		if (QCC_PR_CheckKeyword(true, "_length"))
		{	//for compat with gmqcc. use array.length in fte instead.
			pbool bracket = QCC_PR_CheckToken("(");
			/*QCC_type_t *t;
			t = QCC_PR_ParseType(false, true);
			if (t)
			{
				if (bracket)
					QCC_PR_Expect(")");
				return QCC_DefToRef(retbuf, QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], QCC_MakeIntConst(0), QCC_MakeIntConst(t->size), NULL));
			}
			else*/
			{
				int sz = 0;
				int oldstcount = numstatements;
				QCC_ref_t refbuf, *r;
				r = QCC_PR_RefExpression(&refbuf, TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				if (r->type == REF_ARRAYHEAD || r->type == REF_POINTERARRAY)
					sz = r->arraysize;
				else if (r->cast == type_string)
				{
					QCC_sref_t d = QCC_RefToDef(r, false);
					const QCC_eval_t *c = QCC_SRef_EvalConst(d);
					if (c)
						sz = strlen(&strings[c->string]);	//_length("hello") does NOT include the null (like strlen), but is bytes not codepoints
					else
						QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "_length(string) requires an initialised constant");
				}
				else if (r->cast == type_vector)
					sz = 3;	//might as well. considering that vectors can be indexed as an array.
				else
					QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "_length() unsupported argument type for intrinsic");
				QCC_FreeTemp(r->base);
				if (r->index.cast)
					QCC_FreeTemp(r->index);
				//the term should not have side effects, or generate any actual statements.
				numstatements = oldstcount;
				if (bracket)
					QCC_PR_Expect(")");
				return QCC_DefToRef(retbuf, QCC_MakeIntConst(sz));
			}
		}
	}
	return QCC_PR_ParseRefValue (retbuf, pr_classtype, !(exprflags&EXPR_DISALLOW_ARRAYASSIGN), true, true);
}

static int QCC_NumericTypeRanking(etype_t t)
{
	switch(t)
	{
	case ev_double:	return 15;
	case ev_float:	return 10;
	//large gap, to try to de-prioritize the opcodes that mix float and int types.
	case ev_uint64:	return 6;
	case ev_int64:	return 3;
	case ev_uint:	return 1;
	case ev_integer:return 0;
	default: return -1; //unranked
	}
}
//type promotions:
//identity=0 > inheritance=1 > null=2 > double=3 > float=4 > uint64=5 > int64=6 > uint=7 > int=8 > short=9 > char=10 > variant=11 > vector=12 > failure
static int QCC_canConv(QCC_sref_t from, etype_t to)
{
	int frompri;
	int topri;

	while(from.cast->type == ev_accessor || from.cast->type == ev_boolean)	//no opcodes use accessors. convert to their real type.
		from.cast = from.cast->parentclass;

	if (from.cast->type == to)
		return 0;	//identity

	if (pr_classtype)
	{
		if (from.cast->type == ev_field)
		{
			if (from.cast->aux_type->type == to)
				return 1;
		}
	}

	if (QCC_SRef_IsNull(from))
		return 2;

	frompri = QCC_NumericTypeRanking(from.cast->type);
	topri = QCC_NumericTypeRanking(to);
	if (frompri >= 0 && topri >= 0)
	{	//3...12
		if (topri >= frompri)
			return 3+(topri-frompri);
		else
			return 150+(frompri-topri);
	}

	//somewhat high penalty, ensures the other side is correct
	if (to == ev_variant)
		return 9;
	if (from.cast->type == ev_variant)
		return 10;

	//triggers a warning.
	if (from.cast->type == ev_float && to == ev_vector)
		return 200;
	//triggers a warning. conversion works by using _x
	if (from.cast->type == ev_vector && to == ev_float)
		return 201;

	return -1;
}
/*
==============
QCC_PR_RefExpression
==============
*/

QCC_ref_t *QCC_PR_BuildAccessorRef(QCC_ref_t *retbuf, QCC_sref_t base, QCC_sref_t index, struct accessor_s *accessor, pbool readonly)
{
	retbuf->postinc = 0;
	retbuf->type = REF_ACCESSOR;
	retbuf->base = base;
	retbuf->index = index;
	retbuf->accessor = accessor;
	retbuf->cast = accessor->type;
	retbuf->readonly = readonly;
	retbuf->bitofs = 0;
	retbuf->arraysize = 0;
	return retbuf;
}
QCC_ref_t *QCC_PR_BuildRef(QCC_ref_t *retbuf, unsigned int reftype, QCC_sref_t base, QCC_sref_t index, QCC_type_t *cast, pbool	readonly, unsigned int bitofs)
{
	retbuf->postinc = 0;
	retbuf->type = reftype;
	retbuf->base = base;
	retbuf->index = index;
	retbuf->cast = cast?cast:base.cast;
	retbuf->readonly = readonly;
	retbuf->accessor = NULL;
	retbuf->bitofs = bitofs;
	retbuf->arraysize = base.sym->arraysize;
	return retbuf;
}
QCC_ref_t *QCC_DefToRef(QCC_ref_t *retbuf, QCC_sref_t def)
{
	return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				def, 
				nullsref,
				def.cast,
				!def.sym || !!def.sym->constant || def.sym->temp, 0);
}
/*
void QCC_StoreToOffset(int dest, int source, QCC_type_t *type)
{
	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
	case ev_float:
		QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F, source, dest, 0, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement(OP_STORE_V, source, dest, 0, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement(OP_STORE_ENT, source, dest, 0, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement(OP_STORE_S, source, dest, 0, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement(OP_STORE_FNC, source, dest, 0, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement(OP_STORE_FLD, source, dest, 0, false);
		break;
	case ev_integer:
		QCC_PR_SimpleStatement(OP_STORE_I, source, dest, 0, false);
		break;
	case ev_pointer:
		QCC_PR_SimpleStatement(OP_STORE_P, source, dest, 0, false);
		break;
	}
}*/
static void QCC_StoreToSRef(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type, pbool preservesource, pbool preservedest)
{
	unsigned int i;
	int flags = 0;
	if (preservesource)
		flags |= STFL_PRESERVEA;
	if (preservedest)
		flags |= STFL_PRESERVEB;
	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
	case ev_struct:
	case ev_union:
	case ev_enum:
		//don't bother trying to optimise any temps here, its not likely to happen anyway.
		if (QCC_SRef_IsNull(source))
		{
			for (i = 0; i+2 < type->size; i+=3, dest.ofs += 3)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_V], QCC_MakeVectorConst(0,0,0), dest, nullsref, false);
			if (QCC_OPCodeValid(&pr_opcodes[OP_STORE_I64]))
				for (; i+1 < type->size; i+=2, dest.ofs+=2)
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_I64], QCC_MakeInt64Const(0), dest, nullsref, false);
			for (; i < type->size; i++, dest.ofs++)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], QCC_MakeIntConst(0), dest, nullsref, false);
		}
		else
		{
			for (i = 0; i+2 < type->size; i+=3, dest.ofs += 3, source.ofs += 3)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_V], source, dest, nullsref, false);
			if (QCC_OPCodeValid(&pr_opcodes[OP_STORE_I64]))
				for (; i+1 < type->size; i+=2, dest.ofs+=2, source.ofs+=2)
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_I64], source, dest, nullsref, false);
			for (; i < type->size; i++, dest.ofs++, source.ofs++)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], source, dest, nullsref, false);
			source.ofs -= type->size;
		}
		dest.ofs -= type->size;
		if (!preservesource)
			QCC_FreeTemp(source);
		if (!preservedest)
			QCC_FreeTemp(dest);
		break;
	case ev_float:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], source, dest, NULL, flags));
		break;
	case ev_vector:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], source, dest, NULL, flags));
		break;
	case ev_entity:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], source, dest, NULL, flags));
		break;
	case ev_string:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_S], source, dest, NULL, flags));
		break;
	case ev_function:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], source, dest, NULL, flags));
		break;
	case ev_field:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], source, dest, NULL, flags));
		break;
	case ev_boolean:
		QCC_StoreToSRef(dest, source, type->parentclass, preservesource, preservedest);
		return;
	case ev_integer:
	case ev_uint:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], source, dest, NULL, flags));
		break;
	case ev_int64:
	case ev_uint64:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], source, dest, NULL, flags));
		break;
	case ev_pointer:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_P], source, dest, NULL, flags));
		break;
	}
}
//if readable, returns source (or dest if the store was folded), otherwise returns NULL
static QCC_sref_t QCC_CollapseStore(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type, pbool readable, pbool preservedest)
{
	if (opt_assignments && OpAssignsToC(statements[numstatements-1].op) && source.sym == statements[numstatements-1].c.sym && source.ofs == statements[numstatements-1].c.ofs)
	{
		if (source.sym->temp)
		{
			QCC_statement_t *statement = &statements[numstatements-1];
			statement->c.sym = dest.sym;
			statement->c.ofs = dest.ofs;
			dest.sym->referenced = true;

			optres_assignments++;
			QCC_FreeTemp(source);
			if (readable)
				return dest;
			if (!preservedest)
				QCC_FreeTemp(dest);
			return nullsref;
		}
	}
	QCC_StoreToSRef(dest, source, type, readable, preservedest);
	if (readable)
		return source;
	return nullsref;
}
static void QCC_StoreToPointer(QCC_sref_t dest, QCC_sref_t idx, QCC_sref_t source, QCC_type_t *type)
{
	pbool freedest = false;
	if (type->type == ev_variant)
		type = source.cast;

	if (type->type == ev_bitfld)
	{
		if (type->bits == 8)
			QCC_PR_Statement_SmallStore(OP_STOREP_I8, source, dest, idx);
		else if (type->bits == 16)
			QCC_PR_Statement_SmallStore(OP_STOREP_I16, source, dest, idx);
		else
			QCC_PR_ParseError(ERR_TYPEMISMATCH, "ptr-to-bitfld has unsupported bit depth");
		if (freedest)
			QCC_FreeTemp(dest);
		return;
	}

	while (type->type == ev_accessor)
		type = type->parentclass;

	if (idx.sym)
	{
		if (type->align != 32)
		{	//these all assume word indexes. but our index is actually elements... misalign. eww.
			idx = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], idx, QCC_MakeIntConst(32/type->align), NULL, STFL_PRESERVEA);
			dest = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], dest, idx, NULL, STFL_PRESERVEA);
			idx = nullsref;
		}
		else if (!QCC_OPCode_StorePOffset())
		{	//can't do an offset store yet... bake any index into the pointer.
			freedest = true;
			dest = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, idx, NULL, STFL_PRESERVEA);
			idx = nullsref;
		}
	}

	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
	case ev_struct:
	case ev_union:
		{
			unsigned int i = 0;
			if (QCC_OPCode_StorePOffset())
			{	//store-with-offset works.
				unsigned int bits = type->bits;
				if (!bits)
					bits = type->size*32;
				if (0 && bits > 96)	//if its going to be bit, we can save some adds by combining pointer and base... but its unsafe where tempbuffers are involved.
				{
					dest = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, idx, NULL, STFL_PRESERVEA);
					idx = nullsref;
				}
				if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_V]))
				for (; i+(32*3) <= bits; i+=32*3)
				{
					QCC_sref_t newidx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, QCC_MakeIntConst(i/32), NULL, STFL_PRESERVEA):QCC_MakeIntConst(i/32);
					if (QCC_SRef_IsNull(source))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], QCC_MakeVectorConst(0,0,0), dest, newidx, false);
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], source, dest, newidx, false);
						source.ofs += 3;
					}
					QCC_FreeTemp(newidx);
				}
				if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I64]))
				for (; i+(32*2) <= type->size; i+=32*2)
				{
					QCC_sref_t newidx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, QCC_MakeIntConst(i/32), NULL, STFL_PRESERVEA):QCC_MakeIntConst(i/32);
					if (QCC_SRef_IsNull(source))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I64], QCC_MakeVectorConst(0,0,0), dest, newidx, false);
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I64], source, dest, newidx, false);
						source.ofs += 2;
					}
					QCC_FreeTemp(newidx);
				}
				for (; i+32 <= bits; i+=32)
				{
					QCC_sref_t  newidx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, QCC_MakeIntConst(i/32), NULL, STFL_PRESERVEA):QCC_MakeIntConst(i/32);
					if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I]))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I], source, dest, newidx, false);
					else
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], source, dest, newidx, false);
					QCC_FreeTemp(newidx);
					source.ofs += 1;
				}
				if (i+16 <= bits)
				{
					QCC_sref_t  newidx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], idx, QCC_MakeIntConst(2), NULL, STFL_PRESERVEA), QCC_MakeIntConst(i/16), NULL, 0):QCC_MakeIntConst(i/16);
					QCC_PR_Statement_SmallStore(OP_STOREP_I16, source, dest, newidx);
					QCC_FreeTemp(newidx);
					source.ofs += 1;
					i+=16;
				}
				else if (i+8 <= bits)
				{
					QCC_sref_t  newidx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], QCC_MakeIntConst(i/8), idx, NULL, STFL_PRESERVEB):QCC_MakeIntConst(i/8);
					QCC_PR_Statement_SmallStore(OP_STOREP_I8, source, dest, newidx);
					QCC_FreeTemp(newidx);
					source.ofs += 1;
					i+=8;
				}
				if (i != bits)
					QCC_PR_ParseError(ERR_INTERNAL, "Underwrote");
			}
			else
			{	//no store-with-offset.
				for (i = 0; i+2 < type->size; i+=3)
				{
					QCC_sref_t newptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, QCC_MakeIntConst(i), NULL, STFL_PRESERVEA);
					if (QCC_SRef_IsNull(source))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], QCC_MakeVectorConst(0,0,0), newptr, idx, false);
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], source, newptr, idx, false);
						source.ofs += 3;
					}
					QCC_FreeTemp(newptr);
				}
				if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I64]))
				for (; i+1 < type->size; i+=2)
				{
					QCC_sref_t newptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, QCC_MakeIntConst(i), NULL, STFL_PRESERVEA);
					if (QCC_SRef_IsNull(source))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I64], QCC_MakeVectorConst(0,0,0), newptr, idx, false);
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I64], source, newptr, idx, false);
						source.ofs += 2;
					}
					QCC_FreeTemp(newptr);
				}
				for (; i < type->size; i++)
				{
					QCC_sref_t newptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, QCC_MakeIntConst(i), NULL, STFL_PRESERVEA);
					if (QCC_SRef_IsNull(source))
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], QCC_MakeIntConst(0), newptr, idx, false);
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], source, newptr, idx, false);
						source.ofs += 1;
					}
					QCC_FreeTemp(newptr);
				}
			}
		}
		break;
	case ev_float:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], source, dest, idx, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], source, dest, idx, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_ENT], source, dest, idx, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_S], source, dest, idx, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FNC], source, dest, idx, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, idx, false);
		break;
	case ev_integer:
	case ev_uint:
		if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I]))
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I], source, dest, idx, false);
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, idx, false);
		break;
	case ev_double:
	case ev_int64:
	case ev_uint64:
		if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I64]))
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I64], source, dest, idx, false);
		else
		{
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, idx, false);
			if (QCC_OPCode_StorePOffset())
				idx = idx.sym?QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, QCC_MakeIntConst(1), NULL, STFL_PRESERVEA):QCC_MakeIntConst(1);
			else
				dest = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest, QCC_MakeIntConst(1), NULL, STFL_PRESERVEA);
			source.ofs+=1;
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, idx, false);
		}
		break;
	case ev_pointer:
		if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_P]))
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_P], source, dest, idx, false);
		else if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I]))
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I], source, dest, idx, false);
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, idx, false);
		break;
	}
	if (freedest)
		QCC_FreeTemp(dest);
}
static void QCC_StoreToGPointer(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type)
{
	pbool freedest = false;
	if (type->type == ev_variant)
		type = source.cast;

	while (type->type == ev_accessor)
		type = type->parentclass;

	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
		QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, dest, "QCC_StoreToPointer doesn't know how to store to that type");
	case ev_struct:
	case ev_union:
	case ev_double:
	case ev_int64:
	case ev_uint64:
		{
			unsigned int i;
			for (i = 0; i+2 < type->size; i+=3)
			{
				QCC_sref_t newptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], dest, QCC_MakeIntConst(i), NULL, STFL_PRESERVEA);
				if (QCC_SRef_IsNull(source))
					QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_V], QCC_MakeVectorConst(0,0,0), newptr, nullsref, false);
				else
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_V], source, newptr, nullsref, false);
					source.ofs += 3;
				}
				QCC_FreeTemp(newptr);
			}
			for (; i < type->size; i++)
			{
				QCC_sref_t newptr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], dest, QCC_MakeIntConst(i), NULL, STFL_PRESERVEA);
				if (QCC_SRef_IsNull(source))
					QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_F], QCC_MakeIntConst(0), newptr, nullsref, false);
				else
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_F], source, newptr, nullsref, false);
					source.ofs += 1;
				}
				QCC_FreeTemp(newptr);
			}
		}
		break;
	case ev_float:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_F], source, dest, nullsref, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_V], source, dest, nullsref, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_ENT], source, dest, nullsref, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_S], source, dest, nullsref, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_FNC], source, dest, nullsref, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_FLD], source, dest, nullsref, false);
		break;
	case ev_integer:
	case ev_uint:
	case ev_pointer:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GSTOREP_I], source, dest, nullsref, false);
		break;
	}
	if (freedest)
		QCC_FreeTemp(dest);
}
static QCC_sref_t QCC_LoadFromPointer(QCC_sref_t source, QCC_sref_t idx, QCC_type_t *type)
{
	QCC_sref_t ret;
	int op;

	if (type->type == ev_bitfld)
	{
		if (type->bits == 8)
		{
			ret = QCC_PR_StatementFlags(&pr_opcodes[(type->parentclass->type == ev_integer)?OP_LOADP_I8:OP_LOADP_U8], source, idx, NULL, STFL_PRESERVEA|STFL_PRESERVEB);	//get pointer to precise def.
			ret.cast = type->parentclass;
			return ret;
		}
		else if (type->bits == 16)
		{
			ret = QCC_PR_StatementFlags(&pr_opcodes[(type->parentclass->type == ev_integer)?OP_LOADP_I16:OP_LOADP_U16], source, idx, NULL, STFL_PRESERVEA|STFL_PRESERVEB);	//get pointer to precise def.
			ret.cast = type->parentclass;
			return ret;
		}
		QCC_PR_ParseError(ERR_TYPEMISMATCH, "ptr-to-bitfld has unsupported bit depth");
	}
//	if (type->align != 32)
//		QCC_PR_ParseWarning(ERR_TYPEMISMATCH, "(%s) align is %i, not 32... %ibit", type->name, type->align, type->bits);

	while (type->type == ev_accessor)
		type = type->parentclass;

	switch(type->type)
	{
	default:
	case ev_struct:
	case ev_union:
		{
			unsigned int i;
			ret = QCC_GetTemp(type);
			for (i = 0; i+2 < type->size; i+=3)
			{
				QCC_sref_t ofs = QCC_MakeIntConst(i);
				if (idx.sym)
					ofs = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, ofs, NULL, STFL_PRESERVEA);

				QCC_PR_SimpleStatement(&pr_opcodes[OP_LOADP_V], source, ofs, ret, false);
				QCC_FreeTemp(ofs);
				ret.ofs += 3;
			}
			for (; i < type->size; i++)
			{
				QCC_sref_t ofs = QCC_MakeIntConst(i);
				if (idx.sym)
					ofs = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], idx, ofs, NULL, STFL_PRESERVEA);

				QCC_PR_SimpleStatement(&pr_opcodes[OP_LOADP_I], source, ofs, ret, false);
				QCC_FreeTemp(ofs);
				ret.ofs += 1;
			}
			ret.ofs -= type->size;
			return ret;
		}
		break;
//	case ev_double:		op = OP_LOADP_D;	break;
	case ev_int64:		op = OP_LOADP_I64;	break;
	case ev_uint64:		op = OP_LOADP_I64;	break;
	case ev_float:		op = OP_LOADP_F;	break;
	case ev_string:		op = OP_LOADP_S;	break;
	case ev_vector:		op = OP_LOADP_V;	break;
	case ev_entity:		op = OP_LOADP_ENT;	break;
	case ev_field:		op = OP_LOADP_FLD;	break;
	case ev_function:	op = OP_LOADP_FNC;	break;
	case ev_integer:	op = OP_LOADP_I;	break;
	case ev_uint:		op = OP_LOADP_I;	break;
	case ev_void:
	case ev_variant:
	case ev_bitfld:
		QCC_PR_ParseError(ERR_TYPEMISMATCH, "ptr-to-variants must be cast to a different type before being read");
		op = OP_LOADP_I;	break;
	case ev_pointer:
		op = OP_LOADP_P;
		if (!QCC_OPCodeValid(&pr_opcodes[op]))
			op = OP_LOADP_I;
		break;
	}
	ret = QCC_PR_StatementFlags(&pr_opcodes[op], source, idx, NULL, STFL_PRESERVEA|STFL_PRESERVEB);	//get pointer to precise def.
	ret.cast = type;
	return ret;
}
static void QCC_StoreToArray(QCC_sref_t base, QCC_sref_t index, QCC_sref_t source, QCC_type_t *t)
{
	/*if its assigned to, generate a functioncall to do the store*/
	QCC_sref_t funcretr;

	if (QCC_OPCodeValid(&pr_opcodes[OP_GLOBALADDRESS]))
	{
		QCC_sref_t addr;
		//ptr = &base[index];
		if (QCC_OPCode_StorePOffset())
			addr = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], base, nullsref, NULL);
		else
		{
			addr = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], base, index, NULL);
			index = nullsref;
		}
		//*ptr = source
		QCC_StoreToPointer(addr, index.cast?QCC_SupplyConversion(index, ev_integer, true):index, source, t);
		source.sym->referenced = true;
		QCC_FreeTemp(addr);
		QCC_FreeTemp(source);
	}
	else if (QCC_OPCodeValid(&pr_opcodes[OP_GSTOREP_F]))
	{
		QCC_sref_t addr;
		//ptr = &base[index];
		addr = QCC_DP_GlobalAddress(base, QCC_SupplyConversion(index, ev_integer, true), 0);
		//*ptr = source
		QCC_StoreToGPointer(addr, source, t);
		source.sym->referenced = true;
		QCC_FreeTemp(addr);
		QCC_FreeTemp(source);
	}
	else
	{
		const char *basename = QCC_GetSRefName(base);
		base.sym->referenced = true;
		QCC_FreeTemp(base);

		funcretr = QCC_PR_GetSRef(NULL, qcva("ArraySet*%s", basename), base.sym->scope, false, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
		if (!funcretr.cast)
		{
			QCC_type_t *arraysetfunc = qccHunkAlloc(sizeof(*arraysetfunc));
			struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*2);
			arraysetfunc->size = 1;
			arraysetfunc->type = ev_function;
			arraysetfunc->aux_type = type_void;
			arraysetfunc->params = fparms;
			arraysetfunc->num_parms = 2;
			arraysetfunc->name = "ArraySet";
			fparms[0].type = type_float;
			fparms[1].type = base.sym->type;
			funcretr = QCC_PR_GetSRef(arraysetfunc, qcva("ArraySet*%s", basename), base.sym->scope, true, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
			funcretr.sym->generatedfor = base.sym;
		}

		if (QCC_SRef_IsNull(source))
		{
			if (base.sym->type->type == ev_vector)
				source = QCC_MakeVectorConst(0,0,0);
			else
				source = QCC_MakeFloatConst(0);
		}
		else if (source.cast->type != t->type)
		{
			char typea[128], typeb[128];
			TypeName(source.cast, typea, sizeof(typea));
			TypeName(t, typeb, sizeof(typeb));
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "Type mismatch on indexed assignment of %s: %s %s, needed %s.", basename, typea, QCC_GetSRefName(source), typeb);
		}

		if (base.sym->type->type == ev_vector)
		{
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			index = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], index, QCC_MakeFloatConst(3), NULL);
		}

		qcc_usefulstatement=true;
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall2(nullsref, funcretr, QCC_SupplyConversion(index, ev_float, true), NULL, source, NULL));
	}
}
QCC_sref_t QCC_LoadFromArray(QCC_sref_t base, QCC_sref_t index, QCC_type_t *t, pbool preserve)
{
	int flags;
	int accel;

	//1: hexen2's FETCH_GBL opcodes take float indicies, but have a built in boundscheck that wrecks havoc with vectors and structs (and thus sucks when the types don't match)
	//2: fte's LOADA opcodes use ints without hidden boundcheck (engine will still ensure it reads an actual global), and vectors are not special(needs a separate *3 opcode) so they're struct friendly.
	//3: dp's GLOAD opcodes -- like fte's but without any offsetting so you need an extra addition.

	if (index.cast->type != ev_float || t->type != base.cast->type)
		accel = 2;
	else
		accel = 1;

	if (accel == 2 && !QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]))
		accel = QCC_OPCodeValid(&pr_opcodes[OP_GLOAD_F])&&!base.sym->temp?3:1;	//chose best for ints
	if (accel == 1 && (!base.sym->arraylengthprefix || !QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F])))
		accel = QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F])?2:0;	//chose best for floats

	if (accel == 3)
	{	//dp-style, somewhat annoying.
		if (index.cast->type == ev_integer)
			flags = preserve?STFL_PRESERVEA|STFL_PRESERVEB:0;
		else
		{
			flags = preserve?STFL_PRESERVEA:0;
			if (preserve)
			{
				flags = STFL_PRESERVEA;
				QCC_UnFreeTemp(index);
			}
			QCC_SupplyConversion(index, ev_integer, true);
		}

		index = QCC_DP_GlobalAddress(base, index, flags);
		switch(t->type)
		{
		case ev_string:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_S], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_float:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_F], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_vector:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_V], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_entity:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_ENT], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_field:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_FLD], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_function:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_FNC], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_pointer:
		case ev_integer:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_GLOAD_I], index, nullsref, NULL, flags);	//get pointer to precise def.
			break;
		case ev_variant:
		case ev_struct:
		case ev_union:
		case ev_int64:
		case ev_uint64:
		case ev_double:
			{
				QCC_sref_t r;
				unsigned int i;
				r = QCC_GetTemp(t);

				//we can just bias the base offset instead of lots of statements to add to the index. how handy.
				for (i = 0; i < t->size; )
				{
					if (t->size - i >= 3)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_GLOAD_V], index, nullsref, r, false);
						i+=3;
						index = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], index, QCC_MakeIntConst(3), NULL);
						r.ofs+=3;
					}
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_GLOAD_I], index, nullsref, r, false);
						i++;
						index = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], index, QCC_MakeIntConst(1), NULL);
						r.ofs++;
					}
				}
				if (!preserve)
					QCC_FreeTemp(index);
				r.ofs -= i;
				return r;
			}
			return nullsref;
		default:
			QCC_PR_ParseError(ERR_NOVALIDOPCODES, "Unable to load type %s with GLOAD... oops.", basictypenames[t->type]);
			return nullsref;
		}

		base.cast = t;
		return base;
	}
	else if (accel == 2)
	{	//fte-style, simpler indexing.
		if (index.cast->type == ev_integer)
			flags = preserve?STFL_PRESERVEA|STFL_PRESERVEB:0;
		else
		{
			flags = preserve?STFL_PRESERVEA:0;
			if (preserve)
			{
				flags = STFL_PRESERVEA;
				QCC_UnFreeTemp(index);
			}
			index = QCC_SupplyConversion(index, ev_integer, true);
		}
		switch(t->type)
		{
		case ev_string:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_S], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_float:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_F], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_vector:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_V], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_entity:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_ENT], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_field:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_FLD], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_function:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_FNC], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_pointer:	//no OP_LOADA_P
		case ev_integer:
		case ev_uint:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_I], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_int64:
		case ev_uint64:
		case ev_double:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_I64], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_bitfld:
		case ev_variant:
		case ev_struct:
		case ev_union:
			{
				QCC_sref_t r;
				unsigned int i;
				r = QCC_GetTemp(t);

				//we can just bias the base offset instead of lots of statements to add to the index. how handy.
				for (i = 0; i < t->size; )
				{
					if (t->size - i >= 3)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_LOADA_V], base, index, r, false);
						i+=3;
						base.ofs += 3;
						r.ofs+=3;
					}
					else if (t->size - i >= 2 && QCC_OPCodeValid(&pr_opcodes[OP_LOADA_I64]))
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_LOADA_I64], base, index, r, false);
						i+=2;
						base.ofs += 2;
						r.ofs+=2;
					}
					else
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_LOADA_I], base, index, r, false);
						i++;
						base.ofs++;
						r.ofs++;
					}
				}
				if (!preserve)
				{
					QCC_FreeTemp(base);
					QCC_FreeTemp(index);
				}
				r.ofs -= i;
				base.ofs -= i;
				return r;
			}
			return nullsref;
		default:
			QCC_PR_ParseError(ERR_NOVALIDOPCODES, "Unable to load type %s with LOADA... oops.", basictypenames[t->type]);
			return nullsref;
		}

		base.cast = t;
		return base;
	}
	else if (accel == 1)
	{	//hexen2-style, using float indexes and built-in bounds that hurts for arrays in structs.
		if (!base.sym->arraysize)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array lookup on non-array");
		if (!base.sym->arraylengthprefix)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array lookup on symbol without length prefix");

		if (base.sym->temp)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array lookup on a temp");

		if (index.cast->type == ev_float)
			flags = preserve?STFL_PRESERVEA|STFL_PRESERVEB:0;
		else
		{
			flags = preserve?STFL_PRESERVEA:0;
			if (preserve)
			{
				flags = STFL_PRESERVEA;
				QCC_UnFreeTemp(index);
			}
			QCC_SupplyConversion(index, ev_float, true);
		}

		/*hexen2 format has opcodes to read arrays (but has no way to write)*/
		switch(t->type)
		{
		case ev_field:
		case ev_pointer:
		case ev_integer:
		case ev_float:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_F], base, index, NULL, flags);	//get pointer to precise def.
			base.cast = t;
			break;
		case ev_vector:
			//hexen2 uses element indicies. we internally use words.
			//words means you can pack vectors into structs without the offset needing to be a multiple of 3.
			//as its floats, I'm going to try using 0/0.33/0.66 just for the luls
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			index = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], index, QCC_MakeFloatConst(3), NULL, flags&STFL_PRESERVEA);
			flags &= ~STFL_PRESERVEB;
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_V], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_string:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_S], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_entity:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_E], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_function:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_FNC], base, index, NULL, flags);	//get pointer to precise def.
			break;
		default:
			{
				QCC_sref_t r, newidx;
				unsigned int i;
				r = QCC_GetTemp(t);

				for (i = 0; i < t->size; )
				{	//we can't cheese these offsets because OP_FETCH_GBL_ has some built-in bounds check based upon a global just before the start of the array.
					//I'm going to skip using vector reads, because offsets get weird then, necessitating division ops, and breaking bounds checks.
					if (i)
					{
						newidx = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], index, QCC_MakeFloatConst(i), NULL, STFL_PRESERVEA);
					//	if (t->size - i >= 3)
					//		newidx = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], newidx, QCC_MakeFloatConst(3), NULL, 0);
					}
					else
					{
						newidx = index;
						QCC_UnFreeTemp(newidx);
					}

					/*if (t->size - i >= 3)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_V], base, newidx, r, false);
						i+=3;
						r.ofs+=3;
					}
					else*/
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_F], base, newidx, r, false);
						i++;
						r.ofs++;
					}
					QCC_FreeTemp(newidx);
				}
				if (!preserve)
				{
					QCC_FreeTemp(base);
					QCC_FreeTemp(index);
				}
				r.ofs -= i;
				return r;
			}
			QCC_PR_ParseError(ERR_NOVALIDOPCODES, "No op available to read array with FETCHGBL");
			return nullsref;
		}
		base.cast = t;
		return base;
	}
	else
	{
		/*emulate the array access using a function call to do the read for us*/
		QCC_sref_t args[1], funcretr;

		base.sym->referenced = true;

		if (base.cast->type == ev_field && base.sym->constant && !base.sym->initialized && !flag_boundchecks && flag_fasttrackarrays)
		{
			int i;
			//denormalised floats means we could do:
			//return (add_f: base + (mul_f: index*1i))
			//make sure the array has no gaps
			//the initialised thing is to ensure that it doesn't contain random consecutive system fields that might get remapped weirdly by an engine.
			for (i = 1; i < base.sym->arraysize; i++)
			{
				if (QCC_SRef_DataWord(base, i)->_int != QCC_SRef_DataWord(base, i-1)->_int+1)
					break;
			}
			//its contiguous. we'll do this in two instructions.
			if (i == base.sym->arraysize)
			{
				if (QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]))
					return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], base, index, NULL, 0);
				else
				{
					QCC_PR_ParseWarning(WARN_DENORMAL, "using denormals to accelerate field-array access, which is unsafe");	
					return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], base, QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], index, QCC_MakeIntConst(1), NULL, 0), NULL, 0);
				}
			}
		}

		funcretr = QCC_PR_GetSRef(NULL, qcva("ArrayGet*%s", QCC_GetSRefName(base)), base.sym->scope, false, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
		if (!funcretr.cast)
		{
			QCC_type_t *ftype = qccHunkAlloc(sizeof(*ftype));
			struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*1);
			ftype->size = 1;
			ftype->type = ev_function;
			ftype->aux_type = base.cast;
			ftype->params = fparms;
			ftype->num_parms = 1;
			ftype->name = "ArrayGet";
			fparms[0].type = type_float;
			funcretr = QCC_PR_GetSRef(ftype, qcva("ArrayGet*%s", QCC_GetSRefName(base)), base.sym->scope, true, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
			funcretr.sym->generatedfor = base.sym;
			if (!funcretr.sym->constant)
				externs->Printf("not constant?\n");
		}

		if (preserve)
			QCC_UnFreeTemp(index);
		else
			QCC_FreeTemp(base);

		/*make sure the function type that we're calling exists*/

		if (base.sym->type->type == ev_vector)
		{
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			args[0] = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], QCC_SupplyConversion(index, ev_float, true), QCC_MakeFloatConst(3), NULL);
			base = QCC_PR_GenerateFunctionCall1(nullsref, funcretr, args[0], NULL);
			base.cast = t;
		}
		else
		{
			if (t->size > 1)
			{
				QCC_sref_t r;
				unsigned int i;
				int old_op = opt_assignments;
				base = QCC_GetTemp(t);
				index = QCC_SupplyConversion(index, ev_float, true);

				for (i = 0; i < t->size; i++)
				{
					if (i)
						args[0] = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], index, QCC_MakeFloatConst(i), NULL, STFL_PRESERVEA);
					else
					{
						QCC_UnFreeTemp(index);
						args[0] = index;
						opt_assignments = false;
					}
					QCC_UnFreeTemp(funcretr);
					r = QCC_PR_GenerateFunctionCall1(nullsref, funcretr, args[0], type_float);
					opt_assignments = old_op;
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], r, base, NULL, STFL_PRESERVEB));
					base.ofs++;
				}
				QCC_FreeTemp(funcretr);
				QCC_FreeTemp(index);
				base.ofs -= i;
			}
			else
			{
				base = QCC_PR_GenerateFunctionCall1(nullsref, funcretr, QCC_SupplyConversion(index, ev_float, true), type_float);
			}
			base.cast = t;
		}
	}
	return base;
}

static pbool QCC_RefNeedsCalls(QCC_ref_t *ref)
{
	if (ref->type == REF_ACCESSOR)
		return true;
	if (ref->type == REF_ARRAY)
	{
		if (ref->index.cast)
		{
			int accel;
			if (QCC_SRef_EvalConst(ref->index))
				return false;	//can short it.

			if (ref->index.cast->type != ev_float || ref->cast->type != ref->base.cast->type)
				accel = 2;
			else
				accel = 1;
			if (accel == 2 && !QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]))
				accel = QCC_OPCodeValid(&pr_opcodes[OP_GLOAD_F])&&!ref->base.sym->temp?3:1;
			if (accel == 1 && (!ref->base.sym->arraylengthprefix || !QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F])))
				accel = QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F])?2:0;

			return !accel;	//if we've no acceleration, we need a call.
		}
	}
	return false;
}

QCC_sref_t QCC_BitfieldToDef(QCC_sref_t field, unsigned int bitofs)
{
//Note: this assumes the bitfield does not cross its basetype boundary.
	QCC_type_t *basetype = field.cast->parentclass;
	unsigned int bits = field.cast->bits;
	QCC_sref_t mask = QCC_MakeUIntConst((bitofs<<8)|bits);
	mask.sym->referenced = true;
	//shift it by the base.
	if (basetype->type == ev_integer)
		field = QCC_PR_StatementFlags(&pr_opcodes[OP_BITEXTEND_I], field, mask, NULL, 0);
	else if (basetype->type == ev_uint)
		field = QCC_PR_StatementFlags(&pr_opcodes[OP_BITEXTEND_U], field, mask, NULL, 0);
	else
		QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, field, "unsupported bitfield type");
	field.cast = basetype;
	return field;
}

//reads a ref as required
//the result sref should ALWAYS be freed, even if freetemps is set.
QCC_sref_t QCC_RefToDef(QCC_ref_t *ref, pbool freetemps)
{
	QCC_sref_t tmp = nullsref, idx;
	QCC_sref_t ret = ref->base;
	unsigned int bitofs = ref->bitofs;

	if (ref->postinc)
	{
		int inc = ref->postinc;
		if (ref->bitofs)
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, ret, "post increment operator on bitfield");
		ref->postinc = 0;
		//read the value, without preventing the store later
		ret = QCC_RefToDef(ref, false);
		//archive off the old value
		tmp = QCC_GetTemp(ret.cast);
		QCC_StoreToSRef(tmp, ret, ret.cast, false, true);
		ret = tmp;
		//update the value
		switch(ref->cast->type)
		{
		case ev_float:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], ret, QCC_MakeFloatConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_double:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_D], ret, QCC_MakeDoubleConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_string:
		case ev_integer:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_uint:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_U], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_int64:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I64], ret, QCC_MakeInt64Const(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_uint64:
			QCC_StoreSRefToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_U64], ret, QCC_MakeInt64Const(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_pointer:
			if (ref->cast->aux_type->bits)
			{
				if (flag_undefwordsize)
					QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, ret, "not allowed to make assumptions about pointer sizes. sorry.");
				inc *= ref->cast->aux_type->bits/8;
				tmp = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA);
			}
			else
			{
				inc *= ref->cast->aux_type->size;
				tmp = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA);
			}
			tmp.cast = ref->cast;	//make sure its the right pointer type. QCC_StoreSRefToRef is picky.
			QCC_StoreSRefToRef(ref, tmp, false, !freetemps);
			break;
		default:
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, ret, "post increment operator not supported with this type");
			break;
		}
		//hack any following uses of the ref to refer to the temp
		ref->type = REF_GLOBAL;
		ref->base = ret;
		ref->index = nullsref;
		ref->readonly = true;

		return ret;
	}

	switch(ref->type)
	{
	case REF_THISCALL:
	case REF_NONVIRTUAL:
		if (freetemps)
			QCC_FreeTemp(ref->index);
		else
			QCC_UnFreeTemp(ret);
		break;
	case REF_POINTERARRAY:
	case REF_ARRAYHEAD:
		{
			QCC_ref_t buf;
			ref = QCC_PR_GenerateAddressOf(&buf, ref);
			return QCC_RefToDef(ref, freetemps);
		}
		break;
	case REF_GLOBAL:
	case REF_ARRAY:
		if (!ret.sym->type->size)
			QCC_PR_ParseErrorPrintSRef(ERR_BADARRAYSIZE, ret, "symbol definition is incomplete");
		if (ref->index.cast)
		{
			//FIXME: array reads of array[immediate+offset] should generate (array+immediate)[offset] instead.
			//FIXME: this needs to be deprecated (and moved elsewhere)
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(ref->index);
			if (idxeval)
			{
				ref->base.sym->referenced = true;
				if (ref->index.sym)
					ref->index.sym->referenced = true;
				if (ref->cast->align)
				{
					bitofs += QCC_Eval_Int(idxeval, ref->index.cast)*ref->cast->align;
					ret.ofs += bitofs>>5;
					bitofs&=31;
				}
				else
					ret.ofs += QCC_Eval_Int(idxeval, ref->index.cast);

				if (freetemps)
					QCC_FreeTemp(ref->index);
				else
					QCC_UnFreeTemp(ret);
			}
			else
			{
				if (ref->cast->align!=32)	///bits must be a multiple of 8 here.
				{
					QCC_ref_t buf;
					QCC_UnFreeTemp(ret);
					tmp = QCC_RefToDef(QCC_PR_GenerateAddressOf(&buf, ref), freetemps);
					ret = QCC_LoadFromPointer(tmp, nullsref, ref->cast);
					if (ref->cast->type == ev_bitfld)
						ret.cast = ref->cast->parentclass;
					else
						ret.cast = ref->cast;
					return ret;
				}
				else
					ret = QCC_LoadFromArray(ref->base, ref->index, ref->cast, !freetemps);
			}
		}
		else if (freetemps)
			QCC_FreeTemp(ref->index);
		else
			QCC_UnFreeTemp(ret);
		break;
	case REF_POINTER:
		if (!ret.sym->symbolheader->symbolsize)
			QCC_PR_ParseErrorPrintSRef(ERR_BADARRAYSIZE, ret, "symbol definition is incomplete");

		if (ref->index.cast)
		{
//			if (!freetemps)
				QCC_UnFreeTemp(ref->index);
			idx = QCC_SupplyConversion(ref->index, ev_integer, true);
		}
		else
			idx = nullsref;
		if (ref->cast->type == ev_bitfld && ref->bitofs)
			idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], idx, QCC_MakeIntConst(ref->bitofs / ref->cast->bits), NULL);
		tmp = QCC_LoadFromPointer(ref->base, idx, ref->cast);
		QCC_FreeTemp(idx);
		if (freetemps)
			QCC_PR_DiscardRef(ref);
		return tmp;
	case REF_FIELD:
		return QCC_PR_ExpandField(ref->base, ref->index, ref->cast, freetemps?0:(STFL_PRESERVEA|STFL_PRESERVEB));
	case REF_STRING:
		if (ref->cast->type == ev_float)
		{
			idx = ref->index.cast?QCC_SupplyConversion(ref->index, ev_float, true):nullsref;
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_C], ref->base, idx, NULL, freetemps?0:(STFL_PRESERVEA|STFL_PRESERVEB));
		}
		else
		{
			idx = ref->index.cast?QCC_SupplyConversion(ref->index, ev_integer, true):nullsref;
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_U8], ref->base, idx, NULL, freetemps?0:(STFL_PRESERVEA|STFL_PRESERVEB));
		}
	case REF_ACCESSOR:
		if (ref->accessor && ref->accessor->getset_func[0].cast)
		{
			int args = 0;
			QCC_sref_t arg[2];
			if (ref->accessor->getset_isref[0] == 1)
			{
				if (ref->base.sym->temp)
					QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, ref->base, "Accessor %s(get) cannot be used on a temporary accessor reference", ref->accessor?ref->accessor->fieldname:"");
				//there shouldn't really be any need for this, but its problematic if the accessor is a field.
				arg[args++] = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], ref->base, nullsref, NULL);
			}
			else
				arg[args++] = ref->base;
			if (ref->accessor->indexertype)
				arg[args++] = ref->index.cast?QCC_SupplyConversion(ref->index, ref->accessor->indexertype->type, true):QCC_MakeIntConst(0);
			QCC_ForceUnFreeDef(ref->accessor->getset_func[0].sym);
			return QCC_PR_GenerateFunctionCallSref(nullsref, ref->accessor->getset_func[0], arg, args);
		}
		else if (ref->accessor && ref->accessor->staticval.cast)
		{
			QCC_ForceUnFreeDef(ref->accessor->staticval.sym);
			return ref->accessor->staticval;
		}
		else
			QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, ref->base, "Accessor %s has no get function", ref->accessor?ref->accessor->fieldname:"");
		break;
	}
	ret.cast = ref->cast;

	if (ret.cast->type == ev_bitfld)
		ret = QCC_BitfieldToDef(ret, bitofs);
	return ret;
}

//return value is the 'source', unless we folded the store and stripped a temp, in which case it'll be the new value at the given location, either way should have the same value as source.
QCC_sref_t QCC_StoreSRefToRef(QCC_ref_t *dest, QCC_sref_t source, pbool readable, pbool preservedest)
{
	const QCC_eval_t *eval;
	QCC_ref_t ptrref;
	pbool nullsrc = QCC_SRef_IsNull(source);
	if (dest->readonly)
	{
		QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to constant %s", QCC_GetSRefName(dest->base));
		QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, dest->base);
		if (dest->index.cast)
			QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, dest->index);
	}

	if (!nullsrc)
	/*{
		if (dest->cast->type == ev_struct || dest->cast->type == ev_union)
		{
			QCC_FreeTemp(source);
			source = QCC_MakeIntConst(0);
		}
		else if (dest->cast->type == ev_vector)
		{
			QCC_FreeTemp(source);
			source = QCC_MakeVectorConst(0, 0, 0);
		}
		source.cast = dest->cast;
	}
	else*/
	{
		QCC_type_t *t = source.cast;
		while(t)
		{
			if (!typecmp_lax(t, dest->cast))
				break;
			t = t->parentclass;
		}
		if (!t && !(source.cast->type == ev_pointer && dest->cast->type == ev_pointer && (source.cast->aux_type->type == ev_void || source.cast->aux_type->type == ev_variant)) && source.cast->type != ev_variant && dest->cast->type != ev_variant)
		{	//extra check to allow void*->any*
			char typea[256];
			char typeb[256];
			if (source.cast->type == ev_variant || dest->cast->type == ev_variant)
				QCC_PR_ParseWarning(WARN_IMPLICITVARIANTCAST, "type mismatch: %s %s to %s %s.%s", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base), QCC_GetSRefName(dest->index));
			else if (dest->cast->type == ev_bitfld && dest->cast->parentclass == source.cast)
				;	//dest is a bitfield of the same source type. silent truncation is fine.
			else if ((dest->cast->type == ev_float ||
					 dest->cast->type == ev_integer ||
					 dest->cast->type == ev_uint ||
					 dest->cast->type == ev_int64 ||
					 dest->cast->type == ev_uint64 ||
					 dest->cast->type == ev_double ||
					 dest->cast->type == ev_boolean) && (
					 source.cast->type == ev_float ||
					 source.cast->type == ev_integer ||
					 source.cast->type == ev_uint ||
					 source.cast->type == ev_int64 ||
					 source.cast->type == ev_uint64 ||
					 source.cast->type == ev_double ||
					 source.cast->type == ev_boolean))
			{
				if (dest->cast->type == ev_boolean)
					source = QCC_SupplyConversion(QCC_PR_GenerateLogicalTruth(source, "cannot convert to boolean"), dest->cast->parentclass->type, true);
				else
					source = QCC_SupplyConversion(source, dest->cast->type, true);
			}
			else
			{
				TypeName(source.cast, typea, sizeof(typea));
				TypeName(dest->cast, typeb, sizeof(typeb));
				if (dest->type == REF_FIELD)
				{
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s %s.%s", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base), QCC_GetSRefName(dest->index));
					QCC_PR_ParsePrintDef(WARN_STRICTTYPEMISMATCH, source.sym);
				}
				else if (dest->index.cast && strcmp("IMMEDIATE", dest->index.sym->name))
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s %s[%s]", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base), QCC_GetSRefName(dest->index));
				else
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s %s", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base));
			}
		}
	}

	if (dest->cast->type == ev_bitfld && dest->type != REF_POINTER && QCC_SRef_EvalConst(dest->index))
	{
		unsigned int bits = dest->cast->bits;
		unsigned int bitofs = dest->bitofs;
		QCC_sref_t old;
		if (bits == 32 && !bitofs)
			;	//o.O no packing needed.
		else
		{
			QCC_type_t *bittype = dest->cast;
			dest->bitofs = 0;
			if (dest->cast->align != 32)
			{
				int idx = QCC_Eval_Int(QCC_SRef_EvalConst(dest->index), dest->index.cast);
				if (idx)
				{
					idx *= dest->cast->align;
					bitofs += idx&31;
					idx /= 32;
					dest->index = QCC_MakeUIntConst(idx);
				}
			}
			dest->cast = dest->cast->parentclass;
			old = QCC_RefToDef(dest, false);
			dest->bitofs = bitofs;
			dest->cast = bittype;

			source = QCC_PR_Statement_BitCopy(source, bitofs, bits, old);
		}

//		if (readable)
//			QCC_PR_ParseWarning(ERR_PARSEERRORS, "left operand (%s) is a bitfield and not readable! oh noes!", QCC_GetSRefName(dest->base));
	}
	else if (dest->bitofs)
		QCC_PR_ParseWarning(ERR_INTERNAL, "left operand (%s) has a bit offset", QCC_GetSRefName(dest->base));

	for(;;)
	{
		switch(dest->type)
		{
		case REF_ARRAYHEAD:
		case REF_POINTERARRAY:
			QCC_PR_ParseWarning(ERR_PARSEERRORS, "left operand must be an l-value (did you mean %s[0]?)", QCC_GetSRefName(dest->base));
			if (!preservedest)
				QCC_PR_DiscardRef(dest);
			break;
		default:
			QCC_PR_ParseWarning(ERR_PARSEERRORS, "left operand must be an l-value (unsupported reference type)");
			if (!preservedest)
				QCC_PR_DiscardRef(dest);
			break;
		case REF_GLOBAL:
		case REF_ARRAY:
			if (!dest->index.cast || QCC_SRef_EvalConst(dest->index))
			{
				QCC_sref_t dd;
	//			QCC_PR_ParseWarning(0, "FIXME: trying to do references: assignments to arrays with const offset not supported.\n");
		case REF_NONVIRTUAL:
				dd.cast = dest->cast;
				dd.ofs = dest->base.ofs;
				dd.sym = dest->base.sym;

				if ((eval=QCC_SRef_EvalConst(dest->index)))
				{
					if (!preservedest)
						QCC_FreeTemp(dest->index);
					dd.ofs += QCC_Eval_Int(eval, dest->index.cast);
				}

				//FIXME: can dest even be a temp?
//				if (readable)
//					QCC_UnFreeTemp(source);
				source = QCC_CollapseStore(dd, source, dest->cast, readable, preservedest);
			}
			else
			{
				if (readable)
					QCC_UnFreeTemp(source);
				QCC_StoreToArray(dest->base, dest->index, source, dest->cast);
			}
			break;
		case REF_POINTER:
			source.sym->referenced = true;
			QCC_StoreToPointer(dest->base, dest->index.cast?QCC_SupplyConversion(dest->index, ev_integer, true):nullsref, source, dest->cast);
			if (dest->base.sym)
				dest->base.sym->referenced = true;
			if (!preservedest)
				QCC_FreeTemp(dest->base);
			if (!readable)
			{
				QCC_FreeTemp(source);
				source = nullsref;
			}
			break;
		case REF_STRING:
			{
				QCC_sref_t addr = dest->base;
				QCC_sref_t idx = dest->index;
				int op;
				if (source.cast->type==ev_float && QCC_OPCodeValid(&pr_opcodes[OP_STOREP_C]))
					op = OP_STOREP_C;
				else if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I8]))
					op = OP_STOREP_I8;
				else if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_C]))
					op = OP_STOREP_C;
				else
					op = OP_STOREP_I8;

				if (op == OP_STOREP_C)
					source = QCC_SupplyConversion(source, ev_float, true);
				else
					source = QCC_SupplyConversion(source, ev_integer, true);

				if (idx.cast)
					idx = QCC_SupplyConversion(idx, ev_integer, true);
				if (idx.sym && !QCC_OPCode_StorePOffset())
				{	//can't do an offset store yet... bake any index into the pointer.
					if (idx.cast)
						addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], addr, idx, NULL);
					idx = nullsref;
				}
				QCC_PR_Statement_SmallStore(op, source, addr, idx);
			}
			break;
		case REF_ACCESSOR:
			if (dest->accessor && dest->accessor->getset_func[1].cast)
			{
				int args = 0;
				QCC_sref_t arg[3];
				if (dest->accessor->getset_isref[1] == 1)
				{
					if (dest->base.sym->temp)
						QCC_PR_ParseErrorPrintDef(ERR_NOFUNC, dest->base.sym, "Accessor %s(set) cannot be used on a temporary accessor reference", dest->accessor?dest->accessor->fieldname:"");
					//there shouldn't really be any need for this, but its problematic if the accessor is a field.
					arg[args++] = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], dest->base, nullsref, NULL);
				}
				else
					arg[args++] = dest->base;
				if (dest->accessor->indexertype)
					arg[args++] = dest->index.cast?QCC_SupplyConversion(dest->index, dest->accessor->indexertype->type, true):QCC_MakeIntConst(0);
				arg[args++] = source;

				if (readable)	//if we're returning source, make sure it can't get freed
					QCC_UnFreeTemp(source);
				QCC_ForceUnFreeDef(dest->accessor->getset_func[1].sym);
				QCC_FreeTemp(QCC_PR_GenerateFunctionCallSref(nullsref, dest->accessor->getset_func[1], arg, args));
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, dest->base, "Accessor has no set function");
			break;
		case REF_FIELD:
			{
				int storef_opcode;
				//fixme: we should do this earlier, to preserve original instruction ordering.
				//such that self.enemy = (self = world); still has the same result (more common with function calls)

				dest->base.sym->referenced = true;
				dest->index.sym->referenced = true;
				source.sym->referenced = true;

				if (dest->cast->type == ev_float)
					storef_opcode = OP_STOREF_F;
				else if (dest->cast->type == ev_vector)
					storef_opcode = OP_STOREF_V;
				else if (dest->cast->type == ev_string)
					storef_opcode = OP_STOREF_S;
				else if (	dest->cast->type == ev_entity ||
							dest->cast->type == ev_field ||
							dest->cast->type == ev_function ||
							dest->cast->type == ev_pointer ||
							dest->cast->type == ev_integer ||
							dest->cast->type == ev_uint ||
							dest->cast->type == ev_struct ||
							dest->cast->type == ev_union ||
							dest->cast->type == ev_enum ||
							dest->cast->type == ev_bitfld ||
							dest->cast->type == ev_int64 ||
							dest->cast->type == ev_uint64 ||
							dest->cast->type == ev_double)
					storef_opcode = OP_STOREF_I;
				else
					storef_opcode = OP_DONE;
				//don't use it for arrays. address+storep_with_offset is less opcodes.
				if (storef_opcode!=OP_DONE && dest->index.cast->size == dest->cast->size && QCC_OPCodeValid(&pr_opcodes[storef_opcode]))
				{
					//doesn't generate any temps.
					int sz = dest->cast->size, i;
					if (nullsrc)
					{
						for (i = 0; i+2 < sz; i+=3, dest->index.ofs += 3)
							QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREF_V], dest->base, dest->index, QCC_MakeVectorConst(0,0,0), true);
						if (QCC_OPCodeValid(&pr_opcodes[OP_STOREF_I64]))
							for (; i+1 < sz; i+=2, dest->index.ofs += 2)
								QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREF_V], dest->base, dest->index, QCC_MakeVectorConst(0,0,0), true);
						for (; i < sz; i++, dest->index.ofs++)
							QCC_PR_SimpleStatement(&pr_opcodes[storef_opcode], dest->base, dest->index, QCC_MakeIntConst(0), true);
					}
					else
					{
						for (i = 0; i+2 < sz; i+=3, dest->index.ofs += 3, source.ofs += 3)
							QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREF_V], dest->base, dest->index, source, true);
						if (QCC_OPCodeValid(&pr_opcodes[OP_STOREF_I64]))
							for (; i+1 < sz; i+=2, dest->index.ofs += 2, source.ofs += 2)
								QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREF_I64], dest->base, dest->index, source, true);
						for (; i < sz; i++, dest->index.ofs++, source.ofs++)
							QCC_PR_SimpleStatement(&pr_opcodes[storef_opcode], dest->base, dest->index, source, true);
						source.ofs -= i;
					}
					dest->index.ofs -= i;
					if (!readable)
					{
						QCC_FreeTemp(source);
						source = nullsref;
					}

					if (!preservedest)
					{
						QCC_FreeTemp(dest->base);
						QCC_FreeTemp(dest->index);
					}
				}
				else if (1)//dest->cast->type >= ev_variant)
				{
					QCC_sref_t t;
					int sz = dest->cast->size, i;
					for (i = 0; i+2 < sz; i+=3, dest->index.ofs += 3, source.ofs+=3)
					{
						t = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
						QCC_StoreToPointer(t, nullsref, nullsrc?QCC_MakeVectorConst(0,0,0):source, type_vector);
						QCC_FreeTemp(t);
					}
					if (QCC_OPCodeValid(&pr_opcodes[OP_STOREP_I64]))
					for (; i+1 < sz; i+=2, dest->index.ofs += 2, source.ofs+=2)
					{
						t = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
						QCC_StoreToPointer(t, nullsref, nullsrc?QCC_MakeInt64Const(0):source, type_int64);
						QCC_FreeTemp(t);
					}
					for (; i < sz; i++, dest->index.ofs++, source.ofs++)
					{
						t = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
						QCC_StoreToPointer(t, nullsref, nullsrc?QCC_MakeFloatConst(0):source, (dest->cast->size!=1)?type_float:dest->cast);
						QCC_FreeTemp(t);
					}
					source.ofs -= i;
					dest->index.ofs -= i;

					if (!readable)
					{
						QCC_FreeTemp(source);
						source = nullsref;
					}
					if (!preservedest)
					{
						QCC_FreeTemp(dest->base);
						QCC_FreeTemp(dest->index);
					}
				}
				else
				{
					dest = QCC_PR_BuildRef(&ptrref, REF_POINTER,
									QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, preservedest?STFL_PRESERVEA:0),	//pointer address
									nullsref, (dest->index.cast->type == ev_field)?dest->index.cast->aux_type:type_variant, dest->readonly, 0);
					preservedest = false;
					continue;
				}
			}
			break;
		}
		break;
	}
	return source;
}

/*QCC_ref_t *QCC_PR_RefTerm (QCC_ref_t *ref, unsigned int exprflags)
{
	return QCC_DefToRef(ref, QCC_PR_Term(exprflags));
}*/
QCC_sref_t QCC_PR_Term (unsigned int exprflags)
{
	QCC_ref_t refbuf;
	return QCC_RefToDef(QCC_PR_RefTerm(&refbuf, exprflags), true);
}
QCC_sref_t	QCC_PR_ParseValue (QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers)
{
	QCC_ref_t refbuf;
	return QCC_RefToDef(QCC_PR_ParseRefValue(&refbuf, assumeclass, allowarrayassign, expandmemberfields, makearraypointers), true);
}
QCC_sref_t QCC_PR_ParseArrayPointer (QCC_sref_t d, pbool allowarrayassign, pbool makestructpointers)
{
	QCC_ref_t refbuf;
	QCC_ref_t inr;
	QCC_DefToRef(&inr, d);
	return QCC_RefToDef(QCC_PR_ParseRefArrayPointer(&refbuf, &inr, allowarrayassign, makestructpointers), true);
}

void QCC_PR_DiscardRef(QCC_ref_t *ref)
{
	if (ref->postinc)
	{
		QCC_sref_t oval;
		int inc = ref->postinc;
		if (ref->bitofs)
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, ref->base, "post increment operator on bitfield");
		ref->postinc = 0;
		//read the value
		oval = QCC_RefToDef(ref, false);
		//and update it
		switch(oval.cast->type)
		{
		case ev_float:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], oval, QCC_MakeFloatConst(inc), NULL, 0);
			break;
		case ev_double:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_D], oval, QCC_MakeDoubleConst(inc), NULL, 0);
			break;
		case ev_string:
		case ev_integer:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], oval, QCC_MakeIntConst(inc), NULL, 0);
			break;
		case ev_uint:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_U], oval, QCC_MakeIntConst(inc), NULL, 0);
			break;
		case ev_int64:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I64], oval, QCC_MakeInt64Const(inc), NULL, 0);
			break;
		case ev_uint64:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_U64], oval, QCC_MakeInt64Const(inc), NULL, 0);
			break;
		case ev_pointer:
			if (ref->cast->aux_type->bits)
			{
				inc *= ref->cast->aux_type->bits/8;
				oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], oval, QCC_MakeIntConst(inc), NULL, 0);
			}
			else
			{
				inc *= ref->cast->aux_type->size;
				oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], oval, QCC_MakeIntConst(inc), NULL, 0);
			}
			oval.cast = ref->cast;
			break;
		default:
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, oval, "post increment operator not supported with this type");
			break;
		}
		QCC_StoreSRefToRef(ref, oval, false, false);
		qcc_usefulstatement = true;
	}
	else
	{
		QCC_FreeTemp(ref->base);
		if (ref->index.cast)
			QCC_FreeTemp(ref->index);
	}
}

static QCC_opcode_t *QCC_PR_ChooseOpcode(QCC_sref_t lhs, QCC_sref_t rhs, QCC_opcode_t **priority)
{
	QCC_opcode_t	*op, *oldop;

	QCC_opcode_t *bestop;
	int numconversions, c;

	etype_t type_a;
	etype_t type_c;

	op = oldop = *priority++;

	// type check
	type_a = lhs.cast->type;
//	type_b = rhs.cast->type;

	if (type_a == ev_enum)
	{
		if (lhs.cast == rhs.cast)
		{
			lhs.cast = lhs.cast->aux_type;
			rhs.cast = rhs.cast->aux_type;
//			type_a = lhs.cast->type;
		}
	}

	if (type_a == ev_boolean)
	{
		lhs.cast = lhs.cast->parentclass;
		type_a = lhs.cast->type;
	}
	if (rhs.cast->type == ev_boolean)
		rhs.cast = rhs.cast->parentclass;

	if (op->name[0] == '.')// field access gets type from field
	{
		if (rhs.cast->aux_type)
			type_c = rhs.cast->aux_type->type;
		else
			type_c = -1;	// not a field
	}
	else
		type_c = ev_void;

	bestop = NULL;
	numconversions = 32767;
	while (op)
	{
		if (!(type_c != ev_void && type_c != (*op->type_c)->type))
		{
			if (!STRCMP (op->name , oldop->name))	//matches
			{
				//return values are never converted - what to?
//				if (type_c != ev_void && type_c != op->type_c->type->type)
//				{
//					op++;
//					continue;
//				}

				if (op->associative!=ASSOC_LEFT)
				{//assignment
#if 0
					if (op->type_a == &type_pointer)	//ent var
					{
						/*FIXME: I don't like this code*/
						if (lhs->type->type != ev_pointer)
							c = -200;	//don't cast to a pointer.
						else if ((*op->type_c)->type == ev_void && op->type_b == &type_pointer && rhs.cast->type == ev_pointer)
							c = 0;	//generic pointer... fixme: is this safe? make sure both sides are equivelent
						else if (lhs->type->aux_type->type != (*op->type_b)->type)	//if e isn't a pointer to a type_b
							c = -200;	//don't let the conversion work
						else
							c = QCC_canConv(rhs, (*op->type_c)->type);
					}
					else
#endif
					{
						c=QCC_canConv(rhs, (*op->type_b)->type);
						if (type_a != (*op->type_a)->type)	//in this case, a is the final assigned value
							c = -300;	//don't use this op, as we must not change var b's type
						else if ((*op->type_a)->type == ev_pointer && lhs.cast->aux_type->type != (*op->type_a)->aux_type->type)
							c = -300;	//don't use this op if its a pointer to a different type
					}
				}
				else
				{
					int l = QCC_canConv(lhs, (*op->type_a)->type);
					int r = QCC_canConv(rhs, (*op->type_b)->type);
					c = min(l,r);
					if (c >= 0)
					{
						c = max(l,r);
						/*QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "%s (%i): Possible conversion from %s to %s (%i), %s to %s (%i)",
							op->opname, c,
							lhs.cast->name, (*op->type_a)->name, l,
							rhs.cast->name, (*op->type_b)->name, r);*/
					}
				}

				if (c>=0 && c < numconversions)
				{
					bestop = op;
					numconversions=c;
					if (c == 0)//can't get less conversions than 0...
						break;
				}
			}
			else
				break;
		}
		op = *priority++;
	}
	if (bestop == NULL)
	{
//		if (oldop->priority == CONDITION_PRIORITY)
//			op = oldop;
//		else
		{
			char temp1[256];
			char temp2[256];
			op = oldop;
			QCC_PR_ParseWarning(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, "type mismatch for %s (%s%s%s and %s%s%s)", oldop->name, col_type,TypeName(lhs.cast,temp1,sizeof(temp1)),col_none, col_type,TypeName(rhs.cast,temp2,sizeof(temp2)),col_none);
			QCC_PR_ParsePrintSRef(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, lhs);
			QCC_PR_ParsePrintSRef(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, rhs);
		}
	}
	else
	{
		op = bestop;
		/*if (numconversions>3)
		{
			c=QCC_canConv(lhs, (*op->type_a)->type);
			if (c>3)
				QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", lhs.cast->name, (*op->type_a)->name);
			c=QCC_canConv(rhs, (*op->type_b)->type);
			if (c>3)
				QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", rhs.cast->name, (*op->type_b)->name);
		}*/
	}
	return op;
}

//used to optimise logicops slightly.
static pbool QCC_OpHasSideEffects(QCC_statement_t *st)
{
	//function calls potentially always have side effects (and are expensive)
	if ((st->op >= OP_CALL0 && st->op <= OP_CALL8) || (st->op >= OP_CALL1H && st->op <= OP_CALL8H))
		return true;
	//otherwise if we're assigning to some variable (that isn't a temp) then it has a side effect.
	//FIXME: this doesn't catch op_address+op_storep_*, but that should generally not happen as logicops is tied to a single statement.
	if (st->c.sym && !st->c.sym->temp && OpAssignsToC(st->op))
		return true;
	if (st->b.sym && !st->b.sym->temp && OpAssignsToB(st->op))
		return true;
	if (st->a.sym && !st->a.sym->temp && OpAssignsToA(st->op))
		return true;
	return false;
}

QCC_ref_t *QCC_PR_RefExpression (QCC_ref_t *retbuf, int priority, int exprflags)
{
	QCC_ref_t rhsbuf;
//	QCC_dstatement32_t	*st;
	QCC_opcode_t	*op;

	int opnum;

	QCC_ref_t		*lhsr, *rhsr;
	QCC_sref_t		lhsd, rhsd;

	if (priority == 0)
	{
		lhsr = QCC_PR_RefTerm (retbuf, exprflags);
		if (!STRCMP(pr_token, "++"))
		{
			if (lhsr->readonly)
				QCC_PR_ParseError(ERR_PARSEERRORS, "postincrement: lhs is readonly");
			lhsr->postinc += 1;
			QCC_PR_Lex();
		}
		else if (!STRCMP(pr_token, "--"))
		{
			if (lhsr->readonly)
				QCC_PR_ParseError(ERR_PARSEERRORS, "postdecrement: lhs is readonly");
			lhsr->postinc += -1;
			QCC_PR_Lex();
		}
		return lhsr;
	}

	lhsr = QCC_PR_RefExpression (retbuf, priority-1, exprflags);

	while (1)
	{
		if (priority == FUNC_PRIORITY && QCC_PR_CheckToken ("(") )
		{
			qcc_usefulstatement=true;
			lhsd = QCC_PR_ParseFunctionCall (lhsr);
			lhsr = QCC_DefToRef(&rhsbuf, lhsd);
			lhsr = QCC_PR_ParseRefArrayPointer(retbuf, lhsr, true, true);
			if (lhsr == &rhsbuf)
			{
				*retbuf = rhsbuf;
				lhsr = retbuf;
			}
		}
		if (priority == TERNARY_PRIORITY && QCC_PR_CheckToken ("?"))
		{
			//if we have no int types, force all ints to floats here, just to ensure that we don't end up with non-constant ints that we then can't cope with.

			QCC_sref_t r;
			QCC_statement_t *fromj, *elsej, *truthstore;
			QCC_sref_t val = QCC_RefToDef(lhsr, true);
			const QCC_eval_t *eval = QCC_SRef_EvalConst(val);
			pbool lvalisnull = false;
			if (pr_scope)
				eval = NULL; //FIXME: we need the gotos to avoid sideeffects, which is annoying.
			if (QCC_PR_CheckToken(":"))
			{
				if (eval)
				{
					if (QCC_Eval_Truth(eval, val.cast, false))
					{
						QCC_FreeTemp(QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA));
						return QCC_DefToRef(retbuf, val);
					}
					else
					{
						QCC_FreeTemp(val);
						val = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
						return QCC_DefToRef(retbuf, val);
					}
				}

				eval = NULL;
				//r=a?:b -> if (a) r=a else r=b;
				fromj = QCC_Generate_OP_IFNOT(val, true);
				lvalisnull = QCC_SRef_IsNull(val);
#if 1
				//hack: make local, not temp. this prevents assignment/temp folding...
				r = QCC_MakeSRefForce(QCC_PR_DummyDef(r.cast=val.cast, "ternary", pr_scope, 0, NULL, 0, false, GDF_STRIP), 0, val.cast);
#else
				r = QCC_GetTemp(val.cast);
#endif
				QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[(r.cast->size>=3)?OP_STORE_V:OP_STORE_F], val, r, &truthstore, STFL_PRESERVEB));
			}
			else
			{
				if (eval)
				{
					if (eval->_int)
					{
						QCC_FreeTemp(val);
						val = QCC_PR_Expression(TOP_PRIORITY, 0);
						QCC_PR_Expect(":");
						QCC_FreeTemp(QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA));
						return QCC_DefToRef(retbuf, val);
					}
					else
					{
						QCC_FreeTemp(val);
						QCC_FreeTemp(QCC_PR_Expression(TOP_PRIORITY, 0));
						QCC_PR_Expect(":");
						val = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
						return QCC_DefToRef(retbuf, val);
					}
				}
				fromj = QCC_Generate_OP_IFNOT(val, false);

				val = QCC_PR_Expression(TOP_PRIORITY, 0);
				if (val.cast->type == ev_integer && !QCC_OPCodeValid(&pr_opcodes[OP_STORE_IF]))
					val = QCC_SupplyConversion(val, ev_float, true);
				lvalisnull = QCC_SRef_IsNull(val);
#if 1
				//hack: make local, not temp. this prevents assignment/temp folding...
				r = QCC_MakeSRefForce(QCC_PR_DummyDef(r.cast=val.cast, "ternary", pr_scope, 0, NULL, 0, false, GDF_STRIP), 0, val.cast);
#else
				r = QCC_GetTemp(val.cast);
#endif

				//fixme: QCC_StoreToSRef if it were not for saving truthstore
				switch(r.cast->size)
				{
				case 3:
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], val, r, &truthstore, STFL_PRESERVEB));
					break;
				case 2:
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], val, r, &truthstore, STFL_PRESERVEB));
					break;
				case 1:
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], val, r, &truthstore, STFL_PRESERVEB));
					break;
				default:
					QCC_PR_ParseError(ERR_BADEXTENSION, "oversized ternary result");
					break;
				}
				//r can be stomped upon until its reused anyway

				QCC_PR_Expect(":");
			}
			if (fromj)
			{
				QCC_PR_Statement(&pr_opcodes[OP_GOTO], nullsref, nullsref, &elsej);
				fromj->b.jumpofs = &statements[numstatements] - fromj;
			}
			else
				elsej = NULL;
			val = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			if (val.cast->type == ev_integer && !QCC_OPCodeValid(&pr_opcodes[OP_STORE_IF]))
				val = QCC_SupplyConversion(val, ev_float, true);

			if (r.cast->type == ev_integer && val.cast->type == ev_float)
			{	//cond?5i:5.1  should be accepted. change the initial store_f to a store_if.
				truthstore->op = OP_STORE_IF;
				r.cast = type_float;
			}
			else if (r.cast->type == ev_float && (val.cast->type == ev_integer||val.cast->type == ev_uint))
			{
				//cond?5.1:5i  should be accepted. change the just-parsed value to a float, as needed.
				val = QCC_SupplyConversion(val, ev_float, true);
			}

			//promote to unsigned if one isn't.
			else if (r.cast->type == ev_uint && val.cast->type == ev_integer)
				val.cast = type_uint;
			else if (r.cast->type == ev_integer && val.cast->type == ev_uint)
				r.cast = type_uint;
			else if (r.cast->type == ev_uint64 && val.cast->type == ev_int64)
				val.cast = type_uint64;
			else if (r.cast->type == ev_int64 && val.cast->type == ev_uint64)
				r.cast = type_uint64;

			if (typecmp(val.cast, r.cast) != 0)
			{
				while (val.cast->type == ev_boolean)
					val.cast = val.cast->parentclass;
				while (r.cast->type == ev_boolean)
					r.cast = r.cast->parentclass;

				if (typecmp(val.cast, r.cast) == 0)
					;
				else if (QCC_SRef_IsNull(val) && r.cast->size == val.cast->size)
					val.cast = r.cast;	//null is null... unless its a vector...
				else if (lvalisnull && r.cast->size == val.cast->size)
					r.cast = val.cast;	//null is null... unless its a vector...
				else if (typecmp_lax(val.cast, r.cast) != 0)
				{
					char typebuf1[256];
					char typebuf2[256];
					QCC_PR_ParseWarning(0, "Type mismatch on ternary operator: %s vs %s", TypeName(r.cast, typebuf1, sizeof(typebuf1)), TypeName(val.cast, typebuf2, sizeof(typebuf2)));
				}
				else
				{
					//if they're mixed int/float, cast to floats.
					QCC_PR_ParseError(0, "Ternary operator with mismatching types\n");
				}
			}

			QCC_StoreToSRef(r, val, val.cast, false, true);
//			QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[(r.cast->size>=3)?OP_STORE_V:OP_STORE_F], val, r, NULL, STFL_PRESERVEB));

			if (elsej)
				elsej->a.jumpofs = &statements[numstatements] - elsej;
			return QCC_DefToRef(retbuf, r);
		}

		opnum=0;

		if (pr_token_type == tt_immediate && *pr_token=='-')
		{
			//work around (4-3) being parsed as 4 -3 with no operator between
			//(don't get confused by "-foo" strings though
			if (pr_immediate_type->type == ev_float || pr_immediate_type->type == ev_double ||
				pr_immediate_type->type == ev_integer || pr_immediate_type->type == ev_uint ||
				pr_immediate_type->type == ev_int64 || pr_immediate_type->type == ev_uint64)
			{
				QCC_PR_IncludeChunk(pr_token, true, NULL);
				strcpy(pr_token, "+");//two negatives would make a positive.
				pr_token_type = tt_punct;
			}
		}

		if (pr_token_type != tt_punct)
		{
			if (priority == TOP_PRIORITY)
				QCC_PR_ParseWarning(WARN_UNEXPECTEDPUNCT, "Expected punctuation");
		}

		if (priority == ASSIGN_PRIORITY)
		{	//assignments
			QCC_opcode_t **ops = NULL, **ops_ptr;
			char *opname = NULL;
			int i;
			if (QCC_PR_CheckToken ("="))
			{
				ops = opcodes_store;
				ops_ptr = NULL;
				opname = "=";
			}
			else if (QCC_PR_CheckToken ("+="))
			{
				ops = opcodes_addstore;
				ops_ptr = opcodes_addstorep;
				opname = "+=";
			}
			else if (QCC_PR_CheckToken ("-="))
			{
				ops = opcodes_substore;
				ops_ptr = opcodes_substorep;
				opname = "-=";
			}
			else if (QCC_PR_CheckToken ("|="))
			{
				ops = opcodes_orstore;
				ops_ptr = opcodes_orstorep;
				opname = "|=";
			}
			else if (QCC_PR_CheckToken ("&="))
			{
				ops = opcodes_andstore;
				ops_ptr = NULL;
				opname = "&=";
			}
			else if (QCC_PR_CheckToken ("&~="))
			{
				ops = opcodes_clearstore;
				ops_ptr = opcodes_clearstorep;
				opname = "&~=";
			}
			else if (QCC_PR_CheckToken ("^="))
			{
				ops = opcodes_xorstore;
				ops_ptr = NULL;
				opname = "^=";
			}
			else if (QCC_PR_CheckToken ("*="))
			{
				ops = opcodes_mulstore;
				ops_ptr = opcodes_mulstorep;
				opname = "*=";
			}
			else if (QCC_PR_CheckToken ("<<="))
			{
				ops = opcodes_shlstore;
				ops_ptr = opcodes_none;
				opname = "<<=";
			}
			else if (QCC_PR_CheckToken (">>="))
			{
				ops = opcodes_shrstore;
				ops_ptr = opcodes_none;
				opname = ">>=";
			}
			else if (QCC_PR_CheckToken ("/="))
			{
				ops = opcodes_divstore;
				ops_ptr = opcodes_divstorep;
				opname = "/=";
			}
			else if (QCC_PR_CheckToken ("<=>"))
			{
				ops = opcodes_spaceship;
				ops_ptr = opcodes_none;
				opname = "<=>";
			}
			else
			{
				ops = NULL;
				ops_ptr = NULL;
				opname = NULL;
			}

			if (ops)
			{
				if (lhsr->postinc)
					QCC_PR_ParseError(ERR_INTERNAL, "Assignment to post-inc result");
				if (lhsr->readonly)
				{
					QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to const");
					QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, lhsr->base);
					if (lhsr->index.cast)
						QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, lhsr->index);
				}

				rhsr = QCC_PR_RefExpression (&rhsbuf, priority, exprflags | EXPR_DISALLOW_ARRAYASSIGN|EXPR_DISALLOW_COMMA);

				if ((lhsr->type == REF_POINTERARRAY || lhsr->type == REF_ARRAYHEAD) && lhsr->arraysize != 0 &&
					(rhsr->cast->type == ev_union && rhsr->cast->num_parms == 1 && !rhsr->cast->params[0].paramname) &&
					rhsr->cast->params[0].arraysize == lhsr->arraysize)
				{	//this is some sort of assignment, but these types are logically const pointers that cannot be assigned to themselves.
					//change the lhsr to a REF_POINTER instead
					if (lhsr->type == REF_POINTERARRAY)
						lhsr->type = REF_POINTER;
					else
						lhsr->type = REF_ARRAY;
					lhsr->cast = rhsr->cast;
					lhsr->arraysize = 0;	//just in case.
				}

				if (conditional&1)
					QCC_PR_ParseWarning(WARN_ASSIGNMENTINCONDITIONAL, "suggest parenthesis for assignment used as truth value");

				//FIXME: if this is a simple store, rhsr->type == REF_FIELD, and lhsd is a simple def, then we should just expand the field directly to the lhsd for more effient .structs
				rhsd = QCC_RefToDef(rhsr, true);

				if (ops_ptr && (lhsr->type == REF_FIELD || lhsr->type == REF_POINTER))
				{
					//*ptr += 5;
					//can become (address), addstorep_f 5,ptr,sideeffect
					//instead of loadf_f, add_f, address, storep_f
					//or instead of loadp_f, add_f, storep_f

					for (i = 0; (op=ops_ptr[i]); i++)
					{
						if (QCC_OPCodeValid(op))
						{
							if ((*op->type_b)->type == rhsd.cast->type && (*op->type_a)->type == ev_pointer && (*op->type_c)->type == lhsr->cast->type)
								break;
						}
					}
					if (op)
					{
						QCC_ref_t ptr;
						qcc_usefulstatement = true;
						lhsr = QCC_PR_GenerateAddressOf(&ptr, lhsr);
						lhsd = QCC_RefToDef(lhsr, true);
						rhsd = QCC_PR_Statement(op, rhsd, lhsd, NULL);
						lhsr = QCC_DefToRef(retbuf, rhsd);	//we read the rhs, we can just return that as the result
						lhsr->readonly = true;	//(a=b)=c is an error
						continue;
					}
				}
				if (ops != opcodes_store)
				{
					lhsd = QCC_RefToDef(lhsr, false);
					for (i = 0; (op=ops[i]); i++)
					{
//						if (QCC_OPCodeValid(op))
						{
							if ((*op->type_b)->type == rhsd.cast->type && (*op->type_a)->type == lhsd.cast->type)
								break;
						}
					}
					if (!ops[i])
					{
						rhsd = QCC_EvaluateCast(rhsd, lhsd.cast, true);
						for (i = 0; ops[i]; i++)
						{
							op = ops[i];
	//						if (QCC_OPCodeValid(op))
							{
								if ((*op->type_b)->type == rhsd.cast->type && (*op->type_a)->type == lhsd.cast->type)
									break;
							}
						}
						if (!ops[i])
							QCC_PR_ParseError(0, "Type mismatch on assignment. %s %s %s is not supported", lhsd.cast->name, opname, rhsd.cast->name);
					}

					if (op->associative != ASSOC_LEFT)
						rhsd = QCC_PR_Statement(op, lhsd, rhsd, NULL);
					else
						rhsd = QCC_PR_Statement(op, lhsd, rhsd, NULL);

					//convert so we don't have issues with: i = (int)(float)(i+f)
					//this will also catch things like vec *= vec; which would be trying to store a float into a vector.
					rhsd = QCC_SupplyConversionForAssignment(lhsr, rhsd, lhsr->cast, true);
				}
				else
				{
#if 1
					rhsd = QCC_EvaluateCast(rhsd, lhsr->cast, true);
#else
					/*if (flag_qccx && lhsr->cast->type == ev_pointer && rhsd.cast->type == ev_float)
					{	//&%555 = 4.0;
						char destname[256];
						QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit pointer dereference on assignment to %s", QCC_GetRefName(lhsr, destname, sizeof(destname)));
						lhsd = QCC_RefToDef(lhsr, true);
						lhsr = QCC_PR_BuildRef(retbuf, REF_POINTER, lhsd, nullsref, lhsd.cast->aux_type, false);
					}
					else */if (QCC_SRef_IsNull(rhsd))
					{
						QCC_FreeTemp(rhsd);
						rhsd = QCC_MakeIntConst(0);
						/*if (lhsr->cast->type == ev_vector)
							rhsd = QCC_MakeVectorConst(0,0,0);
						else if (lhsr->cast->type == ev_struct || lhsr->cast->type == ev_union)
						{
							QCC_PR_ParseError(0, "Type mismatch on assignment. %s %s %s is not supported", lhsr->cast->name, opname, rhsd.cast->name);
						}
						else if(lhsr->cast->type == ev_float)
							rhsd = QCC_MakeFloatConst(0);
						else if(lhsr->cast->type == ev_integer)
							rhsd = QCC_MakeIntConst(0);
						else
							rhsd = QCC_MakeIntConst(0);
						rhsd.cast = lhsr->cast;*/
					}
					else
						rhsd = QCC_SupplyConversionForAssignment(lhsr, rhsd, lhsr->cast, true);
#endif
				}
				rhsd = QCC_StoreSRefToRef(lhsr, rhsd, true, false);	//FIXME: this should not always be true, but we don't know if the caller actually needs it
				qcc_usefulstatement = true;
				lhsr = QCC_DefToRef(retbuf, rhsd);	//we read the rhs, we can just return that as the result
				lhsr->readonly = true;	//(a=b)=c is an error
			}
			else
				break;
		}
		else
		{
			QCC_statement_t *logicjump;
			QCC_statement_t *logictest;
			//go straight for the correct priority.
			for (op = opcodeprioritized[priority][opnum]; op; op = opcodeprioritized[priority][++opnum])
	//		for (op=pr_opcodes ; op->name ; op++)
			{
	//			if (op->priority != priority)
	//				continue;
				if (!QCC_PR_CheckToken (op->name))
					continue;

				logicjump = NULL;
				lhsd = QCC_RefToDef(lhsr, true);
				if (opt_logicops && (lhsd.cast->size == 1 || lhsd.cast->type == ev_vector))
				{
					if (!strcmp(op->name, "&&"))		//guarenteed to be false if the lhs is false
					{
						if (lhsd.cast->type == ev_vector && flag_ifvector)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_MUL_V], lhsd, lhsd, NULL, STFL_PRESERVEA);
						if (lhsd.cast->type == ev_string && flag_ifstring)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_S], lhsd, QCC_MakeStringConst(""), NULL, 0);
						if (lhsd.cast->type == ev_double)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_D], lhsd, QCC_MakeDoubleConst(0), NULL, 0);
						if (lhsd.cast->type == ev_int64 || lhsd.cast->type == ev_uint64)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_I64], lhsd, QCC_MakeInt64Const(0), NULL, 0);
						if (!QCC_Eval_Truth(QCC_SRef_EvalConst(lhsd), lhsd.cast, false))
						{
							QCC_ClobberDef(NULL);	//FIXME...
							logicjump = QCC_Generate_OP_IFNOT(lhsd, true);
						}
					}
					else if (!strcmp(op->name, "||"))	//guarenteed to be true if the lhs is true
					{
						if (lhsd.cast->type == ev_vector && flag_ifvector)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_MUL_V], lhsd, lhsd, NULL, STFL_PRESERVEA);
						if (lhsd.cast->type == ev_string && flag_ifstring)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_S], lhsd, QCC_MakeStringConst(""), NULL, 0);
						if (lhsd.cast->type == ev_double)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_D], lhsd, QCC_MakeDoubleConst(0), NULL, 0);
						if (lhsd.cast->type == ev_int64 || lhsd.cast->type == ev_uint64)
							lhsd = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_I64], lhsd, QCC_MakeInt64Const(0), NULL, 0);
						if (!QCC_Eval_Truth(QCC_SRef_EvalConst(lhsd), lhsd.cast, false))
						{
							QCC_ClobberDef(NULL);	//FIXME...
							logicjump = QCC_Generate_OP_IF(lhsd, true);
						}
					}
				}

				rhsr = QCC_PR_RefExpression (&rhsbuf, priority-1, exprflags | EXPR_DISALLOW_ARRAYASSIGN);

				if (op->associative!=ASSOC_LEFT)
				{
					QCC_PR_ParseError(ERR_INTERNAL, "internal error: should be unreachable\n");
				}
				else
				{
					rhsd = QCC_RefToDef(rhsr, true);
					op = QCC_PR_ChooseOpcode(lhsd, rhsd, &opcodeprioritized[priority][opnum]);

					if ((*op->type_a)->type != lhsd.cast->type && (*op->type_a)->type != ev_variant)
					{
						if (QCC_canConv(lhsd, (*op->type_a)->type) >= 100)
							QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", lhsd.cast->name, (*op->type_a)->name);
						lhsd = QCC_EvaluateCast(lhsd, (*op->type_a), true);
					}
					if ((*op->type_b)->type != rhsd.cast->type && (*op->type_b)->type != ev_variant)
					{
						if (QCC_canConv(rhsd, (*op->type_b)->type) >= 100)
							QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", rhsd.cast->name, (*op->type_b)->name);
						rhsd = QCC_EvaluateCast(rhsd, (*op->type_b), true);
					}

					if (logicjump)	//logic shortcut jumps to just before the if. the rhs is uninitialised if the jump was taken, but the lhs makes it deterministic.
					{
						logicjump->flags |= STF_LOGICOP;
						logicjump->b.jumpofs = &statements[numstatements] - logicjump;
						if (logicjump->b.jumpofs == 1)
						{
							numstatements--;	//err, that was pointless.
							logicjump = NULL;
						}
						else if (logicjump->b.jumpofs == 2 && !QCC_OpHasSideEffects(&logicjump[1]))
						{
							logicjump[0] = logicjump[1];
							numstatements--;	//don't bother if the jump is the same cost as the thing we're trying to skip (calls are expensive despite being a single opcode).
							logicjump = NULL;
						}
						else
							optres_logicops++;
					}

					if (logicjump)
					{
						lhsd = QCC_PR_Statement (op, lhsd, rhsd, &logictest);
						if (!logictest)
							numstatements = logicjump-statements;
						else
							logicjump->b.jumpofs = logictest - logicjump;
					}
					else
						lhsd = QCC_PR_Statement (op, lhsd, rhsd, NULL);
					lhsr = QCC_DefToRef(retbuf, lhsd);
				}

				if (priority > 1 && exprflags & EXPR_WARN_ABOVE_1)
					QCC_PR_ParseWarning(WARN_UNARYNOTSCOPE, "suggest parenthesis for unary operator that applies to multiple terms");

				break;
			}
			if (!op)
				break;
		}
	}
	if (lhsr == NULL)
		QCC_PR_ParseError(ERR_INTERNAL, "e == null");

	if (!(exprflags&EXPR_DISALLOW_COMMA) && priority == TOP_PRIORITY && QCC_PR_CheckToken (","))
	{
		QCC_PR_DiscardRef(lhsr);
		if (!qcc_usefulstatement)
			QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Statement does not do anything");
		qcc_usefulstatement = false;
		lhsr = QCC_PR_RefExpression(retbuf, TOP_PRIORITY, exprflags);
	}
	return lhsr;
}

QCC_sref_t QCC_PR_Expression (int priority, int exprflags)
{
	QCC_ref_t refbuf, *ret;
	ret = QCC_PR_RefExpression(&refbuf, priority, exprflags);
	return QCC_RefToDef(ret, true);
}
//parse the expression and discard the result. generate a warning if there were no assignments
//this avoids generating getter statements from RefToDef in QCC_PR_Expression.
static void QCC_PR_DiscardExpression (int priority, int exprflags)
{
	QCC_ref_t refbuf, *ref;
	pbool olduseful = qcc_usefulstatement;
	qcc_usefulstatement = false;

	ref = QCC_PR_RefExpression(&refbuf, priority, exprflags);
	QCC_PR_DiscardRef(ref);

	if (ref->cast->type != ev_void && !qcc_usefulstatement)
	{
//		int osl = pr_source_line;
//		pr_source_line = statementstart;
		QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Statement does not do anything");
//		pr_source_line = osl;
	}
	qcc_usefulstatement = olduseful;
}

long QCC_PR_IntConstExpr(void)
{
	//fixme: should make sure that no actual statements are generated
	QCC_sref_t def = QCC_PR_Expression(TOP_PRIORITY, 0);
	const QCC_eval_t *ev = QCC_SRef_EvalConst(def);
	if (ev)
	{
		QCC_FreeTemp(def);
		def.sym->referenced = true;
		switch(def.cast->type)
		{
		case ev_integer:
			return ev->_int;
		case ev_float:
			{
				int i = ev->_float;
				if ((float)i == ev->_float)
					return i;
			}
			break;
		case ev_double:
			{
				int i = ev->_double;
				if ((double)i == ev->_double)
					return i;
			}
			break;
		case ev_uint:
			return ev->_uint;
		case ev_int64:
			return ev->i64;
		case ev_uint64:
			return ev->u64;
		default:
			QCC_PR_ParseError(ERR_NOTACONSTANT, "Value is not an integer constant");
		}
	}
	QCC_PR_ParseError(ERR_NOTACONSTANT, "Value is not an integer constant");
	return true;
}

static void QCC_PR_GotoStatement (QCC_statement_t *patch2, char *labelname)
{
	if (num_gotos >= max_gotos)
	{
		max_gotos += 8;
		pr_gotos = realloc(pr_gotos, sizeof(*pr_gotos)*max_gotos);
	}

	if (!QC_strlcpy(pr_gotos[num_gotos].name, labelname, sizeof(pr_gotos[num_gotos].name)))
		QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Label name too long");
	pr_gotos[num_gotos].lineno = pr_source_line;
	pr_gotos[num_gotos].statementno = patch2 - statements;

	num_gotos++;
}

/*
static pbool QCC_PR_StatementBlocksMatch(QCC_statement_t *p1, int p1count, QCC_statement_t *p2, int p2count)
{
	if (p1count != p2count)
		return false;

	while(p1count>0)
	{
		if (p1->op != p2->op)
			return false;
		if (memcmp(&p1->a, &p2->a, sizeof(p1->a)))
			return false;
		if (memcmp(&p1->b, &p2->b, sizeof(p1->b)))
			return false;
		if (memcmp(&p1->c, &p2->c, sizeof(p1->c)))
			return false;
		p1++;
		p2++;
		p1count--;
	}

	return true;
}*/

//vanilla qc only has an OP_IFNOT_I, others will be emulated as required, so we tend to need to emulate other opcodes.
QCC_statement_t *QCC_Generate_OP_IF(QCC_sref_t e, pbool preserve)
{
	unsigned int flags = (preserve?STFL_PRESERVEA:0);
	QCC_statement_t *st;
	int op = 0;
	while (e.cast->type == ev_accessor)
		e.cast = e.cast->parentclass;

	switch(e.cast->type)
	{
	//int/pointer types
	case ev_entity:
	case ev_field:
	case ev_function:
	case ev_pointer:
	case ev_integer:
	case ev_uint:
	case ev_boolean:	//should be 0, 1i, or 1.0f, either way -0 isn't a problem so we can use the vanilla OP_IF_I for this
		op = OP_IF_I;
		break;

	//emulated types
	case ev_string:
		QCC_PR_ParseWarning(WARN_IFSTRING_USED, "if (string) tests for null, not empty.");
		if (flag_ifstring)
			op = OP_IF_S;
		else
			op = OP_IF_I;
		break;
	case ev_float:
		if (flag_iffloat || QCC_OPCodeValid(&pr_opcodes[OP_IF_F]))
			op = OP_IF_F;
		else
			op = OP_IF_I;
		break;
	case ev_vector:
		if (flag_ifvector)
		{
			e = QCC_PR_StatementFlags (&pr_opcodes[OP_NOT_V], e, nullsref, NULL, flags);
			op = OP_IFNOT_I;
		}
		else
			op = OP_IF_I;
		break;

	case ev_double:
		e = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_D], e, QCC_MakeDoubleConst(0), NULL, flags);
		op = OP_IF_I;
		break;
	case ev_int64:
	case ev_uint64:
		e = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_I64], e, QCC_MakeInt64Const(0), NULL, flags);
		op = OP_IF_I;
		break;

	case ev_variant:
	case ev_struct:
	case ev_union:
	case ev_void:
	default:
		QCC_PR_ParseWarning(WARN_CONDITIONALTYPEMISMATCH, "conditional type mismatch: %s", basictypenames[e.cast->type]);
		op = OP_IF_I;
		break;
	}

	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &st, flags));
	return st;
}
QCC_statement_t *QCC_Generate_OP_IFNOT(QCC_sref_t e, pbool preserve)
{
	unsigned int flags = (preserve?STFL_PRESERVEA:0);
	QCC_statement_t *st;
	int op = 0;

	if (!e.cast)
		e.cast = type_void;
	while (e.cast->type == ev_accessor)
		e.cast = e.cast->parentclass;
	switch(e.cast->type)
	{
	//int/pointer types
	case ev_entity:
	case ev_field:
	case ev_function:
	case ev_pointer:
	case ev_integer:
	case ev_uint:
	case ev_boolean:	//should be 0, 1i, or 1.0f, either way -0 isn't a problem so we can use the vanilla OP_IFNOT_I for this
		op = OP_IFNOT_I;
		break;

	//emulated types
	case ev_string:
		QCC_PR_ParseWarning(WARN_IFSTRING_USED, "if (string) tests for null, not empty");
		if (flag_ifstring)
			op = OP_IFNOT_S;
		else
			op = OP_IFNOT_I;
		break;
	case ev_float:
		if (flag_iffloat || QCC_OPCodeValid(&pr_opcodes[OP_IFNOT_F]))
			op = OP_IFNOT_F;
		else
			op = OP_IFNOT_I;
		break;
	case ev_vector:
		if (flag_ifvector)
		{
			e = QCC_PR_StatementFlags (&pr_opcodes[OP_NOT_V], e, nullsref, NULL, flags);
			op = OP_IF_I;
		}
		else
		{
			QCC_PR_ParseWarning(WARN_IFVECTOR_DISABLED, "if (vector) tests only the first element with the current compiler flags");
			op = OP_IFNOT_I;
		}
		break;

	case ev_double:
		e = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_D], e, QCC_MakeDoubleConst(0), NULL, flags);
		op = OP_IFNOT_I;
		break;
	case ev_int64:
	case ev_uint64:
		e = QCC_PR_StatementFlags (&pr_opcodes[OP_NE_I64], e, QCC_MakeInt64Const(0), NULL, flags);
		op = OP_IFNOT_I;
		break;

	case ev_variant:
	case ev_struct:
	case ev_union:
	case ev_void:
	default:
		QCC_PR_ParseWarning(WARN_CONDITIONALTYPEMISMATCH, "conditional type mismatch: %s", basictypenames[e.cast->type]);
		op = OP_IFNOT_I;
		break;
	}

	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &st, flags));
	return st;
}

//for consistancy with if+ifnot
QCC_statement_t *QCC_Generate_OP_GOTO(void)
{
	QCC_statement_t *st;
	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_GOTO], nullsref, nullsref, &st, STFL_DISCARDRESULT));
	return st;
}

//called for return statements
static void PR_GenerateReturnOuts(void)
{
	unsigned int i;
	QCC_sref_t p;
	QCC_def_t *local;
	int parm;

	for (i = 0, parm = 0, local = pr.local_head.nextlocal; i < pr_scope->type->num_parms; i++)
	{
		if (pr_scope->type->params[i].out)
		{
			if (parm > MAX_PARMS)
				p = extra_parms[parm-MAX_PARMS];
			else
			{
				p.sym = &def_parms[parm];
				p.ofs = 0;
				p.cast = type_vector;
			}
			QCC_ForceUnFreeDef(p.sym);
			QCC_StoreToSRef(p, QCC_MakeSRefForce(local, 0, local->type), local->type, false, false);
		}
		parm += (pr_scope->type->params[i].type->size+2)/3;
		local = local->deftail->nextlocal;
	}
}

static void QCC_PR_ParseStatement_Using(void)
{	//the 'using(...) {}' control statement exists mainly to mute warnings about deprecations within its block.
	//it does this by basically creating a private alias for each name given in the parenthasis (new=orig to give the alias a different name).
	QCC_def_t	*d;

	QCC_def_t *subscopestop;
	QCC_def_t *subscopestart = pr.local_tail;
	//QCC_type_t *lt = NULL, *type;
	pbool block = QCC_PR_CheckToken("(");
	do
	{
		/*type = QCC_PR_ParseType (false, true);
		if (type)
		{
			d = QCC_PR_GetDef (type, QCC_PR_ParseName(), pr_scope, true, 0, 0);
			if (QCC_PR_CheckToken("="))
				QCC_PR_ParseInitializerDef(d, 0);
			QCC_FreeDef(d);
			lt = type;
		}
		else*/
		{	//define an alias of the variable with the same name
			const char *name = QCC_PR_ParseName();

			QCC_def_t *def = QCC_PR_GetDef (NULL, name, pr_scope, false, 0, 0);
			if (!def)
				QCC_PR_ParseError(ERR_NOTDEFINED, "%s is not defined", name);
			def->referenced = true;
			if (QCC_PR_CheckToken("="))
				name = QCC_PR_ParseName();	//they wanted a different name for it.
			if (def->deprecated)
			{
				if (*def->deprecated)	//we have a reason for it
					QCC_PR_ParseWarning(WARN_MUTEDEPRECATEDVARIABLE, "Variable \"%s\" is deprecated: %s", def->name, def->deprecated);
				else		//we don't have any reason for it.
					QCC_PR_ParseWarning(WARN_MUTEDEPRECATEDVARIABLE, "Variable \"%s\" is deprecated", def->name);
			}
			def = QCC_PR_DummyDef(def->type, name, pr_scope, def->arraysize, def, 0, true, GDF_STRIP|GDF_ALIAS);
		}
	} while(QCC_PR_CheckToken(","));

	pr_assumetermtype = NULL;
	if (block)
		QCC_PR_Expect(")");
	else
	{	//applies to whole rest of scope...
		QCC_PR_Expect(";");
		return;
	}

	subscopestop = pr_subscopedlocals?NULL:pr.local_tail->nextlocal;

	QCC_PR_ParseStatement();	//don't give the hanging ';' warning.

	//remove any new locals from the hashtable.
	//typically this is just the stuff inside the for(here;;)
	for (d = subscopestart->nextlocal; d != subscopestop; d = d->nextlocal)
	{
		if (!d->subscoped_away)
		{
			pHash_RemoveData(&localstable, d->name, d);
			d->subscoped_away = true;
		}
	}

	return;
}

/*
============
QCC_PR_ParseStatement_For

pulled out of QCC_PR_ParseStatement because of stack use.
============
*/
static void QCC_PR_ParseStatement_For(void)
{
	int continues;
	int breaks;
	int i;
	QCC_sref_t				e;
	QCC_def_t	*d;
	QCC_statement_t		*patch1, *patch2, *patch3, *patch4;


	int old_numstatements;
	int numtemp;
	QCC_def_t *subscopestop;
	QCC_def_t *subscopestart = pr.local_tail;

	QCC_statement_t		temp[256];

	continues = num_continues;
	breaks = num_breaks;

	QCC_PR_Expect("(");
	if (!QCC_PR_CheckToken(";"))
	{
		QCC_type_t *lt = NULL, *type;
		do
		{
			type = QCC_PR_ParseType (false, true, false);
			if (type)
			{
				d = QCC_PR_GetDef (type, QCC_PR_ParseName(), pr_scope, true, 0, 0);
				if (QCC_PR_CheckToken("="))
					QCC_PR_ParseInitializerDef(d, 0);
				QCC_FreeDef(d);
				lt = type;
			}
			else
			{
				pr_assumetermtype = lt;
				pr_assumetermscope = pr_scope;
				pr_assumetermflags = 0;
				QCC_PR_DiscardExpression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			}
		} while(QCC_PR_CheckToken(","));
		pr_assumetermtype = NULL;
		QCC_PR_Expect(";");
	}
	subscopestop = pr_subscopedlocals?NULL:pr.local_tail->nextlocal;

	QCC_ClobberDef(NULL);

	patch2 = &statements[numstatements];	//restart of the loop
	if (!QCC_PR_CheckToken(";"))
	{
		conditional = 1;
		e = QCC_PR_Expression(TOP_PRIORITY, 0);
		conditional = 0;
		QCC_PR_Expect(";");
	}
	else
		e = nullsref;

	if (e.cast)	//final condition+jump
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_IFNOT_I], e, nullsref, &patch1, STFL_DISCARDRESULT));
	else
		patch1 = NULL;

	if (!QCC_PR_CheckToken(")"))
	{
		old_numstatements = numstatements;
		QCC_PR_DiscardExpression(TOP_PRIORITY, 0);

		numtemp = numstatements - old_numstatements;
		if (numtemp > sizeof(temp)/sizeof(temp[0]))
			QCC_PR_ParseError(ERR_TOOCOMPLEX, "Update expression too large");
		numstatements = old_numstatements;
		for (i = 0 ; i < numtemp ; i++)
		{
			temp[i] = statements[numstatements + i];
		}

		QCC_PR_Expect(")");
	}
	else
		numtemp = 0;

	//parse the statement block
	if (!QCC_PR_CheckToken(";"))
		QCC_PR_ParseStatement();	//don't give the hanging ';' warning.
	patch3 = &statements[numstatements];	//location for continues
	//reinsert the 'increment' statements. lets hope they didn't have any gotos...
	for (i = 0 ; i < numtemp ; i++)
	{
		statements[numstatements] = temp[i];
		statements[numstatements].linenum = pr_token_line_last;
		numstatements++;
	}
	patch4 = QCC_Generate_OP_GOTO();
	patch4->a.jumpofs = patch2 - patch4;
	if (patch1)
		patch1->b.jumpofs = &statements[numstatements] - patch1;	//condition failure jumps here

	//fix up breaks+continues
	if (breaks != num_breaks)
	{
		for(i = breaks; i < num_breaks; i++)
		{
			patch1 = &statements[pr_breaks[i]];
			statements[pr_breaks[i]].a.jumpofs = &statements[numstatements] - patch1;
		}
		num_breaks = breaks;
	}
	if (continues != num_continues)
	{
		for(i = continues; i < num_continues; i++)
		{
			patch1 = &statements[pr_continues[i]];
			statements[pr_continues[i]].a.jumpofs = patch3 - patch1;
		}
		num_continues = continues;
	}


	//remove any new locals from the hashtable.
	//typically this is just the stuff inside the for(here;;)
	for (d = subscopestart->nextlocal; d != subscopestop; d = d->nextlocal)
	{
		if (!d->subscoped_away)
		{
			pHash_RemoveData(&localstable, d->name, d);
			d->subscoped_away = true;
		}
	}

	return;
}
/*
============
PR_ParseStatement

============
*/
void QCC_PR_ParseStatement (void)
{
	int continues;
	int breaks;
	int cases;
	int i;
	QCC_sref_t				e, e2;
	QCC_def_t	*d;
	QCC_statement_t		*patch1, *patch2, *patch3;
	int statementstart = pr_source_line;
	pbool wasuntil;

	QCC_ClobberDef(NULL);	//make sure any conditionals don't weird out.

	if (QCC_PR_CheckToken ("{"))
	{
		int startingtypes = numtypeinfos;
		d = pr.local_tail;
		while (!QCC_PR_CheckToken("}"))
			QCC_PR_ParseStatement ();

		if (pr_subscopedlocals)
		{	//remove any new locals from the hashtable.
			for (d = d->nextlocal; d; d = d->nextlocal)
			{
				if (!d->subscoped_away)
				{
					pHash_RemoveData(&localstable, d->name, d);
					d->subscoped_away = true;
				}
			}
		}
		for (; startingtypes < numtypeinfos; startingtypes++)
		{
			if (qcc_typeinfo[startingtypes].typedefed)
			{
				qcc_typeinfo[startingtypes].typedefed = false;
				pHash_RemoveData(&typedeftable, qcc_typeinfo[startingtypes].name, &qcc_typeinfo[startingtypes]);
			}
		}
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_return, "return"))
	{
		/*
		accumulate behaviour requires the ability to just run code without explicit returns.
		return = foo; sets the value that will be returned when the function finally exits, without returning now.
		return; returns that value now, without execing later accumulations.
		return 5; also returns now.
		*/

		if (QCC_PR_CheckToken (";"))
		{
			PR_GenerateReturnOuts();
			if (pr_scope->type->aux_type->type != ev_void)
			{	//accumulated functions are not required to return anything, on the assumption that a previous 'part' of the function did so
				if (pr_scope->type->aux_type->size > type_vector->size)
					QCC_PR_ParseError(ERR_BADEXTENSION, "\'%s\' returned nothing, expected %s", pr_scope->name, pr_scope->type->aux_type->name);	//just make it fatal. too lazy to handle it
				else if ((!pr_scope->def || !pr_scope->def->accumulate) && !pr_scope->returndef.cast)
					QCC_PR_ParseWarning(WARN_MISSINGRETURNVALUE, "\'%s\' returned nothing, expected %s", pr_scope->name, pr_scope->type->aux_type->name);
				//this should not normally happen
				if (!pr_scope->returndef.cast)
				{	//but if it does, allocate a local that can be return=foo; before the return. depend upon qc's null initialisation rules for the default value.
					pr_scope->returndef = QCC_PR_GetSRef(pr_scope->type->aux_type, "ret*", pr_scope, true, 0, GDF_NONE);
					QCC_FreeTemp(pr_scope->returndef);
				}
			}
			if (pr_scope->returndef.cast)
			{
				QCC_ForceUnFreeDef(pr_scope->returndef.sym);
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], pr_scope->returndef, nullsref, NULL));
				return;
			}
//			if (opt_return_only)
//				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));
//			else
			if (pr_scope->type->aux_type->type == ev_vector)	//make sure bad returns don't return junk in the y+z members.
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], QCC_MakeVectorConst(0,0,0), nullsref, NULL));
			else
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], nullsref, nullsref, NULL));
			return;
		}

		if (QCC_PR_CheckToken ("="))
		{
			QCC_ref_t r;
			if (pr_scope->type->aux_type->size > type_vector->size)
				QCC_PR_ParseError(ERR_BADEXTENSION, "\'%s\' not supported when returning %s", pr_scope->name, pr_scope->type->aux_type->name);	//just make it fatal. too lazy to handle it
			if (!pr_scope->returndef.cast)
				pr_scope->returndef = QCC_PR_GetSRef(pr_scope->type->aux_type, "ret*", pr_scope, true, 0, GDF_NONE);
			else
				QCC_ForceUnFreeDef(pr_scope->returndef.sym);

			e = QCC_PR_Expression(TOP_PRIORITY, 0);
			QCC_PR_Expect (";");

			QCC_StoreSRefToRef(QCC_PR_BuildRef(&r, REF_GLOBAL, pr_scope->returndef, nullsref, pr_scope->type->aux_type, false, 0), e, false, false);
			return;
		}

		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		QCC_PR_Expect (";");
		if (QCC_SRef_IsNull(e))
		{
			QCC_FreeTemp(e);
			//return __NULL__; is allowed regardless of actual return type.
			switch (pr_scope->type->aux_type->type)
			{
			case ev_vector:
				e = QCC_MakeVectorConst(0, 0, 0);
				break;
			default:
			case ev_float:
				e = QCC_MakeFloatConst(0);
				break;
			}
			e.cast = pr_scope->type->aux_type;
		}
		else if (pr_scope->type->aux_type->type != e.cast->type)
		{
			if (pr_scope->type->aux_type->type == ev_void)
			{	//returning a value inside a function defined to return void is bad dude.
				QCC_PR_ParseWarning(WARN_WRONGRETURNTYPE, "\'%s\' returned %s, expected %s", pr_scope->name, e.sym->type->name, pr_scope->type->aux_type->name);
				e = QCC_EvaluateCast(e, type_variant, true);
			}
			else
				e = QCC_EvaluateCast(e, pr_scope->type->aux_type, true);
		}
		PR_GenerateReturnOuts();
		if (pr_scope->type->aux_type->size > type_vector->size)
		{
			QCC_StoreToPointer(pr_scope->returndef, nullsref, e, pr_scope->type->aux_type);
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], pr_scope->returndef, nullsref, NULL));	//standard return has no real meaning, might as well return the address though even though we'll treat it as void.
		}
		else
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], e, nullsref, NULL));
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_exit, "exit"))
	{
		PR_GenerateReturnOuts();
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));
		QCC_PR_Expect (";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_loop, "loop"))
	{
		continues = num_continues;
		breaks = num_breaks;

		patch2 = &statements[numstatements];
		QCC_PR_ParseStatement ();
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch3));
		patch3->a.jumpofs = patch2 - patch3;

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch1 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.jumpofs = &statements[numstatements] - patch1;	//jump to after the return-to-top goto
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch1 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.jumpofs = patch2 - patch1;	//jump back to top
			}
			num_continues = continues;
		}
		return;
	}

	wasuntil = QCC_PR_CheckKeyword(keyword_until, "until");
	if (wasuntil || QCC_PR_CheckKeyword(keyword_while, "while"))
	{
		const QCC_eval_t *eval;
		continues = num_continues;
		breaks = num_breaks;

		QCC_PR_Expect ("(");
		patch2 = &statements[numstatements];
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		eval = QCC_SRef_EvalConst(e);
		if (eval && /*opt_compound_jumps &&*/ e.cast->type == ev_float)
		{
			//optres_compound_jumps++;
			QCC_FreeTemp(e);
			if ((!eval->_float) != wasuntil)
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch1));
			else
				patch1 = NULL;
		}
		else if (wasuntil)
			patch1 = QCC_Generate_OP_IF(e, false);
		else
			patch1 = QCC_Generate_OP_IFNOT(e, false);
		QCC_PR_Expect (")");	//after the line number is noted..
		QCC_PR_ParseStatement ();
		patch3 = QCC_Generate_OP_GOTO();
		patch3->a.jumpofs = patch2 - patch3;
		if (patch1)
		{
			if (patch1->op == OP_GOTO)
				patch1->a.jumpofs = &statements[numstatements] - patch1;
			else
				patch1->b.jumpofs = &statements[numstatements] - patch1;
		}

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch1 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.jumpofs = &statements[numstatements] - patch1;	//jump to after the return-to-top goto
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch1 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.jumpofs = patch2 - patch1;	//jump back to top
			}
			num_continues = continues;
		}
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_for, "for"))
	{
		QCC_PR_ParseStatement_For();
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_do, "do"))
	{
		const QCC_eval_t *eval;
		pbool until;
		continues = num_continues;
		breaks = num_breaks;

		patch1 = &statements[numstatements];
		QCC_PR_ParseStatement ();
		until = QCC_PR_CheckKeyword(keyword_until, "until");
		if (!until)
			QCC_PR_Expect ("while");
		QCC_PR_Expect ("(");
		patch3 = &statements[numstatements];
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		eval = QCC_SRef_EvalConst(e);
		if (eval)
		{
			if (until?!eval->_int:eval->_int)
			{
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch2));
				patch2->a.jumpofs = patch1 - patch2;
			}
			QCC_FreeTemp(e);
		}
		else
		{
			if (until)
				patch2 = QCC_Generate_OP_IFNOT(e, false);
			else
				patch2 = QCC_Generate_OP_IF(e, false);
			patch2->b.jumpofs = patch1 - patch2;
		}

		QCC_PR_Expect (")");
		QCC_PR_Expect (";");

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch2 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.jumpofs = &statements[numstatements] - patch2;
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{	//continue in do{}while(cond); jumps to the while, not the do.
			for(i = continues; i < num_continues; i++)
			{
				patch2 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.jumpofs = patch3 - patch2;
			}
			num_continues = continues;
		}

		return;
	}

	if (QCC_PR_CheckKeyword(keyword_local, "local"))
	{
//		if (locals_end != numpr_globals)	//is this breaking because of locals?
//			QCC_PR_ParseWarning("local vars after temp vars\n");
		QCC_PR_ParseDefs (NULL, true);
		return;
	}

	if (pr_token_type == tt_name)
	{
		QCC_type_t *type = QCC_TypeForName(pr_token);
		if (type)
		{
			if (strncmp(pr_file_p, "::", 2))
			{
				QCC_PR_ParseDefs (NULL, false);
				return;
			}
		}

		if ((keyword_var && !STRCMP ("var", pr_token)) ||
			(keyword_noref && !STRCMP ("noref", pr_token)) ||
			(keyword_string && !STRCMP ("string", pr_token)) ||
			(keyword_float && !STRCMP ("float", pr_token)) ||
			(keyword_double && !STRCMP ("double", pr_token)) ||
			(keyword_entity && !STRCMP ("entity", pr_token)) ||
			(keyword_vector && !STRCMP ("vector", pr_token)) ||
			(keyword_integer && !STRCMP ("integer", pr_token)) ||
			(keyword_unsigned && !STRCMP ("unsigned", pr_token)) ||
			(keyword_signed && !STRCMP ("signed", pr_token)) ||
			(keyword_long && !STRCMP ("long", pr_token)) ||
			(keyword_int && !STRCMP ("int", pr_token)) ||
			(keyword_short && !STRCMP ("short", pr_token)) ||
			(keyword_char && !STRCMP ("char", pr_token)) ||
			(				!STRCMP ("_Bool", pr_token)) ||
			(keyword_register && !STRCMP ("register", pr_token)) ||
			(keyword_volatile && !STRCMP ("volatile", pr_token)) ||
			(keyword_static && !STRCMP ("static", pr_token)) ||
			(keyword_class && !STRCMP ("class", pr_token)) ||
			(keyword_struct && !STRCMP ("struct", pr_token)) ||
			(keyword_union && !STRCMP ("union", pr_token)) ||
			(keyword_enum && !STRCMP ("enum", pr_token)) ||
			(keyword_extern && !STRCMP ("extern", pr_token)) ||
			(keyword_auto && !STRCMP ("auto", pr_token)) ||
			(keyword_typedef && !STRCMP ("typedef", pr_token)) ||
			(keyword_const && !STRCMP ("const", pr_token)))
		{
			QCC_PR_ParseDefs (NULL, true);
			return;
		}
	}
	if (pr_token_type == tt_punct && QCC_PR_PeekToken ("."))
	{	//for local .entity without var/local
		QCC_PR_ParseDefs (NULL, true);
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_state, "state"))
	{
		QCC_PR_Expect("[");
		QCC_PR_ParseState();
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckToken("#"))
	{
		char *name;
		float frame = pr_immediate._float;
		QCC_PR_Lex();
		name = QCC_PR_ParseName();
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STATE], QCC_MakeFloatConst(frame), QCC_PR_GetSRef(type_function, name, NULL, false, 0, false), NULL));
		QCC_PR_Expect(";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_if, "if"))
	{
		unsigned int oldnumst, oldlab;
		pbool striptruth = false;
		pbool stripfalse = false;
		const QCC_eval_t *eval;
		int negate = QCC_PR_CheckKeyword(keyword_not, "not")/*hexenc*/;
		if (!negate && QCC_PR_CheckToken("!"))
		{
			QCC_PR_ParseWarning (WARN_FTE_SPECIFIC, "if !() is specific to fteqcc");
			negate = 2;
		}
		else if (negate && qcc_targetformat != QCF_HEXEN2 && qcc_targetformat != QCF_UHEXEN2 && qcc_targetformat != QCF_FTEH2)
			QCC_PR_ParseWarning (WARN_FTE_SPECIFIC, "if not() is specific to fteqcc or hexen2");

		QCC_PR_Expect ("(");
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		if (negate == 2)
		{
			if (e.cast->type == ev_string/*deal with empty properly*/
				|| e.cast->type == ev_float/*deal with -0.0*/
				|| e.cast->type == ev_accessor/*its complicated*/ ) 
			{
				e = QCC_PR_GenerateLogicalNot(e, "Type mismatch: !%s");
				negate = 0;
			}
		}

		eval = QCC_SRef_EvalConst(e);

//		negate = negate != 0;

		oldnumst = numstatements;
		if (eval)
		{
			if (e.cast->type == ev_float)
				striptruth = eval->_float == 0;
			else
				striptruth = eval->_int == 0;
			if (negate)
				striptruth = !striptruth;
			stripfalse = !striptruth;
			patch1 = NULL;
			QCC_FreeTemp(e);

			if (striptruth)
				patch1 = QCC_Generate_OP_GOTO();
		}
		else if (negate)
		{
			patch1 = QCC_Generate_OP_IF(e, false);
		}
		else
		{
			patch1 = QCC_Generate_OP_IFNOT(e, false);
		}

		QCC_PR_Expect (")");	//close bracket is after we save the statement to mem (so debugger does not show the if statement as being on the line after

		oldlab = num_labels;
		QCC_PR_ParseStatement ();
		if (striptruth && oldlab == num_labels)
		{
			QCC_UngenerateStatements(oldnumst);
			patch1 = NULL;
		}
		else
			striptruth = false;

		if (QCC_PR_CheckKeyword (keyword_else, "else"))
		{
			int lastwasreturn;
			lastwasreturn = statements[numstatements-1].op == OP_RETURN || statements[numstatements-1].op == OP_DONE ||
				statements[numstatements-1].op == OP_GOTO;

			//the last statement of the if was a return, so we don't need the goto at the end
			if (lastwasreturn && opt_compound_jumps && patch1 && !QCC_AStatementJumpsTo(numstatements, patch1-statements, numstatements))
			{
//				QCC_PR_ParseWarning(0, "optimised the else");
				optres_compound_jumps++;
				if (patch1)
					patch1->b.jumpofs = &statements[numstatements] - patch1;
				oldnumst = numstatements;
				oldlab = num_labels;
				QCC_PR_ParseStatement ();
				if (stripfalse && oldlab == num_labels)
				{
					patch2 = NULL;
					QCC_UngenerateStatements(oldnumst);

					if (patch1)
						patch1->b.jumpofs = &statements[numstatements] - patch1;
				}
			}
			else
			{
//				QCC_PR_ParseWarning(0, "using the else");
				oldnumst = numstatements;
				if (striptruth)
					patch2 = NULL;
				else
					patch2 = QCC_Generate_OP_GOTO();
				if (patch1)
					patch1->b.jumpofs = &statements[numstatements] - patch1;

				oldlab = num_labels;
				QCC_PR_ParseStatement ();
				if (stripfalse && oldlab == num_labels)
				{
					patch2 = NULL;
					QCC_UngenerateStatements(oldnumst);

					if (patch1)
						patch1->b.jumpofs = &statements[numstatements] - patch1;
				}

				if (patch2)
					patch2->a.jumpofs = &statements[numstatements] - patch2;

/*FIXME: this doesn't work right
				if (patch1 && patch2)
				{
					if (QCC_PR_StatementBlocksMatch(patch1+1, patch2-(patch1+1), patch2+1, &statements[numstatements] - (patch2+1)))
						QCC_PR_ParseWarning(0, "Two identical blocks each side of an else");
				}
*/
			}
		}
		else if (patch1)
		{
			if (patch1->op == OP_GOTO)
				patch1->a.jumpofs = &statements[numstatements] - patch1;
			else
				patch1->b.jumpofs = &statements[numstatements] - patch1;
		}

		return;
	}
	if (QCC_PR_CheckKeyword(keyword_switch, "switch"))
	{
		int op;
		int hcstyle;
		int defaultcase = -1;
		int oldst;
		QCC_type_t *switchtype;
		struct accessor_s *acc;

		breaks = num_breaks;
		cases = num_cases;


		QCC_PR_Expect ("(");

		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;
		e.sym->referenced = true;

		//expands

		//switch (CONDITION)
		//{
		//case 1:
		//	break;
		//case 2:
		//default:
		//	break;
		//}

		//to

		// x = CONDITION, goto start
		// l1:
		//	goto end
		// l2:
		// def:
		//	goto end
		//	goto end			P1
		// start:
		//	if (x == 1) goto l1;
		//	if (x == 2) goto l2;
		//	goto def
		// end:

		//x is emitted in an opcode, stored as a register that we cannot access later.
		//it should be possible to nest these.

		switchtype = e.cast;
		switch(switchtype->type==ev_enum?switchtype->aux_type->type:switchtype->type)
		{
		case ev_float:
			op = OP_SWITCH_F;
			break;
		case ev_entity:	//whu???
			op = OP_SWITCH_E;
			break;
		case ev_vector:
			op = OP_SWITCH_V;
			break;
		case ev_string:
			op = OP_SWITCH_S;
			break;
		case ev_function:
			op = OP_SWITCH_FNC;
			break;
		default:	//err hmm.
			op = 0;
			break;
		}

		if (op)
			hcstyle = QCC_OPCodeValid(&pr_opcodes[op]);
		else
			hcstyle = false;

		QCC_ClobberDef(NULL);

		if (hcstyle)
			QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &patch1, STFL_DISCARDRESULT));
		else
		{
			patch1 = QCC_Generate_OP_GOTO();	//fixme: rearrange this, to avoid the goto
			QCC_FreeTemp(e);
		}

		QCC_PR_Expect (")");	//close bracket is after we save the statement to mem (so debugger does not show the if statement as being on the line after

		oldst = numstatements;
		QCC_PR_ParseStatement ();

		//this is so that a missing goto at the end of your switch doesn't end up in the jumptable again
		if (oldst == numstatements || !QCC_StatementIsAJump(numstatements-1, numstatements-1) || QCC_AStatementJumpsTo(numstatements, pr_scope->code, numstatements))
		{
			patch2 = QCC_Generate_OP_GOTO();	//the P1 statement/the theyforgotthebreak statement.
//			QCC_PR_ParseWarning(0, "emitted goto");
		}
		else
		{
			patch2 = NULL;
//			QCC_PR_ParseWarning(0, "No goto");
		}

		if (hcstyle)
			patch1->b.jumpofs = &statements[numstatements] - patch1;	//the goto start part
		else
			patch1->a.jumpofs = &statements[numstatements] - patch1;	//the goto start part

		oldst = numstatements;

		for (acc = switchtype->accessors; acc; acc = acc->next)
		{
			const QCC_eval_t *match = QCC_SRef_EvalConst(acc->staticval);
			if (!match)
				continue;	//not an enum value, ignore it.
			for (i = cases; i < num_cases; i++)
			{
				if (!pr_casesref[i].cast)
					break;	//its a default
				if (pr_casesref2[i].cast)
				{	//caserange
					break;	//FIXME: too lazy to check these.
				}
				else
				{	//case
					if (pr_casesref[i].type == REF_GLOBAL && pr_casesref[i].cast == switchtype)
					{
						const QCC_eval_t *eval = QCC_SRef_EvalConst(pr_casesref[i].base);
						if (!eval)
							break;	//can't verify it
						if (!memcmp(eval, match, sizeof(*eval)*switchtype->size))
							break;	//validated.
					}
					else
						break;	//can't verify it
				}
			}
			if (i == num_cases)
			{
				QCC_PR_ParseWarning(0, "%s::%s not part of switch", switchtype->name, acc->fieldname);
			}
		}

		QCC_ForceUnFreeDef(e.sym);	//in the following code, e should still be live
		for (i = cases; i < num_cases; i++)
		{
			if (!pr_casesref[i].cast)
			{
				if (defaultcase >= 0)
					QCC_PR_ParseError(ERR_MULTIPLEDEFAULTS, "Duplicated default case");
				defaultcase = i;
			}
			else
			{
				QCC_sref_t dmin, dmax;
				dmin = QCC_RefToDef(&pr_casesref[i], true);
				if (dmin.cast->type != e.cast->type)
					dmin = QCC_SupplyConversion(dmin, e.cast->type, true);
				if (pr_casesref2[i].cast)
				{
					dmax = QCC_RefToDef(&pr_casesref2[i], true);
					if (dmax.cast->type != e.cast->type)
						dmax = QCC_SupplyConversion(dmax, e.cast->type, true);

					if (hcstyle)
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CASERANGE], dmin, dmax, &patch3));
						patch3->c.jumpofs = &statements[pr_cases[i]] - patch3;
					}
					else
					{
						QCC_sref_t e3;

						if (e.cast->type == ev_float)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_F], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_F], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_F], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_double)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_D], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_D], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_F], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_integer)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_I], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_I], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_I], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_uint)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_U], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_U], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_I], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_int64)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_I64], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_I64], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_I], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_uint64)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_U64], e, dmin, NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_U64], e, dmax, NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_I], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
						}
						else
							QCC_PR_ParseWarning(WARN_SWITCHTYPEMISMATCH, "switch caserange MUST be a float or integer");
					}
				}
				else
				{
					if (hcstyle)
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CASE], dmin, nullsref, &patch3));
						patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
					}
					else
					{
						const QCC_eval_t *eval = QCC_SRef_EvalConst(dmin);
						if (!eval || eval->_int)
						{
							switch(e.cast->type==ev_enum?e.cast->aux_type->type:e.cast->type)
							{
							case ev_float:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_F], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_entity:	//whu???
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_E], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_vector:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_V], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_string:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_S], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_function:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_FNC], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_field:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_FNC], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_pointer:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_P], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_integer:
							case ev_uint:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_I], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_int64:
							case ev_uint64:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_I64], e, dmin, NULL, STFL_PRESERVEA);
								break;
							case ev_double:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_D], e, dmin, NULL, STFL_PRESERVEA);
								break;
							default:
								QCC_PR_ParseError(ERR_BADSWITCHTYPE, "Bad switch type");
								e2 = nullsref;
								break;
							}
							QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_IF_I], e2, nullsref, &patch3));
						}
						else
						{
							QCC_FreeTemp(dmin);
							QCC_UnFreeTemp(e);
							patch3 = QCC_Generate_OP_IFNOT(e, false);
						}
						patch3->b.jumpofs = &statements[pr_cases[i]] - patch3;
					}
				}
			}
		}
		QCC_FreeTemp(e);
		if (defaultcase>=0)
		{
			patch3 = QCC_Generate_OP_GOTO();
			patch3->a.jumpofs = &statements[pr_cases[defaultcase]] - patch3;
		}

		num_cases = cases;


		patch3 = &statements[numstatements];
		if (patch2)
			patch2->a.jumpofs = patch3 - patch2;	//set P1 jump

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch2 = &statements[pr_breaks[i]];
				patch2->a.jumpofs = patch3 - patch2;
			}
			num_breaks = breaks;
		}

		//update the jumptable statements to hide as part of the switch itself.
		while (oldst < numstatements)
			statements[oldst++].linenum = patch1->linenum;
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_using, "using"))
	{
		QCC_PR_ParseStatement_Using();
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_asm, "asm"))
	{
		if (QCC_PR_CheckToken("{"))
		{
			while (!QCC_PR_CheckToken("}"))
				QCC_PR_ParseAsm ();
		}
		else
			QCC_PR_ParseAsm ();
		return;
	}

	//frikqcc-style labels
	if (QCC_PR_CheckToken(":"))
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_BADLABELNAME, "invalid label name \"%s\"", pr_token);
			return;
		}

		for (i = 0; i < num_labels; i++)
			if (!STRNCMP(pr_labels[i].name, pr_token, sizeof(pr_labels[num_labels].name) -1))
			{
				QCC_PR_ParseWarning(WARN_DUPLICATELABEL, "Duplicate label %s", pr_token);
				QCC_PR_Lex();
				return;
			}

		if (num_labels >= max_labels)
		{
			max_labels += 8;
			pr_labels = realloc(pr_labels, sizeof(*pr_labels)*max_labels);
		}

		QC_strlcpy(pr_labels[num_labels].name, pr_token, sizeof(pr_labels[num_labels].name));
		pr_labels[num_labels].lineno = pr_source_line;
		pr_labels[num_labels].statementno = numstatements;

		num_labels++;

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_goto, "goto"))
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_NOLABEL, "invalid label name \"%s\"", pr_token);
			return;
		}

		patch2 = QCC_Generate_OP_GOTO();

		QCC_PR_GotoStatement (patch2, pr_token);

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		QCC_PR_Expect(";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_break, "break"))
	{
		if (!STRCMP ("(", pr_token))
		{	//make sure it wasn't a call to the break function.
			QCC_PR_IncludeChunk("break(", true, NULL);
			QCC_PR_Lex();	//so it sees the break.
		}
		else
		{
			if (num_breaks >= max_breaks)
			{
				max_breaks += 8;
				pr_breaks = realloc(pr_breaks, sizeof(*pr_breaks)*max_breaks);
			}
			pr_breaks[num_breaks] = numstatements;
			QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, NULL);
			num_breaks++;
			QCC_PR_Expect(";");
			return;
		}
	}
	if (QCC_PR_CheckKeyword(keyword_continue, "continue"))
	{
		if (num_continues >= max_continues)
		{
			max_continues += 8;
			pr_continues = realloc(pr_continues, sizeof(*pr_continues)*max_continues);
		}
		pr_continues[num_continues] = numstatements;
		QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, NULL);
		num_continues++;
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_case, "case"))
	{
		if (num_cases >= max_cases)
		{
			max_cases += 8;
			pr_cases = realloc(pr_cases, sizeof(*pr_cases)*max_cases);
			pr_casesref = realloc(pr_casesref, sizeof(*pr_casesref)*max_cases);
			pr_casesref2 = realloc(pr_casesref2, sizeof(*pr_casesref2)*max_cases);
		}
		pr_cases[num_cases] = numstatements;
		QCC_PR_RefExpression(&pr_casesref[num_cases], TOP_PRIORITY, EXPR_DISALLOW_COMMA);
		if (QCC_PR_CheckToken(".."))
		{
			//const QCC_eval_t *evalmin, *evalmax;
			QCC_PR_RefExpression (&pr_casesref2[num_cases], TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			//pr_casesref2[num_cases] = QCC_SupplyConversion(pr_casesdef2[num_cases], pr_casesdef[num_cases].cast->type, true);

			/*evalmin = QCC_Ref_EvalConst(pr_casesref[num_cases]);
			evalmax = QCC_Ref_EvalConst(pr_casesref2[num_cases]);
			if (evalmin && evalmax)
			{
				if ((pr_casesref[num_cases].cast->type == ev_integer && evalmin->_int >= evalmax->_int) ||
					(pr_casesref[num_cases].cast->type == ev_float && evalmin->_float >= evalmax->_float))
					QCC_PR_ParseError(ERR_CASENOTIMMEDIATE, "Caserange statement uses backwards range\n");
			}*/
		}
		else
			pr_casesref2[num_cases].cast = NULL;

		if (numstatements != pr_cases[num_cases])
			QCC_PR_ParseError(ERR_CASENOTIMMEDIATE, "Case statements may not use formulas\n");	//fixme: insert them...
		num_cases++;
		QCC_PR_Expect(":");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_default, "default"))
	{
		if (num_cases >= max_cases)
		{
			max_cases += 8;
			pr_cases = realloc(pr_cases, sizeof(*pr_cases)*max_cases);
			pr_casesref = realloc(pr_casesref, sizeof(*pr_casesref)*max_cases);
			pr_casesref2 = realloc(pr_casesref2, sizeof(*pr_casesref2)*max_cases);
		}
		pr_cases[num_cases] = numstatements;
		pr_casesref[num_cases].cast = NULL;
		pr_casesref2[num_cases].cast = NULL;
		num_cases++;
		QCC_PR_Expect(":");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_thinktime, "thinktime"))
	{
		QCC_sref_t nextthink;
		QCC_sref_t time;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		QCC_PR_Expect(":");
		e2 = QCC_PR_Expression (TOP_PRIORITY, 0);
		e2 = QCC_SupplyConversion(e2, ev_float, true);
		if (e.cast->type != ev_entity || e2.cast->type != ev_float)
			QCC_PR_ParseError(ERR_THINKTIMETYPEMISMATCH, "thinktime type mismatch");

		if (QCC_OPCodeValid(&pr_opcodes[OP_THINKTIME]))
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_THINKTIME], e, e2, NULL));
		else
		{
			nextthink = QCC_PR_GetSRef(NULL, "nextthink", NULL, false, 0, false);
			if (!nextthink.cast)
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", "nextthink");
			time = QCC_PR_GetSRef(type_float, "time", NULL, false, 0, false);
			if (!time.cast)
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", "time");
			nextthink = QCC_PR_Statement(&pr_opcodes[OP_ADDRESS], e, nextthink, NULL);
			time = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], time, e2, NULL);
			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_F], time, nextthink, NULL));
		}
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckToken(";"))
	{
		int osl = pr_source_line;
		pr_source_line = statementstart;
		if (!expandedemptymacro)
		{
			if (!currentchunk || !currentchunk->cnst)
				QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Hanging ';'");
			while (QCC_PR_CheckToken(";"))
				;
		}
		pr_source_line = osl;
		return;
	}

	//C-style labels.
	if (pr_token_type == tt_name && pr_file_p[0] == ':' && pr_file_p[1] != ':')
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_BADLABELNAME, "invalid label name \"%s\"", pr_token);
			return;
		}

		for (i = 0; i < num_labels; i++)
			if (!STRNCMP(pr_labels[i].name, pr_token, sizeof(pr_labels[num_labels].name) -1))
			{
				QCC_PR_ParseWarning(WARN_DUPLICATELABEL, "Duplicate label %s", pr_token);
				QCC_PR_Lex();
				return;
			}

		if (num_labels >= max_labels)
		{
			max_labels += 8;
			pr_labels = realloc(pr_labels, sizeof(*pr_labels)*max_labels);
		}

		QC_strlcpy(pr_labels[num_labels].name, pr_token, sizeof(pr_labels[num_labels].name));
		pr_labels[num_labels].lineno = pr_source_line;
		pr_labels[num_labels].statementno = numstatements;

		num_labels++;

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		QCC_PR_Expect(":");
		return;
	}

//	qcc_functioncalled=0;

	QCC_PR_DiscardExpression (TOP_PRIORITY, 0);
	expandedemptymacro = false;
	QCC_PR_Expect (";");

//	qcc_functioncalled=false;
}


/*
==============
PR_ParseState

States are special functions made for convenience.  They automatically
set frame, nextthink (implicitly), and think (allowing forward definitions).

// void() name = [framenum, nextthink] {code}
// expands to:
// function void name ()
// {
//		self.frame=framenum;
//		self.nextthink = time + 0.1;
//		self.think = nextthink
//		<code>
// };
==============
*/
void QCC_PR_ParseState (void)
{
	QCC_sref_t	s1, def;

	pbool isinc;

	//FIXME: this is ambiguous with pre-inc and post-inc logic.
	if ((isinc=QCC_PR_CheckToken("++")) || QCC_PR_CheckToken("--"))
	{
		const QCC_eval_t *first, *last;
		int dir = 0;
		int op = OP_CSTATE;
		if (QCC_PR_CheckToken("("))
		{
			op = OP_CWSTATE;
			if (!QCC_PR_CheckToken("w"))
				QCC_PR_Expect("W");
			QCC_PR_Expect(")");
		}

//		s1 = QCC_PR_ParseImmediate ();

		s1 = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
		s1 = QCC_SupplyConversion(s1, ev_float, true);

		QCC_PR_Expect("..");
//		def = QCC_PR_ParseImmediate ();
		def = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
		def = QCC_SupplyConversion(def, ev_float, true);
		QCC_PR_Expect ("]");

		if (s1.cast->type != ev_float || def.cast->type != ev_float)
			QCC_PR_ParseError(ERR_STATETYPEMISMATCH, "state type mismatch");

		first = QCC_SRef_EvalConst(s1);
		last = QCC_SRef_EvalConst(def);
		if (first&&last)
		{	//whether its a ++ or -- doesn't really matter, but hcc generates an error so we should at least generate a warning.
			dir = (last->_float >= first->_float)?1:-1;
			if (isinc)
			{
				if (first->_float > last->_float)
					QCC_PR_ParseWarning(ERR_STATETYPEMISMATCH, "Forwards State Cycle with backwards range");
			}
			else
			{
				if (first->_float < last->_float)
					QCC_PR_ParseWarning(ERR_STATETYPEMISMATCH, "Forwards State Cycle with backwards range");
			}
		}


		if (QCC_OPCodeValid(&pr_opcodes[op]))
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[op], s1, def, NULL));
		else
		{
			QCC_statement_t *patch1, *entercycf, *entercycb, *fwd, *back;
			QCC_sref_t t1, t2;
			QCC_sref_t framef, frame;
			QCC_sref_t self;
			QCC_sref_t cycle_wrapped;

			self = QCC_PR_GetSRef(type_entity, "self", NULL, false, 0, false);
			framef = QCC_PR_GetSRef(NULL, (op==OP_CWSTATE)?"weaponframe":"frame", NULL, false, 0, false);
			cycle_wrapped = QCC_PR_GetSRef(type_float, "cycle_wrapped", NULL, false, 0, false);

			frame = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_F], self, framef, NULL, 0);
			if (cycle_wrapped.cast)
				QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(0), cycle_wrapped, NULL, STFL_PRESERVEB));

			if (dir)
				fwd = NULL;	//can skip the checks
			else
			{
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_GE_F], def, s1, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				fwd = QCC_Generate_OP_IFNOT(t1, false);
			}
			if (dir >= 0)
			{	//this block is the 'it's in a forwards direction'
				//make sure the frame is within the bounds given.
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], frame, s1, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				t2 = QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], frame, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				t1 = QCC_PR_Statement(&pr_opcodes[OP_OR_F], t1, t2, NULL);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					entercycf = QCC_Generate_OP_GOTO();
				}
				patch1->b.jumpofs = &statements[numstatements] - patch1;

				QCC_PR_SimpleStatement(&pr_opcodes[OP_ADD_F], frame, QCC_MakeFloatConst(1), frame, false);
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], frame, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					if (cycle_wrapped.cast)
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(1), cycle_wrapped, NULL, STFL_PRESERVEB));
				}
				patch1->b.jumpofs = &statements[numstatements] - patch1;
			}
			else entercycf = NULL;
			if (fwd)
			{
				back = QCC_Generate_OP_GOTO();
				fwd->b.jumpofs = &statements[numstatements] - fwd;
			}
			else
				back = NULL;
			if (dir <= 0)
			{	//reverse animation.

				//make sure the frame is within the bounds given.
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], frame, s1, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				t2 = QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], frame, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB);
				t1 = QCC_PR_Statement(&pr_opcodes[OP_OR_F], t1, t2, NULL);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					entercycb = QCC_Generate_OP_GOTO();
				}
				patch1->b.jumpofs = &statements[numstatements] - patch1;

				QCC_PR_SimpleStatement(&pr_opcodes[OP_SUB_F], frame, QCC_MakeFloatConst(1), frame, false);
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], frame, def, NULL, STFL_PRESERVEA);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEB));
					if (cycle_wrapped.cast)
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(1), cycle_wrapped, NULL, 0));
				}
				patch1->b.jumpofs = &statements[numstatements] - patch1;
			}
			else entercycb = NULL;

			if (back)
				back->a.jumpofs = &statements[numstatements] - back;

			if (entercycf)
				/*out of range*/entercycf->a.jumpofs = &statements[numstatements] - entercycf;
			if (entercycb)
				/*out of range*/entercycb->a.jumpofs = &statements[numstatements] - entercycb;
			//self.frame = frame happens with the normal state opcode.
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[(op==OP_CWSTATE)?OP_WSTATE:OP_STATE], frame, QCC_MakeSRef(pr_scope->def, 0, pr_scope->type), NULL));
		}
		return;
	}

	s1 = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
	s1 = QCC_SupplyConversion(s1, ev_float, true);

	if (!QCC_PR_CheckToken (","))
		QCC_PR_ParseWarning(WARN_UNEXPECTEDPUNCT, "missing comma in state definition");

	pr_assumetermtype = type_function;
	pr_assumetermscope = pr_scope->parentscope;
	pr_assumetermflags = GDF_CONST | (pr_assumetermscope?GDF_STATIC:0);
	def = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
	if (typecmp(def.cast, type_function))
	{
		if (!QCC_SRef_IsNull(def))
		{
			char typebuf1[256];
			char typebuf2[256];
			QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCH, def, "Type mismatch: %s, should be %s", TypeName(def.cast, typebuf1, sizeof(typebuf1)), TypeName(type_function, typebuf2, sizeof(typebuf2)));
		}
	}
	pr_assumetermtype = NULL;

	QCC_PR_Expect ("]");

	QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STATE], s1, def, NULL));
}

void QCC_PR_ParseAsm(void)
{
	QCC_statement_t *patch1;
	int op, p;
	QCC_sref_t a, b, c;

	if (QCC_PR_CheckKeyword(keyword_local, "local"))
	{
		QCC_PR_ParseDefs (NULL, true);
		return;
	}

	for (op = 0; op < OP_NUMOPS; op++)
	{
		if (!STRCMP(pr_token, pr_opcodes[op].opname))
		{
			QCC_PR_Lex();
			if (/*pr_opcodes[op].priority==-1 &&*/ pr_opcodes[op].associative!=ASSOC_LEFT)
			{
				if (pr_opcodes[op].type_a==NULL)
				{
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], nullsref, nullsref, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->a.ofs = p;
					}

					QCC_PR_Lex();
				}
				else if (pr_opcodes[op].type_b==NULL)
				{
					a = QCC_PR_ParseValue(pr_classtype, false, false, true);
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], a, nullsref, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->b.ofs = (int)p;
					}

					QCC_PR_Lex();
				}
				else
				{
					if (QCC_PR_CheckName("void"))
						a = nullsref;
					else
						a = QCC_PR_ParseValue(pr_classtype, false, false, true);
					QCC_PR_Expect(",");
					if (QCC_PR_CheckName("void"))
						b = nullsref;
					else
						b = QCC_PR_ParseValue(pr_classtype, false, false, true);
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], a, b, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->c.ofs = p;
					}
				}
			}
			else
			{
				if (pr_opcodes[op].type_a != &type_void)
				{
					if (QCC_PR_CheckName("void"))
						a = nullsref;
					else
						a = QCC_PR_ParseValue(pr_classtype, false, false, true);
				}
				else
					a=nullsref;
				if (pr_opcodes[op].type_b != &type_void)
				{
					QCC_PR_CheckToken(",");
					if (QCC_PR_CheckName("void"))
						b = nullsref;
					else
						b = QCC_PR_ParseValue(pr_classtype, false, false, true);
				}
				else
					b=nullsref;
				if (pr_opcodes[op].associative==ASSOC_LEFT && pr_opcodes[op].type_c != &type_void)
				{
					QCC_PR_CheckToken(",");
					if (QCC_PR_CheckName("void"))
						c = nullsref;
					else
						c = QCC_PR_ParseValue(pr_classtype, false, false, true);
				}
				else
					c=nullsref;

				QCC_PR_SimpleStatement(&pr_opcodes[op], a, b, c, true);
			}

			QCC_PR_Expect(";");
			return;
		}
	}
	QCC_PR_ParseError(ERR_BADOPCODE, "Bad op code name %s", pr_token);
}

static pbool QCC_FuncJumpsTo(int first, int last, int statement)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + statements[st].a.jumpofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + statements[st].b.jumpofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + statements[st].c.jumpofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
	}
	return false;
}
/*
static pbool QCC_FuncJumpsToRange(int first, int last, int firstr, int lastr)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + (signed)statements[st].a >= firstr && st + (signed)statements[st].a <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + (signed)statements[st].b >= firstr && st + (signed)statements[st].b <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + (signed)statements[st].c >= firstr && st + (signed)statements[st].c <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
	}
	return false;
}
*/
#if 0
void QCC_CompoundJumps(int first, int last)
{
	//jumps to jumps are reordered so they become jumps to the final target.
	int statement;
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			statement = st + (signed)statements[st].a;
			if (statements[statement].op == OP_RETURN || statements[statement].op == OP_DONE)
			{	//goto leads to return. Copy the command out to remove the goto.
				statements[st].op = statements[statement].op;
				statements[st].a = statements[statement].a;
				statements[st].b = statements[statement].b;
				statements[st].c = statements[statement].c;
				optres_compound_jumps++;
			}
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].a = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].a;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			statement = st + (signed)statements[st].b;
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].b = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].b;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			statement = st + (signed)statements[st].c;
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].c = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].c;
				optres_compound_jumps++;
			}
		}
	}
}
#else
static void QCC_CompoundJumps(int first, int last)
{
	//jumps to jumps are reordered so they become jumps to the final target.
	int statement;
	int st;
	int infloop;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			statement = st + statements[st].a.jumpofs;
			if (statements[statement].op == OP_RETURN || statements[statement].op == OP_DONE)
			{	//goto leads to return. Copy the command out to remove the goto.
				statements[st] = statements[statement];
				optres_compound_jumps++;
			}
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].a.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + statements[st].a.jumpofs;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			statement = st + statements[st].b.jumpofs;
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].b.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + statements[st].b.jumpofs;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			statement = st + statements[st].c.jumpofs;
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].c.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + statements[st].c.jumpofs;
				optres_compound_jumps++;
			}
		}
	}
}
#endif

static void QCC_CheckForDeadAndMissingReturns(int first, int last, int rettype)
{
	QCC_function_t *fnc;
	int st, st2;

	if (statements[last-1].op == OP_DONE)
		last--;	//don't want the done

	if (rettype != ev_void)
		if (statements[last-1].op != OP_RETURN)
		{
			if (statements[last-1].op != OP_GOTO || statements[last-1].a.jumpofs > 0)
			{
				QCC_PR_Warning(WARN_MISSINGRETURN, s_filen, statements[last].linenum, "%s: not all control paths return a value", pr_scope->name );
				return;
			}
		}

	for (st = first; st < last; st++)
	{
		if (statements[st].op == OP_RETURN || statements[st].op == OP_GOTO)
		{
			st++;
			if (st == last)
				continue;	//erm... end of function doesn't count as unreachable.

			if (!opt_compound_jumps)
			{	//we can ignore single statements like these without compound jumps (compound jumps correctly removes all).
				if (statements[st].op == OP_GOTO)	//inefficient compiler, we can ignore this.
					continue;
				if (statements[st].op == OP_DONE)	//inefficient compiler, we can ignore this.
					continue;
			}
			//always allow random return statements, because people like putting returns after switches even when all the switches have a return.
			if (statements[st].op == OP_RETURN)	//inefficient compiler, we can ignore this.
				continue;

			//check for embedded functions. FIXME: should generate outside of the parent function.
			for (fnc = pr_scope+1; fnc < &functions[numfunctions]; fnc++)
			{
				if (fnc->code == st)
					break;
			}
			if (fnc < &functions[numfunctions])
				continue;

			//make sure something goes to just after this return.
			for (st2 = first; st2 < last; st2++)
			{
				if (pr_opcodes[statements[st2].op].associative == ASSOC_RIGHT)
				{
					if (pr_opcodes[statements[st2].op].type_a == NULL)
					{
						if (st2 + statements[st2].a.jumpofs == st)
							break;
					}
					if (pr_opcodes[statements[st2].op].type_b == NULL)
					{
						if (st2 + statements[st2].b.jumpofs == st)
							break;
					}
					if (pr_opcodes[statements[st2].op].type_c == NULL)
					{
						if (st2 + statements[st2].c.jumpofs == st)
							break;
					}
				}
			}
			if (st2 == last)
			{
				QCC_PR_Warning(WARN_UNREACHABLECODE, pr_scope->filen, statements[st].linenum, "%s: contains unreachable code (line %i)", pr_scope->name, statements[st].linenum);
			}
			continue;
		}
		if (rettype != ev_void && pr_opcodes[statements[st].op].associative == ASSOC_RIGHT)
		{
			if (pr_opcodes[statements[st].op].type_a == NULL)
			{
				if (st + statements[st].a.jumpofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
			if (pr_opcodes[statements[st].op].type_b == NULL)
			{
				if (st + statements[st].b.jumpofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
			if (pr_opcodes[statements[st].op].type_c == NULL)
			{
				if (st + statements[st].c.jumpofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
		}
	}
}

pbool QCC_StatementIsAJump(int stnum, int notifdest)	//only the unconditionals.
{
	if (statements[stnum].op == OP_RETURN)
		return true;
	if (statements[stnum].op == OP_DONE)
		return true;
	if (statements[stnum].op == OP_GOTO)
		if (statements[stnum].a.jumpofs != notifdest)
			return true;
	return false;
}

int QCC_AStatementJumpsTo(int targ, int first, int last)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + statements[st].a.jumpofs == targ && statements[st].a.ofs)
			{
				return true;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + statements[st].b.jumpofs == targ)
			{
				return true;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + statements[st].c.jumpofs == targ)
			{
				return true;
			}
		}
	}

	for (st = 0; st < num_labels; st++)	//assume it's used.
	{
		if (pr_labels[st].statementno == targ)
			return true;
	}

	for (st = 0; st < num_cases; st++)	//assume it's used.
	{
		if (pr_cases[st] == targ)
			return true;
	}

	return false;
}
/*
//goes through statements, if it sees a matching statement earlier, it'll strim out the current.
void QCC_CommonSubExpressionRemoval(int first, int last)
{
	int cur;	//the current
	int prev;	//the earlier statement
	for (cur = last-1; cur >= first; cur--)
	{
		if (pr_opcodes[statements[cur].op].priority == -1)
			continue;
		for (prev = cur-1; prev >= first; prev--)
		{
			if (statements[prev].op >= OP_CALL0 && statements[prev].op <= OP_CALL8)
			{
				optres_test1++;
				break;
			}
			if (statements[prev].op >= OP_CALL1H && statements[prev].op <= OP_CALL8H)
			{
				optres_test1++;
				break;
			}
			if (pr_opcodes[statements[prev].op].right_associative)
			{	//make sure no changes to var_a occur.
				if (statements[prev].b == statements[cur].a)
				{
					optres_test2++;
					break;
				}
				if (statements[prev].b == statements[cur].b && !pr_opcodes[statements[cur].op].right_associative)
				{
					optres_test2++;
					break;
				}
			}
			else
			{
				if (statements[prev].c == statements[cur].a)
				{
					optres_test2++;
					break;
				}
				if (statements[prev].c == statements[cur].b && !pr_opcodes[statements[cur].op].right_associative)
				{
					optres_test2++;
					break;
				}
			}

			if (statements[prev].op == statements[cur].op)
				if (statements[prev].a == statements[cur].a)
					if (statements[prev].b == statements[cur].b)
						if (statements[prev].c == statements[cur].c)
						{
							if (!QCC_FuncJumpsToRange(first, last, prev, cur))
							{
								statements[cur].op = OP_STORE_F;
								statements[cur].a = 28;
								statements[cur].b = 28;
								optres_comexprremoval++;
							}
							else
								optres_test1++;
							break;
						}
		}
	}
}
*/

//follow branches (by recursing).
//stop on first read(error, return statement) or write(no error, return -1)
//end-of-block returns 0, done/return/goto returns -2
static int QCC_CheckOneUninitialised(int firststatement, int laststatement, union QCC_eval_basic_s *min, union QCC_eval_basic_s *max)
{
#define SPLIT \
		if (!(ofs > max || ofs+sz <= min)) \
		{	\
			if (min < ofs)	\
			{	/*keep checking before*/ \
				ret = QCC_CheckOneUninitialised(i + 1, laststatement, min, ofs);	\
				if (ret > 0)	\
					return ret;	\
			}	\
			if (ofs+sz < max)	\
			{	/*keep checking after*/ \
				ret = QCC_CheckOneUninitialised(i + 1, laststatement, ofs+sz, max);	\
				if (ret > 0)	\
					return ret;	\
			}	\
			if (iswrite) \
				return -1; /*okay, we wrote it all*/ \
			return i;	/*an error when its a read*/ \
		}
	int ret;
	int i;
	QCC_statement_t *st;

	for (i = firststatement; i < laststatement; i++)
	{
		st = &statements[i];

		if (st->op == OP_DONE || st->op == OP_RETURN)
		{
			if (st->a.cast && st->a.sym)
			{
				union QCC_eval_basic_s *ofs = st->a.sym->symboldata+st->a.ofs;
				int sz = st->a.cast->size;
				if (!(ofs > max || ofs+sz < min))
					return i;
			}
			return -2;
		}

//		this code catches gotos, but can cause issues with while statements.
//		if (st->op == OP_GOTO && (int)st->a < 1)
//			return -2;

		if (pr_opcodes[st->op].type_a && st->a.sym)
		{
			union QCC_eval_basic_s *ofs = st->a.sym->symboldata+st->a.ofs;
			int sz = st->a.cast->size;
			pbool iswrite = OpAssignsToA(st->op) || st->op == OP_GLOBALADDRESS; /* address-of counts as a write here, because we're too dumb to track when/if that is assigned to.*/

			SPLIT
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && st->a.jumpofs > 0 && !st->a.sym)
		{
			int jump = i + st->a.jumpofs;
			ret = QCC_CheckOneUninitialised(i + 1, jump, min, max);
			if (ret > 0)
				return ret;
			i = jump-1;
		}

		if (pr_opcodes[st->op].type_b && st->b.sym)
		{
			union QCC_eval_basic_s *ofs = st->b.sym->symboldata+st->b.jumpofs;
			int sz = st->b.cast->size;
			pbool iswrite = OpAssignsToB(st->op);

			SPLIT
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && st->b.jumpofs > 0 && !st->b.sym && !(st->flags & STF_LOGICOP))
		{
			int jump = i + st->b.jumpofs;
			//check if there's an else.
			st = &statements[jump-1];
			if (st->op == OP_GOTO && st->a.jumpofs > 0)
			{
				int jump2 = jump-1 + st->a.ofs;
				int rett = QCC_CheckOneUninitialised(i + 1, jump - 1, min, max);
				if (rett > 0)
					return rett;
				ret = QCC_CheckOneUninitialised(jump, jump2, min, max);
				if (ret > 0)
					return ret;
				if (rett < 0 && ret < 0)
					return (rett == ret)?ret:-1;	//inited or aborted in both, don't need to continue along this branch
				i = jump2-1;
			}
			else
			{
				ret = QCC_CheckOneUninitialised(i + 1, jump, min, max);
				if (ret > 0)
					return ret;
				i = jump-1;
			}
			continue;
		}

		if (pr_opcodes[st->op].type_c && st->c.sym)
		{
			union QCC_eval_basic_s *ofs = st->c.sym->symboldata+st->c.ofs;
			int sz = st->c.cast->size;
			pbool iswrite = OpAssignsToC(st->op);

			SPLIT
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && st->c.jumpofs > 0 && !st->c.sym)
		{
			int jump = i + st->c.jumpofs;
			ret = QCC_CheckOneUninitialised(i + 1, jump, min, max);
			if (ret > 0)
				return ret;
			i = jump-1;

			continue;
		}
	}

	return 0;
#undef SPLIT
}

static pbool QCC_CheckUninitialised(int firststatement, int laststatement)
{
	QCC_def_t *local, *c, *uninit;
	unsigned int i;
	pbool result = false;
	unsigned int paramend = FIRST_LOCAL;
	QCC_type_t *type = pr_scope->type;
	int err;

	//assume all, because we don't care for optimisations once we know we're not going to compile anything (removes warning about uninitialised unknown variables/typos).
	if (pr_error_count)
		return true;

	for (i = 0; i < type->num_parms; i++)
	{
		paramend += type->params[i].type->size;
	}

	for (local = pr.local_head.nextlocal; local; local = local->nextlocal)
	{
		if (local->constant)
			continue;	//will get some other warning, so we don't care.
		if (local->isstatic)
			continue;	//not a real local, so will be properly initialised.
		if (local->symbolheader != local)
			continue;	//ignore slave symbols, cos they're not interesting and should have been checked as part of the parent.
		if (local->isparameter)
			continue;
		if (local->arraysize)
			continue;	//probably indexed. we won't detect things properly. its the user's resposibility to check. :(
		err = QCC_CheckOneUninitialised(firststatement, laststatement, local->symboldata, local->symboldata + local->type->size * (local->arraysize?local->arraysize:1));
		if (err > 0)
		{	//try to refine it to a single component if we can.
			uninit = NULL;
			for (c = local; ; c = c->next)
			{
				if (c != local)
				{
					err = QCC_CheckOneUninitialised(firststatement, laststatement, c->symboldata, c->symboldata + c->type->size * (c->arraysize?c->arraysize:1));
					if (err > 0)
					{
						if (uninit)
						{
							uninit = NULL;
							break;
						}
						uninit = c;
					}
				}
				if (c == local->deftail)
					break;	//that was the last of them.
			}
			if (!uninit)	//otherwise give up and print the whole struct.
				uninit = local;
			QCC_PR_Warning(WARN_UNINITIALIZED, s_filen, statements[err].linenum, "Potentially uninitialised variable %s%s%s", col_symbol,uninit->name,col_none);
			result = true;

//			break;
		}
	}

	return result;
}

void QCC_Marshal_Locals(int firststatement, int laststatement)
{
	QCC_def_t *local;
	pbool error = false;

	if (pr.local_head.nextlocal)	//only check if there's actually somthing to check
	{	//FIXME: we should just insert extra statements to clear any we deem uninitialised, instead of generating errors etc.
		//then we can overlap all functions always without worrying.
		if (flag_allowuninit)
		{
			if (qccwarningaction[WARN_UNINITIALIZED])
				QCC_CheckUninitialised(firststatement, laststatement);	//still need to call it for warnings, but if those warnings are off we can skip the cost
		}
		else if (!opt_locals_overlapping)
		{
			if (qccwarningaction[WARN_UNINITIALIZED])
				QCC_CheckUninitialised(firststatement, laststatement);	//still need to call it for warnings, but if those warnings are off we can skip the cost
			error = true;	//always use the legacy behaviour
		}
		else if (QCC_CheckUninitialised(firststatement, laststatement))
		{
			error = true;
	//		QCC_PR_Note(ERR_INTERNAL, strings+s_file, pr_source_line, "Not overlapping locals from %s due to uninitialised locals", pr_scope->name);
		}
		else
		{
			//make sure we're allowed to marshall this function's locals
			for (local = pr.local_head.nextlocal; local; local = local->nextlocal)
			{
				if (local->isstatic)
					continue;	//static variables are actually globals
				if (local->constant && local->initialized)
					continue;	//as are initialised consts, because its pointless otherwise.
				if (local->symbolheader && local->symbolheader->scope != local->scope)
					continue;

				//FIXME: check for uninitialised locals.
				//these matter when the function goes recursive (and locals marshalling counts as recursive every time).
				if (local->symboldata[0]._int)
				{
					if (!error)
						QCC_PR_Note(ERR_INTERNAL, local->filen, local->s_line, "Marshaling non-const initialised %s", local->name);
					error = true;
				}

				/*if (local->constant)
				{
					QCC_PR_Note(ERR_INTERNAL, local->filen, local->s_line, "Marshaling const %s", local->name);
					error = true;
				}*/
			}
		}

		//func(&somelocal) reuses the same memory address for both caller and callee. there's nothing we safely do to fix recursive functions, but we can at least stop -Olo from breaking things more.
		if (!error)
		{
			int i;
			QCC_statement_t *st;
			for (i = firststatement; i < laststatement; i++)
			{
				st = &statements[i];

				if (st->op == OP_GLOBALADDRESS && st->a.sym->scope && !st->a.sym->isstatic)
				{
					error = true;
					break;
				}
			}
		}
	}

	if (error)
		pr_scope->privatelocals = true;
	else
		pr_scope->privatelocals = false;
	pr_scope->firstlocal = pr.local_head.nextlocal;
	pr.local_head.nextlocal = NULL;
	pr.local_tail = &pr.local_head;
}

#ifdef WRITEASM
static void QCC_WriteGUIAsmFunction(QCC_function_t	*sc, unsigned int firststatement)
{
	unsigned int			i;
//	QCC_type_t *type;
	char typebuf[512];

	char line[2048];
	extern int currentsourcefile;

//	type = sc->type;

	for (i = firststatement; i < (unsigned int)numstatements; i++)
	{
		line[0] = 0;
//		QC_snprintfz(line, sizeof(line), "%i  ", QCC_VarAtOffset(statements[i].a));
		QC_strlcat(line, pr_opcodes[statements[i].op].opname, sizeof(line));
		if (pr_opcodes[statements[i].op].type_a != &type_void)
		{
//			if (strlen(pr_opcodes[statements[i].op].opname)<6)
//				QC_strlcat(line, "  ", sizeof(line));
			if (pr_opcodes[statements[i].op].type_a)
				QC_snprintfz(typebuf, sizeof(typebuf), "  %s", QCC_VarAtOffset(statements[i].a));
			else
				QC_snprintfz(typebuf, sizeof(typebuf), "  %i", statements[i].a.jumpofs);
			QC_strlcat(line, typebuf, sizeof(line));
			if (pr_opcodes[statements[i].op].type_b != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_b)
					QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].b));
				else
					QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].b.jumpofs);
				QC_strlcat(line, typebuf, sizeof(line));
				if (pr_opcodes[statements[i].op].type_c != &type_void && (pr_opcodes[statements[i].op].associative==ASSOC_LEFT || statements[i].c.cast))
				{
					if (pr_opcodes[statements[i].op].type_c)
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].c));
					else
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].c.jumpofs);
					QC_strlcat(line, typebuf, sizeof(line));
				}
			}
			else
			{
				if (pr_opcodes[statements[i].op].type_c != &type_void)
				{
					if (pr_opcodes[statements[i].op].type_c)
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].c));
					else
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].c.jumpofs);
					QC_strlcat(line, typebuf, sizeof(line));
				}
			}
		}	
		else
		{
			if (pr_opcodes[statements[i].op].type_c != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_c)
					QC_snprintfz(typebuf, sizeof(typebuf), "  %s", QCC_VarAtOffset(statements[i].c));
				else
					QC_snprintfz(typebuf, sizeof(typebuf), "  %i", statements[i].c.jumpofs);
				QC_strlcat(line, typebuf, sizeof(line));
			}
		}
		if (currentsourcefile)
			externs->Printf("code: %s:%i: %i:%s;\n", sc->filen, statements[i].linenum, currentsourcefile, line);
		else
			externs->Printf("code: %s:%i: %s;\n", sc->filen, statements[i].linenum, line);
	}
}

void QCC_WriteAsmFunction(QCC_function_t	*sc, unsigned int firststatement, QCC_def_t *firstparm)
{
	unsigned int			i;
	QCC_def_t *o = firstparm;
	QCC_type_t *type;
	char typebuf[512];

	if (sc->parentscope)	//don't print dupes.
		return;

	if (flag_guiannotate)
		QCC_WriteGUIAsmFunction(sc, firststatement);

	if (!asmfile)
		return;

	type = sc->type;
	fprintf(asmfile, "%s(", TypeName(type->aux_type, typebuf, sizeof(typebuf)));
	for (o = pr.local_head.nextlocal, i = 0; i < type->num_parms; i++)
	{
		if (i)
			fprintf(asmfile, ", ");

		if (o)
		{
			fprintf(asmfile, "%s %s", TypeName(o->type, typebuf, sizeof(typebuf)), o->name);
			o = o->nextlocal;
		}
		else
			fprintf(asmfile, "%s", TypeName(type->params[i].type, typebuf, sizeof(typebuf)));
	}

	fprintf(asmfile, ") %s = asm\n{\n", sc->name);

	QCC_fprintfLocals(asmfile, o);

	for (i = firststatement; i < (unsigned int)numstatements; i++)
	{
		fprintf(asmfile, "\t%s", pr_opcodes[statements[i].op].opname);
		if (pr_opcodes[statements[i].op].type_a != &type_void)
		{
			if (strlen(pr_opcodes[statements[i].op].opname)<6)
				fprintf(asmfile, "\t");
			if (pr_opcodes[statements[i].op].type_a)
				fprintf(asmfile, "\t%s", QCC_VarAtOffset(statements[i].a));
			else
				fprintf(asmfile, "\t%i", statements[i].a.ofs);
			if (pr_opcodes[statements[i].op].type_b != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_b)
					fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].b));
				else
					fprintf(asmfile, ",\t%i", statements[i].b.ofs);
				if (pr_opcodes[statements[i].op].type_c != &type_void && (pr_opcodes[statements[i].op].associative==ASSOC_LEFT || statements[i].c.sym))
				{
					if (pr_opcodes[statements[i].op].type_c)
						fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].c));
					else
						fprintf(asmfile, ",\t%i", statements[i].c.ofs);
				}
			}
			else
			{
				if (pr_opcodes[statements[i].op].type_c != &type_void)
				{
					if (pr_opcodes[statements[i].op].type_c)
						fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].c));
					else
						fprintf(asmfile, ",\t%i", statements[i].c.ofs);
				}
			}
		}	
		else
		{
			if (pr_opcodes[statements[i].op].type_c != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_c)
					fprintf(asmfile, "\t%s", QCC_VarAtOffset(statements[i].c));
				else
					fprintf(asmfile, "\t%i", statements[i].c.ofs);
			}
		}
		fprintf(asmfile, "; /*%i*/\n", statements[i].linenum);
	}

	fprintf(asmfile, "}\n\n");
}
#endif

static QCC_function_t *QCC_PR_GenerateBuiltinFunction (QCC_def_t *def, int builtinnum, char *builtinname)
{
	QCC_function_t *func;
	if (numfunctions >= MAX_FUNCTIONS)
		QCC_PR_ParseError(ERR_INTERNAL, "Too many functions - %i\nAdd '-max_functions %i' to the commandline", numfunctions, (numfunctions+4096)&~4095);
	func = &functions[numfunctions++];
	func->filen = s_filen;
	func->unitn = s_unitn;
	func->s_filed = s_filed;
	func->line = def->s_line;	//FIXME
	if (builtinname==def->name)
		func->name = builtinname;
	else
	{
		func->name = qccHunkAlloc(strlen(builtinname)+1);
		strcpy(func->name, builtinname);
	}
	func->builtin = builtinnum;
	func->code = -1;
	func->type = def->type;
	func->firstlocal = NULL;
	func->def = def;
	return func;
}
static QCC_function_t *QCC_PR_GenerateQCFunction (QCC_def_t *def, QCC_type_t *type, unsigned int *pif_flags)
{
	QCC_function_t *func = NULL;
	if (numfunctions >= MAX_FUNCTIONS)
		QCC_PR_ParseError(ERR_INTERNAL, "Too many functions - %i\nAdd '-max_functions %i' to the commandline", numfunctions, (numfunctions+4096)&~4095);

	if (!pif_flags)
		;
	else if ((*pif_flags & PIF_ACCUMULATE) && !(*pif_flags & PIF_WRAP))
	{
		if (def->symboldata[0].function)
		{
			func = &functions[def->symboldata[0].function];	//just resume the old one...

			if (func->def != def || func->type != type || func->parentscope != pr_scope)
				QCC_PR_ParseError(ERR_INTERNAL, "invalid function accumulation");

			//fixme: validate stuff
			return func;
		}
	}
	else if ((*pif_flags&PIF_WRAP) && def->symboldata[0].function)
	{
		QCC_def_t *locals;
		QCC_function_t *prior;
		func = &functions[def->symboldata[0].function];
		if ((*pif_flags&PIF_AUTOWRAP) && func->statements && func->def == def && func->type == type && func->parentscope == pr_scope)
		{
			*pif_flags &= ~(PIF_WRAP|PIF_AUTOWRAP);
			return func;	//looks like we should be able to just reuse it. no need to wrap.
		}
		prior = &functions[numfunctions++];
		memcpy(prior, func, sizeof(*prior));

		//FIXME: we need a proper algorithm to generate valid anonymous function names.
		prior->name = qccHunkAlloc(6+strlen(func->name)+1);
		strcpy(prior->name, "prior*");
		strcpy(prior->name+6, func->name);

		memset(func, 0, sizeof(*func));

		for (locals = prior->firstlocal; locals; locals = locals->nextlocal)
		{
			if (locals->scope != func)
				QCC_PR_ParseError(ERR_INTERNAL, "internal consistency check failed while wrapping %s", def->name);
			locals->scope = prior;
		}
	}
	else if (*pif_flags&PIF_WRAP)
	{
		QCC_PR_ParseError(ERR_INTERNAL, "cannot wrap bodyless function %s", def->name);
		return NULL;
	}
	if (!func)
		func = &functions[numfunctions++];
	func->filen = s_filen;
	func->unitn = s_unitn;
	func->s_filed = s_filed;
	func->line = pr_source_line;//def?def->s_line:0;	//FIXME
	func->name = def?def->name:"";
	func->builtin = 0;
	func->code = numstatements;
	func->firstlocal = NULL;
	func->def = def;
	func->type = type;
	func->parentscope = pr_scope;
	return func;
}

static void QCC_PR_ResumeFunction(QCC_function_t *f)
{
	if (pr_scope != f)
	{
		pr_scope = f;

		//reset the locals chain
		pr.local_head.nextlocal = f->firstlocal;
		pr.local_tail = &pr.local_head;
		while (pr.local_tail->nextlocal)
			pr.local_tail = pr.local_tail->nextlocal;

		//qcvm sees the function start here.
		f->code = numstatements;

		if (f->statements)
		{
			memcpy(statements+f->code, f->statements, sizeof(*statements) * f->numstatements);
			numstatements += f->numstatements;

			f->statements = NULL;
			f->numstatements = 0;
		}
	}
}
static void QCC_PR_FinaliseFunction(QCC_function_t *f)
{
	QCC_statement_t *st;
	pbool needsdone=false;

	QCC_PR_ResumeFunction(f);

	pr_token_line_last = f->line_end;
	s_filen = f->filen;
	
	if (f->returndef.cast && f->type->aux_type->size <= type_vector->size)
	{
		PR_GenerateReturnOuts();
		QCC_ForceUnFreeDef(f->returndef.sym);
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], f->returndef, nullsref, NULL));
	}

	if (f->code == numstatements)
		needsdone = true;
	else if (statements[numstatements - 1].op != OP_RETURN && statements[numstatements - 1].op != OP_DONE)
		needsdone = true;

	if (opt_return_only && !needsdone)
		needsdone = QCC_FuncJumpsTo(f->code, numstatements, numstatements);

	// emit an end of statements opcode
	if (!opt_return_only || needsdone)
	{
		/*if (pr_classtype)
		{
			QCC_def_t *e, *e2;
			e = QCC_PR_GetDef(NULL, "__oself", pr_scope, false, 0);
			e2 = QCC_PR_GetDef(NULL, "self", NULL, false, 0);
			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], e, QCC_PR_DummyDef(pr_classtype, "self", pr_scope, 0, e2->ofs, false), NULL));
		}*/

		if (needsdone)
			PR_GenerateReturnOuts();
		QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, &st);
	}
	else
		optres_return_only++;

	QCC_CheckForDeadAndMissingReturns(f->code, numstatements, f->type->aux_type->type);

//	if (opt_comexprremoval)
//		QCC_CommonSubExpressionRemoval(f->code, numstatements);


	QCC_RemapLockedTemps(f->code, numstatements);
	QCC_Marshal_Locals(f->code, numstatements);
	QCC_WriteAsmFunction(f, f->code, f->firstlocal);

	if (opt_compound_jumps)
		QCC_CompoundJumps(f->code, numstatements);
}
void QCC_PR_FinaliseFunctions(void)
{
	QCC_function_t *f;
	if (numinitstatements)
	{
		QCC_statement_t *ns;
		f = functions+numfunctions;
		if (f==functions+numfunctions) for (f = functions+1; f < functions+numfunctions; f++) if (!strcmp(f->name, "_atinit")) break;
		if (f==functions+numfunctions) for (f = functions+1; f < functions+numfunctions; f++) if (!strcmp(f->name, "init")) break;
		if (f==functions+numfunctions) for (f = functions+1; f < functions+numfunctions; f++) if (!strcmp(f->name, "initents")) break;
		if (f==functions+numfunctions) for (f = functions+1; f < functions+numfunctions; f++) if (!strcmp(f->name, "m_init")||!strcmp(f->name, "CSQC_Init")||!strcmp(f->name, "worldspawn")) break;
		if (f==functions+numfunctions)
			QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Construction statements were not part of any function. call _atinit manually.");
		else
		{
			//copy it into the start of the function. shouldn't be any locals/temps/etc to worry about.
			ns = qccHunkAlloc((numinitstatements + f->numstatements) * sizeof(*ns));
			memcpy(ns, initstatements, numinitstatements*sizeof(*ns));
			memcpy(ns+numinitstatements, f->statements, f->numstatements*sizeof(*ns));
			f->statements = ns;
			f->numstatements += numinitstatements;
			numinitstatements = 0;
		}
	}

	for (f = functions; f < functions+numfunctions; f++)
	{
		if (f->statements)
			QCC_PR_FinaliseFunction(f);
	}
}
/*
============
PR_ParseImmediateStatements

Parse a function body

If def is set, allows stuff to refer back to a def for the function.
============
*/
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type, unsigned int pif_flags)
{
	unsigned int u, p;
	QCC_function_t	*f;
	QCC_sref_t		parm;

	QCC_def_t *prior = NULL, *d, *lastparm;
	pbool mergeargs;

	int startingtypes = numtypeinfos;

	conditional = 0;

	expandedemptymacro = false;

//
// check for builtin function definition #1, #2, etc
//
// hexenC has void name() : 2;
	if (!(pif_flags & PIF_ACCUMULATE))
	{
		if (QCC_PR_CheckToken ("#") || QCC_PR_CheckToken (":"))
		{
			int binum = 0;
			if (pr_token_type == tt_immediate
			&& pr_immediate_type == type_float
			&& pr_immediate._float == (int)pr_immediate._float)
				binum = (int)pr_immediate._float;
			else if (pr_token_type == tt_immediate && pr_immediate_type == type_integer)
				binum = pr_immediate._int;
			else
				QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
			f = QCC_PR_GenerateBuiltinFunction(def, binum, def->name);
			QCC_PR_Lex ();

			return f;
		}
		if (QCC_PR_CheckKeyword(keyword_external, "external"))
		{	//reacc style builtin
			if (pr_token_type != tt_immediate
			|| pr_immediate_type != type_float
			|| pr_immediate._float != (int)pr_immediate._float)
				QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
			f = QCC_PR_GenerateBuiltinFunction(def, (int)-pr_immediate._float, def->name);
			QCC_PR_Lex ();
			QCC_PR_Expect(";");

			return f;
		}
	}

//	if (type->vargs)
//		QCC_PR_ParseError (ERR_FUNCTIONWITHVARGS, "QC function with variable arguments and function body");

	f = QCC_PR_GenerateQCFunction(def, type, &pif_flags);

	QCC_PR_ResumeFunction(f);

	QCC_RemapLockedTemps(-1, -1);

	mergeargs = !!f->firstlocal;
//
// define the basic parms
//
	if (mergeargs)
	{
		QCC_def_t *arg;
		for (u=0, p=0, arg=f->firstlocal ; u<type->num_parms; u++, arg = arg->deftail->nextlocal)
		{
			QCC_PR_DummyDef(type->params[u].type, pr_parm_names[u], pr_scope, 0, arg, 0, true, GDF_PARAMETER);
		}

		if (type->vargcount)
		{
			if (!pr_parm_argcount_name)
				QCC_Error(ERR_INTERNAL, "I forgot what the va_count argument is meant to be called");
			else
				QCC_PR_DummyDef(type_float, pr_parm_argcount_name, pr_scope, 0, arg, 0, true, 0);
		}
	}
	else
	{
		for (u=0, p=0 ; u<type->num_parms; u++)
		{
			unsigned int o;
			if (!*pr_parm_names[u])
			{
				QC_snprintfz(pr_parm_names[u], sizeof(pr_parm_names[u]), "$arg_%u", u);
				QCC_PR_ParseWarning(WARN_PARAMWITHNONAME, "Parameter %u of %s is not named", u+1, pr_scope->name);
			}
			parm = QCC_PR_GetSRef (type->params[u].type, pr_parm_names[u], pr_scope, 2, 0, 0);
			parm.sym->used = true;	//make sure system parameters get seen by the engine, even if the names are stripped..
			parm.sym->referenced = true;

			for (o = 0; o < (type->params[u].type->size+2)/3; o++)
			{
				if (p < MAX_PARMS)
				{
					parm.sym->isparameter = true;
					parm.ofs+=3;//no need to copy anything, as the engine will do it for us.
				}
				else
				{	//extra parms need to be explicitly copied.
					if (!extra_parms[p - MAX_PARMS].sym)
					{
						char name[128];
						QC_snprintfz(name, sizeof(name), "$parm%u", p);
						extra_parms[p - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
					}
					else
						QCC_ForceUnFreeDef(extra_parms[p - MAX_PARMS].sym);
					QCC_UnFreeTemp(parm);
					extra_parms[p - MAX_PARMS].cast = parm.cast;
					if (type->params[u].type->size-o*3 >= 3)
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_V], extra_parms[p - MAX_PARMS], parm, NULL));
						parm.ofs+=3;
					}
					else if (type->params[u].type->size-o*3 == 2 && QCC_OPCodeValid(&pr_opcodes[OP_STORE_I64]))
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_I64], extra_parms[p - MAX_PARMS], parm, NULL));
						parm.ofs+=2;
					}
					else if (type->params[u].type->size-o*3 == 2)
					{
						QCC_sref_t t = extra_parms[p - MAX_PARMS];
						QCC_UnFreeTemp(t);
						QCC_UnFreeTemp(parm);
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_F], t, parm, NULL));
						t.ofs++;
						parm.ofs++;
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_F], t, parm, NULL));
						parm.ofs++;
					}
					else
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_F], extra_parms[p - MAX_PARMS], parm, NULL));
						parm.ofs++;
					}
				}
				p++;
			}
			QCC_FreeTemp(parm);
		}

		if (type->vargcount)
		{
			if (!pr_parm_argcount_name)
				QCC_Error(ERR_INTERNAL, "I forgot what the va_count argument is meant to be called");
			else
			{
				QCC_sref_t va_passcount = QCC_PR_GetSRef(type_float, "__va_count", NULL, true, 0, 0);
				QCC_sref_t va_count = QCC_PR_GetSRef(type_float, pr_parm_argcount_name, pr_scope, true, 0, GDF_SCANLOCAL);
				QCC_sref_t numparms = QCC_MakeFloatConst(type->num_parms);
				va_passcount.sym->referenced = true;
				QCC_PR_SimpleStatement(&pr_opcodes[OP_SUB_F], va_passcount, numparms, va_count, false);
				QCC_FreeTemp(numparms);
				QCC_FreeTemp(va_passcount);
				QCC_FreeTemp(va_count);
			}
		}
	}
	if (f->type->aux_type->size > type_vector->size)
	if (!mergeargs)
	{	//okay, awkward... for large returns, the caller passed us a pointer via OFS_RETURN
		f->returndef = QCC_PR_GetSRef(QCC_PR_PointerType(f->type->aux_type), "ret**", f, true, 0, 0);
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_P], QCC_MakeSRefForce(&def_ret, 0, type_variant), f->returndef, nullsref, false);
	}

	if (type->vargs)
	{
		int i;
		int maxvacount = 24;
		QCC_sref_t a;
		//if we have opcode extensions, we can use those instead of via a function. this allows to use proper locals for the vargs.
		//otherwise we have to use static/globals instead, so that the child function can access them without clobbering. FIXME: this is silly. we should be able to bias our locals to avoid stomping the array access' locals
		pbool opcodeextensions = QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]) || QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]);
		QCC_sref_t va_list;
		va_list = QCC_PR_GetSRef(type_vector, "__va_list", pr_scope, true, maxvacount, GDF_SCANLOCAL|(opcodeextensions?0:GDF_STATIC));
		if (QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]) && !QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]))
			va_list.sym->arraylengthprefix = true;
		if (!mergeargs)
		{
			for (i = 0; i < maxvacount; i++)
			{
				QCC_ref_t varef;
				u = i + type->num_parms;
				if (u >= MAX_PARMS)
				{
					if (!extra_parms[u - MAX_PARMS].sym)
					{
						char name[128];
						QC_snprintfz(name, sizeof(name), "$parm%u", u);
						extra_parms[u - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
					}
					else
						QCC_ForceUnFreeDef(extra_parms[u - MAX_PARMS].sym);
					a = extra_parms[u - MAX_PARMS];
				}
				else
				{
					a.sym = &def_parms[u];
					a.ofs = 0;
					QCC_ForceUnFreeDef(a.sym);
				}
				a.cast = type_vector;
				QCC_UnFreeTemp(va_list);
				QCC_StoreSRefToRef(QCC_PR_BuildRef(&varef, REF_ARRAY, va_list, QCC_MakeIntConst(i*3), type_vector, false, 0), a, false, false);
			}
		}
		QCC_FreeTemp(va_list);
	}

	if (pif_flags & (PIF_WRAP|PIF_AUTOWRAP))
	{	//if we're wrapping, then we moved the old function entry to the end and reused it for our function.
		//so we need to define some local that refers to the prior def.
		int funcref = numfunctions-1;
		QCC_sref_t priorim = QCC_MakeUniqueConst(type_function, &funcref);
		priorim.sym->referenced = true;
		priorim.cast = f->type;
		prior = QCC_PR_DummyDef(f->type, "prior", f, 0, priorim.sym, 0, true, GDF_CONST|GDF_STATIC|GDF_SCANLOCAL);	//create a union into it
		prior->initialized = true;
		prior->filen = functions[numfunctions-1].filen;
		prior->s_filed = functions[numfunctions-1].s_filed;
		prior->s_line = functions[numfunctions-1].line;

		QCC_FreeTemp(priorim);
	}

	/*if (pr_classtype)
	{
		QCC_def_t *e, *e2;
		e = QCC_PR_GetDef(pr_classtype, "__oself", pr_scope, true, 0);
		e2 = QCC_PR_GetDef(type_entity, "self", NULL, true, 0);
		QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], QCC_PR_DummyDef(pr_classtype, "self", pr_scope, 0, e2->ofs, false), e, NULL));
	}*/

	lastparm = pr.local_tail;

//
// check for a state opcode
//
	if (QCC_PR_CheckToken ("["))
		QCC_PR_ParseState ();

	//accumulate implies wrap when the first function wasn't defined to accumulate (should really be explicit, but gmqcc compat)
	if (pif_flags & PIF_AUTOWRAP)
	{
		QCC_def_t *arg;
		QCC_sref_t args[MAX_PARMS+MAX_EXTRA_PARMS];
		QCC_sref_t r;
		unsigned int i;
		for (i=0, arg=pr.local_head.nextlocal ; i<type->num_parms; i++, arg = arg->deftail->nextlocal)
		{
			QCC_ForceUnFreeDef(arg);
			args[i].sym = arg;
			args[i].ofs = 0;
			args[i].cast = type->params[i].type;
		}
		r = QCC_PR_GenerateFunctionCallSref(nullsref, QCC_MakeSRefForce(prior, 0, prior->type), args, type->num_parms);
		prior->referenced = true;

		if (f->type->aux_type->type == ev_void)
			QCC_FreeTemp(r);
		else
		{
			if (!f->returndef.cast)
				f->returndef = QCC_PR_GetSRef(f->type->aux_type, "ret*", pr_scope, true, 0, 0);
			QCC_StoreToSRef(f->returndef, r, type, false, false);
		}
	}

	if (QCC_PR_CheckKeyword (keyword_asm, "asm"))
	{
		QCC_PR_Expect ("{");
		while (STRCMP ("}", pr_token))
			QCC_PR_ParseAsm ();
	}
	else
	{
		if (!(pif_flags & PIF_ACCUMULATE) && QCC_PR_CheckKeyword (keyword_var, "var"))	//reacc support
		{	//parse lots of locals
			char *name;
			do {
				name = QCC_PR_ParseName();
				QCC_PR_Expect(":");
				QCC_FreeDef(QCC_PR_GetDef(QCC_PR_ParseType(false, false, false), name, pr_scope, true, 0, false));
				QCC_PR_Expect(";");
			} while(!QCC_PR_CheckToken("{"));
		}
		else
			QCC_PR_Expect ("{");
//
// parse regular statements
//
		while (STRCMP ("}", pr_token))	//not check token to avoid the lex consuming following pragmas
		{
			QCC_PR_ParseStatement ();
			QCC_FreeTemps();
		}
	}

	QCC_FreeTemps();

	if (prior && !prior->referenced)
		QCC_PR_ParseError(ERR_REDECLARATION, "Wrapper function \"%s\" does not refer to its prior function", def->name);

	pr_token_line_last = pr_token_line;

	// this is cheap
//	if (type->aux_type->type)
//		if (statements[numstatements - 1].op != OP_RETURN)
//			QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );

	if (num_gotos)
	{
		int i, j;
		for (i = 0; i < num_gotos; i++)
		{
			for (j = 0; j < num_labels; j++)
			{
				if (!strcmp(pr_gotos[i].name, pr_labels[j].name))
				{
					if (!pr_opcodes[statements[pr_gotos[i].statementno].op].type_a)
						statements[pr_gotos[i].statementno].a.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					else if (!pr_opcodes[statements[pr_gotos[i].statementno].op].type_b)
						statements[pr_gotos[i].statementno].b.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					else
						statements[pr_gotos[i].statementno].c.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					break;
				}
			}
			if (j == num_labels)
			{
				num_gotos = 0;
				QCC_PR_ParseError(ERR_NOLABEL, "Goto statement with no matching label \"%s\"", pr_gotos[i].name);
			}
		}
		num_gotos = 0;
	}

	f->line_end = pr_token_line_last;
	if (1)//(pif_flags & PIF_ACCUMULATE) || pr_scope->parentscope)
	{	//FIXME: should probably always take this path, but kinda pointless until we have relocs for defs
		QCC_RemapLockedTemps(f->code, numstatements);
		QCC_Marshal_Locals(f->code, numstatements);
//		QCC_WriteAsmFunction(f, f->code, f->firstlocal);	//FIXME: this will print the entire function, not just the part that we added. and we'll print it all again later, too. should probably make it a function attribute that we check at the end.

		f->numstatements = numstatements - f->code;
		f->statements = qccHunkAlloc(sizeof(*statements)*f->numstatements);
		memcpy(f->statements, statements+f->code, sizeof(*statements) * f->numstatements);
		numstatements = f->code;
	}
	else
	{
		QCC_PR_FinaliseFunction(f);
	}
	pr_scope = NULL;

	if (num_labels)
		num_labels = 0;

	if (num_cases)
	{
		num_cases = 0;
		QCC_PR_ParseError(ERR_ILLEGALCASES, "%s: function contains illegal cases", f->name);
	}
	if (num_continues)
	{
		num_continues=0;
		QCC_PR_ParseError(ERR_ILLEGALCONTINUES, "%s: function contains illegal continues", f->name);
	}
	if (num_breaks)
	{
		num_breaks=0;
		QCC_PR_ParseError(ERR_ILLEGALBREAKS, "%s: function contains illegal breaks", f->name);
	}

	//clean up the locals. remove parms from the hashtable but don't clean subscoped_away so that we can repopulate on the next accumulation
	for (d = f->firstlocal; d != lastparm->nextlocal; d = d->nextlocal)
	{
		if (!d->subscoped_away)
			pHash_RemoveData(&localstable, d->name, d);
	}
	//any non-arg locals defined within the accumulation should not be visible next time around.
	for (; d; d = d->nextlocal)
	{
		if (!d->subscoped_away)
		{
			pHash_RemoveData(&localstable, d->name, d);
			d->subscoped_away = true;
		}
	}
#if 0//def _DEBUG
	for (u = 0; u < localstable.numbuckets; u++)
	{
		if (localstable.bucket[u])
			localstable.bucket[u] = NULL;
	}
#endif

	for (; startingtypes < numtypeinfos; startingtypes++)
	{
		if (qcc_typeinfo[startingtypes].typedefed)
		{
			qcc_typeinfo[startingtypes].typedefed = false;
			pHash_RemoveData(&typedeftable, qcc_typeinfo[startingtypes].name, &qcc_typeinfo[startingtypes]);
		}
	}

	QCC_PR_Lex();

	return f;
}

static void QCC_PR_ArrayRecurseDivideRegular(QCC_sref_t array, QCC_sref_t index, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	int stride;

	if (array.cast->type == ev_vector)
		stride = 3;
	else
		stride = 1;	//struct arrays should be 1, so that every element can be accessed...

	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(min+1), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, array, nullsref, &st);
		st->a.ofs += min*stride;
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(mid+1), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArrayRecurseDivideRegular(array, index, min, mid);
		if (st)
			st->b.jumpofs = numstatements - (st-statements);
		QCC_PR_ArrayRecurseDivideRegular(array, index, mid, max);
	}
}

//the idea here is that we return a vector, the caller then figures out the extra 3rd.
//This is useful when we have a load of indexes.
static void QCC_PR_ArrayRecurseDivideUsingVectors(QCC_sref_t array, QCC_sref_t index, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(min+1), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, array, nullsref, &st);
		st->a.ofs += min*3;
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(mid+1), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArrayRecurseDivideUsingVectors(array, index, min, mid);
		if (st)
			st->b.jumpofs = numstatements - (st-statements);
		QCC_PR_ArrayRecurseDivideUsingVectors(array, index, mid, max);
	}
}

//returns a vector overlapping the result needed.
QCC_def_t *QCC_PR_EmitArrayGetVector(QCC_sref_t array)
{
	QCC_sref_t temp, index;
	QCC_def_t *func;

	int numslots;

	QCC_type_t *ftype = qccHunkAlloc(sizeof(*ftype));
	struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*1);
	ftype->size = 1;
	ftype->type = ev_function;
	ftype->aux_type = type_vector;
	ftype->params = fparms;
	ftype->num_parms = 1;
	ftype->name = "ArrayGet";
	fparms[0].type = type_float;

	//array shouldn't ever be a vector array
	numslots = array.sym->arraysize*array.cast->size;
	numslots = (numslots+2)/3;

	s_filen = array.sym->filen;
	s_filed = array.sym->s_filed;
	func = QCC_PR_GetDef(ftype, qcva("ArrayGetVec*%s", array.sym->name), NULL, true, 0, false);

	pr_source_line = pr_token_line_last = array.sym->s_line;	//thankfully these functions are emitted after compilation.

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	pr_scope = QCC_PR_GenerateQCFunction(func, ftype, NULL);
	pr_source_line = pr_token_line_last = pr_scope->line = array.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->filen = array.sym->filen;
	pr_scope->s_filed = array.sym->s_filed;
	func->symboldata[0]._int = pr_scope - functions;

	index = QCC_PR_GetSRef(type_float, "index___", pr_scope, true, 0, false);
	index.sym->referenced = true;
	temp = QCC_PR_GetSRef(type_float, "div3___", pr_scope, true, 0, false);
	QCC_PR_SimpleStatement(pr_opcodes+OP_DIV_F, index, QCC_MakeFloatConst(3), temp, false);
	QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, temp, temp, temp, false);//round down to int

	QCC_PR_ArrayRecurseDivideUsingVectors(array, temp, 0, numslots);

	QCC_PR_Statement(pr_opcodes+OP_RETURN, QCC_MakeVectorConst(0,0,0), nullsref, NULL);	//err... we didn't find it, give up.
	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);	//err... we didn't find it, give up.

	func->initialized = 1;


	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();

	return func;
}

void QCC_PR_EmitArrayGetFunction(QCC_def_t *scope, QCC_def_t *arraydef, char *arrayname)
{
	QCC_sref_t vectortrick;
	QCC_sref_t index, thearray = QCC_MakeSRefForce(arraydef, 0, arraydef->type);

	QCC_statement_t *st;
	QCC_sref_t eq;
	QCC_statement_t *bc1=NULL, *bc2=NULL;

//	QCC_sref_t fasttrackpossible = nullsref;
	int numslots;

	numslots = thearray.sym->arraysize;
	if (!numslots)
		numslots = 1;
	if (thearray.cast->type != ev_vector)
		numslots *= thearray.cast->size;

//	if (flag_fasttrackarrays && numslots > 6)
//		fasttrackpossible = QCC_PR_GetSRef(type_float, "__ext__fasttrackarrays", NULL, true, 0, false);

	s_filen = scope->filen;
	s_filed = scope->s_filed;

	vectortrick = nullsref;
//	if (numslots >= 15 && thearray.cast->type != ev_vector)
//	{
//		vectortrick.sym = QCC_PR_EmitArrayGetVector(thearray);
//		vectortrick.cast = vectortrick.sym->type;
//	}

	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type, NULL);
	pr_source_line = pr_token_line_last = pr_scope->line = thearray.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->filen = thearray.sym->filen;
	pr_scope->s_filed = thearray.sym->s_filed;

	index = QCC_PR_GetSRef(type_float, "__indexg", pr_scope, true, 0, GDF_PARAMETER);

	scope->initialized = true;
	scope->symboldata[0]._int = pr_scope - functions;

/*	if (fasttrackpossible)
	{
		QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, fasttrackpossible, nullsref, &st);
		//fetch_gbl takes: (float size, variant array[]), float index, variant pos
		//note that the array size is coded into the globals, one index before the array.

		if (thearray.cast->type == ev_vector)
			QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_V], thearray, index, sref_ret, true);
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_F], thearray, index, sref_ret, true);

		QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_RETURN], sref_ret, nullsref, NULL));

		//finish the jump
		st->b.ofs = &statements[numstatements] - st;
	}
*/

	if (flag_boundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), nullsref, &bc1));

	if (vectortrick.cast)
	{
		QCC_sref_t div3, intdiv3, ret;

		//okay, we've got a function to retrieve the var as part of a vector.
		//we need to work out which part, x/y/z that it's stored in.
		//0,1,2 = i - ((int)i/3 *) 3;

		if (flag_boundchecks)
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_GE_F, index, QCC_MakeFloatConst(numslots), NULL, STFL_PRESERVEA), nullsref, &bc2));

		div3 = QCC_PR_GetSRef(type_float, "div3___", pr_scope, true, 0, false);
		intdiv3 = QCC_PR_GetSRef(type_float, "intdiv3___", pr_scope, true, 0, false);

		div3.sym->referenced = true;
		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, index, index, index, false);
		QCC_PR_SimpleStatement(pr_opcodes+OP_DIV_F, index, QCC_MakeFloatConst(3), div3, false);
		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, div3, div3, intdiv3, false);

//		ret = QCC_PR_GenerateFunctionCall1(nullsref, floor, index, type_float);

		QCC_UnFreeTemp(index);
		ret = QCC_PR_GenerateFunctionCall1(nullsref, vectortrick, index, type_float);

		div3 = QCC_PR_Statement(pr_opcodes+OP_MUL_F, intdiv3, QCC_MakeFloatConst(3), NULL);
		QCC_PR_SimpleStatement(pr_opcodes+OP_SUB_F, index, div3, index, false);
		QCC_FreeTemp(div3);

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		ret.cast = type_float;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(1+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(2+0.5), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;
		ret.ofs-=3;
		QCC_FreeTemp(ret);
	}
	else
	{
//		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, index, index, index, false);
		QCC_PR_ArrayRecurseDivideRegular(thearray, index, 0, numslots);
	}

	if (bc1)
		bc1->b.jumpofs = &statements[numstatements] - bc1;
	if (bc2)
		bc2->b.jumpofs = &statements[numstatements] - bc2;
	if (bc1 || bc2)
	{
		QCC_sref_t errfnc = QCC_PR_GetSRef(NULL, "error", NULL, false, 0, false);
		QCC_sref_t sprintffnc = QCC_PR_GetSRef(NULL, "sprintf", NULL, false, 0, false);
		QCC_sref_t errmsg;
		if (sprintffnc.cast)
		{	//if sprintf is defined, we generate a more verbose error message. this message should appear in at least one of the engines the mod was made for, and in others it should be obivious enough.
			QCC_sref_t args[3];
			args[0] = QCC_MakeStringConst("bounds check failed (0 <= %g < %g is false)\n");
			QCC_UnFreeTemp(index);
			args[1] = index;
			args[2] = QCC_MakeFloatConst(numslots);
			errmsg = QCC_PR_GenerateFunctionCallSref(nullsref, sprintffnc, args, 3);
		}
		else
			errmsg = QCC_MakeStringConst("bounds check failed\n");
		if (!errfnc.cast)
		{
			errfnc = QCC_MakeIntConst(~0);
			errfnc.cast = type_function;
		}
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall1(nullsref, errfnc, errmsg, type_string));
	}
	QCC_FreeTemp(index);
	//we get here if they tried reading beyond the end of the array with bounds checks disabled. just return the last valid element.
	if (thearray.cast->type == ev_vector)
		thearray.ofs += (numslots-1)*3;
	else
		thearray.ofs += (numslots-1);
	QCC_PR_Statement(pr_opcodes+OP_RETURN, thearray, nullsref, &st);
	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);

	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();
}

static void QCC_PR_ArraySetRecurseDivide(QCC_sref_t array, QCC_sref_t index, QCC_sref_t value, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	int stride;

	if (array.cast->type == ev_vector)
		stride = 3;
	else
		stride = 1;	//struct arrays should be 1, so that every element can be accessed...

	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(min+1), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 3;
		if (stride == 3)
			QCC_PR_StatementFlags(pr_opcodes+OP_STORE_V, value, array, &st, STFL_PRESERVEB);
		else
			QCC_PR_StatementFlags(pr_opcodes+OP_STORE_F, value, array, &st, STFL_PRESERVEB);
		st->b.ofs += min*stride;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, nullsref, nullsref, NULL);
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(mid+1), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArraySetRecurseDivide(array, index, value, min, mid);
		if (st)
			st->b.jumpofs = numstatements - (st-statements);
		QCC_PR_ArraySetRecurseDivide(array, index, value, mid, max);
	}
}

void QCC_PR_EmitArraySetFunction(QCC_def_t *scope, QCC_def_t *arraydef, char *arrayname)
{
	QCC_sref_t index, value, thearray = QCC_MakeSRefForce(arraydef, 0, arraydef->type);

	QCC_sref_t fasttrackpossible;
	int numslots;
	QCC_statement_t *bc1=NULL, *bc2=NULL;

	if (thearray.cast->type == ev_vector)
		numslots = thearray.sym->arraysize;
	else
		numslots = thearray.sym->arraysize*thearray.cast->size;

	fasttrackpossible = nullsref;
	if (flag_fasttrackarrays && numslots > 6)
		fasttrackpossible = QCC_PR_GetSRef(type_float, "__ext__fasttrackarrays", NULL, true, 0, false);

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	s_filen = arraydef->filen;
	s_filed = arraydef->s_filed;
	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type, NULL);
	pr_source_line = pr_token_line_last = pr_scope->line = thearray.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->filen = thearray.sym->filen;
	pr_scope->s_filed = thearray.sym->s_filed;

	index = QCC_PR_GetSRef(type_float, "indexs___", pr_scope, true, 0, GDF_PARAMETER);
	value = QCC_PR_GetSRef(thearray.cast, "value___", pr_scope, true, 0, GDF_PARAMETER);

	scope->initialized = true;
	scope->symboldata[0]._int = pr_scope - functions;

	if (fasttrackpossible.cast)
	{
		QCC_statement_t *st;

		QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, fasttrackpossible, nullsref, &st);
		//note that the array size is coded into the globals, one index before the array.

		QCC_PR_SimpleStatement(&pr_opcodes[OP_CONV_FTOI], index, nullsref, index, true);	//address stuff is integer based, but standard qc (which this accelerates in supported engines) only supports floats
		if (flag_boundchecks)
			QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], index, QCC_MakeSRef(NULL, numslots, NULL), nullsref, true);//annoy the programmer. :p
		if (thearray.cast->type == ev_vector)//shift it upwards for larger types
			QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_I], index, QCC_MakeIntConst(thearray.cast->size), index, true);
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GLOBALADDRESS], thearray, index, index, true);	//comes with built in add
		if (thearray.cast->type == ev_vector)
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], value, index, nullsref, true);	//*b = a
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], value, index, nullsref, true);
		QCC_PR_Statement(&pr_opcodes[OP_RETURN], value, nullsref, NULL);

		//finish the jump
		st->b.jumpofs = &statements[numstatements] - st;
	}

	if (flag_boundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), nullsref, &bc1));
	if (flag_boundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_GE_F, index, QCC_MakeFloatConst(numslots), NULL, STFL_PRESERVEA), nullsref, &bc2));

	QCC_PR_ArraySetRecurseDivide(thearray, index, value, 0, numslots);

	if (bc1)
		bc1->b.jumpofs = &statements[numstatements] - bc1;
	if (bc2)
		bc2->b.jumpofs = &statements[numstatements] - bc2;
	if (bc1 || bc2)
	{
		QCC_sref_t errfnc = QCC_PR_GetSRef(NULL, "error", NULL, false, 0, false);
		QCC_sref_t errmsg = QCC_MakeStringConst("bounds check failed\n");
		if (!errfnc.cast)
		{
			errfnc = QCC_MakeIntConst(~0);
			errfnc.cast = type_function;
		}
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall1(nullsref, errfnc, errmsg, type_string));
	}

	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);



	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();
}

//register a def, and all of it's sub parts.
//only the main def is of use to the compiler.
//the subparts are emitted to the compiler and allow correct saving/loading
//be careful with fields, this doesn't allocated space, so will it allocate fields. It only creates defs at specified offsets.
QCC_def_t *QCC_PR_DummyDef(QCC_type_t *type, const char *name, QCC_function_t *scope, int arraysize, QCC_def_t *rootsymbol, unsigned int ofs, int referable, unsigned int flags)
{
	char array[64];
	char newname[256];
	int a;
	QCC_def_t *def, *first=NULL;
	char typebuf[1024];
	pbool redec;

	while (rootsymbol && rootsymbol->symbolheader != rootsymbol)
	{
		ofs += rootsymbol->ofs;
		rootsymbol = rootsymbol->symbolheader;
	}

#define KEYWORD(x) if (!STRCMP(name, #x) && keyword_##x) do{if (keyword_##x)QCC_PR_ParseWarning(WARN_KEYWORDDISABLED, "\""#x"\" keyword used as variable name%s", keywords_coexist?" - coexisting":" - disabling");keyword_##x=keywords_coexist;}while(0)
	if (name)
	{
		KEYWORD(var);
		KEYWORD(thinktime);
		KEYWORD(for);
		KEYWORD(switch);
		KEYWORD(case);
		KEYWORD(local);
		KEYWORD(default);
		KEYWORD(goto);
		if (type->type != ev_function)
			KEYWORD(break);
		KEYWORD(continue);
		KEYWORD(state);
		KEYWORD(string);
		if (qcc_targetformat != QCF_HEXEN2 && qcc_targetformat != QCF_UHEXEN2)
			KEYWORD(float);	//hmm... hexen2 requires this...
		KEYWORD(entity);
		KEYWORD(vector);
		KEYWORD(const);
		KEYWORD(asm);
	}

	if (!type)
		return NULL;

	for (a = -1; a < arraysize||a==-1; a++)
	{
		if (a == -1)
			*array = '\0';
		else
			QC_snprintfz(array, sizeof(array), "[%i]", a);

		if (name)
			QC_snprintfz(newname, sizeof(newname), "%s%s", name, array);
		else
			QC_snprintfz(newname, sizeof(newname), "%s", array);

		// allocate a new def
		if (a == -1 && rootsymbol && !rootsymbol->symbolsize)
		{	//we had a prototype, but now we get to define everything else.
			def = rootsymbol;
			referable = false; //should already be added.
			redec = true;

			if (def->constant != !!(flags & GDF_CONST) ||
				def->isstatic != !!(flags & GDF_STATIC) ||
				def->isparameter != !!(flags & GDF_PARAMETER) ||
				def->autoderef != !!(flags & GDF_AUTODEREF))
				QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "%s%s%s %s%s%s redeclared with different storage type", col_type,TypeName(type, typebuf, sizeof(typebuf)),col_none, col_symbol,name,col_none);
		}
		else
		{
			def = (void *)qccHunkAlloc (sizeof(QCC_def_t));
			memset (def, 0, sizeof(*def));
			def->next = NULL;

			def->name = (void *)qccHunkAlloc (strlen(newname)+1);
			strcpy (def->name, newname);
			redec = false;
		}
		def->arraysize = a>=0?0:arraysize;

		def->s_line = pr_source_line;
		def->unitn = s_unitn;
		def->filen = s_filen;
		def->s_filed = s_filed;
		if (a>=0)
			def->initialized = 1;

		def->type = type;

		def->scope = scope;
		def->constant = !!(flags & GDF_CONST);
		def->isstatic = !!(flags & GDF_STATIC);
		def->isparameter = !!(flags & GDF_PARAMETER);
		def->autoderef = !!(flags & GDF_AUTODEREF);	//should be a pointer type...
		if (arraysize && a < 0)
		{	//array headers can be stripped safely.
			def->saved = false;
			def->strip = true;
			def->arraylengthprefix = !rootsymbol && !def->localscope && QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]);	//only prefix arrays with a length if its not an embedded symbol, its not a (true)local, and if there's actually a point to doing it.
		}
		else
		{
			def->saved = (!arraysize || a>=0) && !!(flags & GDF_SAVED);	//saved is never set on the head def in an array.
			def->strip = !!(flags & GDF_STRIP);
			def->arraylengthprefix = false;
		}
		def->allowinline = !!(flags & GDF_INLINE);

		if (flags & GDF_USED)
		{
			def->nostrip = true;
			def->used = true;
			def->referenced = true;
		}

		def->ofs = ofs + ((a>0)?type->size*a:0);
		if (!first)
			first = def;
		if (!rootsymbol || (def==rootsymbol && !rootsymbol->symboldata))
		{
			if (!rootsymbol)
			{
				rootsymbol = first;
				rootsymbol->deftail = rootsymbol;
			}
			if ((flags & GDF_POSTINIT) || arraysize<0 || !type->size)
				rootsymbol->symboldata = NULL;
			else
				rootsymbol->symboldata = qccHunkAlloc ((def->arraysize?def->arraysize:1) * type->size * sizeof(float));
		}

		if (redec)
			;	//symbol already linked.
		else if (rootsymbol != def && !(flags&GDF_ALIAS))
		{	//we're inserting the symbol into the 'middle' of the parent after the fact. so this is kinda messy.
			if (!rootsymbol->deftail)
				rootsymbol->deftail = rootsymbol;
			def->next = rootsymbol->deftail->next;
			rootsymbol->deftail->next = def;
			if (pr.def_tail == rootsymbol->deftail)
				pr.def_tail = def;	//urgh

			if (scope)// && !(flags & (GDF_CONST|GDF_STATIC)))
			{	//constants are never considered locals. static (local) variables are also not counted as locals as they're logically globals in all regards other than visibility.

				//just insert it at the start. who cares.
				def->nextlocal = rootsymbol->deftail->nextlocal;
				rootsymbol->deftail->nextlocal = def;
				if (pr.local_tail == rootsymbol->deftail)
					pr.local_tail = def;
				def->localscope = true;
			}

			rootsymbol->deftail = def;
			first->deftail = def;
		}
		else
		{
			pr.def_tail->next = def;
			pr.def_tail = def;
			first->deftail = def;

			if (scope)// && !(flags & (GDF_CONST|GDF_STATIC)))
			{	//constants are never considered locals. static (local) variables are also not counted as locals as they're logically globals in all regards other than visibility.
				pr.local_tail->nextlocal = def;
				pr.local_tail = def;
				def->localscope = true;
			}
		}

		def->symbolheader = rootsymbol;
		if (arraysize < 0 || !type->size)
			break;	//don't define the rest if the struct/union ain't defined properly yet

		def->symboldata = (rootsymbol->symboldata?rootsymbol->symboldata + def->ofs:NULL);
		if (type->bits)
			def->symbolsize = (((def->arraysize?def->arraysize:1) * ((type->bits+7)/8))+(VMWORDSIZE-1))/VMWORDSIZE;
		else
			def->symbolsize = (def->arraysize?def->arraysize:1) * type->size;

		if ((type->type == ev_struct||type->type == ev_union) && (!arraysize || a>=0))
		{
			unsigned int partnum;
			QCC_type_t *parttype;
			def->saved = false;	//struct headers don't get saved.
			for (partnum = 0; partnum < (type->type == ev_union?max(1,type->num_parms):type->num_parms); partnum++)
			{
				parttype = type->params[partnum].type;
				while (parttype->type == ev_accessor)
					parttype = parttype->parentclass;
				switch (parttype->type)
				{
				case ev_vector:
					QC_snprintfz(newname, sizeof(newname), "%s.%s", def->name, type->params[partnum].paramname);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags);

//					QC_snprintfz(newname, sizeof(newname), "%s.%s_x", def->name, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs - rootsymbol->ofs, false, flags | GDF_CONST);
//					QC_snprintfz(newname, sizeof(newname), "%s.%s_y", def->name, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs+1 - rootsymbol->ofs, false, flags | GDF_CONST);
//					QC_snprintfz(newname, sizeof(newname), "%s.%s_z", def->name, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs+2 - rootsymbol->ofs, false, flags | GDF_CONST);
					break;

				case ev_accessor:	//shouldn't happen.
				case ev_enum:
				case ev_float:
				case ev_double:
				case ev_boolean:
				case ev_string:
				case ev_entity:
				case ev_field:
				case ev_pointer:
				case ev_integer:
				case ev_uint:
				case ev_int64:
				case ev_uint64:
				case ev_struct:
				case ev_union:
				case ev_variant:	//for lack of any better alternative
					if (type->params[partnum].paramname)
						QC_snprintfz(newname, sizeof(newname), "%s.%s", def->name, type->params[partnum].paramname);
					else	//anon or something (eg array types).
						QC_snprintfz(newname, sizeof(newname), "%s", def->name);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags);
					break;

				case ev_function:
					QC_snprintfz(newname, sizeof(newname), "%s.%s", def->name, parttype->name);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags)->initialized = true;
					break;
				case ev_bitfld:	//FIXME: breaks saved games
				case ev_void:
					break;
				case ev_typedef:	//invalid
					QCC_PR_ParseWarning(ERR_INTERNAL, "unexpected typedef");
					break;
				}
			}
		}
		else if (type->type == ev_vector && !arraysize)
		{	//do the vector thing.
			QC_snprintfz(newname, sizeof(newname), "%s_x", def->name);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+0, referable, (flags&~GDF_SAVED) | GDF_STRIP);
			QC_snprintfz(newname, sizeof(newname), "%s_y", def->name);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+1, referable, (flags&~GDF_SAVED) | GDF_STRIP);
			QC_snprintfz(newname, sizeof(newname), "%s_z", def->name);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+2, referable, (flags&~GDF_SAVED) | GDF_STRIP);
		}
		else if (type->type == ev_field)
		{
			if (type->aux_type->type == ev_vector && !arraysize && *def->name != ':')
			{
				//do the vector thing.
				QC_snprintfz(newname, sizeof(newname), "%s_x", def->name);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+0, referable, flags);
				QC_snprintfz(newname, sizeof(newname), "%s_y", def->name);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+1, referable, flags);
				QC_snprintfz(newname, sizeof(newname), "%s_z", def->name);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+2, referable, flags);
			}
		}
	}

	if (referable)
	{
//		if (!arraysize && first->type->type != ev_field)
//			first->constant = false;
		if (scope)
			pHash_Add(&localstable, first->name, first, qccHunkAlloc(sizeof(bucket_t)));
		else
			pHash_Add(&globalstable, first->name, first, qccHunkAlloc(sizeof(bucket_t)));

		if (!scope && asmfile)
			fprintf(asmfile, "%s %s;\n", TypeName(first->type, typebuf, sizeof(typebuf)), first->name);
	}

	return first;
}

/*
============
PR_GetDef

If type is NULL, it will match any type
If arraysize=0, its not an array and has 1 element.
If arraysize>0, its an array and requires array notation
If arraysize<0, its an array with undefined size - GetDef will fail if its not already allocated.

If allocate is 0, will only get the def
If allocate is 1, a new def will be allocated if it can't be found
If allocate is 2, a new def will be allocated, and it'll error if there's a dupe with scope (for ensuring that arguments are created properly)
============
*/

QCC_def_t *QCC_PR_GetDef (QCC_type_t *type, const char *name, struct QCC_function_s *scope, pbool allocate, int arraysize, unsigned int flags)
{
	int ofs;
	QCC_def_t		*def;
//	char element[MAX_NAME];
	QCC_def_t *foundstatic = NULL;
	char typebuf1[1024], typebuf2[1024];
	int ins, insmax;

	if (!allocate)
		arraysize = -1;
	else if (!strncmp(name, "autocvar_", 9))
	{
		if (scope)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined with local scope. promoting to global.", name);
		else if (flags & GDF_CONST)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined as constant. attempting to correct that for you.", name);
		else if (flags & GDF_STATIC)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined as static. attempting to correct that for you.", name);
		scope = NULL;
		flags &= ~(GDF_CONST|GDF_STATIC);
		if (!(flags & GDF_STRIP))
			flags |= GDF_USED;	//aka nostrip
	}

	if (pHash_Get != &Hash_Get)
	{
		ins = 0;
		insmax = allocate?1:2;
	}
	else
	{
		ins = 1;
		insmax = 2;
	}
	for (; ins < insmax; ins++)
	{
		if (scope)
		{
			//FIXME: should we be scanning the locals list instead, and remove the localstable?
			def = pHash_Get(&localstable, name);

			while(def)
			{
				//ignore differing case the first time around.
				if (ins == 0 && strcmp(def->name, name))
				{
					def = pHash_GetNext(&localstable, name, def);
					continue;		// in a different function
				}

				if ( def->scope && def->scope != scope)
				{
					struct QCC_function_s *pscope = NULL;
					if (def->isstatic)
					{
						for (pscope = scope->parentscope; pscope; pscope = pscope->parentscope)
							if (def->scope == pscope)
								break;
					}
					if (!pscope)
					{
						def = pHash_GetNext(&localstable, name, def);
						continue;		// in a different function
					}
				}

				if (type && typecmp(def->type, type))
				{
					if (scope && allocate && pr_subscopedlocals)
					{	//assume it was defined in a different subscope. hopefully we'll start favouring the new one until it leaves subscope.
						def = pHash_GetNext(&localstable, name, def);
						continue;
					}
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Type mismatch on redeclaration of %s%s%s. %s%s%s, should be %s%s%s",col_symbol,name,col_none, col_type,TypeName(type, typebuf1, sizeof(typebuf1)),col_none, col_type,TypeName(def->type, typebuf2, sizeof(typebuf2)),col_none);
				}
				if (def->arraysize != arraysize && arraysize>=0)
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHARRAYSIZE, def, "Array sizes for redecleration of %s%s%s do not match (%i -> %i)",col_symbol,name,col_none, def->arraysize,arraysize);
				if (allocate && scope && !(flags & GDF_STATIC))
				{
					if (pr_subscopedlocals)
					{	//subscopes mean that the later one replaces the first, hopefully.
						def = pHash_GetNext(&localstable, name, def);
						continue;
					}
					if (allocate == 2)
						QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Duplicate definition of %s%s%s.", col_symbol,name,col_none);
					if (def->isstatic)
						QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "nonstatic redeclaration of %s%s%s ignored", col_symbol,name,col_none);
					else
						QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "%s%s%s duplicate definition ignored", col_symbol,name,col_none);
					QCC_PR_ParsePrintDef(WARN_DUPLICATEDEFINITION, def);
	//				if (!scope)
	//					QCC_PR_ParsePrintDef(def);
				}
				else if (allocate && (flags & GDF_STATIC) && !def->isstatic)
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "static redefinition of %s%s%s follows non-static definition.", col_symbol,name,col_none);

				QCC_ForceUnFreeDef(def);
				return def;
			}
		}

		def = pHash_Get(&globalstable, name);

		while(def)
		{
			//ignore differing case the first time around.
			if (ins == 0 && strcmp(def->name, name))
			{
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			if ( (def->scope || (scope && allocate)) && def->scope != scope)
			{
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			//ignore it if its static in some other file.
			if ((def->isstatic||(flags&GDF_STATIC)) && strcmp(def->unitn, scope?scope->unitn:s_unitn))
			{
				if (!foundstatic)
					foundstatic = def;	//save it off purely as a warning.
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			if (def->assumedtype && !(flags & GDF_BASICTYPE))
			{
				if (allocate)
				{	//if we're asserting a type for it in some def then it'll no longer be assumed.
					if (def->type->type != type->type)
						QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Type mismatch on redeclaration of %s%s%s. Basic types are different.",col_symbol,name,col_none);
					def->type = type;
					def->assumedtype = false;
					def->filen = s_filen;
					def->s_line = pr_source_line;
					if (flags & GDF_CONST)
						def->constant = true;
				}
				else
				{	//we know enough of its type to write it out, but not enough to actually use it safely, so pretend this def isn't defined yet.
					def = pHash_GetNext(&globalstable, name, def);
					continue;
				}
			}

			if (type && typecmp(def->type, type))
			{
				if (pr_scope || typecmp_lax(def->type, type))
				{
					if (!pr_scope && (
						!strcmp("droptofloor", def->name)	||	//vanilla
						!strcmp("callfunction", def->name)	||	//should be (..., string name) but dpextensions gets this wrong.
						!strcmp("trailparticles", def->name)	//dp got the two arguments the wrong way. fteqw doesn't care any more, but dp is still wrong.
						))
					{
						//this is a hack. droptofloor was wrongly declared in vanilla qc, which causes problems with replacement extensions.qc.
						//yes, this is a selfish lazy hack for this, there's probably a better way, but at least we spit out a warning still.
						QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "%s builtin was wrongly redefined as %s. ignoring later definition",name, TypeName(type, typebuf1, sizeof(typebuf1)));
						QCC_PR_ParsePrintDef(WARN_COMPATIBILITYHACK, def);
					}
					else
					{
						int flen = strlen(s_filen);
						if (!pr_scope && flen >= 13 && !QC_strcasecmp(s_filen+flen-13, "extensions.qc") && def->type->type == ev_function)
						{
							//this is a hack. droptofloor was wrongly declared in vanilla qc, which causes problems with replacement extensions.qc.
							//yes, this is a selfish lazy hack for this, there's probably a better way, but at least we spit out a warning still.
							QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "%s builtin was redefined as %s. ignoring alternative definition",name, TypeName(type, typebuf1, sizeof(typebuf1)));
							QCC_PR_ParsePrintDef(WARN_COMPATIBILITYHACK, def);
						}
						else if (def->unused && !def->referenced && allocate && !def->scope)
						{	//previous def was norefed and still wasn't used yet.
							QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "Type redeclaration of %s %s replaces existing variable", TypeName(type, typebuf1, sizeof(typebuf1)), name);
							QCC_PR_ParsePrintDef(WARN_COMPATIBILITYHACK, def);
							def = pHash_GetNext(&localstable, name, def);
							continue;
						}
						else
						{
							if (type->type == ev_function && type->vargs && !type->num_parms && def->type->type == ev_function)
								;	//c89-style dumb redeclarations...
							else	//unequal even when we're lax
								QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Type mismatch on redeclaration of %s%s%s. %s%s%s, should be %s%s%s",col_symbol,name,col_none, col_type,TypeName(type, typebuf1, sizeof(typebuf1)),col_none, col_type,TypeName(def->type, typebuf2, sizeof(typebuf2)),col_none);
						}
					}
				}
				else
				{
					if (type->type != ev_function || type->num_parms != def->type->num_parms || !(def->type->vargs && !type->vargs))
					{
						//if the second def simply has no ..., don't bother warning about it.

						QCC_PR_ParseWarning (WARN_TYPEMISMATCHREDECOPTIONAL, "Optional arguments differ on redeclaration of %s%s%s. %s%s%s, should be %s%s%s",col_symbol,name,col_none, col_type,TypeName(type, typebuf1, sizeof(typebuf1)),col_none, col_type,TypeName(def->type, typebuf2, sizeof(typebuf2)),col_none);
						QCC_PR_ParsePrintDef(WARN_TYPEMISMATCHREDECOPTIONAL, def);

						if (type->type == ev_function)
						{
							//update the def's type to the new one if the mandatory argument count is longer
							//FIXME: don't change the param names!
							if (type->num_parms > def->type->num_parms)
								def->type = type;
						}
					}
				}
			}
			if (def->arraysize != arraysize && arraysize>=0)
			{
				if (allocate && !def->symbolsize)
					QCC_PR_DummyDef(def->type, name, def->scope, arraysize, def, 0, true, flags);
				else
					QCC_PR_ParseErrorPrintDef(ERR_TYPEMISMATCHARRAYSIZE, def, "Array sizes for redecleration of %s do not match (%i->%i)",name, def->arraysize,arraysize);
			}
			if (!def->symbolsize && def->arraysize >= 0)	//variables are allowed to be declared (with their addresses taken) before struct types are even defined.
				QCC_PR_DummyDef(def->type, name, def->scope, def->arraysize, def, 0, true, flags);

			if (allocate && scope && !(flags & GDF_STATIC))
			{
				if (allocate == 2)
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Duplicate definition of %s.", name);
				if (pr_scope)
				{	//warn? or would that be pointless?
					def = pHash_GetNext(&globalstable, name, def);
					continue;		// in a different function
				}

				if (def->isstatic)
					QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "nonstatic redeclaration of %s ignored", name);
				else
					QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "%s duplicate definition ignored", name);
				QCC_PR_ParsePrintDef(WARN_DUPLICATEDEFINITION, def);
	//			if (!scope)
	//				QCC_PR_ParsePrintDef(def);
			}
			QCC_ForceUnFreeDef(def);
			return def;
		}
	}

	if (foundstatic && !allocate && !(flags & GDF_SILENT))
	{
		QCC_PR_ParseWarning (WARN_SAMENAMEASGLOBAL, "%s defined static", name);
		QCC_PR_ParsePrintDef(WARN_SAMENAMEASGLOBAL, foundstatic);
	}

	if (flags & GDF_SCANLOCAL)
	{
		for (def = pr.local_head.nextlocal; def; def = def->nextlocal)
		{
			if (!strcmp(def->name, name))
			{
				if (allocate && def->arraysize != arraysize)
					continue;
				if (allocate)
				{	//relink it
					pHash_Add(&localstable, name, def, qccHunkAlloc(sizeof(bucket_t)));
					def->subscoped_away = false;
				}
				QCC_ForceUnFreeDef(def);
				return def;
			}
		}
	}

	if (!allocate)
		return NULL;

	if (scope && qccwarningaction[WARN_SAMENAMEASGLOBAL])
	{
		def = QCC_PR_GetDef(NULL, name, NULL, false, arraysize, GDF_SILENT);
		if (def && def->type->type == type->type)
		{	//allow type differences. this means that arguments called 'min' or 'mins' are accepted with the 'min' builtin or the 'mins' field in existance.
			QCC_PR_ParseWarning(WARN_SAMENAMEASGLOBAL, "Local \"%s\" hides global with same name and type", name);
			QCC_PR_ParsePrintDef(WARN_SAMENAMEASGLOBAL, def);
		}
		QCC_FreeDef(def);
	}

	ofs = 0;

	def = QCC_PR_DummyDef(type, name, scope, arraysize, NULL, ofs, true, flags);

	QCC_ForceUnFreeDef(def);
	return def;
}
QCC_sref_t QCC_PR_GetSRef (QCC_type_t *type, const char *name, QCC_function_t *scope, pbool allocate, int arraysize, unsigned int flags)
{
	QCC_sref_t sr;
	QCC_def_t *def = QCC_PR_GetDef(type, name, scope, allocate, arraysize, flags);
	if (def)
	{
		sr.sym = def;
		sr.cast = def->type;
		sr.ofs = 0;
		if (def->deprecated)
		{
			if (*def->deprecated)	//we have a reason for it
				QCC_PR_ParseWarning(pr_ignoredeprecation?WARN_MUTEDEPRECATEDVARIABLE:WARN_DEPRECATEDVARIABLE, "Variable \"%s\" is deprecated: %s", def->name, def->deprecated);
			else		//we don't have any reason for it.
				QCC_PR_ParseWarning(pr_ignoredeprecation?WARN_MUTEDEPRECATEDVARIABLE:WARN_DEPRECATEDVARIABLE, "Variable \"%s\" is deprecated", def->name);
		}
		return sr;
	}
	return nullsref;
}

//.union {};
static QCC_def_t *QCC_PR_DummyFieldDef(QCC_type_t *type, QCC_function_t *scope, int arraysize, unsigned int *fieldofs, unsigned int saved)
{
	char array[64];
	char newname[256];
	int a, parms, o;
	QCC_def_t *def, *first=NULL;
	unsigned int maxfield, startfield;
	QCC_type_t *ftype;
	pbool isunion;
	startfield = *fieldofs;
	maxfield = startfield;

	for (a = 0; a < (arraysize?arraysize:1); a++)
	{
		if (a == 0)
			*array = '\0';
		else
			QC_snprintfz(array, sizeof(array), "[%i]", a);

//	externs->Printf("Emited %s\n", newname);

		if ((type)->type == ev_struct||(type)->type == ev_union)
		{
			int memberalen;
			int partnum;
			QCC_type_t *parttype;
			isunion = ((type)->type == ev_union);
			for (partnum = 0, parms = (type)->num_parms; partnum < parms; partnum++)
			{
				parttype = type->params[partnum].type;
				while(parttype->type == ev_accessor)
					parttype = parttype->parentclass;

				memberalen = type->params[partnum].arraysize;

				switch (parttype->type)
				{
				case ev_union:
				case ev_struct:
					if (!*type->params[partnum].paramname)
					{	//recursively generate new fields
						QC_snprintfz(newname, sizeof(newname), "%s%s", type->params[partnum].paramname, array);
						def = QCC_PR_DummyFieldDef(parttype, scope, memberalen, fieldofs, saved);
						break;
					}
					//fallthrough. any named structs will become global structs that contain field references. hopefully.
				case ev_enum:
				case ev_accessor:
				case ev_float:
				case ev_double:
				case ev_boolean:
				case ev_string:
				case ev_vector:
				case ev_entity:
				case ev_field:
				case ev_pointer:
				case ev_integer:
				case ev_uint:
				case ev_int64:
				case ev_uint64:
				case ev_variant:
				case ev_function:
					if (!*type->params[partnum].paramname)
					{
						QCC_PR_ParseWarning(WARN_CONFLICTINGUNIONMEMBER, "nameless field union/struct generating nameless def.");
						break;
					}
					QC_snprintfz(newname, sizeof(newname), "%s%s", type->params[partnum].paramname, array);
					ftype = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
					ftype->aux_type = parttype;
					if (parttype->type == ev_vector)
						ftype->size = parttype->size;	//vector fields create a _y and _z too, so we need this still.
					def = QCC_PR_GetDef(NULL, newname, scope, false, memberalen, saved);
					if (!def)
					{
						def = QCC_PR_GetDef(ftype, newname, scope, true, memberalen, saved);
						if (parttype->type == ev_function)
							def->initialized = true;
						for (o = 0; o < parttype->size*(memberalen?memberalen:1); o++)
							def->symboldata[o]._int = *fieldofs + o;
						*fieldofs += parttype->size*(memberalen?memberalen:1);
					}
					else
					{
						QCC_PR_ParseWarning(WARN_CONFLICTINGUNIONMEMBER, "conflicting offsets for nameless union/struct expansion of %s. Ignoring new def.", newname);
						QCC_PR_ParsePrintDef(WARN_CONFLICTINGUNIONMEMBER, def);
						//hcc just PR_GetDefs the fields. it allocates field space as part of the def, which is skipped if it already exists.
						//so don't update fieldofs, because that would result in incompatibilities.
					}
					QCC_FreeDef(def);
					break;
				case ev_void:
				case ev_bitfld:	//FIXME: breaks saved games
					break;
				case ev_typedef:	//invalid
					QCC_PR_ParseWarning(ERR_INTERNAL, "unexpected typedef");
					break;
				}
				if (*fieldofs > maxfield)
					maxfield = *fieldofs;
				if (isunion)
					*fieldofs = startfield;
			}
		}
	}

	*fieldofs = maxfield;	//final size of the union.
	return first;
}



static void QCC_PR_ExpandUnionToFields(QCC_type_t *type, unsigned int *fields)
{
	QCC_type_t *pass = type->aux_type;
	QCC_PR_DummyFieldDef(pass, pr_scope, 1, fields, GDF_SAVED|GDF_CONST);
}

//copies tmp into def
//FIXME: is basedef redundant?
//FIXME: is type redundant?
static pbool QCC_PR_GenerateInitializerType(QCC_def_t *basedef, QCC_sref_t tmp, QCC_sref_t def, QCC_type_t *type, unsigned bitofs, unsigned int flags)
{
	pbool ret = true;
	unsigned i;

	def.ofs += bitofs>>5;
	bitofs&=31;

	if (bitofs && !type->bits)
		QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "'%s' is not word aligned", basedef?basedef->name:"<unknown>");
	if (basedef && (!basedef->scope || basedef->constant || basedef->isstatic))
	{
		if (!tmp.sym->constant)
		{
			if (basedef->scope && !basedef->isstatic && !basedef->initialized)
			{
//				QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "initializer for '%s' is not constant", basedef->name);
//				QCC_PR_ParsePrintSRef(WARN_GMQCC_SPECIFIC, tmp);
//				basedef->constant = false;
				goto finalnotconst;
			}
			QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "initializer for '%s' is not constant", basedef->name);
			QCC_PR_ParsePrintSRef(ERR_BADIMMEDIATETYPE, tmp);
		}
		else
		{
			tmp.sym->referenced = true;
			if ((!basedef->scope||basedef->isstatic) && tmp.sym->reloc)
			{
				QCC_def_t *dt;
				if (!flag_pointerrelocs)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "preinitialised pointer variables is disabled for this target");
				dt = QCC_PR_DummyDef(type_pointer, "$reloc", basedef->scope, 0, def.sym, def.ofs, false, GDF_CONST);
				dt->reloc = tmp.sym->reloc;
				dt->referenced = true;
				dt->constant = 1;
				dt->initialized = 1;
				dt->strip = true;	//engine does need to know it... but there should be some other def that we're mapping intohere.

				for (i = 0; (unsigned)i < type->size; i++)
					QCC_SRef_DataWord(def, i)->_int = QCC_SRef_DataWord(tmp, i)->_int;//tmp.ofs + tmp.sym->reloc->symbolheader->arraylengthprefix;

				QCC_FreeTemp(tmp);
				return ret;
			}
			if ((!basedef->scope||basedef->isstatic) && def.cast->type==ev_pointer && tmp.sym->type->type==ev_string)
			{
				QCC_def_t *dt;
				dt = QCC_PR_DummyDef(type_string, "$reloc", basedef->scope, 0, def.sym, def.ofs, false, GDF_CONST);
				dt->reloc = tmp.sym->reloc;
				dt->referenced = true;
				dt->constant = 1;
				dt->initialized = 1;
				dt->strip = true;	//engine does need to know it... but there should be some other def that we're mapping intohere.

				for (i = 0; (unsigned)i < type->size; i++)
					QCC_SRef_DataWord(def, i)->_int = QCC_SRef_DataWord(tmp, i)->_int;//tmp.ofs + tmp.sym->reloc->symbolheader->arraylengthprefix;

				QCC_FreeTemp(tmp);
				return ret;
			}

			if (basedef->initialized && !basedef->unused && !(flags & PIF_STRONGER))
			{	//dupe initialisation. compare the two.

				if (!tmp.sym->initialized)
				{
					//FIXME: we NEED to support relocs somehow
					QCC_PR_ParseWarning(WARN_UNINITIALIZED, "initializer is not initialised, %s will be treated as 0", QCC_GetSRefName(tmp));
					QCC_PR_ParsePrintSRef(WARN_UNINITIALIZED, tmp);
				}

				for (i = 0; (unsigned)i < type->size; i++)
					if (QCC_SRef_DataWord(def,i)->_int != QCC_SRef_DataWord(tmp,i)->_int)
					{
						if (!def.sym->arraysize && def.cast->type == ev_function && !strcmp(def.sym->name, "parseentitydata") && (functions[QCC_SRef_DataWord(def, i)->_int].builtin == 608 || functions[QCC_SRef_DataWord(def, i)->_int].builtin == 613))
						{	//dpextensions is WRONG, and claims it to be 608. its also too common, so lets try working around that.
							if (functions[QCC_SRef_DataWord(def, i)->_int].builtin == 608)
								functions[QCC_SRef_DataWord(def, i)->_int].builtin = 613;
							QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "incompatible redeclaration. Please validate builtin numbers. parseentitydata is #613");
							QCC_PR_ParsePrintSRef(WARN_COMPATIBILITYHACK, tmp);
						}
						else if (!def.sym->arraysize && def.cast->type == ev_function && functions[QCC_SRef_DataWord(def, i)->_int].code>-1 && functions[QCC_SRef_DataWord(tmp, i)->_int].code==-1)
						{
							QCC_PR_ParseWarning (WARN_COMPATIBILITYHACK, "incompatible redeclaration. Ignoring replacement of qc function with builtin.");
							QCC_PR_ParsePrintSRef(WARN_COMPATIBILITYHACK, tmp);
						}
						else
							QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "incompatible redeclaration");
					}
			}
			else
			{
				const int *srcdata = (const void*)QCC_SRef_EvalConst(tmp);
				if (!srcdata)
				{
					if ((!basedef->scope||basedef->isstatic) && def.cast->type == ev_function && def.sym->symboldata==basedef->symboldata)
					{	//set to a function which is not yet initialised. insert a function reloc at this location, so we can update it once it is actually known.
						for (i = 0; (unsigned)i < type->size; i++)
						{
							QCC_def_t *dt;
							dt = QCC_PR_DummyDef(type_function, "$relocf", basedef->scope, 0, def.sym, def.ofs, false, GDF_CONST);
							dt->reloc = tmp.sym;
							dt->referenced = true;
							dt->constant = 1;
							dt->initialized = 1;
							dt->strip = true;	//hide it. engine doesn't need to know.

							QCC_SRef_DataWord(def, i)->_int = 0;
						}
					}
					else
					{
						QCC_PR_ParseWarning(WARN_NOTCONSTANT, "initializer for %s is not initialised yet, %s will be treated as 0", QCC_GetSRefName(def), QCC_GetSRefName(tmp));
						QCC_PR_ParsePrintSRef(WARN_NOTCONSTANT, tmp);

						for (i = 0; (unsigned)i < type->size; i++)
							QCC_SRef_DataWord(def, i)->_int = 0;
					}
				}
				else if (type->bits)
				{	//a small type/bitfield...
					unsigned int old, new;
					if (bitofs + type->bits > 32)
						QCC_PR_ParseWarning(ERR_INTERNAL, "bitfield cross 32bit boundary");
					old = (QCC_SRef_DataWord(def, bitofs>>5)->_int&~(((1u<<type->bits)-1)<<bitofs));
					new = (srcdata[0] & ((1u<<type->bits)-1)) << bitofs;
					QCC_SRef_DataWord(def, bitofs>>5)->_int = old|new;
				}
				else
				{
					for (i = 0; (unsigned)i < type->size; i++)
						QCC_SRef_DataWord(def, i)->_int = srcdata[i];
				}
			}
		}
	}
	else
	{
		QCC_sref_t rhs;
		pbool nullsource;
finalnotconst:
		rhs = tmp;
		nullsource = QCC_SRef_IsNull(rhs);
		if (def.sym->initialized)
			QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "%s initialised twice", basedef->name);
		else if (type->bits)
		{
			if (bitofs + type->bits > 32)
				QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "%s dynamically initialised (%i bit)", basedef->name, type->bits);
			rhs = QCC_PR_Statement_BitCopy(nullsource?QCC_MakeVectorConst(0,0,0):rhs, bitofs, type->bits, def);
			if (rhs.sym == def.sym && rhs.ofs == def.ofs && rhs.cast == def.cast)
			{
				QCC_FreeTemp(rhs);
				return ret;
			}
			//else still need to copy it.
		}

		ret = 0;
		for (i = 0; (unsigned)i < type->size; )
		{
			if (type->size - i >= 3)
			{
				rhs.cast = def.cast = type_vector;
				if (type->size - i == 3)
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], nullsource?QCC_MakeVectorConst(0,0,0):rhs, def, NULL, STFL_PRESERVEB));
					return ret;
				}
				else
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], nullsource?QCC_MakeVectorConst(0,0,0):rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				i+=3;
				def.ofs += 3;
				rhs.ofs += 3;
			}
			else if (type->size - i >= 2)
			{
				rhs.cast = def.cast = type_vector;
				if (type->size - i == 2)
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], nullsource?QCC_MakeVectorConst(0,0,0):rhs, def, NULL, STFL_PRESERVEB));
					return ret;
				}
				else
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], nullsource?QCC_MakeVectorConst(0,0,0):rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				i+=2;
				def.ofs += 2;
				rhs.ofs += 2;
			}
			else
			{
				if (nullsource)
					rhs = QCC_MakeIntConst(0);

				if (def.cast->type == ev_function)
				{
					rhs.cast = def.cast = type_function;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else if (def.cast->type == ev_string)
				{
					rhs.cast = def.cast = type_string;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_S], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_S], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else if (def.cast->type == ev_entity)
				{
					rhs.cast = def.cast = type_entity;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else if (def.cast->type == ev_field)
				{
					rhs.cast = def.cast = type_field;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else if (def.cast->type == ev_integer || def.cast->type == ev_uint)
				{
					rhs.cast = def.cast = type_integer;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else if (def.cast->type == ev_int64 || def.cast->type == ev_uint64 || def.cast->type == ev_double)
				{
					rhs.cast = def.cast = type_int64;
					if (type->size - i == 2)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I64], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				else
				{
					rhs.cast = def.cast = type_float;
					if (type->size - i == 1)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], rhs, def, NULL, STFL_PRESERVEB));
						return ret;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
				}
				i++;
				def.ofs++;
				rhs.ofs++;
			}
		}
	}
	QCC_FreeTemp(tmp);
	return ret;
}

QCC_sref_t QCC_PR_ParseInitializerType_Internal(int arraysize, QCC_def_t *basedef, QCC_sref_t def, unsigned int bitofs, unsigned int flags)
{
	QCC_sref_t tmp;
	int i;
	pbool ret = true;

	if (arraysize)
	{
		if (pr_token_type == tt_immediate && pr_immediate_type == type_string && def.cast->type == ev_bitfld && def.cast->bits == 8)
		{
			if (pr_immediate_strlen > arraysize)	//problem!
				QCC_PR_ParseWarning (ERR_BADARRAYSIZE, "initializer-string too long");

			for (i = 0; i < arraysize; i++)
			{
				tmp = QCC_MakeIntConst((i < pr_immediate_strlen)?pr_immediate_string[i]:0);
				tmp = QCC_EvaluateCast(tmp, def.cast, true);
			//	QCC_ForceUnFreeDef(tmp.sym);
				ret &= QCC_PR_GenerateInitializerType(basedef, tmp, def, def.cast, bitofs, flags);
				bitofs += def.cast->bits;
			}
			QCC_PR_Lex();
		}
		else
		{
			//arrays go recursive
			QCC_PR_Expect("{");
			if (!QCC_PR_CheckToken("}"))
			{
				for (i = 0; i < arraysize; i++)
				{
					ret &= QCC_PR_ParseInitializerType(0, basedef, def, bitofs, flags);
					if (def.cast->bits)
						bitofs += def.cast->bits;
					else
						def.ofs += def.cast->size;
					if (!QCC_PR_CheckToken(","))
					{
						QCC_PR_Expect("}");
						break;
					}
					if (QCC_PR_CheckToken("}"))
						break;
				}
			}
		}
	}
	else
	{
		QCC_type_t *type = def.cast;
		if (type->type == ev_function && pr_token_type == tt_punct)
		{
			/*begin function special case*/
			QCC_function_t *parentfunc = pr_scope;
			QCC_function_t *f;
			char fname[256];
			const char *defname = QCC_GetSRefName(def);

			tmp = nullsref;
			*fname = 0;

			if (QCC_PR_CheckToken ("#") || QCC_PR_CheckToken (":"))
			{
				int binum = 0;
				if (pr_token_type == tt_immediate
				&& pr_immediate_type == type_float
				&& pr_immediate._float == (int)pr_immediate._float)
					binum = (int)pr_immediate._float;
				else if (pr_token_type == tt_immediate && pr_immediate_type == type_integer)
					binum = pr_immediate._int;
				else if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
					QC_strlcpy(fname, pr_immediate_string, sizeof(fname));
				else if (pr_token_type == tt_name)
					QC_strlcpy(fname, pr_token, sizeof(fname));
				else 
					QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
				QCC_PR_Lex();

				if (!*fname && QCC_PR_CheckToken (":"))
					QC_strlcpy(fname, QCC_PR_ParseName(), sizeof(fname));

				//if the builtin already exists, just use that dfunction instead
				if (basedef && basedef->initialized)
				{
					if (*fname)
					{
						for (i = 1; i < numfunctions; i++)
						{
							if (functions[i].code == -1 && functions[i].builtin == binum)
							{
								if (!*functions[i].name)
								{
									functions[i].name = qccHunkAlloc(strlen(fname)+1);
									strcpy(functions[i].name, fname);
								}
								if (!strcmp(functions[i].name, fname))
								{
									tmp = QCC_MakeIntConst(i);
									break;
								}
							}
						}
					}
					else
					{
						for (i = 1; i < numfunctions; i++)
						{
							if (functions[i].code == -1 && functions[i].builtin == binum)
							{
								if (!*functions[i].name || !strcmp(functions[i].name, defname))
								{
									tmp = QCC_MakeIntConst(i);
									break;
								}
							}
						}
					}
				}

				if (!tmp.cast)
					f = QCC_PR_GenerateBuiltinFunction(def.sym, binum, *fname?fname:def.sym->name);
				else
					f = NULL;
			}
			else if (QCC_PR_PeekToken("{") || QCC_PR_PeekToken("["))
			{
				if (basedef)
				{
					if (flags&PIF_WRAP)
					{
						if (!basedef->initialized || !QCC_SRef_Data(def)->_int)
							QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "wrapper function does not wrap anything");
					}
					else if (basedef->initialized == 1 && !(flags & PIF_STRONGER))
					{
						//normally this is an error, but to aid supporting new stuff with old, we convert it into a warning if a vanilla(ish) qc function replaces extension builtins.
						//the qc function is the one that is used, but there is a warning so you know how to gain efficiency.
						int bi = -1;
						if (def.cast->type == ev_function && !arraysize)
						{
							if (!strcmp(defname, "anglemod") || !strcmp(defname, "crossproduct"))
								bi = QCC_SRef_Data(def)->_int;
						}
						if (bi <= 0 || bi >= numfunctions)
							bi = 0;
						else
							bi = functions[bi].code;
						if (bi < 0)
						{
							QCC_PR_ParseWarning(WARN_NOTSTANDARDBEHAVIOUR, "%s already declared as a builtin", defname);
							QCC_PR_ParsePrintSRef(WARN_NOTSTANDARDBEHAVIOUR, def);
							basedef->unused = true;
						}
						else
						{
							QCC_PR_ParseWarning (ERR_REDECLARATION, "redeclaration of function body");
							QCC_PR_ParsePrintSRef(WARN_NOTSTANDARDBEHAVIOUR, def);
						}
					}
				}

				if (pr_scope)
				{
//					QCC_PR_ParseErrorPrintSRef (ERR_INITIALISEDLOCALFUNCTION, def, "initialisation of function body within function body");
					//save some state of the parent
					QCC_def_t *firstlocal = pr.local_head.nextlocal;
					QCC_def_t *lastlocal = pr.local_tail;
					QCC_function_t *parent = pr_scope;
					QCC_statement_t *patch;

					//FIXME: make sure gotos/labels/cases/continues/breaks are not broken by this.

					//generate a goto statement around the nested function, so that nothing is hurt.
					patch = QCC_Generate_OP_GOTO();
					f = QCC_PR_ParseImmediateStatements (def.sym->isstatic?def.sym:NULL, type, flags&PIF_WRAP);
					patch->a.jumpofs = &statements[numstatements] - patch;
					if (patch->a.jumpofs == 1)
						numstatements--;	//never mind then.

					//make sure parent state is restored properly.
					pr.local_head.nextlocal = firstlocal;
					pr.local_tail = lastlocal;
					pr_scope = parent;
				}
				else
					f = QCC_PR_ParseImmediateStatements (def.sym, type, flags&PIF_WRAP);

				//allow dupes if its a builtin
				if (basedef && !f->code && basedef->initialized)
				{
					for (i = 1; i < numfunctions; i++)
					{
						if (functions[i].code == -f->builtin)
						{
							tmp = QCC_MakeIntConst(i);
							break;
						}
					}
				}
			}
			else
			{
				f = NULL;
				tmp = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				tmp = QCC_EvaluateCast(tmp, type, true);
			}
			if (!tmp.cast && f)
			{	
				if (type->aux_type->size > 3 && !autoprototype)	//fixme: handle properly, without breaking __out
					QCC_PR_ParseWarning(WARN_SLOW_LARGERETURN, "Function %s returns (large) %s. This is inefficient.", def.sym->name, TypeName(type->aux_type, fname,sizeof(fname)));

				pr_scope = parentfunc;
				tmp = QCC_MakeIntConst(f - functions);

				if (!basedef && def.sym->temp)
				{	//skip the store-to-temp
//					QCC_FreeTemp(def);
					tmp.cast = def.cast;
					return tmp;
				}
			}
		}
		else if (type->type == ev_string && QCC_PR_CheckName("_"))
		{
			char trname[128];
			QCC_PR_Expect("(");
			if (pr_token_type != tt_immediate || pr_immediate_type->type != ev_string)
				QCC_PR_ParseError(0, "_() intrinsic accepts only a string immediate");
			if (!pr_scope || basedef->constant || basedef->isstatic)
			{
				//if this is a static initialiser, embed a dotranslate in there
				QCC_SRef_Data(def)->_int = QCC_CopyString (pr_immediate_string);

				if (!pr_scope || def.sym->constant)
				{
					QCC_def_t *dt;
					QC_snprintfz(trname, sizeof(trname), "dotranslate_%i", ++dotranslate_count);
					dt = QCC_PR_DummyDef(type_string, trname, pr_scope, 0, def.sym, def.ofs, true, GDF_CONST);
					dt->referenced = true;
					dt->constant = 1;
					dt->initialized = 1;
				}
				QCC_PR_Lex();
				QCC_PR_Expect(")");
				return ret?def:nullsref;
			}
			tmp = QCC_MakeTranslateStringConst(pr_immediate_string);
			QCC_PR_Lex();
			QCC_PR_Expect(")");
		}
		else if (type->type == ev_union && type->num_parms == 1 && !type->params->paramname)
		{	//weird typedefed array hack
			def.cast = (type)->params[0].type;
			ret &= QCC_PR_ParseInitializerType((type)->params[0].arraysize, basedef, def, bitofs, flags);
			def.cast = type;
			return ret?def:nullsref;
		}
		else if ((type->type == ev_struct || type->type == ev_union) && QCC_PR_CheckToken("{"))
		{
			//structs go recursive
			QCC_type_t *parenttype;
			unsigned int partnum;
			pbool isunion;
			gofs_t offset = def.ofs;
			gofs_t reloffset;
			unsigned int *isinited = alloca(sizeof(*isinited)*type->size);
			const char *mname;
			struct QCC_typeparam_s *tp;
			memset(isinited, 0, sizeof(pbool)*type->size);

			if (QCC_PR_PeekToken("."))	//won't be a float (.0 won't match)
			{	//designated initialisers
				for(;;)
				{
					if (QCC_PR_CheckToken("."))
					{
						mname = QCC_PR_ParseName();
						QCC_PR_Expect("=");
						tp = QCC_PR_FindStructMember(type, mname, &def.ofs, &reloffset);
						if (isinited[def.ofs] & (1u<<reloffset))
							QCC_PR_ParseError (ERR_EXPECTED, "%s.%s was already ininitialised", type->name, mname);
						isinited[def.ofs] |= 1u<<reloffset;
						def.ofs += offset;
						def.cast = tp->type;

						ret &= QCC_PR_ParseInitializerType(tp->arraysize, basedef, def, bitofs+reloffset, flags);
					}
					else
						QCC_PR_ParseError (ERR_EXPECTED, "designated initialisers require all members to be initialised the same way");

					if (QCC_PR_CheckToken("}"))
						break;
					QCC_PR_Expect(",");
				}
				partnum = 0;
			}
			else if (type->parentclass)
			{
				if (!QCC_PR_CheckToken("}"))
					QCC_PR_ParseError (ERR_EXPECTED, "inherited structs must use designated initialisers");
			}
			else
			{
				//FIXME: inheritance makes stuff weird
//				int i;
				isunion = ((type)->type == ev_union);
				for (partnum = 0; partnum < (type)->num_parms; partnum++)
				{
					if (QCC_PR_CheckToken("}"))
						break;
					if ((type)->params[partnum].isvirtual)
						continue;	//these are pre-initialised....
//					if ((type)->params[partnum].optional)
//						continue;	//float parts of a vector.

					def.cast = (type)->params[partnum].type;
					def.ofs = (type)->params[partnum].ofs;
					if (isinited[def.ofs] & (1<<(type)->params[partnum].bitofs))
					{
						QCC_PR_ParseError (ERR_EXPECTED, "member %s.%s was already ininitialised", type->name, (type)->params[partnum].paramname);
						continue;
					}
					isinited[def.ofs] |= (1<<(type)->params[partnum].bitofs);
					/*i = (type)->params[partnum].arraysize;
					if (!i)
						i = 1;
					if ((type)->params[partnum].type->bits)
						;
					else
					{
						i *= (type)->params[partnum].type->size;
						while(i --> 0)
							isinited[def.ofs+i] |= ~0;
					}*/
					def.ofs += offset;

					ret &= QCC_PR_ParseInitializerType((type)->params[partnum].arraysize, basedef, def, bitofs + (type)->params[partnum].bitofs, flags);
					if (isunion || !QCC_PR_CheckToken(","))
					{
						QCC_PR_Expect("}");
						break;
					}
				}
			}

			//anything not already set needs to be filled with a default value.
			for (parenttype=type; parenttype; parenttype = parenttype->parentclass)
			{
				for (partnum = 0; partnum < (parenttype)->num_parms; partnum++)
				{
					//copy into the def. should be from a const.
					def.cast = (parenttype)->params[partnum].type;
					def.ofs = (parenttype)->params[partnum].ofs;
					if (isinited[def.ofs] & (1<<(parenttype)->params[partnum].bitofs))
						continue;
					isinited[def.ofs] |= (1<<(parenttype)->params[partnum].bitofs);
					def.ofs += offset;
					if ((parenttype)->params[partnum].defltvalue.cast)
						tmp = (parenttype)->params[partnum].defltvalue;
					else
						tmp = QCC_MakeIntConst(0);
					QCC_ForceUnFreeDef(tmp.sym);
					QCC_PR_GenerateInitializerType(basedef, tmp, def, def.cast, bitofs, flags);
				}
			}
			def.cast = type;
			def.ofs = offset;
			return ret?def:nullsref;
		}
		else if (type->type == ev_vector && QCC_PR_PeekToken("{"))
		{	//vectors can be treated as an array of 3 floats.
			def.cast = type_float;
			ret &= QCC_PR_ParseInitializerType(3, basedef, def, bitofs, flags);
			def.cast = type;
			return ret?def:nullsref;
		}
		else if (type->type == ev_pointer && QCC_PR_CheckToken("{"))
		{
			//generate a temp array
			QCC_ref_t buf, buf2;
			tmp.sym = QCC_PR_DummyDef(type->aux_type, NULL, pr_scope, 0, NULL, 0, false, GDF_STRIP|(pr_scope?GDF_STATIC:0));
			tmp.ofs = 0;
			tmp.cast = tmp.sym->type;

			tmp.sym->refcount+=1;

			//fill up the array
			do
			{
				//expand the array
				unsigned int newsize = tmp.sym->arraysize * tmp.cast->size;
				if (tmp.sym->symbolsize < newsize)
				{
					void *newdata;
					newsize += 64 * tmp.cast->size;
					newdata = qccHunkAlloc (newsize * sizeof(float));
					memcpy(newdata, tmp.sym->symboldata, tmp.sym->symbolsize*sizeof(float));
					tmp.sym->symboldata = newdata;
					tmp.sym->symbolsize = newsize;
				}
				tmp.sym->arraysize++;
				//generate the def...
				QCC_PR_DummyDef(tmp.cast, NULL, pr_scope, 0, tmp.sym, tmp.ofs, false, GDF_STRIP|(pr_scope?GDF_STATIC:0));

				//and fill it in.
				ret &= QCC_PR_ParseInitializerType(0, tmp.sym, tmp, bitofs, flags);
				tmp.ofs += type->aux_type->size;
			} while(QCC_PR_CheckToken(","));
			QCC_PR_Expect("}");

			//drop the size back down to something sane
			tmp.sym->symbolsize = tmp.ofs*sizeof(float);

			//grab the address of it.
			tmp.ofs = 0;
			tmp = QCC_RefToDef(QCC_PR_GenerateAddressOf(&buf, QCC_DefToRef(&buf2, tmp)), true);
		}
		else
		{
			tmp = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			tmp = QCC_EvaluateCast(tmp, type, true);
		}

		ret = QCC_PR_GenerateInitializerType(basedef, tmp, def, type, bitofs, flags);
	}
	return ret?def:nullsref;
}
QCC_sref_t QCC_PR_ParseInitializerTemp(QCC_type_t *type)
{
	QCC_sref_t def = QCC_GetTemp(type);
	QCC_sref_t imm;
	imm = QCC_PR_ParseInitializerType_Internal(0, NULL, def, 0, 0);
	if (imm.cast)
	{
		if (imm.cast != type)
			QCC_PR_ParseError(ERR_INTERNAL, "QCC_PR_ParseInitializerTemp changed type\n");
		QCC_FreeTemp(def);
		return imm;	//just use the immediate.
	}
	return def;	//use our silly temp.
}
//returns true where its a const/static initialiser. false if non-const/final initialiser
pbool QCC_PR_ParseInitializerType(int arraysize, QCC_def_t *basedef, QCC_sref_t def, unsigned bitofs, unsigned int flags)
{
	if (QCC_PR_ParseInitializerType_Internal(arraysize, basedef, def, bitofs, flags).cast)
		return true;
	return false;
}

void QCC_PR_ParseInitializerDef(QCC_def_t *def, unsigned int flags)
{
	pr_ignoredeprecation = !!def->deprecated;	//mute deprecation warnings if the symbol we're defining has its own warning.
	if (QCC_PR_ParseInitializerType(def->arraysize, def, QCC_MakeSRef(def, 0, def->type), 0, flags))
		if (!def->initialized)
			def->initialized = 1;
	pr_ignoredeprecation = false;
	QCC_FreeDef(def);
}
QCC_sref_t QCC_PR_ParseDefaultInitialiser(QCC_type_t *type)
{
	QCC_sref_t ref = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
	if (!ref.sym->constant)
		QCC_PR_ParseError(0, "Default value not a constant\n");
	if (!ref.sym->initialized)
	{
		if (autoprototype)
			return nullsref;
		QCC_PR_ParseError(0, "Default value not initialized yet\n");
	}
	return QCC_EvaluateCast(ref, type, true);
}

int accglobalsblock;	//0 = error, 1 = var, 2 = function, 3 = objdata


QCC_type_t *QCC_PR_ParseEnum(pbool flags)
{
	const char *name = NULL;
	QCC_sref_t		sref;
	puint64_t next_i = flags?1:0;
	double next_f = next_i;
	struct accessor_s *acc;
	QCC_type_t *enumtype = NULL, *basetype;
	pbool strictenum = false;
	basetype = (flag_assume_integer?type_integer:type_float);

	if (!QCC_PR_CheckToken("{"))
	{
		QCC_type_t *type;

		strictenum = QCC_PR_CheckName("class");	//c++11 style

		type = autoprototyped?NULL:QCC_PR_ParseType(false, true, false);	//legacy behaviour
		if (type)
		{
			QCC_PR_ParseWarning(WARN_DEPRECACTEDSYNTAX, "legacy enum base type. Use \"enum [class] [name_e]:type\" instead");
			basetype = type;
		}
		else
		{
			if (pr_token_type == tt_name)
				name = QCC_PR_ParseName();
			else
				name = NULL;
			if (QCC_PR_CheckToken(":"))
			{
				basetype = QCC_PR_ParseType(false, false, false);
				if (!basetype)
					QCC_PR_ParseError(ERR_NOTATYPE, "enumflags - must be numeric type");
			}
			else if (strictenum)
				QCC_PR_Expect(":");
		}
		QCC_PR_Expect("{");
	}

	if (name && !enumtype)
	{
		enumtype = QCC_TypeForName(name);
		if (!enumtype)
		{
			if (strictenum)
			{
				enumtype = QCC_PR_NewType(name, basetype->type, true);
				enumtype->aux_type = basetype;
				enumtype->type = ev_enum;
			}
			else
				enumtype = QCC_PR_NewType(name, basetype->type, true);
			enumtype->size = basetype->size;
		}
	}

	while(1)
	{
		name = QCC_PR_ParseName();
		if (QCC_PR_CheckToken("="))
		{
			/*if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_float)
			{
				iv = fv = pr_immediate._float;
				QCC_PR_Lex();
			}
			else if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_integer)
			{
				fv = iv = pr_immediate._int;
				QCC_PR_Lex();
			}
			else*/
			{
				const QCC_eval_t *eval;
				sref = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				sref = QCC_SupplyConversion(sref, basetype->type, true);
				eval = QCC_SRef_EvalConst(sref);
				if (eval)
				{
					if (sref.cast->type == ev_float)
						next_i = next_f = eval->_float;
					else if (sref.cast->type == ev_double)
						next_i = next_f = eval->_double;
					else if (sref.cast->type == ev_integer)
						next_f = (int)(next_i = eval->_int);
					else if (sref.cast->type == ev_uint)
						next_f = next_i = eval->_uint;
					else if (sref.cast->type == ev_int64)
						next_f = (longlong)(next_i = eval->i64);
					else if (sref.cast->type == ev_uint64)
						next_f = next_i = eval->u64;
				}
				else if (sref.sym)
					QCC_PR_ParseError(ERR_NOTANUMBER, "enum - %s is not a compile-time constant", sref.sym->name);
				else
					QCC_PR_ParseError(ERR_NOTANUMBER, "enum - not a number");

				//do this, because we can. with any luck we'll just hit the same const anyway, and if not then we may have managed to avoid hitting a global.
				if (basetype->type==ev_integer)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeIntConst(next_i);
				}
				else if (basetype->type==ev_uint)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeUIntConst(next_i);
				}
				else if (basetype->type==ev_int64)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeInt64Const(next_i);
				}
				else if (basetype->type==ev_uint64)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeUInt64Const(next_i);
				}
				else if (basetype->type==ev_float)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeFloatConst(next_f);
				}
				else if (basetype->type==ev_double)
				{
					QCC_FreeTemp(sref);
					sref = QCC_MakeDoubleConst(next_f);
				}
			}
		}
		else
		{
			if (basetype->type==ev_integer)
				sref = QCC_MakeIntConst(next_i);
			else if (basetype->type==ev_uint)
				sref = QCC_MakeUIntConst(next_i);
			else if (basetype->type==ev_int64)
				sref = QCC_MakeInt64Const(next_i);
			else if (basetype->type==ev_uint64)
				sref = QCC_MakeUInt64Const(next_i);
			else if (basetype->type==ev_float)
				sref = QCC_MakeFloatConst(next_f);
			else if (basetype->type==ev_double)
				sref = QCC_MakeDoubleConst(next_f);
			else
				QCC_PR_ParseError(ERR_NOTANUMBER, "values for enums of this type must be initialised");
		}
		if (flags)
		{
			int bits = 0;
			puint64_t i;
			if (basetype->type == ev_float)
				i = (longlong)next_f;
			else if (basetype->type == ev_double)
				i = (longlong)next_f;
			else if (basetype->type == ev_integer)
				i = (int)next_i;
			else if (basetype->type == ev_uint)
				i = (unsigned int)next_i;
			else if (basetype->type == ev_int64)
				i = (longlong)next_i;
			else if (basetype->type == ev_uint64)
				i = (unsigned longlong)next_i;
			else
				QCC_PR_ParseError(ERR_NOTATYPE, "enumflags - must be numeric type");
			if (basetype->type!=ev_integer && (double)i != next_f)
				QCC_PR_ParseWarning(WARN_ENUMFLAGS_NOTINTEGER, "enumflags - %f not an integer value", next_f);
			else
			{
				while(i)
				{
					if (((i>>1u)<<1u) != i)
						bits++;
					i>>=1u;
				}
				if (bits > 1)	//be mute about 0.
					QCC_PR_ParseWarning(WARN_ENUMFLAGS_NOTINTEGER, "enumflags - %#"pPRIx64"(%"pPRIi64") has multiple bits set", next_i, next_i);
			}
		}

		sref.sym->referenced = true;

		//value gets added to global pool too (but referable whenever not strict)
		//we just generate an entirely new def (within the parent's pr_globals allocation). this also gives 'symbol was defined HERE' info.
		sref.sym = QCC_PR_DummyDef(sref.cast, name, pr_scope, 0, sref.sym, sref.ofs, !strictenum, GDF_CONST|GDF_STRIP);
		sref.sym->initialized = true;	//must be true for it to have been considered a compile-time constant.
		sref.ofs = 0;
		if (enumtype)
		{
			//generate enumname::valname symbol info
			for (acc = enumtype->accessors; acc; acc = acc->next)
				if (!strcmp(acc->fieldname, name))
				{
					const QCC_eval_t *old, *new;
					old = QCC_SRef_EvalConst(acc->staticval);
					new = QCC_SRef_EvalConst(sref);
					if (old && old == new && !typecmp(acc->staticval.cast, sref.cast))
						break;
					QCC_PR_ParseError(ERR_TOOMANYINITIALISERS, "%s::%s already declared", enumtype->name, name);
					break;
				}
			if (!acc)
			{
				acc = qccHunkAlloc(sizeof(*acc));
				acc->fieldname = (char*)name;
				acc->next = enumtype->accessors;
				acc->type = enumtype;//sref.cast;
				acc->indexertype = NULL;
				enumtype->accessors = acc;
				acc->staticval = sref;
				acc->staticval.cast = enumtype;
			}
		}
		QCC_FreeTemp(sref);

		if (flags)
		{
			next_f *= 2;
			next_i <<= 1;

			if (!next_i)
				next_f = next_i = 1;	//so you can start with an explicit =0 without needing an =1.
		}
		else
		{
			next_f++;
			next_i++;
		}

		if (QCC_PR_CheckToken("}"))
			break;
		QCC_PR_Expect(",");
		if (QCC_PR_CheckToken("}"))
			break; // accept trailing comma
	}
	return enumtype?enumtype:basetype;
}

QCC_sref_t QCC_PR_ParseDefArray(QCC_type_t **type, char *name, pbool istypedef, pbool fixedsize)
{
	QCC_sref_t exr;
	QCC_sref_t dynlength = nullsref;
	const QCC_eval_t *eval;
	size_t dim[16];
	int dims = 0;
	do
	{
		dim[dims] = 0;	//not known yet...

		if (dims== 0 && QCC_PR_CheckToken("]"))
		{
			//not specified, but that may be okay... perhaps.
		}
		else
		{
			exr = QCC_PR_Expression(TOP_PRIORITY, 0);
			eval = QCC_SRef_EvalConst(exr);
			if (pr_scope && dims==0 && QCC_OPCodeValid(&pr_opcodes[OP_PUSH]) && !flag_qcfuncs && !fixedsize)
				dynlength = exr;
			else if (eval)
			{
				dim[dims] = QCC_Eval_Int(eval, exr.cast);
				QCC_FreeTemp(exr);
			}
			else
			{
				QCC_PR_ParseWarning (ERR_BADARRAYSIZE, "Array length is not a constant");
				QCC_FreeTemp(exr);
			}
			QCC_PR_Expect("]");
		}
		dims++;
	} while(QCC_PR_CheckToken ("["));

	if (dynlength.cast && QCC_PR_PeekToken("="))
	{	//if its initialised then we need to statically init it. annoying.
		eval = QCC_SRef_EvalConst(dynlength);
		if (eval)
		{
			dim[0] = QCC_Eval_Int(eval, dynlength.cast);
			QCC_FreeTemp(dynlength);
		}
		else
			QCC_PR_ParseWarning (ERR_BADARRAYSIZE, "Array length is not a constant");
		dynlength = nullsref;
	}

#if 1
	if (dim[0] == 0 && !istypedef && !dynlength.cast)
	{
		char *oldprfile = pr_file_p;
		int oldline = pr_token_line_last;
		int oldsline = pr_source_line;
		int depth;

		//FIXME: preprocessor will hate this with a passion.
		if (QCC_PR_CheckToken("="))
		{
			if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
				dim[0] = pr_immediate_strlen+1;
			else
			{
				QCC_PR_Expect("{");
				dim[0]++;
				depth = 1;
				while(1)
				{
					if(pr_token_type == tt_eof)
					{
						QCC_PR_ParseError (ERR_EOF, "EOF inside definition of %s", name);
						break;
					}
					else if (depth == 1 && QCC_PR_CheckToken(","))
					{
						if (QCC_PR_CheckToken("}"))
							break;
						dim[0]++;
					}
					else if (QCC_PR_CheckToken("{") || QCC_PR_CheckToken("["))
						depth++;
					else if (QCC_PR_CheckToken("}") || QCC_PR_CheckToken("]"))
					{
						depth--;
						if (depth == 0)
							break;
					}
					else
						QCC_PR_Lex();
				}
			}

			pr_file_p = oldprfile;
			pr_token_line = oldline;
			pr_source_line = oldsline;

			pr_token_type = tt_punct;
			pr_immediate_type = type_void;
			strcpy(pr_token, "=");
		}
	}
#endif

	while (dims-- > 1)
	{	//go backwards. last parsed is the innermost array.
		*type = QCC_GenArrayType(*type, dim[dims]);
	}

	if (!dynlength.cast)
		dynlength.ofs = dim[0];
	return dynlength;
}

/*
================
PR_ParseDefs

Called at the outer layer and when a local statement is hit
================
*/
void QCC_PR_ParseDefs (const char *classname, pbool fatal_unused)
{
	char		*name;
	QCC_type_t		*basetype, *type, *defclass;
	QCC_def_t		*def, *d;
	QCC_sref_t		dynlength;
	QCC_function_t	*f;
	pbool shared=false;
	pbool isstatic=defaultstatic;
	pbool externfnc=false;
	pbool isauto=false;
	pbool istypedef=false;
	pbool isconstant = false;
	pbool isvar = false;
	pbool isinitialised = false;
	pbool noref = defaultnoref;
	pbool nosave = defaultnosave;
	pbool allocatenew = true;
	pbool inlinefunction = false;
	pbool allowinline = false;
	pbool dostrip = false;
	pbool dowrap = false;
	pbool doweak = false;
	pbool forceused = false;
	pbool accumulate = false;
	pbool hadmodifier = false; //if true then its a def. and we assume int when the tye was omitted.
	const char *deprecated = NULL;
	int arraysize;
	unsigned int gd_flags;
	const char *aliasof = NULL;

	pr_assumetermtype = NULL;
	pr_ignoredeprecation = false;

	while (QCC_PR_CheckToken(";"))
		;

	if (flag_acc)
	{
		char *oldp;
		if (QCC_PR_CheckKeyword (keyword_codesys, "CodeSys"))	//reacc support.
		{
			if (ForcedCRC)
				QCC_PR_ParseError(ERR_BADEXTENSION, "progs crc was already specified - only one is allowed");
			ForcedCRC = (int)pr_immediate._float;
			QCC_PR_Lex();
			QCC_PR_Expect(";");
			return;
		}

		oldp = pr_file_p;
		if (QCC_PR_CheckKeyword (keyword_var, "var"))	//reacc support.
		{
			if (accglobalsblock == 3)
			{
				if (!QCC_PR_GetDef(type_void, "end_sys_fields", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_fields", NULL, true, 0, false);
			}

			QCC_PR_ParseName();
			if (QCC_PR_CheckToken(":"))
				accglobalsblock = 1;
			pr_file_p = oldp;
			QCC_PR_Lex();
		}

		if (QCC_PR_CheckKeyword (keyword_function, "function"))	//reacc support.
		{
			accglobalsblock = 2;
		}
		if (QCC_PR_CheckKeyword (keyword_objdata, "objdata"))	//reacc support.
		{
			if (accglobalsblock == 3)
			{
				if (!QCC_PR_GetDef(type_void, "end_sys_fields", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_fields", NULL, true, 0, false);
			}
			else
				if (!QCC_PR_GetDef(type_void, "end_sys_globals", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_globals", NULL, true, 0, false);
			accglobalsblock = 3;
		}

		if (!pr_scope)
		switch(accglobalsblock)//reacc support.
		{
		case 1:
			{
			char *oldp = pr_file_p;
			name = QCC_PR_ParseName();
			if (!QCC_PR_CheckToken(":"))	//nope, it wasn't!
			{
				QCC_PR_IncludeChunk(name, true, NULL);
				QCC_PR_Lex();
				QCC_PR_UnInclude();
				pr_file_p = oldp;
				break;
			}
			if (QCC_PR_CheckKeyword(keyword_object, "object"))
				QCC_PR_GetDef(type_entity, name, NULL, true, 0, true);
			else if (QCC_PR_CheckKeyword(keyword_string, "string"))
				QCC_PR_GetDef(type_string, name, NULL, true, 0, true);
			else if (QCC_PR_CheckKeyword(keyword_real, "real"))
			{
				def = QCC_PR_GetDef(type_float, name, NULL, true, 0, true);
				if (QCC_PR_CheckToken("="))
				{
					def->symboldata[0]._float = pr_immediate._float;
					QCC_PR_Lex();
				}
			}
			else if (QCC_PR_CheckKeyword(keyword_vector, "vector"))
			{
				def = QCC_PR_GetDef(type_vector, name, NULL, true, 0, true);
				if (QCC_PR_CheckToken("="))
				{
					QCC_PR_Expect("[");
					def->symboldata[0].vector[0] = pr_immediate._float;
					QCC_PR_Lex();
					def->symboldata[0].vector[1] = pr_immediate._float;
					QCC_PR_Lex();
					def->symboldata[0].vector[2] = pr_immediate._float;
					QCC_PR_Lex();
					QCC_PR_Expect("]");
				}
			}
			else if (QCC_PR_CheckKeyword(keyword_pfunc, "pfunc"))
				QCC_PR_GetDef(type_function, name, NULL, true, 0, true);
			else
				QCC_PR_ParseError(ERR_BADNOTTYPE, "Bad type\n");
			QCC_PR_Expect (";");

			if (QCC_PR_CheckKeyword (keyword_system, "system"))
				QCC_PR_Expect (";");
			return;
			}
		case 2:
			name = QCC_PR_ParseName();
			QCC_PR_GetDef(type_function, name, NULL, true, 0, true);
			QCC_PR_CheckToken (";");
			return;
		case 3:
			{
				char *oldp = pr_file_p;
				name = QCC_PR_ParseName();
				if (!QCC_PR_CheckToken(":"))	//nope, it wasn't!
				{
					QCC_PR_IncludeChunk(name, true, NULL);
					QCC_PR_Lex();
					QCC_PR_UnInclude();
					pr_file_p = oldp;
					break;
				}
				if (QCC_PR_CheckKeyword(keyword_object, "object"))
					def = QCC_PR_GetDef(QCC_PR_FieldType(type_entity), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
				else if (QCC_PR_CheckKeyword(keyword_string, "string"))
					def = QCC_PR_GetDef(QCC_PR_FieldType(type_string), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
				else if (QCC_PR_CheckKeyword(keyword_real, "real"))
					def = QCC_PR_GetDef(QCC_PR_FieldType(type_float), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
				else if (QCC_PR_CheckKeyword(keyword_vector, "vector"))
					def = QCC_PR_GetDef(QCC_PR_FieldType(type_vector), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
				else if (QCC_PR_CheckKeyword(keyword_pfunc, "pfunc"))
					def = QCC_PR_GetDef(QCC_PR_FieldType(type_function), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
				else
				{
					QCC_PR_ParseError(ERR_BADNOTTYPE, "Bad type\n");
					QCC_PR_Expect (";");
					return;
				}

				if (!def->initialized)
				{
					unsigned int u;
					def->initialized = 1;
					for (u = 0; u < def->type->size*(def->arraysize?def->arraysize:1); u++)	//make arrays of fields work.
					{
						if (*(int *)&def->symboldata[u])
						{
							QCC_PR_ParseWarning(0, "Field def already has a value:");
							QCC_PR_ParsePrintDef(0, def);
						}
						*(int *)&def->symboldata[u] = pr.size_fields+u;
					}

					pr.size_fields += u;
				}

				QCC_PR_Expect (";");
				return;
			}
		}
	}

	while(1)
	{
		//storage classes...
		if (QCC_PR_CheckKeyword (keyword_typedef, "typedef"))		//not storage but defines a type instead.
			istypedef=true;
		else if (QCC_PR_CheckKeyword(keyword_extern, "extern"))		//came from some other unit (or at least outside of the current function)
		{
			if (!hadmodifier && pr_token_type == tt_immediate && pr_immediate_type==type_string)
			{	//this C++ism is for parsing types, we don't currently change operator precedence, keywords, etc
				pbool old_qcfuncs = flag_qcfuncs;
				pbool old_cpriority = flag_cpriority;
				pbool old_assumeint = flag_assume_integer;
				pbool old_assumef64 = flag_assume_double;
				pbool old_assumevar = flag_assumevar;
				pbool old_subscope = pr_subscopedlocals;
				if (!strcasecmp(pr_token, "C"))
				{
					flag_qcfuncs = false;	//ignore * on funcptrs, promote varg floats to doubles, some other quirks
					flag_cpriority = true;
					flag_assume_integer = true;
					//flag_assume_double = true;	//not forced, too annoying, but is popped so you can explicitly enable it within the block for some reason.
					flag_assumevar = true;
					pr_subscopedlocals = true;
				}
				else if (!strcasecmp(pr_token, "QC"))
				{
					flag_qcfuncs = true;	//no promotion, func references are simply references, etc
					//flag_cpriority = false; //refrain from changing it
					//flag_assume_integer = false;
					//flag_assume_double = false;
					//flag_assumevar = false;
					//pr_subscopedlocals = false;
				}
				QCC_PR_Lex();

				if (!QCC_PR_CheckToken ("{"))
					QCC_PR_ParseDefs(NULL, false);
				else for(;;)
				{
					if (QCC_PR_CheckToken("}"))
						break;
					if (pr_token_type == tt_eof)
					{
						QCC_PR_Expect ("}");
						break;
					}
					QCC_PR_ParseDefs(NULL, false);
				}
				//restore stuff to original mode.
				//FIXME: reset all flags+keywords?
				flag_qcfuncs = old_qcfuncs;
				flag_cpriority = old_cpriority;
				flag_assume_integer = old_assumeint;
				flag_assume_double = old_assumef64;
				flag_assumevar = old_assumevar;
				pr_subscopedlocals = old_subscope;
				return;
			}
			externfnc=true;
		}
		else if (QCC_PR_CheckKeyword(keyword_auto, "auto"))			//allocated on the stack like C's locals. may automatically/temporarily be promoted to register... default for C code.
			isauto=true;
		else if (QCC_PR_CheckKeyword(keyword_register, "register"))	//allow a leading register keyword. technically EVERYTHING is a register in qc, so we can just ignore this.
			isauto=false;
		else if (QCC_PR_CheckKeyword(keyword_static, "static"))		//comes from a global.
			isstatic = true;
		//else if (QCC_PR_CheckKeyword(keyword_static, "register"))	//bans address-of FIXME: this is checked as part of the type as it needs to be valid for function args too.
		//	isregister = true;
		//else if (QCC_PR_CheckKeyword(keyword_static, "local"))	//QC weirdness. like register, but you can still take an address of it etc...
		//	islocal = true;

		else if (QCC_PR_CheckKeyword(keyword_shared, "shared"))
		{
			shared=true;
			if (pr_scope)
				QCC_PR_ParseError (ERR_NOSHAREDLOCALS, "Cannot have shared locals");
		}
		else if (QCC_PR_CheckKeyword(keyword_const, "const"))
			isconstant = true;
		else if (QCC_PR_CheckKeyword(keyword_var, "var"))
			isvar = true;
		else if (!pr_scope && QCC_PR_CheckKeyword(keyword_nonstatic, "nonstatic"))
			isstatic = false;
		else if (QCC_PR_CheckKeyword(keyword_unused, "unused") || QCC_PR_CheckKeyword(keyword_noref, "noref"))
			noref=true;
		else if (QCC_PR_CheckKeyword(keyword_used, "used"))
			forceused=true;
		else if (QCC_PR_CheckKeyword(keyword_nosave, "nosave"))
			nosave = true;
		else if (QCC_PR_CheckKeyword(keyword_strip, "strip") || QCC_PR_CheckKeyword(keyword_ignore, "ignore"))
			dostrip = true;
		else if (QCC_PR_CheckKeyword(keyword_inline, "inline"))
			allowinline = true;
		else if (QCC_PR_CheckKeyword(keyword_wrap, "wrap"))
			dowrap = true;
		else if (QCC_PR_CheckKeyword(keyword_weak, "weak"))
			doweak = true;
		else if (QCC_PR_CheckKeyword(keyword_accumulate, "accumulate"))
			accumulate = true;
		else if (QCC_PR_CheckKeyword(false, "deprecated"))
		{
			if (!QCC_PR_CheckToken("("))
				deprecated = "";
			else
			{
				if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
				{
					if (deprecated)
					{
						char *n = qccHunkAlloc(strlen(deprecated)+2+strlen(pr_immediate_string)+1);
						sprintf(n, "%s; %s", deprecated, pr_immediate_string);
						deprecated = n;
					}
					else
						deprecated = strcpy(qccHunkAlloc(strlen(pr_immediate_string)+1), pr_immediate_string);
					QCC_PR_Lex();
				}
				else
					deprecated = "";
				QCC_PR_Expect(")");
			}
		}
		else if ((hadmodifier||(flag_attributes && !pr_scope)) && QCC_PR_CheckToken("["))
		{
			QCC_PR_Expect("[");
			while(pr_token_type != tt_eof)
			{
				if (QCC_PR_CheckToken(","))
					continue;
				else if (QCC_PR_CheckToken("]"))
					break;
				else if (QCC_PR_CheckName("alias"))
				{
					QCC_PR_Expect("(");
					if (pr_token_type == tt_name)
						aliasof = QCC_PR_ParseName();
					else if (pr_token_type == tt_immediate)
					{
						aliasof = strcpy(qccHunkAlloc(strlen(pr_immediate_string)+1), pr_immediate_string);
						QCC_PR_Lex();
					}
					QCC_PR_Expect(")");
				}
				else if (QCC_PR_CheckName("accumulate"))
				{
					if (accumulate)
						QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "accumulating an accumulating accumulation");
					accumulate = true;
				}
				else if (QCC_PR_CheckName("eraseable"))
					noref = true;
				else if (QCC_PR_CheckKeyword(false, "deprecated"))
				{
					if (!QCC_PR_CheckToken("("))
						deprecated = "";
					else
					{
						if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
						{
							deprecated = strcpy(qccHunkAlloc(strlen(pr_immediate_string)+1), pr_immediate_string);
							QCC_PR_Lex();
						}
						else
							QCC_PR_ParseError (ERR_EXPECTED, "expected string");
						QCC_PR_Expect(")");
					}
				}
				else
				{
					QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "Unknown attribute \"%s\"", pr_token);
					while(pr_token_type != tt_eof)
					{
						if (QCC_PR_PeekToken("]"))
							break;
						if (QCC_PR_PeekToken(","))
							break;
						QCC_PR_Lex();
					}
				}
			}
			QCC_PR_Expect("]");
		}
		else
			break;
		hadmodifier = true;
	}

	basetype = QCC_PR_ParseType (false, hadmodifier, !flag_qcfuncs);
	if (basetype == NULL)	//ignore
	{
		if (hadmodifier)
		{	//banned since c99...
			//if (c23 && isauto) basetype=guess; else
			basetype = type_integer;
		}
		else
			return;
	}

	inlinefunction = type_inlinefunction;

	if (externfnc && !pr_scope && basetype->type != ev_function)
	{
		if (flag_qcfuncs)
			QCC_PR_ParseWarning(WARN_IGNOREDKEYWORD, "extern keyword may only apply to qc functions.");
		externfnc=false;
	}

	if (!pr_scope && QCC_PR_CheckKeyword(keyword_function, "function"))	//reacc support.
	{
		name = QCC_PR_ParseName ();
		QCC_PR_Expect("(");
		type = QCC_PR_ParseFunctionTypeReacc(false, basetype);
		QCC_PR_Expect(";");

		if (istypedef)
			return;
		else
			def = QCC_PR_GetDef (basetype, name, NULL, true, 0, false);

		if (autoprototype || dostrip)
		{	//ignore the code and stuff

			if (QCC_PR_CheckKeyword(keyword_external, "external"))
			{	//builtin
				QCC_PR_Lex();
				QCC_PR_Expect(";");
			}
			else
			{
				int blev = 1;

				while (!QCC_PR_CheckToken("{"))	//skip over the locals.
				{
					if (pr_token_type == tt_eof)
					{
						QCC_PR_ParseError(0, "Unexpected EOF");
						break;
					}
					QCC_PR_Lex();
				}

				//balance out the { and }
				while(blev)
				{
					if (pr_token_type == tt_eof)
						break;
					if (QCC_PR_CheckToken("{"))
						blev++;
					else if (QCC_PR_CheckToken("}"))
						blev--;
					else
						QCC_PR_Lex();	//ignore it.
				}
			}
			return;
		}
		else
		{
			def->referenced = true;

			f = QCC_PR_ParseImmediateStatements (def, basetype, false);

			def->initialized = 1;
			def->isstatic = isstatic;
			def->symboldata[0].function = numfunctions;
			f->def = def;
	//				if (pr_dumpasm)
	//					PR_PrintFunction (def);

			if (numfunctions >= MAX_FUNCTIONS)
				QCC_Error(ERR_INTERNAL, "Too many function defs");
		}
		return;
	}

//	if (pr_scope && (type->type == ev_field) )
//		QCC_PR_ParseError ("Fields must be global");

	do
	{
		isinitialised = false;
		type = basetype;

		if (QCC_PR_CheckToken (";"))
		{
			if (istypedef)
			{
				QCC_PR_ParseWarning(WARN_UNEXPECTEDPUNCT, "typedef defines no types");
				return;
			}
			if (type->type == ev_field && (type->aux_type->type == ev_union || type->aux_type->type == ev_struct))
			{
				QCC_PR_ExpandUnionToFields(type, &pr.size_fields);
				return;
			}
			if (type->type == ev_struct && strcmp(type->name, "struct"))
				return;	//allow named structs
			if (type->type == ev_entity && type != type_entity)
				return;	//allow forward class definititions with or without a variable. 
			if (type->type == ev_accessor)	//accessors shouldn't trigger problems if they're just a type.
				return;
//			if (type->type == ev_union)
//			{
//				return;
//			}
//			QCC_PR_ParseError (WARN_TYPEWITHNONAME, "type (%s) with no name", type->name);
			return;
		}

		if (!istypedef && !classname && type->typedefed && QCC_PR_CheckToken("::"))
		{
			classname = type->name;	//FIXME: doesn't work with commas...
			type = type_invalid;
		}
		else while (QCC_PR_CheckToken ("*"))
			type = QCC_PointerTypeTo(type);

		arraysize = 0;
		name = NULL;
		while(QCC_PR_CheckToken ("("))
		{
			QCC_type_t *ftype;
			int isptr = 0;
			ftype = QCC_PR_ParseFunctionType(false, type);
			if (ftype)
			{	//qc-style function... possibly returning a function.
				type = ftype;
			}
			else
			{	//c-style function pointer
				while (QCC_PR_CheckToken("*"))
					isptr++;
				name = QCC_PR_ParseName ();
				if (QCC_PR_CheckToken ("["))
				{
					if (QCC_PR_CheckToken ("]"))
						arraysize = -1;
					else
					{
						arraysize = QCC_PR_IntConstExpr();
						QCC_PR_Expect ("]");
					}
				}
				QCC_PR_Expect (")");
				if (!istypedef)
					isvar |= isptr>0;
				if (QCC_PR_CheckToken ("("))
				{
					type = QCC_PR_ParseFunctionType(false, type);
					if (!type)
						QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
				}
				while(isptr-- > 1)	//C's function pointers are basically the same as a qc function reference, though the definition is a bit different.
					type = QCC_PointerTypeTo(type);
				break;
			}
		}
		if (!name)
			name = QCC_PR_ParseName ();

		if (!istypedef && !classname && QCC_PR_CheckToken("::"))
		{
			classname = name;	//FIXME: doesn't work with commas...
			name = QCC_PR_ParseName();
		}

		if (type == type_invalid)
		{
			type = type_void;
			if (!strcmp(classname, name))
				;	//constructor. allowed.
			else
				QCC_PR_ParseWarning(ERR_NOTATYPE, "no type specified for %s::%s. this is only allowed for constructors", classname, name);
		}

//check for an array

		dynlength = nullsref;
		if (!arraysize && QCC_PR_CheckToken ("["))
		{
			dynlength = QCC_PR_ParseDefArray(&type, name, istypedef, istypedef||isstatic);
			if (dynlength.cast)
				arraysize = 0;
			else
			{
				arraysize = dynlength.ofs;
				if (!arraysize)
					arraysize = -1;
			}
		}

		if (QCC_PR_CheckToken("("))
		{
			if (inlinefunction)
				QCC_PR_ParseWarning(WARN_UNSAFEFUNCTIONRETURNTYPE, "Function returning function. Is this what you meant? (suggestion: use typedefs)");
			inlinefunction = false;
			if (!flag_qcfuncs && pr_scope)
				isconstant = externfnc = true;	//in C, a locally defined function refers to an external one.
			type = QCC_PR_ParseFunctionType(false, type);
			if (!type)
				QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
		}

		allocatenew = true;
		if (classname)
		{
			unsigned int ofs, bitofs;
			struct QCC_typeparam_s *p;
			char *membername = name;
			name = qccHunkAlloc(strlen(classname) + strlen(name) + 3);
			sprintf(name, "%s::%s", classname, membername);
			defclass = QCC_TypeForName(classname);
			allocatenew = (dynlength.cast || aliasof)?false:2;
			if (defclass && defclass->type == ev_struct)
				allocatenew = false;
			else if (!defclass || !defclass->parentclass)
				QCC_PR_ParseError(ERR_NOTANAME, "%s is not a class\n", classname);

			if (defclass->type == ev_struct)
			{
				p = QCC_PR_FindStructMember(defclass, membername, &ofs, &bitofs);
				if (p && p->isvirtual)
					type = QCC_PR_MakeThiscall(type, defclass);
			}
		}
		else
			defclass = NULL;

		if (istypedef+externfnc+isauto+isstatic+shared+(aliasof!=NULL)>1)
			QCC_PR_ParseWarning(WARN_IGNOREDKEYWORD, "Conflicting storage classes.");
		if (isconstant+isvar>1)
			QCC_PR_ParseWarning(WARN_IGNOREDKEYWORD, "Conflicting constness.");
		if ((allowinline || dowrap || doweak || accumulate) && type->type != ev_function)
			QCC_PR_ParseWarning(WARN_IGNOREDKEYWORD, "function modifier on non-function.");

		if (istypedef)
		{
			QCC_type_t *old;
			if (externfnc||shared||isconstant||isvar||forceused||dostrip||allowinline||dowrap||doweak||accumulate||aliasof||deprecated
					||(isstatic && !defaultstatic)
					||(noref && !defaultnoref)
					||(nosave && !defaultnosave) )
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "bad combination of modifiers with typedef (defining %s%s%s)", col_type,name,col_none);
			if (arraysize)
			{
				struct QCC_typeparam_s *param = qccHunkAlloc(sizeof(*param));
//				QCC_PR_ParseWarning(ERR_BADEXTENSION, "unsupported typedefed array (defining %s%s%s[%i])", col_type,name,col_none, arraysize);
				param->type = type;
				param->arraysize = arraysize;
				param->paramname = NULL;
				type = QCC_PR_NewType(name, ev_union, true);
				type->params = param;
				type->num_parms = 1;
				type->size = param->type->size * param->arraysize;
			}
			else if (dynlength.cast)
			{
				QCC_PR_ParseWarning(ERR_BADEXTENSION, "unsupported typedefed array (defining %s%s%s[])", col_type,name,col_none);
				type = QCC_PointerTypeTo(type);
			}

			old = QCC_TypeForName(name);
			if (old && old->scope == pr_scope)
			{
				if (typecmp(old, type))
				{
					char obuf[1024];
					char nbuf[1024];
					QCC_PR_ParseWarning(ERR_NOTATYPE, "Cannot redeclare typedef %s%s%s from %s%s%s to %s%s%s", col_type,name,col_none, col_type,TypeName(old, obuf, sizeof(obuf)),col_none, col_type,TypeName(type, nbuf, sizeof(nbuf)),col_none);
				}
			}
			else
			{
				old = type;
				type = QCC_PR_NewType(name, ev_typedef, true);
				type->aux_type = old;
				type->scope = pr_scope;
			}

			def = NULL;
			continue;
		}

		isinitialised = QCC_PR_CheckToken ("=") || ((type->type == ev_function) && (pr_token[0] == '{' || pr_token[0] == '[' || pr_token[0] == ':'));

		gd_flags = 0;
		if (isstatic)
			gd_flags |= GDF_STATIC;
		if (isconstant || (!isvar && !pr_scope && ((isinitialised && !flag_assumevar) || type->type == ev_function || type->type == ev_field)))
			gd_flags |= GDF_CONST;	//initialised things are assumed to be consts, unless assumevar is specified. functions and fields are always assumed to be consts even when not initialised. nothing is assumed const at local scope.
		if (!nosave)
			gd_flags |= GDF_SAVED;
		if (allowinline)
			gd_flags |= GDF_INLINE;
		if (dostrip)
			gd_flags |= GDF_STRIP;
		else if (forceused)	//FIXME: make proper pragma(used) thingie
			gd_flags |= GDF_USED;

		if (!type->size)
		{
//			char buf[1024];
//			QCC_PR_ParseError(ERR_BADEXTENSION, "type %s%s%s not yet defined, cannot create %s%s%s", col_type,TypeName(type,buf,sizeof(buf)),col_none, col_name,name,col_none);
		}
		if (dynlength.cast && !aliasof)
		{
			const QCC_eval_t *eval;
			size_t fixedlen;
			dynlength = QCC_SupplyConversion(dynlength, ev_uint, true);
			eval = QCC_SRef_EvalConst(dynlength);
			if (eval)
			{
				gd_flags |= GDF_AUTODEREF;
				fixedlen = QCC_Eval_Int(eval, dynlength.cast);
				def = QCC_PR_GetDef (QCC_PR_PointerType(QCC_GenArrayType(type, fixedlen)), name, pr_scope, allocatenew, 0, gd_flags);
			}
			else
				def = QCC_PR_GetDef (QCC_PR_PointerType(type), name, pr_scope, allocatenew, 0, gd_flags);

			if (type->bits)	//OP_PUSH takes words. we need to round up.
			{
				dynlength = QCC_PR_Statement(pr_opcodes+OP_MUL_I, dynlength, QCC_MakeUIntConst(type->bits/8), NULL);	//convert to bytes
				dynlength = QCC_PR_Statement(pr_opcodes+OP_ADD_I, dynlength, QCC_MakeUIntConst(3), NULL);	//extend by the worst case
				dynlength = QCC_PR_Statement(pr_opcodes+OP_DIV_I, dynlength, QCC_MakeUIntConst(4), NULL);	//convert to words
			}
			else if (type->size != 1)
				dynlength = QCC_PR_Statement(pr_opcodes+OP_MUL_I, dynlength, QCC_MakeIntConst(type->size), NULL);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_PUSH], dynlength, nullsref, QCC_MakeSRef(def, 0, def->type), false);	//push *(int*)&a elements
			QCC_FreeTemp(dynlength);
			QCC_FreeDef(def);
		}
		else if (isauto)
		{
			gd_flags |= GDF_AUTODEREF;
			if (arraysize)
				type = QCC_GenArrayType(type, arraysize);
			def = QCC_PR_GetDef (QCC_PR_PointerType(type), name, pr_scope, allocatenew, 0, gd_flags);
			if (pr_scope && !isstatic)
				QCC_PR_SimpleStatement(&pr_opcodes[OP_PUSH], QCC_MakeIntConst(type->size), nullsref, QCC_MakeSRef(def, 0, def->type), false);	//push *(int*)&a elements
			else
			{
				QCC_sref_t f = QCC_PR_EmulationFunc(memalloc);
				if (!f.cast) f = QCC_PR_EmulationFunc(malloc);
				if (!f.cast) QCC_PR_ParseError(ERR_BADEXTENSION, "memalloc not yet defined, cannot define __auto %s at global scope", name);
				if (QCC_OPCodeValid(&pr_opcodes[OP_CALL1H]))
					QCC_PR_SimpleInitStatement(&pr_opcodes[OP_CALL1H], f, QCC_MakeIntConst(type->size*4), nullsref);
				else
				{
					QCC_PR_SimpleInitStatement(&pr_opcodes[OP_STORE_F], QCC_MakeIntConst(type->size*4), QCC_MakeSRef(&def_parms[0], 0, type_integer), nullsref);
					QCC_PR_SimpleInitStatement(&pr_opcodes[OP_CALL1], f, nullsref, nullsref);
				}
				QCC_PR_SimpleInitStatement(&pr_opcodes[QCC_OPCodeValid(&pr_opcodes[OP_STORE_P])?OP_STORE_P:OP_STORE_I], QCC_MakeSRef(&def_ret, 0, def->type), QCC_MakeSRef(def, 0, def->type), nullsref);
			}
		}
		else
		{
			if (aliasof)
			{
				if (dynlength.cast)
					QCC_PR_ParseWarning(ERR_BADEXTENSION, "array aliases are not supported");
				def = QCC_PR_GetDef (NULL, aliasof, externfnc?NULL:pr_scope, false, arraysize, gd_flags);
				if (!def)
					QCC_PR_ParseError(ERR_BADEXTENSION, "%s not yet defined, cannot create %s as an alias", aliasof, name);
				def->referenced = true;
				def = QCC_PR_DummyDef(type, name, externfnc?NULL:pr_scope, arraysize, def, 0, true, gd_flags);
			}
			else if (allocatenew != 1)
			{	//we always allocate here, because it lets us handle syntax errors a little more gracefully, but an error is still an error and will be fatal later.
				def = QCC_PR_GetDef (type, name, externfnc?NULL:pr_scope, false, arraysize, gd_flags);
				if (!def)
				{
					QCC_PR_ParseWarning(allocatenew?WARN_MEMBERNOTDEFINED:ERR_NOTDEFINED, "%s is not part of class %s", name, classname);
					def = QCC_PR_GetDef (type, name, externfnc?NULL:pr_scope, true, arraysize, gd_flags);
				}
			}
			else
				def = QCC_PR_GetDef (type, name, externfnc?NULL:pr_scope, allocatenew, arraysize, gd_flags);
		}

		if (!def)
			QCC_PR_ParseError(ERR_NOTANAME, "%s is not part of class %s", name, classname);

		if (accumulate && (def->type->type != ev_function || def->arraysize != 0))
			QCC_PR_ParseError(ERR_NOTAFUNCTION, "accumulate applies only to functions, not %s", def->name);

		if (noref)
		{
			def->unused = true;
			def->referenced = true;
		}

		if (!def->initialized && shared)	//shared count as initiialised
		{
			def->shared = shared;
			def->initialized = true;
			def->nofold = true;
		}
		if (externfnc && !pr_scope)
		{
			def->initialized = true;
			def->isextern = true;
		}

		if (deprecated)
			def->deprecated = deprecated;

		if (isstatic)
		{
			if (!strcmp(def->filen, s_filen))
				def->isstatic = isstatic;
			else //if (type->type != ev_function && defaultstatic)	//functions don't quite consitiute a definition
				QCC_PR_ParseErrorPrintDef (ERR_REDECLARATION, def, "can't redefine non-static as static");
		}

// check for an initialization
		/*if (type->type == ev_function && (pr_scope))
		{
			if ( QCC_PR_CheckToken ("=") )
			{
				QCC_PR_ParseError (ERR_INITIALISEDLOCALFUNCTION, "local functions may not be initialised");
			}

			d = def;
			while (d != def->deftail)
			{
				d = d->next;
				d->initialized = 1;	//fake function
				d->symboldata[0].function = 0;
			}

			continue;
		}*/

		if (type->type == ev_field && QCC_PR_CheckName ("alias"))
		{
			QCC_PR_ParseError(ERR_INTERNAL, "FTEQCC does not support this variant of decompiled hexenc\nPlease obtain the original version released by Raven Software instead.");
			name = QCC_PR_ParseName();
		}
		else if (isinitialised)	//this is an initialisation (or a function)
		{
			pbool isconst;
			QCC_type_t *parentclass;
			if (aliasof)
				QCC_PR_ParseError (ERR_SHAREDINITIALISED, "alias %s may not be initialised", name);
			if (def->shared)
				QCC_PR_ParseError (ERR_SHAREDINITIALISED, "shared values may not be assigned an initial value");

			//if weak, only use the first non-weak version of the function
			if (autoprototype || dostrip || (def->initialized && doweak) || (!def->initialized && doweak && dowrap))
			{	//ignore the code and stuff
				if ((dostrip || (doweak && dowrap)))
					def->unused = true;
				if (dostrip)
					def->referenced = true;
				if (QCC_PR_CheckToken("["))
				{
					while (!QCC_PR_CheckToken("]"))
					{
						if (pr_token_type == tt_eof)
							break;
						QCC_PR_Lex();
					}
				}
				if (QCC_PR_CheckToken("{"))
				{
					int blev = 1;
					//balance out the { and }
					while(blev)
					{
						if (pr_token_type == tt_eof)
							break;
						if (QCC_PR_CheckToken("{"))
							blev++;
						else if (QCC_PR_CheckToken("}"))
							blev--;
						else
							QCC_PR_Lex();	//ignore it.
					}
				}
				else
				{
					if (type->type == ev_string && QCC_PR_CheckName("_"))
					{
						QCC_PR_Expect("(");
						QCC_PR_Lex();
						QCC_PR_Expect(")");
					}
					else
					{
						QCC_PR_CheckToken("#");
						do
						{
							QCC_PR_Lex();
						} while (*pr_token && strcmp(pr_token, ",") && strcmp(pr_token, ";"));
					}
				}
				QCC_FreeDef(def);
				continue;
			}

			parentclass = pr_classtype;
			pr_classtype = defclass?defclass:pr_classtype;
			if (flag_assumevar)
				isconst = isconstant || (!isvar && !pr_scope && (type->type == ev_function || type->type == ev_field));
			else
				isconst = (isconstant || (!isvar && !pr_scope));
			if (isconst != def->constant)
			{	//we should only be optimising consts if its initialised, so it shouldn't have been read as 0 at any point so far.
				QCC_PR_ParseWarning(WARN_REDECLARATIONMISMATCH, "Redeclaration of %s would change to %sconst.", def->name, isconst?"":"non-");
				QCC_PR_ParsePrintDef(WARN_REDECLARATIONMISMATCH, def);
//				def->constant = isconst;
			}
			if (accumulate || def->accumulate)
			{
				unsigned int pif_flags = PIF_ACCUMULATE;
				if (!def->initialized)
					def->accumulate |= true;	//first time
				else if (dowrap)
					pif_flags |= PIF_WRAP;		//explicitly wrapping a prior accumulation...
				else if (!def->accumulate)
				{
					QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "%s redeclared to accumulate after initial declaration", def->name);
					pif_flags |= PIF_WRAP|PIF_AUTOWRAP;	//wrap it automatically so its not so obvious
				}
				def->accumulate |= true;
				def->initialized = true;
				def->symboldata[0].function = QCC_PR_ParseImmediateStatements (def, def->type, pif_flags) - functions;
			}
			else
				QCC_PR_ParseInitializerDef(def, (dowrap?PIF_WRAP:0)|(def->weak?PIF_STRONGER:0));
			if (doweak)
				def->weak = true;
			else
				def->weak = false;
			pr_classtype = parentclass;

			if (!def->nofold && def->constant && def->initialized && def->symbolheader == def && def->ofs == 0 && !def->arraysize)
			{
				QCC_def_t *base = NULL;
				const QCC_eval_t *val = (const QCC_eval_t *)&def->symboldata[0];
				if (type->type == ev_float)
					base = QCC_MakeFloatConst(val->_float).sym;
				else if (type->type == ev_integer)
					base = QCC_MakeIntConst(val->_int).sym;
				else if (type->type == ev_vector)
					base = QCC_MakeVectorConst(val->vector[0], val->vector[1], val->vector[2]).sym;
				if (base && !base->symbolheader->nofold)
				{
					def->ofs = base->ofs;
					def->symbolheader = base->symbolheader;
				}
			}
		}
		else
		{
			if (1)//isconstant || isvar)
			{
				int c = isconstant;
				if (type->type == ev_function)
					c |= !isvar && !pr_scope;
				if (type->type == ev_field)
				{
					if (c)
						c = 2;
					else if (isvar || (pr_scope && !isstatic))
						c = 0;
					else
						c = 1;
				}
				if (def->constant != c)
				{
					QCC_PR_ParseWarning(WARN_REDECLARATIONMISMATCH, "Redeclaration of %s changes const.", def->name);
					QCC_PR_ParsePrintDef(WARN_REDECLARATIONMISMATCH, def);
				}
			}
			if (accumulate)
			{
				if (def->initialized)
					QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "%s redeclared to accumulate after initial declaration", def->name);
				def->accumulate |= true;
			}

			if (dostrip)
				def->referenced = true;
			else if (type->type == ev_field)
			{
				//fields are const by default, even when not initialised (as they are initialised behind the scenes)
				if (!def->initialized && def->constant)
				{
					unsigned int i;
					def->initialized = true;
					//if the field already has a value, don't allocate new field space for it as that would confuse things.
					//otherwise allocate new space.
					if (def->symboldata[0]._int)
					{
						for (i = 0; i < type->size*(arraysize?arraysize:1); i++)	//make arrays of fields work.
						{
							if (def->symboldata[i]._int != i + def->symboldata[0]._int)
							{
								QCC_PR_ParseWarning(0, "Inconsistant field def:");
								QCC_PR_ParsePrintDef(0, def);
								break;
							}
						}
					}
					else
					{
						for (i = 0; i < type->size*(arraysize?arraysize:1); i++)	//make arrays of fields work.
						{
							if (def->symboldata[i]._int)
							{
								QCC_PR_ParseWarning(0, "Field def already has a value:");
								QCC_PR_ParsePrintDef(0, def);
							}
							def->symboldata[i]._int = pr.size_fields+i;
						}

						pr.size_fields += i;
					}
				}
			}
		}

		d = def;
		QCC_FreeDef(d);
		while (d != def->deftail)
		{
			d = d->next;
			d->constant = def->constant;
			d->initialized = def->initialized;
		}
	} while (QCC_PR_CheckToken (","));

	if (type->type == ev_function)
		QCC_PR_CheckTokenComment (";", def?&def->comment:NULL);
	else
	{
		if (!QCC_PR_CheckTokenComment (";", def?&def->comment:NULL))
			QCC_PR_ParseWarning(WARN_UNDESIRABLECONVENTION, "Missing semicolon at end of definition");
	}
}

/*
============
PR_CompileFile

compiles the 0 terminated text, adding defintions to the pr structure
============
*/
void QCC_PR_LexWhitespace (pbool inhibitpreprocessor);
pbool	QCC_PR_CompileFile (char *string, char *filename)
{
	char *tmp;
	jmp_buf oldjb;
	if (!pr.memory)
		QCC_Error (ERR_INTERNAL, "PR_CompileFile: Didn't clear");

	QCC_PR_ClearGrabMacros (true);	// clear the frame macros

	compilingfile = filename;

	s_unitn = s_filen = tmp = qccHunkAlloc(strlen(filename)+1);
	strcpy(tmp, filename);
	if (opt_filenames)
	{
		optres_filenames += strlen(filename);
		s_filed = 0;
	}
	else
		s_filed = QCC_CopyString (filename);

	pr_file_p = string;
	pr_assumetermtype = NULL;
	pr_ignoredeprecation = false;

	pr_source_line = 0;

	memcpy(&oldjb, &pr_parse_abort, sizeof(oldjb));

	if( setjmp( pr_parse_abort ) ) {
		pr_error_count++;
		// dont count it as error
	} else {
		//clock up the first line
		QCC_PR_NewLine (false);

		QCC_PR_Lex ();	// read first token
	}

	if (preprocessonly)
	{
		pbool white = false;
		static int line = 1;//pr_source_line;
		static const char *fname = NULL;
		static QCC_string_t fnamed = 0;
		while(pr_token_type != tt_eof)
		{
//			white = (qcc_iswhite(*pr_file_p) || (*pr_file_p == '/' && (pr_file_p[1] == '/' || pr_file_p[1] == '*')));
//			QCC_PR_LexWhitespace (false);

			if (fnamed != s_filed)
			{
				line = pr_token_line;
				fname = s_filen;
				fnamed = s_filed;
				externs->Printf("\n#pragma file(%s)\n",fname);
				externs->Printf("#pragma line(%i)\n",line);
			}
			else
			{
				//if there's whitespace next, make sure we represent that
				while(line < pr_token_line)
				{	//keep line numbers correct by splurging multiple newlines.
					externs->Printf("\n");
					white = false;
					line++;
				}
				if (white)
					externs->Printf(" ");
			}

			if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
			{
				const char *s = pr_token;
				externs->Printf("\"");
				while (*s)
				{
					switch(*s)
					{
					case 0:
						externs->Printf("%c", 0);
						break;
					case '\\':
						externs->Printf("\\\\");
						break;
					case '\"':
						externs->Printf("\\\"");
						break;
					case '\r':
						externs->Printf("\\r");
						break;
					case '\n':
						externs->Printf("\\n");
						break;
					case '\t':
						externs->Printf("\\t");
						break;
					default:
						externs->Printf("%c", *s);
						break;
					}
					s++;
				}
				externs->Printf("\"");
			}
			else
				externs->Printf("%s", pr_token);
			white = (qcc_iswhite(*pr_file_p) || (*pr_file_p == '/' && (pr_file_p[1] == '/' || pr_file_p[1] == '*')));
			QCC_PR_Lex();
		}
	}
	else while (pr_token_type != tt_eof)
	{
		if (setjmp(pr_parse_abort))
		{
			num_continues = 0;
			num_breaks = 0;
			num_cases = 0;
			if (++pr_error_count > MAX_ERRORS)
			{
				memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));
				return false;
			}
			QCC_PR_SkipToSemicolon ();
			if (pr_token_type == tt_eof)
			{
				memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));
				return false;
			}
		}

		pr_scope = NULL;	// outside all functions

		QCC_PR_ParseDefs (NULL, true);

#if 0//def _DEBUG
		if (!pr_error_count)
		{
			QCC_def_t *d;
			unsigned int i;
			for (i = 0; i < MAX_PARMS; i++)
			{
				d = &def_parms[i];
				if (d->refcount)
				{
					QCC_sref_t sr;
					sr.sym = d;
					sr.cast = d->type;
					sr.ofs = 0;
					QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %i references still held on %s (%s)", d->refcount, d->name, QCC_VarAtOffset(sr));
					d->refcount = 0;
				}
			}
			for (d = pr.def_head.next; d; d = d->next)
			{
				if (d->refcount)
				{
					QCC_sref_t sr;
					sr.sym = d;
					sr.cast = d->type;
					sr.ofs = 0;
					QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %i references still held on %s (%s)", d->refcount, d->name, QCC_VarAtOffset(sr));
					d->refcount = 0;
				}
			}
			for (i = 0; i < tempsused; i++)
			{
				d = tempsinfo[i].def;
				if (d->refcount)
				{
					QCC_sref_t sr;
					sr.sym = d;
					sr.cast = d->type;
					sr.ofs = 0;
					QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %i references still held on %s (%s)", d->refcount, d->name, QCC_VarAtOffset(sr));
					d->refcount = 0;
				}
			}
		}
#endif
	}
	memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));

	return (pr_error_count == 0);
}

pbool QCC_Include(const char *filename, pbool newunit)
{
	char *newfile;
	char fname[512];
	char *opr_file_p;
	const char *os_unitn;
	const char *os_filen;
	QCC_string_t os_filed;
	int opr_source_line;
	char *ocompilingfile;
	struct qcc_includechunk_s *oldcurrentchunk;

	ocompilingfile = compilingfile;
	os_unitn = s_unitn;
	os_filen = s_filen;
	os_filed = s_filed;
	opr_source_line = pr_source_line;
	opr_file_p = pr_file_p;
	oldcurrentchunk = currentchunk;

	strcpy(fname, filename);
	QCC_LoadFile(fname, (void*)&newfile);

	if (!newunit)
	{
		QCC_PR_IncludeChunkEx(newfile, false, NULL, NULL);
		s_filen = strcpy(qccHunkAlloc(strlen(filename)+1), filename);
		return true;
	}

	currentchunk = NULL;
	pr_file_p = newfile;
	QCC_PR_CompileFile(newfile, fname);
	currentchunk = oldcurrentchunk;

	compilingfile = ocompilingfile;
	s_unitn = os_unitn;
	s_filen = os_filen;
	s_filed = os_filed;
	pr_source_line = opr_source_line;
	pr_file_p = opr_file_p;

	if (pr_error_count > MAX_ERRORS)
		longjmp (pr_parse_abort, 1);

//	QCC_PR_IncludeChunk(newfile, false, fname);

	return true;
}
void QCC_Cleanup(void)
{
	free(pr_breaks);
	free(pr_continues);
	free(pr_cases);
	free(pr_casesref);
	free(pr_casesref2);
	max_breaks = max_continues = max_cases = num_continues = num_breaks = num_cases = 0;
	pr_breaks = NULL;
	pr_continues = NULL;
	pr_cases = NULL;
	pr_casesref = NULL;
	pr_casesref2 = NULL;
	*compilingrootfile = 0;

#ifdef _DEBUG
	OpAssignsTo_Debug();
#endif
}
#endif
