
#ifndef PROGSLIB_H
#define PROGSLIB_H

#include "progtype.h"
#include <stdlib.h>
#ifdef _MSC_VER
	#define VARGS __cdecl
#endif
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#if defined(_WIN32)
		#include <stdio.h>
		#ifdef __MINGW_PRINTF_FORMAT
			#define LIKEPRINTF(x) __attribute__((format(__MINGW_PRINTF_FORMAT,x,x+1)))
		#else
			#define LIKEPRINTF(x) __attribute__((format(ms_printf,x,x+1)))
		#endif
	#else
		#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
	#endif
#endif
#ifndef LIKEPRINTF
	#define LIKEPRINTF(x)
#endif
#ifndef VARGS
	#define VARGS
#endif

#if __STDC_VERSION__ >= 202311L // c23
	#define FALLTHROUGH  [[fallthrough]];
#elif defined(__GNUC__) && __GNUC__ >= 7
	#define FALLTHROUGH __attribute__((fallthrough));
#elif defined(__clang__) && __clang_major__ >= 7
	#define FALLTHROUGH __attribute__((fallthrough));
#else
	#define FALLTHROUGH
#endif

#if defined(_M_IX86) || defined(__i386__)	//supported arch
	#if defined(__GNUC__) || defined(_MSC_VER)	//supported compilers (yay for inline asm)
	//#define QCJIT
	#endif
#endif

#define QCBUILTIN ASMCALL

#ifdef _WIN32
#define PDECL __cdecl
#else
#define PDECL
#endif

#ifdef QCJIT
#define ASMCALL VARGS
#else
#define ASMCALL PDECL
#endif


#define QCGC

struct edict_s;
struct entvars_s;
struct globalvars_s;
struct qcthread_s;
typedef struct pubprogfuncs_s pubprogfuncs_t;
typedef void (ASMCALL *builtin_t) (pubprogfuncs_t *prinst, struct globalvars_s *gvars);

enum ereftype_e
{
	ER_ENTITY,
	ER_FREE,
	ER_OBJECT	//custom sized, no vm/engine fields.
};
#define ED_ISFREE(e) ((e)->ereftype != ER_ENTITY)

//used by progs engine. All nulls is reset.
typedef struct {
	const char *varname;
	struct fdef_s *ofs32;

	int spare[2];
} evalc_t;
#define sizeofevalc sizeof(evalc_t)
typedef enum {
//vanilla types
ev_void,
ev_string,	//offset into the string table - but if the high bit is set then its probably some special thing.
ev_float,	//can hold up to 24 bits... sucks, but this is our basic numeric type.
ev_vector,	//3 floats.
ev_entity,	//index into the edicts array (vanilla used byte offsets from world).
ev_field,	//index into the per-entity field table.
ev_function,//all functions are called via reference.
ev_pointer,	//exists in vanilla - *(&ent.fld) opcodes are valid there - how else would you store to a field?
//extended types
ev_integer,	//our first extended type... probably won't help performance much but at least it doesn't have the imprecision issue of floats.
ev_uint,	//mostly just reuses int opcodes.
ev_int64,	//large int type, because we can. might be useful for system handles perhaps? dunno, probably not that useful.
ev_uint64,	//mostly just reuses int64 opcodes.
ev_double,	//useful for timers, for the extra precision.
//qc-only types
ev_variant,	//used primarily for builtin args, or for type punning casts without using pointers. should never be used for a global.
ev_struct,	//big complex type
ev_union,	//not really sure why this is separate from struct
ev_accessor,//some weird type to provide class-like functions over a basic type.
ev_enum,	//just a numeric type
ev_typedef,	//so typedefs can refer to their original type (primarily for structs).
ev_boolean,	//exists to optimise if(-0) workarounds. engine just sees int/float. uses parentclass
ev_bitfld,	//erk... structs only... converted to their parentclass on read.
} etype_t;
enum {
	DEBUG_TRACE_OFF,		//debugging should be off.
	DEBUG_TRACE_INTO,		//debug into functions
	DEBUG_TRACE_OVER,		//switch debugging off while executing child functions (and back on afterwards)
	DEBUG_TRACE_OUT,		//keep running until the end of the current function (trigger single-stepping again at that point)
	DEBUG_TRACE_ABORTERROR,	//give up with an endgame.
//	DEBUG_TRACE_ABORTSTACK,	//stop executing, without any errors.
	DEBUG_TRACE_NORESUME	//line number or something changed, but we should still be sitting at the debugger.
};

