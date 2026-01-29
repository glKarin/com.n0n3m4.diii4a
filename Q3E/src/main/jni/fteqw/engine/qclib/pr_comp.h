// this file is shared by the execution and compiler

/*i'm part way through making this work
I've given up now that I can't work out a way to load pointers.
Setting them should be fine.
*/
#ifndef __PR_COMP_H__
#define __PR_COMP_H__
#include "progtype.h"

/*
#ifdef USE_MSVCRT_DEBUG
void *BZ_MallocNamed(int size, char *file, int line);
void *BZ_ReallocNamed(void *data, int newsize, char *file, int line);
void BZ_Free(void *data);
#define BZ_Malloc(size) BZ_MallocNamed(size, __FILE__, __LINE__)
#define BZ_Realloc(ptr, size) BZ_ReallocNamed(ptr, size, __FILE__, __LINE__)
#define malloc BZ_Malloc
#define realloc BZ_Realloc
#define free BZ_Free
#endif
*/

typedef int dstring_t;
#define QCC_string_t dstring_t

#if defined(_MSC_VER) && _MSC_VER < 1300
#define prclocks_t unsigned __int64
#define ull2dbl(x)	((double)(__int64)x)
#else
#define prclocks_t unsigned long long
#define ull2dbl(x) ((double)x)
#endif

//typedef enum {ev_void, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_pointer, ev_integer, ev_struct, ev_union} etype_t;
//				0			1		2			3			4		5			6				7			8		9			10

#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28


enum qcop_e {
	OP_DONE,	//0
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,
	
	OP_EQ_F,	//10
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,
	
	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,
	
	OP_LE_F,	//20
	OP_GE_F,
	OP_LT_F,
	OP_GT_F,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,	//30

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,	//40
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF_I,
	OP_IFNOT_I,		//50
	OP_CALL0,		//careful... hexen2 and q1 have different calling conventions
	OP_CALL1,		//remap hexen2 calls to OP_CALL2H
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,		//60
	OP_GOTO,
	OP_AND_F,
	OP_OR_F,
	
	OP_BITAND_F,
	OP_BITOR_F,

	
	//these following ones are Hexen 2 constants.
	
	OP_MULSTORE_F,	//66 redundant, for h2 compat
	OP_MULSTORE_VF,	//67 redundant, for h2 compat
	OP_MULSTOREP_F,	//68
	OP_MULSTOREP_VF,//69

	OP_DIVSTORE_F,	//70 redundant, for h2 compat
	OP_DIVSTOREP_F,	//71

	OP_ADDSTORE_F,	//72 redundant, for h2 compat
	OP_ADDSTORE_V,	//73 redundant, for h2 compat
	OP_ADDSTOREP_F,	//74
	OP_ADDSTOREP_V,	//75

	OP_SUBSTORE_F,	//76 redundant, for h2 compat
	OP_SUBSTORE_V,	//77 redundant, for h2 compat
	OP_SUBSTOREP_F,	//78
	OP_SUBSTOREP_V,	//79

	OP_FETCH_GBL_F,	//80 has built-in bounds check
	OP_FETCH_GBL_V,	//81 has built-in bounds check
	OP_FETCH_GBL_S,	//82 has built-in bounds check
	OP_FETCH_GBL_E,	//83 has built-in bounds check
	OP_FETCH_GBL_FNC,//84 has built-in bounds check

	OP_CSTATE,		//85
	OP_CWSTATE,		//86

	OP_THINKTIME,	//87 shortcut for OPA.nextthink=time+OPB

	OP_BITSETSTORE_F,	//88 redundant, for h2 compat
	OP_BITSETSTOREP_F,	//89
	OP_BITCLRSTORE_F,	//90
	OP_BITCLRSTOREP_F,	//91

	OP_RAND0,		//92	OPC = random()
	OP_RAND1,		//93	OPC = random()*OPA
	OP_RAND2,		//94	OPC = random()*(OPB-OPA)+OPA
	OP_RANDV0,		//95	//3d/box versions of the above.
	OP_RANDV1,		//96
	OP_RANDV2,		//97

	OP_SWITCH_F,	//98	switchref=OPA; PC += OPB   --- the jump allows the jump table (such as it is) to be inserted after the block.
	OP_SWITCH_V,	//99
	OP_SWITCH_S,	//100
	OP_SWITCH_E,	//101
	OP_SWITCH_FNC,	//102

