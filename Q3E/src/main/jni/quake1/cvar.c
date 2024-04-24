/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2021 DarkPlaces contributors

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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

const char *cvar_dummy_description = "custom cvar";
static const char *cvar_null_string = "";

cvar_state_t cvars_all;
cvar_state_t cvars_null;

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar(cvar_state_t *cvars, const char *var_name, unsigned neededflags)
{
	unsigned hashindex;
	cvar_hash_t *hash;

	// use hash lookup to minimize search time
	hashindex = CRC_Block((const unsigned char *)var_name, strlen(var_name)) % CVAR_HASHSIZE;
	for (hash = cvars->hashtable[hashindex];hash;hash = hash->next)
		if (!strcmp (var_name, hash->cvar->name) && (hash->cvar->flags & neededflags))
			return hash->cvar;
		else
			for (char **alias = hash->cvar->aliases; alias && *alias; alias++)
				if (!strcmp (var_name, *alias) && (hash->cvar->flags & neededflags))
					return hash->cvar;
	return NULL;
}

cvar_t *Cvar_FindVarAfter(cvar_state_t *cvars, const char *prev_var_name, unsigned neededflags)
{
	cvar_t *var;

	if (*prev_var_name)
	{
		var = Cvar_FindVar(cvars, prev_var_name, neededflags);
		if (!var)
			return NULL;
		var = var->next;
	}
	else
		var = cvars->vars;

	// search for the next cvar matching the needed flags
	while (var)
	{
		if (var->flags & neededflags)
			break;
		var = var->next;
	}
	return var;
}

/**
 * Returns a pointer to the pointer stored in hashtable[] (or the one it links to)
 * because we'll need to update that when deleting a cvar as other cvar(s) may share its hashindex.
 */
static cvar_hash_t **Cvar_FindVarLink(cvar_state_t *cvars, const char *var_name, cvar_t **prev_alpha, unsigned neededflags)
{
	unsigned hashindex;
	cvar_t *cvar;
	cvar_hash_t **hashlinkptr;

	// use hash lookup to minimize search time
	hashindex = CRC_Block((const unsigned char *)var_name, strlen(var_name)) % CVAR_HASHSIZE;
	if(prev_alpha) *prev_alpha = NULL;
	hashlinkptr = &cvars->hashtable[hashindex];
	for (hashlinkptr = &cvars->hashtable[hashindex]; *hashlinkptr; hashlinkptr = &(*hashlinkptr)->next)
	{
		cvar = (*hashlinkptr)->cvar;
		if (!strcmp (var_name, cvar->name) && (cvar->flags & neededflags))
			goto match;
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strcmp (var_name, *alias) && (cvar->flags & neededflags))
					goto match;
	}
	return NULL;

match:
	if(!prev_alpha || cvar == cvars->vars)
		return hashlinkptr;
	*prev_alpha = cvars->vars;
	// if prev_alpha happens to become NULL then there has been some inconsistency elsewhere
	// already - should I still insert '*prev_alpha &&' in the loop?
	while((*prev_alpha)->next != cvar)
		*prev_alpha = (*prev_alpha)->next;
	return hashlinkptr;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValueOr(cvar_state_t *cvars, const char *var_name, float def, unsigned neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return def;
	return atof (var->string);
}

float Cvar_VariableValue(cvar_state_t *cvars, const char *var_name, unsigned neededflags)
{
	return Cvar_VariableValueOr(cvars, var_name, 0, neededflags);
}

/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableStringOr(cvar_state_t *cvars, const char *var_name, const char *def, unsigned neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return def;
	return var->string;
}

const char *Cvar_VariableString(cvar_state_t *cvars, const char *var_name, unsigned neededflags)
{
	return Cvar_VariableStringOr(cvars, var_name, cvar_null_string, neededflags);
}

/*
============
Cvar_VariableDefString
============
*/
const char *Cvar_VariableDefString(cvar_state_t *cvars, const char *var_name, unsigned neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return cvar_null_string;
	return var->defstring;
}

/*
============
Cvar_VariableDescription
============
*/
const char *Cvar_VariableDescription(cvar_state_t *cvars, const char *var_name, unsigned neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return cvar_null_string;
	return var->description;
}


