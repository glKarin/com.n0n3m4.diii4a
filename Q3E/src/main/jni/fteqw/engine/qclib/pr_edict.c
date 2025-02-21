

#define PROGSUSED
struct edict_s;
#include "progsint.h"
//#include "crc.h"
#include "qcc.h"

#ifdef _WIN32
//this is windows  all files are written with this endian standard. we do this to try to get a little more speed.
#define NOENDIAN
#endif

#define qcc_iswhite(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == '\v')

pbool	ED_ParseEpair (progfuncs_t *progfuncs, size_t qcptr, unsigned int fldofs, int fldtype, char *s);

/*
=================
QC_ClearEdict

Sets everything to NULL
=================
*/
void PDECL QC_ClearEdict (pubprogfuncs_t *ppf, struct edict_s *ed)
{
//	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *e = (edictrun_t *)ed;
	int num = e->entnum;
	memset (e->fields, 0, e->fieldsize);
	e->ereftype = ER_ENTITY;
	e->entnum = num;
}

struct edict_s *PDECL ED_AllocIndex (pubprogfuncs_t *ppf, unsigned int num, pbool object, size_t extrasize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *e;

	unsigned int fields_size;

	if (num >= prinst.maxedicts)
	{
		externs->Sys_Error ("ED_AllocIndex: index %u exceeds limit of %u", num, prinst.maxedicts);
		return NULL;
	}
	if (!object)
	{
		while(sv_num_edicts < num)
		{	//fill in any holes
			e = (edictrun_t*)EDICT_NUM(progfuncs, sv_num_edicts);
			if (!e)
			{
				e = (edictrun_t*)ED_AllocIndex(&progfuncs->funcs, sv_num_edicts, object, extrasize);
				e->ereftype=ER_FREE;
			}
			sv_num_edicts++;
		}
		if (num >= sv_num_edicts)
			sv_num_edicts=num+1;
	}

	e = prinst.edicttable[num];
	if (!e)
	{
		e = (void*)externs->memalloc(externs->edictsize);
		prinst.edicttable[num] = e;
		memset(e, 0, externs->edictsize);
	}

	fields_size = object?0:prinst.fields_size;
	fields_size += extrasize;
	if (e->fieldsize != fields_size)
	{
		if (e->fields)
			progfuncs->funcs.AddressableFree(&progfuncs->funcs, e->fields);
		e->fields = progfuncs->funcs.AddressableAlloc(&progfuncs->funcs, fields_size);
		if (!e->fields)
			externs->Sys_Error ("ED_Alloc: Unable to allocate more field space");
		e->fieldsize = fields_size;

//		e->fields = PRAddressableExtend(progfuncs, NULL, fields_size, 0);
	}
	e->entnum = num;
	memset (e->fields, 0, e->fieldsize);

	e->ereftype = object?ER_OBJECT:ER_ENTITY;

	if (externs->entspawn)
		externs->entspawn((struct edict_s *) e, false);
	return (struct edict_s*)e;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
struct edict_s *PDECL ED_Alloc (pubprogfuncs_t *ppf, pbool object, size_t extrasize)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	unsigned int			i;
	edictrun_t		*e;

	if (object)
	{
		//objects are allocated at the end (won't be networked, so this reduces issues with users on old protocols).
		//also they're potentially higher than num_edicts, which is handy.
		for ( i=prinst.maxedicts-1 ; i>0 ; i--)
		{
			e = (edictrun_t*)EDICT_NUM(progfuncs, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || (e->ereftype==ER_FREE && ( e->freetime < 2 || *externs->gametime - e->freetime > 0.5 ) ))
				return ED_AllocIndex(&progfuncs->funcs, i, object, extrasize);
		}
		externs->Sys_Error ("ED_Alloc: no free edicts (max is %i)", prinst.maxedicts);
	}

	//define this to wastefully allocate extra ents, to test network capabilities.
#define STEP 1//((i <= 32)?1:8)

	for ( i=0 ; i<sv_num_edicts ; i+=STEP)
	{
		e = (edictrun_t*)EDICT_NUM(progfuncs, i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e || (e->ereftype==ER_FREE && ( e->freetime < 2 || *externs->gametime - e->freetime > 0.5 ) ))
			return ED_AllocIndex(&progfuncs->funcs, i, object, extrasize);
	}

	if (i >= prinst.maxedicts-1)	//try again, but use timed out ents.
	{
		for ( i=0 ; i<sv_num_edicts ; i+=STEP)
		{
			e = (edictrun_t*)EDICT_NUM(progfuncs, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || (e->ereftype==ER_FREE))
				return ED_AllocIndex(&progfuncs->funcs, i, object, extrasize);
		}

		if (i >= prinst.maxedicts-2)
		{
			PR_RunWarning(&progfuncs->funcs, "Running out of edicts\n");
		}
		if (i >= prinst.maxedicts-1)
		{
			size_t size;
			char *buf;
			buf = PR_SaveEnts(&progfuncs->funcs, NULL, &size, 0, 0);
			progfuncs->funcs.parms->WriteFile("edalloc.dump", buf, size);
			externs->Sys_Error ("ED_Alloc: no free edicts (max is %i)", prinst.maxedicts);
		}
	}

	return ED_AllocIndex(&progfuncs->funcs, i, object, extrasize);
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void PDECL ED_Free (pubprogfuncs_t *ppf, struct edict_s *ed, pbool instant)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *e = (edictrun_t *)ed;
//	SV_UnlinkEdict (ed);		// unlink from world bsp

	if (e->ereftype == ER_FREE)	//this happens on start.bsp where an onlyregistered trigger killtargets itself (when all of this sort die after 1 trigger anyway).
	{
		if (prinst.pr_depth)
			externs->Printf("Tried to free free entity within %s\n", prinst.pr_xfunction->s_name+progfuncs->funcs.stringtable);
		else
			externs->Printf("Engine tried to free free entity\n");
//		if (developer.value == 1)
//			progfuncs->funcs.pr_trace = true;
		return;
	}

	if (externs->entcanfree)
		if (!externs->entcanfree(ed))	//can stop an ent from being freed.
			return;

	e->ereftype = ER_FREE;
	e->freetime = instant?0:(float)*externs->gametime;

/*
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
*/
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
ddef16_t *ED_GlobalAtOfs16 (progfuncs_t *progfuncs, int ofs)
{
	ddef16_t		*def;
	unsigned int			i;

	for (i=0 ; i<pr_progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs16[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}
ddef32_t *ED_GlobalAtOfs32 (progfuncs_t *progfuncs, unsigned int ofs)
{
	ddef32_t		*def;
	unsigned int			i;

	for (i=0 ; i<pr_progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs32[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
fdef_t *ED_FieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs)
{
//	ddef_t		*def;
	unsigned int			i;

	for (i=0 ; i<prinst.numfields ; i++)
	{
		if (prinst.field[i].ofs == ofs)
			return &prinst.field[i];
	}
	return NULL;
}
fdef_t *ED_ClassFieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs, const char *classname)
{
	int classnamelen = strlen(classname);
	unsigned int j;
	const char *mname;
	for (j = 0; j < prinst.numfields; j++)
	{
		if (prinst.field[j].ofs == ofs)
		{
			mname = prinst.field[j].name;
			if (!strncmp(mname, classname, classnamelen) && mname[classnamelen] == ':')
			{
				//okay, we have a match...
				return &prinst.field[j];
			}
		}
	}
	return ED_FieldAtOfs(progfuncs, ofs);
}
fdef_t *PDECL ED_FieldInfo (pubprogfuncs_t *ppf, unsigned int *count)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	*count = prinst.numfields;
	return prinst.field;
}
/*
============
ED_FindField
============
*/
fdef_t *ED_FindField (progfuncs_t *progfuncs, const char *name)
{
	unsigned int			i;

	for (i=0 ; i<prinst.numfields ; i++)
	{
		if (!strcmp(prinst.field[i].name, name) )
			return &prinst.field[i];
	}
	return NULL;
}


/*
============
ED_FindGlobal
============
*/
ddef16_t *ED_FindGlobal16 (progfuncs_t *progfuncs, const char *name)
{
	ddef16_t		*def;
	unsigned int			i;

	for (i=1 ; i<pr_progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs16[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
			return def;
	}
	return NULL;
}
ddef32_t *ED_FindGlobal32 (progfuncs_t *progfuncs, const char *name)
{
	ddef32_t		*def;
	unsigned int			i;

	for (i=1 ; i<pr_progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs32[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
			return def;
	}
	return NULL;
}

unsigned int ED_FindGlobalOfs (progfuncs_t *progfuncs, char *name)
{
	ddef16_t *d16;
	ddef32_t *d32;
	switch(current_progstate->structtype)
	{
	case PST_KKQWSV:
	case PST_DEFAULT:
		d16 = ED_FindGlobal16(progfuncs, name);
		return d16?d16->ofs:0;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		d32 = ED_FindGlobal32(progfuncs, name);
		return d32?d32->ofs:0;
	default:
		externs->Sys_Error("ED_FindGlobalOfs - bad struct type");
	}
	return 0;
}

ddef16_t *ED_FindGlobalFromProgs16 (progfuncs_t *progfuncs, progstate_t *ps, const char *name)
{
	ddef16_t		*def;
	unsigned int			i;

	for (i=1 ; i<ps->progs->numglobaldefs ; i++)
	{
		def = &ps->globaldefs16[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
			return def;
	}
	return NULL;
}
ddef32_t *ED_FindGlobalFromProgs32 (progfuncs_t *progfuncs, progstate_t *ps, const char *name)
{
	ddef32_t		*def;
	unsigned int			i;

	for (i=1 ; i<ps->progs->numglobaldefs ; i++)
	{
		def = &ps->globaldefs32[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
			return def;
	}
	return NULL;
}

ddef16_t *ED_FindTypeGlobalFromProgs16 (progfuncs_t *progfuncs, progstate_t *ps, const char *name, int type)
{
	ddef16_t		*def;
	unsigned int			i;

	for (i=1 ; i<ps->progs->numglobaldefs ; i++)
	{
		def = &ps->globaldefs16[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
		{
			if (ps->types)
			{
				if (ps->types[def->type&~DEF_SAVEGLOBAL].type != type)
					continue;
			}
			else if ((def->type&(~DEF_SAVEGLOBAL)) != type)
				continue;
			return def;
		}
	}
	return NULL;
}


ddef32_t *ED_FindTypeGlobalFromProgs32 (progfuncs_t *progfuncs, progstate_t *ps, const char *name, int type)
{
	ddef32_t		*def;
	unsigned int			i;

	for (i=1 ; i<ps->progs->numglobaldefs ; i++)
	{
		def = &ps->globaldefs32[i];
		if (!strcmp(def->s_name+progfuncs->funcs.stringtable,name) )
		{
			if (ps->types)
			{
				if (ps->types[def->type&~DEF_SAVEGLOBAL].type != type)
					continue;
			}
			else if ((def->type&(~DEF_SAVEGLOBAL)) != (unsigned)type)
				continue;
			return def;
		}
	}
	return NULL;
}

unsigned int *ED_FindGlobalOfsFromProgs (progfuncs_t *progfuncs, progstate_t *ps, char *name, int type)
{
	ddef16_t		*def16;
	ddef32_t		*def32;
	static unsigned int pos;
	switch(ps->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		def16 = ED_FindTypeGlobalFromProgs16(progfuncs, ps, name, type);
		if (!def16)
			return NULL;
		pos = def16->ofs;
		return &pos;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		def32 = ED_FindTypeGlobalFromProgs32(progfuncs, ps, name, type);
		if (!def32)
			return NULL;
		return &def32->ofs;
	default:
		externs->Sys_Error("ED_FindGlobalOfsFromProgs - bad struct type");
	}
	return 0;
}

/*
============
ED_FindFunction
============
*/
mfunction_t *ED_FindFunction (progfuncs_t *progfuncs, const char *name, progsnum_t *prnum, progsnum_t fromprogs)
{
	mfunction_t		*func;
	unsigned int				i;
	char *sep;

	progsnum_t pnum;

	if (prnum)
	{
		sep = strchr(name, ':');
		if (sep)
		{
			pnum = atoi(name);
			name = sep+1;
		}
		else
		{
			if (fromprogs>=0)
				pnum = fromprogs;
			else
				pnum = prinst.pr_typecurrent;
		}
		*prnum = pnum;
	}
	else
		pnum = prinst.pr_typecurrent;

	if ((unsigned)pnum > (unsigned)prinst.maxprogs)
	{
		externs->Printf("Progsnum %"pPRIi" out of bounds\n", pnum);
		return NULL;
	}

	if (!pr_progstate[pnum].progs)
		return NULL;

	for (i=1 ; i<pr_progstate[pnum].progs->numfunctions ; i++)
	{
		func = &pr_progstate[pnum].functions[i];
		if (!strcmp(func->s_name+progfuncs->funcs.stringtable,name) )
			return func;
	}
	return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a human-readable type specific manner
if verbose, contains entity field listing etc too
=============
*/
char *PR_ValueString (progfuncs_t *progfuncs, etype_t type, eval_t *val, pbool verbose)
{
	static char	line[4096];
	fdef_t			*fielddef;
	mfunction_t	*f;

#ifdef DEF_SAVEGLOBAL
	type &= ~DEF_SAVEGLOBAL;
#endif

	if (current_progstate && pr_types)
		type = pr_types[type].type;

	if (!val)
		type = ev_void;

	switch (type)
	{
	case ev_struct:
		QC_snprintfz (line, sizeof(line), "struct");
		break;
	case ev_union:
		QC_snprintfz (line, sizeof(line), "union");
		break;
	case ev_string:
#ifndef QCGC
		if (((unsigned int)val->string & STRING_SPECMASK) == STRING_TEMP)
			return "<Stale Temporary String>";
		else
#endif
			QC_snprintfz (line, sizeof(line), "%s", PR_StringToNative(&progfuncs->funcs, val->string));
		break;
	case ev_entity:
		fielddef = ED_FindField(progfuncs, "classname");
		if (fielddef && (unsigned)val->edict < (unsigned)sv_num_edicts)
		{
			edictrun_t *ed;
			string_t *v;
			ed = (edictrun_t *)EDICT_NUM(progfuncs, val->edict);
			v = (string_t *)((char *)edvars(ed) + fielddef->ofs*4);
			QC_snprintfz (line, sizeof(line), "entity %i(%s)", val->edict, PR_StringToNative(&progfuncs->funcs, *v));
		}
		else
			QC_snprintfz (line, sizeof(line), "entity %i", val->edict);

		if (verbose && (unsigned)val->edict < (unsigned)sv_num_edicts)
		{
			struct edict_s *ed = EDICT_NUM(progfuncs, val->edict);
			size_t size = strlen(line);
			if (ed)
				PR_SaveEnt(&progfuncs->funcs, line, &size, sizeof(line), ed);
		}
		break;
	case ev_function:
		if (!val->function)
			QC_snprintfz (line, sizeof(line), "NULL function");
		else
		{
			if ((val->function & 0xff000000)>>24 >= prinst.maxprogs || !pr_progstate[(val->function & 0xff000000)>>24].functions)
				QC_snprintfz (line, sizeof(line), "Bad function %"pPRIi":%"pPRIi"", (val->function & 0xff000000)>>24, val->function & ~0xff000000);
			else
			{
				if ((val->function &~0xff000000) >= pr_progs->numfunctions)
					QC_snprintfz (line, sizeof(line), "bad function %"pPRIi":%"pPRIi"\n", (val->function & 0xff000000)>>24, val->function & ~0xff000000);
				else
				{
					f = pr_progstate[(val->function & 0xff000000)>>24].functions + (val->function & ~0xff000000);
					QC_snprintfz (line, sizeof(line), "%"pPRIi":%s()", (val->function & 0xff000000)>>24, f->s_name+progfuncs->funcs.stringtable);
				}
			}
		}
		break;
	case ev_field:
		fielddef = ED_FieldAtOfs (progfuncs,  val->_int + progfuncs->funcs.fieldadjust);
		if (!fielddef)
			QC_snprintfz (line, sizeof(line), ".??? (#%i)", val->_int);
		else
			QC_snprintfz (line, sizeof(line), ".%s (#%i)", fielddef->name, val->_int);
		break;
	case ev_void:
		QC_snprintfz (line, sizeof(line), "void type");
		break;
	case ev_float:
		QC_snprintfz (line, sizeof(line), "%g", val->_float);
		break;
	case ev_double:
		QC_snprintfz (line, sizeof(line), "%g", val->_double);
		break;
	case ev_integer:
		QC_snprintfz (line, sizeof(line), "%"pPRIi, val->_int);
		break;
	case ev_uint:
		QC_snprintfz (line, sizeof(line), "%"pPRIu, val->_uint);
		break;
	case ev_int64:
		QC_snprintfz (line, sizeof(line), "%"pPRIi64, val->i64);
		break;
	case ev_uint64:
		QC_snprintfz (line, sizeof(line), "%"pPRIu64, val->u64);
		break;
	case ev_vector:
		QC_snprintfz (line, sizeof(line), "'%g %g %g'", val->_vector[0], val->_vector[1], val->_vector[2]);
		break;
	case ev_pointer:
		QC_snprintfz (line, sizeof(line), "%#x", val->_int);
		{
//			int entnum;
//			int valofs;
		//FIXME: :/
//			entnum = ((qbyte *)val->edict - (qbyte *)sv_edicts) / pr_edict_size;
//			valofs = (int *)val->edict - (int *)edvars(EDICT_NUM(progfuncs, entnum));
//			fielddef = ED_FieldAtOfs (progfuncs, valofs );
//			if (fielddef)
//				sprintf(line, "ent%i.%s", entnum, fielddef->s_name);
		}
		break;
	case ev_accessor:
		QC_snprintfz (line, sizeof(line), "(accessor)");
		break;
	default:
		QC_snprintfz (line, sizeof(line), "(bad type %i)", type);
		break;
	}

	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char *PDECL PR_UglyValueString (pubprogfuncs_t *ppf, etype_t type, eval_t *val)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	static char	line[4096];
	fdef_t		*fielddef;
	mfunction_t	*f;
	int i, j;

#ifdef DEF_SAVEGLOBAL
	type &= ~DEF_SAVEGLOBAL;
#endif

//	if (pr_types)
//		type = pr_types[type].type;

	switch (type)
	{
	case ev_struct:
		sprintf (line, "structures cannot yet be saved");
		break;
	case ev_union:
		sprintf (line, "unions cannot yet be saved");
		break;
	case ev_string:
		{
			char *outs = line;
			int outb = sizeof(line)-2;
			const char *ins;
#ifndef QCGC
			if (((unsigned int)val->string & STRING_SPECMASK) == STRING_TEMP)
				return "<Stale Temporary String>";
			else
#endif
				ins = PR_StringToNative(&progfuncs->funcs, val->string);
			//markup the output string.
			while(*ins && outb > 0)
			{
				switch(*ins)
				{
				case '\n':
					*outs++ = '\\';
					*outs++ = 'n';
					ins++;
					outb-=2;
					break;
				case '\"':
					*outs++ = '\\';
					*outs++ = '"';
					ins++;
					outb-=2;
					break;
				case '\\':
					*outs++ = '\\';
					*outs++ = '\\';
					ins++;
					outb-=2;
					break;
				default:
					*outs++ = *ins++;
					outb--;
					break;
				}
			}
			*outs = 0;
		}
		break;
	case ev_entity:
		sprintf (line, "%i", val->_int);
		break;
	case ev_function:
		i = (val->function & 0xff000000)>>24;	//progs number
		if ((unsigned)i >= prinst.maxprogs || !pr_progstate[(unsigned)i].progs)
			sprintf (line, "BAD FUNCTION INDEX: %#"pPRIx"", val->function);
		else
		{
			j = (val->function & ~0xff000000);	//function number
			if ((unsigned)j >= pr_progstate[(unsigned)i].progs->numfunctions)
				sprintf(line, "%i:%s", i, "CORRUPT FUNCTION POINTER");
			else
			{
				f = pr_progstate[(unsigned)i].functions + j;
				sprintf (line, "%i:%s", i, f->s_name+progfuncs->funcs.stringtable);
			}
		}
		break;
	case ev_field:
		fielddef = ED_FieldAtOfs (progfuncs, val->_int + progfuncs->funcs.fieldadjust);
		if (fielddef)
			sprintf (line, "%s", fielddef->name);
		else
			sprintf (line, "bad field %i", type);
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		if (val->_float == (int)val->_float)
			sprintf (line, "%i", (int)val->_float);	//an attempt to cut down on the number of .000000 vars..
		else
			sprintf (line, "%f", val->_float);
		break;
	case ev_double:
		if (val->_double == (pint64_t)val->_double)
			sprintf (line, "%"pPRIi64, (pint64_t)val->_double);	//an attempt to cut down on the number of .000000 vars..
		else
			sprintf (line, "%f", val->_double);
		break;
	case ev_integer:
		sprintf (line, "%"pPRIi, val->_int);
		break;
	case ev_uint:
		sprintf (line, "%"pPRIu, val->_uint);
		break;
	case ev_int64:
		sprintf (line, "%"pPRIi64, val->i64);
		break;
	case ev_uint64:
		sprintf (line, "%"pPRIu64, val->u64);
		break;
	case ev_vector:
		if (val->_vector[0] == (int)val->_vector[0] && val->_vector[1] == (int)val->_vector[1] && val->_vector[2] == (int)val->_vector[2])
			sprintf (line, "%i %i %i", (int)val->_vector[0], (int)val->_vector[1], (int)val->_vector[2]);
		else
			sprintf (line, "%g %g %g", val->_vector[0], val->_vector[1], val->_vector[2]);
		break;
	case ev_pointer:
		QC_snprintfz (line, sizeof(line), "%#x", val->_int);
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}

	return line;
}

//compatible with Q1 (for savegames)
char *PR_UglyOldValueString (progfuncs_t *progfuncs, etype_t type, eval_t *val)
{
	static char	line[4096];
	fdef_t		*fielddef;
	mfunction_t	*f;

#ifdef DEF_SAVEGLOBAL
	type &= ~DEF_SAVEGLOBAL;
#endif

	if (pr_types)
		type = pr_types[type].type;

	switch (type)
	{
	case ev_struct:
		QC_snprintfz (line, sizeof(line), "structures cannot yet be saved");
		break;
	case ev_union:
		QC_snprintfz (line, sizeof(line), "unions cannot yet be saved");
		break;
	case ev_string:
		//FIXME: we should probably add markup. vanilla does _not_, so we can expect problems reloading anyway.
		QC_snprintfz (line, sizeof(line), "%s", PR_StringToNative(&progfuncs->funcs, val->string));
		break;
	case ev_entity:
		QC_snprintfz (line, sizeof(line), "%i", val->edict);
		break;
	case ev_function:
		f = pr_progstate[(val->function & 0xff000000)>>24].functions + (val->function & ~0xff000000);
		QC_snprintfz (line, sizeof(line), "%s", f->s_name+progfuncs->funcs.stringtable);
		break;
	case ev_field:
		fielddef = ED_FieldAtOfs (progfuncs, val->_int + progfuncs->funcs.fieldadjust);
		QC_snprintfz (line, sizeof(line), "%s", fielddef->name);
		break;
	case ev_void:
		QC_snprintfz (line, sizeof(line), "void");
		break;
	case ev_float:
		if (val->_float == (int)val->_float)
			QC_snprintfz (line, sizeof(line), "%i", (int)val->_float);	//an attempt to cut down on the number of .000000 vars..
		else
			QC_snprintfz (line, sizeof(line), "%f", val->_float);
		break;
	case ev_double:
		if (val->_double == (int)val->_double)
			QC_snprintfz (line, sizeof(line), "%i", (int)val->_double);	//an attempt to cut down on the number of .000000 vars..
		else
			QC_snprintfz (line, sizeof(line), "%f", val->_double);
		break;
	case ev_integer:
		QC_snprintfz (line, sizeof(line), "%"pPRIi, val->_int);
		break;
	case ev_uint:
		QC_snprintfz (line, sizeof(line), "%"pPRIu, val->_uint);
		break;
	case ev_int64:
		QC_snprintfz (line, sizeof(line), "%"pPRIi64, val->i64);
		break;
	case ev_uint64:
		QC_snprintfz (line, sizeof(line), "%"pPRIu64, val->u64);
		break;
	case ev_vector:
		if (val->_vector[0] == (int)val->_vector[0] && val->_vector[1] == (int)val->_vector[1] && val->_vector[2] == (int)val->_vector[2])
			QC_snprintfz (line, sizeof(line), "%i %i %i", (int)val->_vector[0], (int)val->_vector[1], (int)val->_vector[2]);
		else
			QC_snprintfz (line, sizeof(line), "%f %f %f", val->_vector[0], val->_vector[1], val->_vector[2]);
		break;
	case ev_pointer:
		QC_snprintfz (line, sizeof(line), "%#x", val->_int);
		break;
	default:
		QC_snprintfz (line, sizeof(line), "bad type %i", type);
		break;
	}

	return line;
}

char *PR_TypeString(progfuncs_t *progfuncs, etype_t type)
{
#ifdef DEF_SAVEGLOBAL
	type &= ~DEF_SAVEGLOBAL;
#endif

	if (pr_types)
		type = pr_types[type].type;

	switch (type)
	{
	case ev_struct:
		return "struct";
	case ev_union:
		return "union";
	case ev_string:
		return "string";
	case ev_entity:
		return "entity";
	case ev_function:
		return "function";
	case ev_field:
		return "field";
	case ev_void:
		return "void";
	case ev_float:
		return "float";
	case ev_double:
		return "double";
	case ev_vector:
		return "vector";
	case ev_integer:
		return "integer";
	case ev_uint:
		return "uint";
	case ev_int64:
		return "int64";
	case ev_uint64:
		return "uint64";
	default:
		return "BAD TYPE";
	}
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PR_GlobalString (progfuncs_t *progfuncs, int ofs, struct QCC_type_s **typehint)
{
	char	*s;
	int		i;
	ddef16_t	*def16;
	ddef32_t	*def32, def32tmp;
	void	*val;
	static char	line[128];

	switch (current_progstate->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		def16 = ED_GlobalAtOfs16(progfuncs, ofs);
		if (def16)
		{
			def32 = &def32tmp;
			def32->ofs = def16->ofs;
			def32->type = def16->type;
			def32->s_name = def16->s_name;
		}
		else
			def32 = NULL;
		break;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		def32 = ED_GlobalAtOfs32(progfuncs, ofs);
		break;
	default:
		externs->Sys_Error("Bad struct type in PR_GlobalString");
		return "";
	}

	val = (void *)&pr_globals[ofs];
	if (!def32)
	{
		etype_t type;
		//urgh, this is so hideous
#if !defined(MINIMAL) && !defined(OMIT_QCC)
		if (typehint == &type_float)
			type = ev_float;
		else if (typehint == &type_string)
			type = ev_string;
		else if (typehint == &type_vector)
			type = ev_vector;
		else if (typehint == &type_function)
			type = ev_function;
		else if (typehint == &type_field)
			type = ev_field;
		else
#endif
			type = ev_integer;
		s = PR_ValueString (progfuncs, type, val, false);
		sprintf (line,"%i(?)%s", ofs, s);
	}
	else
	{
		s = PR_ValueString (progfuncs, def32->type, val, false);
		sprintf (line,"%i(%s)%s", ofs, def32->s_name+progfuncs->funcs.stringtable, s);
	}

	i = strlen(line);
	for ( ; i<20 ; i++)
		strcat (line," ");
	strcat (line," ");
	return line;
}

char *PR_GlobalStringNoContents (progfuncs_t *progfuncs, int ofs)
{
	int		i;
	ddef16_t	*def16;
	ddef32_t	*def32;
	int nameofs = 0;
	static char	line[128];

	switch (current_progstate->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		def16 = ED_GlobalAtOfs16(progfuncs, ofs);
		if (def16)
			nameofs = def16->s_name;
		break;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		def32 = ED_GlobalAtOfs32(progfuncs, ofs);
		if (def32)
			nameofs = def32->s_name;
		break;
	default:
		externs->Sys_Error("Bad struct type in PR_GlobalStringNoContents");
	}

	if (nameofs)
		sprintf (line,"%i(%s)", ofs, nameofs+progfuncs->funcs.stringtable);
	else
	{
		if (ofs >= OFS_RETURN && ofs < OFS_PARM0)
			sprintf (line,"%i(return_%c)", ofs, 'x' + (ofs - OFS_RETURN)%3);
		else if (ofs >= OFS_PARM0 && ofs < RESERVED_OFS)
			sprintf (line,"%i(parm%i_%c)", ofs, (ofs - OFS_PARM0)/3, 'x' + (ofs - OFS_PARM0)%3);
		else
			sprintf (line,"%i(?""?""?)", ofs);
	}

	i = strlen(line);
	for ( ; i<20 ; i++)
		strcat (line," ");
	strcat (line," ");

	return line;
}

char *PR_GlobalStringImmediate (progfuncs_t *progfuncs, int ofs)
{
	int		i;
	static char	line[128];
	sprintf (line,"%i", ofs);

	i = strlen(line);
	for ( ; i<20 ; i++)
		strcat (line," ");
	strcat (line," ");

	return line;
}

/*
=============
ED_Print

For debugging
=============
*/
void PDECL ED_Print (pubprogfuncs_t *ppf, struct edict_s *ed)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	int		l;
	fdef_t	*d;
	int		*v;
	unsigned int		i;unsigned int j;
	const char	*name;
	int		type;

	if (((edictrun_t *)ed)->ereftype == ER_FREE)
	{
		externs->Printf ("FREE\n");
		return;
	}

	externs->Printf("\nEDICT %i:\n", NUM_FOR_EDICT(progfuncs, (struct edict_s *)ed));
	for (i=1 ; i<prinst.numfields ; i++)
	{
		d = &prinst.field[i];
		name = d->name;
		l = strlen(name);
		if (l >= 2 && name[l-2] == '_')
			continue;	// skip _x, _y, _z vars

		v = (int *)((char *)edvars(ed) + d->ofs*4);

	// if the value is still all 0, skip the field
#ifdef DEF_SAVEGLOBAL
		type = d->type & ~DEF_SAVEGLOBAL;
#else
		type = d->type;
#endif

		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		externs->Printf ("%s",name);
		l = strlen (name);
		while (l++ < 15)
			externs->Printf (" ");

		externs->Printf ("%s\n", PR_ValueString(progfuncs, d->type, (eval_t *)v, false));
	}
}
#if 0
void ED_PrintNum (progfuncs_t *progfuncs, int ent)
{
	ED_Print (&progfuncs->funcs, EDICT_NUM(progfuncs, ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts (progfuncs_t *progfuncs)
{
	unsigned int		i;

	externs->Printf ("%i entities\n", sv_num_edicts);
	for (i=0 ; i<sv_num_edicts ; i++)
		ED_PrintNum (progfuncs, i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count (progfuncs_t *progfuncs)
{
	unsigned int		i;
	edictrun_t	*ent;
	unsigned int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<sv_num_edicts ; i++)
	{
		ent = (edictrun_t *)EDICT_NUM(progfuncs, i);
		if (ent->isfree)
			continue;
		active++;
//		if (ent->v.solid)
//			solid++;
//		if (ent->v.model)
//			models++;
//		if (ent->v.movetype == MOVETYPE_STEP)
//			step++;
	}

	externs->Printf ("num_edicts:%3i\n", sv_num_edicts);
	externs->Printf ("active    :%3i\n", active);
//	Con_Printf ("view      :%3i\n", models);
//	Con_Printf ("touch     :%3i\n", solid);
//	Con_Printf ("step      :%3i\n", step);

}
#endif


//============================================================================


/*
=============
ED_NewString
=============
*/
char *PDECL ED_NewString (pubprogfuncs_t *ppf, const char *string, int minlength, pbool demarkup)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	char	*newc, *new_p;
	int		i,l;

	minlength++;

	l = strlen(string) + 1;

	newc = progfuncs->funcs.AddressableAlloc (&progfuncs->funcs, l<minlength?minlength:l);
	if (!newc)
		return progfuncs->funcs.stringtable;

	new_p = newc;

	for (i=0 ; i< l ; i++)
	{
		if (demarkup && string[i] == '\\' && i < l-1 && string[i+1] != 0)
		{
			i++;
			switch(string[i])
			{
			case 'n': *new_p++ = '\n'; break;
			case '\'': *new_p++ = '\''; break;
			case '\"': *new_p++ = '\"'; break;
			case 'r': *new_p++ = '\r'; break;
			default:
				*new_p++ = '\\';
				i--;
				break;
			}
		}
		else
			*new_p++ = string[i];
	}

	return newc;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
pbool	PDECL ED_ParseEval (pubprogfuncs_t *ppf, eval_t *eval, int type, const char *s)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	int		i;
	progsnum_t	module;
	char	string[128];
	fdef_t	*def;
	char	*v, *w;
	string_t st;
	mfunction_t	*func;

	switch (type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
#ifdef QCGC
		st = PR_AllocTempString(&progfuncs->funcs, s);
#else
		st = PR_StringToProgs(&progfuncs->funcs, ED_NewString (&progfuncs->funcs, s, 0, true));
#endif
		eval->string = st;
		break;

	case ev_float:
		eval->_float = (float)atof (s);
		break;
	case ev_double:
		eval->_double = atof (s);
		break;

	case ev_integer:
		eval->_int = strtol (s, NULL, 0);
		break;
	case ev_uint:
		eval->_uint = strtoul (s, NULL, 0);
		break;
	case ev_int64:
		eval->i64 = strtoll (s, NULL, 0);
		break;
	case ev_uint64:
		eval->u64 = strtoull (s, NULL, 0);
		break;

	case ev_vector:
		strncpy (string, s, sizeof(string));
		string[sizeof(string)-1] = 0;
		v = string;
		w = string;
		for (i=0 ; i<3 ; i++)
		{
			while (*v && *v != ' ')
				v++;
			if (!*v)
			{
				eval->_vector[i] = (float)atof (w);
				w = v;
			}
			else
			{
				*v = 0;
				eval->_vector[i] = (float)atof (w);
				w = v = v+1;
			}
		}
		break;

	case ev_entity:
		if (!strncmp(s, "entity ", 7))	//cope with etos weirdness.
			s += 7;
		eval->edict = atoi (s);
		break;

	case ev_field:
		def = ED_FindField (progfuncs, s);
		if (!def)
		{
			externs->Printf ("Can't find field %s\n", s);
			return false;
		}
		eval->_int = def->ofs;
		break;

	case ev_function:
		if (s[1]==':'&&s[2]=='\0')
		{
			eval->function = 0;
			return true;
		}
		func = ED_FindFunction (progfuncs, s, &module, -1);
		if (!func)
		{
			externs->Printf ("Can't find function %s\n", s);
			return false;
		}
		eval->function = (func - pr_progstate[module].functions) | (module<<24);
		break;

	default:
		return false;
	}
	return true;
}

pbool	ED_ParseEpair (progfuncs_t *progfuncs, size_t qcptr, unsigned int fldofs, int fldtype, char *s)
{
	pint64_t	i;
	puint64_t	u;
	progsnum_t module;
	fdef_t	*def;
	string_t st;
	mfunction_t	*func;
	int type = fldtype & ~DEF_SAVEGLOBAL;
	double d;
	eval_t *eval = (eval_t *)(progfuncs->funcs.stringtable + qcptr + (fldofs*sizeof(int)));

	switch (type)
	{
	case ev_string:
#ifdef QCGC
		st = PR_AllocTempString(&progfuncs->funcs, s);
#else
		st = PR_StringToProgs(&progfuncs->funcs, ED_NewString (&progfuncs->funcs, s, 0, true));
#endif
		eval->string = st;
		break;

	case ev_float:
		while(*s == ' ' || *s == '\t')
			s++;
		d = strtod(s, &s);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->_float = d;
		if (*s)
			return false;	//some kind of junk in there.
		break;
	case ev_double:
		while(*s == ' ' || *s == '\t')
			s++;
		d = strtod(s, &s);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->_double = d;
		if (*s)
			return false;	//some kind of junk in there.
		break;

	case ev_integer:
		while(*s == ' ' || *s == '\t')
			s++;
		i = strtol(s, &s, 0);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->_int = i;
		if (*s)
			return false;	//some kind of junk in there.
		break;
	case ev_entity:	//ent references are simple ints for us.
	case ev_uint:
		while(*s == ' ' || *s == '\t')
			s++;
		u = strtoul(s, &s, 0);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->_uint = u;
		if (*s)
			return false;	//some kind of junk in there.
		break;

	case ev_int64:
		while(*s == ' ' || *s == '\t')
			s++;
		i = strtoll(s, &s, 0);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->i64 = i;
		if (*s)
			return false;	//some kind of junk in there.
		break;
	case ev_uint64:
		while(*s == ' ' || *s == '\t')
			s++;
		u = strtoull(s, &s, 0);
		while(*s == ' ' || *s == '\t')
			s++;
		eval->u64 = u;
		if (*s)
			return false;	//some kind of junk in there.
		break;

	case ev_vector:
		for (i=0 ; i<3 ; i++)
		{
			while(*s == ' ' || *s == '\t')
				s++;
			d = strtod(s, &s);
			eval->_vector[i] = d;
		}
		while(*s == ' ' || *s == '\t')
			s++;
		if (*s)
			return false;	//some kind of junk in there.
		break;

	case ev_field:
		def = ED_FindField (progfuncs, s);
		if (!def)
		{
			externs->Printf ("Can't find field %s\n", s);
			return false;
		}
		eval->_int = def->ofs;
		break;

	case ev_function:
		if (s[0] && s[1]==':'&&s[2]=='\0')	//this isn't right...
		{
			eval->function = 0;
			return true;
		}
		func = ED_FindFunction (progfuncs, s, &module, -1);
		if (!func)
		{
			externs->Printf ("Can't find function %s\n", s);
			return false;
		}
		eval->function = (func - pr_progstate[module].functions) | (module<<24);
		break;

	default:
		return false;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
#if 1
static const char *ED_ParseEdict (progfuncs_t *progfuncs, const char *data, edictrun_t *ent, pbool *out_maphack)
{
	fdef_t		*key;
	pbool	init;
	char		keyname[256];
	int			n;
	int			nest = 1;

//	eval_t		*val;

	init = false;

// clear it
//	if (ent != (edictrun_t *)sv_edicts)	// hack
//		memset (ent+1, 0, pr_edict_size - sizeof(edictrun_t));

// go through all the dictionary pairs
	while (1)
	{
	// parse key
		data = QCC_COM_Parse (data);
		if (qcc_token[0] == '}')
		{
			if (--nest)
				continue;
			break;
		}
		if (qcc_token[0] == '{' && !qcc_token[1])
			nest++;
		if (!data)
		{
			externs->Printf ("ED_ParseEntity: EOF without closing brace\n");
			return NULL;
		}
		if (nest > 1)
			continue;

		strncpy (keyname, qcc_token, sizeof(keyname)-1);
		keyname[sizeof(keyname)-1] = 0;

		// another hack to fix heynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value
		data = QCC_COM_Parse (data);
		if (!data)
		{
			externs->Printf ("ED_ParseEntity: EOF without closing brace\n");
			return NULL;
		}

		if (qcc_token[0] == '}')
		{
			externs->Printf ("ED_ParseEntity: closing brace without data\n");
			return NULL;
		}

		init = true;

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
		{
			if (externs->badfield)
				externs->badfield(&progfuncs->funcs, (struct edict_s*)ent, keyname, qcc_token);
			continue;
		}

		if (!strcmp(keyname, "angle"))	//Quake anglehack - we've got to leave it in cos it doesn't work for quake otherwise, and this is a QuakeC lib!
		{
			if ((key = ED_FindField (progfuncs, "angles")))
			{
				QC_snprintfz (qcc_token, sizeof(qcc_token), "0 %f 0", atof(qcc_token));	//change it from yaw to 3d angle
				goto cont;
			}
		}

		key = ED_FindField (progfuncs, keyname);
		if (!key)
		{
			if (!strcmp(keyname, "light"))	//Quake lighthack - allows a field name and a classname to go by the same thing in the level editor
				if ((key = ED_FindField (progfuncs, "light_lev")))
					goto cont;
			if (externs->badfield && externs->badfield(&progfuncs->funcs, (struct edict_s*)ent, keyname, qcc_token))
				continue;
			PR_DPrintf ("'%s' is not a field\n", keyname);
			continue;
		}

cont:
		switch(key->type)
		{
		case ev_function:
		case ev_field:
		case ev_entity:
		case ev_pointer:
			*out_maphack = true;	//one of these types of fields means evil maphacks are at play.
			break;
		}
		if (!ED_ParseEpair (progfuncs, (char*)ent->fields - progfuncs->funcs.stringtable, key->ofs, key->type, qcc_token))
		{
			if (externs->badfield && externs->badfield(&progfuncs->funcs, (struct edict_s*)ent, keyname, qcc_token))
				continue;
			continue;
//			Sys_Error ("ED_ParseEdict: parse error on entities");
		}
	}

	if (!init)
		ent->ereftype = ER_FREE;

	return data;
}
#endif

static void PR_Cat(char *out, const char *in, size_t *len, size_t max)
{
	size_t newl = strlen(in);
	max-=1;
	if (*len + newl > max)
		newl = max - *len;	//truncate
	memcpy(out + *len, in, newl+1);
	*len += newl;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/

char *ED_WriteGlobals(progfuncs_t *progfuncs, char *buf, size_t *bufofs, size_t bufmax)	//switch first.
{
#define AddS(str) PR_Cat(buf, str, bufofs, bufmax)
	int		*v;
	ddef32_t		*def32;
	ddef16_t		*def16;
	unsigned int			i;
//	unsigned int j;
	const char	*name;
	int			type;
	int curprogs = prinst.pr_typecurrent;
	int len;
	switch(current_progstate->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			def16 = &pr_globaldefs16[i];
			name = def16->s_name + progfuncs->funcs.stringtable;
			len = strlen(name);
			if (!*name)
				continue;
			if (len >= 2 && name[len-2] == '_' && (name[len-1] == 'x' || name[len-1] == 'y' || name[len-1] == 'z'))
				continue;	// skip _x, _y, _z vars (vector components, which are saved as one vector not 3 floats)

			type = def16->type;

#ifdef DEF_SAVEGLOBAL
			if ( !(def16->type & DEF_SAVEGLOBAL) )
				continue;
			type &= ~DEF_SAVEGLOBAL;
#endif
			if (current_progstate->types)
				type = current_progstate->types[type].type;
			if (type == ev_function)
			{
				v = (int *)&current_progstate->globals[def16->ofs];
				if ((v[0]&0xff000000)>>24 == (unsigned)curprogs)	//same progs
				{
					if (!progfuncs->funcs.stringtable[current_progstate->functions[v[0]&0x00ffffff].s_name])
						continue;
					else if (!strcmp(current_progstate->functions[v[0]&0x00ffffff].s_name+ progfuncs->funcs.stringtable, name))	//names match. Assume function is at initial value.
						continue;
				}

				if (curprogs!=0)
				if ((v[0]&0xff000000)>>24 == 0)
					if (!ED_FindFunction(progfuncs, name, NULL, curprogs))	//defined as extern
					{
						if (!progfuncs->funcs.stringtable[pr_progstate[0].functions[v[0]&0x00ffffff].s_name])
							continue;
						else if (!strcmp(pr_progstate[0].functions[v[0]&0x00ffffff].s_name + progfuncs->funcs.stringtable, name))	//same name.
							continue;
					}

				//else function has been redirected externally.
				goto add16;
			}
			else if (type != ev_string	//anything other than these is not saved
			&& type != ev_float
			&& type != ev_double
			&& type != ev_integer
			&& type != ev_uint
			&& type != ev_int64
			&& type != ev_uint64
			&& type != ev_entity
			&& type != ev_vector)
				continue;

			v = (int *)&current_progstate->globals[def16->ofs];

/*			// make sure the value is not null, where there's no point in saving
			for (j=0 ; j<type_size[type] ; j++)
				if (v[j])
					break;
			if (j == type_size[type])
				continue;
*/
 add16:
			AddS("\""); AddS(name); AddS("\" \""); AddS(PR_UglyValueString(&progfuncs->funcs, def16->type&~DEF_SAVEGLOBAL, (eval_t *)v)); AddS("\"\n");
		}
		break;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			size_t nlen;
			def32 = &pr_globaldefs32[i];
			name = PR_StringToNative(&progfuncs->funcs, def32->s_name);
			nlen = strlen(name);
			if (nlen >= 3 && name[nlen-2] == '_')
				continue;	// skip _x, _y, _z vars (vector components, which are saved as one vector not 3 floats)

			type = def32->type;

#ifdef DEF_SAVEGLOBAL
			if ( !(def32->type & DEF_SAVEGLOBAL) )
				continue;
			type &= ~DEF_SAVEGLOBAL;
#endif
			if (current_progstate->types)
				type = current_progstate->types[type].type;
			if (type == ev_function)
			{
				v = (int *)&current_progstate->globals[def32->ofs];
				if ((v[0]&0xff000000)>>24 == (unsigned)curprogs)	//same progs
					if (!strcmp(current_progstate->functions[v[0]&0x00ffffff].s_name+ progfuncs->funcs.stringtable, name))	//names match. Assume function is at initial value.
						continue;

				if (curprogs!=0)
				if ((v[0]&0xff000000)>>24 == 0)
					if (!ED_FindFunction(progfuncs, name, NULL, curprogs))	//defined as extern
						if (!strcmp(pr_progstate[0].functions[v[0]&0x00ffffff].s_name+ progfuncs->funcs.stringtable, name))	//same name.
							continue;

				//else function has been redirected externally.
				goto add32;
			}
			else if (type != ev_string	//anything other than these is not saved
			&& type != ev_float
			&& type != ev_double
			&& type != ev_integer
			&& type != ev_uint
			&& type != ev_int64
			&& type != ev_uint64
			&& type != ev_entity
			&& type != ev_vector)
				continue;

			v = (int *)&current_progstate->globals[def32->ofs];

/*			// make sure the value is not null, where there's no point in saving
			for (j=0 ; j<type_size[type] ; j++)
				if (v[j])
					break;
			if (j == type_size[type])
				continue;*/
add32:
			AddS("\""); AddS(name); AddS("\" \""); AddS(PR_UglyValueString(&progfuncs->funcs, def32->type&~DEF_SAVEGLOBAL, (eval_t *)v)); AddS("\"\n");
		}
		break;
	default:
		externs->Sys_Error("Bad struct type in SaveEnts");
	}

	return buf;
#undef AddS
}

char *ED_WriteEdict(progfuncs_t *progfuncs, edictrun_t *ed, char *buf, size_t *bufofs, size_t bufmax, pbool q1compatible)
{
#define AddS(str) PR_Cat(buf, str, bufofs, bufmax)
	fdef_t	*d;
	int		*v;
	unsigned int		i;unsigned int j;
	const char	*name;
	int		type;
	int len;
	char *tmp;

	for (i=0 ; i<prinst.numfields ; i++)
	{
		d = &prinst.field[i];
		name = d->name;
		len = strlen(name);
		if (len>4 && (name[len-2] == '_' && (name[len-1] == 'x' || name[len-1] == 'y' || name[len-1] == 'z')))
			continue;	// skip _x, _y, _z vars

		v = (int *)((char*)ed->fields + d->ofs*4);

	// if the value is still all 0, skip the field
#ifdef DEF_SAVEGLOBAL
		type = d->type & ~DEF_SAVEGLOBAL;
#else
		type = d->type;
#endif

		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		//add it to the file
		AddS("\""); AddS(name); AddS("\" ");
		if (q1compatible)
			tmp = PR_UglyOldValueString(progfuncs, d->type, (eval_t *)v);
		else
			tmp = PR_UglyValueString(&progfuncs->funcs, d->type, (eval_t *)v);
		AddS("\""); AddS(tmp); AddS("\"\n");
	}

	return buf;
#undef AddS
}

//just a simple helper that makes sure the s_name+s_file values are actually valid. some qccs generate really dodgy values intended to crash decompilers, but also crash debuggers too.
static char *PR_StaticString(progfuncs_t *progfuncs, string_t thestring)
{
	if (thestring <= 0 || thestring >= progfuncs->funcs.stringtablesize)
		return "???";
	return thestring + progfuncs->funcs.stringtable;
}

char *PR_SaveCallStack (progfuncs_t *progfuncs, char *buf, size_t *bufofs, size_t bufmax)
{
#define AddS(str) PR_Cat(buf, str, bufofs, bufmax)
	char buffer[8192];
	const mfunction_t	*f;
	int			i;
	int progs;

	int arg;
	int *globalbase;

	progs = -1;

	if (prinst.pr_depth == 0)
	{
		AddS ("<NO STACK>\n");
		return buf;
	}

	globalbase = (int *)pr_globals + prinst.pr_xfunction->parm_start + prinst.pr_xfunction->locals;

	prinst.pr_stack[prinst.pr_depth].f = prinst.pr_xfunction;
	for (i=prinst.pr_depth ; i>0 ; i--)
	{
		f = prinst.pr_stack[i].f;

		if (!f)
		{
			AddS ("<NO FUNCTION>\n");
		}
		else
		{
			if (prinst.pr_stack[i].progsnum != progs)
			{
				progs = prinst.pr_stack[i].progsnum;

				sprintf(buffer, "//%i %s\n", progs, pr_progstate[progs].filename);
				AddS (buffer);
			}
			if (!f->s_file)
				sprintf(buffer, "\t\"%i:%s\"\n", progs, PR_StaticString(progfuncs, f->s_name));
			else
				sprintf(buffer, "\t\"%i:%s\" //%s\n", progs, PR_StaticString(progfuncs, f->s_name), PR_StaticString(progfuncs, f->s_file));
			AddS (buffer);

			AddS ("\t{\n");
			for (arg = 0; arg < f->locals; arg++)
			{
				ddef16_t *local;
				local = ED_GlobalAtOfs16(progfuncs, f->parm_start+arg);
				if (!local)
					sprintf(buffer, "\t\tofs%i %i // %f\n", f->parm_start+arg, *(int *)(globalbase - f->locals+arg), *(float *)(globalbase - f->locals+arg) );
				else
				{
					if (local->type == ev_entity)
					{
						sprintf(buffer, "\t\t\"%s\" \"entity %i\"\n", PR_StaticString(progfuncs, local->s_name), ((eval_t*)(globalbase - f->locals+arg))->edict);
					}
					else
						sprintf(buffer, "\t\t\"%s\"\t\"%s\"\n", PR_StaticString(progfuncs, local->s_name), PR_ValueString(progfuncs, local->type, (eval_t*)(globalbase - f->locals+arg), false));

					if (local->type == ev_vector)
						arg+=2;
				}
				AddS (buffer);
			}
			AddS ("\t}\n");

			if (i == prinst.pr_depth)
				globalbase = prinst.localstack + prinst.localstack_used - f->locals;
			else
				globalbase -= f->locals;
		}
	}
	return buf;
#undef AddS
}

//there are two ways of saving everything.
//0 is to save just the entities.
//1 is to save the entites, and all the progs info so that all the variables are saved off, and it can be reloaded to exactly how it was (provided no files or data has been changed outside, like the progs.dat for example)
//2 is for vanilla-compatible saved games
//3 is a (human-readable) coredump
//4 is binary saved games.
char *PDECL PR_SaveEnts(pubprogfuncs_t *ppf, char *buf, size_t *bufofs, size_t bufmax, int alldata)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
#define AddS(str) PR_Cat(buf, str, bufofs, bufmax)
	unsigned int a;
	char *buffree = NULL;
	int oldprogs;

	if (!buf)
	{
		if (bufmax <= 0)
			bufmax = 5*1024*1024;
		buffree = buf = externs->memalloc(bufmax);
	}
	*bufofs = 0;

	switch(alldata)
	{
	default:
		return NULL;
	case 2:
		//special Q1 savegame compatability mode.
		//engine will need to store references to progs type and will need to preload the progs and inti the ents itself before loading.

		//Make sure there is only 1 progs loaded.
		for (a = 1; a < prinst.maxprogs; a++)
		{
			if (pr_progstate[a].progs)
				break;
		}
		if (!pr_progstate[0].progs || a != prinst.maxprogs)	//the state of the progs wasn't Q1 compatible.
		{
			externs->memfree(buffree);
			return NULL;
		}

		//write the globals
		AddS ("{\n");

		oldprogs = prinst.pr_typecurrent;
		PR_SwitchProgs(progfuncs, 0);
		ED_WriteGlobals(progfuncs, buf, bufofs, bufmax);
		PR_SwitchProgs(progfuncs, oldprogs);

		AddS ("}\n");


		//write the ents
		for (a = 0; a < sv_num_edicts; a++)
		{
			char head[64];
			edictrun_t *ed = (edictrun_t *)EDICT_NUM(progfuncs, a);

			QC_snprintfz(head, sizeof(head), "{//%i\n", a);
			AddS (head);

			if (ed->ereftype == ER_ENTITY)	//free entities write a {} with no data. the loader detects this specifically.
				ED_WriteEdict(progfuncs, ed, buf, bufofs, bufmax, true);

			AddS ("}\n");
		}

		return buf;

	case 0:	//Writes entities only
		break;
	case 1:
	case 3:
		AddS("general {\n");
		AddS(qcva("\"maxprogs\" \"%i\"\n", prinst.maxprogs));
//		AddS(qcva("\"maxentities\" \"%i\"\n", maxedicts));
//		AddS(qcva("\"mem\" \"%i\"\n", hunksize));
//		AddS(qcva("\"crc\" \"%i\"\n", header_crc));
		AddS(qcva("\"numentities\" \"%i\"\n", sv_num_edicts));
		AddS("}\n");

		oldprogs = prinst.pr_typecurrent;

		for (a = 0; a < prinst.maxprogs; a++)
		{
			if (!pr_progstate[a].progs)
				continue;
			{
				AddS (qcva("progs %i {\n", a));
				AddS (qcva("\"filename\" \"%s\"\n", pr_progstate[a].filename));
				AddS (qcva("\"crc\" \"%i\"\n", pr_progstate[a].progs->crc));
				AddS ("}\n");
			}
		}

		if (alldata == 3)
		{
			//include callstack
			AddS("stacktrace {\n");
			PR_SaveCallStack(progfuncs, buf, bufofs, bufmax);
			AddS("}\n");
		}

		for (a = 0; a < prinst.maxprogs; a++)	//I would mix, but external functions rely on other progs being loaded
		{
			if (!pr_progstate[a].progs)
				continue;

			AddS (qcva("globals %i {\n", a));

			PR_SwitchProgs(progfuncs, a);

			ED_WriteGlobals(progfuncs, buf, bufofs, bufmax);

			AddS ("}\n");
		}
		PR_SwitchProgs(progfuncs, oldprogs);
	}

	for (a = 0; a < sv_num_edicts; a++)
	{
		edictrun_t *ed = (edictrun_t *)EDICT_NUM(progfuncs, a);

		if (!ed || ed->ereftype != ER_ENTITY)
			continue;

		AddS (qcva("entity %i{\n", a));

		ED_WriteEdict(progfuncs, ed, buf, bufofs, bufmax, false);

		AddS ("}\n");
	}

	return buf;

#undef AddS
}

//if 'general' block is found, this is a compleate state, otherwise, we should spawn entities like
int PDECL PR_LoadEnts(pubprogfuncs_t *ppf, const char *file, void *ctx, void (PDECL *memoryreset) (pubprogfuncs_t *progfuncs, void *ctx), void (PDECL *entspawned) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend), pbool(PDECL *extendedterm)(pubprogfuncs_t *progfuncs, void *ctx, const char **extline))
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	const char *datastart;

//	eval_t *selfvar = NULL;
//	eval_t *var;
//	const char *spawnwarned[20] = {NULL};

	char filename[128];
	int num;
	edictrun_t *ed=NULL;
	ddef16_t *d16;
	ddef32_t *d32;
	void *oldglobals = NULL;
	int oldglobalssize = 0;

	extern edictrun_t tempedict;

	int entsize = 0;
	int numents = 0;
	pbool maphacks = false;

	pbool resethunk=0;
	pbool isloadgame;
	if (file && !strncmp(file, "loadgame", 8))
	{	//this is internally inserted for legacy saved games.
		isloadgame = true;
		numents = -1;
		file+=8;
	}
	else
		isloadgame = false;

	while(1)
	{
		datastart = file;

		if (extendedterm)
		{
			//skip simple leading whitespace
			while (qcc_iswhite(*file))
				file++;
			if (file[0] == '/' && file[1] == '*')
			{	//looks like we have a hidden extension.
				file+=2;
				for(;;)
				{
					//skip to end of line
					if (!*file)
						break;	//unexpected EOF
					else if (file[0] == '*' && file[1] == '/')
					{	//end of comment
						file+=2;
						break;
					}
					else if (*file != '\n')
					{
						file++;
						continue;
					}
					file++;	//skip past the \n
					while (*file == ' ' || *file == '\t')
						file++;	//skip leading indentation

					if (file[0] == '*' && file[1] == '/')
					{	//end of comment
						file+=2;
						break;
					}
					else if (*file == '/')
						continue;	//embedded comment. ignore the line. not going to do nested comments, because those are not normally valid anyway, just C++-style inside C-style.
					else if (extendedterm(ppf, ctx, &file))
						;	//found a term we recognised
					else
						;	//unknown line, but this is a comment so whatever

				}
				continue;
			}
		}

		file = QCC_COM_Parse(file);
		if (file == NULL)
			break;	//finished reading file
		else if (!strcmp(qcc_token, "Version"))
		{
			file = QCC_COM_Parse(file);
			//qcc_token is a version number
		}
		else if (!strcmp(qcc_token, "entity"))
		{
			if (entsize == 0 && resethunk)	//edicts have not yet been initialized, and this is a compleate load (memsize has been set)
			{
				entsize = PR_InitEnts(&progfuncs->funcs, prinst.maxedicts);
//				sv_num_edicts = numents;

				for (num = 0; num < numents; num++)
				{
					ed = (edictrun_t *)EDICT_NUM(progfuncs, num);

					if (!ed)
					{
						ed = (edictrun_t *)ED_AllocIndex(&progfuncs->funcs, num, false, 0);
						ed->ereftype = ER_FREE;
						if (externs->entspawn)
							externs->entspawn((struct edict_s *) ed, true);
					}
				}
			}

			file = QCC_COM_Parse(file);
			num = atoi(qcc_token);
			datastart = file;
			file = QCC_COM_Parse(file);
			if (qcc_token[0] != '{')
				externs->Sys_Error("Progs loading found %s, not '{'", qcc_token);
			if (!resethunk)
				ed = (edictrun_t *)ED_Alloc(&progfuncs->funcs, false, 0);
			else
			{
				ed = (edictrun_t *)EDICT_NUM(progfuncs, num);

				if (!ed)
				{
					externs->Sys_Error("Edict was not allocated\n");
					ed = (edictrun_t *)ED_AllocIndex(&progfuncs->funcs, num, false, 0);
				}
			}
			ed->ereftype = ER_ENTITY;
			if (externs->entspawn)
				externs->entspawn((struct edict_s *) ed, true);
			file = ED_ParseEdict(progfuncs, file, ed, &maphacks);

			if (entspawned)
				entspawned(ppf, (struct edict_s *)ed, ctx, datastart, file);
		}
		else if (!strcmp(qcc_token, "progs"))
		{
			file = QCC_COM_Parse(file);
			num = atoi(qcc_token);
			file = QCC_COM_Parse(file);
			if (qcc_token[0] != '{')
				externs->Sys_Error("Progs loading found %s, not '{'", qcc_token);


			filename[0] = '\0';

			while(1)
			{
				file = QCC_COM_Parse(file);	//read the key
				if (!file)
					externs->Sys_Error("EOF in progs block");

				if (!strcmp("filename", qcc_token))	//check key get and save values
					{file = QCC_COM_Parse(file); strcpy(filename, qcc_token);}
				else if (!strcmp("crc", qcc_token))
					{file = QCC_COM_Parse(file); /*header_crc = atoi(qcc_token);*/}
				else if (!strcmp("numbuiltins", qcc_token))	//no longer supported.
					{file = QCC_COM_Parse(file); /*qcc_token unused*/}
				else if (qcc_token[0] == '}')	//end of block
					break;
				else
					externs->Sys_Error("Bad key \"%s\" in progs block", qcc_token);
			}

			PR_ReallyLoadProgs(progfuncs, filename, &pr_progstate[num], true);

			if (num == 0 && oldglobals)
			{
				if (pr_progstate[0].globals_bytes == oldglobalssize)
					memcpy(pr_progstate[0].globals, oldglobals, pr_progstate[0].globals_bytes);
				free(oldglobals);
				oldglobals = NULL;
			}

			PR_SwitchProgs(progfuncs, 0);
		}
		else if (!strcmp(qcc_token, "globals"))
		{
			if (entsize == 0 && resethunk)	//by the time we parse some globals, we MUST have loaded all progs
			{
				entsize = PR_InitEnts(&progfuncs->funcs, prinst.maxedicts);
				if (memoryreset)
					memoryreset(&progfuncs->funcs, ctx);
//				sv_num_edicts = numents;

				for (num = 0; num < numents; num++)
				{
					ed = (edictrun_t *)EDICT_NUM(progfuncs, num);

					if (!ed)
					{
						ed = (edictrun_t *)ED_AllocIndex(&progfuncs->funcs, num, false, 0);
						ed->ereftype = ER_FREE;
					}

					if (externs->entspawn)
						externs->entspawn((struct edict_s *) ed, true);
				}
			}

			file = QCC_COM_Parse(file);
			num = atoi(qcc_token);

			file = QCC_COM_Parse(file);
			if (qcc_token[0] != '{')
				externs->Sys_Error("Globals loading found \'%s\', not '{'", qcc_token);

			PR_SwitchProgs(progfuncs, num);
			while (1)
			{
				file = QCC_COM_Parse(file);
				if (qcc_token[0] == '}')
					break;
				else if (!qcc_token[0] || !file)
					externs->Sys_Error("EOF when parsing global values");

				switch(current_progstate->structtype)
				{
				case PST_DEFAULT:
				case PST_KKQWSV:
					if (!(d16 = ED_FindGlobal16(progfuncs, qcc_token)))
					{
						externs->Printf("global value %s not found\n", qcc_token);
						file = QCC_COM_Parse(file);
					}
					else
					{
						file = QCC_COM_Parse(file);
						ED_ParseEpair(progfuncs, (char*)pr_globals - progfuncs->funcs.stringtable, d16->ofs, d16->type, qcc_token);
					}
					break;
				case PST_QTEST:
				case PST_FTE32:
				case PST_UHEXEN2:
					if (!(d32 = ED_FindGlobal32(progfuncs, qcc_token)))
					{
						externs->Printf("global value %s not found\n", qcc_token);
						file = QCC_COM_Parse(file);
					}
					else
					{
						file = QCC_COM_Parse(file);
						ED_ParseEpair(progfuncs, (char*)pr_globals - progfuncs->funcs.stringtable, d32->ofs, d32->type, qcc_token);
					}
					break;
				default:
					externs->Sys_Error("Bad struct type in LoadEnts");
				}
			}
			PR_SwitchProgs(progfuncs, 0);

//			file = QCC_COM_Parse(file);
//			if (com_token[0] != '}')
//				Sys_Error("Progs loading found %s, not '}'", qcc_token);
		}
		else if (!strcmp(qcc_token, "general"))
		{
			QC_StartShares(progfuncs);
//			QC_InitShares();	//forget stuff
//			pr_edict_size = 0;
			prinst.max_fields_size=0;

			file = QCC_COM_Parse(file);
			if (qcc_token[0] != '{')
				externs->Sys_Error("Progs loading found %s, not '{'", qcc_token);

			while(1)
			{
				file = QCC_COM_Parse(file);	//read the key
				if (!file)
					externs->Sys_Error("EOF in general block");

				if (!strcmp("maxprogs", qcc_token))	//check key get and save values
					{file = QCC_COM_Parse(file); prinst.maxprogs = atoi(qcc_token);}
//				else if (!strcmp("maxentities", com_token))
//					{file = QCC_COM_Parse(file); maxedicts = atoi(qcc_token);}
//				else if (!strcmp("mem", com_token))
//					{file = QCC_COM_Parse(file); memsize = atoi(qcc_token);}
//				else if (!strcmp("crc", com_token))
//					{file = QCC_COM_Parse(file); crc = atoi(qcc_token);}
				else if (!strcmp("numentities", qcc_token))
					{file = QCC_COM_Parse(file); numents = atoi(qcc_token);}
				else if (qcc_token[0] == '}')	//end of block
					break;
				else
					externs->Sys_Error("Bad key \"%s\" in general block", qcc_token);
			}

			if (oldglobals)
				free(oldglobals);
			oldglobals = NULL;
			if (pr_progstate[0].globals_bytes)
			{
				oldglobals = malloc(pr_progstate[0].globals_bytes);
				if (oldglobals)
				{
					oldglobalssize = pr_progstate[0].globals_bytes;
					memcpy(oldglobals, pr_progstate[0].globals, oldglobalssize);
				}
				else
					externs->Printf("Unable to alloc %i bytes\n", pr_progstate[0].globals_bytes);
			}

			PRAddressableFlush(progfuncs, 0);
			resethunk=true;

			pr_progstate = PRHunkAlloc(progfuncs, sizeof(progstate_t) * prinst.maxprogs, "progstatetable");
			prinst.pr_typecurrent=0;

			sv_num_edicts = 1;	//set up a safty buffer so things won't go horribly wrong too often
			sv_edicts=(struct edict_s *)&tempedict;
			prinst.edicttable = (struct edictrun_s**)(progfuncs->funcs.edicttable = &sv_edicts);
			progfuncs->funcs.edicttable_length = numents;

			sv_num_edicts = numents;	//should be fine

//			PR_Configure(crc, NULL, memsize, maxedicts, maxprogs);
		}
		else if (!strcmp(qcc_token, "{"))
		{
			if (isloadgame)
			{
				if (numents == -1)	//globals
				{
					while (1)
					{
						file = QCC_COM_Parse(file);
						if (qcc_token[0] == '}')
							break;
						else if (!qcc_token[0] || !file)
							externs->Sys_Error("EOF when parsing global values");

						switch(current_progstate->structtype)
						{
						case PST_DEFAULT:
						case PST_KKQWSV:
							if (!(d16 = ED_FindGlobal16(progfuncs, qcc_token)))
							{
								externs->Printf("global value %s not found\n", qcc_token);
								file = QCC_COM_Parse(file);
							}
							else
							{
								file = QCC_COM_Parse(file);
								ED_ParseEpair(progfuncs, (char*)pr_globals - progfuncs->funcs.stringtable, d16->ofs, d16->type, qcc_token);
							}
							break;
						case PST_QTEST:
						case PST_FTE32:
						case PST_UHEXEN2:
							if (!(d32 = ED_FindGlobal32(progfuncs, qcc_token)))
							{
								externs->Printf("global value %s not found\n", qcc_token);
								file = QCC_COM_Parse(file);
							}
							else
							{
								file = QCC_COM_Parse(file);
								ED_ParseEpair(progfuncs, (char*)pr_globals - progfuncs->funcs.stringtable, d32->ofs, d32->type, qcc_token);
							}
							break;
						default:
							externs->Sys_Error("Bad struct type in LoadEnts");
						}
					}
				}
				else
				{
					ed = (edictrun_t *)ED_AllocIndex(&progfuncs->funcs, numents, false, 0);

					if (externs->entspawn)
						externs->entspawn((struct edict_s *) ed, true);

					ed->ereftype = ER_ENTITY;
					file = ED_ParseEdict (progfuncs, file, ed, &maphacks);
				}
				sv_num_edicts = ++numents;
				continue;
			}

			if (entsize == 0 && resethunk)	//edicts have not yet been initialized, and this is a compleate load (memsize has been set)
			{
				entsize = PR_InitEnts(&progfuncs->funcs, prinst.maxedicts);
//				sv_num_edicts = numents;

				for (num = 0; num < numents; num++)
				{
					ed = (edictrun_t *)EDICT_NUM(progfuncs, num);

					if (!ed)
					{
						ed = (edictrun_t *)ED_AllocIndex(&progfuncs->funcs, num, false, 0);
						ed->ereftype = ER_FREE;
					}
				}
			}

			if (!ed)	//first entity
				ed = (edictrun_t *)EDICT_NUM(progfuncs, 0);
			else
				ed = (edictrun_t *)ED_Alloc(&progfuncs->funcs, false, 0);
			ed->ereftype = ER_ENTITY;
			if (externs->entspawn)
				externs->entspawn((struct edict_s *) ed, true);
			file = ED_ParseEdict(progfuncs, file, ed, &maphacks);

			if (entspawned)
				entspawned(ppf, (struct edict_s *)ed, ctx, datastart, file);
		}
		else if (extendedterm && extendedterm(ppf, ctx, &datastart))
			file = datastart;
		else
			externs->Sys_Error("Bad entity lump: '%s' not recognised (last ent was %i)", qcc_token, ed?ed->entnum:0);
	}
	if (resethunk)
	{
		if (externs->loadcompleate)
			externs->loadcompleate(entsize);

		sv_num_edicts = numents;
	}

	if (oldglobals)
		free(oldglobals);
	oldglobals = NULL;

	if (resethunk)
	{
		return entsize;
	}
	else
		return prinst.max_fields_size;
}

//FIXME: maxsize is ignored.
char *PDECL PR_SaveEnt (pubprogfuncs_t *ppf, char *buf, size_t *size, size_t maxsize, struct edict_s *ed)
{
#define AddS(str) PR_Cat(buf, str, size, maxsize)
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	fdef_t	*d;
	int		*v;
	unsigned int		i;unsigned int j;
	const char	*name, *mname;
	const char	*classname = NULL;
	int		classnamelen = 0;
	int		type;

//	if (ed->free)
//		continue;

	AddS ("{\n");


	for (i=0 ; i<prinst.numfields ; i++)
	{
		int len;

		d = &prinst.field[i];
		name = d->name;
		len = strlen(name); // should we skip vars with no name?
		if (len > 2 && name[len-2] == '_' && (name[len-1] == 'x' || name[len-1] == 'y' || name[len-1] == 'z'))
			continue;	// skip _x, _y, _z vars

		v = (int*)((edictrun_t*)ed)->fields + d->ofs;

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		if (strstr(name, "::"))
		{
			if (*name != ':')
				continue;	//don't directly generate anything from class::foo

			if (!classname)
			{
				fdef_t *cnfd;
				cnfd = ED_FindField(progfuncs, "classname");
				if (cnfd)
				{
					string_t *v;
					v = (string_t *)((char *)edvars(ed) + cnfd->ofs*4);
					classname = PR_StringToNative(&progfuncs->funcs, *v);
				}
				else
					classname = "";
				classnamelen = strlen(classname);
			}
			for (j = i+1; j < prinst.numfields; j++)
			{
				if (prinst.field[j].ofs == d->ofs)
				{
					mname = prinst.field[j].name;
					if (!strncmp(mname, classname, classnamelen) && mname[classnamelen] == ':')
					{
						//okay, we have a match...
						name = prinst.field[j].name;
						break;
					}
				}
			}
		}

		//add it to the file
		AddS("\""); AddS(name); AddS("\" \""); AddS(PR_UglyValueString(&progfuncs->funcs, d->type, (eval_t *)v)); AddS("\"\n");
	}

	AddS ("}\n");

	return buf;
}
struct edict_s *PDECL PR_RestoreEnt (pubprogfuncs_t *ppf, const char *buf, size_t *size, struct edict_s *ed)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *ent;
	const char *start = buf;
	pbool maphacks = false;	//don't really care.

	buf = QCC_COM_Parse(buf);	//read the key
	if (!buf || !*qcc_token)
		return NULL;

	if (strcmp(qcc_token, "{"))
	{
		externs->Printf("PR_RestoreEnt: with no opening brace");
		return NULL;
	}

	if (!ed)
		ent = (edictrun_t *)ED_Alloc(&progfuncs->funcs, false, 0);
	else
		ent = (edictrun_t *)ed;

	if (ent->ereftype == ER_FREE && externs->entspawn)
	{
		memset (ent->fields, 0, ent->fieldsize);
		ent->ereftype = ER_ENTITY;
		externs->entspawn((struct edict_s *) ent, false);
	}
	if (ent->ereftype != ER_ENTITY)
		return NULL;	//not allowed to spawn it into that ent.

	buf = ED_ParseEdict(progfuncs, buf, ent, &maphacks);

	*size = buf - start;

	return (struct edict_s *)ent;
}

#define Host_Error Sys_Error

//return true if pr_progs needs recompiling (source files have changed)
pbool PR_TestRecompile(progfuncs_t *progfuncs)
{
	int newsize;
	int num, found=0, lost=0, changed=0;
	includeddatafile_t *s;
	if (!pr_progs->ofsfiles)
		return false;

	num = *(int*)((char *)pr_progs + pr_progs->ofsfiles);
	s = (includeddatafile_t *)((char *)pr_progs + pr_progs->ofsfiles+4);
	while(num>0)
	{
		newsize = externs->FileSize(s->filename);
		if (newsize == -1)	//ignore now missing files. - the referencer must have changed...
			lost++;
		else if (s->size != newsize)	//file
			changed++;
		else
			found++;

		s++;
		num--;
	}
	if (lost > found+changed)
		return false;
	if (changed)
		return true;
	return false;
}
/*
#ifdef _DEBUG
//this is for debugging.
//I'm using this to detect incorrect string types while converting 32bit string pointers with bias to bound indexes.
void PR_TestForWierdness(progfuncs_t *progfuncs)
{
	unsigned int i;
	int e;
	edictrun_t *ed;
	for (i = 0; i < pr_progs->numglobaldefs; i++)
	{
		if ((pr_globaldefs16[i].type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_string)
		{
			if (G_INT(pr_globaldefs16[i].ofs) < 0 || G_INT(pr_globaldefs16[i].ofs) >= addressableused)
				externs->Printf("String type irregularity on \"%s\" \"%s\"\n", pr_globaldefs16[i].s_name+progfuncs->funcs.stringtable, G_INT(pr_globaldefs16[i].ofs)+progfuncs->funcs.stringtable);
		}
	}

	for (i = 0; i < numfields; i++)
	{
		if ((field[i].type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_string)
		{
			for (e = 0; e < sv_num_edicts; e++)
			{
				ed = (edictrun_t*)EDICT_NUM(progfuncs, e);
				if (ed->isfree)
					continue;
				if (((int *)ed->fields)[field[i].ofs] < 0 || ((int *)ed->fields)[field[i].ofs] >= addressableused)
					externs->Printf("String type irregularity \"%s\" \"%s\"\n", field[i].name, ((int *)ed->fields)[field[i].ofs]+progfuncs->funcs.stringtable);
			}
		}
	}
}
#endif
*/

void PR_CleanUpStatements16(progfuncs_t *progfuncs, dstatement16_t *st, pbool hexencalling)
{
	unsigned int numst = pr_progs->numstatements;
	unsigned int numglob = pr_progs->numglobals+3;	//+3 because I'm too lazy to deal with vectors
	unsigned int i;
	for (i=0 ; i<numst ; i++)
	{
		if (st[i].op >= OP_CALL1 && st[i].op <= OP_CALL8 && hexencalling)
			st[i].op += OP_CALL1H - OP_CALL1;
		if (st[i].op >= OP_RAND0 && st[i].op <= OP_RANDV2 && hexencalling)
			if (!st[i].c)
				st[i].c = OFS_RETURN;

		//sanitise inputs
		if (st[i].a >= numglob)
			if (st[i].op != OP_GOTO)
				st[i].op = ~0;
		if (st[i].b >= numglob)
			if (st[i].op != OP_IFNOT_I && st[i].op != OP_IF_I &&
				st[i].op != OP_IFNOT_F && st[i].op != OP_IF_F &&
				st[i].op != OP_IFNOT_S && st[i].op != OP_IF_S &&
				st[i].op != OP_BOUNDCHECK && st[i].op != OP_CASE)
				st[i].op = ~0;
		if (st[i].c >= numglob)
			if (st[i].op != OP_BOUNDCHECK && st[i].op != OP_CASERANGE)
				st[i].op = ~0;
	}
}
void PR_CleanUpStatements32(progfuncs_t *progfuncs, dstatement32_t *st, pbool hexencalling)
{
	unsigned int numst = pr_progs->numstatements;
	unsigned int numglob = pr_progs->numglobals+3;	//+3 because I'm too lazy to deal with vectors
	unsigned int i;
	for (i=0 ; i<numst ; i++)
	{
		if (st[i].op >= OP_CALL1 && st[i].op <= OP_CALL8 && hexencalling)
			st[i].op += OP_CALL1H - OP_CALL1;
		if (st[i].op >= OP_RAND0 && st[i].op <= OP_RANDV2 && hexencalling)
			if (!st[i].c)
				st[i].c = OFS_RETURN;

		//sanitise inputs
		if (st[i].a >= numglob)
			if (st[i].op != OP_GOTO)
				st[i].op = ~0;
		if (st[i].b >= numglob)
			if (st[i].op != OP_IFNOT_I && st[i].op != OP_IF_I &&
				st[i].op != OP_IFNOT_F && st[i].op != OP_IF_F &&
				st[i].op != OP_IFNOT_S && st[i].op != OP_IF_S &&
				st[i].op != OP_BOUNDCHECK && st[i].op != OP_CASE)
				st[i].op = ~0;
		if (st[i].c >= numglob)
			if (st[i].op != OP_BOUNDCHECK && st[i].op != OP_CASERANGE)
				st[i].op = ~0;
	}
}

char *decode(int complen, int len, int method, char *info, char *buffer);
unsigned char *PDECL PR_GetHeapBuffer (void *ctx, size_t bufsize)
{
	return PRHunkAlloc(ctx, bufsize+1, "proginfo");
}
/*
===============
PR_LoadProgs
===============
*/
pbool PR_ReallyLoadProgs (progfuncs_t *progfuncs, const char *filename, progstate_t *progstate, pbool complain)
{
	unsigned int		i, j, type;
//	float	fl;
//	int len;
//	int num;
//	dfunction_t *f, *f2;
	ddef16_t *d16;
	ddef32_t *d32;
	int *d2;
	eval_t *eval;
	char *s;
	int progstype;
	int trysleft = 2;
//	bool qfhack = false;
	pbool isfriked = false;	//all imediate values were stripped, which causes problems with strings.
	pbool hexencalling = false;	//hexen style calling convention. The opcodes themselves are used as part of passing the arguments;
	ddef16_t *gd16, *fld16;
	float *glob;
	dfunction_t *fnc;
	mfunction_t *fnc2;
	dstatement16_t *st16;
	size_t fsz;
	int len;

	int hmark=0xffffffff;

	int reorg = prinst.reorganisefields || prinst.numfields;

	int stringadjust;
	int pointeradjust;

	int *basictypetable;

	current_progstate = progstate;

	strcpy(current_progstate->filename, filename);


// flush the non-C variable lookup cache
//	for (i=0 ; i<GEFV_CACHESIZE ; i++)
//		gefvCache[i].field[0] = 0;

	memset(&prinst.spawnflagscache, 0, sizeof(evalc_t));

	if (externs->autocompile == PR_COMPILEALWAYS)	//always compile before loading
	{
		externs->Printf("Forcing compile of progs %s\n", filename);
		if (!CompileFile(progfuncs, filename))
			return false;
	}

//	CRC_Init (&pr_crc);

retry:
	hmark = PRHunkMark(progfuncs);
	pr_progs = externs->ReadFile(filename, PR_GetHeapBuffer, progfuncs, &fsz, false);
	if (!pr_progs)
	{
		if (externs->autocompile == PR_COMPILENEXIST || externs->autocompile == PR_COMPILECHANGED)	//compile if file is not found (if 2, we have already tried, so don't bother)
		{
			if (hmark==0xffffffff)	//first try
			{
				externs->Printf("couldn't open progs %s. Attempting to compile.\n", filename);
				CompileFile(progfuncs, filename);
			}
			pr_progs = externs->ReadFile(filename, PR_GetHeapBuffer, progfuncs, &fsz, false);
			if (!pr_progs)
			{
				externs->Printf("Couldn't find or compile file %s\n", filename);
				return false;
			}
		}
		else if (externs->autocompile == PR_COMPILEIGNORE)
			return false;
		else
		{
			externs->Printf("Couldn't find file %s\n", filename);
			return false;
		}
	}

//	for (i=0 ; i<len ; i++)
//		CRC_ProcessByte (&pr_crc, ((byte *)pr_progs)[i]);

// byte swap the header
#ifndef NOENDIAN
	for (i=0 ; i<standard_dprograms_t_size/sizeof(int); i++)
		((int *)pr_progs)[i] = PRLittleLong ( ((int *)pr_progs)[i] );
#endif

	if (pr_progs->version == PROG_VERSION)
	{
//		externs->Printf("Opening standard progs file \"%s\"\n", filename);
		current_progstate->structtype = PST_DEFAULT;
	}
	else if (pr_progs->version == PROG_QTESTVERSION)
	{
		current_progstate->structtype = PST_QTEST;
	}
	else if (pr_progs->version == PROG_EXTENDEDVERSION)
	{
#ifndef NOENDIAN
		for (i = standard_dprograms_t_size/sizeof(int); i < sizeof(dprograms_t)/sizeof(int); i++)
			((int *)pr_progs)[i] = PRLittleLong ( ((int *)pr_progs)[i] );
#endif
		if (pr_progs->secondaryversion == PROG_SECONDARYVERSION16)
		{
//			externs->Printf("Opening 16bit fte progs file \"%s\"\n", filename);
			current_progstate->structtype = PST_DEFAULT;
		}
		else if (pr_progs->secondaryversion == PROG_SECONDARYVERSION32)
		{
//			externs->Printf("Opening 32bit fte progs file \"%s\"\n", filename);
			current_progstate->structtype = PST_FTE32;
		}
		else if (pr_progs->secondaryversion == PROG_SECONDARYUHEXEN2)
		{
//			externs->Printf("Opening uhexen2 progs file \"%s\"\n", filename);
			current_progstate->structtype = PST_UHEXEN2;
			pr_progs->version = PROG_VERSION;	//not fte.
		}
		else if (pr_progs->secondaryversion == PROG_SECONDARYKKQWSV)
		{
//			externs->Printf("Opening KK7 progs file \"%s\"\n", filename);
			current_progstate->structtype = PST_KKQWSV;	//KK progs. Yuck. Disabling saving would be a VERY good idea.
			pr_progs->version = PROG_VERSION;	//not fte.
		}
		else
		{
			externs->Printf ("%s has no v7 verification code, assuming kkqwsv format\n", filename);
//			externs->Printf("Opening KK7 progs file \"%s\"\n", filename);
			current_progstate->structtype = PST_KKQWSV;	//KK progs. Yuck. Disabling saving would be a VERY good idea.
			pr_progs->version = PROG_VERSION;	//not fte.
		}
/*		else
		{
			externs->Printf ("Progs extensions are not compatible\nTry recompiling with the FTE compiler\n");
			HunkFree(hmark);
			pr_progs=NULL;
			return false;
		}
*/	}
	else
	{
		externs->Printf ("%s has wrong version number (%i should be %i)\n", filename, pr_progs->version, PROG_VERSION);
		PRHunkFree(progfuncs, hmark);
		pr_progs=NULL;
		return false;
	}

//progs contains enough info for use to recompile it.
	if (trysleft && (externs->autocompile == PR_COMPILECHANGED  || externs->autocompile == PR_COMPILEEXISTANDCHANGED) && pr_progs->version == PROG_EXTENDEDVERSION)
	{
		if (PR_TestRecompile(progfuncs))
		{
			externs->Printf("Source file has changed\nRecompiling.\n");
			if (CompileFile(progfuncs, filename))
			{
				PRHunkFree(progfuncs, hmark);
				pr_progs=NULL;

				trysleft--;
				goto retry;
			}
		}
	}
	if (!trysleft)	//the progs exists, let's just be happy about it.
		externs->Printf("Progs is out of date and uncompilable\n");

	if (externs->CheckHeaderCrc && !externs->CheckHeaderCrc(&progfuncs->funcs, prinst.pr_typecurrent, pr_progs->crc, filename))
	{
//		externs->Printf ("%s system vars have been modified, progdefs.h is out of date\n", filename);
		PRHunkFree(progfuncs, hmark);
		pr_progs=NULL;
		return false;
	}

	if (pr_progs->version == PROG_EXTENDEDVERSION && pr_progs->blockscompressed && !QC_decodeMethodSupported(2))
	{
		externs->Printf ("%s uses compression\n", filename);
		PRHunkFree(progfuncs, hmark);
		pr_progs=NULL;
		return false;
	}

	fnc = (dfunction_t *)((pbyte *)pr_progs + pr_progs->ofs_functions);
	pr_strings = ((char *)pr_progs + pr_progs->ofs_strings);
	current_progstate->globaldefs = *(void**)&gd16 = (void *)((pbyte *)pr_progs + pr_progs->ofs_globaldefs);
	current_progstate->fielddefs = *(void**)&fld16 = (void *)((pbyte *)pr_progs + pr_progs->ofs_fielddefs);
	current_progstate->statements = (void *)((pbyte *)pr_progs + pr_progs->ofs_statements);

	glob = pr_globals = (void *)((pbyte *)pr_progs + pr_progs->ofs_globals);
	current_progstate->globals_bytes = pr_progs->numglobals*sizeof(*pr_globals);

	pr_linenums=NULL;
	pr_types=NULL;
	if (pr_progs->version == PROG_EXTENDEDVERSION)
	{
		if (pr_progs->ofslinenums)
			pr_linenums = (int *)((pbyte *)pr_progs + pr_progs->ofslinenums);
		if (pr_progs->ofs_types)
			pr_types = (typeinfo_t *)((pbyte *)pr_progs + pr_progs->ofs_types);

		//start decompressing stuff...
		if (pr_progs->blockscompressed & 1)	//statements
		{
			switch(current_progstate->structtype)
			{
			case PST_DEFAULT:
				len=sizeof(dstatement16_t)*pr_progs->numstatements;
				break;
			case PST_FTE32:
			case PST_UHEXEN2:
				len=sizeof(dstatement32_t)*pr_progs->numstatements;
				break;
			default:
				externs->Sys_Error("Bad struct type");
				len = 0;
			}
			s = PRHunkAlloc(progfuncs, len, "dstatements");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_statements16), len, 2, (char *)(((int *)pr_statements16)+1), s);

			current_progstate->statements = (dstatement16_t *)s;
		}
		if (pr_progs->blockscompressed & 2)	//global defs
		{
			switch(current_progstate->structtype)
			{
			case PST_DEFAULT:
				len=sizeof(ddef16_t)*pr_progs->numglobaldefs;
				break;
			case PST_FTE32:
			case PST_UHEXEN2:
				len=sizeof(ddef32_t)*pr_progs->numglobaldefs;
				break;
			default:
				externs->Sys_Error("Bad struct type");
				len = 0;
			}
			s = PRHunkAlloc(progfuncs, len, "dglobaldefs");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_globaldefs16), len, 2, (char *)(((int *)pr_globaldefs16)+1), s);

			gd16 = *(ddef16_t**)&current_progstate->globaldefs = (ddef16_t *)s;
		}
		if (pr_progs->blockscompressed & 4)	//fields
		{
			switch(current_progstate->structtype)
			{
			case PST_DEFAULT:
				len=sizeof(ddef16_t)*pr_progs->numglobaldefs;
				break;
			case PST_FTE32:
			case PST_UHEXEN2:
				len=sizeof(ddef32_t)*pr_progs->numglobaldefs;
				break;
			default:
				externs->Sys_Error("Bad struct type");
				len = 0;
			}
			s = PRHunkAlloc(progfuncs, len, "progfieldtable");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_fielddefs16), len, 2, (char *)(((int *)pr_fielddefs16)+1), s);

			*(ddef16_t**)&current_progstate->fielddefs = (ddef16_t *)s;
		}
		if (pr_progs->blockscompressed & 8)	//functions
		{
			len=sizeof(dfunction_t)*pr_progs->numfunctions;
			s = PRHunkAlloc(progfuncs, len, "dfunctiontable");
			QC_decode(progfuncs, PRLittleLong(*(int *)fnc), len, 2, (char *)(((int *)fnc)+1), s);

			fnc = (dfunction_t *)s;
		}
		if (pr_progs->blockscompressed & 16)	//string table
		{
			len=sizeof(char)*pr_progs->numstrings;
			s = PRHunkAlloc(progfuncs, len, "dstringtable");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_strings), len, 2, (char *)(((int *)pr_strings)+1), s);

			pr_strings = (char *)s;
		}
		if (pr_progs->blockscompressed & 32)	//globals
		{
			len=sizeof(float)*pr_progs->numglobals;
			s = PRHunkAlloc(progfuncs, len + sizeof(float)*2, "dglobaltable");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_globals), len, 2, (char *)(((int *)pr_globals)+1), s);

			glob = pr_globals = (float *)s;
		}
		if (pr_linenums && pr_progs->blockscompressed & 64)	//line numbers
		{
			len=sizeof(int)*pr_progs->numstatements;
			s = PRHunkAlloc(progfuncs, len, "dlinenumtable");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_linenums), len, 2, (char *)(((int *)pr_linenums)+1), s);

			pr_linenums = (int *)s;
		}
		if (pr_types && pr_progs->blockscompressed & 128)	//types
		{
			len=sizeof(typeinfo_t)*pr_progs->numtypes;
			s = PRHunkAlloc(progfuncs, len, "dtypes");
			QC_decode(progfuncs, PRLittleLong(*(int *)pr_types), len, 2, (char *)(((int *)pr_types)+1), s);

			pr_types = (typeinfo_t *)s;
		}
	}

	len=sizeof(char)*pr_progs->numstrings;
	s = PRAddressableExtend(progfuncs, pr_strings, len, 0);
	pr_strings = (char *)s;

	len=sizeof(float)*pr_progs->numglobals;
	s = PRAddressableExtend(progfuncs, pr_globals, len, sizeof(float)*2);
	glob = pr_globals = (float *)s;

	if (progfuncs->funcs.stringtable)
	{
		stringadjust = pr_strings - progfuncs->funcs.stringtable;
		pointeradjust = (char*)glob - progfuncs->funcs.stringtable;
	}
	else
	{
		stringadjust = 0;
		pointeradjust = (char*)glob - pr_strings;
	}

	if (!pr_linenums)
	{
		unsigned int lnotype = *(unsigned int*)"LNOF";
		unsigned int version = 1;
		int ohm;
		unsigned int *file;
		char lnoname[128];
		ohm = PRHunkMark(progfuncs);

		strcpy(lnoname, filename);
		StripExtension(lnoname);
		strcat(lnoname, ".lno");
		file = externs->ReadFile(lnoname, PR_GetHeapBuffer, progfuncs, &fsz, false);
		if (file)
		{
			if (	file[0] != lnotype
				||	file[1] != version
				||	file[2] != pr_progs->numglobaldefs
				||	file[3] != pr_progs->numglobals
				||	file[4] != pr_progs->numfielddefs
				||	file[5] != pr_progs->numstatements
				)
			{
				PRHunkFree(progfuncs, ohm);	//whoops: old progs or incompatible
			}
			else
				pr_linenums = file + 6;
		}
	}

	pr_cp_functions = NULL;