	OP_CASE,		//103	if (OPA===switchref) PC += OPB
	OP_CASERANGE,	//104   if (OPA<=switchref&&switchref<=OPB) PC += OPC





	//the rest are added
	//mostly they are various different ways of adding two vars with conversions.

	//hexen2 calling convention (-TH2 requires us to remap OP_CALLX to these on load, -TFTE just uses these directly.)
	OP_CALL1H,	//OFS_PARM0=OPB
	OP_CALL2H,	//OFS_PARM0,1=OPB,OPC
	OP_CALL3H,	//no extra args
	OP_CALL4H,
	OP_CALL5H,
	OP_CALL6H,		//110
	OP_CALL7H,
	OP_CALL8H,


	OP_STORE_I,
	OP_STORE_IF,			//OPB.f = (float)OPA.i (makes more sense when written as a->b)
	OP_STORE_FI,			//OPB.i = (int)OPA.f
	
	OP_ADD_I,
	OP_ADD_FI,				//OPC.f = OPA.f + OPB.i
	OP_ADD_IF,				//OPC.f = OPA.i + OPB.f	-- redundant...
  
	OP_SUB_I,				//OPC.i = OPA.i - OPB.i
	OP_SUB_FI,		//120	//OPC.f = OPA.f - OPB.i
	OP_SUB_IF,				//OPC.f = OPA.i - OPB.f

	OP_CONV_ITOF,			//OPC.f=(float)OPA.i -- useful mostly so decompilers don't do weird stuff.
	OP_CONV_FTOI,			//OPC.i=(int)OPA.f
	OP_LOADP_ITOF,				//OPC.f=(float)(*OPA).i	-- fixme: rename to LOADP_ITOF
	OP_LOADP_FTOI,				//OPC.i=(int)(*OPA).f
	OP_LOAD_I,
	OP_STOREP_I,
	OP_STOREP_IF,
	OP_STOREP_FI,

	OP_BITAND_I,	//130
	OP_BITOR_I,

	OP_MUL_I,
	OP_DIV_I,
	OP_EQ_I,
	OP_NE_I,

	OP_IFNOT_S,	//compares string empty, rather than just null.
	OP_IF_S,

	OP_NOT_I,

	OP_DIV_VF,

	OP_BITXOR_I,		//140
	OP_RSHIFT_I,
	OP_LSHIFT_I,

	OP_GLOBALADDRESS,	//C.p = &A + B.i*4
	OP_ADD_PIW,			//C.p = A.p + B.i*4

	OP_LOADA_F,
	OP_LOADA_V,	
	OP_LOADA_S,
	OP_LOADA_ENT,
	OP_LOADA_FLD,
	OP_LOADA_FNC,	//150
	OP_LOADA_I,

	OP_STORE_P,
	OP_LOAD_P,

	OP_LOADP_F,
	OP_LOADP_V,	
	OP_LOADP_S,
	OP_LOADP_ENT,
	OP_LOADP_FLD,
	OP_LOADP_FNC,
	OP_LOADP_I,		//160

	OP_LE_I,
	OP_GE_I,
	OP_LT_I,
	OP_GT_I,

	OP_LE_IF,
	OP_GE_IF,
	OP_LT_IF,
	OP_GT_IF,

	OP_LE_FI,
	OP_GE_FI,		//170
	OP_LT_FI,
	OP_GT_FI,

	OP_EQ_IF,
	OP_EQ_FI,

	//-------------------------------------
	//string manipulation.
	OP_ADD_SF,	//(char*)c = (char*)a + (float)b    add_fi->i
	OP_SUB_S,	//(float)c = (char*)a - (char*)b    sub_ii->f
	OP_STOREP_C,//(float)c = *(char*)b = (float)a
	OP_LOADP_C,	//(float)c = *(char*)
	//-------------------------------------


