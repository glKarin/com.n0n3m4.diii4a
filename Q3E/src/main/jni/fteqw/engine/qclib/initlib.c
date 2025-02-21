#define PROGSUSED
#include "progsint.h"
#include <stdlib.h>

static void PR_FreeAllTemps			(progfuncs_t *progfuncs);

typedef struct prmemb_s {
	struct prmemb_s *prev;
	int level;
} prmemb_t;
void *PRHunkAlloc(progfuncs_t *progfuncs, int ammount, const char *name)
{
	prmemb_t *mem;
	ammount = sizeof(prmemb_t)+((ammount + 3)&~3);
	mem = progfuncs->funcs.parms->memalloc(ammount); 
	memset(mem, 0, ammount);
	mem->prev = prinst.memblocks;
	if (!prinst.memblocks)
		mem->level = 1;
	else
		mem->level = ((prmemb_t *)prinst.memblocks)->level+1;
	prinst.memblocks = mem;

	return ((char *)mem)+sizeof(prmemb_t);
}
static void *PDECL QC_HunkAlloc(pubprogfuncs_t *ppf, int ammount, char *name)
{
	return PRHunkAlloc((progfuncs_t*)ppf, ammount, name);
}

int PRHunkMark(progfuncs_t *progfuncs)
{
	return ((prmemb_t *)prinst.memblocks)->level;
}
void PRHunkFree(progfuncs_t *progfuncs, int mark)
{
	prmemb_t *omem;
	while(prinst.memblocks)
	{
		if (prinst.memblocks->level <= mark)
			return;

		omem = prinst.memblocks;
		prinst.memblocks = prinst.memblocks->prev;
		externs->memfree(omem);
	}
	return;
}

/*if we ran out of memory, the vm can allocate a new block, but doing so requires fixing up all sorts of pointers*/
static void PRAddressableRelocate(progfuncs_t *progfuncs, char *oldb, char *newb, int oldlen)
{
	unsigned int i;
	edictrun_t *e;
	for (i=0 ; i<prinst.maxedicts; i++)
	{
		e = (edictrun_t *)(prinst.edicttable[i]);
		if (e && (char*)e->fields >= oldb && (char*)e->fields < oldb+oldlen)
			e->fields = ((char*)e->fields - oldb) + newb;
	}

	if (progfuncs->funcs.stringtable >= oldb && progfuncs->funcs.stringtable < oldb+oldlen)
		progfuncs->funcs.stringtable = (progfuncs->funcs.stringtable - oldb) + newb;

	for (i=0; i < prinst.maxprogs; i++)
	{
		if ((char*)prinst.progstate[i].globals >= oldb && (char*)prinst.progstate[i].globals < oldb+oldlen)
			prinst.progstate[i].globals = (float*)(((char*)prinst.progstate[i].globals - oldb) + newb);
		if (prinst.progstate[i].strings >= oldb && prinst.progstate[i].strings < oldb+oldlen)
			prinst.progstate[i].strings = (prinst.progstate[i].strings - oldb) + newb;
	}

	for (i = 0; i < prinst.numfields; i++)
	{
		if (prinst.field[i].name >= oldb && prinst.field[i].name < oldb+oldlen)
			prinst.field[i].name = (prinst.field[i].name - oldb) + newb;
	}

	externs->addressablerelocated(&progfuncs->funcs, oldb, newb, oldlen);
}

//for 64bit systems. :)
//addressable memory is memory available to the vm itself for writing.
//once allocated, it cannot be freed for the lifetime of the VM.
//if src is null, data srcsize is left uninitialised for speed.
//pad is always 0-filled.
void *PRAddressableExtend(progfuncs_t *progfuncs, void *src, size_t srcsize, int pad)
{
	char *ptr;
	int ammount = (srcsize+pad + 4)&~3;	//round up to 4
	pad = ammount - srcsize;
	pad++;	//make sure there's always a null, to allow strings to be a little more lazy.
	if (prinst.addressableused + ammount >= prinst.addressablesize)
	{
		/*only do this if the caller states that it can cope with addressable-block relocations/resizes*/
		if (externs->addressablerelocated)
		{
#if defined(_WIN32) && !defined(WINRT)
			char *newblock;
		#if 0//def _DEBUG
			int oldtot = addressablesize;
		#endif
			int newsize = (prinst.addressableused + ammount + 4096) & ~(4096-1);
			newblock = VirtualAlloc (NULL, prinst.addressablesize, MEM_RESERVE, PAGE_NOACCESS);
			if (newblock)
			{
				VirtualAlloc (newblock, prinst.addressableused, MEM_COMMIT, PAGE_READWRITE);
				memcpy(newblock, prinst.addressablehunk, prinst.addressableused);
		#if 0//def _DEBUG
				VirtualAlloc (prinst.addressablehunk, oldtot, MEM_RESERVE, PAGE_NOACCESS);
		#else
				VirtualFree (prinst.addressablehunk, 0, MEM_RELEASE);
		#endif
				PRAddressableRelocate(progfuncs, prinst.addressablehunk, newblock, prinst.addressableused);
				prinst.addressablehunk = newblock;
				prinst.addressablesize = newsize;
			}
#else
			int newsize = (prinst.addressableused + ammount + 1024*1024) & ~(1024*1024-1);
			char *newblock = malloc(newsize);
			if (newblock)
			{
				PRAddressableRelocate(progfuncs, prinst.addressablehunk, newblock, prinst.addressableused);
				free(prinst.addressablehunk);
				prinst.addressablehunk = newblock;
				prinst.addressablesize = newsize;
			}
#endif
		}

		if (prinst.addressableused + ammount >= prinst.addressablesize)
			externs->Sys_Error("Not enough addressable memory for progs VM (using %gmb)", prinst.addressablesize/(1024.0*1024));
	}

	prinst.addressableused += ammount;
	progfuncs->funcs.stringtablesize = prinst.addressableused;

#if defined(_WIN32) && !defined(WINRT)
	if (!VirtualAlloc (prinst.addressablehunk, prinst.addressableused+1, MEM_COMMIT, PAGE_READWRITE))
		externs->Sys_Error("VirtualAlloc failed. Blame windows.");
#endif

	ptr = &prinst.addressablehunk[prinst.addressableused-ammount];
	if (src)
		memcpy(ptr, src, srcsize);
#ifdef _DEBUG
	else
		memset(ptr, 0xcc, srcsize);
#endif
	memset(ptr+srcsize, 0, pad);
	return &prinst.addressablehunk[prinst.addressableused-ammount];
}


#define MARKER_USED 0xC2A4F5A6u
#define MARKER_FREE 0xF1E3E3E7u
typedef struct
{
#ifdef _DEBUG
	unsigned int marker;
#endif
	unsigned int next;
	unsigned int prev;
	unsigned int size;	//includes header size
} qcmemfreeblock_t;
typedef struct
{
	unsigned int marker;
#ifdef _DEBUG
	unsigned int next;
	unsigned int prev;
#endif
	unsigned int size;	//includes header size
} qcmemusedblock_t;
static void PF_fmem_unlink(progfuncs_t *progfuncs, qcmemfreeblock_t *p)
{
	qcmemfreeblock_t *np;
#ifdef _DEBUG
	if (p->marker != MARKER_FREE)
	{
		externs->Printf("PF_fmem_unlink: memory corruption\n");
		PR_StackTrace(&progfuncs->funcs, false);
	}
	p->marker = 0;
#endif
	if (p->prev)
	{
		np = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + p->prev);
		np->next = p->next;
	}
	else
		progfuncs->inst.mfreelist = p->next;
	if (p->next)
	{
		np = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + p->next);
		np->prev = p->prev;
	}
}

void PR_memvalidate (progfuncs_t *progfuncs)
{
	qcmemfreeblock_t *p;
	unsigned int b,l;

	b = prinst.mfreelist;
	l = 0;
	while (b)
	{
		if ((size_t)b >= (size_t)prinst.addressableused)
		{
			externs->Printf("PF_memalloc: memory corruption\n");
			PR_StackTrace(&progfuncs->funcs, false);
			return;
		}
		p = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + b);

		if (
#ifdef _DEBUG
			p->marker != MARKER_FREE ||
#endif
			p->prev != l ||
			(p->next && p->next < b + p->size) ||
			p->next >= prinst.addressableused ||
			b + p->size >= prinst.addressableused ||
			p->prev >= b)
		{
			externs->Printf("PF_memalloc: memory corruption\n");
			PR_StackTrace(&progfuncs->funcs, false);
			return;
		}
		l = b;
		b = p->next;
	}
}