//	pr_strings = ((char *)pr_progs + pr_progs->ofs_strings);
	gd16 = *(ddef16_t**)&current_progstate->globaldefs = (ddef16_t *)((pbyte *)pr_progs + pr_progs->ofs_globaldefs);
	fld16 = (ddef16_t *)((pbyte *)pr_progs + pr_progs->ofs_fielddefs);
//	pr_statements16 = (dstatement16_t *)((qbyte *)pr_progs + pr_progs->ofs_statements);
	pr_globals = glob;
	st16 = pr_statements16;
#undef pr_globals
#undef pr_globaldefs16
#undef pr_functions
#undef pr_statements16
#undef pr_fielddefs16


	current_progstate->edict_size = pr_progs->entityfields * 4 + externs->edictsize;

	if (sizeof(mfunction_t) > sizeof(qtest_function_t))
		externs->Sys_Error("assumption no longer works");

// byte swap the lumps
	switch(current_progstate->structtype)
	{
	case PST_QTEST:
		// qtest needs a struct remap
		pr_cp_functions = (mfunction_t*)fnc;
		fnc2 = pr_cp_functions;
		for (i=0 ; i<pr_progs->numfunctions; i++)
		{
			//qtest functions are bigger, so we can just do this in-place
			qtest_function_t qtfunc = ((qtest_function_t*)fnc)[i];

			fnc2[i].first_statement	= PRLittleLong (qtfunc.first_statement);
			fnc2[i].parm_start	= PRLittleLong (qtfunc.parm_start);
			fnc2[i].s_name	= (string_t)PRLittleLong (qtfunc.s_name);
			fnc2[i].s_file	= (string_t)PRLittleLong (qtfunc.s_file);
			fnc2[i].numparms	= PRLittleLong (qtfunc.numparms);
			fnc2[i].locals	= PRLittleLong (qtfunc.locals);

			for (j=0; j<MAX_PARMS;j++)
				fnc2[i].parm_size[j] = PRLittleLong (qtfunc.parm_size[j]);

			fnc2[i].s_name += stringadjust;
			fnc2[i].s_file += stringadjust;
		}
		break;
	case PST_KKQWSV:
	case PST_DEFAULT:
	case PST_FTE32:
	case PST_UHEXEN2:
		pr_cp_functions = PRHunkAlloc(progfuncs, sizeof(*pr_cp_functions)*pr_progs->numfunctions, "mfunctions");
		for (i=0,fnc2=pr_cp_functions; i<pr_progs->numfunctions; i++, fnc2++)
		{
			fnc2->first_statement	= PRLittleLong (fnc[i].first_statement);
			fnc2->parm_start	= PRLittleLong (fnc[i].parm_start);
			fnc2->s_name	= (string_t)PRLittleLong ((long)fnc[i].s_name) + stringadjust;
			fnc2->s_file	= (string_t)PRLittleLong ((long)fnc[i].s_file) + stringadjust;
			fnc2->numparms	= PRLittleLong (fnc[i].numparms);
			fnc2->locals	= PRLittleLong (fnc[i].locals);

			for (j=0; j<MAX_PARMS;j++)
				fnc2->parm_size[j] = fnc[i].parm_size[j];
		}
		break;
	default:
		externs->Sys_Error("Bad struct type");
	}

	//actual global values