	OP_MUL_IF,
	OP_MUL_FI,		//180
	OP_MUL_VI,
	OP_MUL_IV,
	OP_DIV_IF,
	OP_DIV_FI,
	OP_BITAND_IF,
	OP_BITOR_IF,
	OP_BITAND_FI,
	OP_BITOR_FI,
	OP_AND_I,
	OP_OR_I,		//190
	OP_AND_IF,
	OP_OR_IF,
	OP_AND_FI,
	OP_OR_FI,
	OP_NE_IF,
	OP_NE_FI,

//fte doesn't really model two separate pointer types. these are thus special-case things for array access only.
	OP_GSTOREP_I,
	OP_GSTOREP_F,
	OP_GSTOREP_ENT,
	OP_GSTOREP_FLD,		//200
	OP_GSTOREP_S,
	OP_GSTOREP_FNC,		
	OP_GSTOREP_V,
	OP_GADDRESS,	//poorly defined opcode, which makes it too unreliable to actually use.
	OP_GLOAD_I,
	OP_GLOAD_F,
	OP_GLOAD_FLD,
	OP_GLOAD_ENT,
	OP_GLOAD_S,
	OP_GLOAD_FNC,		//210

//back to ones that we do use.
	OP_BOUNDCHECK,
	OP_UNUSED,	//used to be OP_STOREP_P, which is now emulated with OP_STOREP_I, fteqcc nor fte generated it
	OP_PUSH,	//push 4octets onto the local-stack (which is ALWAYS poped on function return). Returns a pointer.
	OP_POP,		//pop those ones that were pushed (don't over do it). Needs assembler.

	OP_SWITCH_I,//hmm.
	OP_GLOAD_V,
//r3349+
	OP_IF_F,		//compares as an actual float, instead of treating -0 as positive.
	OP_IFNOT_F,

//r5697+
	OP_STOREF_V,	//3 elements...
	OP_STOREF_F,	//1 fpu element...
	OP_STOREF_S,	//1 string reference
	OP_STOREF_I,	//1 non-string reference/int

//r5744+
	OP_STOREP_I8,	//((char*)b)[(int)c] = (int)a
	OP_LOADP_U8,	//(int)c = *(unsigned char*)

//r5768+
//opcodes for 32bit uints
	OP_LE_U,		//aka GE
	OP_LT_U,		//aka GT
	OP_DIV_U,		//don't need mul+add+sub
	OP_RSHIFT_U,	//lshift is the same for signed+unsigned

//opcodes for 64bit ints
	OP_ADD_I64,
	OP_SUB_I64,
	OP_MUL_I64,
	OP_DIV_I64,
	OP_BITAND_I64,
	OP_BITOR_I64,
	OP_BITXOR_I64,
	OP_LSHIFT_I64I,
	OP_RSHIFT_I64I,
	OP_LE_I64,		//aka GE
	OP_LT_I64,		//aka GT
	OP_EQ_I64,
	OP_NE_I64,
//extra opcodes for 64bit uints
	OP_LE_U64,		//aka GE
	OP_LT_U64,		//aka GT
	OP_DIV_U64,
	OP_RSHIFT_U64I,

//general 64bitness
	OP_STORE_I64,
	OP_STOREP_I64,
	OP_STOREF_I64,
	OP_LOAD_I64,
	OP_LOADA_I64,
	OP_LOADP_I64,
//various conversions for our 64bit types (yay type promotion)
	OP_CONV_UI64, //zero extend
	OP_CONV_II64, //sign extend
	OP_CONV_I64I,	//truncate
	OP_CONV_FD,	//extension
	OP_CONV_DF,	//truncation
	OP_CONV_I64F,	//logically a promotion (always signed)
	OP_CONV_FI64,	//demotion (always signed)
	OP_CONV_I64D,	//'promotion' (always signed)
	OP_CONV_DI64,	//demotion (always signed)

//opcodes for doubles.
	OP_ADD_D,
	OP_SUB_D,
	OP_MUL_D,
	OP_DIV_D,
	OP_LE_D,
	OP_LT_D,
	OP_EQ_D,
	OP_NE_D,

//r6614+
	OP_STOREP_I16, //((short*)b)[(int)c] = (int)a
	OP_LOADP_I16,	//(int)c = *(signed short*)a   (sign extends)
	OP_LOADP_U16,	//(unsigned int)c = *(unsigned short*)a
	OP_LOADP_I8,	//(unsigned int)c = *(signed char*)a	(sign extends)
	OP_BITEXTEND_I,	//sign extend (for signed bitfields)
	OP_BITEXTEND_U,	//zero extend (for unsigned bitfields)
	OP_BITCOPY_I,	//copy lower bits from the input to some part of the output
	OP_CONV_UF,	//OPC.f=(float)OPA.i -- 0xffffffffu*0.5=0 otherwise.
	OP_CONV_FU,	//OPC.i=(int)OPA.f
	OP_CONV_U64D,	//OPC.d=(double)OPA.u64 -- useful mostly so decompilers don't do weird stuff.
	OP_CONV_DU64,	//OPC.u64=(uint64_t)OPA.d
	OP_CONV_U64F,	//OPC.f=(float)OPA.u64 -- useful mostly so decompilers don't do weird stuff.
	OP_CONV_FU64,	//OPC.u64=(uint64_t)OPA.f