static void *PDECL PR_memalloc (pubprogfuncs_t *ppf, unsigned int size)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	qcmemfreeblock_t *p, *np;
	qcmemusedblock_t *ub = NULL;
	unsigned int b,n;
	/*round size up*/
	size = (size+sizeof(qcmemusedblock_t) + 63) & ~63;

	PR_memvalidate(progfuncs);

	b = prinst.mfreelist;
	while (b)
	{
		if (/*b < 0 || */b+sizeof(qcmemfreeblock_t) >= prinst.addressableused)
		{
			externs->Printf("PF_memalloc: memory corruption\n");
			PR_StackTrace(&progfuncs->funcs, false);
			return NULL;
		}
		p = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + b);
		if (p->size >= size)
		{
			if ((p->next && p->next < b + p->size) ||
				p->next >= prinst.addressableused ||
				b + p->size >= prinst.addressableused ||
				p->prev >= b)
			{
				externs->Printf("PF_memalloc: memory corruption\n");
				PR_StackTrace(&progfuncs->funcs, false);
				return NULL;
			}

			ub = (qcmemusedblock_t*)p;
			if (p->size > size + 63)
			{
				/*make a new header just after it, with basically the same properties, and shift the important fields over*/
				n = b + size;
				np = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + b + size);
#ifdef _DEBUG
				np->marker = MARKER_FREE;
#endif
				np->prev = p->prev;
				np->next = p->next;
				np->size = p->size - size;
				if (np->prev)
				{
					p = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + np->prev);
					p->next = n;
				}
				else
					prinst.mfreelist = n;
				if (p->next)
				{
					p = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + np->next);
					p->prev = n;
				}
			}
			else
			{
				size = p->size; /*alloc the entire block*/
				/*unlink this entry*/
				PF_fmem_unlink(progfuncs, p);
			}
			break;
		}
		b = p->next;
	}

	/*assign more space*/
	if (!ub)
	{
		ub = PRAddressableExtend(progfuncs, NULL, size, 0);
		if (!ub)
		{
			externs->Printf("PF_memalloc: memory exausted\n");
			PR_StackTrace(&progfuncs->funcs, false);
			return NULL;
		}
		//FIXME: merge with previous block
	}
	memset(ub, 0, size);
	ub->marker = MARKER_USED;
	ub->size = size;

	PR_memvalidate(progfuncs);

	return ub+1;
}
static void PDECL PR_memfree (pubprogfuncs_t *ppf, void *memptr)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	qcmemusedblock_t *ub;
	qcmemfreeblock_t *b, *nb, *pb; 
	unsigned int pa, na;	//prev addr, next addr
	unsigned int size;
	unsigned int ptr = memptr?((char*)memptr - progfuncs->funcs.stringtable):0;

	/*freeing NULL is ignored*/
	if (!ptr)
		return;
	PR_memvalidate(progfuncs);
	ptr -= sizeof(qcmemusedblock_t);
	if (/*ptr < 0 ||*/ ptr >= prinst.addressableused)
	{
		ptr += sizeof(qcmemusedblock_t);
		if (ptr < prinst.addressableused && !*(char*)memptr)
		{
			//the empty string is a point of contention. while we can detect it from fteqcc, its best to not give any special favours (other than nicer debugging, where possible)
			//we might not actually spot it from other qccs, so warning about it where possible is probably a very good thing.
			externs->Printf("PF_memfree: unable to free the non-null empty string constant at %x\n", ptr);
		}
		else
			externs->Printf("PF_memfree: pointer invalid - out of range (%x >= %x)\n", ptr, (unsigned int)prinst.addressableused);
		PR_StackTrace(&progfuncs->funcs, false);
		return;
	}

	//this is the used block that we're trying to free
	ub = (qcmemusedblock_t*)(progfuncs->funcs.stringtable + ptr);
	if (ub->marker != MARKER_USED || ub->size <= sizeof(*ub) || ptr + ub->size > (unsigned int)prinst.addressableused)
	{
		externs->Printf("PR_memfree: pointer lacks marker - double-freed?\n");
		PR_StackTrace(&progfuncs->funcs, false);
		return;
	}
	ub->marker = 0;	//invalidate it
	size = ub->size;
	ub = NULL;

	//we have an (ordered) list of free blocks.
	//in order to free our memory, we need to find the free block before+after the 'new' block
	for (na = prinst.mfreelist, pa = 0; ;)
	{
		if (/*na < 0 ||*/ na >= prinst.addressableused)
		{
			externs->Printf("PF_memfree: memory corruption\n");
			PR_StackTrace(&progfuncs->funcs, false);
			return;
		}
		if (!na || na >= ptr)
		{
			pb = pa?(qcmemfreeblock_t*)(progfuncs->funcs.stringtable + pa):NULL;
			if (pb && pa+pb->size>ptr)
			{	//previous free block extends into the block that we're trying to free.
				externs->Printf("PF_memfree: double free\n");
				PR_StackTrace(&progfuncs->funcs, false);
				return;
			}
#ifdef _DEBUG
			if (pb && pb->marker != MARKER_FREE)
			{
				externs->Printf("PF_memfree: use-after-free?\n");
				PR_StackTrace(&progfuncs->funcs, false);
				return;
			}
#endif

			nb = na?(qcmemfreeblock_t*)(progfuncs->funcs.stringtable + na):NULL;
			if (nb && ptr+size > na)
			{
				externs->Printf("PF_memfree: block extends into neighbour\n");
				PR_StackTrace(&progfuncs->funcs, false);
				return;
			}
#ifdef _DEBUG
			if (nb && nb->marker != MARKER_FREE)
			{
				externs->Printf("PF_memfree: use-after-free?\n");
				PR_StackTrace(&progfuncs->funcs, false);
				return;
			}
#endif

			/*generate the free block, now we know its proper values*/
			b = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + ptr);
#ifdef _DEBUG
			b->marker = MARKER_FREE;
#endif
			b->prev = pa;
			b->next = na;
			b->size = size;
			if (na)
				nb->prev = ptr;
			if (!pa)
				prinst.mfreelist = ptr;
			else
				pb->next = ptr;

			/*extend this block and kill the next if they are adjacent*/
			if (na && b->next == ptr + size)
			{
				b->size += nb->size; 
				PF_fmem_unlink(progfuncs, nb);
			}
			/*we're adjacent to the previous block, so merge them by killing the newly freed region*/
			if (pa && pa + pb->size == ptr)
			{
				pb->size += size;
				PF_fmem_unlink(progfuncs, b);
			}
			break;
		}

		pa = na;
		b = (qcmemfreeblock_t*)(progfuncs->funcs.stringtable + pa);
		na = b->next;
	}

	PR_memvalidate(progfuncs);
}

static void *PDECL PR_memrealloc (pubprogfuncs_t *ppf, void *memptr, unsigned int newsize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	qcmemusedblock_t *ub;
	unsigned int ptr = memptr?((char*)memptr - progfuncs->funcs.stringtable):0;
	void *newptr;
	unsigned int oldsize;

	/*freeing NULL is ignored*/
	if (!ptr)	//realloc instead of malloc is accepted.
		return PR_memalloc(ppf, newsize);
	PR_memvalidate(progfuncs);
	ptr -= sizeof(qcmemusedblock_t);
	if (/*ptr < 0 ||*/ ptr >= prinst.addressableused)
	{
		ptr += sizeof(qcmemusedblock_t);
		if (ptr < prinst.addressableused && !*(char*)memptr)
		{
			//the empty string is a point of contention. while we can detect it from fteqcc, its best to not give any special favours (other than nicer debugging, where possible)
			//we might not actually spot it from other qccs, so warning about it where possible is probably a very good thing.
			externs->Printf("PR_memrealloc: unable to free the non-null empty string constant at %x\n", ptr);
		}
		else
			externs->Printf("PR_memrealloc: pointer invalid - out of range (%x >= %x)\n", ptr, (unsigned int)prinst.addressableused);
		PR_StackTrace(&progfuncs->funcs, false);
		return NULL;
	}

	//this is the used block that we're trying to free
	ub = (qcmemusedblock_t*)(progfuncs->funcs.stringtable + ptr);
	if (ub->marker != MARKER_USED || ub->size <= sizeof(*ub) || ptr + ub->size > (unsigned int)prinst.addressableused)
	{
		externs->Printf("PR_memrealloc: pointer lacks marker - double-freed?\n");
		PR_StackTrace(&progfuncs->funcs, false);
		return NULL;
	}
	oldsize = ub->size;
	oldsize -= sizeof(qcmemusedblock_t);	//ignore the header.

	newptr = PR_memalloc(ppf, newsize);
	if (oldsize > newsize)
		oldsize = newsize;		//don't copy it all.
	memcpy(newptr, memptr, oldsize);
	newsize -= oldsize;
	memset((char*)newptr+oldsize, 0, newsize);	//clear out any extended part.
	PR_memfree(ppf, memptr);	//free the old.

	return newptr;
}

