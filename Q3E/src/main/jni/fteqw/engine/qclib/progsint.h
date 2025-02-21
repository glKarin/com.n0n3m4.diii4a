#ifndef PROGSINT_H_INCLUDED
#define PROGSINT_H_INCLUDED

#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#define _CRT_NONSTDC_NO_WARNINGS
	#ifndef _CRT_SECURE_NO_DEPRECATE
		#define _CRT_SECURE_NO_DEPRECATE
	#endif
	#ifndef _CRT_NONSTDC_NO_DEPRECATE
		#define _CRT_NONSTDC_NO_DEPRECATE
	#endif
	#ifndef AVAIL_ZLIB
		#ifdef _MSC_VER
			//#define AVAIL_ZLIB
		#endif
	#endif
#ifndef _XBOX
	#include <windows.h>
#else 
	#include <xtl.h>
#endif
#else
	#include <stdarg.h>
	#include <math.h>

	#include <stdlib.h>
	#include <setjmp.h>
	#include <string.h>
	#include <ctype.h>

	#ifndef __declspec
		#define __declspec(mode)
	#endif
//#define _inline inline
#endif
typedef unsigned char pbyte;
#include <stdio.h>

#define DLL_PROG
#ifndef PROGSUSED
#define PROGSUSED
#endif

#define false 0
#define true 1

#include "progtype.h"
#include "progslib.h"

#include "pr_comp.h"

#ifndef safeswitch
	//safeswitch(foo){safedefault: break;}
	//switch, but errors for any omitted enum values despite the presence of a default case.
	//(gcc will generally give warnings without the default, but sometimes you don't have control over the source of your enumeration values)
	#if (__GNUC__ >= 4)
		#define safeswitch	\
			_Pragma("GCC diagnostic push")	\
			_Pragma("GCC diagnostic error \"-Wswitch-enum\"") \
			_Pragma("GCC diagnostic error \"-Wswitch-default\"") \
			switch
		#define safedefault _Pragma("GCC diagnostic pop") default
	#else
		#define safeswitch switch
		#define safedefault default
	#endif
#endif


#ifdef _MSC_VER
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

#ifndef stricmp
#ifdef _WIN32
	//Windows-specific...
	#define stricmp _stricmp
	#define strnicmp _strnicmp
#else
	//Posix
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif
#endif

//extern progfuncs_t *progfuncs;
typedef struct sharedvar_s
{
	int varofs;
	int size;
} sharedvar_t;
typedef struct
{
	mfunction_t		*f;
	unsigned char	stepping;
	unsigned char	progsnum;
	int				s;
	int				pushed;
	prclocks_t		timestamp;
} prstack_t;

#if defined(QCGC) && defined(MULTITHREAD)
	#define THREADEDGC
#endif

typedef struct
{
	unsigned int size;			//size of the data.
	char value[4];				//contents of the tempstring (or really any binary data - but not tempstring references because we don't mark these!).
} tempstr_t;

//FIXME: the defines hidden inside this structure are evil.
typedef struct prinst_s
 {
	//temp strings are GCed, and can be created by engine, builtins, or just by ent parsing code.
	tempstr_t **tempstrings;
	unsigned int maxtempstrings;
#if defined(QCGC)
	unsigned int nexttempstring;
	unsigned int livetemps;	//increased on alloc, decremented after sweep
	#ifdef THREADEDGC
		struct qcgccontext_s *gccontext;
	#endif
#else
	unsigned int numtempstrings;
	unsigned int numtempstringsstack;
#endif

	//alloced strings are generally used to direct strings outside of the vm itself, sharing them with general engine state.
	char **allocedstrings;
	int maxallocedstrings;
	int numallocedstrings;

	struct progstate_s * progstate;
#define pr_progstate prinst.progstate
	unsigned int maxprogs;

	progsnum_t pr_typecurrent;	//active index into progstate array. fixme: remove in favour of only using current_progstate

	struct progstate_s *current_progstate;
#define current_progstate prinst.current_progstate

	char * watch_name;
	eval_t * watch_ptr;
	eval_t watch_old;
	etype_t watch_type;

	unsigned int numshares;
	sharedvar_t *shares;	//shared globals, not including parms
	unsigned int maxshares;

	struct prmemb_s     *memblocks;

	unsigned int maxfields;
	unsigned int numfields;
	fdef_t *field;	//biggest size

	int reorganisefields;


//pr_exec.c
	//call stack
#define	MAX_STACK_DEPTH		1024	//insanely high value requried for xonotic.
	prstack_t pr_stack[MAX_STACK_DEPTH];
	int pr_depth;

	//locals
#define	LOCALSTACK_SIZE		(65536*16)	//in words
	int *localstack;
	int localstack_used;
	int spushed; //extra

	//step-by-step debug state
	int debugstatement;
	int exitdepth;

	pbool profiling;
	prclocks_t profilingalert;	//one second, in cpu clocks
	mfunction_t	*pr_xfunction;	//active function
	int pr_xstatement;			//active statement

//pr_edict.c
	evalc_t spawnflagscache;
	unsigned int fields_size;	// in bytes
	unsigned int max_fields_size;


//initlib.c
	int mfreelist;
	char * addressablehunk;
	size_t addressableused;
	size_t addressablesize;

	unsigned int maxedicts;
	struct edictrun_s **edicttable;
} prinst_t;