/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable(cvar_state_t *cvars, const char *partial, unsigned neededflags)
{
	cvar_t		*cvar;
	size_t		len;

	len = strlen(partial);

	if (!len)
		return NULL;

// check functions
	for (cvar=cvars->vars ; cvar ; cvar=cvar->next)
		if (!strncasecmp (partial,cvar->name, len) && (cvar->flags & neededflags))
			return cvar->name;

	return NULL;
}


/*
	CVar_CompleteCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com

*/
int Cvar_CompleteCountPossible(cvar_state_t *cvars, const char *partial, unsigned neededflags)
{
	cvar_t	*cvar;
	size_t	len;
	int		h;

	h = 0;
	len = strlen(partial);

	if (!len)
		return	0;

	// Loop through the cvars and count all possible matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
		if (!strncasecmp(partial, cvar->name, len) && (cvar->flags & neededflags))
			h++;
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strncasecmp(partial, *alias, len) && (cvar->flags & neededflags))
					h++;
		
	return h;
}

/*
	CVar_CompleteBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
const char **Cvar_CompleteBuildList(cvar_state_t *cvars, const char *partial, unsigned neededflags)
{
	const cvar_t *cvar;
	size_t len = 0;
	size_t bpos = 0;
	size_t sizeofbuf = (Cvar_CompleteCountPossible(cvars, partial, neededflags) + 1) * sizeof(const char *);
	const char **buf;

	len = strlen(partial);
	buf = (const char **)Mem_Alloc(tempmempool, sizeofbuf + sizeof(const char *));
	// Loop through the alias list and print all matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
		if (!strncasecmp(partial, cvar->name, len) && (cvar->flags & neededflags))
			buf[bpos++] = cvar->name;
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strncasecmp(partial, *alias, len) && (cvar->flags & neededflags))
					buf[bpos++] = *alias;
		

	buf[bpos] = NULL;
	return buf;
}

void Cvar_PrintHelp(cvar_t *cvar, const char *name, qbool full)
{
	// Aliases are purple, cvars are yellow
	if (strcmp(cvar->name, name))
		Con_Printf("^6%s^7 (alias of ^3%s^7)", name, cvar->name);
	else
		Con_Printf("^3%s^7", name);
	Con_Printf(" is \"%s^7\" [\"%s^7\"]", ((cvar->flags & CF_PRIVATE) ? "********"/*hunter2*/ : cvar->string), cvar->defstring);
	if (full)
		Con_Printf(" %s", cvar->description);
	Con_Print("\n");
}

// written by LadyHavoc
void Cvar_CompleteCvarPrint(cvar_state_t *cvars, const char *partial, unsigned neededflags)
{
	cvar_t *cvar;
	size_t len = strlen(partial);
	// Loop through the command list and print all matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
		if (!strncasecmp(partial, cvar->name, len) && (cvar->flags & neededflags))
			Cvar_PrintHelp(cvar, cvar->name, true);
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strncasecmp (partial, *alias, len) && (cvar->flags & neededflags))
					Cvar_PrintHelp(cvar, *alias, true);

		
}

/// Check if a cvar is held by some progs
/// in which case its name is returned, otherwise NULL.
static const char *Cvar_IsAutoCvar(cvar_t *var)
{
	int i;
	prvm_prog_t *prog;
	for (i = 0;i < PRVM_PROG_MAX;i++)
	{
		prog = &prvm_prog_list[i];
		if (prog->loaded && var->globaldefindex[i] >= 0)
			return prog->name;
	}
	return NULL;
}