#ifndef NOENDIAN
	for (i=0 ; i<pr_progs->numglobals ; i++)
		((int *)glob)[i] = PRLittleLong (((int *)glob)[i]);
#endif

	if (pr_types)
	{
		for (i=0 ; i<pr_progs->numtypes ; i++)
		{
#ifndef NOENDIAN
			pr_types[i].type = PRLittleLong(current_progstate->types[i].type);
			pr_types[i].next = PRLittleLong(current_progstate->types[i].next);
			pr_types[i].aux_type = PRLittleLong(current_progstate->types[i].aux_type);
			pr_types[i].num_parms = PRLittleLong(current_progstate->types[i].num_parms);
			pr_types[i].ofs = PRLittleLong(current_progstate->types[i].ofs);
			pr_types[i].size = PRLittleLong(current_progstate->types[i].size);
			pr_types[i].name = PRLittleLong(current_progstate->types[i].name);
#endif
			pr_types[i].name += stringadjust;
		}
	}

	QC_FlushProgsOffsets(progfuncs);
	switch(current_progstate->structtype)
	{
	case PST_KKQWSV:
	case PST_DEFAULT:
		//byteswap the globals and fix name offsets
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
#ifndef NOENDIAN
			gd16[i].type = PRLittleShort (gd16[i].type);
			gd16[i].ofs = PRLittleShort (gd16[i].ofs);
			gd16[i].s_name = (string_t)PRLittleLong ((long)gd16[i].s_name);
#endif
			gd16[i].s_name += stringadjust;
		}

		//byteswap fields and fix name offets. Also register the fields (which will result in some offset adjustments in the globals segment).
		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