typedef struct progfuncs_s
{
	struct pubprogfuncs_s funcs;
	struct prinst_s	inst;	//private fields. Leave alone.
} progfuncs_t;

#define prinst progfuncs->inst
#define externs progfuncs->funcs.parms

#include "qcd.h"

#define STRING_SPECMASK	0xc0000000	//
#define STRING_TEMP		0x80000000	//temp string, will be collected.
#define STRING_STATIC	0xc0000000	//pointer to non-qcvm string.
#define STRING_NORMAL_	0x00000000	//stringtable/mutable. should always be a fallthrough
#define STRING_NORMAL2_	0x40000000	//stringtable/mutable. should always be a fallthrough

typedef struct
{
	int			targetflags;	//weather we need to mark the progs as a newer version
	char		*name;
	char		*opname;
	int		priorityclass;
	enum {ASSOC_LEFT, ASSOC_RIGHT, ASSOC_RIGHT_RESULT}			associative;
	struct QCC_type_s		**type_a, **type_b, **type_c;

	unsigned int flags;	//OPF_*
	//ASSIGNS_B
	//ASSIGNS_IB
	//ASSIGNS_C
	//ASSIGNS_IC
} QCC_opcode_t;
extern	QCC_opcode_t	pr_opcodes[];		// sized by initialization
#define OPF_VALID		0x001	//we're allowed to use this opcode in the current target.
#define OPF_STD			0x002	//reads a+b, writes c.
#define OPF_STORE		0x010	//b+=a or just b=a
#define OPF_STOREPTR	0x020	//the form of c=(*b+=a)
#define OPF_STOREPTROFS	0x040	//a[c] <- b   (c must be 0 when QCC_OPCode_StorePOffset returns false)
#define OPF_STOREFLD	0x080	//a.b <- c
#define OPF_LOADPTR		0x100
#define OPF_STDUNARY	0x200	//reads a, writes c.
//FIXME: add jumps



#if defined(_MSC_VER) && _MSC_VER < 1900
#define Q_vsnprintf _vsnprintf
#else
#define Q_vsnprintf vsnprintf
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define sv_num_edicts (*externs->num_edicts)
#define sv_edicts (*externs->edicts)

#define PR_DPrintf externs->DPrintf
//#define printf syntax error
//#define Sys_Error externs->Sys_Error

int PRHunkMark(progfuncs_t *progfuncs);
void PRHunkFree(progfuncs_t *progfuncs, int mark);
void *PRHunkAlloc(progfuncs_t *progfuncs, int size, const char *name);
void *PRAddressableExtend(progfuncs_t *progfuncs, void *src, size_t srcsize, int pad);

#ifdef printf
#undef LIKEPRINTF
#define LIKEPRINTF(x)
#endif

//void *HunkAlloc (int size);
char *VARGS qcva (char *text, ...) LIKEPRINTF(1);
void QC_InitShares(progfuncs_t *progfuncs);
void QC_StartShares(progfuncs_t *progfuncs);
void PDECL QC_AddSharedVar(pubprogfuncs_t *progfuncs, int num, int type);
void PDECL QC_AddSharedFieldVar(pubprogfuncs_t *progfuncs, int num, char *stringtable);
void QC_AddFieldGlobal(pubprogfuncs_t *ppf, int *globdata);
int PDECL QC_RegisterFieldVar(pubprogfuncs_t *progfuncs, unsigned int type, const char *name, signed long requestedpos, signed long originalofs);
pbool PDECL QC_Decompile(pubprogfuncs_t *progfuncs, const char *fname);
int PDECL PR_ToggleBreakpoint(pubprogfuncs_t *progfuncs, const char *filename, int linenum, int flag);
void    StripExtension (char *path);


