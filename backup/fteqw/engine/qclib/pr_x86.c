/*
when I say JIT, I mean load time, not execution time.

notes:
	qc jump offsets are all constants. we have no variable offset jumps (other than function calls/returns)
	field remapping... fields are in place, and cannot be adjusted. if a field is not set to 0, its assumed to be a constant.

optimisations:
	none at the moment...
	instructions need to be chained. stuff that writes to C should be cacheable, etc. maybe we don't even need to do the write to C
	it should also be possible to fold in eq+ifnot, so none of this silly storeing of floats in equality tests

	this means that we need to track which vars are cached and in what form: fpreg, ireg+floatasint, ireg+float.
	certain qccx hacks can use fpu operations on ints, so do what the instruction says, rather than considering an add an add regardless of types.

	OP_AND_F, OP_OR_F etc will generally result in ints, and we should be able to keep them as ints if they combine with other ints.

	some instructions are jump sites. any cache must be flushed before the start of the instruction.
	some variables are locals, and will only ever be written by a single instruction, then read by the following instruction. such temps do not need to be written, or are overwritten later in the function anyway.
	such locals need to be calculated PER FUNCTION as (fte)qcc can overlap locals making multiple distinct locals on a single offset.

	store locals on a proper stack instead of the current absurd mechanism.

	eax - tmp
	ebx - prinst->edicttable
	ecx	- tmp
	edx - tmp
	esi - debug opcode number
	edi - tmp (because its preserved by subfunctions
	ebp -

  to use gas to provide binary opcodes:
  vim -N blob.s && as blob.s && objdump.exe -d a.out


  notable mods to test:
  prydon gate, due to fpu mangling to carry values between maps
*/

#define PROGSUSED
#include "progsint.h"

#ifdef QCJIT

#ifndef _WIN32
#include <sys/mman.h>
#endif

static float ta, tb, nullfloat=0;

struct jitstate
{
	unsigned int *statementjumps;	//[MAX_STATEMENTS*3]
	unsigned char **statementoffsets; //[MAX_STATEMENTS]
	unsigned int numjumps;
	unsigned char *code;
	unsigned int codesize;
	unsigned int jitstatements;

	float *glob;
	unsigned int cachedglobal;
	unsigned int cachereg;
};

static void Jit_EmitByte(struct jitstate *jit, unsigned char byte)
{
	jit->code[jit->codesize++] = byte;
}
static void Jit_Emit4Byte(struct jitstate *jit, unsigned int value)
{
	jit->code[jit->codesize++] = (value>> 0)&0xff;
	jit->code[jit->codesize++] = (value>> 8)&0xff;
	jit->code[jit->codesize++] = (value>>16)&0xff;
	jit->code[jit->codesize++] = (value>>24)&0xff;
}
static void Jit_EmitAdr(struct jitstate *jit, void *value)
{
	Jit_Emit4Byte(jit, (unsigned int)value);
}
static void Jit_EmitFloat(struct jitstate *jit, float value)
{
	union {float f; unsigned int i;} u;
	u.f = value;
	Jit_Emit4Byte(jit, u.i);
}
static void Jit_Emit2Byte(struct jitstate *jit, unsigned short value)
{
	jit->code[jit->codesize++] = (value>> 0)&0xff;
	jit->code[jit->codesize++] = (value>> 8)&0xff;
}

static void Jit_EmitFOffset(struct jitstate *jit, const void *func, int bias)
{
	union {const void *f; unsigned int i;} u;
	u.f = func;
	u.i -= (unsigned int)&jit->code[jit->codesize+bias];
	Jit_Emit4Byte(jit, u.i);
}

static void Jit_Emit4ByteJump(struct jitstate *jit, int statementnum, int offset)
{
	jit->statementjumps[jit->numjumps++] = jit->codesize;
	jit->statementjumps[jit->numjumps++] = statementnum;
	jit->statementjumps[jit->numjumps++] = offset;

	//the offset is filled in later
	jit->codesize += 4;
}

#ifdef _WIN32
#undef REG_NONE
#endif

enum
{
	REG_EAX,
	REG_ECX,
	REG_EDX,
	REG_EBX,	//note: edicttable
	REG_ESP,
	REG_EBP,
	REG_ESI,
	REG_EDI,

	/*I'm not going to list S1 here, as that makes things too awkward*/
	REG_S0,
	REG_NONE
};
#define XOR(sr,dr) EmitByte(0x31);EmitByte(0xc0 | (sr<<3) | dr);
#define CLEARREG(reg) XOR(reg,reg)
#define LOADREG(addr, reg) if (reg == REG_EAX) {EmitByte(0xa1);} else {EmitByte(0x8b); EmitByte((reg<<3) | 0x05);} EmitAdr(addr);
#define STOREREG(reg, addr) if (reg == REG_EAX) {EmitByte(0xa3);} else {EmitByte(0x89); EmitByte((reg<<3) | 0x05);} EmitAdr(addr);
#define STOREF(f, addr) EmitByte(0xc7);EmitByte(0x05); EmitAdr(addr);EmitFloat(f);
#define STOREI(i, addr) EmitByte(0xc7);EmitByte(0x05); EmitAdr(addr);Emit4Byte(i);
#define SETREGI(val,reg) EmitByte(0xbe);Emit4Byte(val);

#define ARGREGS(a,b,c)	GCache_Load(jit, op[i].a, a, op[i].b, b, op[i].c, c)
#define RESULTREG(r) GCache_Store(jit, op[i].c, r)


#define EmitByte(v) Jit_EmitByte(jit, v)
#define EmitAdr(v) Jit_EmitAdr(jit, v)
#define EmitFOffset(a,b) Jit_EmitFOffset(jit, a, b)
#define Emit4ByteJump(a,b) Jit_Emit4ByteJump(jit, a, b)
#define Emit4Byte(v) Jit_Emit4Byte(jit, v)
#define EmitFloat(v) Jit_EmitFloat(jit, v)

#define LocalJmp(v) Jit_LocalJmp(jit, v)
#define LocalLoc() Jit_LocalLoc(jit)

//for the purposes of the cache, 'temp' offsets are only read when they have been written only within the preceeding control block.
//if they were read at any other time, then we must write them out in full.
//this logic applies only to locals of a function.
//#define USECACHE