void PRAddressableFlush(progfuncs_t *progfuncs, size_t totalammount)
{
	prinst.addressableused = 0;
	prinst.mfreelist = 0;

	if (totalammount <= 0)	//flush
	{
		totalammount = prinst.addressablesize;
//		return;
	}

#if defined(_WIN32) && !defined(WINRT)
	if (prinst.addressablehunk && prinst.addressablesize != totalammount)
	{
		VirtualFree(prinst.addressablehunk, 0, MEM_RELEASE);	//doesn't this look complicated? :p
		prinst.addressablehunk = NULL;
	}
	if (!prinst.addressablehunk)
		prinst.addressablehunk = VirtualAlloc (prinst.addressablehunk, totalammount, MEM_RESERVE, PAGE_NOACCESS);
#else
	if (prinst.addressablehunk && prinst.addressablesize != totalammount)
	{
		free(prinst.addressablehunk);
		prinst.addressablehunk = NULL;
	}
	if (!prinst.addressablehunk)
		prinst.addressablehunk = malloc(totalammount);	//linux will allocate-on-use anyway, which is handy.
//	memset(prinst.addressablehunk, 0xff, totalammount);
#endif
	if (!prinst.addressablehunk)
		externs->Sys_Error("Out of memory\n");
	prinst.addressablesize = totalammount;
	progfuncs->funcs.stringtablemaxsize = totalammount;
}

int PDECL PR_InitEnts(pubprogfuncs_t *ppf, int max_ents)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *e;
	prinst.maxedicts = max_ents;

	sv_num_edicts = 0;

#if 0
	{
		int i;
		for (i = 0; i < prinst.numfields; i++)
		{
			externs->Printf("%s(%i) %i -> %i\n", prinst.field[i].name, prinst.field[i].type, prinst.field[i].progsofs, prinst.field[i].ofs);
		}
	}
#endif

	prinst.max_fields_size = prinst.fields_size;

	prinst.edicttable = (struct edictrun_s**)(progfuncs->funcs.edicttable = PRHunkAlloc(progfuncs, prinst.maxedicts*sizeof(struct edicts_s *), "edicttable"));
	progfuncs->funcs.edicttable_length = prinst.maxedicts;
	e = PRHunkAlloc(progfuncs, externs->edictsize, "edict0");
	e->fieldsize = prinst.fields_size;
	e->entnum = 0;
	e->ereftype = ER_ENTITY;
	sv_edicts = (struct edict_s *)e;
	sv_num_edicts = 1;
	progfuncs->funcs.edicttable[0] = sv_edicts;
	e->fields = PRAddressableExtend(progfuncs, NULL, e->fieldsize, prinst.max_fields_size-e->fieldsize);
	QC_ClearEdict(&progfuncs->funcs, sv_edicts);

	if (externs->entspawn)
		externs->entspawn(sv_edicts, false);

	return prinst.max_fields_size;
}
edictrun_t tempedict={ER_FREE};	//used as a safty buffer
static float tempedictfields[2048];

static void PDECL PR_Configure (pubprogfuncs_t *ppf, size_t addressable_size, int max_progs, pbool profiling)	//can be used to wipe all memory
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	unsigned int i;
	edictrun_t *e;

	prinst.max_fields_size=0;
	prinst.fields_size = 0;
	progfuncs->funcs.stringtable = 0;
	QC_StartShares(progfuncs);
	QC_InitShares(progfuncs);

	for ( i=1 ; i<prinst.maxedicts; i++)
	{
		e = (edictrun_t *)(prinst.edicttable[i]);
		prinst.edicttable[i] = NULL;
//		e->entnum = i;
		if (e)
			externs->memfree(e);
	}

	PRHunkFree(progfuncs, 0);	//clear mem - our hunk may not be a real hunk.
	if (addressable_size == (size_t)-1)
	{
#if defined(_WIN64) && !defined(WINRT)
		addressable_size = 0x80000000;	//use of virtual address space rather than physical memory means we can just go crazy and use the max of 2gb.
#elif defined(FTE_TARGET_WEB)
		addressable_size = 8*1024*1024;
#else
		addressable_size = 32*1024*1024;
#endif
	}
	if (addressable_size > 0x80000000)
		addressable_size = 0x80000000;
	PRAddressableFlush(progfuncs, addressable_size);	
	progfuncs->funcs.stringtable = prinst.addressablehunk;

	pr_progstate = PRHunkAlloc(progfuncs, sizeof(progstate_t) * max_progs, "progstatetable");

/*		for(a = 0; a < max_progs; a++)
		{
			pr_progstate[a].progs = NULL;
		}		
*/
		
	prinst.maxprogs = max_progs;
	prinst.pr_typecurrent=-1;

	PR_FreeAllTemps(progfuncs);

	prinst.reorganisefields = false;

	prinst.profiling = profiling;
	prinst.profilingalert = Sys_GetClockRate();
	progfuncs->funcs.edicttable_length = prinst.maxedicts = 0;
	prinst.edicttable = (edictrun_t**)(progfuncs->funcs.edicttable = &sv_edicts);
	sv_num_edicts = 0;	//set up a safty buffer so things won't go horribly wrong too often
	sv_edicts=(struct edict_s *)&tempedict;
	tempedict.readonly = true;
	tempedict.fields = tempedictfields;
	tempedict.ereftype = ER_OBJECT;
}



static struct globalvars_s *PDECL PR_globals (pubprogfuncs_t *ppf, progsnum_t pnum)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	if (pnum < 0)
	{
		if (!current_progstate)
		{
			static float fallback[RESERVED_OFS];
			return (struct globalvars_s *)fallback;	//err.. you've not loaded one yet.
		}
		return (struct globalvars_s *)current_progstate->globals;
	}
	return (struct globalvars_s *)pr_progstate[pnum].globals;
}

static struct entvars_s *PDECL PR_entvars (pubprogfuncs_t *ppf, struct edict_s *ed)
{
//	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	if (((edictrun_t *)ed)->ereftype != ER_ENTITY)
		return NULL;

	return (struct entvars_s *)edvars(ed);
}

static pbool PDECL PR_GetFunctionInfo(pubprogfuncs_t *ppf, func_t func, int *args, pbyte **argsizes, int *builtinnum, char *funcname, size_t funcnamesize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;

	unsigned int pnum;
	unsigned int fnum;
	mfunction_t *f;

	pnum = (func & 0xff000000)>>24;
	fnum = (func & 0x00ffffff);

	if (pnum >= prinst.maxprogs || !pr_progstate[pnum].functions)
		return false;
	else if (fnum >= pr_progstate[pnum].progs->numfunctions)
		return false;
	else
	{
		f = pr_progstate[pnum].functions + fnum;
		if (args)
			*args = f->numparms;
		if (argsizes)
			*argsizes = f->parm_size;
		if (builtinnum)
			*builtinnum = -f->first_statement;
		if (funcname)
		{
			const char *srcname = PR_StringToNative(ppf, f->s_name);
			size_t nlen = strlen(srcname);
			if (nlen < funcnamesize)
				memcpy(funcname, srcname, nlen+1);
			else
				*funcname = 0;
		}
		return true;
	}
}