#define edvars(ed) (((edictrun_t*)ed)->fields)	//pointer to the field vars, given an edict


void SetEndian(void);
extern short   (*PRBigShort) (short l);
extern short   (*PRLittleShort) (short l);
extern int     (*PRBigLong) (int l);
extern int     (*PRLittleLong) (int l);
extern float   (*PRBigFloat) (float l);
extern float   (*PRLittleFloat) (float l);



/*
#ifndef COMPILER
typedef union eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				_int;
	int				edict;
	progsnum_t		prog;	//so it can easily be changed
} eval_t;
#endif
*/
typedef struct edictrun_s
{
	enum ereftype_e	ereftype;
	float			freetime;			// realtime when the object was freed
	unsigned int	entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//causes error when QC tries writing to it. (quake's world entity)
	void			*fields;

// other fields from progs come immediately after
} edictrun_t;


int PDECL Comp_Begin(pubprogfuncs_t *progfuncs, int nump, const char **parms);
int PDECL Comp_Continue(pubprogfuncs_t *progfuncs);

pbool PDECL PR_SetWatchPoint(pubprogfuncs_t *progfuncs, const char *desc, const char *location);
char *PDECL PR_EvaluateDebugString(pubprogfuncs_t *progfuncs, const char *key);
char *PDECL PR_SaveEnts(pubprogfuncs_t *progfuncs, char *mem, size_t *size, size_t maxsize, int mode);
int PDECL PR_LoadEnts(pubprogfuncs_t *ppf, const char *file, void *ctx, void (PDECL *memoryreset) (pubprogfuncs_t *progfuncs, void *ctx), void (PDECL *entspawned) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend), pbool(PDECL *extendedterm)(pubprogfuncs_t *progfuncs, void *ctx, const char **extline));
char *PDECL PR_SaveEnt (pubprogfuncs_t *progfuncs, char *buf, size_t *size, size_t maxsize, struct edict_s *ed);
struct edict_s *PDECL PR_RestoreEnt (pubprogfuncs_t *progfuncs, const char *buf, size_t *size, struct edict_s *ed);
void PDECL PR_StackTrace (pubprogfuncs_t *progfuncs, int showlocals);

eval_t *PR_GetReadTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize);
eval_t *PR_GetWriteTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize);

extern int noextensions;

typedef enum
{
	PST_DEFAULT,//everything 16bit
	PST_FTE32,	//everything 32bit
	PST_KKQWSV, //32bit statements, 16bit globaldefs. NO SAVED GAMES.
	PST_QTEST,	//16bit statements, 32bit globaldefs(other differences converted on load)
	PST_UHEXEN2,//everything 32bit like fte's without a header, but with pre-padding rather than post-extended (little-endian) types.
} progstructtype_t;

#ifndef COMPILER
typedef struct progstate_s
{
	dprograms_t		*progs;
	mfunction_t		*functions;
	char			*strings;
	union {
		ddefXX_t		*globaldefs;
		ddef16_t		*globaldefs16;	//vanilla, kk
		ddef32_t		*globaldefs32;	//fte, qtest
	};
	union {
		ddefXX_t		*fielddefs;
		ddef16_t		*fielddefs16;	//vanilla, kk
		ddef32_t		*fielddefs32;	//fte, qtest
	};
//	union {
		void	*statements;
//		dstatement16_t *statements16;	//vanilla, qtest
//		dstatement32_t *statements32;	//fte, kk
//	};
//	void			*global_struct;
	float			*globals;			// same as pr_global_struct
	unsigned int	globals_bytes;	// in bytes

	typeinfo_t	*types;

	int				edict_size;	// in bytes

	char			filename[128];

	int *linenums;	//debug versions only

	progstructtype_t structtype;	//specifies the sized struct types above. FIXME: should probably just load as 32bit or something.

#ifdef QCJIT
	struct jitstate *jit;
#endif
} progstate_t;

//============================================================================


#define pr_progs			current_progstate->progs
#define	pr_cp_functions		current_progstate->functions
#define	pr_strings			current_progstate->strings
#define	pr_globaldefs16		((ddef16_t*)current_progstate->globaldefs16)
#define	pr_globaldefs32		((ddef32_t*)current_progstate->globaldefs32)
#define	pr_fielddefs16		((ddef16_t*)current_progstate->fielddefs16)
#define	pr_fielddefs32		((ddef32_t*)current_progstate->fielddefs32)
#define	pr_statements16		((dstatement16_t*)current_progstate->statements)
#define	pr_statements32		((dstatement32_t*)current_progstate->statements)
//#define	pr_global_struct	current_progstate->global_struct
#define pr_globals			current_progstate->globals
#define pr_linenums			current_progstate->linenums
#define pr_types			current_progstate->types