	OP_NUMREALOPS,

	/*
	These ops are emulated out, always, and are only present in the compiler.
	*/

	OP_BITSETSTORE_I,	//220
	OP_BITSETSTOREP_I,
	OP_BITCLRSTORE_I,

	OP_MULSTORE_I,
	OP_DIVSTORE_I,
	OP_ADDSTORE_I,
	OP_SUBSTORE_I,
	OP_MULSTOREP_I,
	OP_DIVSTOREP_I,
	OP_ADDSTOREP_I,
	OP_SUBSTOREP_I,	//230

	OP_MULSTORE_IF,
	OP_MULSTOREP_IF,
	OP_DIVSTORE_IF,
	OP_DIVSTOREP_IF,
	OP_ADDSTORE_IF,
	OP_ADDSTOREP_IF,
	OP_SUBSTORE_IF,
	OP_SUBSTOREP_IF,

	OP_MULSTORE_FI,
	OP_MULSTOREP_FI,	//240
	OP_DIVSTORE_FI,
	OP_DIVSTOREP_FI,
	OP_ADDSTORE_FI,
	OP_ADDSTOREP_FI,
	OP_SUBSTORE_FI,
	OP_SUBSTOREP_FI,

	OP_MULSTORE_VI,
	OP_MULSTOREP_VI,

	OP_LOADA_STRUCT,
	OP_LOADP_P,
	OP_STOREP_P,

	OP_BITNOT_F,
	OP_BITNOT_I,

	OP_EQ_P,
	OP_NE_P,
	OP_LE_P,
	OP_GE_P,
	OP_LT_P,
	OP_GT_P,

	OP_ANDSTORE_F,
	OP_BITCLR_F,
	OP_BITCLR_I,
	OP_BITCLR_V,

	OP_ADD_SI,
	OP_ADD_IS,
	OP_ADD_PF,
	OP_ADD_FP,
	OP_ADD_PI,
	OP_ADD_IP,
	OP_ADD_PU,
	OP_ADD_UP,

	OP_SUB_SI,
	OP_SUB_PF,
	OP_SUB_PI,
	OP_SUB_PU,

	OP_SUB_PP,

	OP_MOD_F,
	OP_MOD_I,
	OP_MOD_FI,
	OP_MOD_IF,
	OP_MOD_V,

	OP_BITXOR_F,
	OP_RSHIFT_F,
	OP_LSHIFT_F,
	OP_RSHIFT_IF,
	OP_LSHIFT_IF,
	OP_RSHIFT_FI,
	OP_LSHIFT_FI,

	OP_AND_ANY,
	OP_OR_ANY,

	OP_ADD_EI,
	OP_ADD_EF,
	OP_SUB_EI,
	OP_SUB_EF,

	OP_BITAND_V,
	OP_BITOR_V,
	OP_BITNOT_V,
	OP_BITXOR_V,

	OP_POW_F,
	OP_POW_I,
	OP_POW_FI,
	OP_POW_IF,
	OP_CROSS_V,

	OP_EQ_FLD,
	OP_NE_FLD,

	OP_SPACESHIP_F,	//lame
	OP_SPACESHIP_S,	//basically strcmp.


	//uint32 opcodes. they match the int32 ones so emulation is basically swapping them over.
	OP_ADD_U,
	OP_SUB_U,
	OP_MUL_U,
	OP_MOD_U,	//complex
	OP_BITAND_U,
	OP_BITOR_U,
	OP_BITXOR_U,
	OP_BITNOT_U,	//BITXOR ~0
	OP_BITCLR_U,
	OP_LSHIFT_U,	//same as signed (unlike rshift)
	OP_GE_U,	//LT_U
	OP_GT_U,	//LE_U
//	OP_AND_U,
//	OP_OR_U,
	OP_EQ_U,
	OP_NE_U,