/// we assume that prog is already set to the target progs
static void Cvar_UpdateAutoCvar(cvar_t *var)
{
	int i;
	int j;
	const char *s;
	vec3_t v;
	prvm_prog_t *prog;
	for (i = 0;i < PRVM_PROG_MAX;i++)
	{
		prog = &prvm_prog_list[i];
		if (prog->loaded && var->globaldefindex[i] >= 0)
		{
			// MUST BE SYNCED WITH prvm_edict.c PRVM_Prog_Load
			switch(prog->globaldefs[var->globaldefindex[i]].type & ~DEF_SAVEGLOBAL)
			{
			case ev_float:
				PRVM_GLOBALFIELDFLOAT(prog->globaldefs[var->globaldefindex[i]].ofs) = var->value;
				break;
			case ev_vector:
				s = var->string;
				VectorClear(v);
				for (j = 0;j < 3;j++)
				{
					while (*s && ISWHITESPACE(*s))
						s++;
					if (!*s)
						break;
					v[j] = atof(s);
					while (!ISWHITESPACE(*s))
						s++;
					if (!*s)
						break;
				}
				VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
				break;
			case ev_string:
				PRVM_ChangeEngineString(prog, var->globaldefindex_stringno[i], var->string);
				PRVM_GLOBALFIELDSTRING(prog->globaldefs[var->globaldefindex[i]].ofs) = var->globaldefindex_stringno[i];
				break;
			}
		}
	}
}

/// called after loading a savegame
void Cvar_UpdateAllAutoCvars(cvar_state_t *cvars)
{
	cvar_t *var;
	for (var = cvars->vars ; var ; var = var->next)
		Cvar_UpdateAutoCvar(var);
}

void Cvar_Callback(cvar_t *var)
{
	if (var == NULL)
	{
		Con_Print(CON_WARN "Cvar_Callback: var == NULL\n");
		return;
	}

	if(var->callback)
		var->callback(var);
}

/*
============
Cvar_Set
============
*/
extern cvar_t sv_disablenotify;
static void Cvar_SetQuick_Internal (cvar_t *var, const char *value)
{
	qbool changed;
	size_t valuelen;

	changed = strcmp(var->string, value) != 0;
	// LadyHavoc: don't reallocate when there is no change
	if (!changed)
		goto cvar_callback;

	// LadyHavoc: don't reallocate when the buffer is the same size
	valuelen = strlen(value);
	if (!var->string || strlen(var->string) != valuelen)
	{
		Z_Free ((char *)var->string);	// free the old value string

		var->string = (char *)Z_Malloc (valuelen + 1);
	}
	memcpy ((char *)var->string, value, valuelen + 1);
	var->value = atof (var->string);
	var->integer = (int) var->value;
	if ((var->flags & CF_NOTIFY) && sv.active && !sv_disablenotify.integer)
		SV_BroadcastPrintf("\003^3Server cvar \"%s\" changed to \"%s\"\n", var->name, var->string);
#if 0
	// TODO: add infostring support to the server?
	if ((var->flags & CF_SERVERINFO) && changed && sv.active)
	{
		InfoString_SetValue(svs.serverinfo, sizeof(svs.serverinfo), var->name, var->string);
		if (sv.active)
		{
			MSG_WriteByte (&sv.reliable_datagram, svc_serverinfostring);
			MSG_WriteString (&sv.reliable_datagram, var->name);
			MSG_WriteString (&sv.reliable_datagram, var->string);
		}
	}
#endif
	if (var->flags & CF_USERINFO)
		CL_SetInfo(var->name, var->string, true, false, false, false);

	Cvar_UpdateAutoCvar(var);

cvar_callback:
	// Call the function stored in the cvar for bounds checking, cleanup, etc
	Cvar_Callback(var);
}

void Cvar_SetQuick (cvar_t *var, const char *value)
{
	if (var == NULL)
	{
		Con_Print(CON_WARN "Cvar_SetQuick: var == NULL\n");
		return;
	}

	if (!(var->flags & CF_REGISTERED) && !(var->flags & CF_ALLOCATED))
	{
		Con_Printf(CON_WARN "Warning: Cvar_SetQuick() cannot set unregistered cvar \"%s\"\n", var->name);
		return; // setting an unregistered engine cvar crashes
	}

	if (developer_extra.integer)
		Con_DPrintf("Cvar_SetQuick({\"%s\", \"%s\", %i, \"%s\"}, \"%s\");\n", var->name, var->string, var->flags, var->defstring, value);

	Cvar_SetQuick_Internal(var, value);
}