//============================================================================

void PR_Init (void);

pbool PR_RunWarning (pubprogfuncs_t *progfuncs, char *error, ...);

void PDECL PR_ExecuteProgram (pubprogfuncs_t *progfuncs, func_t fnum);
progsnum_t PDECL PR_LoadProgs(pubprogfuncs_t *progfncs, const char *s);
pbool PR_ReallyLoadProgs (progfuncs_t *progfuncs, const char *filename, progstate_t *progstate, pbool complain);

void *PRHunkAlloc(progfuncs_t *progfuncs, int ammount, const char *name);

void PR_Profile_f (void);

struct edict_s *PDECL ED_Alloc (pubprogfuncs_t *progfuncs, pbool object, size_t extrasize);
struct edict_s *PDECL ED_AllocIndex (pubprogfuncs_t *progfuncs, unsigned int num, pbool object, size_t extrasize);
void PDECL ED_Free (pubprogfuncs_t *progfuncs, struct edict_s *ed, pbool instant);

#ifdef QCGC
void PR_RunGC			(progfuncs_t *progfuncs);
#else
void PR_FreeTemps			(progfuncs_t *progfuncs, int depth);
#endif

string_t PDECL PR_AllocTempString			(pubprogfuncs_t *ppf, const char *str);
char *PDECL ED_NewString (pubprogfuncs_t *ppf, const char *string, int minlength, pbool demarkup);
// returns a copy of the string allocated from the server's string heap

void PDECL ED_Print (pubprogfuncs_t *progfuncs, struct edict_s *ed);
//void ED_Write (FILE *f, edictrun_t *ed);

//void ED_WriteGlobals (FILE *f);
void ED_ParseGlobals (char *data);

//void ED_LoadFromFile (char *data);

//define EDICT_NUM(n) ((edict_t *)(sv.edicts+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv.edicts)/pr_edict_size)

struct edict_s *PDECL QC_EDICT_NUM(pubprogfuncs_t *progfuncs, unsigned int n);
unsigned int PDECL QC_NUM_FOR_EDICT(pubprogfuncs_t *progfuncs, struct edict_s *e);

#define EDICT_NUM(pf, num)	QC_EDICT_NUM(&pf->funcs,num)
#define NUM_FOR_EDICT(pf, e) QC_NUM_FOR_EDICT(&pf->funcs,e)

//#define	NEXT_EDICT(e) ((edictrun_t *)( (byte *)e + pr_edict_size))

#define	EDICT_TO_PROG(pf, e) (((edictrun_t*)e)->entnum)
#define PROG_TO_EDICT_PB(pf, e) ((struct edictrun_s *)prinst.edicttable[e])	//index already validated
#define PROG_TO_EDICT_UB(pf, e) ((struct edictrun_s *)prinst.edicttable[((unsigned int)(e)<prinst.maxedicts)?e:0])	//(safely) pokes world if the index is otherwise invalid

//============================================================================

#define	G_FLOAT(o) (pr_globals[o])
#define	G_FLOAT2(o) (pr_globals[OFS_PARM0 + o*3])
#define	G_INT(o) (*(int *)&pr_globals[o])
#define	G_EDICT(o) ((edict_t *)((qbyte *)sv_edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (*(string_t *)&pr_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])
#define G_PROG(o) G_FLOAT(o)	//simply so it's nice and easy to change...

#define	RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
//#define	E_STRING(e,o) (*(string_t *)&((float*)(e+1))[o])

extern	const unsigned int		type_size[];


extern	unsigned short		pr_crc;

void VARGS PR_RunError (pubprogfuncs_t *progfuncs, const char *error, ...) LIKEPRINTF(2);

void ED_PrintEdicts (progfuncs_t *progfuncs);
void ED_PrintNum (progfuncs_t *progfuncs, int ent);


pbool PR_SwitchProgs(progfuncs_t *progfuncs, progsnum_t type);
pbool PR_SwitchProgsParms(progfuncs_t *progfuncs, progsnum_t newprogs);