static void GCache_Load(struct jitstate *jit, int ao, int ar, int bo, int br, int co, int cr)
{
#if USECACHE
	if (jit->cachedreg != REG_NONE)
	{
		/*something is cached, if its one of the input offsets then can chain the instruction*/

		if (jit->cachedglobal === ao && ar != REG_NONE)
		{
			if (jit->cachedreg == ar)
				ar = REG_NONE;
		}
		if (jit->cachedglobal === bo && br != REG_NONE)
		{
			if (jit->cachedreg == br)
				br = REG_NONE;
		}
		if (jit->cachedglobal === co && cr != REG_NONE)
		{
			if (jit->cachedreg == cr)
				cr = REG_NONE;
		}

		if (!istemp(ao))
		{
			/*purge the old cache*/
			switch(jit->cachedreg)
			{
			case REG_NONE:
				break;
			case REG_S0:
				//fstps glob[C]
				EmitByte(0xd9);EmitByte(0x1d);EmitAdr(jit->glob + jit->cachedglobal);
				break;
			default:
				STOREREG(jit->cachedreg, jit->glob + jit->cachedglobal);
				break;
		}
		jit->cachedglobal = -1;
		jit->cachedreg = REG_NONE;
	}

#endif
	switch(ar)
	{
	case REG_NONE:
		break;
	case REG_S0:
		//flds glob[A]
		EmitByte(0xd9);EmitByte(0x05);EmitAdr(jit->glob + ao);
		break;
	default:
		LOADREG(jit->glob + ao, ar);
		break;
	}

	switch(br)
	{
	case REG_NONE:
		break;
	case REG_S0:
		//flds glob[A]
		EmitByte(0xd9);EmitByte(0x05);EmitAdr(jit->glob + bo);
		break;
	default:
		LOADREG(jit->glob + bo, br);
		break;
	}

	switch(cr)
	{
	case REG_NONE:
		break;
	case REG_S0:
		//flds glob[A]
		EmitByte(0xd9);EmitByte(0x05);EmitAdr(jit->glob + co);
		break;
	default:
		LOADREG(jit->glob + co, cr);
		break;
	}
}
static void GCache_Store(struct jitstate *jit, int ofs, int reg)
{
#if USECACHE
	jit->cachedglobal = ofs;
	jit->cachedreg = reg;
#else
	switch(reg)
	{
	case REG_NONE:
		break;
	case REG_S0:
		//fstps glob[C]
		EmitByte(0xd9);EmitByte(0x1d);EmitAdr(jit->glob + ofs);
		break;
	default:
		STOREREG(reg, jit->glob + ofs);
		break;
	}
#endif
}

static void *Jit_LocalLoc(struct jitstate *jit)
{
	return &jit->code[jit->codesize];
}
static void *Jit_LocalJmp(struct jitstate *jit, int cond)
{
	/*floating point ops don't set the sign flag, thus we use the 'above/below' instructions instead of 'greater/less' instructions*/
	if (cond == OP_GOTO)
		Jit_EmitByte(jit, 0xeb);	//jmp
	else if (cond == OP_LE_F)
		Jit_EmitByte(jit, 0x76);	//jbe
	else if (cond == OP_GE_F)
		Jit_EmitByte(jit, 0x73);	//jae
	else if (cond == OP_LT_F)
		Jit_EmitByte(jit, 0x72);	//jb
	else if (cond == OP_GT_F)
		Jit_EmitByte(jit, 0x77);	//ja
	else if (cond == OP_LE_I)
		Jit_EmitByte(jit, 0x7e);	//jle
	else if (cond == OP_LT_I)
		Jit_EmitByte(jit, 0x7c);	//jl
	else if ((cond >= OP_NE_F && cond <= OP_NE_FNC) || cond == OP_NE_I)
		Jit_EmitByte(jit, 0x75);	//jne
	else if ((cond >= OP_EQ_F && cond <= OP_EQ_FNC) || cond == OP_EQ_I)
		Jit_EmitByte(jit, 0x74);	//je
#if defined(DEBUG) && defined(_WIN32)
	else
	{
		OutputDebugString("oh noes!\n");
		return NULL;
	}
#endif

	Jit_EmitByte(jit, 0);

	return Jit_LocalLoc(jit);
}
static void LocalJmpLoc(void *jmp, void *loc)
{
	int offs;
	unsigned char *a = jmp;
	offs = (char *)loc - (char *)jmp;
#if defined(DEBUG) && defined(_WIN32)
	if (offs > 127 || offs <= -128)
	{
		OutputDebugStringA("bad jump\n");
		a[-2] = 0xcd;
		a[-1] = 0xcc;
		return;
	}
#endif
	a[-1] = offs;
}

static void FixupJumps(struct jitstate *jit)
{
	unsigned int j;
	unsigned char *codesrc;
	unsigned char *codedst;
	unsigned int offset;

	unsigned int v;

	for (j = 0; j < jit->numjumps;)
	{
		v = jit->statementjumps[j++];
		codesrc = &jit->code[v];

		v = jit->statementjumps[j++];
		codedst = jit->statementoffsets[v];

		v = jit->statementjumps[j++];
		offset = (int)(codedst - (codesrc-v));	//3rd term because the jump is relative to the instruction start, not the instruction's offset

		codesrc[0] = (offset>> 0)&0xff;
		codesrc[1] = (offset>> 8)&0xff;
		codesrc[2] = (offset>>16)&0xff;
		codesrc[3] = (offset>>24)&0xff;
	}
}

int ASMCALL PR_LeaveFunction (progfuncs_t *progfuncs);
int ASMCALL PR_EnterFunction (progfuncs_t *progfuncs, dfunction_t *f, int progsnum);

void PR_CloseJit(struct jitstate *jit)
{
	if (jit)
	{
		free(jit->statementjumps);
		free(jit->statementoffsets);
#ifndef _WIN32
		munmap(jit->code, jit->jitstatements * 500);
#else
		free(jit->code);
#endif
		free(jit);
	}
}

#if 0
//called from jit code
static PDECL PR_CallFuncion(progfuncs_t *progfuncs, int fnum)
{
	int callerprogs;
	int newpr;
	unsigned int fnum;

	fnum = OPA->function;

	glob = NULL;	//try to derestrict it.

	callerprogs=prinst.pr_typecurrent;			//so we can revert to the right caller.
	newpr = (fnum & 0xff000000)>>24;	//this is the progs index of the callee
	fnum &= ~0xff000000;				//the callee's function index.

	//if it's an external call, switch now (before any function pointers are used)
	if (callerprogs != newpr || !fnum || fnum > pr_progs->numfunctions)
	{
		char *msg = fnum?"OP_CALL references invalid function in %s\n":"NULL function from qc (inside %s).\n";
		PR_SwitchProgsParms(progfuncs, callerprogs);

		glob = pr_globals;
		if (!progfuncs->funcs.debug_trace)
			QCFAULT(&progfuncs->funcs, msg, PR_StringToNative(&progfuncs->funcs, pr_xfunction->s_name));

		//skip the instruction if they just try stepping over it anyway.
		PR_StackTrace(&progfuncs->funcs, 0);
		printf(msg, PR_StringToNative(&progfuncs->funcs, pr_xfunction->s_name));

		pr_globals[OFS_RETURN] = 0;
		pr_globals[OFS_RETURN+1] = 0;
		pr_globals[OFS_RETURN+2] = 0;
		break;
	}

	newf = &pr_cp_functions[fnum & ~0xff000000];

	if (newf->first_statement <= 0)
	{	// negative statements are built in functions
		/*calling a builtin in another progs may affect that other progs' globals instead, is the theory anyway, so args and stuff need to move over*/
		if (prinst.pr_typecurrent != 0)
		{
			//builtins quite hackily refer to only a single global.
			//for builtins to affect the globals of other progs, we need to first switch to the progs that it will affect, so they'll be correct when we switch back
			PR_SwitchProgsParms(progfuncs, 0);
		}
		i = -newf->first_statement;
//			p = pr_typecurrent;
		if (i < externs->numglobalbuiltins)
		{
#ifndef QCGC
			prinst.numtempstringsstack = prinst.numtempstrings;
#endif
			(*externs->globalbuiltins[i]) (&progfuncs->funcs, (struct globalvars_s *)current_progstate->globals);

			//in case ed_alloc was called
			num_edicts = sv_num_edicts;

			if (prinst.continuestatement!=-1)
			{
				st=&pr_statements[prinst.continuestatement];
				prinst.continuestatement=-1;
				glob = pr_globals;
				break;
			}
		}
		else
		{
//					if (newf->first_statement == -0x7fffffff)
//						((builtin_t)newf->profile) (progfuncs, (struct globalvars_s *)current_progstate->globals);
//					else
				PR_RunError (&progfuncs->funcs, "Bad builtin call number - %i", -newf->first_statement);
		}
//			memcpy(&pr_progstate[p].globals[OFS_RETURN], &current_progstate->globals[OFS_RETURN], sizeof(vec3_t));
		PR_SwitchProgsParms(progfuncs, (progsnum_t)callerprogs);

		//decide weather non debugger wants to start debugging.
		s = st-pr_statements;
		return s;
	}
//		PR_SwitchProgsParms((OPA->function & 0xff000000)>>24);
	s = PR_EnterFunction (progfuncs, newf, callerprogs);
	st = &pr_statements[s];
}
#endif

struct jitstate *PR_GenerateJit(progfuncs_t *progfuncs)
{
	struct jitstate *jit;

	void *j0, *l0;
	void *j1, *l1;
	void *j2, *l2;
	unsigned int i;
	dstatement16_t *op = (dstatement16_t*)current_progstate->statements;
	unsigned int numstatements = current_progstate->progs->numstatements;
	unsigned int numglobals = current_progstate->progs->numglobals+3;	//vectors are annoying.
	int *glob = (int*)current_progstate->globals;
	unsigned int numfunctions = current_progstate->progs->numfunctions;
	mfunction_t *func;
//	pbyte *isconst;
	pbool failed = false;

	jit = malloc(sizeof(*jit));
	jit->jitstatements = numstatements;

//	isconst = malloc(numglobals*sizeof(*isconst));
	jit->statementjumps = malloc(numstatements*3*sizeof(int));
	jit->statementoffsets = malloc(numstatements*sizeof(*jit->statementoffsets));
#ifndef _WIN32
	jit->code = mmap(NULL, numstatements*500, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else
	jit->code = malloc(numstatements*500);
#endif
	if (!jit->code)
		return NULL;

	jit->numjumps = 0;
	jit->codesize = 0;

	for (i = 0; i < numstatements; i++)
		jit->statementoffsets[i] = NULL;
//	for (i = 0; i < numglobals; i++)
//		isconst[i] = true;
	for (i = 0; i < numfunctions; i++)
	{
		
	}
	for (i = 0; i < numstatements; i++)
	{
		//figure out which statements are jumped to. these are statements that must flush registers prior to execution.
		switch(op[i].op)
		{
		case OP_GOTO:
			jit->statementoffsets[i + (short)op[i].a] = (void*)~0;
			break;
		case OP_IF_I:
		case OP_IFNOT_I:
		case OP_IF_F:
		case OP_IFNOT_F:
		case OP_IF_S:
		case OP_IFNOT_S:
		case OP_CASE:
			jit->statementoffsets[i + (short)op[i].b] = (void*)~0;
			break;
		case OP_CASERANGE:
			jit->statementoffsets[i + (short)op[i].c] = (void*)~0;
			break;
		}
		//we probably can't do anything about consts.
		//we might be able to do something about locals, but we would need to fix this to generate per-function.
		//we CAN do something about consts, most of them anyway.
		//visible types 
/*
		if (OpAssignsToA(op[i].op))
		{
			if (op[i].a >= numglobals)
				failed = true;
			else
				isconst[op[i].a] = false;
		}
		if (OpAssignsToB(op[i].op))
		{
			if (op[i].b >= numglobals)
				failed = true;
			else
				isconst[op[i].b] = false;
		}
		if (OpAssignsToC(op[i].op))
		{
			if (op[i].c >= numglobals)
				failed = true;
			else
				isconst[op[i].c] = false;
		}
*/
	}

	for (i = 0; i < numstatements && !failed; i++)
	{
		if (jit->statementoffsets[i])
		{
			//FIXME: flush any registers.
		}
		jit->statementoffsets[i] = &jit->code[jit->codesize];

#ifdef _DEBUG
		/*DEBUG*/
		SETREGI(op[i].op, REG_ESI);
#endif

		switch(op[i].op)
		{
		//jumps
		case OP_IF_I:
			//integer compare
			//if a, goto b

			//cmpl $0,glob[A]
			EmitByte(0x83);EmitByte(0x3d);EmitAdr(glob + op[i].a);EmitByte(0x0);
			//jne B
			EmitByte(0x0f);EmitByte(0x85);Emit4ByteJump(i + (signed short)op[i].b, -4);
			break;

		case OP_IFNOT_I:
			//integer compare
			//if !a, goto b

			//cmpl $0,glob[A]
			EmitByte(0x83);EmitByte(0x3d);EmitAdr(glob + op[i].a);EmitByte(0x0);
			//je B
			EmitByte(0x0f);EmitByte(0x84);Emit4ByteJump(i + (signed short)op[i].b, -4);
			break;

		case OP_GOTO:
			EmitByte(0xE9);Emit4ByteJump(i + (signed short)op[i].a, -4);
			break;
			
		//function returns
		case OP_DONE:
		case OP_RETURN:
			//done and return are the same

			//part 1: store A into OFS_RETURN

			if (!op[i].a)
			{
				//assumption: anything that returns address 0 is a void or zero return.
				//thus clear eax and copy that to the return vector.
				CLEARREG(REG_EAX);
				STOREREG(REG_EAX, glob + OFS_RETURN+0);
				STOREREG(REG_EAX, glob + OFS_RETURN+1);
				STOREREG(REG_EAX, glob + OFS_RETURN+2);
			}
			else
			{
				LOADREG(glob + op[i].a+0, REG_EAX);
				LOADREG(glob + op[i].a+1, REG_EDX);
				LOADREG(glob + op[i].a+2, REG_ECX);
				STOREREG(REG_EAX, glob + OFS_RETURN+0);
				STOREREG(REG_EDX, glob + OFS_RETURN+1);
				STOREREG(REG_ECX, glob + OFS_RETURN+2);
			}
			
			//call leavefunction to get the return address
			
//			pushl progfuncs
			EmitByte(0x68);EmitAdr(progfuncs);
//			call PR_LeaveFunction
			EmitByte(0xe8);EmitFOffset(PR_LeaveFunction, 4);
//			add $4,%esp
			EmitByte(0x83);EmitByte(0xc4);EmitByte(0x04);
//			movl pr_depth,%edx
			EmitByte(0x8b);EmitByte(0x15);EmitAdr(&pr_depth);
//			cmp prinst->exitdepth,%edx
			EmitByte(0x3b);EmitByte(0x15);EmitAdr(&prinst.exitdepth);
//			je returntoc
			j1 = LocalJmp(OP_EQ_E);
//				mov statementoffsets[%eax*4],%eax
				EmitByte(0x8b);EmitByte(0x04);EmitByte(0x85);EmitAdr(jit->statementoffsets+1);
//				jmp *eax
				EmitByte(0xff);EmitByte(0xe0);
//			returntoc:
			l1 = LocalLoc();
//			ret
			EmitByte(0xc3);

			LocalJmpLoc(j1,l1);
			break;

		//function calls
		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
			//FIXME: the size of this instruction is going to hurt cache performance if every single function call is expanded into this HUGE CHUNK of gibberish!
			//FIXME: consider the feasability of just calling a C function and just jumping to the address it returns.

		//save the state in place the rest of the engine can cope with
			//movl $i, pr_xstatement
			EmitByte( 0xc7);EmitByte(0x05);EmitAdr(&pr_xstatement);Emit4Byte(i);
			//movl $(op[i].op-OP_CALL0), pr_argc
			EmitByte( 0xc7);EmitByte(0x05);EmitAdr(&progfuncs->funcs.callargc);Emit4Byte(op[i].op-OP_CALL0);

		//figure out who we're calling, and what that involves
			//%eax = glob[A]
			LOADREG(glob + op[i].a, REG_EAX);
		//eax is now the func num

			//mov %eax,%ecx
			EmitByte(0x89); EmitByte(0xc1);
			//shr $24,%ecx
			EmitByte(0xc1); EmitByte(0xe9); EmitByte(0x18);
		//ecx is now the progs num for the new func

/*
			//cmp %ecx,pr_typecurrent
			EmitByte(0x39); EmitByte(0x0d); EmitAdr(&pr_typecurrent);
			//je sameprogs
			j1 = LocalJmp(OP_EQ_I);
			{
				//can't handle switching progs

				//FIXME: recurse though PR_ExecuteProgram
				//push eax
				//push progfuncs
				//call PR_ExecuteProgram
				//add $8,%esp
				//remember to change the je above

				//err... exit depth? no idea
				EmitByte(0xcd);EmitByte(op[i].op);	//int $X


				//ret
				EmitByte(0xc3);
			}
			//sameprogs:
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);
*/

			//andl $0x00ffffff, %eax
			EmitByte(0x25);Emit4Byte(0x00ffffff);
			
			//mov $sizeof(dfunction_t),%edx
			EmitByte(0xba);Emit4Byte(sizeof(dfunction_t));
			//mul %edx
			EmitByte(0xf7); EmitByte(0xe2);
			//add pr_functions,%eax
			EmitByte(0x05); EmitAdr(current_progstate->functions);

		//eax is now the dfunction_t to be called
		//edx is clobbered.

			//mov (%eax),%edx
			EmitByte(0x8b);EmitByte(0x10);
		//edx is now the first statement number
			//cmp $0,%edx
			EmitByte(0x83);EmitByte(0xfa);EmitByte(0x00);
			//jl isabuiltin
			j1 = LocalJmp(OP_LT_I);
			{
				/* call the function*/
				//push %ecx
				EmitByte(0x51);
				//push %eax
				EmitByte(0x50);
				//pushl progfuncs
				EmitByte(0x68);EmitAdr(progfuncs);
				//call PR_EnterFunction
				EmitByte(0xe8);EmitFOffset(PR_EnterFunction, 4);
				//sub $12,%esp
				EmitByte(0x83);EmitByte(0xc4);EmitByte(0xc);
		//eax is now the next statement number (first of the new function, usually equal to ecx, but not always)

				//jmp statementoffsets[%eax*4]
				EmitByte(0xff);EmitByte(0x24);EmitByte(0x85);EmitAdr(jit->statementoffsets+1);
			}
			/*its a builtin, figure out which, and call it*/
			//isabuiltin:
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);

			//push current_progstate->globals
			EmitByte(0x68);EmitAdr(current_progstate->globals);
			//push progfuncs
			EmitByte(0x68);EmitAdr(progfuncs);
			//neg %edx
			EmitByte(0xf7);EmitByte(0xda);
			//call externs->globalbuiltins[%edx,4]
//FIXME: make sure this dereferences
			EmitByte(0xff);EmitByte(0x14);EmitByte(0x95);EmitAdr(externs->globalbuiltins);
			//add $8,%esp
			EmitByte(0x83);EmitByte(0xc4);EmitByte(0x8);

		//but that builtin might have been Abort()

			LOADREG(&prinst.continuestatement, REG_EAX);
			//cmp $-1,%eax
			EmitByte(0x83);EmitByte(0xf8);EmitByte(0xff);
			//je donebuiltincall
			j1 = LocalJmp(OP_EQ_I);
			{
				//mov $-1,prinst->continuestatement
				EmitByte(0xc7);EmitByte(0x05);EmitAdr(&prinst.continuestatement);Emit4Byte((unsigned int)-1);

				//jmp statementoffsets[%eax*4]
				EmitByte(0xff);EmitByte(0x24);EmitByte(0x85);EmitAdr(jit->statementoffsets);
			}
			//donebuiltincall:
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);
			break;

		case OP_MUL_F:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fmuls glob[B]
			EmitByte(0xd8);EmitByte(0x0d);EmitAdr(glob + op[i].b);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;
		case OP_DIV_F:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fdivs glob[B]
			EmitByte(0xd8);EmitByte(0x35);EmitAdr(glob + op[i].b);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;
		case OP_ADD_F:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fadds glob[B]
			EmitByte(0xd8);EmitByte(0x05);EmitAdr(glob + op[i].b);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;
		case OP_SUB_F:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fsubs glob[B]
			EmitByte(0xd8);EmitByte(0x25);EmitAdr(glob + op[i].b);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;

		case OP_NOT_F:
			//fldz
			EmitByte(0xd9);EmitByte(0xee);
			//fcomps	glob[A]
			EmitByte(0xd8); EmitByte(0x1d); EmitAdr(glob + op[i].a);
			//fnstsw %ax
			EmitByte(0xdf);EmitByte(0xe0);
			//testb 0x40,%ah
			EmitByte(0xf6);EmitByte(0xc4);EmitByte(0x40);
			
			j1 = LocalJmp(OP_NE_F);
			{
				STOREF(0.0f, glob + op[i].c);
				j2 = LocalJmp(OP_GOTO);
			}
			{
				//noteq:
				l1 = LocalLoc();
				STOREF(1.0f, glob + op[i].c);
			}
			//end:
			l2 = LocalLoc();
			LocalJmpLoc(j1,l1);
			LocalJmpLoc(j2,l2);
			break;

		case OP_STORE_F:
		case OP_STORE_S:
		case OP_STORE_ENT:
		case OP_STORE_FLD:
		case OP_STORE_FNC:
			LOADREG(glob + op[i].a, REG_EAX);
			STOREREG(REG_EAX, glob + op[i].b);
			break;

		case OP_STORE_V:
			LOADREG(glob + op[i].a+0, REG_EAX);
			LOADREG(glob + op[i].a+1, REG_EDX);
			LOADREG(glob + op[i].a+2, REG_ECX);
			STOREREG(REG_EAX, glob + op[i].b+0);
			STOREREG(REG_EDX, glob + op[i].b+1);
			STOREREG(REG_ECX, glob + op[i].b+2);
			break;

		case OP_LOAD_F:
		case OP_LOAD_S:
		case OP_LOAD_ENT:
		case OP_LOAD_FLD:
		case OP_LOAD_FNC:
		case OP_LOAD_V:
		//a is the ent number, b is the field
		//c is the dest

			LOADREG(glob + op[i].a, REG_EAX);
			LOADREG(glob + op[i].b, REG_ECX);

		//FIXME: bound eax (ent number)
		//FIXME: bound ecx (field index)
			//mov (ebx,eax,4).%eax
			EmitByte(0x8b); EmitByte(0x04); EmitByte(0x83);
		//eax is now an edictrun_t
			//mov fields(,%eax,4),%edx
			EmitByte(0x8b);EmitByte(0x50);EmitByte((int)&((edictrun_t*)NULL)->fields);
		//edx is now the field array for that ent

			//mov fieldajust(%edx,%ecx,4),%eax
			EmitByte(0x8b); EmitByte(0x84); EmitByte(0x8a); Emit4Byte(progfuncs->funcs.fieldadjust*4);

			STOREREG(REG_EAX, glob + op[i].c)

			if (op[i].op == OP_LOAD_V)
			{
				//mov fieldajust+4(%edx,%ecx,4),%eax
				EmitByte(0x8b); EmitByte(0x84); EmitByte(0x8a); Emit4Byte(4+progfuncs->funcs.fieldadjust*4);
				STOREREG(REG_EAX, glob + op[i].c+1)

				//mov fieldajust+8(%edx,%ecx,4),%eax
				EmitByte(0x8b); EmitByte(0x84); EmitByte(0x8a); Emit4Byte(8+progfuncs->funcs.fieldadjust*4);
				STOREREG(REG_EAX, glob + op[i].c+2)
			}
			break;

		case OP_ADDRESS:
			//a is the ent number, b is the field
		//c is the dest

			LOADREG(glob + op[i].a, REG_EAX);
			LOADREG(glob + op[i].b, REG_ECX);

		//FIXME: bound eax (ent number)
		//FIXME: bound ecx (field index)
			//mov (ebx,eax,4).%eax
			EmitByte(0x8b); EmitByte(0x04); EmitByte(0x83);
		//eax is now an edictrun_t
			//mov fields(,%eax,4),%edx
			EmitByte(0x8b);EmitByte(0x50);EmitByte((int)&((edictrun_t*)NULL)->fields);
		//edx is now the field array for that ent
			//mov fieldajust(%edx,%ecx,4),%eax	//offset = progfuncs->fieldadjust
			//EmitByte(0x8d); EmitByte(0x84); EmitByte(0x8a); EmitByte(progfuncs->funcs.fieldadjust*4);
			EmitByte(0x8d); EmitByte(0x84); EmitByte(0x8a); Emit4Byte(progfuncs->funcs.fieldadjust*4);
			STOREREG(REG_EAX, glob + op[i].c);
			break;

		case OP_STOREP_F:
		case OP_STOREP_S:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:
		case OP_STOREP_FNC:
			LOADREG(glob + op[i].a, REG_EAX);
			LOADREG(glob + op[i].b, REG_ECX);
			//mov %eax,(%ecx)
			EmitByte(0x89);EmitByte(0x01);
			break;

		case OP_STOREP_V:
			LOADREG(glob + op[i].b, REG_ECX);

			LOADREG(glob + op[i].a+0, REG_EAX);
			//mov %eax,0(%ecx)
			EmitByte(0x89);EmitByte(0x01);

			LOADREG(glob + op[i].a+1, REG_EAX);
			//mov %eax,4(%ecx)
			EmitByte(0x89);EmitByte(0x41);EmitByte(0x04);

			LOADREG(glob + op[i].a+2, REG_EAX);
			//mov %eax,8(%ecx)
			EmitByte(0x89);EmitByte(0x41);EmitByte(0x08);
			break;

		case OP_NE_I:
		case OP_NE_E:
		case OP_NE_FNC:
		case OP_EQ_I:
		case OP_EQ_E:
		case OP_EQ_FNC:
			//integer equality
			LOADREG(glob + op[i].a, REG_EAX);

			//cmp glob[B],%eax
			EmitByte(0x3b); EmitByte(0x04); EmitByte(0x25); EmitAdr(glob + op[i].b);
			j1 = LocalJmp(op[i].op);
			{
				STOREF(0.0f, glob + op[i].c);
				j2 = LocalJmp(OP_GOTO);
			}
			{
				l1 = LocalLoc();
				STOREF(1.0f, glob + op[i].c);
			}
			l2 = LocalLoc();
			LocalJmpLoc(j1,l1);
			LocalJmpLoc(j2,l2);
			break;

		case OP_NOT_I:
		case OP_NOT_ENT:
		case OP_NOT_FNC:
			//cmp glob[B],$0
			EmitByte(0x83); EmitByte(0x3d); EmitAdr(glob + op[i].a); EmitByte(0x00); 
			j1 = LocalJmp(OP_NE_I);
			{
				STOREF(1.0f, glob + op[i].c);
				j2 = LocalJmp(OP_GOTO);
			}
			{
				l1 = LocalLoc();
				STOREF(0.0f, glob + op[i].c);
			}
			l2 = LocalLoc();
			LocalJmpLoc(j1,l1);
			LocalJmpLoc(j2,l2);
			break;

		case OP_BITOR_F:	//floats...
			//flds glob[A]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].a);
			//flds glob[B]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].b);
			//fistp tb
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(&tb);
			//fistp ta
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(&ta);
			LOADREG(&ta, REG_EAX)
			//or %eax,tb
			EmitByte(0x09); EmitByte(0x05);EmitAdr(&tb);
			//fild tb
			EmitByte(0xdb); EmitByte(0x05);EmitAdr(&tb);
			//fstps glob[C]
			EmitByte(0xd9); EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;

		case OP_BITAND_F:
			//flds glob[A]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].a);
			//flds glob[B]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].b);
			//fistp tb
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(&tb);
			//fistp ta
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(&ta);
			/*two args are now at ta and tb*/
			LOADREG(&ta, REG_EAX)
			//and tb,%eax
			EmitByte(0x21); EmitByte(0x05);EmitAdr(&tb);
			/*we just wrote the int value to tb, convert that to a float and store it at c*/
			//fild tb
			EmitByte(0xdb); EmitByte(0x05);EmitAdr(&tb);
			//fstps glob[C]
			EmitByte(0xd9); EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;

		case OP_AND_F:
			//test floats properly, so we don't get confused with -0.0
			//FIXME: is it feasable to grab the value as an int and test it against 0x7fffffff?

			//flds	glob[A]
			EmitByte(0xd9); EmitByte(0x05); EmitAdr(glob + op[i].a);
			//fcomps	nullfloat
			EmitByte(0xd8); EmitByte(0x1d); EmitAdr(&nullfloat);
			//fnstsw	%ax
			EmitByte(0xdf); EmitByte(0xe0);
			//test	$0x40,%ah
			EmitByte(0xf6); EmitByte(0xc4);EmitByte(0x40);
			//jz onefalse
			EmitByte(0x75); EmitByte(0x1f);

			//flds	glob[B]
			EmitByte(0xd9); EmitByte(0x05); EmitAdr(glob + op[i].b);
			//fcomps	nullfloat
			EmitByte(0xd8); EmitByte(0x1d); EmitAdr(&nullfloat);
			//fnstsw	%ax
			EmitByte(0xdf); EmitByte(0xe0);
			//test	$0x40,%ah
			EmitByte(0xf6); EmitByte(0xc4);EmitByte(0x40);
			//jnz onefalse
			EmitByte(0x75); EmitByte(0x0c);

			//mov float0,glob[C]
			EmitByte(0xc7); EmitByte(0x05); EmitAdr(glob + op[i].c); EmitFloat(1.0f);
			//jmp done
			EmitByte(0xeb); EmitByte(0x0a);

			//onefalse:
			//mov float1,glob[C]
			EmitByte(0xc7); EmitByte(0x05); EmitAdr(glob + op[i].c); EmitFloat(0.0f);
			//done:
			break;
		case OP_OR_F:
			//test floats properly, so we don't get confused with -0.0

			//flds	glob[A]
			EmitByte(0xd9); EmitByte(0x05); EmitAdr(glob + op[i].a);
			//fcomps	nullfloat
			EmitByte(0xd8); EmitByte(0x1d); EmitAdr(&nullfloat);
			//fnstsw	%ax
			EmitByte(0xdf); EmitByte(0xe0);
			//test	$0x40,%ah
			EmitByte(0xf6); EmitByte(0xc4);EmitByte(0x40);
			//je onetrue
			EmitByte(0x74); EmitByte(0x1f);

			//flds	glob[B]
			EmitByte(0xd9); EmitByte(0x05); EmitAdr(glob + op[i].b);
			//fcomps	nullfloat
			EmitByte(0xd8); EmitByte(0x1d); EmitAdr(&nullfloat);
			//fnstsw	%ax
			EmitByte(0xdf); EmitByte(0xe0);
			//test	$0x40,%ah
			EmitByte(0xf6); EmitByte(0xc4);EmitByte(0x40);
			//je onetrue
			EmitByte(0x74); EmitByte(0x0c);

			//mov float0,glob[C]
			EmitByte(0xc7); EmitByte(0x05); EmitAdr(glob + op[i].c); EmitFloat(0.0f);
			//jmp done
			EmitByte(0xeb); EmitByte(0x0a);

			//onetrue:
			//mov float1,glob[C]
			EmitByte(0xc7); EmitByte(0x05); EmitAdr(glob + op[i].c); EmitFloat(1.0f);
			//done:
			break;

		case OP_EQ_S:
		case OP_NE_S:
			{
			//put a in ecx
			LOADREG(glob + op[i].a, REG_ECX);
			//put b in edi
			LOADREG(glob + op[i].b, REG_EDI);
/*
			//early out if they're equal
			//cmp %ecx,%edi
			EmitByte(0x39); EmitByte(0xc0 | (REG_EDI<<3) | REG_ECX);
			j1c = LocalJmp(OP_EQ_S);

			//if a is 0, check if b is ""
			//jecxz ais0
			EmitByte(0xe3); EmitByte(0x1a);

			//if b is 0, check if a is ""
  			//cmp $0,%edi
			EmitByte(0x83); EmitByte(0xff); EmitByte(0x00);
			//jne bnot0
			EmitByte(0x75); EmitByte(0x2a);
			{
				//push a
				EmitByte(0x51);
				//push progfuncs
				EmitByte(0x68); EmitAdr(progfuncs);
				//call PR_StringToNative
				EmitByte(0xe8); EmitFOffset(PR_StringToNative,4);
				//add $8,%esp
				EmitByte(0x83); EmitByte(0xc4); EmitByte(0x08);
				//cmpb $0,(%eax)
				EmitByte(0x80); EmitByte(0x38); EmitByte(0x00);
				j1b = LocalJmp(OP_EQ_S);
				j0b = LocalJmp(OP_GOTO);
			}

			//ais0:
			{
				//push edi
				EmitByte(0x57);
				//push progfuncs
				EmitByte(0x68); EmitAdr(progfuncs);
				//call PR_StringToNative
				EmitByte(0xe8); EmitFOffset(PR_StringToNative,4);
				//add $8,%esp
				EmitByte(0x83); EmitByte(0xc4); EmitByte(0x08);
				//cmpb $0,(%eax)
				EmitByte(0x80); EmitByte(0x38); EmitByte(0x00);
				//je _true
				EmitByte(0x74); EmitByte(0x36);
				//jmp _false
				EmitByte(0xeb); EmitByte(0x28);
			}
			//bnot0:
*/
LOADREG(glob + op[i].a, REG_ECX);
			//push ecx
			EmitByte(0x51);
			//push progfuncs
			EmitByte(0x68); EmitAdr(progfuncs);
			//call PR_StringToNative
			EmitByte(0xe8); EmitFOffset(PR_StringToNative,4);
			//push %eax
			EmitByte(0x50);

LOADREG(glob + op[i].b, REG_EDI);
			//push %edi
			EmitByte(0x57);
			//push progfuncs
			EmitByte(0x68); EmitAdr(progfuncs);
			//call PR_StringToNative
			EmitByte(0xe8); EmitFOffset(PR_StringToNative,4);
			//add $8,%esp
			EmitByte(0x83); EmitByte(0xc4); EmitByte(0x08);


			//push %eax
			EmitByte(0x50);
			//call strcmp
			EmitByte(0xe8); EmitFOffset(strcmp,4);
			//add $16,%esp
			EmitByte(0x83); EmitByte(0xc4); EmitByte(0x10);

			//cmp $0,%eax
			EmitByte(0x83); EmitByte(0xf8); EmitByte(0x00);
			j1 = LocalJmp(OP_EQ_S);
			{
				l0 = LocalLoc();
				STOREF((op[i].op == OP_NE_S)?1.0f:0.0f, glob + op[i].c);
				j2 = LocalJmp(OP_GOTO);
			}
			{
				l1 = LocalLoc();
				STOREF((op[i].op == OP_NE_S)?0.0f:1.0f, glob + op[i].c);
			}
			l2 = LocalLoc();

//			LocalJmpLoc(j0b, l0);
			LocalJmpLoc(j1, l1);
//			LocalJmpLoc(j1b, l1);
			LocalJmpLoc(j2, l2);
			}
			break;

		case OP_NOT_S:
			LOADREG(glob + op[i].a, REG_EAX)

			//cmp $0,%eax
			EmitByte(0x83); EmitByte(0xf8); EmitByte(0x00);
			j2 = LocalJmp(OP_EQ_S);

			//push %eax
			EmitByte(0x50);
			//push progfuncs
			EmitByte(0x68); EmitAdr(progfuncs);
			//call PR_StringToNative
			EmitByte(0xe8); EmitFOffset(PR_StringToNative,4);
			//add $8,%esp
			EmitByte(0x83); EmitByte(0xc4); EmitByte(0x08);

			//cmpb $0,(%eax)
			EmitByte(0x80); EmitByte(0x38); EmitByte(0x00);
			j1 = LocalJmp(OP_EQ_S);
			{
				STOREF(0.0f, glob + op[i].c);
				j0 = LocalJmp(OP_GOTO);
			}
			{
				l1 = LocalLoc();
				STOREF(1.0f, glob + op[i].c);
			}
			l2 = LocalLoc();
			LocalJmpLoc(j2, l1);
			LocalJmpLoc(j1, l1);
			LocalJmpLoc(j0, l2);
			break;

		case OP_ADD_V:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+0);
			//fadds glob[B]
			EmitByte(0xd8);EmitByte(0x05);EmitAdr(glob + op[i].b+0);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+0);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+1);
			//fadds glob[B]
			EmitByte(0xd8);EmitByte(0x05);EmitAdr(glob + op[i].b+1);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+1);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+2);
			//fadds glob[B]
			EmitByte(0xd8);EmitByte(0x05);EmitAdr(glob + op[i].b+2);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+2);
			break;
		case OP_SUB_V:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+0);
			//fsubs glob[B]
			EmitByte(0xd8);EmitByte(0x25);EmitAdr(glob + op[i].b+0);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+0);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+1);
			//fsubs glob[B]
			EmitByte(0xd8);EmitByte(0x25);EmitAdr(glob + op[i].b+1);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+1);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+2);
			//fsubs glob[B]
			EmitByte(0xd8);EmitByte(0x25);EmitAdr(glob + op[i].b+2);
			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+2);
			break;

		case OP_MUL_V:
			//this is actually a dotproduct
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+0);
			//fmuls glob[B]
			EmitByte(0xd8);EmitByte(0x0d);EmitAdr(glob + op[i].b+0);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+1);
			//fmuls glob[B]
			EmitByte(0xd8);EmitByte(0x0d);EmitAdr(glob + op[i].b+1);

			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+2);
			//fmuls glob[B]
			EmitByte(0xd8);EmitByte(0x0d);EmitAdr(glob + op[i].b+2);

			//faddp
			EmitByte(0xde);EmitByte(0xc1);
			//faddp
			EmitByte(0xde);EmitByte(0xc1);

			//fstps glob[C]
			EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;

		case OP_EQ_F:
		case OP_NE_F:
		case OP_LE_F:
		case OP_GE_F:
		case OP_LT_F:
		case OP_GT_F:
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].b);
			//flds glob[B]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fcomip %st(1),%st
			EmitByte(0xdf);EmitByte(0xe9);
			//fstp %st(0)	(aka: pop)
			EmitByte(0xdd);EmitByte(0xd8);

			j1 = LocalJmp(op[i].op);
			{
				STOREF(0.0f, glob + op[i].c);
				j2 = LocalJmp(OP_GOTO);
			}
			{
				l1 = LocalLoc();
				STOREF(1.0f, glob + op[i].c);
			}
			l2 = LocalLoc();
			LocalJmpLoc(j1,l1);
			LocalJmpLoc(j2,l2);
			break;

		case OP_MUL_FV:
		case OP_MUL_VF:
			//
			{
				int v;
				int f;
				if (op[i].op == OP_MUL_FV)
				{
					f = op[i].a;
					v = op[i].b;
				}
				else
				{
					v = op[i].a;
					f = op[i].b;
				}

				//flds glob[F]
				EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + f);

				//flds glob[V0]
				EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + v+0);
				//fmul st(1)
				EmitByte(0xd8);EmitByte(0xc9);
				//fstps glob[C]
				EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+0);

				//flds glob[V0]
				EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + v+1);
				//fmul st(1)
				EmitByte(0xd8);EmitByte(0xc9);
				//fstps glob[C]
				EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+1);

				//flds glob[V0]
				EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + v+2);
				//fmul st(1)
				EmitByte(0xd8);EmitByte(0xc9);
				//fstps glob[C]
				EmitByte(0xd9);EmitByte(0x1d);EmitAdr(glob + op[i].c+2);

				//fstp %st(0)	(aka: pop)
				EmitByte(0xdd);EmitByte(0xd8);
			}
			break;

		case OP_STATE:
			//externs->stateop(progfuncs, OPA->_float, OPB->function);
			//push b
			EmitByte(0xff);EmitByte(0x35);EmitAdr(glob + op[i].b);
			//push a
			EmitByte(0xff);EmitByte(0x35);EmitAdr(glob + op[i].a);
			//push $progfuncs
			EmitByte(0x68); EmitAdr(progfuncs);
			//call externs->stateop
			EmitByte(0xe8); EmitFOffset(externs->stateop, 4);
			//add $12,%esp
			EmitByte(0x83); EmitByte(0xc4); EmitByte(0x0c);
			break;