void Cvar_Set(cvar_state_t *cvars, const char *var_name, const char *value)
{
	cvar_t *var;
	var = Cvar_FindVar(cvars, var_name, ~0);
	if (var == NULL)
	{
		Con_Printf(CON_WARN "Cvar_Set: variable %s not found\n", var_name);
		return;
	}
	Cvar_SetQuick(var, value);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValueQuick(cvar_t *var, float value)
{
	char val[MAX_INPUTLINE];

	if ((float)((int)value) == value)
		dpsnprintf(val, sizeof(val), "%i", (int)value);
	else
		dpsnprintf(val, sizeof(val), "%f", value);
	Cvar_SetQuick(var, val);
}

void Cvar_SetValue(cvar_state_t *cvars, const char *var_name, float value)
{
	char val[MAX_INPUTLINE];

	if ((float)((int)value) == value)
		dpsnprintf(val, sizeof(val), "%i", (int)value);
	else
		dpsnprintf(val, sizeof(val), "%f", value);
	Cvar_Set(cvars, var_name, val);
}

void Cvar_RegisterCallback(cvar_t *variable, void (*callback)(cvar_t *))
{
	if (variable == NULL)
	{
		Con_Print(CON_WARN "Cvar_RegisterCallback: var == NULL\n");
		return;
	}

	if (!(variable->flags & cmd_local->cvars_flagsmask))
	{
		if (developer_extra.integer)
			Con_DPrintf("^6Cvar_RegisterCallback: rejecting cvar \"%s\"\n", variable->name);
		return;
	}

	variable->callback = callback;
}

void Cvar_RegisterVirtual(cvar_t *variable, const char *name )
{
	cvar_state_t *cvars = &cvars_all;
	cvar_hash_t *hash;
	unsigned hashindex;

	if (!(variable->flags & cmd_local->cvars_flagsmask))
	{
		if (developer_extra.integer)
			Con_DPrintf("^6Cvar_RegisterVirtual: rejecting cvar \"%s\" alias \"%s\"\n", variable->name, name);
		return;
	}

	if(!*name)
	{
		Con_Printf(CON_WARN "Cvar_RegisterVirtual: invalid virtual cvar name\n");
		return;
	}

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, name))
	{
		Con_Printf(CON_WARN "Cvar_RegisterVirtual: %s is a command\n", name);
		return;
	}

	if(Cvar_FindVar(&cvars_all, name, 0))
	{
		Con_Printf(CON_WARN "Cvar_RegisterVirtual: %s is a cvar\n", name);
		return;
	}

	// Resize the variable->aliases list to have room for another entry and a null terminator.
	// This zero-pads when resizing, so we don't need to write the NULL terminator manually here.
	// Also if aliases is NULL this allocates fresh for the correct size, so it's fine to just do this.
	variable->aliases = (char **)Z_Realloc(variable->aliases, sizeof(char *) * (variable->aliases_size + 2));
	// Add the new alias, and increment the number of aliases in the list
	variable->aliases[variable->aliases_size++] = Z_strdup(name);

	// link to head of list in this hash table index
	hash = (cvar_hash_t *)Z_Malloc(sizeof(cvar_hash_t));
	hashindex = CRC_Block((const unsigned char *)name, strlen(name)) % CVAR_HASHSIZE;
	hash->next = cvars->hashtable[hashindex];
	cvars->hashtable[hashindex] = hash;
	hash->cvar = variable;
}