func_t PDECL PR_FindFunc(pubprogfuncs_t *ppf, const char *funcname, progsnum_t pnum)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	mfunction_t *f=NULL;
	if (pnum == PR_ANY)
	{
		for (pnum = 0; (unsigned)pnum < prinst.maxprogs; pnum++)
		{
			if (!pr_progstate[pnum].progs)
				continue;
			f = ED_FindFunction(progfuncs, funcname, &pnum, pnum);
			if (f)
				break;
		}
	}
	else if (pnum == PR_ANYBACK)	//run backwards
	{
		for (pnum = prinst.maxprogs-1; pnum >= 0; pnum--)
		{
			if (!pr_progstate[pnum].progs)
				continue;
			f = ED_FindFunction(progfuncs, funcname, &pnum, pnum);
			if (f)
				break;
		}
	}
	else
		f = ED_FindFunction(progfuncs, funcname, &pnum, pnum);
	if (!f)
		return 0;

	{
	ddef16_t *var16;
	ddef32_t *var32;
	progstate_t *ps = &pr_progstate[pnum];
	switch(ps->structtype)
	{
	case PST_KKQWSV:
	case PST_DEFAULT:
		var16 = ED_FindTypeGlobalFromProgs16(progfuncs, ps, funcname, ev_function);	//we must make sure we actually have a function def - 'light' is defined as a field before it is defined as a function.
		if (!var16)
			return (f - ps->functions) | (pnum << 24);
		return *(int *)&ps->globals[var16->ofs];
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		var32 = ED_FindTypeGlobalFromProgs32(progfuncs, ps, funcname, ev_function);	//we must make sure we actually have a function def - 'light' is defined as a field before it is defined as a function.
		if (!var32)
			return (f - ps->functions) | (pnum << 24);
		return *(int *)&ps->globals[var32->ofs];
	}
	externs->Sys_Error("Error with def size (PR_FindFunc)");
	}
	return 0;
}

static void PDECL QC_FindPrefixedGlobals(pubprogfuncs_t *ppf, int pnum, char *prefix, void (PDECL *found) (pubprogfuncs_t *progfuncs, char *name, union eval_s *val, etype_t type, void *ctx), void *ctx)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	unsigned int i;
	ddef16_t		*def16;
	ddef32_t		*def32;
	int len = strlen(prefix);

	if (pnum == PR_CURRENT)
		pnum = prinst.pr_typecurrent;
	if (pnum == PR_ANY)
	{
		for (pnum = 0; (unsigned)pnum < prinst.maxprogs; pnum++)
		{
			if (!pr_progstate[pnum].progs)
				continue;
			QC_FindPrefixedGlobals(ppf, pnum, prefix, found, ctx);
		}
		return;
	}

	if (!pr_progstate[pnum].progs)
		return;

	switch(pr_progstate[pnum].structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		for (i=1 ; i<pr_progstate[pnum].progs->numglobaldefs ; i++)
		{
			def16 = &pr_progstate[pnum].globaldefs16[i];
			if (!strncmp(def16->s_name+progfuncs->funcs.stringtable,prefix, len))
				found(&progfuncs->funcs, def16->s_name+progfuncs->funcs.stringtable, (eval_t *)&pr_progstate[pnum].globals[def16->ofs], def16->type, ctx);
		}
		break;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		for (i=1 ; i<pr_progstate[pnum].progs->numglobaldefs ; i++)
		{
			def32 = &pr_progstate[pnum].globaldefs32[i];
			if (!strncmp(def32->s_name+progfuncs->funcs.stringtable,prefix, len))
				found(&progfuncs->funcs, def32->s_name+progfuncs->funcs.stringtable, (eval_t *)&pr_progstate[pnum].globals[def32->ofs], def32->type, ctx);
		}
		break;
	}
}
static pbool	PDECL PR_FindBuiltins	(pubprogfuncs_t *ppf, progsnum_t prnum, int binum, pbool (PDECL *found) (pubprogfuncs_t *progfuncs, const char *name, void *ctx), void *ctx)	//calls the callback for each function reference that's mapped to the specified builtin number.
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	mfunction_t		*func;
	unsigned int				i;

	if ((unsigned)prnum > (unsigned)prinst.maxprogs)
	{
		externs->Printf("Progsnum %"pPRIi" out of bounds\n", prnum);
		return false;
	}

	if (!pr_progstate[prnum].progs)
		return false;

	if (binum < 0)
		return false;	//invalid
	binum = -binum;

	for (i=1 ; i<pr_progstate[prnum].progs->numfunctions ; i++)
	{
		func = &pr_progstate[prnum].functions[i];
		if (func->first_statement == binum)
			if (!found(ppf, PR_StringToNative(ppf, func->s_name), ctx))
				return false;
	}
	return true;
}

eval_t *PDECL PR_FindGlobal(pubprogfuncs_t *ppf, const char *globname, progsnum_t pnum, etype_t *type)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	unsigned int i;
	ddef16_t *var16;
	ddef32_t *var32;
	progstate_t *cp;
	if (type)
		*type = ev_void;
	if (pnum == PR_CURRENT && current_progstate)
		cp = current_progstate;
	else if (pnum == PR_ANY)
	{
		eval_t *ev;
		for (i = 0; i < prinst.maxprogs; i++)
		{
			if (!pr_progstate[i].progs)
				continue;
			ev = PR_FindGlobal(&progfuncs->funcs, globname, i, type);
			if (ev)
				return ev;
		}
		return NULL;
	}
	else if (pnum >= 0 && (unsigned)pnum < prinst.maxprogs && pr_progstate[pnum].progs)
		cp = &pr_progstate[pnum];
	else
		return NULL;
	switch(cp->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		if (!(var16 = ED_FindGlobalFromProgs16(progfuncs, cp, globname)))
			return NULL;

		if (type)
			*type = var16->type;
		return (eval_t *)&cp->globals[var16->ofs];
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		if (!(var32 = ED_FindGlobalFromProgs32(progfuncs, cp, globname)))
			return NULL;

		if (type)
			*type = var32->type;
		return (eval_t *)&cp->globals[var32->ofs];
	}
	externs->Sys_Error("Error with def size (PR_FindGlobal)");
	return NULL;
}

static char *PDECL PR_VarString (pubprogfuncs_t *ppf, int	first)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	int		i;
	static char out[1024];
	const char *s;

	out[0] = 0;
	for (i=first ; i<progfuncs->funcs.callargc ; i++)
	{
		s = PR_StringToNative(ppf, G_STRING(OFS_PARM0+i*3));
		if (s)
		{
			if (strlen(out) + strlen(s) + 1 >= sizeof(out))
				return out;
			strcat (out, s);
		}
	}
	return out;
}

static int PDECL PR_QueryField (pubprogfuncs_t *ppf, unsigned int fieldoffset, etype_t *type, char const**name, evalc_t *fieldcache)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	fdef_t *var;
	var = ED_FieldAtOfs(progfuncs, fieldoffset);
	if (!var)
		return false;

	if (type)
		*type = var->type & ~(DEF_SAVEGLOBAL|DEF_SHARED);
	if (name)
		*name = var->name;
	if (fieldcache)
	{
		fieldcache->ofs32 = var;
		fieldcache->varname = var->name;
	}
		
	return true;
}

eval_t *PDECL QC_GetEdictFieldValue(pubprogfuncs_t *ppf, struct edict_s *ed, const char *name, etype_t type, evalc_t *cache)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	fdef_t *var;
	if (!cache)
	{
		var = ED_FindField(progfuncs, name);
		if (!var || (var->type != type && type))
			return NULL;
		return (eval_t *) &(((int*)(((edictrun_t*)ed)->fields))[var->ofs]);
	}
	if (!cache->varname)
	{
		cache->varname = name;
		var = ED_FindField(progfuncs, name);		
		if (!var || (var->type != type && type))
		{
			cache->ofs32 = NULL;
			return NULL;
		}
		cache->ofs32 = var;
		cache->varname = var->name;
		if (!ed)
			return (void*)~0;	//something not null
		return (eval_t *) &(((int*)(((edictrun_t*)ed)->fields))[var->ofs]);
	}
	if (cache->ofs32 == NULL)
		return NULL;
	return (eval_t *) &(((int*)(((edictrun_t*)ed)->fields))[cache->ofs32->ofs]);
}