#if 1
/*		case OP_NOT_V:
			//flds 0
			//flds glob[A+0]
			//fcomip %st(1),%st
			//jne _true
			//flds glob[A+1]
			//fcomip %st(1),%st
			//jne _true
			//flds glob[A+1]
			//fcomip %st(1),%st
			//jne _true
			//mov 1,C
			//jmp done
			//_true:
			//mov 0,C
			//done:
			break;
*/
			
		case OP_NOT_V:
			EmitByte(0xcd);EmitByte(op[i].op);
			printf("QCJIT: instruction %i is not implemented\n", op[i].op);
			break;
#endif
		case OP_NE_V:
		case OP_EQ_V:
		{
			void *f0, *f1, *f2, *floc;
//compare v[0]
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+0);
			//flds glob[B]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].b+0);
			//fcomip %st(1),%st
			EmitByte(0xdf);EmitByte(0xe9);
			//fstp %st(0)	(aka: pop)
			EmitByte(0xdd);EmitByte(0xd8);

			/*if the condition is true, don't fail*/
			j1 = LocalJmp(op[i].op);
			{
				STOREF(0.0f, glob + op[i].c);
				f0 = LocalJmp(OP_GOTO);
			}
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);

//compare v[1]
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+1);
			//flds glob[B]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].b+1);
			//fcomip %st(1),%st
			EmitByte(0xdf);EmitByte(0xe9);
			//fstp %st(0)	(aka: pop)
			EmitByte(0xdd);EmitByte(0xd8);

			/*if the condition is true, don't fail*/
			j1 = LocalJmp(op[i].op);
			{
				STOREF(0.0f, glob + op[i].c);
				f1 = LocalJmp(OP_GOTO);
			}
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);