/*
============
Cvar_Link

Links a variable to the variable list and hashtable
============
*/
static void Cvar_Link(cvar_t *variable, cvar_state_t *cvars)
{
	cvar_t *current, *next;
	cvar_hash_t *hash;
	unsigned hashindex;
	/*
	 * Link the variable in
	 * alphanumerical order
	 */
	for( current = NULL, next = cvars->vars ; next && strcmp( next->name, variable->name ) < 0 ; current = next, next = next->next )
		;
	if(current)
		current->next = variable;
	else
		cvars->vars = variable;
	variable->next = next;

	// link to head of list in this hash table index
	hash = (cvar_hash_t *)Z_Malloc(sizeof(cvar_hash_t));
	hashindex = CRC_Block((const unsigned char *)variable->name, strlen(variable->name)) % CVAR_HASHSIZE;
	hash->next = cvars->hashtable[hashindex];
	hash->cvar = variable;
	cvars->hashtable[hashindex] = hash;
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	cvar_state_t *cvars = &cvars_all;
	cvar_t *current, *cvar;
	int i;

	if (!(variable->flags & cmd_local->cvars_flagsmask))
	{
		if (developer_extra.integer)
			Con_DPrintf("^2Cvar_RegisterVariable: rejecting cvar \"%s\"\n", variable->name);
		return;
	}

	if (developer_extra.integer)
		Con_DPrintf("Cvar_RegisterVariable({\"%s\", \"%s\", %i});\n", variable->name, variable->string, variable->flags);

	// first check to see if it has already been defined
	cvar = Cvar_FindVar(cvars, variable->name, ~0);
	if (cvar)
	{
		if (cvar->flags & CF_ALLOCATED)
		{
			if (developer_extra.integer)
				Con_DPrintf("...  replacing existing allocated cvar {\"%s\", \"%s\", %i}\n", cvar->name, cvar->string, cvar->flags);
			// fixed variables replace allocated ones
			// (because the engine directly accesses fixed variables)
			// NOTE: this isn't actually used currently
			// (all cvars are registered before config parsing)
			variable->flags &= ~CF_ALLOCATED;
			// cvar->string is now owned by variable instead
			variable->string = cvar->string;
			variable->defstring = cvar->defstring;
			variable->value = atof (variable->string);
			variable->integer = (int) variable->value;
			// Preserve autocvar status.
			memcpy(variable->globaldefindex, cvar->globaldefindex, sizeof(variable->globaldefindex));
			memcpy(variable->globaldefindex_stringno, cvar->globaldefindex_stringno, sizeof(variable->globaldefindex_stringno));
			// replace cvar with this one...
			variable->next = cvar->next;
			if (cvars->vars == cvar)
			{
				// head of the list is easy to change
				cvars->vars = variable;
			}
			else
			{
				// otherwise find it somewhere in the list
				for (current = cvars->vars;current->next != cvar;current = current->next)
					;
				current->next = variable;
			}

			// get rid of old allocated cvar
			// (but not cvar->string and cvar->defstring, because we kept those)
			Z_Free((char *)cvar->name);
			Z_Free(cvar);
		}
		else
			Con_DPrintf("Can't register variable %s, already defined\n", variable->name);
		return;
	}

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, variable->name))
	{
		Con_Printf(CON_WARN "Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}

	// copy the value off, because future sets will Z_Free it
	variable->name = Z_strdup(variable->name);
	variable->string = Z_strdup(variable->string);
	variable->defstring = Z_strdup(variable->string);
	variable->value = atof (variable->string);
	variable->integer = (int) variable->value;
	variable->aliases = NULL;
	variable->aliases_size = 0;
	variable->initstring = NULL;

	// Mark it as not an autocvar.
	for (i = 0;i < PRVM_PROG_MAX;i++)
		variable->globaldefindex[i] = -1;

	// Safe for Cvar_SetQuick()
	variable->flags |= CF_REGISTERED;

	Cvar_Link(variable, cvars);
}

/*
============
Cvar_Get

Adds a newly allocated variable to the variable list or sets its value.
============
*/
cvar_t *Cvar_Get(cvar_state_t *cvars, const char *name, const char *value, unsigned flags, const char *newdescription)
{
	cvar_t *cvar;
	int i;

	if (developer_extra.integer)
		Con_DPrintf("Cvar_Get(\"%s\", \"%s\", %i);\n", name, value, flags);

	// first check to see if it has already been defined
	cvar = Cvar_FindVar(cvars, name, ~0);
	if (cvar)
	{
		cvar->flags |= flags;
		Cvar_SetQuick_Internal (cvar, value);
		if(newdescription && (cvar->flags & CF_ALLOCATED))
		{
			if(cvar->description != cvar_dummy_description)
				Z_Free((char *)cvar->description);

			if(*newdescription)
				cvar->description = Z_strdup(newdescription);
			else
				cvar->description = cvar_dummy_description;
		}
		return cvar;
	}

	// check for pure evil
	if (!*name)
	{
		Con_Printf(CON_WARN "Cvar_Get: invalid variable name\n");
		return NULL;
	}

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, name))
	{
		Con_Printf(CON_WARN "Cvar_Get: %s is a command\n", name);
		return NULL;
	}

	// allocate a new cvar, cvar name, and cvar string
	// TODO: factorize the following code with the one at the end of Cvar_RegisterVariable()
	// FIXME: these never get Z_Free'd
	cvar = (cvar_t *)Z_Malloc(sizeof(cvar_t));
	cvar->flags = flags | CF_ALLOCATED;
	cvar->name = Z_strdup(name);
	cvar->string = Z_strdup(value);
	cvar->defstring = Z_strdup(value);
	cvar->value = atof (cvar->string);
	cvar->integer = (int) cvar->value;
	cvar->aliases = NULL;
	cvar->aliases_size = 0;
	cvar->initstring = NULL;

	if(newdescription && *newdescription)
		cvar->description = Z_strdup(newdescription);
	else
		cvar->description = cvar_dummy_description; // actually checked by VM_cvar_type

	// Mark it as not an autocvar.
	for (i = 0;i < PRVM_PROG_MAX;i++)
		cvar->globaldefindex[i] = -1;

	Cvar_Link(cvar, cvars);

	return cvar;
}