#ifndef NOENDIAN
			fld16[i].type = PRLittleShort (fld16[i].type);
			fld16[i].ofs = PRLittleShort (fld16[i].ofs);
			fld16[i].s_name = (string_t)PRLittleLong ((long)fld16[i].s_name);
#endif
			if (reorg)
			{
				if (pr_types)
					type = pr_types[fld16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
				else
					type = fld16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);

				if (progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent)	//we need to make sure all fields appear in their original place.
					QC_RegisterFieldVar(&progfuncs->funcs, type, fld16[i].s_name+pr_strings, 4*(fld16[i].ofs+progfuncs->funcs.fieldadjust), fld16[i].ofs);
				else if (type == ev_vector)	//emit vector vars early, so their fields cannot be alocated before the vector itself. (useful against scramblers)
				{
					QC_RegisterFieldVar(&progfuncs->funcs, type, fld16[i].s_name+pr_strings, -1, fld16[i].ofs);
				}
			}
			else
			{
				fdef_t *nf;
				if (prinst.numfields+1>prinst.maxfields)
				{
					i = prinst.maxfields;
					prinst.maxfields += 32;
					nf = externs->memalloc(sizeof(fdef_t) * prinst.maxfields);
					memcpy(nf, prinst.field, sizeof(fdef_t) * i);
					externs->memfree(prinst.field);
					prinst.field = nf;
				}
				nf = &prinst.field[prinst.numfields];
				nf->name = fld16[i].s_name+pr_strings;
				nf->type = fld16[i].type;
				nf->progsofs = fld16[i].ofs;
				nf->ofs = fld16[i].ofs;

				if (prinst.fields_size < (nf->ofs+type_size[nf->type])*sizeof(pvec_t))
				{
					prinst.fields_size = (nf->ofs+type_size[nf->type])*sizeof(pvec_t);
					progfuncs->funcs.activefieldslots = nf->ofs+type_size[nf->type];
				}

				prinst.numfields++;
			}
			fld16[i].s_name += stringadjust;
		}
		if (reorg && !(progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent))
		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
			if (pr_types)
				type = pr_types[fld16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
			else
				type = fld16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
			if (type != ev_vector)
				QC_RegisterFieldVar(&progfuncs->funcs, type, fld16[i].s_name+pr_strings-stringadjust, -1, fld16[i].ofs);
		}

		break;
	case PST_QTEST:
		// qtest needs a struct remap
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			qtest_def_t qtdef = ((qtest_def_t *)pr_globaldefs32)[i];

			pr_globaldefs32[i].type = qtdef.type;
			pr_globaldefs32[i].s_name = qtdef.s_name;
			pr_globaldefs32[i].ofs = qtdef.ofs;
		}
		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
			qtest_def_t qtdef = ((qtest_def_t *)pr_fielddefs32)[i];

			pr_fielddefs32[i].type = qtdef.type;
			pr_fielddefs32[i].s_name = qtdef.s_name;
			pr_fielddefs32[i].ofs = qtdef.ofs;
		}
		// passthrough
	case PST_FTE32:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
