/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// this file is shared by quake and qcc

#ifndef PR_COMP_H
#define PR_COMP_H

typedef unsigned int	func_t;
typedef int	string_t;

typedef enum etype_e {ev_void, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_pointer} etype_t;


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


typedef enum opcode_e
{
	OP_DONE,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF,
	OP_IFNOT,
	OP_CALL0,
	OP_CALL1,
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,
	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR,

	// TODO: actually support Hexen 2?
	
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

	//hexen2 calling convention (-TH2 requires us to remap OP_CALLX to these on load, -TFTE just uses these directly.)
	OP_CALL1H,	//OFS_PARM0=OPB
	OP_CALL2H,	//OFS_PARM0,1=OPB,OPC
	OP_CALL3H,	//no extra args
	OP_CALL4H,
	OP_CALL5H,
	OP_CALL6H,
	OP_CALL7H,
	OP_CALL8H,

	OP_STORE_I,
	OP_STORE_IF,
	OP_STORE_FI,

	OP_ADD_I,
	OP_ADD_FI,
	OP_ADD_IF,

	OP_SUB_I,
	OP_SUB_FI,
	OP_SUB_IF,

	OP_CONV_IF,
	OP_CONV_FI,
	
	OP_LOADP_IF,
	OP_LOADP_FI,

	OP_LOAD_I,

	OP_STOREP_I,
	OP_STOREP_IF,
	OP_STOREP_FI,

	OP_BITAND_I,
	OP_BITOR_I,

	OP_MUL_I,
	OP_DIV_I,
	OP_EQ_I,
	OP_NE_I,

	OP_IFNOT_S,

	OP_IF_S,

	OP_NOT_I,

	OP_DIV_VF,

	OP_BITXOR_I,
	OP_RSHIFT_I,
	OP_LSHIFT_I,

	OP_GLOBALADDRESS,
	OP_ADD_PIW,

	OP_LOADA_F,
	OP_LOADA_V,	
	OP_LOADA_S,
	OP_LOADA_ENT,
	OP_LOADA_FLD,
	OP_LOADA_FNC,
	OP_LOADA_I,

	OP_STORE_P,
	OP_LOAD_P,

	OP_LOADP_F,
	OP_LOADP_V,	
	OP_LOADP_S,
	OP_LOADP_ENT,
	OP_LOADP_FLD,
	OP_LOADP_FNC,
	OP_LOADP_I,

	OP_LE_I,
	OP_GE_I,
	OP_LT_I,
	OP_GT_I,
	
	OP_LE_IF,
	OP_GE_IF,
	OP_LT_IF,
	OP_GT_IF,

	OP_LE_FI,
	OP_GE_FI,
	OP_LT_FI,
	OP_GT_FI,

	OP_EQ_IF,
	OP_EQ_FI,

	OP_ADD_SF,
	OP_SUB_S,
	OP_STOREP_C,
	OP_LOADP_C,

	OP_MUL_IF,
	OP_MUL_FI,
	OP_MUL_VI,
	OP_MUL_IV,
	OP_DIV_IF,
	OP_DIV_FI,
	OP_BITAND_IF,
	OP_BITOR_IF,
	OP_BITAND_FI,
	OP_BITOR_FI,
	OP_AND_I,
	OP_OR_I,
	OP_AND_IF,
	OP_OR_IF,
	OP_AND_FI,
	OP_OR_FI,
	OP_NE_IF,
	OP_NE_FI,

	OP_GSTOREP_I,
	OP_GSTOREP_F,
	OP_GSTOREP_ENT,
	OP_GSTOREP_FLD,
	OP_GSTOREP_S,
	OP_GSTOREP_FNC,		
	OP_GSTOREP_V,
	OP_GADDRESS,
	OP_GLOAD_I,
	OP_GLOAD_F,
	OP_GLOAD_FLD,
	OP_GLOAD_ENT,
	OP_GLOAD_S,
	OP_GLOAD_FNC,
	OP_BOUNDCHECK,

	OP_UNUSED,	//used to be OP_STOREP_P, which is now emulated with OP_STOREP_I, fteqcc nor fte generated it
	OP_PUSH,	//push 4octets onto the local-stack (which is ALWAYS poped on function return). Returns a pointer.
	OP_POP,		//pop those ones that were pushed (don't over do it). Needs assembler.
	OP_SWITCH_I,

	OP_GLOAD_V,

	OP_IF_F,		//compares as an actual float, instead of treating -0 as positive.
	OP_IFNOT_F,

	OP_STOREF_V,	//3 elements...
	OP_STOREF_F,	//1 fpu element...
	OP_STOREF_S,	//1 string reference
	OP_STOREF_I,	//1 non-string reference/int
}
opcode_t;

// Statements (16 bit format) - 8 bytes each
typedef struct statement16_s
{
	uint16_t	op;
	int16_t		a, b, c;
}
dstatement16_t;

