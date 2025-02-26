/*************************************************************************
** QVM
** Copyright (C) 2003 by DarkOne
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**************************************************************************
** Quake3 compatible virtual machine
*************************************************************************/

/*
spike's changes.
masks are now done by modulus rather than and
VM_POINTER contains a mask check.
ds_mask is set to the mem allocated for stack and data.
builtins range check written to buffers.

QVM_Step placed in QVM_Exec for efficiency.

an invalid statement was added at the end of the statements to prevent the qvm walking off.
stack pops/pushes are all tested. An extra stack entry was faked to prevent stack checks on double-stack operators from overwriting.


Fixme: there is always the possibility that I missed a potential virus loophole..
Also, can efficiency be improved much?
*/


#include "quakedef.h"

#ifdef VM_ANY

#if defined(_MSC_VER)	//fix this please
	#ifdef inline
		#undef inline
	#endif
	#define inline __forceinline
#endif



#define RETURNOFFSETMARKER NULL

typedef enum vm_type_e
{
	VM_NONE,
	VM_NATIVE,
	VM_BYTECODE,
	VM_BUILTIN
} vm_type_t;

struct vm_s {
// common
	vm_type_t type;
	char filename[MAX_OSPATH];
	sys_calldll_t syscalldll;
	sys_callqvm_t syscallqvm;

// shared
	void *hInst;

// native
	qintptr_t (EXPORT_FN *vmMain)(qintptr_t command, qintptr_t arg0, qintptr_t arg1, qintptr_t arg2, qintptr_t arg3, qintptr_t arg4, qintptr_t arg5, qintptr_t arg6, qintptr_t arg7);
};