/// For "quiet" mode pass NULL as the callername.
/// Returns true if the cvar was deleted.
static qbool Cvar_Delete(cvar_state_t *cvars, const char *name, const char *callername)
{
	cvar_t *cvar, *prev;
	cvar_hash_t **hashlinkptr, *oldhashlink;
	const char *progname;

	hashlinkptr = Cvar_FindVarLink(cvars, name, &prev, ~0);
	if(!hashlinkptr)
	{
		if (callername)
			Con_Printf("%s: cvar \"%s\" is not defined.\n", callername, name);
		return false;
	}
	cvar = (*hashlinkptr)->cvar;

	if(!(cvar->flags & CF_ALLOCATED))
	{
		if (callername)
			Con_Printf(CON_WARN "%s: engine cvar \"%s\" cannot be deleted!\n", callername, cvar->name);
		return false;
	}
	if ((progname = Cvar_IsAutoCvar(cvar)))
	{
		if (callername)
			Con_Printf(CON_WARN "%s: unable to delete cvar \"%s\", it is an autocvar used by running %s progs!\n", callername, cvar->name, progname);
		return false;
	}

	if(cvar == cvars->vars)
		cvars->vars = cvar->next;
	else
		prev->next = cvar->next;

	if(cvar->description != cvar_dummy_description)
		Z_Free((char *)cvar->description);
	Z_Free((char *)cvar->name);
	Z_Free((char *)cvar->string);
	Z_Free((char *)cvar->defstring);
	Z_Free(cvar);

	oldhashlink = *hashlinkptr;
	*hashlinkptr = (*hashlinkptr)->next;
	Z_Free(oldhashlink);

	return true;
}

qbool Cvar_Readonly (cvar_t *var, const char *cmd_name)
{
	if (var->flags & CF_READONLY)
	{
		Con_Print(CON_WARN);
		if(cmd_name)
			Con_Printf("%s: ",cmd_name);
		Con_Printf("%s is read-only\n", var->name);
		return true;
	}
	return false;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qbool	Cvar_Command (cmd_state_t *cmd)
{
	cvar_state_t	*cvars = cmd->cvars;
	cvar_t			*v;

	// check variables
	v = Cvar_FindVar(cvars, Cmd_Argv(cmd, 0), (cmd->cvars_flagsmask));
	if (!v)
		return false;

	// perform a variable print or set
	if (Cmd_Argc(cmd) == 1)
	{
		Cvar_PrintHelp(v, Cmd_Argv(cmd, 0), true);
		return true;
	}

	if (developer_extra.integer)
		Con_DPrint("Cvar_Command: ");

	if(Cvar_Readonly(v, NULL))
		return true;

	Cvar_SetQuick(v, Cmd_Argv(cmd, 1));
	if (developer_extra.integer)
		Con_DPrint("\n");
	return true;
}

void Cvar_UnlockDefaults(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// unlock the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
		var->flags &= ~CF_DEFAULTSET;
}

void Cvar_LockDefaults_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// lock in the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if (!(var->flags & CF_DEFAULTSET))
		{
			size_t alloclen;

			//Con_Printf("locking cvar %s (%s -> %s)\n", var->name, var->string, var->defstring);
			var->flags |= CF_DEFAULTSET;
			Z_Free((char *)var->defstring);
			alloclen = strlen(var->string) + 1;
			var->defstring = (char *)Z_Malloc(alloclen);
			memcpy((char *)var->defstring, var->string, alloclen);
		}
	}
}

