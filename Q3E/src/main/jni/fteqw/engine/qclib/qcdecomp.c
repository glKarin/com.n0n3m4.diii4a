#if !defined(MINIMAL) && !defined(OMIT_QCC)

//decompiling a progs should normally be done by walking the function table and emitting each def leading up to the one that refers to the function in question.
//this of course assumes strict ordering

//#include "qcc.h"
#include "progsint.h"
#include "setjmp.h"

#define	MAX_PARMS	8

// I put the following here to resolve "undefined reference to `__imp__vsnprintf'" with MinGW64 ~ Moodles
#if 0//def _WIN32
	#if (_MSC_VER >= 1400)
		//with MSVC 8, use MS extensions
		#define snprintf linuxlike_snprintf_vc8
		int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...) LIKEPRINTF(3);
		#define vsnprintf(a, b, c, d) vsnprintf_s(a, b, _TRUNCATE, c, d)
	#else
		//msvc crap
		#define snprintf linuxlike_snprintf
		int VARGS linuxlike_snprintf(char *buffer, int size, const char *format, ...) LIKEPRINTF(3);
		#define vsnprintf linuxlike_vsnprintf
		int VARGS linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr);
	#endif
#endif

typedef struct QCC_type_s
{
	etype_t			type;

	struct QCC_type_s	*next;
// function types are more complex
	struct QCC_type_s	*aux_type;	// return type or field type
	int				num_parms;	// -1 = variable args
//	struct QCC_type_s	*parm_types[MAX_PARMS];	// only [num_parms] allocated

	int ofs;	//inside a structure.
	int size;
	char *name;

} QCC_type_t;


extern QCC_type_t	*qcc_typeinfo;
extern int numtypeinfos;
extern int maxtypeinfos;
extern QCC_type_t	*type_void;// = {ev_void/*, &def_void*/};
extern QCC_type_t	*type_string;// = {ev_string/*, &def_string*/};
extern QCC_type_t	*type_float;// = {ev_float/*, &def_float*/};
extern QCC_type_t	*type_vector;// = {ev_vector/*, &def_vector*/};
extern QCC_type_t	*type_entity;// = {ev_entity/*, &def_entity*/};
extern QCC_type_t	*type_field;// = {ev_field/*, &def_field*/};
extern QCC_type_t	*type_function;// = {ev_function/*, &def_function*/,NULL,&type_void};
// type_function is a void() function used for state defs
extern QCC_type_t	*type_pointer;// = {ev_pointer/*, &def_pointer*/};
extern QCC_type_t	*type_integer;// = {ev_integer/*, &def_integer*/};
extern QCC_type_t	*type_floatpointer;
extern QCC_type_t	*type_intpointer;

extern QCC_type_t	*type_floatfield;// = {ev_field/*, &def_field*/, NULL, &type_float};
QCC_type_t *QCC_PR_NewType (char *name, int basictype, pbool typedefed);

#if 0

jmp_buf decompilestatementfailure;

QCC_type_t **ofstype;
qbyte *ofsflags;

int SafeOpenWrite (char *filename, int maxsize);
void SafeWrite(int hand, void *buf, long count);
int SafeSeek(int hand, int ofs, int mode);
void SafeClose(int hand);
void VARGS writes(int hand, char *msg, ...)
{
	va_list va;
	char buf[4192];

	va_start(va, msg);
	Q_vsnprintf (buf,sizeof(buf)-1, msg, va);
	va_end(va);

	SafeWrite(hand, buf, strlen(buf));
};