typedef struct fdef_s
{
	unsigned int	type;		//if DEF_SAVEGLOBAL bit is set then the variable needs to be saved in savegames
	int				ofs;		//runtime offset. add fieldadj to get the real array index.
	unsigned int	progsofs;	//used at loading time, so maching field offsets (unions/members) are positioned at the same runtime offset.
	const char *	name;		//proper name for the field.
} fdef_t;

//the number of pointers to variables (as opposed to functions - those are fine) in these structures is excessive.
//Many of the functions are also obsolete.
struct pubprogfuncs_s
{
	int progsversion;	//PROGSTRUCT_VERSION

	void	(PDECL *Shutdown)					(pubprogfuncs_t *inst);

	void	(PDECL *Configure)					(pubprogfuncs_t *prinst, size_t addressablesize, int max_progs, pbool enableprofiling);		//configure buffers and memory. Used to reset and must be called first. Flushes a running VM.
	progsnum_t	(PDECL *LoadProgs)				(pubprogfuncs_t *prinst, const char *s);	//load a progs
	int		(PDECL *InitEnts)					(pubprogfuncs_t *prinst, int max_ents);	//returns size of edicts for use with nextedict macro
	void	(PDECL *ExecuteProgram)				(pubprogfuncs_t *prinst, func_t fnum);	//start execution
	struct globalvars_s	*(PDECL *globals)		(pubprogfuncs_t *prinst, progsnum_t num);	//get the globals of a progs
	struct entvars_s	*(PDECL *entvars)		(pubprogfuncs_t *prinst, struct edict_s *ent);	//return a pointer to the entvars of an ent. can be achieved via the edict_t structure instead, so obsolete.

	void	(VARGS *RunError)					(pubprogfuncs_t *prinst, const char *msg, ...) LIKEPRINTF(2);		//builtins call this to say there was a problem
	void	(PDECL *PrintEdict)					(pubprogfuncs_t *prinst, struct edict_s *ed);	//get a listing of all vars on an edict (sent back via 'print')

	struct edict_s	*(PDECL *EntAlloc)			(pubprogfuncs_t *prinst, pbool object, size_t extrasize);			//allocate a random index.
	struct edict_s	*(PDECL *EntAllocIndex)		(pubprogfuncs_t *prinst, unsigned int idx, pbool object, size_t extrasize);	//allocate a specific index.
	void	(PDECL *EntFree)					(pubprogfuncs_t *prinst, struct edict_s *ed, pbool instant);

	struct edict_s	*(PDECL *EdictNum)			(pubprogfuncs_t *prinst, unsigned int n);		//get the nth edict
	unsigned int		(PDECL *NumForEdict)	(pubprogfuncs_t *prinst, struct edict_s *e);	//so you can find out what that 'n' will be

	char	*(PDECL *VarString)					(pubprogfuncs_t *prinst, int	first);	//returns a string made up of multiple arguments

	struct progstate_s **progstate;	//internal to the library.
	int		numprogs;

	func_t	(PDECL *FindFunction)				(pubprogfuncs_t *prinst, const char *funcname, progsnum_t num);

	int		(PDECL *StartCompile)				(pubprogfuncs_t *prinst, int argv, const char **argc);	//1 if can compile, 0 if failed to compile
	int		(PDECL *ContinueCompile)			(pubprogfuncs_t *prinst);	//2 if finished, 1 if more to go, 0 if failed

