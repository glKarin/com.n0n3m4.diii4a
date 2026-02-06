/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of the fteqw tools.

FTEQW and the Quake III Arena source code are free software; you can redistribute them
and/or modify them under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

FTEQW and the Quake III Arena source code are distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
Copyright (C) 2007 David Walton

GPL.

This file is the core of an assembler/linker to generate q3-compatable qvm files. It is based on the version in the Q3 source code, but rewritten from the ground up.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../engine/qclib/hash.h"


//from qfiles.h
#define	VM_MAGIC	0x12721444
struct vmheader {
	int		vmMagic;

	int		instructionCount;

	int		codeOffset;
	int		codeLength;

	int		dataOffset;	// data then lit are here
	int		dataLength;
	int		litLength;	// ( dataLength - litLength ) should be byteswapped on load
	int		bssLength;	// zero filled memory appended to datalength
};
//end of qfiles.h



typedef enum {
	OP_UNDEF, 

	OP_IGNORE, 

	OP_BREAK, 

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[yop
	OP_ARG,
	OP_BLOCK_COPY,

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
} opcode_t;


typedef struct {
	char	*name;
	int		opcode;
} sourceOps_t;

sourceOps_t		sourceOps[] = {
#include "opstrings.h"
};


enum segs {
	SEG_CODE,
	SEG_DATA,
	SEG_LIT,
	SEG_BSS,

	SEG_MAX,
	SEG_UNKNOWN
};

#define SYMBOLBUCKETS 4096
#define LOCALSYMBOLBUCKETS 1024
#define USEDATBLOCK 64
#define STACKSIZE 0x10000

#ifdef WIN32
#define snprintf _snprintf
#endif

struct usedat {
	struct usedat *next;
	int count;
	
	char seg[USEDATBLOCK];
	int offset[USEDATBLOCK];
};

struct symbol {
	struct symbol *next;

	int inseg;	//this is where the actual value is defined
	int atoffset;

	struct usedat usedat;	//this is where it is used
				//to help with cpu cache, the first is embeded
	
	bucket_t bucket;	//for the hash tables

	char name[1];	//a common C hack
};

struct segment {
	void *data;
	unsigned int base;
	unsigned int length;
	unsigned int available;	//before it needs a realloc

	int isdummy;	//set if there's no real data assosiated with this segment
};

struct assembler {
	struct segment segs[SEG_MAX];
	struct segment *curseg;

	struct symbol *symbols;

	hashtable_t symboltable;
	bucket_t symboltablebuckets[SYMBOLBUCKETS];
	
	hashtable_t localsymboltable;
	bucket_t localsymboltablebuckets[LOCALSYMBOLBUCKETS];

	struct symbol *lastsym;	//so we can move symbols that lcc put into the wrong place.
	
	int numinstructions;

	int errorcount;

	int funclocals;
	int funcargs;
	int funcargoffset;

	int verbose;
};

struct workload {
	char output[256];

	int numsrcfiles;
	char **srcfilelist;

	int verbose;
};







void EmitData(struct segment *seg, void *data, int bytes)
{
	unsigned char *d, *s=data;

	if (seg->isdummy)
	{
		seg->length += bytes;
		return;
	}

	if (seg->length + bytes > seg->available)
	{
		unsigned int newlen = (seg->length + bytes + 0x10000) & ~0xffff;
		void *newseg;
		newseg = realloc(seg->data, newlen);
		if (!newseg)
		{
			printf("out of memory\n");
			seg->length += bytes;
			seg->isdummy = 1;
			return;
		}
		memset((char*)newseg+seg->available, 0, newlen - seg->available);
		seg->data = newseg;
		seg->available = newlen;
	}

	d = (unsigned char*)seg->data + seg->length;
	seg->length += bytes;
	while(bytes-- > 0)
		*d++ = *s++;
}

void EmitByte(struct segment *seg, unsigned char data)
{
	EmitData(seg, &data, 1);
}

void SkipData(struct segment *seg, int bytes)
{
	seg->length += bytes;
}

void AlignSegment(struct segment *seg, int alignment)
{
	//alignment must always be power of 2
	alignment--;
	seg->length = (seg->length + alignment) & ~(alignment);
}

void AssembleError(struct assembler *w, char *message, ...)
{
	va_list va;

	va_start(va, message);
	vprintf(message, va);
	va_end(va);
	w->errorcount++;
}

void SegmentHack(struct assembler *w, int seg)
{
	//like id, I don't see any way around this
	//plus compatability. :/
	
	if (w->curseg != &w->segs[seg])
	{
		w->curseg = &w->segs[seg];

		if (w->lastsym)
		{
//			if (*w->lastsym->name != '$')
//				printf("symbol %s segment-switch hack\n", w->lastsym->name);
			w->lastsym->inseg = seg;
			w->lastsym->atoffset = w->curseg->length;
		}
		else
			printf("segment-switch hack\n");
	}
}

void DoSegmentHack(struct assembler *w, int bytes)
{
	if (bytes == 4)
	{
		SegmentHack(w, SEG_DATA);
	}
	else if (bytes == 1)
	{
		SegmentHack(w, SEG_LIT);
	}
	else
		AssembleError(w, "%ibit variables are not supported\n", bytes*8);
}


int ParseToken(char *buffer, int bufferlen, char **input)
{
	unsigned char *in = (unsigned char*)*input;
	*buffer = 0;
	while (*in <= ' ')
	{
		if (!*in++)
			return 0;
	}

	if (*in == ';')	//';' is taken as a comment
		return 0;
	
	do
	{
		if (!--bufferlen)
		{
			printf("name too long\n");
			break;
		}
		*buffer++ = *in++;
	} while (*in > ' ');
	*buffer = 0;
	*input = (char*)in;
	return 1;
}

void AddReference(struct usedat *s, int segnum, int segofs)
{
	if (s->count == USEDATBLOCK)
	{	//we're getting a lot of references for this var
		if (!s->next)
		{
			s->next = malloc(sizeof(*s));
			if (!s->next)
			{
				printf("out of memory\n");
//				AssembleError(w, "out of memory\n");
				return;
			}
			s->next->next = NULL;
			s->next->count = 0;
		}
		AddReference(s->next, segnum, segofs);
		return;
	}
	s->seg[s->count] = segnum;
	s->offset[s->count] = segofs;
	s->count++;
}

struct symbol *GetSymbol(struct assembler *w, char *name)
{
	struct symbol *s;
	hashtable_t *t;
	if (*name == '$')
		t = &w->localsymboltable;
	else
		t = &w->symboltable;

	s = Hash_Get(t, name);
	if (!s)
	{
		s = malloc(sizeof(*s) + strlen(name));
		if (!s)
		{
			AssembleError(w, "out of memory\n");
			return NULL;
		}
		memset(s, 0, sizeof(*s));
		s->next = w->symbols;
		w->symbols = s;
		strcpy(s->name, name);
		Hash_Add(t, s->name, s, &s->bucket);
		s->inseg = SEG_UNKNOWN;
		s->atoffset = 0;
	}

	return s;
}

void DefineSymbol(struct assembler *w, char *symname, int segnum, int segofs)
{
	struct symbol *s;
	hashtable_t *t;
	if (*symname == '$')
		t = &w->localsymboltable;
	else
		t = &w->symboltable;

	if (!*symname)	//bail out
		*(int*)NULL = -3;
		
	s = Hash_Get(t, symname);
	if (s)
	{
		if (s->inseg != SEG_UNKNOWN)
			AssembleError(w, "symbol %s is already defined!\n", symname);
	}
	else
	{
		s = malloc(sizeof(*s) + strlen(symname));
		if (!s)
		{
			printf ("out of memory\n");
			w->errorcount++;
			return;
		}
		memset(s, 0, sizeof(*s));
		s->next = w->symbols;
		w->symbols = s;
		strcpy(s->name, symname);
		Hash_Add(t, s->name, s, &s->bucket);
	}
	w->lastsym = s;
	s->inseg = segnum;
	s->atoffset = segofs;
}

int CalculateExpression(struct assembler *w, char *line)
{
	char *o;
	char valstr[64];
	int val;
	int segnum = w->curseg - w->segs;
	int segoffs = w->curseg->length;
	int isnum;
	struct symbol *s;
	//this is expressed as CONST+NAME-NAME
	//many instructions need to add an additional base before emitting it
	//so set the references to the place that we expect to be
	
	val = 0;
	
	while (*line)
	{
		o = valstr;
		if (*line == '-' || *line == '+')
		{
			*o++ = *line++;
		}
		while (*(unsigned char*)line > ' ')
		{
			if (*line == '+' || *line == '-')
				break;
			*o++ = *line++;
		}
		*o = '\0';

		if (*valstr == '-')
			isnum = 1;
		else if (*valstr == '+')
			isnum = (valstr[1] >= '0' && valstr[1] <= '9');
		else
			isnum = (valstr[0] >= '0' && valstr[0] <= '9');

		if (isnum)
			val += atoi(valstr);
		else
		{
			if (*valstr == '-' || *valstr == '+')
				s = GetSymbol(w, valstr+1);
			else
				s = GetSymbol(w, valstr);
			if (s)	//yeah, doesn't make sense to have two symbols in one expression
				AddReference(&s->usedat, segnum, segoffs);	//but we do it anyway
		}
	}

	return val;
}

void AssembleLine(struct assembler *w, char *line)
{
	int i;
	int opcode;
	char instruction[64];
	if (!ParseToken(instruction, sizeof(instruction), &line))
		return;	//nothing on this line

	//try and get our special instructions first
	switch (instruction[0])
	{
		case 'a':
			if (!strcmp(instruction+1, "lign"))
			{	//align
				//theoretically, this should never make a difference. if it does, the segment hack stuff screwed something up.
				if (ParseToken(instruction, sizeof(instruction), &line))
					AlignSegment(w->curseg, atoi(instruction));
				else
					AssembleError(w, "align without argument\n");
				return;
			}
			if (!strcmp(instruction+1, "ddress"))
			{	//address
//				w->lastsym = NULL;
				if (ParseToken(instruction, sizeof(instruction), &line))
				{
					int expression;
					SegmentHack(w, SEG_DATA);
					expression = CalculateExpression(w, instruction);
					EmitData(w->curseg, &expression, 4);
				}
				return;
			}
			break;
		case 'b':
			if (!strcmp(instruction+1, "ss"))
			{	//bss
				w->curseg = &w->segs[SEG_BSS];
				return;
			}
			if (!strcmp(instruction+1, "yte"))
			{	//byte
				unsigned int value = 0;
				unsigned int bytes = 0;

				if (ParseToken(instruction, sizeof(instruction), &line))
					bytes = atoi(instruction);
				else
					AssembleError(w, "byte without count\n");
				if (ParseToken(instruction, sizeof(instruction), &line))
					value = atoi(instruction);
				else
					AssembleError(w, "byte without value\n");

				if (bytes == 4)
				{
					SegmentHack(w, SEG_DATA);
					EmitData(w->curseg, &value, 4);
				}
				else if (bytes == 1)
				{
					SegmentHack(w, SEG_LIT);
					EmitByte(w->curseg, value);
				}
				else
					AssembleError(w, "%ibit variables are not supported\n", bytes*8);
				return;
			}
			break;
		case 'c':
			if (!strcmp(instruction+1, "ode"))
			{	//code
				w->curseg = &w->segs[SEG_CODE];
				return;
			}
			break;
		case 'd':
			if (!strcmp(instruction+1, "ata"))
			{	//data
				w->curseg = &w->segs[SEG_DATA];
				return;
			}
			break;
		case 'e':
			if (!strcmp(instruction+1, "qu"))
			{	//equ
				char value[32];
				if (ParseToken(instruction, sizeof(instruction), &line))
				{
					if (ParseToken(value, sizeof(value), &line))
						DefineSymbol(w, instruction, w->curseg - w->segs, atoi(value));
					else

						AssembleError(w, "equ without value\n");
				}
				else
					AssembleError(w, "equ without name\n");
				return;
			}
			if (!strcmp(instruction+1, "xport"))
			{	//export (ignored)
				return;
			}
			if (!strcmp(instruction+1, "ndproc"))
			{	//endproc
				int locals = 0, args = 0;
				if (ParseToken(instruction, sizeof(instruction), &line))
					locals = atoi(instruction);
				if (ParseToken(instruction, sizeof(instruction), &line))
					args = atoi(instruction);

				EmitByte(&w->segs[SEG_CODE], OP_PUSH);	//we must return something
				w->numinstructions++;

				//disregard the two vars from above (matches q3asm...)
				locals = 8 + w->funclocals + w->funcargs;
				EmitByte(&w->segs[SEG_CODE], OP_LEAVE);
				EmitData(&w->segs[SEG_CODE], &locals, 4);
				w->numinstructions++;
				return;
			}
			break;
		case 'f':
			if (!strcmp(instruction+1, "ile"))
			{	//file (ignored)
				return;
			}
			break;
		case 'i':
			if (!strcmp(instruction+1, "mport"))
			{	//import (ignored)
				return;
			}
			break;
		case 'l':
			if (!strcmp(instruction+1, "ine"))
			{	//line (ignored)
				return;
			}
			if (!strcmp(instruction+1, "it"))
			{	//lit
				w->curseg = &w->segs[SEG_LIT];
				return;
			}
			break;
		case 'p':
			if (!strcmp(instruction+1, "roc"))
			{	//proc
				int v;
				if (!ParseToken(instruction, sizeof(instruction), &line))
					AssembleError(w, "unnamed function\n");
				DefineSymbol(w, instruction, SEG_CODE, w->numinstructions);
				if (ParseToken(instruction, sizeof(instruction), &line))
					w->funclocals = (atoi(instruction) + 3) & ~3;
				else
					AssembleError(w, "proc without locals\n");
				if (ParseToken(instruction, sizeof(instruction), &line))
					w->funcargs = (atoi(instruction) + 3) & ~3;
				else
					AssembleError(w, "proc without args\n");

				v = 8 + w->funclocals + w->funcargs;
				if (v > 32767)
					AssembleError(w, "function with an aweful lot of args\n");

				EmitByte(&w->segs[SEG_CODE], OP_ENTER);
				EmitData(&w->segs[SEG_CODE], &v, 4);
				w->numinstructions++;
				return;
			}
			if (!strcmp(instruction+1, "op"))
			{	//pop
				EmitByte(&w->segs[SEG_CODE], OP_POP);
				w->numinstructions++;
				return;
			}
			break;
		case 's':
			if (!strcmp(instruction+1, "kip"))
			{	//skip
				if (ParseToken(instruction, sizeof(instruction), &line))
					SkipData(w->curseg, atoi(instruction)); 
				else
					AssembleError(w, "skip without argument\n");
				return;
			}
			break;
		case 'L':
			if (!strncmp(instruction+1, "ABEL", 4))
			{
				if (!ParseToken(instruction, sizeof(instruction), &line))
				{
					printf("label with no name\n");
					return;
				}
				//some evilness here (symbols in the instruction segment are instruction indexes not byte offsets)
				if (w->curseg == &w->segs[SEG_CODE])
					DefineSymbol(w, instruction, w->curseg - w->segs, w->numinstructions);
				else
					DefineSymbol(w, instruction, w->curseg - w->segs, w->curseg->length);
				return;
			}
			break;
		case 'A':
			if (!strncmp(instruction+1, "RG", 2))
			{	//ARG*
				EmitByte(&w->segs[SEG_CODE], OP_ARG);
				w->numinstructions++;
				if (8 + w->funcargoffset > 255)
					AssembleError(w, "too many arguments in fuction\n");
				EmitByte(&w->segs[SEG_CODE], 8 + w->funcargoffset);
				w->funcargoffset += 4;	//reset by a following CALL instruction
				return;
			}
			if (!strncmp(instruction+1, "DDRF", 4))
			{	//ADDRF*
				int exp = 0;
				w->curseg = &w->segs[SEG_CODE];	//this differs, but not for lcc
				EmitByte(&w->segs[SEG_CODE], OP_LOCAL);
				if (ParseToken(instruction, sizeof(instruction), &line))
					exp = CalculateExpression(w, instruction);
				exp += 16 + w->funcargs + w->funclocals;
				EmitData(&w->segs[SEG_CODE], &exp, 4);
				w->numinstructions++;
				return;
			}
			if (!strncmp(instruction+1, "DDRL", 4))
			{	//ADDRL
				int exp = 0;
				w->curseg = &w->segs[SEG_CODE];	//this differs, but not for lcc
				EmitByte(&w->segs[SEG_CODE], OP_LOCAL);
				if (ParseToken(instruction, sizeof(instruction), &line))
					exp = CalculateExpression(w, instruction);
				exp += 8 + w->funcargs;
				EmitData(&w->segs[SEG_CODE], &exp, 4);
				w->numinstructions++;
				return;
			}
			break;
		case 'C':
			if (!strncmp(instruction+1, "ALL", 3))
			{	//CALL*
				EmitByte(&w->segs[SEG_CODE], OP_CALL);
				w->numinstructions++;
				w->funcargoffset = 0;
				return;
			}
			break;
		case 'R':
			if (!strncmp(instruction+1, "ET", 2))
			{
				int v = 8 + w->funclocals + w->funcargs;
				EmitByte(&w->segs[SEG_CODE], OP_LEAVE);
				EmitData(&w->segs[SEG_CODE], &v, 4);
				w->numinstructions++;
				return;
			}
			break;
		default:
			break;
	}

	//okay, we don't know what it is yet, try generic instructions
	
	for (i = 0; i < sizeof(sourceOps) / sizeof(sourceOps[0]); i++)
	{
		if (*(unsigned int*)sourceOps[i].name == *(unsigned int*)instruction && !strcmp(sourceOps[i].name+4, instruction+4))
		{
			opcode = sourceOps[i].opcode;
			if (opcode == OP_IGNORE)
				return;

			if (opcode == OP_SEX8)
			{	//sex is special, apparently
				
				if (ParseToken(instruction, sizeof(instruction), &line))
				{
					if (*instruction == '2')
						opcode = OP_SEX16;
					else if (*instruction != '1')
						AssembleError(w, "bad sign extension\n");
				}
			}

			if (opcode == OP_CVIF || opcode == OP_CVFI)
				line = "";	//so we don't fall afoul looking for aguments (float/int conversions are always 4 anyway)

			if (ParseToken(instruction, sizeof(instruction), &line))
			{
				int expression;
				w->curseg = &w->segs[SEG_CODE];	//this differs, but not for lcc
				EmitByte(&w->segs[SEG_CODE], opcode);
				expression = CalculateExpression(w, instruction);

				if (opcode == OP_BLOCK_COPY)	//string initialisations are block copys too (this could still be wrong, but it matches)
					expression = (expression+3)&~3;
				EmitData(&w->segs[SEG_CODE], &expression, 4);
			}
			else
				EmitByte(&w->segs[SEG_CODE], opcode);

			w->numinstructions++;
			return;
		}
	}

	AssembleError(w, "Cannot handle %s\n", instruction);
}

void AssembleFile(struct assembler *w, int fnum, char *filename)
{
	char linebuffer[1024];
	FILE *f;
	if (w->verbose)
	{
		if (fnum)
			printf("file: %i %s\n", fnum, filename);
		else
			printf("file: %s\n", filename);
	}

	w->curseg = &w->segs[SEG_CODE];

	f = fopen(filename, "rt");
	if (!f)
	{
		snprintf(linebuffer, sizeof(linebuffer)-1, "%s.asm", filename);
		f = fopen(linebuffer, "rt");
	}

	if (f)
	{
		while(fgets(linebuffer, sizeof(linebuffer), f))
			AssembleLine(w, linebuffer);
		fclose(f);
	}
	else
	{
		printf("couldn't open %s\n", filename);
		w->errorcount++;
	}

	//reset the local symbol hash, so we don't find conflicting vars
	memset(w->localsymboltablebuckets, 0, sizeof(w->localsymboltablebuckets));
}



int FixupSymbolReferencesOne(struct assembler *w, struct symbol *s, struct usedat *u)
{
	int i;
	int val;
	unsigned int temp;
	unsigned char *ptr;
	val = w->segs[s->inseg].base + s->atoffset;
	for (i = 0; i < u->count; i++)
	{
		ptr = (unsigned char*)w->segs[(int)u->seg[i]].data;
		ptr += u->offset[i];
		temp = (unsigned int)(ptr[0] | (ptr[1]<<8) | (ptr[2]<<16) | (ptr[3]<<24));
		*(int*)&temp += val;
		ptr[0] = (temp&0x000000ff)>>0;
		ptr[1] = (temp&0x0000ff00)>>8;
		ptr[2] = (temp&0x00ff0000)>>16;
		ptr[3] = (temp&0xff000000)>>24;
	}
	if (u->next)
		i += FixupSymbolReferencesOne(w, s, u->next);
	
	return i;
}


void FixupSymbolReferences(struct assembler *w)
{	//this is our 'second pass'
	struct symbol *s;
	int numsyms = 0;
	int numsymrefs = 0;

	if (w->verbose)
		printf("second 'pass'\n");

	for (s = w->symbols; s; s = s->next)
	{
		if (s->inseg == SEG_UNKNOWN)
		{
			AssembleError(w, "Symbol %s was not defined\n", s->name);
			continue;
		}

		if (w->verbose && !s->usedat.count && *s->name != '$')	//don't ever mention the compiler generated 'static' vars
			printf("%s was never used\n", s->name);
		numsymrefs += FixupSymbolReferencesOne(w, s, &s->usedat);
		numsyms++;
	}


	s = GetSymbol(w, "vmMain");
	if (s)
	{
		if (s->atoffset != 0 || s->inseg != SEG_CODE)
			AssembleError(w, "vmMain *MUST* be the first symbol in the qvm\nReorder your files\n");
	}

	if (w->verbose)
	{
		printf("Found %i symbols\n", numsyms);
		printf("Found %i symbol references\n", numsymrefs);
	}
}

void WriteMapFile(struct assembler *w, char *fname)
{
	int segnum;
	struct symbol *s;
	FILE *f;
	f = fopen(fname, "wt");
	if (!f)
		return;
//if we sorted these, we'd have better compatability with id's q3asm
//(their linker only defines variables in the second pass, their first pass is only to get table sizes)
//our symbols are in the order they were referenced, rather than used.
	for (segnum = 0; segnum < SEG_MAX; segnum++)
	{
		for (s = w->symbols; s; s = s->next)
		{
			if (*s->name == '$')	//don't show locals
				continue;
			if (s->inseg != segnum)
				continue;

			fprintf(f, "%i %8x %s\n", s->inseg, s->atoffset, s->name);
		}
	}
	
	fclose(f);
}





void InitialiseAssembler(struct assembler *w)
{
	int i;
	memset(w, 0, sizeof(*w));

	Hash_InitTable(&w->symboltable, SYMBOLBUCKETS, w->symboltablebuckets);
	Hash_InitTable(&w->localsymboltable, LOCALSYMBOLBUCKETS, w->localsymboltablebuckets);
	
	w->segs[SEG_BSS].isdummy = 1;

	i = 0;
	EmitData(&w->segs[SEG_DATA], &i, 4);	//so NULL really is NULL

}

void WriteOutput(struct assembler *w, char *outputname)
{
	FILE *f;
	struct vmheader h;

	int i;
	for (i = 0; i < SEG_MAX; i++)
	{	//align the segments (yes, even the code segment)
		w->segs[i].length = (w->segs[i].length + 3) & ~3;
	}

	//add the stack to the end of the bss. I don't know if q3 even uses this
	DefineSymbol(w, "_stackStart", SEG_BSS, w->segs[SEG_BSS].length);
	w->segs[SEG_BSS].length += STACKSIZE;
	DefineSymbol(w, "_stackEnd", SEG_BSS, w->segs[SEG_BSS].length);

	w->segs[SEG_CODE].base = 0;
	w->segs[SEG_DATA].base = 0;
	w->segs[SEG_LIT].base = w->segs[SEG_DATA].base + w->segs[SEG_DATA].length;
	w->segs[SEG_BSS].base = w->segs[SEG_LIT].base + w->segs[SEG_LIT].length;

	FixupSymbolReferences(w);
	
	if (w->segs[SEG_CODE].isdummy || w->segs[SEG_DATA].isdummy || w->segs[SEG_LIT].isdummy)
		w->errorcount++;	//one of the segments failed to allocate mem
	
	printf("code bytes: %i\n", w->segs[SEG_CODE].length);
	printf("data bytes: %i\n", w->segs[SEG_DATA].length);
	printf(" lit bytes: %i\n", w->segs[SEG_LIT ].length);
	printf(" bss bytes: %i\n", w->segs[SEG_BSS ].length);
	printf("instruction count: %i\n", w->numinstructions);

	if (w->errorcount)
	{
		printf("%i errors\n", w->errorcount);
		return;
	}
	

	h.vmMagic = VM_MAGIC;
	h.instructionCount = w->numinstructions;
	h.codeLength = w->segs[SEG_CODE].length;
	h.dataLength = w->segs[SEG_DATA].length;
	h.litLength = w->segs[SEG_LIT].length;
	h.bssLength = w->segs[SEG_BSS].length;

	h.codeOffset = sizeof(h);
	h.dataOffset = h.codeOffset + h.codeLength;
	f = fopen(outputname, "wb");
	if (f)
	{
		fwrite(&h, 1, sizeof(h), f);
		fwrite(w->segs[SEG_CODE].data, 1, w->segs[SEG_CODE].length, f);
		fwrite(w->segs[SEG_DATA].data, 1, w->segs[SEG_DATA].length, f);
		fwrite(w->segs[SEG_LIT ].data, 1, w->segs[SEG_LIT ].length, f);
		//logically the bss comes here (but doesn't, cos its bss)
		fclose(f);
	}
	else
		printf("error writing to file: %s\n", outputname);


	WriteMapFile(w, "q3asm2.map");
}










void Assemble(struct workload *w)
{
	int i;

	struct assembler a;

	InitialiseAssembler(&a);
	a.verbose = w->verbose;

	for (i = 0; i < w->numsrcfiles; i++)
		AssembleFile(&a, i+1, w->srcfilelist[i]);
	
	WriteOutput(&a, w->output);
}

int ParseCommandList(struct workload *w, int numcmds, char **cmds)
{
	char **t;

	FILE *f;
	char *buffer, *pp, *pps;
	unsigned int len, numextraargs;
	char *suppargs[64];

	while (numcmds)
	{
		if ((*cmds)[0] == '-')
		{
			if ((*cmds)[1] == 'f')
			{
				numcmds--;
				cmds++;

				f = fopen(*cmds, "rt");
				if (!f)
				{
					char blah[256];
					snprintf(blah, sizeof(blah)-1, "%s.q3asm", *cmds);
					f = fopen(blah, "rt");
				}
				if (f)
				{
					fseek(f, 0, SEEK_END);
					len = ftell(f);
					fseek(f, 0, SEEK_SET);
					buffer = malloc((len+9));
					if (buffer)
					{
						if (fread(buffer, 1, len, f) == len)
						{
							buffer[len] = 0;
							numextraargs = 0;
							pp = buffer;
							do
							{
								if (numextraargs == sizeof(suppargs)/sizeof(suppargs[0]))
									break;
								while (*pp == ' ' || *pp == '\r' || *pp == '\n')
								{
									pp++;
								}
								if (*pp == '\"')
								{
									pp++;
									pps = pp;
									while (*pp && *pp != '\"')
										pp++;
								}
								else
								{
									pps = pp;
									while (*pp && !(*pp == ' ' || *pp == '\r' || *pp == '\n'))
									{
										pp++;
									}
								}
								if (*pp)
									*pp++ = 0;
								suppargs[numextraargs] = pps;
								numextraargs++;
							} while (*pp);
							ParseCommandList(w, numextraargs, suppargs);
						}
						else
						{
							printf("failure reading source file\n");
						}
						free(buffer);
					}
					else
					{
						printf("out of memory\n");
					}
					fclose(f);
				}
				else
				{
					printf("couldn't open \"%s\"\n", *cmds);
				}
			}
			else if ((*cmds)[1] == 'o')
			{
				numcmds--;
				cmds++;

				if (!strchr(*cmds, '.'))
				{
					snprintf(w->output, sizeof(w->output)-1, "%s.qvm", *cmds);
				}
				else
				{
					strncpy(w->output, *cmds, sizeof(w->output)-1);
					w->output[sizeof(w->output)-1] = 0;
				}
			}
			else if ((*cmds)[1] == 'v')
				w->verbose = 1;
			else
			{
				printf("Unrecognised command\n");
				return 1;
			}
		}
		else
		{
			//just a src file
			t = realloc(w->srcfilelist, sizeof(char*)*(w->numsrcfiles+1));
			if (!t)
			{	//if realloc fails, the old pointer is still valid
				printf("out of memory\n");
				return 1;
			}
			w->srcfilelist = t;
			w->srcfilelist[w->numsrcfiles] = malloc(strlen(*cmds)+1);
			if (!w->srcfilelist[w->numsrcfiles])
			{
				printf("out of memory\n");
				return 1;
			}
			strcpy(w->srcfilelist[w->numsrcfiles], *cmds);
			w->numsrcfiles++;
		}
		numcmds--;
		cmds++;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i;
	struct workload *w;

	if (!(w = malloc(sizeof(*w))))
	{
		printf("out of memory (REALLY EARLY!!!)\n");
	}
	else
	{
		w->numsrcfiles = 0;
		w->srcfilelist = NULL;
		strcpy(w->output, "q3asm2.qvm");	//fill in the default options

		if (ParseCommandList(w, argc-1, argv+1))
		{
			printf("Syntax is: q3asm2 [-o <output>] <files>\n");
			printf("       or: q3asm2 -f <listfile>\n");
			printf("or any crazy recursive mixture of the two\n");
		}
		else
		{
			printf("output file: %s\n", w->output);
			printf("%i files\n", w->numsrcfiles);


			Assemble(w);
		}

		for (i = 0; i < w->numsrcfiles; i++)
		{
			free(w->srcfilelist[i]);
		}
		if (w->srcfilelist)
			free(w->srcfilelist);

		free(w);
	}
	return 0;
}