void Cvar_SaveInitState(cvar_state_t *cvars)
{
	cvar_t *c;

	for (c = cvars->vars;c;c = c->next)
		if ((c->flags & (CF_PERSISTENT | CF_READONLY)) == 0)
			c->initstring = Z_strdup(c->string);
}

void Cvar_RestoreInitState(cvar_state_t *cvars)
{
	cvar_t *c, *cp;

	for (cp = cvars->vars; (c = cp);)
	{
		cp = c->next; // get next cvar now in case we delete this cvar

		if (c->initstring)
		{
			// restore this cvar, it existed at init
			Con_DPrintf("Cvar_RestoreInitState: Restoring cvar \"%s\"\n", c->name);

			/* bones_was_here: intentionally NOT restoring defstring in this function.
			 * The only callsite, Host_LoadConfig_f, will re-exec quake.rc, default.cfg, etc.
			 * Defaults are unlocked here, so Cvar_LockDefaults_f will reset the defstring.
			 * This is more correct than restoring the defstring here
			 * because a gamedir change could load configs that should change defaults.
			 */

			Cvar_SetQuick(c, c->initstring);
			c->flags &= ~CF_DEFAULTSET;
		}
		else if (!(c->flags & (CF_PERSISTENT | CF_READONLY))) // cvars with those flags have no initstring AND may not be deleted or reset
		{
			// remove this cvar, it did not exist at init
			Con_DPrintf("Cvar_RestoreInitState: cvar \"%s\"", c->name);
			if (Cvar_Delete(cvars, c->name, NULL))
				Con_DPrint("deleted.\n");
			else
			{
				// In this case, at least reset it to the default.
				Con_DPrint("reset.\n");
				Cvar_SetQuick(c, c->defstring);
			}
		}
	}
}

void Cvar_ResetToDefaults_All_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if((var->flags & (CF_PERSISTENT | CF_READONLY)) == 0)
			Cvar_SetQuick(var, var->defstring);
	}
}

void Cvar_ResetToDefaults_NoSaveOnly_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if ((var->flags & (CF_PERSISTENT | CF_READONLY | CF_ARCHIVE)) == 0)
			Cvar_SetQuick(var, var->defstring);
	}
}


void Cvar_ResetToDefaults_SaveOnly_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if ((var->flags & (CF_PERSISTENT | CF_READONLY | CF_ARCHIVE)) == CF_ARCHIVE)
			Cvar_SetQuick(var, var->defstring);
	}
}

/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (cvar_state_t *cvars, qfile_t *f)
{
	cvar_t	*var;
	char buf1[MAX_INPUTLINE], buf2[MAX_INPUTLINE];

	// don't save cvars that match their default value
	for (var = cvars->vars ; var ; var = var->next) {
		if ((var->flags & CF_ARCHIVE) && (strcmp(var->string, var->defstring) || ((var->flags & CF_ALLOCATED) && !(var->flags & CF_DEFAULTSET))))
		{
			Cmd_QuoteString(buf1, sizeof(buf1), var->name, "\"\\$", false);
			Cmd_QuoteString(buf2, sizeof(buf2), var->string, "\"\\$", false);
			FS_Printf(f, "%s\"%s\" \"%s\"\n", var->flags & CF_ALLOCATED ? "seta " : "", buf1, buf2);
		}
	}
}