#ifndef NOENDIAN
			pr_globaldefs32[i].type = PRLittleLong (pr_globaldefs32[i].type);
			pr_globaldefs32[i].ofs = PRLittleLong (pr_globaldefs32[i].ofs);
			pr_globaldefs32[i].s_name = (string_t)PRLittleLong ((long)pr_globaldefs32[i].s_name);
#endif
			pr_globaldefs32[i].s_name += stringadjust;
		}

		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
	#ifndef NOENDIAN
			pr_fielddefs32[i].type = PRLittleLong (pr_fielddefs32[i].type);
			pr_fielddefs32[i].ofs = PRLittleLong (pr_fielddefs32[i].ofs);
			pr_fielddefs32[i].s_name = (string_t)PRLittleLong ((long)pr_fielddefs32[i].s_name);
	#endif

			if (reorg)
			{
				if (pr_types)
					type = pr_types[pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
				else
					type = pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
				if (progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent)	//we need to make sure all fields appear in their original place.
					QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings, 4*(pr_fielddefs32[i].ofs+progfuncs->funcs.fieldadjust), -1);
				else if (type == ev_vector)
					QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings, -1, pr_fielddefs32[i].ofs);
			}
			pr_fielddefs32[i].s_name += stringadjust;
		}
		if (reorg && !(progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent))
		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
			if (pr_types)
				type = pr_types[pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
			else
				type = pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
			if (type != ev_vector)
				QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings-stringadjust, -1, pr_fielddefs32[i].ofs);
		}
		break;
	case PST_UHEXEN2:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			pr_globaldefs32[i].type = (unsigned int)PRLittleLong (pr_globaldefs32[i].type)>>16;