//compare v[2]
			//flds glob[A]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].a+2);
			//flds glob[B]
			EmitByte(0xd9);EmitByte(0x05);EmitAdr(glob + op[i].b+2);
			//fcomip %st(1),%st
			EmitByte(0xdf);EmitByte(0xe9);
			//fstp %st(0)	(aka: pop)
			EmitByte(0xdd);EmitByte(0xd8);

			/*if the condition is true, don't fail*/
			j1 = LocalJmp(op[i].op);
			{
				STOREF(0.0f, glob + op[i].c);
				f2 = LocalJmp(OP_GOTO);
			}
			l1 = LocalLoc();
			LocalJmpLoc(j1,l1);

//success!
			STOREF(1.0f, glob + op[i].c);

			floc = LocalLoc();
			LocalJmpLoc(f0,floc);
			LocalJmpLoc(f1,floc);
			LocalJmpLoc(f2,floc);
			break;
		}

		/*fteqcc generates these from reading 'fast arrays', and are part of hexenc extras*/
		case OP_FETCH_GBL_F:
		case OP_FETCH_GBL_S:
		case OP_FETCH_GBL_E:
		case OP_FETCH_GBL_FNC:
		case OP_FETCH_GBL_V:
		{
			unsigned int max = ((unsigned int*)glob)[op[i].a-1];
			unsigned int base = op[i].a;
			//flds glob[B]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].b);
			//fistp ta
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(&ta);
			LOADREG(&ta, REG_EAX)
			//FIXME: if eax >= $max, abort

			if (op[i].op == OP_FETCH_GBL_V)
			{
				/*scale the index by 3*/
				SETREGI(3, REG_EDX)
				//mul %edx
				EmitByte(0xf7); EmitByte(0xe2);
			}

			//lookup global
			//mov &glob[base](,%eax,4),%edx
			EmitByte(0x8b);EmitByte(0x14);EmitByte(0x85);Emit4Byte((unsigned int)(glob + base+0));
			STOREREG(REG_EDX, glob + op[i].c+0)
			if (op[i].op == OP_FETCH_GBL_V)
			{
				//mov &glob[base+1](,%eax,4),%edx
				EmitByte(0x8b);EmitByte(0x14);EmitByte(0x85);Emit4Byte((unsigned int)(glob + base+1));
				STOREREG(REG_EDX, glob + op[i].c+1)
				//mov &glob[base+2](,%eax,4),%edx
				EmitByte(0x8b);EmitByte(0x14);EmitByte(0x85);Emit4Byte((unsigned int)(glob + base+2));
				STOREREG(REG_EDX, glob + op[i].c+2)
			}
			break;
		}

		/*fteqcc generates these from writing 'fast arrays'*/
		case OP_GLOBALADDRESS:
			LOADREG(glob + op[i].b, REG_EAX);
			//lea &glob[A](, %eax, 4),%eax
			EmitByte(0x8d);EmitByte(0x04);EmitByte(0x85);EmitAdr(glob + op[i].b+2);
			STOREREG(REG_EAX, glob + op[i].c);
			break;