static struct edict_s *PDECL ProgsToEdict (pubprogfuncs_t *ppf, int progs)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	if ((unsigned)progs >= (unsigned)prinst.maxedicts)
	{
		externs->Printf("Bad entity index %i\n", progs);
		if (prinst.pr_depth)
		{
			PR_StackTrace (ppf, false);
//			progfuncs->funcs.pr_trace += 1;
		}
		progs = 0;
	}
	return (struct edict_s *)PROG_TO_EDICT_PB(progfuncs.inst, progs);
}
static int PDECL EdictToProgs (pubprogfuncs_t *ppf, struct edict_s *ed)
{
//	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	return EDICT_TO_PROG(progfuncs, ed);
}

string_t PDECL PR_StringToProgs			(pubprogfuncs_t *ppf, const char *str)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	char **ntable;
	int i, free=-1;

	if (!str)
		return 0;

	if (str >= progfuncs->funcs.stringtable && str < progfuncs->funcs.stringtable + prinst.addressableused)
		return str - progfuncs->funcs.stringtable;

	for (i = prinst.numallocedstrings-1; i >= 0; i--)
	{
		if (prinst.allocedstrings[i] == str)
			return (string_t)((unsigned int)i | STRING_STATIC);
		if (!prinst.allocedstrings[i])
			free = i;
	}

	if (free != -1)
	{
		i = free;
		prinst.allocedstrings[i] = (char*)str;
		return (string_t)((unsigned int)i | STRING_STATIC);
	}
	if (prinst.numallocedstrings < prinst.maxallocedstrings)
	{
		i = prinst.numallocedstrings++;
		prinst.allocedstrings[i] = (char*)str;
		return (string_t)((unsigned int)i | STRING_STATIC);
	}

	prinst.maxallocedstrings += 1024;
	ntable = progfuncs->funcs.parms->memalloc(sizeof(char*) * prinst.maxallocedstrings); 
	memcpy(ntable, prinst.allocedstrings, sizeof(char*) * prinst.numallocedstrings);
	memset(ntable + prinst.numallocedstrings, 0, sizeof(char*) * (prinst.maxallocedstrings - prinst.numallocedstrings));
	if (prinst.allocedstrings)
		progfuncs->funcs.parms->memfree(prinst.allocedstrings);
	prinst.allocedstrings = ntable;

	i = prinst.numallocedstrings++;
	prinst.allocedstrings[i] = (char*)str;
	return (string_t)((unsigned int)i | STRING_STATIC);
}
//if ed is null, fld points to a global. if str_is_static, then s doesn't need its own memory allocated.
static void PDECL PR_SetStringField(pubprogfuncs_t *progfuncs, struct edict_s *ed, string_t *fld, const char *str, pbool str_is_static)
{
	if (!str)
		*fld = 0;
	else
	{
#ifdef QCGC
		*fld = PR_AllocTempString(progfuncs, str);
#else
		if (!str_is_static)
			str = PR_AddString(progfuncs, str, 0, false);
		*fld = PR_StringToProgs(progfuncs, str);
#endif
	}
}

static char *PDECL PR_RemoveProgsString				(pubprogfuncs_t *ppf, string_t str)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	char *ret;

	//input string is expected to be an allocated string
	//if its a temp, or a constant, just return NULL.
	if (((unsigned int)str & STRING_SPECMASK) == STRING_STATIC)
	{
		int i = str & ~STRING_SPECMASK;
		if (i >= prinst.numallocedstrings)
		{
			PR_RunWarning(&progfuncs->funcs, "invalid static string %x\n", str);
			return NULL;
		}
		if (prinst.allocedstrings[i])
		{
			ret = prinst.allocedstrings[i];
			prinst.allocedstrings[i] = NULL;	//remove it
			return ret;
		}
		else
		{
			PR_RunWarning(&progfuncs->funcs, "invalid static string %x (already free)\n", str);
			return NULL;	//urm, was freed...
		}
	}
	PR_RunWarning(&progfuncs->funcs, "invalid static string %x\n", str);
	return NULL;
}

const char *ASMCALL PR_StringToNative				(pubprogfuncs_t *ppf, string_t str)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	if (((unsigned int)str & STRING_SPECMASK) == STRING_STATIC)
	{
		int i = str & ~STRING_SPECMASK;
		if (i >= prinst.numallocedstrings)
		{	
			if (!progfuncs->funcs.debug_trace)	//don't spam this
				PR_RunWarning(&progfuncs->funcs, "invalid static string %x\n", str);
			return "";
		}
		if (prinst.allocedstrings[i])
			return prinst.allocedstrings[i];
		else
		{
			if (!progfuncs->funcs.debug_trace)
				PR_RunWarning(&progfuncs->funcs, "invalid static string %x\n", str);
			return "";	//urm, was freed...
		}
	}
	if (((unsigned int)str & STRING_SPECMASK) == STRING_TEMP)
	{
		unsigned int i = str & ~STRING_SPECMASK;
		tempstr_t *ts;
		if (i >= prinst.maxtempstrings || !(ts=prinst.tempstrings[i]))
		{
			if (!progfuncs->funcs.debug_trace)
				PR_RunWarning(&progfuncs->funcs, "invalid temp string %x\n", str);
			return "";
		}
		return ts->value;
	}

	if ((unsigned int)str >= (unsigned int)prinst.addressableused)
	{
		if (!progfuncs->funcs.debug_trace)
			PR_RunWarning(&progfuncs->funcs, "invalid string offset %x\n", str);
		return "";
	}
	return progfuncs->funcs.stringtable + str;
}

//guarentees a return value for tempstrings, but requires a small enough data size to do so.
eval_t *PR_GetReadTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize)
{
	static eval_t dummy;	//don't resize anything when reading.
	if (((unsigned int)str & STRING_SPECMASK) == STRING_TEMP)
	{
		unsigned int i = str & ~STRING_SPECMASK;
		tempstr_t *temp;
		if (i < prinst.maxtempstrings && (temp=prinst.tempstrings[i]))
		{
			if (offset + datasize <= temp->size)
				return (eval_t*)(temp->value + offset);
			else if (datasize <= sizeof(dummy))
				return &dummy;
		}
	}
	return NULL;
}
eval_t *PR_GetWriteTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize)
{
	if (((unsigned int)str & STRING_SPECMASK) == STRING_TEMP)
	{
		unsigned int i = str & ~STRING_SPECMASK;
		tempstr_t *temp;
		if (i < prinst.maxtempstrings && (temp=prinst.tempstrings[i]))
		{
			if (offset + datasize >= temp->size)
			{	//access is beyond the current size. expand it.
				unsigned int newsize;
				tempstr_t *newtemp;
				newsize = offset + datasize;
				if (newsize > (1u<<20u))
					return NULL;	//gotta have a cut-off point somewhere.
				newsize = (newsize+sizeof(float)-1)&~(sizeof(float)-1);
				newtemp = progfuncs->funcs.parms->memalloc(sizeof(tempstr_t) - sizeof(((tempstr_t*)NULL)->value) + newsize);
				if (!newtemp)
					return NULL;
				newtemp->size = newsize;
				memcpy(newtemp->value, temp->value, temp->size);
				memset(newtemp->value+temp->size, 0, newsize-temp->size);
				progfuncs->funcs.parms->memfree(temp);
				prinst.tempstrings[i] = temp = newtemp;
			}
			return (eval_t*)(temp->value + offset);
		}
	}
	return NULL;
}