//this is a bit weird. qvm plugins always come from $basedir/$mod/plugins/$foo.qvm
//but native plugins never come from $basedir/$mod/ - too many other engines blindly allow dll downloads etc. Its simply far too insecure if people use other engines.
//q3 gamecode allows it however. yes you could probably get fte to connect via q3 instead and get such a dll.
qboolean QVM_LoadDLL(vm_t *vm, const char *name, qboolean binroot, void **vmMain, sys_calldll_t syscall)
{
	void (EXPORT_FN *dllEntry)(sys_calldll_t syscall);
	dllhandle_t *hVM;

	char fname[MAX_OSPATH*2];
	char gpath[MAX_OSPATH];
	char displaypath[MAX_OSPATH*2];
	void *iterator;

	dllfunction_t funcs[] =
	{
		{(void*)&dllEntry, "dllEntry"},
		{(void*)vmMain, "vmMain"},
		{NULL, NULL},
	};

	hVM=NULL;
	*fname = 0;

	if (binroot)
	{
		//load eg: basedir/module_gamedir_arch.ext
		Con_DPrintf("Attempting to load native library: %s\n", name);

		// run through the search paths
		iterator = NULL;
		while (!hVM && COM_IteratePaths(&iterator, NULL, 0, gpath, sizeof(gpath)))
		{
#ifndef ANDROID
			if (!hVM && FS_SystemPath(va("%s_%s_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name, gpath), FS_BINARYPATH, fname, sizeof(fname)))
			{
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
#endif
			if (!hVM && FS_SystemPath(va("%s_%s"ARCH_DL_POSTFIX, name, gpath), FS_BINARYPATH, fname, sizeof(fname)))
			{
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}

#ifndef ANDROID
			if (!hVM && FS_SystemPath(va("%s_%s_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name, gpath), FS_ROOT, fname, sizeof(fname)))
			{
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
#endif
			if (!hVM && FS_SystemPath(va("%s_%s"ARCH_DL_POSTFIX, name, gpath), FS_ROOT, fname, sizeof(fname)))
			{
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
		}

#ifdef ANDROID
		if (!hVM && FS_SystemPath(va("%s" ARCH_DL_POSTFIX, name), FS_BINARYPATH, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
#else

		if (!hVM && FS_SystemPath(va("%s_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name), FS_BINARYPATH, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
		if (!hVM && FS_SystemPath(va("%s"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name), FS_BINARYPATH, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
		if (!hVM && FS_SystemPath(va("%s" ARCH_DL_POSTFIX, name), FS_BINARYPATH, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);

		if (!hVM && FS_SystemPath(va("%s_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name), FS_ROOT, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
		if (!hVM && FS_SystemPath(va("%s"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, name), FS_ROOT, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
		if (!hVM && FS_SystemPath(va("%s" ARCH_DL_POSTFIX, name), FS_ROOT, fname, sizeof(fname)))
			hVM = Sys_LoadLibrary(fname, funcs);
#endif
	}
	else
	{
		//load eg: basedir/gamedir/module_arch.ext
		Con_DPrintf("Attempting to load (unsafe) native library: %s\n", name);

		// run through the search paths
		iterator = NULL;
		while (!hVM && COM_IteratePaths(&iterator, gpath, sizeof(gpath), NULL, 0))
		{
			if (!hVM)
			{
				snprintf (fname, sizeof(fname), "%s%s_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, gpath, name);
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
			if (!hVM)
			{
				snprintf (fname, sizeof(fname), "%s%s"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, gpath, name);
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}

#ifdef ARCH_ALTCPU_POSTFIX
			if (!hVM)
			{
				snprintf (fname, sizeof(fname), "%s%s_"ARCH_ALTCPU_POSTFIX ARCH_DL_POSTFIX, gpath, name);
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
			if (!hVM)
			{
				snprintf (fname, sizeof(fname), "%s%s"ARCH_ALTCPU_POSTFIX ARCH_DL_POSTFIX, gpath, name);
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
#endif

			if (!hVM)
			{
				snprintf (fname, sizeof(fname), "%s%s"ARCH_DL_POSTFIX, gpath, name);
				if (FS_DisplayPath(fname, FS_SYSTEM, displaypath, sizeof(displaypath)))
					Con_DLPrintf(2, "Loading native: %s\n", displaypath);
				hVM = Sys_LoadLibrary(fname, funcs);
			}
		}
	}

	if(!hVM) return false;

	Q_strncpyz(vm->filename, fname, sizeof(vm->filename));
	vm->hInst = hVM;

	(*dllEntry)(syscall);

	return true;
}

/*
** Sys_UnloadDLL
*/
void QVM_UnloadDLL(dllhandle_t *handle)
{
	if(handle)
	{
		Sys_CloseLibrary(handle);
	}
}










// ------------------------- * QVM files * -------------------------
#define	VM_MAGIC	0x12721444
#define	VM_MAGIC2	0x12721445

#pragma pack(push,1)
typedef struct vmHeader_s
{
	int vmMagic;

	int instructionCount;

	int codeOffset;
	int codeLength;

	int dataOffset;
	int dataLength;	// should be byteswapped on load
	int litLength;	// copy as is
	int bssLength;	// zero filled memory appended to datalength

	//valid only in V2.
	int		jtrgLength;			// number of jump table targets
} vmHeader_t;
#pragma pack(pop)

// ------------------------- * in memory representation * -------------------------

typedef struct qvm_s
{
// segments
	unsigned int *cs;	// code  segment, each instruction is 2 ints
	qbyte *ds;	// data  segment, partially filled on load (data, lit, bss)
	qbyte *ss;	// stack segment, (follows immediatly after ds, corrupting data before vm)

// pointer registers
	unsigned int *pc;	// program counter, points to cs, goes up
	unsigned int *sp;	// stack pointer, initially points to end of ss, goes down
	unsigned int bp;	// base pointer, initially len_ds+len_ss/2

	unsigned int *min_sp;
	unsigned int *max_sp;
	unsigned int min_bp;
	unsigned int max_bp;

// status
	unsigned int len_cs;	// size of cs
	unsigned int len_ds;	// size of ds
	unsigned int len_ss;	// size of ss
	unsigned int ds_mask; // ds mask (ds+ss)

// memory
	unsigned int mem_size;
	qbyte *mem_ptr;

//	unsigned int cycles;	// command cicles executed
	sys_callqvm_t syscall;
} qvm_t;

qboolean QVM_LoadVM(vm_t *vm, const char *name, sys_callqvm_t syscall);
void QVM_UnLoadVM(qvm_t *qvm);
int QVM_ExecVM(qvm_t *qvm, int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7);


// ------------------------- * OP.CODES * -------------------------

typedef enum qvm_op_e
{
	OP_UNDEF,
	OP_NOP,
	OP_BREAK,

	OP_ENTER, // b32
	OP_LEAVE,	// b32
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,	// b32
	OP_LOCAL,	// b32

	OP_JUMP,

// -------------------

	OP_EQ,	// b32
	OP_NE,	// b32

	OP_LTI,	// b32
	OP_LEI,	// b32
	OP_GTI,	// b32
	OP_GEI,	// b32

	OP_LTU,	// b32
	OP_LEU,	// b32
	OP_GTU,	// b32
	OP_GEU,	// b32

	OP_EQF,	// b32
	OP_NEF,	// b32

	OP_LTF,	// b32
	OP_LEF,	// b32
	OP_GTF,	// b32
	OP_GEF,	// b32

// -------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,
	OP_ARG,	// b8
	OP_BLOCK_COPY,	// b32

//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} qvm_op_t;









// ------------------------- * Init & ShutDown * -------------------------

/*
** QVM_Load
*/
qboolean QVM_LoadVM(vm_t *vm, const char *name, sys_callqvm_t syscall)
{
	char path[MAX_QPATH];
	vmHeader_t header, *srcheader;
	qvm_t *qvm;
	qbyte *raw;
	int n;
	unsigned int i;

	Q_snprintfz(path, sizeof(path), "%s.qvm", name);
	FS_LoadFile(path, (void **)&raw);
// file not found
	if(!raw) return false;
	srcheader=(vmHeader_t*)raw;

	header.vmMagic = LittleLong(srcheader->vmMagic);
	header.instructionCount = LittleLong(srcheader->instructionCount);
	header.codeOffset = LittleLong(srcheader->codeOffset);
	header.codeLength = LittleLong(srcheader->codeLength);
	header.dataOffset = LittleLong(srcheader->dataOffset);
	header.dataLength = LittleLong(srcheader->dataLength);
	header.litLength = LittleLong(srcheader->litLength);
	header.bssLength = LittleLong(srcheader->bssLength);

	if (header.vmMagic==VM_MAGIC2)
	{	//version2 cotains a jump table of sorts
		//it is redundant information and can be ignored
		//its also more useful for jit rather than bytecode
		header.jtrgLength = LittleLong(srcheader->jtrgLength);
	}

// check file
	if( (header.vmMagic!=VM_MAGIC && header.vmMagic!=VM_MAGIC2) || header.instructionCount<=0 || header.codeLength<=0)
	{
		Con_Printf("%s: invalid qvm file\n", name);
		FS_FreeFile(raw);
		return false;
	}

// create vitrual machine
	qvm=Z_Malloc(sizeof(qvm_t));
	qvm->len_cs=header.instructionCount+1;	//bad opcode padding.
	qvm->len_ds=header.dataLength+header.litLength+header.bssLength;
	qvm->len_ss=256*1024;									// 256KB stack space

// memory
	qvm->ds_mask = qvm->len_ds*sizeof(qbyte)+(qvm->len_ss+16*4)*sizeof(qbyte);//+4 for a stack check decrease
	for (i = 0; i < sizeof(qvm->ds_mask)*8-1; i++)
	{
		if ((1<<i) >= qvm->ds_mask)	//is this bit greater than our minimum?
			break;
	}
	qvm->len_ss = (1<<i) - qvm->len_ds*(int)sizeof(qbyte) - 4;	//expand the stack space to fill it.
	qvm->ds_mask = qvm->len_ds*sizeof(qbyte)+(qvm->len_ss+4)*sizeof(qbyte);
	qvm->len_ss -= qvm->len_ss&7;


	qvm->mem_size=qvm->len_cs*sizeof(int)*2 + qvm->ds_mask;
	qvm->mem_ptr=Z_Malloc(qvm->mem_size);
// set pointers
	qvm->cs=(unsigned int*)qvm->mem_ptr;
	qvm->ds=(qbyte*)(qvm->mem_ptr+qvm->len_cs*sizeof(int)*2);
	qvm->ss=(qbyte*)((qbyte*)qvm->ds+qvm->len_ds*sizeof(qbyte));
		//waste 32 bits here.
		//As the opcodes often check stack 0 and 1, with a backwards stack, 1 can leave the stack area. This is where we compensate for it.
// setup registers
	qvm->pc=qvm->cs;
	qvm->sp=(unsigned int*)(qvm->ss+qvm->len_ss);
	qvm->bp=qvm->len_ds+qvm->len_ss/2;
//	qvm->cycles=0;
	qvm->syscall=syscall;

	qvm->ds_mask--;

	qvm->min_sp = (unsigned int*)(qvm->ds+qvm->len_ds+qvm->len_ss/2);
	qvm->max_sp = (unsigned int*)(qvm->ds+qvm->len_ds+qvm->len_ss);
	qvm->min_bp = qvm->len_ds;
	qvm->max_bp = qvm->len_ds+qvm->len_ss/2;

	qvm->bp = qvm->max_bp;

// load instructions
{
	qbyte *src=raw+header.codeOffset;
	int *dst=(int*)qvm->cs;
	int total=header.instructionCount;
	qvm_op_t op;

	for(n=0; n<total; n++)
	{
		op=*src++;
		*dst++=(int)op;
		switch(op)
		{
		case OP_ENTER:
		case OP_LEAVE:
		case OP_CONST:
		case OP_LOCAL:
		case OP_EQ:
		case OP_NE:
		case OP_LTI:
		case OP_LEI:
		case OP_GTI:
		case OP_GEI:
		case OP_LTU:
		case OP_LEU:
		case OP_GTU:
		case OP_GEU:
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
		case OP_BLOCK_COPY:
			*dst++=LittleLong(*(int*)src);
			src+=4;
			break;
		case OP_ARG:
			*dst++=(int)*src++;
			break;
		default:
			*dst++=0;
			break;
		}
	}
	*dst++=OP_BREAK;	//in case someone 'forgot' the return on the last function.
	*dst++=0;
}

// load data segment
{
	int *src=(int*)(raw+header.dataOffset);
	int *dst=(int*)qvm->ds;
	int total=header.dataLength/4;

	for(n=0; n<total; n++)
		*dst++=LittleLong(*src++);

	memcpy(dst, src, header.litLength);
}

	FS_FreeFile(raw);
	Q_strncpyz(vm->filename, path, sizeof(vm->filename));
	vm->hInst = qvm;
	return true;
}

/*
** QVM_UnLoad
*/
void QVM_UnLoadVM(qvm_t *qvm)
{
	Z_Free(qvm->mem_ptr);
	Z_Free(qvm);
}


// ------------------------- * private execution stuff * -------------------------

/*
** QVM_Goto

(inlined this the old fashioned way)
*/
#define QVM_Goto(vm,addr)	\
do{							\
	if(addr<0 || addr>vm->len_cs)	\
		Sys_Error("VM run time error: program jumped off to hyperspace\n");	\
	vm->pc=vm->cs+addr*2;	\
} while(0)

//static void inline QVM_Goto(qvm_t *vm, int addr)
//{
//	if(addr<0 || addr>vm->len_cs)
//		Sys_Error("VM run time error: program jumped off to hyperspace\n");
//	vm->pc=vm->cs+addr*2;
//}

/*
** QVM_Call
**
** calls function
*/
inline static void QVM_Call(qvm_t *vm, int addr)
{
	vm->sp--;
	if (vm->sp < vm->min_sp) Sys_Error("QVM Stack underflow");

	if(addr<0)
	{
	// system trap function
		{
			int *fp;

			fp=(int*)(vm->ds+vm->bp)+2;
			vm->sp[0] = vm->syscall(vm->ds, vm->ds_mask, -addr-1, fp);
			return;
		}
	}

	if(addr>=vm->len_cs)
		Sys_Error("VM run time error: program jumped off to hyperspace\n");

	vm->sp[0]=(vm->pc-vm->cs); // push pc /return address/
	vm->pc=vm->cs+addr*2;
	if (!vm->pc)
		Sys_Error("VM run time error: program called the void\n");
}

/*
** QVM_Enter
**
** [oPC][0][.......]| <- oldBP
** ^BP
*/
inline static void QVM_Enter(qvm_t *vm, int size)
{
	int *fp;

	vm->bp-=size;
	if(vm->bp<vm->min_bp)
		Sys_Error("VM run time error: out of stack\n");

	fp=(int*)(vm->ds+vm->bp);
	fp[0]=vm->sp-vm->max_sp;					// unknown /maybe size/
	fp[1]=*vm->sp++;	// saved PC

	if (vm->sp > vm->max_sp) Sys_Error("QVM Stack overflow");
}

/*
** QVM_Return
** returns failure when returning to the engine.
*/
inline static qboolean QVM_Return(qvm_t *vm, int size)
{
	int *fp;

	fp=(int*)(vm->ds+vm->bp);
	vm->bp+=size;

	if(vm->bp>vm->max_bp)
		Sys_Error("VM run time error: freed too much stack\n");

	if (vm->sp-vm->max_sp != fp[0])
		Sys_Error("VM run time error: stack push/pop mismatch \n");
	vm->pc=vm->cs+fp[1]; // restore PC

	if((unsigned int)fp[1]>=(unsigned int)(vm->len_cs*2))	//explicit casts to make sure the C compiler can't make assumptions about overflows.
	{
		if (fp[1] == -1)
			return false;	//return to engine.
		Sys_Error("VM run time error: program returned to hyperspace (%p, %#x)\n", (char*)vm->cs, fp[1]);
	}
	return true;
}

// ------------------------- * execution * -------------------------

/*
** VM_Exec
*/
int QVM_ExecVM(register qvm_t *qvm, int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
{
//remember that the stack is backwards. push takes 1.

//FIXME: does it matter that our stack pointer (qvm->sp) is backwards compared to q3?
//We are more consistant of course, but this simply isn't what q3 does.

//all stack shifts in this function are referenced through these 2 macros.
#define POP(t)	qvm->sp+=t;if (qvm->sp > qvm->max_sp) Sys_Error("QVM Stack underflow");
#define PUSH(v) qvm->sp--;if (qvm->sp < qvm->min_sp) Sys_Error("QVM Stack overflow");*qvm->sp=v
	qvm_op_t op=-1;
	unsigned int param;

	int *fp;
	unsigned int *oldpc;

	oldpc = qvm->pc;

// setup execution environment
	qvm->pc=qvm->cs-1;
//	qvm->cycles=0;
// prepare local stack
	qvm->bp -= 15*4;	//we have to do this each call for the sake of (reliable) recursion.
	fp=(int*)(qvm->ds+qvm->bp);
// push all params
	fp[0]=0;
	fp[1]=0;
	fp[2]=command;
	fp[3]=arg0;
	fp[4]=arg1;
	fp[5]=arg2;
	fp[6]=arg3;
	fp[7]=arg4;
	fp[8]=arg5;
	fp[9]=arg6;
	fp[10]=arg7;	// arg7;
	fp[11]=0;	// arg8;
	fp[12]=0;	// arg9;
	fp[13]=0;	// arg10;
	fp[14]=0;	// arg11;

	QVM_Call(qvm, 0);

	for(;;)
	{
	// fetch next command
 		op=*qvm->pc++;
		param=*qvm->pc++;
//		qvm->cycles++;

		switch(op)
		{
	// aux
		case OP_UNDEF:
		case OP_NOP:
			break;
		default:
		case OP_BREAK: // break to debugger
			Sys_Error("VM hit an OP_BREAK opcode");
			break;

	// subroutines
		case OP_ENTER:
			QVM_Enter(qvm, param);
			break;
		case OP_LEAVE:
			if (!QVM_Return(qvm, param))
			{
				// pick return value from C stack
				qvm->pc = oldpc;

				qvm->bp += 15*4;
//				if(qvm->bp!=qvm->max_bp)
//					Sys_Error("VM run time error: freed too much stack\n");
				param = qvm->sp[0];
				POP(1);
				return param;
			}
			break;
		case OP_CALL:
			param = *qvm->sp;
			POP(1);
			QVM_Call(qvm, param);
			break;

	// stack
		case OP_PUSH:
			PUSH(*qvm->sp);
			break;
		case OP_POP:
			POP(1);
			break;
		case OP_CONST:
			PUSH(param);
			break;
		case OP_LOCAL:
			PUSH(param+qvm->bp);
			break;

	// branching
		case OP_JUMP:
			param = *qvm->sp;
			POP(1);
			QVM_Goto(qvm, param);
			break;
		case OP_EQ:
			if(qvm->sp[1]==qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_NE:
			if(qvm->sp[1]!=qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTI:
			if(*(signed int*)&qvm->sp[1]<*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEI:
			if(*(signed int*)&qvm->sp[1]<=*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTI:
			if(*(signed int*)&qvm->sp[1]>*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEI:
			if(*(signed int*)&qvm->sp[1]>=*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTU:
			if(*(unsigned int*)&qvm->sp[1]<*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEU:
			if(*(unsigned int*)&qvm->sp[1]<=*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTU:
			if(*(unsigned int*)&qvm->sp[1]>*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEU:
			if(*(unsigned int*)&qvm->sp[1]>=*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_EQF:
			if(*(float*)&qvm->sp[1]==*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_NEF:
			if(*(float*)&qvm->sp[1]!=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTF:
			if(*(float*)&qvm->sp[1]<*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEF:
			if(*(float*)&qvm->sp[1]<=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTF:
			if(*(float*)&qvm->sp[1]>*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEF:
			if(*(float*)&qvm->sp[1]>=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;

	// memory I/O: masks protect main memory
		case OP_LOAD1:
			*(unsigned int*)&qvm->sp[0]=*(unsigned char*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_LOAD2:
			*(unsigned int*)&qvm->sp[0]=*(unsigned short*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_LOAD4:
			*(unsigned int*)&qvm->sp[0]=*(unsigned int*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_STORE1:
			*(qbyte*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=(qbyte)(qvm->sp[0]&0xFF);
			POP(2);
			break;
		case OP_STORE2:
			*(unsigned short*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=(unsigned short)(qvm->sp[0]&0xFFFF);
			POP(2);
			break;
		case OP_STORE4:
			*(unsigned int*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=*(unsigned int*)&qvm->sp[0];
			POP(2);
			break;
		case OP_ARG:
			*(unsigned int*)&qvm->ds[(param+qvm->bp)&qvm->ds_mask]=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BLOCK_COPY:
			if (qvm->sp[1]+param < qvm->ds_mask && qvm->sp[0] + param < qvm->ds_mask)
			{
				memmove(qvm->ds+(qvm->sp[1]&qvm->ds_mask), qvm->ds+(qvm->sp[0]&qvm->ds_mask), param);
			}
			POP(2);
			break;

	// integer arithmetic
		case OP_SEX8:
			if(*(signed int*)&qvm->sp[0]&0x80) *(signed int*)&qvm->sp[0]|=0xFFFFFF00;
			break;
		case OP_SEX16:
			if(*(signed int*)&qvm->sp[0]&0x8000) *(signed int*)&qvm->sp[0]|=0xFFFF0000;
			break;
		case OP_NEGI:
			*(signed int*)&qvm->sp[0]=-*(signed int*)&qvm->sp[0];
			break;
		case OP_ADD:
			*(signed int*)&qvm->sp[1]+=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_SUB:
			*(signed int*)&qvm->sp[1]-=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVI:
			*(signed int*)&qvm->sp[1]/=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVU:
			*(unsigned int*)&qvm->sp[1]/=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;
		case OP_MODI:
			*(signed int*)&qvm->sp[1]%=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MODU:
			*(unsigned int*)&qvm->sp[1]%=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;
		case OP_MULI:
			*(signed int*)&qvm->sp[1]*=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MULU:
			*(unsigned int*)&qvm->sp[1]*=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;

	// logic
		case OP_BAND:
			*(unsigned int*)&qvm->sp[1]&=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BOR:
			*(unsigned int*)&qvm->sp[1]|=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BXOR:
			*(unsigned int*)&qvm->sp[1]^=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BCOM:
			*(unsigned int*)&qvm->sp[0]=~*(unsigned int*)&qvm->sp[0];
			break;
		case OP_LSH:
			*(unsigned int*)&qvm->sp[1]<<=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_RSHI:
			*(signed int*)&qvm->sp[1]>>=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_RSHU:
			*(unsigned int*)&qvm->sp[1]>>=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;

	// floating point arithmetic
		case OP_NEGF:
			*(float*)&qvm->sp[0]=-*(float*)&qvm->sp[0];
			break;
		case OP_ADDF:
			*(float*)&qvm->sp[1]+=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_SUBF:
			*(float*)&qvm->sp[1]-=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVF:
			*(float*)&qvm->sp[1]/=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MULF:
			*(float*)&qvm->sp[1]*=*(float*)&qvm->sp[0];
			POP(1);
			break;

	// format conversion
		case OP_CVIF:
			*(float*)&qvm->sp[0]=(float)(signed int)qvm->sp[0];
			break;
		case OP_CVFI:
			*(signed int*)&qvm->sp[0]=(signed int)(*(float*)&qvm->sp[0]);
			break;
		}
	}
}











// ------------------------- * interface * -------------------------

/*
** VM_PrintInfo
*/
void VM_PrintInfo(vm_t *vm)
{
	qvm_t *qvm;

//	Con_Printf("%s (%p): ", vm->name, vm->hInst);
	Con_Printf("^2%s", vm->filename);

	switch(vm->type)
	{
	case VM_NATIVE:
		Con_Printf(": native\n");
		break;
	case VM_BUILTIN:
		Con_Printf(": built in\n");
		break;

	case VM_BYTECODE:
		Con_Printf(": interpreted\n");
		if((qvm=vm->hInst))
		{
			Con_Printf("  code  length: %d\n", qvm->len_cs);
			Con_Printf("  data  length: %d\n", qvm->len_ds);
			Con_Printf("  stack length: %d\n", qvm->len_ss);
		}
		break;

	default:
		Con_Printf(": unknown\n");
		break;
	}
}

const char *VM_GetFilename(vm_t *vm)
{
	return vm->filename;
}

vm_t *VM_CreateBuiltin(const char *name, sys_calldll_t syscalldll, qintptr_t (*init)(qintptr_t *args))
{
	vm_t *vm = Z_Malloc(sizeof(vm_t));
	Q_strncpyz(vm->filename, name, sizeof(vm->filename));
	vm->syscalldll	= syscalldll;
	vm->syscallqvm	= NULL;
	vm->hInst		= init;
	vm->type		= VM_BUILTIN;
	return vm;
}

/*
** VM_Create
*/
vm_t *VM_Create(const char *dllname, sys_calldll_t syscalldll, const char *qvmname, sys_callqvm_t syscallqvm)
{
	vm_t *vm;

	vm = Z_Malloc(sizeof(vm_t));

// prepare vm struct
	memset(vm, 0, sizeof(vm_t));
	Q_strncpyz(vm->filename, "", sizeof(vm->filename));
	vm->syscalldll=syscalldll;
	vm->syscallqvm=syscallqvm;



	if (syscalldll)
	{
		if (!COM_CheckParm("-nodlls") && !COM_CheckParm("-nosos"))	//:)
		{
			if(QVM_LoadDLL(vm, dllname, !syscallqvm, (void**)&vm->vmMain, syscalldll))
			{
				Con_DPrintf("Creating native machine \"%s\"\n", dllname);
				vm->type=VM_NATIVE;
				return vm;
			}
		}
	}


	if (syscallqvm)
	{
		if(QVM_LoadVM(vm, qvmname, syscallqvm))
		{
			Con_DPrintf("Creating virtual machine \"%s\"\n", qvmname);
			vm->type=VM_BYTECODE;
			return vm;
		}
	}

	Z_Free(vm);
	return NULL;
}

/*
** VM_Destroy
*/
void VM_Destroy(vm_t *vm)
{
	if(!vm) return;

	switch(vm->type)
	{
	case VM_NATIVE:
		if(vm->hInst) QVM_UnloadDLL(vm->hInst);
		break;

	case VM_BYTECODE:
		if(vm->hInst) QVM_UnLoadVM(vm->hInst);
		break;

	case VM_BUILTIN:
	case VM_NONE:
		break;
	}

	Z_Free(vm);
}

/*
** VM_Restart
*/
/*qboolean VM_Restart(vm_t *vm)
{
	char name[MAX_QPATH];
	sys_calldll_t syscalldll;
	sys_callqvm_t syscallqvm;

	if(!vm) return false;

// save params
	Q_strncpyz(name, vm->name, sizeof(name));
	syscalldll=vm->syscalldll;
	syscallqvm=vm->syscallqvm;

// restart
	switch(vm->type)
	{
	case VM_NATIVE:
		if(vm->hInst) QVM_UnloadDLL(vm->hInst);
		break;

	case VM_BYTECODE:
		if(vm->hInst) QVM_UnLoadVM(vm->hInst);
		break;

	case VM_NONE:
		break;
	}

	return VM_Create(vm, name, syscalldll, syscallqvm)!=NULL;
}*/

void *VM_MemoryBase(vm_t *vm)
{
	switch(vm->type)
	{
	case VM_NATIVE:
	case VM_BUILTIN:
		return NULL;
	case VM_BYTECODE:
		return ((qvm_t*)vm->hInst)->ds;
	default:
		return NULL;
	}
}
quintptr_t VM_MemoryMask(vm_t *vm)
{
	switch(vm->type)
	{
	case VM_BYTECODE:
		return ((qvm_t*)vm->hInst)->ds_mask;
	default:
		return ~(quintptr_t)0;
	}
}

/*returns true if we're running a 32bit vm on a 64bit host (in case we need workarounds)*/
qboolean VM_NonNative(vm_t *vm)
{
	switch(vm->type)
	{
	case VM_BYTECODE:
		return sizeof(int) != sizeof(void*);
	case VM_NATIVE:
	case VM_BUILTIN:
		return false;
	default:
		return false;
	}
}

/*
** VM_Call
*/
qintptr_t VARGS VM_Call(vm_t *vm, qintptr_t instruction, ...)
{
	va_list argptr;
	qintptr_t arg[8];

	if(!vm) Sys_Error("VM_Call with NULL vm");

	va_start(argptr, instruction);
	arg[0]=va_arg(argptr, qintptr_t);
	arg[1]=va_arg(argptr, qintptr_t);
	arg[2]=va_arg(argptr, qintptr_t);
	arg[3]=va_arg(argptr, qintptr_t);
	arg[4]=va_arg(argptr, qintptr_t);
	arg[5]=va_arg(argptr, qintptr_t);
	arg[6]=va_arg(argptr, qintptr_t);
	arg[7]=va_arg(argptr, qintptr_t);
	va_end(argptr);

	switch(vm->type)
	{
	case VM_NATIVE:
		return vm->vmMain(instruction, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]);

	case VM_BYTECODE:
		return QVM_ExecVM(vm->hInst, instruction, arg[0]&0xffffffff, arg[1]&0xffffffff, arg[2]&0xffffffff, arg[3]&0xffffffff, arg[4]&0xffffffff, arg[5]&0xffffffff, arg[6]&0xffffffff, arg[7]&0xffffffff);

	case VM_BUILTIN:
		if (!instruction)
			instruction = (qintptr_t)vm->hInst;
		return ((qintptr_t(*)(qintptr_t*))instruction)(arg);

	case VM_NONE:
		return 0;
	}
	return 0;
}

#endif