//		case OP_BOUNDCHECK:
			//FIXME: assert b <= a < c
			break;
		case OP_CONV_FTOI:
			//flds glob[A]
			EmitByte(0xd9); EmitByte(0x05);EmitAdr(glob + op[i].a);
			//fistp glob[C]
			EmitByte(0xdb); EmitByte(0x1d);EmitAdr(glob + op[i].c);
			break;
		case OP_MUL_I:
			LOADREG(glob + op[i].a, REG_EAX);
			//mull glob[C]       (arg*eax => edx:eax)
			EmitByte(0xfc); EmitByte(0x25);EmitAdr(glob + op[i].b);
			STOREREG(REG_EAX, glob + op[i].c);
			break;

		/*other extended opcodes*/
		case OP_BITOR_I:
			LOADREG(glob + op[i].a, REG_EAX)
			//or %eax,tb
			EmitByte(0x0b); EmitByte(0x05);EmitAdr(glob + op[i].b);
			STOREREG(REG_EAX, glob + op[i].c);
			break;


		default:
			{
				enum qcop_e e = op[i].op;
			printf("QCJIT: Extended instruction set %i is not supported, not using jit.\n", e);
			}


			failed = true;
			break;
		}
	}

	if(failed)
	{	
		free(jit->statementjumps);	//[MAX_STATEMENTS]
		free(jit->statementoffsets); //[MAX_STATEMENTS]
		free(jit->code);
		free(jit);
		return NULL;
	}

	FixupJumps(jit);

	/* most likely want executable memory calls somewhere else more common */