//returns null for invalid accesses. WARNING: invalidates any other pointers to the same tempstring so use this before getting any read pointers (or strings!).
void *PR_PointerToNative_Resize(pubprogfuncs_t *inst, pint_t ptr, size_t offset, size_t datasize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)inst;
	if (((unsigned int)ptr & STRING_SPECMASK) == STRING_TEMP)
	{	//buffer. these auto-upsize.
		unsigned int i = ptr & ~STRING_SPECMASK;
		tempstr_t *temp;
		if (i < prinst.maxtempstrings && (temp=prinst.tempstrings[i]))
		{
			if (datasize > temp->size || offset >= temp->size-datasize)
			{	//access is beyond the current size. expand it.
				unsigned int newsize;
				tempstr_t *newtemp;
				newsize = offset + datasize;
				if (newsize > (1u<<20u))
					return NULL;	//gotta have a cut-off point somewhere.
				newsize = (newsize+sizeof(float)-1)&~(sizeof(float)-1);
				newtemp = progfuncs->funcs.parms->memalloc(sizeof(tempstr_t) - sizeof(((tempstr_t*)NULL)->value) + newsize);
				if (!newtemp)
					return NULL;	//erk!
				newtemp->size = newsize;
				memcpy(newtemp->value, temp->value, temp->size);
				memset(newtemp->value+temp->size, 0, newsize-temp->size);
				progfuncs->funcs.parms->memfree(temp);
				prinst.tempstrings[i] = temp = newtemp;
			}
			return (eval_t*)(temp->value + offset);
		}
		return NULL;	//nothing not allocated.
	}
	else
	{	//regular pointer
		offset += ptr;
		if (datasize > inst->stringtablesize || offset >= inst->stringtablesize-datasize || !offset)
			return NULL;	//can't autoresize these. just fail.
		return inst->stringtable + ptr;
	}
	return NULL;
}
void *PR_PointerToNative_MoInvalidate(pubprogfuncs_t *inst, pint_t ptr, size_t offset, size_t datasize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)inst;
	if (((unsigned int)ptr & STRING_SPECMASK) == STRING_TEMP)
	{	//buffer. these auto-upsize.
		unsigned int i = ptr & ~STRING_SPECMASK;
		tempstr_t *temp;
		if (i < prinst.maxtempstrings && (temp=prinst.tempstrings[i]))
		{
			if (datasize > temp->size || offset >= temp->size-datasize)
			{	//access is beyond the current size. we're not allowed to break any other pointers though, so just fail and let the caller handle that.
				return NULL;	//gotta have a cut-off point somewhere.
			}
			return (eval_t*)(temp->value + offset);
		}
		return NULL;	//nothing not allocated.
	}
	else
	{	//regular pointer
		offset += ptr;
		if (datasize > inst->stringtablesize || offset >= inst->stringtablesize-datasize || (!offset && datasize>1))
			return NULL;	//can't autoresize these. just fail.
		return inst->stringtable + ptr;
	}
	return NULL;
}

void QCBUILTIN PF_memgetval (pubprogfuncs_t *inst, struct globalvars_s *globals)
{
	progfuncs_t *progfuncs = (progfuncs_t*)inst;
	//read 32 bits from a pointer.
	int dst = G_INT(OFS_PARM0);
	float ofs = G_FLOAT(OFS_PARM1);
	int size = sizeof(int);
	if (ofs != (float)(int)ofs)
		PR_RunWarning(inst, "PF_memgetval: non-integer offset\n");
	dst += ofs*size;
	if (dst < 0 || dst+size >= inst->stringtablesize)
	{
		PR_RunError(inst, "PF_memgetval: invalid dest\n");
		return;
	}
	if (dst & 3)
		PR_RunWarning(inst, "PF_memgetval: misaligned pointer (%#x)\n", dst);
	G_INT(OFS_RETURN) = *(int*)(inst->stringtable + dst);
}
void QCBUILTIN PF_memsetval (pubprogfuncs_t *inst, struct globalvars_s *globals)
{
	progfuncs_t *progfuncs = (progfuncs_t*)inst;
	//write 32 bits to a pointer.
	int dst = G_INT(OFS_PARM0);
	float ofs = G_FLOAT(OFS_PARM1);
	int val = G_INT(OFS_PARM2);
	int size = sizeof(int);
	if (ofs != (float)(int)ofs)
		PR_RunWarning(inst, "PF_memsetval: non-integer offset\n");
	dst += ofs*size;
	if (dst < 0 || dst+size >= inst->stringtablesize)
	{
		PR_RunError(inst, "PF_memsetval: invalid dest\n");
		return;
	}
	if (dst & 3)
		PR_RunWarning(inst, "PF_memgetval: misaligned pointer (%#x)\n", dst);
	*(int*)(inst->stringtable + dst) = val;
}

//#define GCTIMINGS

#ifdef QCGC
#define smallbool char
static smallbool *PR_QCGC_Mark(void *mem, size_t memsize, size_t numtemps)
{
	unsigned int *str;	//the reference we're considering
	size_t p;
	smallbool *marked;	//just booleans. could compact.
	marked = malloc(sizeof(*marked) * numtemps);
	memset(marked, 0, sizeof(*marked) * numtemps);

	//mark everything the qc has access to, even if it isn't even a string!
	//note that I did try specifically checking only data explicitly marked as a string type, but that was:
	//a) a smidge slower (lots of extra loops and conditions I guess)
	//b) doesn't work with pointers/structs (yes, we assume it'll all be aligned).
	//c) both methods got the same number of false positives in my test (2, probably dead strunzoned references)
	for (str = mem, p = 0; p < memsize; p+=sizeof(*str), str++)
	{
		if ((*str & STRING_SPECMASK) == STRING_TEMP)
		{
			unsigned int idx = *str &~ STRING_SPECMASK;
			if (idx < numtemps)
				marked[idx] = true;
		}
	}
	return marked;
}
static size_t PR_QCGC_Sweep(progfuncs_t *progfuncs, smallbool *marked, tempstr_t **tempstrings, unsigned int numtemps)
{
	unsigned int p;
	unsigned int swept = 0;
#ifdef GCTIMINGS
	unsigned int unswept = 0;
	unsigned int errors = 0;
#endif
	for (p = 0; p < numtemps; p++)
	{
		if (marked[p])
		{	//still live...
#ifdef GCTIMINGS
			unswept++;

			if (!tempstrings[p])
				errors++;
#endif
		}
		else if (tempstrings[p])
		{	//not marked, but was valid at the time our snapshot was taken

#ifdef _DEBUG
			if (tempstrings[p] != prinst.tempstrings[p])
			{	//something weird happened. tempstrings are supposed to be immutable (at least in length).
				externs->Sys_Error("tempstring was reallocated while gc was running");
				continue;
			}
#endif

			swept++;

			//FIXME: Security Race: its possible for a mod to do weird manipulations to access the tempstring while we're still freeing it, allowing it to read something outside of its sandbox.
			//one option would be to have the main thread bounce it back to the worker after its complete, so we can actually free the memory only after main thread has acknowledged that its tempstrings are nulled.
			prinst.tempstrings[p] = NULL;

			externs->memfree(tempstrings[p]);
		}
	}

	free(marked);

	return swept;
}

#ifdef THREADEDGC
#include "quakedef.h"
struct qcgccontext_s
{
	int done;
	unsigned int clearedtemps;	//number of temps that were swept away
	progfuncs_t *progfuncs;	//careful!

	size_t maxtemps;		//so it doesn't go stale
	tempstr_t **tempstrings;//so we don't get confused over temps added while marking