// Statements (32 bit format) - 16 bytes each
typedef struct statement32_s
{
	uint32_t	op;
	int32_t		a, b, c;
}
dstatement32_t;

// Global and fielddefs (16 bit format) - 8 bytes each
typedef struct ddef16_s
{
	uint16_t	type;		// if DEF_SAVEGLOBAL bit is set
							// the variable needs to be saved in savegames
	uint16_t	ofs;
	int32_t	s_name;
}
ddef16_t, dfield16_t;

// Global and fielddefs (32 bit format) - 12 bytes each
typedef struct ddef32_s
{
	uint32_t	type;		// if DEF_SAVEGLOBAL bit is set
							// the variable needs to be saved in savegames
	uint32_t	ofs;
	int32_t	s_name;
}
ddef32_t, dfield32_t, mdef_t;

#define	DEF_SAVEGLOBAL	(1<<15)

#define	MAX_PARMS	8

// Functions - 36 bytes each
typedef struct dfunction_s
{
	int32_t			first_statement;	// negative numbers are builtins
	int32_t		parm_start;			// first local
	int32_t		locals;				// total ints of parms + locals

	int32_t		profile;		// runtime

	int32_t		s_name;			// function name
	int32_t		s_file;			// source file defined in

	int32_t			numparms;		// number of args
	uint8_t			parm_size[MAX_PARMS]; // and size
}
dfunction_t;

typedef struct mfunction_s
{
	int32_t			first_statement;	// negative numbers are builtins
	int32_t		parm_start;
	int32_t		locals;				// total ints of parms + locals

	// these are doubles so that they can count up to 54bits or so rather than 32bit
	double  tprofile;           // realtime in this function
	double  tbprofile;          // realtime in builtins called by this function (NOTE: builtins also have a tprofile!)
	double	profile;		// runtime
	double	builtinsprofile; // cost of builtin functions called by this function
	double	callcount; // times the functions has been called since the last profile call
	double  totaltime; // total execution time of this function DIRECTLY FROM THE ENGINE
	double	tprofile_total;		// runtime (NOTE: tbprofile_total makes no real sense, so not accumulating that)
	double	profile_total;		// runtime
	double	builtinsprofile_total; // cost of builtin functions called by this function
	int     recursion;

	int32_t		s_name;
	int32_t		s_file;			// source file defined in

	int32_t		numparms;
	uint8_t		parm_size[MAX_PARMS];
}
mfunction_t;

typedef struct mstatement_s
{
	opcode_t	op;
	int			operand[3]; // always a global or -1 for unused
	int			jumpabsolute; // only used by IF, IFNOT, GOTO
}
mstatement_t;

// Header - 64 bytes
#define	PROG_VERSION	6
typedef struct dprograms_s
{
	int32_t	version;		// Version (usually 6)
	int32_t	crc;			// CRC-16 of header file. Last 2 bytes are skipped

	// Sizes of and offsets to each section
	uint32_t	ofs_statements;
	uint32_t	numstatements;	// statement 0 is an error

	uint32_t	ofs_globaldefs;
	uint32_t	numglobaldefs;

	uint32_t	ofs_fielddefs;
	uint32_t	numfielddefs;

	uint32_t	ofs_functions;
	uint32_t	numfunctions;	// function 0 is an empty

	uint32_t	ofs_strings;
	uint32_t	numstrings;		// first string is a null string

	uint32_t	ofs_globals;
	uint32_t	numglobals;

	uint32_t	entityfields;
}
dprograms_t;

typedef struct dprograms_v7_s
{	//extended header written by fteqcc.
	dprograms_t	v6;	//for easier casting.

	//debug / version 7 extensions
	uint32_t	ofsfiles;			//ignored. deprecated, should be 0. source files can instead be embedded by simply treating the .dat as a zip.
	uint32_t	ofslinenums;		//ignored. alternative to external .lno files.
	uint32_t	ofsbodylessfuncs;	//unsupported. function names imported from other modules. must be 0.
	uint32_t	numbodylessfuncs;	//unsupported. must be 0.

	uint32_t	ofs_types;			//unsupported+deprecated. rich type info. must be 0.
	uint32_t	numtypes;			//unsupported+deprecated. rich type info. must be 0.
	uint32_t	blockscompressed;	//unsupported. per-block compression. must be 0.

	int32_t		secondaryversion;	//if not known then its kkqwsv's v7, qfcc's v7, or uhexen2's v7, or something. abandon all hope when not recognised.
#define PROG_SECONDARYVERSION16 ((('1'<<0)|('F'<<8)|('T'<<16)|('E'<<24))^(('P'<<0)|('R'<<8)|('O'<<16)|('G'<<24)))	//regular 16bit statements.
#define PROG_SECONDARYVERSION32 ((('1'<<0)|('F'<<8)|('T'<<16)|('E'<<24))^(('3'<<0)|('2'<<8)|('B'<<16)|(' '<<24)))	//statements+globaldefs+fielddefs extended to 32bit.
}
dprograms_v7_t;

#endif