ddef16_t *ED_GlobalAtOfs16 (progfuncs_t *progfuncs, int ofs);
char *VarAtOfs(progfuncs_t *progfuncs, int ofs)
{
	static char buf [4192];
	ddef16_t *def;
	int typen;

	if (ofsflags[ofs]&8)
		def = ED_GlobalAtOfs16(progfuncs, ofs);
	else
		def = NULL;
	if (!def)
	{
		if (ofsflags[ofs]&3)
		{
			if (ofstype[ofs])
				sprintf(buf, "_v_%s_%i", ofstype[ofs]->name, ofs);
			else
				sprintf(buf, "_v_%i", ofs);
		}
		else
		{
			if (ofstype[ofs])
			{
				typen = ofstype[ofs]->type;
				goto evaluateimmediate;
			}
			else
				sprintf(buf, "_c_%i", ofs);
		}
		return buf;
	}
	if (!def->s_name[progfuncs->funcs.stringtable] || !strcmp(progfuncs->funcs.stringtable+def->s_name, "IMMEDIATE"))
	{
		if (current_progstate->types)
			typen = current_progstate->types[def->type & ~DEF_SHARED].type;
		else
			typen = def->type & ~(DEF_SHARED|DEF_SAVEGLOBAL);

evaluateimmediate:
//		return PR_UglyValueString(def->type, (eval_t *)&current_progstate->globals[def->ofs]);
		switch(typen)
		{
		case ev_float:
			sprintf(buf, "%f", G_FLOAT(ofs));
			return buf;
		case ev_vector:
			sprintf(buf, "\'%f %f %f\'", G_FLOAT(ofs), G_FLOAT(ofs+1), G_FLOAT(ofs+2));
			return buf;
		case ev_string:
			{
				char *s, *s2;
				s = buf;
				*s++ = '\"';
				s2 = pr_strings+G_INT(ofs);


				if (s2)
				while(*s2)
				{
					if (*s2 == '\n')
					{
						*s++ = '\\';
						*s++ = 'n';
						s2++;
					}
					else if (*s2 == '\"')
					{
						*s++ = '\\';
						*s++ = '\"';
						s2++;
					}
					else if (*s2 == '\t')
					{
						*s++ = '\\';
						*s++ = 't';
						s2++;
					}
					else
						*s++=*s2++;
				}
				*s++ = '\"';
				*s++ = '\0';
			}
			return buf;
		case ev_pointer:
			sprintf(buf, "_c_pointer_%i", ofs);
			return buf;
		default:
			sprintf(buf, "_c_%i", ofs);
			return buf;
		}
	}
	return def->s_name+progfuncs->funcs.stringtable;
}


int file;