	size_t memsize;
	unsigned int amem[1];
};
void PR_QCGC_Done(void *ctx, void *data, size_t a, size_t b)
{
	struct qcgccontext_s *gc = ctx;
	gc->done = true;
}
void PR_QCGC_Thread(void *ctx, void *data, size_t a, size_t b)
{
	struct qcgccontext_s *gc = ctx;
	progfuncs_t *progfuncs = gc->progfuncs;
	smallbool *marked;

#ifdef GCTIMINGS
	double starttime, markedtime, endtime;
	starttime = Sys_DoubleTime();
#endif
	marked = PR_QCGC_Mark(gc->amem, gc->memsize, gc->maxtemps);
#ifdef GCTIMINGS
	markedtime = Sys_DoubleTime();
#endif
	gc->clearedtemps = PR_QCGC_Sweep(progfuncs, marked, gc->tempstrings, gc->maxtemps);
#ifdef GCTIMINGS
	endtime = Sys_DoubleTime();
	gc->externs->Printf("live: %u, dead: %u, threadtime: mark=%f, sweep=%f, total=%f\n", prinst.livetemps-gc->clearedtemps, gc->clearedtemps, (markedtime - starttime), (endtime - markedtime), endtime-starttime);
#endif

	COM_InsertWork(WG_MAIN, PR_QCGC_Done, gc, NULL, 0, 0);
}
#endif
static void PR_ExpandTempStrings(progfuncs_t *progfuncs, size_t newmax)
{
	tempstr_t **ntable = progfuncs->funcs.parms->memalloc(sizeof(*ntable) * newmax);
	memcpy(ntable, prinst.tempstrings, sizeof(*ntable) * prinst.maxtempstrings);
	memset(ntable+prinst.maxtempstrings, 0, sizeof(*ntable) * (newmax-prinst.maxtempstrings));
	prinst.maxtempstrings = newmax;
	if (prinst.tempstrings)
		progfuncs->funcs.parms->memfree(prinst.tempstrings);
	prinst.tempstrings = ntable;
}
static string_t PDECL PR_AllocTempStringLen			(pubprogfuncs_t *ppf, char **str, unsigned int len)
{
	progfuncs_t *progfuncs = (progfuncs_t *)ppf;
	int i;

	if (!str)
		return 0;

	if (prinst.livetemps == prinst.maxtempstrings)
	{
#ifdef THREADEDGC
		//need to wait for the gc to finish, otherwise it might be wiping freed strings that we're still using.
		while (prinst.gccontext)
		{
			COM_WorkerPartialSync(prinst.gccontext, &prinst.gccontext->done, false);
			PR_RunGC(progfuncs);
		}
#endif

		PR_ExpandTempStrings(progfuncs, prinst.maxtempstrings*2 + 1024);
	}

	for (i = prinst.nexttempstring; i < prinst.maxtempstrings && prinst.tempstrings[i]; i++)
		;
	if (i == prinst.maxtempstrings)
	{
		for (i = 0; i < prinst.nexttempstring && prinst.tempstrings[i]; i++)
			;
		if (i == prinst.nexttempstring)
			return 0;	//panic!
	}
	prinst.nexttempstring = i;
	prinst.livetemps++;

	len = ((len+3)&~3);	//round up, primarily so its safe to use loadp_i to read the last few chars of the string

	prinst.tempstrings[i] = progfuncs->funcs.parms->memalloc(sizeof(tempstr_t) - sizeof(((tempstr_t*)NULL)->value) + len);
	prinst.tempstrings[i]->size = len;
	*str = prinst.tempstrings[i]->value;

	return (string_t)((unsigned int)i | STRING_TEMP);
}
void PR_RunGC (progfuncs_t *progfuncs)
{
#ifdef THREADEDGC
	if (!prinst.gccontext)
#endif
	{
		if (prinst.livetemps < prinst.maxtempstrings/2 || prinst.nexttempstring < prinst.maxtempstrings/2)
		{	//don't bother yet
			return;
		}
#ifdef THREADEDGC
		if (externs->usethreadedgc)
		{
#ifdef GCTIMINGS
			double starttime = Sys_DoubleTime(), endtime;
#endif
			struct qcgccontext_s *gc = prinst.gccontext = malloc(sizeof(*gc) - sizeof(gc->amem) + prinst.addressableused + sizeof(*gc->tempstrings)*prinst.maxtempstrings);
			gc->done = false;
			gc->clearedtemps = 0;
			gc->progfuncs = progfuncs;

			gc->memsize = prinst.addressableused;
			memcpy(gc->amem, prinst.addressablehunk, prinst.addressableused);

			gc->maxtemps = prinst.maxtempstrings;
			gc->tempstrings = (void*)((char*)gc->amem+prinst.addressableused);
			memcpy(gc->tempstrings, prinst.tempstrings, sizeof(*gc->tempstrings)*gc->maxtemps);

			COM_InsertWork(WG_LOADER, PR_QCGC_Thread, gc, NULL, 0, 0);

#ifdef GCTIMINGS
			endtime = Sys_DoubleTime();
			gc->externs->Printf("preparetime=%f\n", (endtime - starttime));
#endif
			return;
		}
#endif
		{	//same-thread gc.
			smallbool *marked = PR_QCGC_Mark(prinst.addressablehunk, prinst.addressableused, prinst.maxtempstrings);
			size_t swept = PR_QCGC_Sweep(progfuncs, marked, prinst.tempstrings, prinst.maxtempstrings);
			prinst.livetemps -= swept;

			//if over half the (max)strings are still live, just increase the max so we are not spamming collections
			if (prinst.livetemps >= prinst.maxtempstrings/2)
				PR_ExpandTempStrings(progfuncs, prinst.maxtempstrings * 2);
		}
	}
#ifdef THREADEDGC
	else if (prinst.gccontext->done)
	{
		prinst.livetemps -= prinst.gccontext->clearedtemps;
		free(prinst.gccontext);
		prinst.gccontext = NULL;

		//if over half the (max)strings are still live, just increase the max so we are not spamming collections
		if (prinst.livetemps >= prinst.maxtempstrings/2)
			PR_ExpandTempStrings(progfuncs, prinst.maxtempstrings * 2);
	}
#endif
}

static void PR_FreeAllTemps			(progfuncs_t *progfuncs)
{
	unsigned int i;
#ifdef THREADEDGC
	while (prinst.gccontext)
	{
		COM_WorkerPartialSync(prinst.gccontext, &prinst.gccontext->done, false);
		PR_RunGC(progfuncs);
	}
#endif
	for (i = 0; i < prinst.maxtempstrings; i++)
	{
		externs->memfree(prinst.tempstrings[i]);
		prinst.tempstrings[i] = NULL;
	}
	prinst.maxtempstrings = 0;
	prinst.nexttempstring = 0;
	prinst.livetemps = 0;
}
#else
static string_t PDECL PR_AllocTempStringLen			(pubprogfuncs_t *ppf, char **str, unsigned int len)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	tempstr_t **ntable, *n;
	int newmax;
	int i;

	if (!str)
		return 0;

	if (prinst.numtempstrings == prinst.maxtempstrings)
	{
		newmax = prinst.maxtempstrings + 1024;
		ntable = progfuncs->funcs.parms->memalloc(sizeof(char*) * newmax);
		memcpy(ntable, prinst.tempstrings, sizeof(char*) * prinst.numtempstrings);
		prinst.maxtempstrings = newmax;
		if (prinst.tempstrings)
			progfuncs->funcs.parms->memfree(prinst.tempstrings);
		prinst.tempstrings = ntable;
	}

	i = prinst.numtempstrings;
	if (i == 0x10000000)
		return 0;

	prinst.numtempstrings++;

	n = progfuncs->funcs.parms->memalloc(sizeof(tempstr_t) - sizeof(((tempstr_t*)NULL)->value) + len);
	n->size = len;
	*str = n->value;
	prinst.tempstrings[i] = n;	//doesn't have its value yet...

	return (string_t)((unsigned int)i | STRING_TEMP);
}
void PR_FreeTemps			(progfuncs_t *progfuncs, int depth)
{
	int i;
	if (depth > prinst.numtempstrings)
	{
		Sys_Error("QC Temp stack inverted\n");
		return;
	}
	for (i = depth; i < prinst.numtempstrings; i++)
	{
		externs->memfree(prinst.tempstrings[i]);
	}

	prinst.numtempstrings = depth;
}
static void PR_FreeAllTemps			(progfuncs_t *progfuncs)
{
	unsigned int i;
	for (i = 0; i < prinst.numtempstrings; i++)
	{
		externs->memfree(prinst.tempstrings[i]);
		prinst.tempstrings[i] = NULL;
	}
	prinst.numtempstrings = 0;
	prinst.nexttempstring = 0;
}
#endif