#ifdef _WIN32
	{
		DWORD old;

		//this memory is on the heap.
		//this means that we must maintain read/write protection, or libc will crash us
		VirtualProtect(jit->code, jit->codesize, PAGE_EXECUTE_READWRITE, &old);
	}
#else
	mprotect(jit->code, jit->codesize, PROT_READ|PROT_EXEC);
#endif

//	externs->WriteFile("jit.x86", jit->code, jit->codesize);

	return jit;
}

static float foo(float arg)
{
	float f;
	if (!arg)
		f = 1;
	else
		f = 0;
	return f;
}

void PR_EnterJIT(progfuncs_t *progfuncs, struct jitstate *jit, int statement)
{
#ifdef __GNUC__
	//call, it clobbers pretty much everything.
	asm("call *%0" :: "r"(jit->statementoffsets[statement+1]),"b"(prinst->edicttable):"cc","memory","eax","ecx","edx","esi","edi");
#elif defined(_MSC_VER)
	void *entry = jit->statementoffsets[statement+1];
	void *edicttable = prinst.edicttable;
	__asm {
		pushad
		mov eax,entry
		mov ebx,edicttable
		call eax
		popad
	}
#else
	#error "Sorry, no idea how to enter assembler safely for your compiler"
#endif
}
#endif