	//uint64 opcodes. they match the int32 ones so emulation is basically swapping them over.
	OP_BITNOT_I64,	//BITXOR ~0
	OP_BITCLR_I64,
	OP_GE_I64,	//LE_I64
	OP_GT_I64,	//LT_I64

	OP_ADD_U64,
	OP_SUB_U64,
	OP_MUL_U64,
	OP_MOD_U64,	//complex
	OP_BITAND_U64,
	OP_BITOR_U64,
	OP_BITXOR_U64,
	OP_BITNOT_U64,	//BITXOR ~0
	OP_BITCLR_U64,
	OP_LSHIFT_U64I,
	OP_GE_U64,	//LE_U64
	OP_GT_U64,	//LT_U64
	OP_EQ_U64,
	OP_NE_U64,

	//generally implemented by forcing to int64.
	OP_BITAND_D,
	OP_BITOR_D,
	OP_BITXOR_D,
	OP_BITNOT_D,
	OP_BITCLR_D,
	OP_LSHIFT_DI,
	OP_RSHIFT_DI,
	OP_GE_D,	//LE_D
	OP_GT_D,	//LT_D

	OP_WSTATE,	//for the 'w' part of CWSTATE. will probably never be used, but hey, hexen2...

	//special/fake opcodes used by the decompiler.
	OPD_GOTO_FORSTART,
	OPD_GOTO_WHILE1,
	OPD_GOTO_BREAK,
	OPD_GOTO_DEFAULT,

	OP_NUMOPS,
#define OP_BIT_BREAKPOINT 0x8000
};

#define	MAX_PARMS	8

// qtest structs (used for reordering and not execution)
typedef struct qtest_statement_s
{
	unsigned int	line; // line number in source code file
	unsigned short	op;
	unsigned short	a,b,c;
} qtest_statement_t;

typedef struct qtest_def_s
{
	unsigned int	type; // no DEFGLOBAL found in qtest progs
	unsigned int	s_name; // different order!
	unsigned int	ofs;
} qtest_def_t;

typedef struct qtest_function_s
{
	int		first_statement;
	int		unused1;
	int		locals;	// assumed! (always 0 in real qtest progs)
	int		profile; // assumed! (always 0 in real qtest progs)

	int		s_name;
	int		s_file;

	int		numparms;
	int		parm_start; // different order
	int		parm_size[MAX_PARMS]; // ints instead of bytes...
} qtest_function_t;

typedef struct statement16_s
{
	unsigned short	op;
	unsigned short	a,b,c;
} dstatement16_t;
typedef struct statement32_s
{
	unsigned int	op;
	unsigned int	a,b,c;
} dstatement32_t;
#define QCC_dstatement16_t dstatement16_t
#define QCC_dstatement32_t dstatement32_t

typedef struct
{
	struct QCC_def_s *sym;
	union
	{
		unsigned int ofs;
//		unsigned int bofs;
		signed int jumpofs;
	};
	struct QCC_type_s *cast;	//the entire sref is considered null if there is no cast, although it *MAY* have an ofs specified if its part of a jump instruction
} QCC_sref_t;
typedef struct qcc_statement_s
{
	unsigned short		op;
	#define STF_LOGICOP (1u<<0)	//do not bother following when looking for uninitialised variables.
	#define STF_NOFOLD (1u<<1)	//do not allow changing its var_c to fold the following store.
	unsigned short		flags;
	QCC_sref_t			a, b, c;
	unsigned int		linenum;
} QCC_statement_t;

//these should be the same except the string type
typedef struct ddef16_s
{
	unsigned short	type;		// if DEF_SAVEGLOBAL bit is set
								// the variable needs to be saved in savegames
	unsigned short	ofs;
	string_t		s_name;
} ddef16_t;

typedef struct ddef32_s
{
	unsigned int	type;		// if DEF_SAVEGLOBAL bit is set
								// the variable needs to be saved in savegames
	unsigned int	ofs;
	string_t		s_name;
} ddef32_t;

typedef void *ddefXX_t;