string_t PDECL PR_AllocTempString			(pubprogfuncs_t *ppf, const char *str)
{
	char *out;
	string_t res;
	size_t len;
	if (!str)
		return 0;
	len = strlen(str)+1;
	res = PR_AllocTempStringLen(ppf, &out, len);
	if (res)
		memcpy(out, str, len);
	return res;
}
static pbool PDECL PR_DumpProfiles (pubprogfuncs_t *ppf, pbool resetprofiles)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	struct progstate_s *ps;
	unsigned int i, f, j, s;
	prclocks_t cpufrequency;
	struct
	{
		char *fname;
		int profile;
		prclocks_t profiletime;
		prclocks_t totaltime;
	} *sorted, t;
	if (!prinst.profiling)
	{
		prinst.profiling = true;
		return false;
	}

	cpufrequency = Sys_GetClockRate();

	for (i = 0; i < prinst.maxprogs; i++)
	{
		ps = &pr_progstate[i];
		if (ps->progs == NULL)	//we havn't loaded it yet, for some reason
			continue;	

		externs->Printf("%s:\n", ps->filename);
		sorted = malloc(sizeof(*sorted) * ps->progs->numfunctions);
		//pull out the functions in order to sort them
		for (s = 0, f = 0; f < ps->progs->numfunctions; f++)
		{
			if (!ps->functions[f].profile)
				continue;
			sorted[s].fname = ps->functions[f].s_name+progfuncs->funcs.stringtable;
			sorted[s].profile = ps->functions[f].profile;
			sorted[s].profiletime = ps->functions[f].profiletime - ps->functions[f].profilechildtime;
			sorted[s].totaltime = ps->functions[f].profiletime;
			if (resetprofiles)
			{
				ps->functions[f].profile = 0;
				ps->functions[f].profiletime = 0;
				ps->functions[f].profilechildtime = 0;
			}
			s++;
		}

		// good 'ol bubble sort
		for (f = 0; f < s; f++)
		{
			for (j = f; j < s; j++)
				if (sorted[f].profiletime > sorted[j].profiletime)
				{
					t = sorted[f];
					sorted[f] = sorted[j];
					sorted[j] = t;
				}
		}

		//print it out
		externs->Printf("%8s %9s %10s: %s\n", "ops", "self-time", "total-time", "function");
		for (f = 0; f < s; f++)
			externs->Printf("%8u %9f %10f: %s\n", sorted[f].profile, ull2dbl(sorted[f].profiletime) / ull2dbl(cpufrequency), ull2dbl(sorted[f].totaltime) / ull2dbl(cpufrequency), sorted[f].fname);
		free(sorted);
	}
	return true;
}

static void PDECL PR_Shutdown(pubprogfuncs_t *ppf);

static pubprogfuncs_t deffuncs = {
	PROGSTRUCT_VERSION,
	PR_Shutdown,
	PR_Configure,
	PR_LoadProgs,
	PR_InitEnts,
	PR_ExecuteProgram,
	PR_globals,
	PR_entvars,
	PR_RunError,
	ED_Print,
	ED_Alloc,
	ED_AllocIndex,
	ED_Free,

	QC_EDICT_NUM,
	QC_NUM_FOR_EDICT,

	PR_VarString,

	NULL,	//progstate
	0,		//numprogs

	PR_FindFunc,
#if defined(MINIMAL) || defined(OMIT_QCC)
	NULL,
	NULL,
#else
	Comp_Begin,
	Comp_Continue,
#endif

	filefromprogs,
	NULL,//filefromnewprogs,

	ED_Print,
	PR_SaveEnts,
	PR_LoadEnts,

	PR_SaveEnt,
	PR_RestoreEnt,

	PR_FindGlobal,

	QC_GetEdictFieldValue,
	ProgsToEdict,
	EdictToProgs,

	PR_EvaluateDebugString,
	0,//trace
	PR_StackTrace,
	PR_ToggleBreakpoint,

	NULL,	//parms
#if 1//defined(MINIMAL) || defined(OMIT_QCC)
	NULL,	//decompile
#else
	QC_Decompile,
#endif
	0,	//callargc
	0,

	0,	//string table(pointer base address)
	0,		//string table size
	0,	//max size
	0,	//field adjust(aditional field offset)
	0,	//field slots allocated (for builtins to clamp field reference args).

	PR_ForkStack,
	PR_ResumeThread,
	PR_AbortStack,
	PR_GetBuiltinCallInfo,
	PR_FindBuiltins,

	QC_RegisterFieldVar,

	ED_NewString,
	QC_HunkAlloc,
	PR_memalloc,
	PR_memrealloc,
	PR_memfree,
	PR_AllocTempString,
	PR_AllocTempStringLen,
	PR_StringToProgs,
	PR_StringToNative,

	PR_QueryField,
	QC_ClearEdict,
	QC_FindPrefixedGlobals,
	PR_SetWatchPoint,

	QC_AddSharedVar,
	QC_AddSharedFieldVar,
	PR_RemoveProgsString,
	PR_GetFunctionInfo,
	PR_GenerateStatementString,
	ED_FieldInfo,
	PR_UglyValueString,
	ED_ParseEval,
	PR_SetStringField,
	PR_DumpProfiles,

	0, NULL,
};
static int PDECL qclib_null_printf(const char *s, ...)
{
	return 0;
}
static void *PDECL qclib_malloc(int size)
{
	return malloc(size);
}
static void PDECL qclib_free(void *ptr)
{
	free(ptr);
}
#ifdef FTE_TARGET_WEB
#undef printf
#define printf NULL	//should be some null wrapper instead
#endif

//progfuncs_t *progfuncs = NULL;
#undef memfree
#undef prinst
#undef extensionbuiltin
#undef field
#undef shares
#undef maxedicts
#undef sv_num_edicts

static void PDECL PR_Shutdown(pubprogfuncs_t *ppf)
{
	void (VARGS *f) (void *);
	progfuncs_t *inst = (progfuncs_t*)ppf;

	unsigned int i;
	edictrun_t *e;

	f = inst->funcs.parms->memfree;

	for ( i=1 ; i<inst->inst.maxedicts; i++)
	{
		e = (edictrun_t *)(inst->funcs.edicttable[i]);
		inst->funcs.edicttable[i] = NULL;
		if (e)
		{
//			e->entnum = i;
			f(e);
		}
	}

	PRHunkFree(inst, 0);

#if defined(_WIN32) && !defined(WINRT)
	VirtualFree(inst->inst.addressablehunk, 0, MEM_RELEASE);	//doesn't this look complicated? :p
#else
	free(inst->inst.addressablehunk);
#endif

	PR_FreeAllTemps(inst);

	if (inst->inst.allocedstrings)
		f(inst->inst.allocedstrings);
	inst->inst.allocedstrings = NULL;
	if (inst->inst.tempstrings)
		f(inst->inst.tempstrings);
	inst->inst.tempstrings = NULL;

	free(inst->inst.watch_name);

	if (inst->inst.field)
		f(inst->inst.field);
	if (inst->inst.shares)
		f(inst->inst.shares);	//free memory
	f(inst);
}

#ifndef WIN32
#define QCLIBINT	//don't use dllspecifications
#endif

#if defined(QCLIBDLL_EXPORTS)
	#ifdef _WIN32
		__declspec(dllexport)
	#else
		__attribute__((visibility("default")))
	#endif
#endif
pubprogfuncs_t * PDECL InitProgs(progexterns_t *ext)
{	
	progfuncs_t *funcs;

	if (!ext)
	{
		static progexterns_t defexterns;
		ext = &defexterns;
	}
	else if (ext->progsversion != PROGSTRUCT_VERSION)
		return NULL;

#undef memalloc
#undef pr_progstate
#undef pr_argc
	funcs = (ext->memalloc?ext->memalloc:qclib_malloc)(sizeof(progfuncs_t));	
	memcpy(&funcs->funcs, &deffuncs, sizeof(pubprogfuncs_t));
	memset(&funcs->inst, 0, sizeof(funcs->inst));

	funcs->funcs.progstate = &funcs->inst.progstate;

	funcs->funcs.parms = ext;


	{
		//defs incase following structure is not passed.
		static struct edict_s *safe_edicts;
		static int safe_num_edicts;
		static double safetime=0;
		if (!ext->progsversion)	ext->progsversion = PROGSTRUCT_VERSION;
		if (!ext->Printf)		ext->Printf = qclib_null_printf;
		if (!ext->DPrintf)		ext->DPrintf = qclib_null_printf;
		if (!ext->Sys_Error)	ext->Sys_Error = (void*)exit;
		if (!ext->memalloc)		ext->memalloc = qclib_malloc;
		if (!ext->memfree)		ext->memfree = qclib_free;
		if (!ext->gametime)		ext->gametime = &safetime;
		if (!ext->edicts)		ext->edicts = &safe_edicts;
		if (!ext->num_edicts)	ext->num_edicts = &safe_num_edicts;
		if (!ext->edictsize)	ext->edictsize = sizeof(edictrun_t);
	}

	SetEndian();
	
	return &funcs->funcs;
}
















#ifdef QCC
void main (int argc, char **argv)
{
	progexterns_t ext;

	progfuncs_t *funcs;
	funcs = InitProgs(&ext);
	if (funcs->PR_StartCompile(argc, argv))
		while(funcs->PR_ContinueCompile());
}
#endif