int ImmediateReadLater(progfuncs_t *progfuncs, progstate_t *progs, unsigned int ofs, int firstst)
{
	dstatement16_t *st;
	if (ofsflags[ofs] & 8)
		return false;	//this is a global/local/pramater, not a temp
	if (!(ofsflags[ofs] & 3))
		return false;	//this is a constant.
	for (st = &((dstatement16_t*)progs->statements)[firstst]; ; st++,firstst++)
	{	//if written, return false, if read, return true.
		if (st->op >= OP_CALL0 && st->op <= OP_CALL8)
		{
			if (ofs == OFS_RETURN)
				return false;
			if (ofs < OFS_PARM0 + 3*((unsigned int)st->op - OP_CALL0))
				return true;
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
		{
			if (ofs == st->b)
				return false;
			if (ofs == st->a)
				return true;
		}
		else
		{
			if (st->a == ofs)
				return true;
			if (st->b == ofs)
				return true;
			if (st->c == ofs)
				return false;
		}

		if (st->op == OP_DONE || st->op == OP_RETURN)	//we missed our chance. (return/done ends any code coherancy).
			return false;
	}
	return false;
}
int ProductReadLater(progfuncs_t *progfuncs, progstate_t *progs, int stnum)
{
	dstatement16_t *st;
	st = &((dstatement16_t*)progs->statements)[stnum];
	if (pr_opcodes[st->op].priority == -1)
	{
		if (st->op >= OP_CALL0 && st->op <= OP_CALL7)
			return ImmediateReadLater(progfuncs, progs, OFS_RETURN, stnum+1);
		return false;//these don't have products...
	}

	if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
		return ImmediateReadLater(progfuncs, progs, st->b, stnum+1);
	else
		return ImmediateReadLater(progfuncs, progs, st->c, stnum+1);
}

void WriteStatementProducingOfs(progfuncs_t *progfuncs, progstate_t *progs, int lastnum, int firstpossible, int ofs)	//recursive, works backwards
{
	int i;
	dstatement16_t *st;
	ddef16_t *def;
	if (ofs == 0)
		longjmp(decompilestatementfailure, 1);
	for (; lastnum >= firstpossible; lastnum--)
	{
		st = &((dstatement16_t*)progs->statements)[lastnum];
		if (st->op >= OP_CALL0 && st->op < OP_CALL7)
		{
			if (ofs != OFS_RETURN)
				continue;
			WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->a);
			writes(file, "(");
			for (i = 0; i < st->op - OP_CALL0; i++)
			{
				WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, OFS_PARM0 + i*3);
				if (i != st->op - OP_CALL0-1)
					writes(file, ", ");
			}
			writes(file, ")");
			return;
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
		{
			if (st->b != ofs)
				continue;
			if (!ImmediateReadLater(progfuncs, progs, st->b, lastnum+1))
			{
				writes(file, "(");
				WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->b);
				writes(file, " ");
				writes(file, pr_opcodes[st->op].name);
				writes(file, " ");
				WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->a);
				writes(file, ")");
				return;
			}
			WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->a);
			return;
		}
		else
		{
			if (st->c != ofs)
				continue;

			if (!ImmediateReadLater(progfuncs, progs, st->c, lastnum+1))
			{
				WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->c);
				writes(file, " = ");
			}
			writes(file, "(");
			WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->a);

			if (!strcmp(pr_opcodes[st->op].name, "."))
				writes(file, pr_opcodes[st->op].name);	//extra spaces around .s are ugly.
			else
			{
				writes(file, " ");
				writes(file, pr_opcodes[st->op].name);
				writes(file, " ");
			}
			WriteStatementProducingOfs(progfuncs, progs, lastnum-1, firstpossible, st->b);
			writes(file, ")");
			return;
		}
	}

	def = ED_GlobalAtOfs16(progfuncs, ofs);
	if (def)
	{
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable, "IMMEDIATE"))
			writes(file, "%s", VarAtOfs(progfuncs, ofs));
		else
			writes(file, "%s", progfuncs->funcs.stringtable+def->s_name);
	}
	else
		writes(file, "%s", VarAtOfs(progfuncs, ofs));
//		longjmp(decompilestatementfailure, 1);
}

int WriteStatement(progfuncs_t *progfuncs, progstate_t *progs, int stnum, int firstpossible)
{
	int count, skip;
	dstatement16_t *st;
	st = &((dstatement16_t*)progs->statements)[stnum];
	switch(st->op)
	{
	case OP_IFNOT_I:
		count = (signed short)st->b;
		writes(file, "if (");
		WriteStatementProducingOfs(progfuncs, progs, stnum, firstpossible, st->a);
		writes(file, ")\r\n");
		writes(file, "{\r\n");
		firstpossible = stnum+1;
		count--;
		stnum++;
		while(count)
		{
			if (ProductReadLater(progfuncs, progs, stnum))
			{
				count--;
				stnum++;
				continue;
			}
			skip = WriteStatement(progfuncs, progs, stnum, firstpossible);
			count-=skip;
			stnum+=skip;
		}
		writes(file, "}\r\n");
		st = &((dstatement16_t*)progs->statements)[stnum];
		if (st->op == OP_GOTO)
		{
			count = (signed short)st->b;
			count--;
			stnum++;

			writes(file, "else\r\n");
			writes(file, "{\r\n");
			while(count)
			{
				if (ProductReadLater(progfuncs, progs, stnum))
				{
					count--;
					stnum++;
					continue;
				}
				skip = WriteStatement(progfuncs, progs, stnum, firstpossible);
				count-=skip;
				stnum+=skip;
			}
			writes(file, "}\r\n");
		}
		break;
	case OP_IF_I:
		longjmp(decompilestatementfailure, 1);
		break;
	case OP_GOTO:
		longjmp(decompilestatementfailure, 1);
		break;
	case OP_RETURN:
	case OP_DONE:
		if (st->a)
			WriteStatementProducingOfs(progfuncs, progs, stnum-1, firstpossible, st->a);
		break;
	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
		WriteStatementProducingOfs(progfuncs, progs, stnum, firstpossible, OFS_RETURN);
		writes(file, ";\r\n");
		break;
	default:
		if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
			WriteStatementProducingOfs(progfuncs, progs, stnum, firstpossible, st->b);
		else
			WriteStatementProducingOfs(progfuncs, progs, stnum, firstpossible, st->c);
		writes(file, ";\r\n");
		break;
	}

	return 1;
}