#ifndef NOENDIAN
			pr_globaldefs32[i].ofs = PRLittleLong (pr_globaldefs32[i].ofs);
			pr_globaldefs32[i].s_name = (string_t)PRLittleLong ((long)pr_globaldefs32[i].s_name);
#endif
			pr_globaldefs32[i].s_name += stringadjust;
		}

		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
			pr_fielddefs32[i].type = (unsigned int)PRLittleLong (pr_fielddefs32[i].type)>>16;
#ifndef NOENDIAN
			pr_fielddefs32[i].ofs = PRLittleLong (pr_fielddefs32[i].ofs);
			pr_fielddefs32[i].s_name = (string_t)PRLittleLong ((long)pr_fielddefs32[i].s_name);
#endif

			if (reorg)
			{
				if (pr_types)
					type = pr_types[pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
				else
					type = pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
				if (progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent)	//we need to make sure all fields appear in their original place.
					QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings, 4*(pr_fielddefs32[i].ofs+progfuncs->funcs.fieldadjust), -1);
				else if (type == ev_vector)
					QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings, -1, pr_fielddefs32[i].ofs);
			}
			pr_fielddefs32[i].s_name += stringadjust;
		}
		if (reorg && !(progfuncs->funcs.fieldadjust && !prinst.pr_typecurrent))
		for (i=0 ; i<pr_progs->numfielddefs ; i++)
		{
			if (pr_types)
				type = pr_types[pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
			else
				type = pr_fielddefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);
			if (type != ev_vector)
				QC_RegisterFieldVar(&progfuncs->funcs, type, pr_fielddefs32[i].s_name+pr_strings-stringadjust, -1, pr_fielddefs32[i].ofs);
		}
		break;
	default:
		externs->Sys_Error("Bad struct type");
	}