// Added by EvilTypeGuy eviltypeguy@qeradiant.com
// 2000-01-09 CvarList command By Matthias "Maddes" Buecher, http://www.inside3d.com/qip/
/*
=========
Cvar_List
=========
*/
void Cvar_List_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;
	const char *partial;
	int count;
	qbool ispattern;
	char vabuf[1024];

	if (Cmd_Argc(cmd) > 1)
	{
		partial = Cmd_Argv(cmd, 1);
		ispattern = (strchr(partial, '*') || strchr(partial, '?'));
		if(!ispattern)
			partial = va(vabuf, sizeof(vabuf), "%s*", partial);
	}
	else
	{
		partial = va(vabuf, sizeof(vabuf), "*");
		ispattern = false;
	}

	count = 0;
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
	{
		if (matchpattern_with_separator(cvar->name, partial, false, "", false))
		{
			Cvar_PrintHelp(cvar, cvar->name, true);
			count++;
		}
		for (char **alias = cvar->aliases; alias && *alias; alias++)
		{
			if (matchpattern_with_separator(*alias, partial, false, "", false))
			{
				Cvar_PrintHelp(cvar, *alias, true);
				count++;
			}
		}
	}

	if (Cmd_Argc(cmd) > 1)
	{
		if(ispattern)
			Con_Printf("%i cvar%s matching \"%s\"\n", count, (count > 1) ? "s" : "", partial);
		else
			Con_Printf("%i cvar%s beginning with \"%s\"\n", count, (count > 1) ? "s" : "", Cmd_Argv(cmd,1));
	}
	else
		Con_Printf("%i cvar(s)\n", count);
}
// 2000-01-09 CvarList command by Maddes

void Cvar_Set_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;

	// make sure it's the right number of parameters
	if (Cmd_Argc(cmd) < 3)
	{
		Con_Printf("Set: wrong number of parameters, usage: set <variablename> <value> [<description>]\n");
		return;
	}

	// check if it's read-only
	cvar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ~0);
	if (cvar)
		if(Cvar_Readonly(cvar,"Set"))
			return;

	if (developer_extra.integer)
		Con_DPrint("Set: ");

	// all looks ok, create/modify the cvar
	Cvar_Get(cvars, Cmd_Argv(cmd, 1), Cmd_Argv(cmd, 2), cmd->cvars_flagsmask, Cmd_Argc(cmd) > 3 ? Cmd_Argv(cmd, 3) : NULL);
}

void Cvar_SetA_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;

	// make sure it's the right number of parameters
	if (Cmd_Argc(cmd) < 3)
	{
		Con_Printf("SetA: wrong number of parameters, usage: seta <variablename> <value> [<description>]\n");
		return;
	}

	// check if it's read-only
	cvar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ~0);
	if (cvar)
		if(Cvar_Readonly(cvar,"SetA"))
			return;

	if (developer_extra.integer)
		Con_DPrint("SetA: ");

	// all looks ok, create/modify the cvar
	Cvar_Get(cvars, Cmd_Argv(cmd, 1), Cmd_Argv(cmd, 2), cmd->cvars_flagsmask | CF_ARCHIVE, Cmd_Argc(cmd) > 3 ? Cmd_Argv(cmd, 3) : NULL);
}

void Cvar_Del_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	int i;

	if(Cmd_Argc(cmd) < 2)
	{
		Con_Printf("%s: wrong number of parameters, usage: unset <variablename1> [<variablename2> ...]\n", Cmd_Argv(cmd, 0));
		return;
	}
	for(i = 1; i < Cmd_Argc(cmd); ++i)
		Cvar_Delete(cvars, Cmd_Argv(cmd, i), Cmd_Argv(cmd, 0));
}

#ifdef FILLALLCVARSWITHRUBBISH
void Cvar_FillAll_f(cmd_state_t *cmd)
{
	char *buf, *p, *q;
	int n, i;
	cvar_t *var;
	qbool verify;
	if(Cmd_Argc(cmd) != 2)
	{
		Con_Printf("Usage: %s length to plant rubbish\n", Cmd_Argv(cmd, 0));
		Con_Printf("Usage: %s -length to verify that the rubbish is still there\n", Cmd_Argv(cmd, 0));
		return;
	}
	n = atoi(Cmd_Argv(cmd, 1));
	verify = (n < 0);
	if(verify)
		n = -n;
	buf = Z_Malloc(n + 1);
	buf[n] = 0;
	for(var = cvars->vars; var; var = var->next)
	{
		for(i = 0, p = buf, q = var->name; i < n; ++i)
		{
			*p++ = *q++;
			if(!*q)
				q = var->name;
		}
		if(verify && strcmp(var->string, buf))
		{
			Con_Printf("\n%s does not contain the right rubbish, either this is the first run or a possible overrun was detected, or something changed it intentionally; it DOES contain: %s\n", var->name, var->string);
		}
		Cvar_SetQuick(var, buf);
	}
	Z_Free(buf);
}
#endif /* FILLALLCVARSWITHRUBBISH */