eval_t *PDECL QC_GetEdictFieldValue(pubprogfuncs_t *progfuncs, struct edict_s *ed, const char *name, etype_t type, evalc_t *cache);
void PDECL PR_GenerateStatementString (pubprogfuncs_t *progfuncs, int statementnum, char *out, int outlen);
fdef_t *PDECL ED_FieldInfo (pubprogfuncs_t *progfuncs, unsigned int *count);
char *PDECL PR_UglyValueString (pubprogfuncs_t *progfuncs, etype_t type, eval_t *val);
pbool	PDECL ED_ParseEval (pubprogfuncs_t *progfuncs, eval_t *eval, int type, const char *s);
char *PR_SaveCallStack (progfuncs_t *progfuncs, char *buf, size_t *bufofs, size_t bufmax);

prclocks_t Sys_GetClockRate(void);
#endif




#ifndef COMPILER

//this is windows - all files are written with this endian standard
//optimisation
//leave undefined if in doubt over os.
#ifdef _WIN32
#define NOENDIAN
#endif




//pr_multi.c

extern pvec3_t pvec3_origin;

struct qcthread_s *PDECL PR_ForkStack	(pubprogfuncs_t *progfuncs);
void PDECL PR_ResumeThread			(pubprogfuncs_t *progfuncs, struct qcthread_s *thread);
void	PDECL PR_AbortStack			(pubprogfuncs_t *progfuncs);
pbool	PDECL PR_GetBuiltinCallInfo	(pubprogfuncs_t *ppf, int *builtinnum, char *function, size_t sizeoffunction);

eval_t *PDECL PR_FindGlobal(pubprogfuncs_t *prfuncs, const char *globname, progsnum_t pnum, etype_t *type);
ddef16_t *ED_FindTypeGlobalFromProgs16 (progfuncs_t *progfuncs, progstate_t *ps, const char *name, int type);
ddef32_t *ED_FindTypeGlobalFromProgs32 (progfuncs_t *progfuncs, progstate_t *ps, const char *name, int type);
ddef16_t *ED_FindGlobalFromProgs16 (progfuncs_t *progfuncs, progstate_t *ps, const char *name);
ddef32_t *ED_FindGlobalFromProgs32 (progfuncs_t *progfuncs, progstate_t *ps, const char *name);
fdef_t *ED_FindField (progfuncs_t *progfuncs, const char *name);
fdef_t *ED_ClassFieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs, const char *classname);
fdef_t *ED_FieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs);
mfunction_t *ED_FindFunction (progfuncs_t *progfuncs, const char *name, progsnum_t *pnum, progsnum_t fromprogs);
func_t PDECL PR_FindFunc(pubprogfuncs_t *progfncs, const char *funcname, progsnum_t pnum);
//void PDECL PR_Configure (pubprogfuncs_t *progfncs, size_t addressable_size, int max_progs);
int PDECL PR_InitEnts(pubprogfuncs_t *progfncs, int maxents);
char *PR_ValueString (progfuncs_t *progfuncs, etype_t type, eval_t *val, pbool verbose);
void PDECL QC_ClearEdict (pubprogfuncs_t *progfuncs, struct edict_s *ed);
void PRAddressableFlush(progfuncs_t *progfuncs, size_t totalammount);
void QC_FlushProgsOffsets(progfuncs_t *progfuncs);

ddef16_t *ED_GlobalAtOfs16 (progfuncs_t *progfuncs, int ofs);
ddef16_t *ED_FindGlobal16 (progfuncs_t *progfuncs, const char *name);
ddef32_t *ED_FindGlobal32 (progfuncs_t *progfuncs, const char *name);
ddef32_t *ED_GlobalAtOfs32 (progfuncs_t *progfuncs, unsigned int ofs);

string_t PDECL PR_StringToProgs			(pubprogfuncs_t *inst, const char *str);
const char *ASMCALL PR_StringToNative				(pubprogfuncs_t *inst, string_t str);

char *PR_GlobalString (progfuncs_t *progfuncs, int ofs, struct QCC_type_s **typehint);
char *PR_GlobalStringNoContents (progfuncs_t *progfuncs, int ofs);
char *PR_GlobalStringImmediate (progfuncs_t *progfuncs, int ofs);

pbool CompileFile(progfuncs_t *progfuncs, const char *filename);

struct jitstate;
struct jitstate *PR_GenerateJit(progfuncs_t *progfuncs);
void PR_EnterJIT(progfuncs_t *progfuncs, struct jitstate *jitstate, int statement);
void PR_CloseJit(struct jitstate *jit);

char *QCC_COM_Parse (const char *data);
extern char	qcc_token[1024];
extern char *basictypenames[];
#endif

#endif