//ifstring fixes arn't performed anymore.
//the following switch just fixes endian and hexen2 calling conventions (by using different opcodes).
	switch(current_progstate->structtype)
	{
	case PST_QTEST:
		for (i=0 ; i<pr_progs->numstatements ; i++)
		{
			qtest_statement_t qtst = ((qtest_statement_t*)st16)[i];

			st16[i].op = PRLittleShort(qtst.op);
			st16[i].a = PRLittleShort(qtst.a);
			st16[i].b = PRLittleShort(qtst.b);
			st16[i].c = PRLittleShort(qtst.c);
			// could use the line info as lno information maybe? is it really worth it?
			// also never assuming h2 calling mechanism
		}
		PR_CleanUpStatements16(progfuncs, st16, false);
		break;
	case PST_DEFAULT:
		for (i=0 ; i<pr_progs->numstatements ; i++)
		{
#ifndef NOENDIAN
			st16[i].op = PRLittleShort(st16[i].op);
			st16[i].a = PRLittleShort(st16[i].a);
			st16[i].b = PRLittleShort(st16[i].b);
			st16[i].c = PRLittleShort(st16[i].c);
#endif
			if (st16[i].op >= OP_CALL1 && st16[i].op <= OP_CALL8)
			{
				if (st16[i].b)
				{
					hexencalling = true;
					break;
				}
			}
		}
		PR_CleanUpStatements16(progfuncs, st16, hexencalling);
		break;

	case PST_UHEXEN2:
		hexencalling = true;
		for (i=0 ; i<pr_progs->numstatements ; i++)
		{
			pr_statements32[i].op = (unsigned int)PRLittleLong(pr_statements32[i].op)>>16;
#ifndef NOENDIAN
			pr_statements32[i].a = PRLittleLong(pr_statements32[i].a);
			pr_statements32[i].b = PRLittleLong(pr_statements32[i].b);
			pr_statements32[i].c = PRLittleLong(pr_statements32[i].c);
#endif
		}
		PR_CleanUpStatements32(progfuncs, pr_statements32, hexencalling);
		break;
	case PST_KKQWSV:
	case PST_FTE32:
		for (i=0 ; i<pr_progs->numstatements ; i++)
		{
#ifndef NOENDIAN
			pr_statements32[i].op = PRLittleLong(pr_statements32[i].op);
			pr_statements32[i].a = PRLittleLong(pr_statements32[i].a);
			pr_statements32[i].b = PRLittleLong(pr_statements32[i].b);
			pr_statements32[i].c = PRLittleLong(pr_statements32[i].c);
#endif
			if (pr_statements32[i].op >= OP_CALL1 && pr_statements32[i].op <= OP_CALL8)
			{
				if (pr_statements32[i].b)
				{
					hexencalling = true;
					break;
				}
			}
		}
		PR_CleanUpStatements32(progfuncs, pr_statements32, hexencalling);
		break;
	}