void WriteAsmStatements(progfuncs_t *progfuncs, progstate_t *progs, int num, int f, char *functionname)
{
	int stn = progs->functions[num].first_statement;
	QCC_opcode_t *op;
	dstatement16_t *st = NULL;
	eval_t *v;

	ddef16_t *def;
	int ofs,i;

	if (!functionname && stn<0)
	{
		//we wrote this one...
		return;
	}

	if (stn>=0)
	{
		for (stn = progs->functions[num].first_statement; stn < (signed int)pr_progs->numstatements; stn++)
		{
			st = &((dstatement16_t*)progs->statements)[stn];
			if (st->op == OP_DONE || st->op == OP_RETURN)
			{
				if (!st->a)
					writes(f, "void(");
				else if (ofstype[st->a])
				{
					writes(f, "%s", ofstype[st->a]->name);
					writes(f, "(");
				}
				else
					writes(f, "function(");
				break;
			}
		}
		st=NULL;
		stn = progs->functions[num].first_statement;
	}
	else
		writes(f, "function(");
	for (ofs = progs->functions[num].parm_start, i = 0; i < progs->functions[num].numparms; i++, ofs+=progs->functions[num].parm_size[i])
	{
		ofsflags[ofs] |= 4;

		def = ED_GlobalAtOfs16(progfuncs, ofs);
		if (def && stn>=0)
		{
			if (st)
				writes(f, ", ");
			st = (void *)0xffff;

			if (!def->s_name[progfuncs->funcs.stringtable])
			{
				char mem[64];
				sprintf(mem, "_p_%i", def->ofs);
				def->s_name = (char*)malloc(strlen(mem)+1)-progfuncs->funcs.stringtable;
				strcpy(def->s_name+progfuncs->funcs.stringtable, mem);
			}

			if (current_progstate->types)
				writes(f, "%s %s", current_progstate->types[def->type&~(DEF_SHARED|DEF_SAVEGLOBAL)].name, def->s_name);
			else
				switch(def->type&~(DEF_SHARED|DEF_SAVEGLOBAL))
				{
				case ev_string:
					writes(f, "%s %s", "string", progfuncs->funcs.stringtable+def->s_name);
					break;
				case ev_float:
					writes(f, "%s %s", "float", progfuncs->funcs.stringtable+def->s_name);
					break;
				case ev_entity:
					writes(f, "%s %s", "entity", progfuncs->funcs.stringtable+def->s_name);
					break;
				case ev_vector:
					writes(f, "%s %s", "vector", progfuncs->funcs.stringtable+def->s_name);
					break;
				default:
					writes(f, "%s %s", "randomtype", progfuncs->funcs.stringtable+def->s_name);
					break;
				}
		}
	}
	for (ofs = progs->functions[num].parm_start+progs->functions[num].numparms, i = progs->functions[num].numparms; i < progs->functions[num].locals; i++, ofs+=1)
		ofsflags[ofs] |= 4;

	if (!progfuncs->funcs.stringtable[progs->functions[num].s_name])
	{
		char mem[64];
		if (!functionname)
		{
			sprintf(mem, "_bi_%i", num);
			progs->functions[num].s_name = (char*)malloc(strlen(mem)+1)-progfuncs->funcs.stringtable;
			strcpy(progs->functions[num].s_name+progfuncs->funcs.stringtable, mem);
		}
		else
		{
			progs->functions[num].s_name = (char*)malloc(strlen(functionname)+1)-progfuncs->funcs.stringtable;
			strcpy(progs->functions[num].s_name+progfuncs->funcs.stringtable, functionname);
		}
	}

	writes(f, ") %s", progfuncs->funcs.stringtable+progs->functions[num].s_name);

	if (stn < 0)
	{
		stn*=-1;
		writes(f, " = #%i;\r\n", stn);
/*
		for (ofs = progs->functions[num].parm_start, i = 0; i < progs->functions[num].numparms; i++, ofs+=progs->functions[num].parm_size[i])
		{
			def = ED_GlobalAtOfs16(progfuncs, ofs);
			if (def)
			{
				def->ofs = 0xffff;

				if (progs->types)
				{
					if (progs->types[def->type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type == ev_vector)
					{
						def = ED_GlobalAtOfs16(progfuncs, ofs);
						def->ofs = 0xffff;
						def = ED_GlobalAtOfs16(progfuncs, ofs+1);
						def->ofs = 0xffff;
						def = ED_GlobalAtOfs16(progfuncs, ofs+2);
						def->ofs = 0xffff;
					}
				}
				else if ((def->type & (~(DEF_SHARED|DEF_SAVEGLOBAL))) == ev_vector)
				{
					def = ED_GlobalAtOfs16(progfuncs, ofs);
					def->ofs = 0xffff;
					def = ED_GlobalAtOfs16(progfuncs, ofs+1);
					def->ofs = 0xffff;
					def = ED_GlobalAtOfs16(progfuncs, ofs+2);
					def->ofs = 0xffff;
				}
			}
		}
		*/
		return;
	}

	if (functionname)	//parsing defs
	{
		writes(f, ";\r\n");
		return;
	}

	if (setjmp(decompilestatementfailure))
	{
		writes(f, "*/\r\n");
		writes(f, " = asm {\r\n");

		stn = progs->functions[num].first_statement;
		for (ofs = progs->functions[num].parm_start+progs->functions[num].numparms, i = progs->functions[num].numparms; i < progs->functions[num].locals; i++, ofs+=1)
		{
			def = ED_GlobalAtOfs16(progfuncs, ofs);
			if (def)
			{
				v = (eval_t *)&((int *)progs->globals)[def->ofs];
				if (current_progstate->types)
					writes(f, "\tlocal %s %s;\r\n", current_progstate->types[def->type&~(DEF_SHARED|DEF_SAVEGLOBAL)].name, def->s_name);
				else
				{
					if (!progfuncs->funcs.stringtable[def->s_name])
					{
						char mem[64];
						sprintf(mem, "_l_%i", def->ofs);
						def->s_name = (char*)malloc(strlen(mem)+1)-progfuncs->funcs.stringtable;
						strcpy(def->s_name+progfuncs->funcs.stringtable, mem);
					}

					switch(def->type&~(DEF_SHARED|DEF_SAVEGLOBAL))
					{
					case ev_string:
						writes(f, "\tlocal %s %s;\r\n", "string", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_float:
						writes(f, "\tlocal %s %s;\r\n", "float", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_entity:
						writes(f, "\tlocal %s %s;\r\n", "entity", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_vector:
						if (v->_vector[0] || v->_vector[1] || v->_vector[2])
							writes(f, "\tlocal vector %s = '%f %f %f';\r\n", progfuncs->funcs.stringtable+def->s_name, v->_vector[0], v->_vector[1], v->_vector[2]);
						else
							writes(f, "\tlocal %s %s;\r\n", "vector", progfuncs->funcs.stringtable+def->s_name);
						ofs+=2;	//skip floats;
						break;
					default:
						writes(f, "\tlocal %s %s;\r\n", "randomtype", progfuncs->funcs.stringtable+def->s_name);
						break;
					}
				}
			}
		}

		while(1)
		{
			st = &((dstatement16_t*)progs->statements)[stn];
			if (!st->op)	//end of function statement!
				break;
			op = &pr_opcodes[st->op];
			writes(f, "\t%s", op->opname);

			if (op->priority==-1&&op->associative==ASSOC_RIGHT)	//last param is a goto
			{
				if (op->type_b == &type_void)
				{
					if (st->a)
						writes(f, " %i", (signed short)st->a);
				}
				else if (op->type_c == &type_void)
				{
					if (st->a)
						writes(f, " %s", VarAtOfs(progfuncs, st->a));
					if (st->b)
						writes(f, " %i", (signed short)st->b);
				}
				else
				{
					if (st->a)
						writes(f, " %s", VarAtOfs(progfuncs, st->a));
					if (st->b)
						writes(f, " %s", VarAtOfs(progfuncs, st->b));
					if (st->c)	//rightness means it uses a as c
						writes(f, " %i", (signed short)st->c);
				}
			}
			else
			{
				if (st->a)
				{
					if (op->type_a == NULL)
						writes(f, " %i", (signed short)st->a);
					else
						writes(f, " %s", VarAtOfs(progfuncs, st->a));
				}
				if (st->b)
				{
					if (op->type_b == NULL)
						writes(f, " %i", (signed short)st->b);
					else
						writes(f, " %s", VarAtOfs(progfuncs, st->b));
				}
				if (st->c && op->associative != ASSOC_RIGHT)	//rightness means it uses a as c
				{
					if (op->type_c == NULL)
						writes(f, " %i", (signed short)st->c);
					else
						writes(f, " %s", VarAtOfs(progfuncs, st->c));
				}
			}

			writes(f, ";\r\n");

			stn++;
		}
	}
	else
	{
		if (!strcmp(progfuncs->funcs.stringtable+progs->functions[num].s_name, "SUB_Remove"))
			file = 0;
		file = f;

		writes(f, "/*\r\n");

		writes(f, " =\r\n{\r\n");

		for (ofs = progs->functions[num].parm_start+progs->functions[num].numparms, i = progs->functions[num].numparms; i < progs->functions[num].locals; i++, ofs+=1)
		{
			def = ED_GlobalAtOfs16(progfuncs, ofs);
			if (def)
			{
				v = (eval_t *)&((int *)progs->globals)[def->ofs];
				if (current_progstate->types)
					writes(f, "\tlocal %s %s;\r\n", current_progstate->types[def->type&~(DEF_SHARED|DEF_SAVEGLOBAL)].name, def->s_name);
				else
				{
					if (!def->s_name[progfuncs->funcs.stringtable])
					{
						char mem[64];
						sprintf(mem, "_l_%i", def->ofs);
						def->s_name = (char*)malloc(strlen(mem)+1)-progfuncs->funcs.stringtable;
						strcpy(def->s_name+progfuncs->funcs.stringtable, mem);
					}

					switch(def->type&~(DEF_SHARED|DEF_SAVEGLOBAL))
					{
					case ev_string:
						writes(f, "\tlocal %s %s;\r\n", "string", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_float:
						writes(f, "\tlocal %s %s;\r\n", "float", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_entity:
						writes(f, "\tlocal %s %s;\r\n", "entity", progfuncs->funcs.stringtable+def->s_name);
						break;
					case ev_vector:
						if (v->_vector[0] || v->_vector[1] || v->_vector[2])
							writes(f, "\tlocal vector %s = '%f %f %f';\r\n", progfuncs->funcs.stringtable+def->s_name, v->_vector[0], v->_vector[1], v->_vector[2]);
						else
							writes(f, "\tlocal %s %s;\r\n", "vector",progfuncs->funcs.stringtable+def->s_name);
						ofs+=2;	//skip floats;
						break;
					default:
						writes(f, "\tlocal %s %s;\r\n", "randomtype", progfuncs->funcs.stringtable+def->s_name);
						break;
					}
				}
			}
		}


		for (stn = progs->functions[num].first_statement; stn < (signed int)pr_progs->numstatements; stn++)
		{
			if (ProductReadLater(progfuncs, progs, stn))
				continue;

			st = &((dstatement16_t*)progs->statements)[stn];
			if (!st->op)
				break;
			WriteStatement(progfuncs, progs, stn, progs->functions[num].first_statement);
		}

		longjmp(decompilestatementfailure, 1);
	}
	writes(f, "};\r\n");
}

void FigureOutTypes(progfuncs_t *progfuncs)
{
	ddef16_t		*def;
	QCC_opcode_t *op;
	unsigned int i,p;
	dstatement16_t *st;

	int parmofs[8];

	ofstype		= realloc(ofstype,		sizeof(*ofstype)*65535);
	ofsflags	= realloc(ofsflags,	sizeof(*ofsflags)*65535);

	maxtypeinfos=256;
	qcc_typeinfo = (void *)realloc(qcc_typeinfo, sizeof(QCC_type_t)*maxtypeinfos);
	numtypeinfos = 0;

	memset(ofstype,		0, sizeof(*ofstype)*65535);
	memset(ofsflags,	0, sizeof(*ofsflags)*65535);

	type_void = QCC_PR_NewType("void", ev_void, true);
	type_string = QCC_PR_NewType("string", ev_string, true);
	type_float = QCC_PR_NewType("float", ev_float, true);
	type_vector = QCC_PR_NewType("vector", ev_vector, true);
	type_entity = QCC_PR_NewType("entity", ev_entity, true);
	type_field = QCC_PR_NewType("field", ev_field, false);
	type_function = QCC_PR_NewType("function", ev_function, false);
	type_pointer = QCC_PR_NewType("pointer", ev_pointer, false);
	type_integer = QCC_PR_NewType("integer", ev_integer, true);

//	type_variant = QCC_PR_NewType("__variant", ev_variant);

	type_floatfield = QCC_PR_NewType("fieldfloat", ev_field, false);
	type_floatfield->aux_type = type_float;
	type_pointer->aux_type = QCC_PR_NewType("pointeraux", ev_float, false);

	type_function->aux_type = type_void;

	for (i = 0,st = pr_statements16; i < pr_progs->numstatements; i++,st++)
	{
		op = &pr_opcodes[st->op];
		if (st->op >= OP_CALL1 && st->op <= OP_CALL8)
		{
			for (p = 0; p < (unsigned int)st->op-OP_CALL0; p++)
			{
				ofstype[parmofs[p]] = ofstype[OFS_PARM0+p*3];
			}
		}
		else if (op->associative == ASSOC_RIGHT)
		{	//assignment
			ofsflags[st->b] |= 1;
			if (st->b >= OFS_PARM0 && st->b < RESERVED_OFS)
				parmofs[(st->b-OFS_PARM0)/3] = st->a;

//			if (st->op != OP_STORE_F || st->b>RESERVED_OFS)	//optimising compilers fix the OP_STORE_V, it's the storef that becomes meaningless (this is the only time that we need this sort of info anyway)
			{
				if (op->type_c && op->type_c != &type_void)
					ofstype[st->a] = *op->type_c;
				if (op->type_b && op->type_b != &type_void)
					ofstype[st->b] = *op->type_b;
			}
		}
		else if (op->type_c)
		{
			ofsflags[st->c] |= 2;

			if (st->c >= OFS_PARM0 && st->b < RESERVED_OFS)	//too complicated
				parmofs[(st->b-OFS_PARM0)/3] = 0;

//			if (st->op != OP_STORE_F || st->b>RESERVED_OFS)	//optimising compilers fix the OP_STORE_V, it's the storef that becomes meaningless (this is the only time that we need this sort of info anyway)
			{
				if (op->type_a && op->type_a != &type_void)
					ofstype[st->a] = *op->type_a;
				if (op->type_b && op->type_b != &type_void)
					ofstype[st->b] = *op->type_b;
				if (op->type_c && op->type_c != &type_void)
					ofstype[st->c] = *op->type_c;
			}
		}
	}


	for (i=0 ; i<pr_progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs16[i];
		ofsflags[def->ofs] |= 8;
		switch(def->type)
		{
		case ev_float:
			ofstype[def->ofs] = type_float;
			break;
		case ev_string:
			ofstype[def->ofs] = type_string;
			break;
		case ev_vector:
			ofstype[def->ofs] = type_vector;
			break;
		default:
			break;
		}
	}
}

pbool PDECL QC_Decompile(pubprogfuncs_t *ppf, char *fname)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	extern progfuncs_t *qccprogfuncs;
	unsigned int i;
	unsigned int fld=0;
	eval_t *v;
//	char *filename;
	int f, type;

	progstate_t progs, *op;

	qccprogfuncs = progfuncs;
	op=current_progstate;

	if (!PR_ReallyLoadProgs(progfuncs, fname, &progs, false))
	{
		return false;
	}

	f=SafeOpenWrite("qcdtest/defs.qc", 1024*512);

	writes(f, "//Decompiled code can contain little type info.\r\n");

	FigureOutTypes(progfuncs);

	for (i = 1; i < progs.progs->numglobaldefs; i++)
	{
		if (!strcmp(progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name, "IMMEDIATE"))
			continue;

		if (ofsflags[pr_globaldefs16[i].ofs] & 4)
			continue;	//this is a local.

		if (current_progstate->types)
			type = progs.types[pr_globaldefs16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
		else
			type = pr_globaldefs16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
		v = (eval_t *)&((int *)progs.globals)[pr_globaldefs16[i].ofs];

		if (!progfuncs->funcs.stringtable[pr_globaldefs16[i].s_name])
		{
			char mem[64];
			if (ofsflags[pr_globaldefs16[i].ofs] & 3)
			{
				ofsflags[pr_globaldefs16[i].ofs] &= ~8;
				continue;	//this is a constant...
			}

			sprintf(mem, "_g_%i", pr_globaldefs16[i].ofs);
			pr_globaldefs16[i].s_name = (char*)malloc(strlen(mem)+1)-progfuncs->funcs.stringtable;
			strcpy(pr_globaldefs16[i].s_name+progfuncs->funcs.stringtable, mem);
		}

		switch(type)
		{
		case ev_void:
			writes(f, "void %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_string:
			if (v->string && *(pr_strings+v->_int))
				writes(f, "string %s = \"%s\";\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name, pr_strings+v->_int);
			else
				writes(f, "string %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_float:
			if (v->_float)
				writes(f, "float %s = %f;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name, v->_float);
			else
				writes(f, "float %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_vector:
			if (v->_vector[0] || v->_vector[1] || v->_vector[2])
				writes(f, "vector %s = '%f %f %f';\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name, v->_vector[0], v->_vector[1], v->_vector[2]);
			else
				writes(f, "vector %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			i+=3;//skip the floats
			break;
		case ev_entity:
			writes(f, "entity %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_field:
//wierd
			fld++;
			if (!v->_int)
				writes(f, "var ");
			switch(pr_fielddefs16[fld].type)
			{
			case ev_string:
				writes(f, ".string %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;

			case ev_float:
				writes(f, ".float %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;

			case ev_vector:
				writes(f, ".float %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;

			case ev_entity:
				writes(f, ".float %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;

			case ev_function:
				writes(f, ".void() %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;

			default:
				writes(f, "field %s;", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
				break;
			}
			if (v->_int)
				writes(f, "/* %i */", v->_int);
			writes(f, "\r\n");
			break;

		case ev_function:
//wierd
			WriteAsmStatements(progfuncs, &progs, ((int *)progs.globals)[pr_globaldefs16[i].ofs], f, pr_globaldefs16[i].s_name+progfuncs->funcs.stringtable);
			break;

		case ev_pointer:
			writes(f, "pointer %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_integer:
			writes(f, "integer %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;

		case ev_union:
			writes(f, "union %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		case ev_struct:
			writes(f, "struct %s;\r\n", progfuncs->funcs.stringtable+pr_globaldefs16[i].s_name);
			break;
		default:
			break;

		}
	}

	for (i = 0; i < progs.progs->numfunctions; i++)
	{
		WriteAsmStatements(progfuncs, &progs, i, f, NULL);
	}

	SafeClose(f);

	current_progstate=op;

	return true;
}
#endif

#endif