	char	*(PDECL *filefromprogs)				(pubprogfuncs_t *prinst, progsnum_t prnum, const char *fname, size_t *size, char *buffer);	//reveals encoded/added files from already loaded progs
	char	*(PDECL *filefromnewprogs)			(pubprogfuncs_t *prinst, const char *prname, const char *fname, size_t *size, char *buffer);	//reveals encoded/added files from a progs on the disk somewhere

	void	(PDECL *ED_Print)					(pubprogfuncs_t *prinst, struct edict_s *ed);
	char	*(PDECL *save_ents)					(pubprogfuncs_t *prinst, char *buf, size_t *size, size_t maxsize, int mode);	//dump the entire progs info into one big self allocated string
	int		(PDECL *load_ents)					(pubprogfuncs_t *prinst, const char *s, void *ctx,
														void (PDECL *memoryreset) (pubprogfuncs_t *progfuncs, void *ctx),
														void (PDECL *entspawned) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend),
														pbool(PDECL *extendedterm)(pubprogfuncs_t *progfuncs, void *ctx, const char **extline)
												); //restore the entire progs state (or just add some more ents) (returns edicts ize)

	char	*(PDECL *saveent)					(pubprogfuncs_t *prinst, char *buf, size_t *size, size_t maxsize, struct edict_s *ed);	//will save just one entities vars
	struct edict_s	*(PDECL *restoreent)		(pubprogfuncs_t *prinst, const char *buf, size_t *size, struct edict_s *ed);	//will restore the entity that had it's values saved (can use NULL for ed)

	union eval_s	*(PDECL *FindGlobal)		(pubprogfuncs_t *prinst, const char *name, progsnum_t num, etype_t *type);	//find a pointer to the globals value

	union eval_s	*(PDECL *GetEdictFieldValue)(pubprogfuncs_t *prinst, struct edict_s *ent, const char *name, etype_t type, evalc_t *s); //get an entityvar (cache it) and return the possible values
	struct edict_s	*(PDECL *ProgsToEdict)		(pubprogfuncs_t *prinst, int progs);	//edicts are stored as ints and need to be adjusted
	int		(PDECL *EdictToProgs)				(pubprogfuncs_t *prinst, struct edict_s *ed);		//edicts are stored as ints and need to be adjusted

	char	*(PDECL *EvaluateDebugString)		(pubprogfuncs_t *prinst, const char *key);	//evaluate a string and return it's value (according to current progs) (expands edict vars)
	int		debug_trace;	//start calling the editor for each line executed	
	void	(PDECL *StackTrace)					(pubprogfuncs_t *prinst, int showlocals);
	int		(PDECL *ToggleBreak)				(pubprogfuncs_t *prinst, const char *filename, int linenum, int mode);

	struct	progexterns_s *parms;	//these are the initial parms, they may be changed

	pbool	(PDECL *Decompile)					(pubprogfuncs_t *prinst, const char *fname);

	int		callargc;	//number of args of built-in call
	int		callprogs;	//which progs it was called from...

	char *stringtable;	//qc strings are all relative. add to a qc string. this is required for support of frikqcc progs that strip string immediates.
	unsigned int stringtablesize;
	unsigned int stringtablemaxsize;
	int fieldadjust;	//FrikQCC style arrays can cause problems due to field remapping. This causes us to leave gaps but offsets identical. except for system fields, qc-addressable variables use their old offsets, this is the bias so that the offset pokes the correct memory.
	unsigned int activefieldslots; //f+=fieldadjust; invalidfield = (f<0)||(f+fldsize>=activefieldslots); note that this does NOT apply to 'object' entities which are variable sized, use ed->fieldsize for those.


	struct qcthread_s *(PDECL *Fork)			(pubprogfuncs_t *prinst);	//returns a pointer to a thread which can be resumed via RunThread.
	void	(PDECL *RunThread)					(pubprogfuncs_t *prinst, struct qcthread_s *thread);
	void	(PDECL *AbortStack)					(pubprogfuncs_t *prinst);	//annigilates the current stack, positioning on a return statement. It is expected that this is only used via a builtin!

	pbool	(PDECL *GetBuiltinCallInfo)			(pubprogfuncs_t *prinst, int *builtinnum, char *function, size_t sizeoffunction);	//call to query the qc's name+index for the builtin
	pbool	(PDECL *FindBuiltins)				(pubprogfuncs_t *progfuncs, progsnum_t prnum, int binum, pbool (PDECL *found) (pubprogfuncs_t *progfuncs, const char *name, void *ctx), void *ctx);	//calls the callback for each function reference that's mapped to the specified builtin number.

	int (PDECL *RegisterFieldVar)				(pubprogfuncs_t *prinst, unsigned int type, const char *name, signed long requestedpos, signed long originalofs);

	char	*(PDECL *AddString)					(pubprogfuncs_t *prinst, const char *val, int minlength, pbool demarkup);	//dump a string into the progs memory (for setting globals and whatnot)
	void	*(PDECL *Tempmem)					(pubprogfuncs_t *prinst, int ammount, char *whatfor);	//grab some mem for as long as the progs stays loaded
	void *(PDECL *AddressableAlloc)				(pubprogfuncs_t *progfuncs, unsigned int ammount); /*returns memory within the qc block, use stringtoprogs to get a usable qc pointer/string*/
	void *(PDECL *AddressableRealloc)			(pubprogfuncs_t *progfuncs, void *oldptr, unsigned int newammount); /*returns memory within the qc block, use stringtoprogs to get a usable qc pointer/string*/
	void (PDECL *AddressableFree)				(pubprogfuncs_t *progfuncs, void *mem); /*frees a block of addressable memory*/
	string_t (PDECL *TempString)				(pubprogfuncs_t *prinst, const char *str);
	string_t (PDECL *AllocTempString)			(pubprogfuncs_t *prinst, char **str, unsigned int len);
	string_t (PDECL *StringToProgs)				(pubprogfuncs_t *prinst, const char *str);	//commonly makes a semi-permanent mapping from some table to the string value. mapping can be removed via RemoveProgsString
	const char *(ASMCALL *StringToNative)		(pubprogfuncs_t *prinst, string_t str);

	int (PDECL *QueryField)						(pubprogfuncs_t *prinst, unsigned int fieldoffset, etype_t *type, char const**name, evalc_t *fieldcache);	//find info on a field definition at an offset

	void (PDECL *EntClear)						(pubprogfuncs_t *progfuncs, struct edict_s *e);
	void (PDECL *FindPrefixGlobals)				(pubprogfuncs_t *progfuncs, int prnum, char *prefix, void (PDECL *found) (pubprogfuncs_t *progfuncs, char *name, union eval_s *val, etype_t type, void *ctx), void *ctx);	//calls the callback for each named global found

	pbool (PDECL *SetWatchPoint)				(pubprogfuncs_t *prinst, const char *desc, const char *location);

	void (PDECL *AddSharedVar)					(pubprogfuncs_t *progfuncs, int start, int size);
	void (PDECL *AddSharedFieldVar)				(pubprogfuncs_t *progfuncs, int num, char *relstringtable);
	char *(PDECL *RemoveProgsString)			(pubprogfuncs_t *progfuncs, string_t str);
	pbool (PDECL *GetFunctionInfo)				(pubprogfuncs_t *progfuncs, func_t func, int *argcount, unsigned char **argsizes, int *builtinnum, char *funcname, size_t funcnamesize); //queries the interesting info from a function def
	void (PDECL *GenerateStatementString)		(pubprogfuncs_t *progfuncs, int statementnum, char *out, int outlen);	//disassembles a specific statement. for debugging reports.
	fdef_t *(PDECL *FieldInfo)					(pubprogfuncs_t *progfuncs, unsigned int *count);
	char *(PDECL *UglyValueString)				(pubprogfuncs_t *progfuncs, etype_t type, union eval_s *val);
	pbool (PDECL *ParseEval)					(pubprogfuncs_t *progfuncs, union eval_s *eval, int type, const char *s);
	void (PDECL *SetStringField)				(pubprogfuncs_t *progfuncs, struct edict_s *ed, string_t *fld, const char *str, pbool str_is_static);	//if ed is null, fld points to a global. if str_is_static, then s doesn't need its own memory allocated.
	pbool (PDECL *DumpProfile)					(pubprogfuncs_t *progfuncs, pbool resetprofiles);

	unsigned int edicttable_length;
	struct edict_s **edicttable;

	//stuff not used by the qclib at all, but provided for lazy user storage.
	struct
	{
		char	*tempstringbase;					//for engine's use. Store your base tempstring pointer here.
		int		tempstringnum;						//for engine's use.
	} user;
};