/*
	if (headercrc == -1)
	{
		isfriked = true;
		if (current_progstate->structtype != PST_DEFAULT)
			externs->Sys_Error("Decompiling a bigprogs");
		return true;
	}
*/
	progstype = current_progstate-pr_progstate;

//	QC_StartShares(progfuncs);

	isfriked = true;
	if (!prinst.pr_typecurrent)	//progs 0 always acts as string stripped.
		isfriked = -1;			//partly to avoid some bad/optimised progs.


	basictypetable = NULL;
	if (prinst.reorganisefields == 2)
	{
		switch(current_progstate->structtype)
		{	//gmqcc fucks up the globals. it writes FLOAT defs instead of field defs. stupid stupid stupid.
		case PST_DEFAULT:
			{
				dstatement16_t *st = current_progstate->statements;
				basictypetable = externs->memalloc(sizeof(*basictypetable) * pr_progs->numglobals);
				memset(basictypetable, 0, sizeof(*basictypetable) * pr_progs->numglobals);
				for (i = 0; i < pr_progs->numstatements; i++)
				{
					switch(st[i].op)
					{
					case OP_ADDRESS:
						if (st[i+1].op == OP_STOREP_V && st[i+1].b == st[i].c)
						{	//following stores a vector to this field.
							if (st[i].b+2u < pr_progs->numglobals)
							{	//vectors are usually 3 fields. if they're not then we're screwed.
								basictypetable[st[i].b+0] = ev_field;
								basictypetable[st[i].b+1] = ev_field;
								basictypetable[st[i].b+2] = ev_field;
							}
							break;
						}
						//fallthrough
					case OP_LOAD_F:
					case OP_LOAD_S:
					case OP_LOAD_ENT:
					case OP_LOAD_FLD:
					case OP_LOAD_FNC:
					case OP_LOAD_I:
					case OP_LOAD_P:
						if (st[i].b < pr_progs->numglobals)
							basictypetable[st[i].b] = ev_field;
						break;
					case OP_LOAD_V:
						if (st[i].b+2u < pr_progs->numglobals)
						{	//vectors are usually 3 fields. if they're not then we're screwed.
							basictypetable[st[i].b+0] = ev_field;
							basictypetable[st[i].b+1] = ev_field;
							basictypetable[st[i].b+2] = ev_field;
						}
						break;
					}
				}

				for (i = 0; i < pr_progs->numglobaldefs; i++)
				{
					ddef16_t *gd = gd16+i;
					switch(gd->type & ~(DEF_SAVEGLOBAL|DEF_SHARED))
					{
					case ev_field:	//depend on _y _z to mark those globals.
						basictypetable[gd->ofs] = ev_field;
						break;
					}
				}

				for (i = 0; i < pr_progs->numglobals; i++)
				{
					if (basictypetable[i] == ev_field)
						QC_AddFieldGlobal(&progfuncs->funcs, (int *)glob + i);
				}
				externs->memfree(basictypetable);
			}
			break;
		case PST_QTEST:		//not likely to need this
		case PST_KKQWSV:	//fixme...
		case PST_FTE32:		//fingers crossed...
		case PST_UHEXEN2:
			break;
		}
	}

//	len = 0;
	switch(current_progstate->structtype)
	{
	case PST_DEFAULT:
	case PST_KKQWSV:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			if (pr_types)
				type = pr_types[gd16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
			else
				type = gd16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);

			if (gd16[i].type & DEF_SHARED)
			{
				gd16[i].type &= ~DEF_SHARED;
				if (pr_types)
					QC_AddSharedVar(&progfuncs->funcs, gd16[i].ofs, pr_types[gd16[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].size);
				else
					QC_AddSharedVar(&progfuncs->funcs, gd16[i].ofs, type_size[type]);
			}
			switch(type)
			{
			case ev_field:
				if (reorg && !basictypetable)
					QC_AddSharedFieldVar(&progfuncs->funcs, i, pr_strings - stringadjust);
				break;
			case ev_pointer:
				if (((int *)glob)[gd16[i].ofs] & 0x80000000)
				{
					((int *)glob)[gd16[i].ofs] &= ~0x80000000;
					((int *)glob)[gd16[i].ofs] += pointeradjust;
					break;
				}
				FALLTHROUGH
			case ev_string:
				if (((unsigned int *)glob)[gd16[i].ofs]>=progstate->progs->numstrings)
					externs->Printf("PR_LoadProgs: invalid string value (%x >= %x) in '%s'\n", ((unsigned int *)glob)[gd16[i].ofs], progstate->progs->numstrings, gd16[i].s_name+pr_strings-stringadjust);
				else if (isfriked != -1)
				{
					if (((int *)glob)[gd16[i].ofs])	//quakec uses string tables. 0 must remain null, or 'if (s)' can break.
					{
						((int *)glob)[gd16[i].ofs] += stringadjust;
						isfriked = false;
					}
					else
						((int *)glob)[gd16[i].ofs] = 0;
				}
				break;
			case ev_function:
				if (((int *)glob)[gd16[i].ofs])	//don't change null funcs
				{
//					if (fnc[((int *)glob)[gd16[i].ofs]].first_statement>=0)	//this is a hack. Make all builtins switch to the main progs first. Allows builtin funcs to cache vars from just the main progs.
						((int *)glob)[gd16[i].ofs] |= progstype << 24;
				}
				break;
			}
		}
		break;
	case PST_QTEST:
	case PST_FTE32:
	case PST_UHEXEN2:
		for (i=0 ; i<pr_progs->numglobaldefs ; i++)
		{
			if (pr_types)
				type = pr_types[pr_globaldefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].type;
			else
				type = pr_globaldefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL);

			if (pr_globaldefs32[i].type & DEF_SHARED)
			{
				pr_globaldefs32[i].type &= ~DEF_SHARED;
				if (pr_types)
					QC_AddSharedVar(&progfuncs->funcs, pr_globaldefs32[i].ofs, pr_types[pr_globaldefs32[i].type & ~(DEF_SHARED|DEF_SAVEGLOBAL)].size);
				else
					QC_AddSharedVar(&progfuncs->funcs, pr_globaldefs32[i].ofs, type_size[type]);
			}
			switch(type)
			{
			case ev_field:
				QC_AddSharedFieldVar(&progfuncs->funcs, i, pr_strings - stringadjust);
				break;
			case ev_pointer:
				if (((int *)glob)[pr_globaldefs32[i].ofs] & 0x80000000)
				{
					((int *)glob)[pr_globaldefs32[i].ofs] &= ~0x80000000;
					((int *)glob)[pr_globaldefs32[i].ofs] += pointeradjust;
					break;
				}
				FALLTHROUGH
			case ev_string:
				if (((unsigned int *)glob)[pr_globaldefs32[i].ofs]>=progstate->progs->numstrings)
					externs->Printf("PR_LoadProgs: invalid string value (%x >= %x) in '%s'\n", ((unsigned int *)glob)[pr_globaldefs32[i].ofs], progstate->progs->numstrings, pr_globaldefs32[i].s_name+pr_strings-stringadjust);
				else if (((int *)glob)[pr_globaldefs32[i].ofs])	//quakec uses string tables. 0 must remain null, or 'if (s)' can break.
				{
					((int *)glob)[pr_globaldefs32[i].ofs] += stringadjust;
					isfriked = false;
				}
				break;
			case ev_function:
				if (((int *)glob)[pr_globaldefs32[i].ofs])	//don't change null funcs
					((int *)glob)[pr_globaldefs32[i].ofs] |= progstype << 24;
				break;
			}
		}

		if (pr_progs->version == PROG_EXTENDEDVERSION && pr_progs->numbodylessfuncs)
		{
			s = &((char *)pr_progs)[pr_progs->ofsbodylessfuncs];
			for (i = 0; i < pr_progs->numbodylessfuncs; i++)
			{
				d32 = ED_FindGlobal32(progfuncs, s);
				d2 = ED_FindGlobalOfsFromProgs(progfuncs, &pr_progstate[0], s, ev_function);
				if (!d2)
					externs->Sys_Error("Runtime-linked function %s was not found in existing progs", s);
				if (!d32)
					externs->Sys_Error("Couldn't find def for \"%s\"", s);
				((int *)glob)[d32->ofs] = (*(func_t *)&pr_progstate[0].globals[*d2]);

				s+=strlen(s)+1;
			}
		}
		break;
	default:
		externs->Sys_Error("Bad struct type");
	}

	if ((isfriked && prinst.pr_typecurrent))	//friked progs only allow one file.
	{
		externs->Printf("You are trying to load a string-stripped progs as an addon.\nThis behaviour is not supported. Try removing some optimizations.");
		PRHunkFree(progfuncs, hmark);
		pr_progs=NULL;
		return false;
	}

	pr_strings+=stringadjust;
	if (!progfuncs->funcs.stringtable)
		progfuncs->funcs.stringtable = pr_strings;

	if (progfuncs->funcs.stringtablesize + progfuncs->funcs.stringtable < pr_strings + pr_progs->numstrings)
		progfuncs->funcs.stringtablesize = (pr_strings + pr_progs->numstrings) - progfuncs->funcs.stringtable;

	//make sure the localstack is addressable to the qc, so we can OP_PUSH okay.
	if (!prinst.localstack)
		prinst.localstack = PRAddressableExtend(progfuncs, NULL, 0, sizeof(float)*LOCALSTACK_SIZE);

	if (externs->MapNamedBuiltin)
	{
		for (i=0,fnc2=pr_cp_functions; i<pr_progs->numfunctions; i++, fnc2++)
		{
			if (i && !fnc2->first_statement)
				fnc2->first_statement = -externs->MapNamedBuiltin(&progfuncs->funcs, pr_progs->crc, PR_StringToNative(&progfuncs->funcs, fnc2->s_name));
		}
	}

	eval = PR_FindGlobal(&progfuncs->funcs, "thisprogs", progstype, NULL);
	if (eval)
		eval->prog = progstype;

	switch(current_progstate->structtype)
	{
	case PST_DEFAULT:
		if (pr_progs->version == PROG_EXTENDEDVERSION && pr_progs->numbodylessfuncs)
		{
			s = &((char *)pr_progs)[pr_progs->ofsbodylessfuncs];
			for (i = 0; i < pr_progs->numbodylessfuncs; i++)
			{
				d16 = ED_FindGlobal16(progfuncs, s);
				if (!d16)
				{
					externs->Printf("\"%s\" requires the external function \"%s\", but the definition was stripped\n", filename, s);
					PRHunkFree(progfuncs, hmark);
					pr_progs=NULL;
					return false;
				}

				((int *)glob)[d16->ofs] = PR_FindFunc(&progfuncs->funcs, s, PR_ANY);
				if (!((int *)glob)[d16->ofs])
					externs->Printf("Warning: Runtime-linked function %s could not be found (loading %s)\n", s, filename);
				s+=strlen(s)+1;
			}
		}
		break;
	case PST_QTEST:
	case PST_KKQWSV:
		break;	//cannot happen anyway.
	case PST_UHEXEN2:
	case PST_FTE32:
		if (pr_progs->version == PROG_EXTENDEDVERSION && pr_progs->numbodylessfuncs)
		{
			s = &((char *)pr_progs)[pr_progs->ofsbodylessfuncs];
			for (i = 0; i < pr_progs->numbodylessfuncs; i++)
			{
				d32 = ED_FindGlobal32(progfuncs, s);
				if (!d32)
				{
					externs->Printf("\"%s\" requires the external function \"%s\", but the definition was stripped\n", filename, s);
					PRHunkFree(progfuncs, hmark);
					pr_progs=NULL;
					return false;
				}

				((int *)glob)[d32->ofs] = PR_FindFunc(&progfuncs->funcs, s, PR_ANY);
				if (!((int *)glob)[d32->ofs])
					externs->Printf("Warning: Runtime-linked function %s could not be found (loading %s)\n", s, filename);
				s+=strlen(s)+1;
			}
		}
		break;
	}

	eval = PR_FindGlobal(&progfuncs->funcs, "__ext__fasttrackarrays", PR_CURRENT, NULL);
	if (eval)	//we support these opcodes
		eval->_float = true;

	return true;
}



struct edict_s *PDECL QC_EDICT_NUM(pubprogfuncs_t *ppf, unsigned int n)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	if (n >= prinst.maxedicts)
		externs->Sys_Error ("QCLIB: EDICT_NUM: bad number %i", n);

	return (struct edict_s*)prinst.edicttable[n];
}

unsigned int PDECL QC_NUM_FOR_EDICT(pubprogfuncs_t *ppf, struct edict_s *e)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	edictrun_t *er = (edictrun_t*)e;
	if (!er || er->entnum >= prinst.maxedicts)
		externs->Sys_Error ("QCLIB: NUM_FOR_EDICT: bad pointer (%p)", e);
	return er->entnum;
}