typedef struct QCC_ddef16_s
{
	unsigned short	type;		// if DEF_SAVEGLOBAL bit is set
								// the variable needs to be saved in savegames
	unsigned short	ofs;
	QCC_string_t		s_name;
} QCC_ddef16_t;

typedef struct QCC_ddef32_s
{
	unsigned int	type;		// if DEF_SAVEGLOBAL bit is set
								// the variable needs to be saved in savegames
	unsigned int	ofs;
	QCC_string_t		s_name;
} QCC_ddef32_t;
#define QCC_ddef_t QCC_ddef32_t

#define	DEF_SAVEGLOBAL 		(1<<15)
#define	DEF_SHARED 		(1<<14)

typedef struct
{
	int		first_statement;	// negative numbers are builtins
	int		parm_start;
	int		locals;				// total ints of parms + locals

	int		profile;		// runtime

	string_t	s_name;
	string_t	s_file;			// source file defined in

	int		numparms;
	pbyte	parm_size[MAX_PARMS];
} dfunction_t;

typedef struct
{
	int		first_statement;	// negative numbers are builtins
	int		parm_start;
	int		locals;				// total ints of parms + locals

	int		profile;						//number of qc instructions executed.
	prclocks_t profiletime;			//total time inside (cpu cycles)
	prclocks_t profilechildtime;	//time inside children (excluding builtins, cpu cycles)

	string_t	s_name;
	string_t	s_file;			// source file defined in

	int		numparms;
	pbyte	parm_size[MAX_PARMS];
} mfunction_t;

#define PROG_QTESTVERSION	3
#define	PROG_VERSION	6
#define PROG_KKQWSVVERSION 7
#define	PROG_EXTENDEDVERSION	7
#define PROG_SECONDARYVERSION16 ((('1'<<0)|('F'<<8)|('T'<<16)|('E'<<24))^(('P'<<0)|('R'<<8)|('O'<<16)|('G'<<24)))	//something unlikly and still meaningful (to me)
#define PROG_SECONDARYVERSION32 ((('1'<<0)|('F'<<8)|('T'<<16)|('E'<<24))^(('3'<<0)|('2'<<8)|('B'<<16)|(' '<<24)))	//something unlikly and still meaningful (to me)
#define PROG_SECONDARYUHEXEN2	((('U'<<0)|('H'<<8)|('2'<<16)|('7'<<24)))	//something unlikly and still meaningful (to me)
#define PROG_SECONDARYKKQWSV	((('K'<<0)|('K'<<8)|('Q'<<16)|('W'<<24)))	//something unlikly and still meaningful (to me)
typedef struct
{
	int		version;
	int		crc;			// check of header file

	unsigned int		ofs_statements;	//comp 1
	unsigned int		numstatements;	// statement 0 is an error

	unsigned int		ofs_globaldefs;	//comp 2
	unsigned int		numglobaldefs;

	unsigned int		ofs_fielddefs;	//comp 4
	unsigned int		numfielddefs;

	unsigned int		ofs_functions;	//comp 8
	unsigned int		numfunctions;	// function 0 is an empty

	unsigned int		ofs_strings;	//comp 16
	unsigned int		numstrings;		// first string is a null string

	unsigned int		ofs_globals;	//comp 32
	unsigned int		numglobals;

	unsigned int		entityfields;

	//debug / version 7 extensions
	unsigned int		ofsfiles;	//non list format. no comp
	unsigned int		ofslinenums;	//numstatements big	//comp 64
	unsigned int		ofsbodylessfuncs;	//no comp
	unsigned int		numbodylessfuncs;

	unsigned int	ofs_types;	//comp 128
	unsigned int	numtypes;
	unsigned int	blockscompressed;

	int	secondaryversion;	//Constant - to say that any version 7 progs are actually ours, not someone else's alterations.
} dprograms_t;
#define standard_dprograms_t_size ((size_t)&((dprograms_t*)NULL)->ofsfiles)





typedef struct {
	char filename[128];
	int size;
	int compsize;
	int compmethod;
	int ofs;
} includeddatafile_t;




typedef struct typeinfo_s
{
	etype_t	type;

	int		next;
	int		aux_type;
	int		num_parms;

	int		ofs;	//inside a structure.
	int		size;
	string_t	name;
} typeinfo_t;

#endif