typedef struct progexterns_s {
	int progsversion;	//PROGSTRUCT_VERSION

	void *(PDECL *ReadFile)				(const char *fname, unsigned char *(PDECL *buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile);
	int (PDECL *FileSize)				(const char *fname);	//-1 if file does not exist
	pbool (PDECL *WriteFile)			(const char *name, void *data, int len);
	int (VARGS *Printf)					(const char *, ...) LIKEPRINTF(1);
	int (VARGS *DPrintf)				(const char *, ...) LIKEPRINTF(1);
	void (VARGS *Sys_Error)				(const char *, ...) LIKEPRINTF(1);
	void (VARGS *Abort)					(const char *, ...) LIKEPRINTF(1);
	pbool (PDECL *CheckHeaderCrc)		(pubprogfuncs_t *inst, progsnum_t idx, int crc, const char *filename);

	void (PDECL *entspawn)				(struct edict_s *ent, int loading);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	pbool (PDECL *entcanfree)			(struct edict_s *ent);	//return true to stop ent from being freed
	void (ASMCALL *stateop)				(pubprogfuncs_t *prinst, float var, func_t func);	//what to do on qc's state opcode.
	void (ASMCALL *cstateop)			(pubprogfuncs_t *prinst, float vara, float varb, func_t currentfunc);		//a hexen2 opcode.
	void (ASMCALL *cwstateop)			(pubprogfuncs_t *prinst, float vara, float varb, func_t currentfunc);	//a hexen2 opcode.
	void (ASMCALL *thinktimeop)			(pubprogfuncs_t *prinst, struct edict_s *ent, float varb);			//a hexen2 opcode.


	//used when loading a game
	int (PDECL *MapNamedBuiltin)		(pubprogfuncs_t *prinst, int headercrc, const char *builtinname);	//return 0 for not found.
	void (PDECL *loadcompleate)			(int edictsize);	//notification to reset any pointers.
	pbool (PDECL *badfield)				(pubprogfuncs_t *prinst, struct edict_s *ent, const char *keyname, const char *value);	//called for any fields that are not registered

	void *(VARGS *memalloc)				(int size);	//small string allocation	malloced and freed randomly by the executor. (use malloc if you want)
	void (VARGS *memfree)				(void * mem);

	int (PDECL *useeditor)				(pubprogfuncs_t *prinst, const char *filename, int *line, int *statement, int funcstart, char *reason, pbool fatal);	//called on syntax errors or step-by-step debugging. line and statement(if line was set to 0) can be used to change the next line. return value is the new debug state to use/step.
	void (PDECL *addressablerelocated)	(pubprogfuncs_t *progfuncs, char *oldb, char *newb, int oldlen);	//called when the progs memory was resized. you must fix up all pointers to globals, strings, fields, addressable blocks.

	builtin_t *globalbuiltins;	//these are available to all progs
	int numglobalbuiltins;

	enum {PR_NOCOMPILE, PR_COMPILENEXIST, PR_COMPILEEXISTANDCHANGED, PR_COMPILECHANGED, PR_COMPILEALWAYS, PR_COMPILEIGNORE} autocompile;

	double *gametime;	//used to prevent the vm from reusing an entity faster than 2 secs.

	pbool usethreadedgc;

	struct edict_s **edicts;	//pointer to the engine's reference to world.
	unsigned int *num_edicts;		//pointer to the engine's edict count.
	int edictsize;	//size of edict_t

	void *user;	/*contains the owner's world reference in FTE*/
} progparms_t, progexterns_t;

#if defined(QCLIBDLL_EXPORTS)
	#ifdef _WIN32
		__declspec(dllexport)
	#else
		__attribute__((visibility("default")))
	#endif
#endif
pubprogfuncs_t * PDECL InitProgs(progparms_t *ext);

typedef union eval_s
{	//FIXME: we should not be using a leading underscore here. that is reserved for libc.
	string_t		string;
	pvec_t			_float;
	pvec_t			_vector[3];
	func_t			function;	//module=0xff000000, func=0x00ffffff
	pint_t			_int;
	puint_t			_uint;
	pint64_t		i64;
	puint64_t		u64;
	pdouble_t		_double;
	pint_t			edict;
	pvec_t			prog;	//so it can easily be changed
} eval_t;

#define PR_CURRENT	-1
#define PR_ANY	-2	//not always valid. Use for finding funcs
#define PR_ANYBACK -3
#define PROGSTRUCT_VERSION 4


#ifndef DLL_PROG
#define PR_Configure(pf, memsize, max_progs, profiling)		(*pf->Configure)			(pf, memsize, max_progs, profiling)
#define PR_LoadProgs(pf, s)									(*pf->LoadProgs)			(pf, s)
#define PR_InitEnts(pf, maxents)							(*pf->InitEnts)				(pf, maxents)
#define PR_ExecuteProgram(pf, fnum)							(*pf->ExecuteProgram)		(pf, fnum)
#define PR_globals(pf, num)									(*pf->globals)				(pf, num)
#define PR_entvars(pf, ent)									(*pf->entvars)				(pf, ent)

#define PR_RegisterFieldVar(pf,type,name,reqofs,qcofs)		(*pf->RegisterFieldVar)		(pf,type,name,reqofs,qcofs)

#define ED_Alloc(pf,isobj,extsize)							(*pf->EntAlloc)				(pf, isobj, extsize)
#define ED_Free(pf, ed)										(*pf->EntFree)				(pf, ed, false)
#define ED_Clear(pf, ed)									(*pf->EntClear)				(pf, ed)

#define PR_LoadEnts(pf, s, ctx, memreset, entcb, extcb)		(*pf->load_ents)			(pf, s, ctx, memreset, entcb, extcb)
#define PR_SaveEnts(pf, buf, size, maxsize, mode)			(*pf->save_ents)			(pf, buf, size, maxsize, mode)

#if 0//def _DEBUG
#define EDICT_NUM(pf, num)									(*pf->EDICT_NUM)			(pf, num)
#else
#define EDICT_NUM_PB(pf, num)								(pf->edicttable[num])
#define EDICT_NUM_UB(pf, num)								EDICT_NUM_PB(pf,(((unsigned int)(num))>=pf->edicttable_length)?0:num)
#endif
#define NUM_FOR_EDICT(pf, e)								(*pf->NumForEdict)			(pf, (struct edict_s*)(e))
#define SetGlobalEdict(pf, ed, ofs)							(*pf->SetGlobalEdict)		(pf, ed, ofs)
#define PR_VarString(pf,first)								(*pf->VarString)			(pf,first)

#define PR_StartCompile(pf,argc,argv)						(*pf->StartCompile)			(pf,argc,argv)
#define PR_ContinueCompile(pf)								(*pf->ContinueCompile)		(pf)

#define PR_StackTrace(pf,locals)							(*pf->StackTrace)			(pf,locals)
#define PR_AbortStack(pf)									(*pf->AbortStack)			(pf)

#define PR_RunError(pf,str)									(*pf->RunError)				(pf,str)

#define PR_PrintEdict(pf,ed)								(*pf->PrintEdict)			(pf, ed)

#define PR_FindFunction(pf, name, num)						(*pf->FindFunction)			(pf, name, num)
#define PR_FindGlobal(pf, name, progs, type)				(*pf->FindGlobal)			(pf, name, progs, type)
#define PR_AddString(pf, ed, len, demarkup)					(*pf->AddString)			(pf, ed, len, demarkup)
#define PR_Alloc(pf,size,whatfor)							(*pf->Tempmem)				(pf, size, whatfor)
#define PR_AddressableAlloc(pf,size)						(*pf->AddressableAlloc)		(pf, size)
#define PR_AddressableFree(pf,mem)							(*pf->AddressableFree)		(pf, mem)

#define PROG_TO_EDICTINDEX(pf, ed)							ed
#define PROG_TO_EDICT(pf, ed)								(*pf->ProgsToEdict)			(pf, ed)
#define EDICT_TO_PROG(pf, ed)								(*pf->EdictToProgs)			(pf, (struct edict_s*)ed)

#define PR_GetString(pf,s)									(*pf->StringToNative)		(pf, s)
#define PR_GetStringOfs(pf,o)								(*pf->StringToNative)		(pf, G_INT(o))
#define PR_SetString(pf, s)									(*pf->StringToProgs)		(pf, s)

#define NEXT_EDICT(pf,o)		EDICT_NUM(pf, NUM_FOR_EDICT(pf, o)+1)
#define	RETURN_EDICT(pf, e) (((pint_t *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(pf, e))


//builtin funcs (which operate on globals)
//To use these outside of builtins, you will likly have to use the 'globals' method.
#define	G_FLOAT(o) (((pvec_t *)pr_globals)[o])
#define	G_FLOAT2(o) (((pvec_t *)pr_globals)[OFS_PARM0 + o*3])
#define	G_DOUBLE(o) (*(pdouble_t *)(((pvec_t *)pr_globals+(o))))
#define	G_INT(o) (((pint_t *)pr_globals)[o])
#define	G_UINT(o) (((puint_t *)pr_globals)[o])
#define	G_INT64(o) (*(pint64_t *)((pint_t *)pr_globals+(o)))
#define	G_UINT64(o) (*(puint64_t *)((puint_t *)pr_globals+(o)))
#define	G_EDICT(pf, o) PROG_TO_EDICT(pf, G_INT(o)) //((edict_t *)((char *) sv.edicts+ *(int *)&((float *)pr_globals)[o]))
#define G_EDICTNUM(pf, o) NUM_FOR_EDICT(pf, G_EDICT(pf, o))
#define	G_VECTOR(o) (&((pvec_t *)pr_globals)[o])
#define	G_FUNCTION(o) (*(func_t *)&((pvec_t *)pr_globals)[o])

/*
#define PR_GetString(p,s) (s?s + p->stringtable:"")
#define PR_GetStringOfs(p,o) (G_INT(o)?G_INT(o) + p->stringtable:"")
#define PR_SetStringOfs(p,o,s) (G_INT(o) = s - p->stringtable)
*/
//#define PR_SetString(p, s) ((s&&*s)?(s - p->stringtable):0)
/**/

#ifdef QCGC
#define PR_NewString(p, s) (p)->TempString(p, s)
#else
#define PR_NewString(p, s) PR_SetString(p, PR_AddString(p, s, 0, false))
#endif

#define ev_prog ev_integer

#define E_STRING(o) (char *)(((pint_t *)((char *)ed) + progparms.edictsize)[o])

//#define pr_global_struct pr_globals

#endif


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


#undef edict_t
#undef globalvars_t

#endif //PROGSLIB_H
