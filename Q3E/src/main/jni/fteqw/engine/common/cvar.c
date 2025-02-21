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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"
#include "shader.h"

cvar_group_t *cvar_groups;

//static bucket_t *cvar_buckets[1024];
//static hashtable_t cvar_hash;

typedef struct {
	size_t maxentries;
	size_t numentries;
	struct
	{
		const char *string;
		void *data;
	} entry[1];
} abucket_t;
typedef struct {
	abucket_t **bucket;
	unsigned int numbuckets;
} ahashtable_t;	//not thread-safe

static abucket_t *cvar_buckets[1024];
static ahashtable_t cvar_hash = {cvar_buckets, countof(cvar_buckets)};

unsigned int Hash_KeyInsensitive(const char *name, unsigned int modulus);
void *AHash_GetInsensitive(ahashtable_t *table, const char *name)
{
	abucket_t *b = table->bucket[Hash_KeyInsensitive(name, table->numbuckets)];
	size_t i;
	if (b)
		for (i = 0; i < b->numentries; i++)
		{
			if (!strcasecmp(b->entry[i].string, name))
				return b->entry[i].data;
		}
	return NULL;
}

void AHash_RemoveDataInsensitive(ahashtable_t *table, const char *name, void *data)
{
	abucket_t *b = table->bucket[Hash_KeyInsensitive(name, table->numbuckets)];
	size_t i;
	for (i = 0; i < b->numentries; i++)
	{
		if (b->entry[i].data == data && !strcasecmp(b->entry[i].string, name))
		{
			//strip it.
			b->numentries--;
			//shift everything down.
			if (b->numentries > i)
				memmove(&b->entry[i], &b->entry[i+1], sizeof(*b->entry)*(b->numentries-i));
			break;
		}
	}
}
void AHash_AddInsensitive(ahashtable_t *table, const char *name, void *data)
{
	unsigned int idx = Hash_KeyInsensitive(name, table->numbuckets);
	abucket_t *b = table->bucket[idx];
	if (!b)
	{	//nothing there!...
		b = table->bucket[idx] = BZ_Malloc(sizeof(*b));
		b->numentries = 0;
		b->maxentries = countof(b->entry);
	}
	else if (b->numentries == b->maxentries)
	{	//can't add anything new
		size_t n = b->maxentries*2;
		table->bucket[idx] = BZ_Malloc(sizeof(*b)-sizeof(b->entry) + sizeof(b->entry)*n);
		memcpy(table->bucket[idx]->entry, b->entry, sizeof(b->entry[0])*b->numentries);
		table->bucket[idx]->numentries = b->numentries;
		table->bucket[idx]->maxentries = n;
		BZ_Free(b);
		b = table->bucket[idx];
	}
	b->entry[b->numentries].data = data;
	b->entry[b->numentries].string = name;
	b->numentries++;
}
void AHash_Cleanup(ahashtable_t *table)
{
	size_t i;
	for (i = 0; i < table->numbuckets; i++)
	{
		if (table->bucket[i] && !table->bucket[i]->numentries)
		{
			BZ_Free(table->bucket[i]);
			table->bucket[i] = NULL;
		}
	}
}

int cvar_watched;

//cvar_t	*cvar_vars;
static char	*cvar_null_string = "";
static char *cvar_zero_string = "0";
static char *cvar_one_string = "1";

static char *Cvar_DefaultAlloc(char *str)
{
	char *c;

	if (str[0] == '\0')
		return cvar_null_string;
	if (str[0] == '0' && str[1] == '\0')
		return cvar_zero_string;
	if (str[0] == '1' && str[1] == '\0')
		return cvar_one_string;

	c = (char *)Z_Malloc(strlen(str)+1);
	Q_strcpy(c, str);

	return c;
}

void Cvar_DefaultFree(char *str)
{
	if (str == cvar_null_string)
		return;
	else if (str == cvar_zero_string)
		return;
	else if (str == cvar_one_string)
		return;
	else
		Z_Free(str);
}

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (const char *var_name)
{
	return AHash_GetInsensitive(&cvar_hash, var_name);
/*
	cvar_group_t	*grp;
	cvar_t	*var;

	for (grp=cvar_groups ; grp ; grp=grp->next)
		for (var=grp->cvars ; var ; var=var->next)
		{
			if (!Q_strcasecmp (var_name, var->name))
				return var;
			if (var->name2 && !Q_strcasecmp (var_name, var->name2))
				return var;
		}
*/
	return NULL;
}

static cvar_group_t *Cvar_FindGroup (const char *group_name)
{
	cvar_group_t	*grp;

	for (grp=cvar_groups ; grp ; grp=grp->next)
		if (!Q_strcasecmp (group_name, grp->name))
			return grp;

	return NULL;
}
static cvar_group_t *Cvar_GetGroup(const char *gname)
{
	cvar_group_t *g;
	if (!gname)
		gname = "Miscilaneous vars";
	g = Cvar_FindGroup(gname);
	if (g)
		return g;

	g = (cvar_group_t*)Z_Malloc(sizeof(cvar_group_t) + strlen(gname)+1);
	g->name = (char*)(g+1);
	strcpy((char*)g->name, gname);
	g->next = NULL;
	g->next = cvar_groups;
	cvar_groups = g;
	return g;
}

// converts a given single cvar flag into a human readable string
static char *Cvar_FlagToName(int flag)
{
	switch (flag)
	{
	case CVAR_ARCHIVE:
		return "archive";
	case CVAR_USERINFO:
		return "userinfo";
	case CVAR_SERVERINFO:
		return "serverinfo";
	case CVAR_NOSET:
		return "noset";
	case CVAR_MAPLATCH:
		return "latch";
	case CVAR_POINTER:
		return "pointer";
	case CVAR_NOTFROMSERVER:
		return "noserver";
	case CVAR_USERCREATED:
		return "userset";
	case CVAR_CHEAT:
		return "cheat";
	case CVAR_SEMICHEAT:
		return "semicheat";
	case CVAR_RENDERERLATCH:
		return "renderlatch";
	case CVAR_VIDEOLATCH:
		return "videolatch";
	case CVAR_SERVEROVERRIDE:
		return "serverlatch";
	case CVAR_RENDERERCALLBACK:
		return "rendercallback";
	case CVAR_NOUNSAFEEXPAND:
		return "nounsafeexpand";
	case CVAR_RULESETLATCH:
		return "rulesetlatch";
	case CVAR_SHADERSYSTEM:
		return "shadersystem";
	case CVAR_NOSAVE:
		return "nosave";
	case CVAR_TELLGAMECODE:
		return "autocvar";
	case CVAR_CONFIGDEFAULT:
		return "";
	case CVAR_NORESET:
		return "noreset";
	case CVAR_RENDEREROVERRIDE:
		return "rendereroverride";
	}

	return NULL;
}

//lists commands, also prints restriction level
#define CLF_RAW 0x1
#define CLF_LEVEL 0x2
#define CLF_ALTNAME 0x4
#define CLF_VALUES 0x8
#define CLF_DEFAULT 0x10
#define CLF_LATCHES 0x20
#define CLF_FLAGS 0x40
#define CLF_FLAGMASK 0x80
#define CLF_CHANGEDONLY 0x100
void Cvar_List_f (void)
{
	cvar_group_t	*grp;
	cvar_t	*cmd;
	char *var, *search, *gsearch;
	int gnum, i, num = 0;
	int listflags = 0, cvarflags = 0;
	int total = 0;
	char strtmp[512];
	char *col;
	static char *cvarlist_help =
"cvarlist list all cvars matching given parameters\n"
"Syntax: cvarlist [-FLdhlrv] [-f flag] [-g group] [cvar]\n"
"  -c includes only the cvars that have been changed from their defaults\n"
"  -F shows cvar flags\n"
"  -L shows latched values\n"
"  -a shows cvar alternate names\n"
"  -d shows default cvar values\n"
"  -f shows only cvars with a matching flag, more than one -f can be used\n"
"  -g shows only cvar groups using wildcards in group\n"
"  -h shows this help message\n"
"  -l shows cvar restriction levels\n"
"  -r removes group and list headers\n"
"  -v shows current values\n"
"  cvar indicates the cvar to show, wildcards (*,?) accepted\n"
"Cvar flags are:"
;

	if (Cmd_Argc() == 1)
		goto showhelp;

	gsearch = search = NULL;
	for (i = 1; i < Cmd_Argc(); i++)
	{
		var = Cmd_Argv(i);
		if (*var == '-')
		{
			// short options
			for (var++; *var; var++)
			{
				switch (*var)
				{
				case 'g':
					// fix this so we can search for multiple groups
					i++;
					if (i >= Cmd_Argc())
					{
						Con_Printf("Missing parameter for -g\nUse cvarlist -h for help\n");
						return;
					}

					gsearch = Cmd_Argv(i);
					break;
				case 'c':
					listflags |= CLF_CHANGEDONLY;
					break;
				case 'a':
					listflags |= CLF_ALTNAME;
					break;
				case 'l':
					listflags |= CLF_LEVEL;
					break;
				case 'r':
					listflags |= CLF_RAW;
					break;
				case 'v':
					listflags |= CLF_VALUES;
					break;
				case 'd':
					listflags |= CLF_DEFAULT;
					break;
				case 'L':
					listflags |= CLF_LATCHES;
					break;
				case 'f':
				{
					char *tmpv;

					listflags |= CLF_FLAGMASK;

					i++;
					if (i >= Cmd_Argc())
					{
						Con_Printf("Missing parameter for -f\nUse cvarlist -h for help\n");
						return;
					}

					tmpv = Cmd_Argv(i);

					for (num = 1; num <= CVAR_LASTFLAG; num <<= 1)
					{
						char *tmp;

						tmp = Cvar_FlagToName(num);

						if (tmp && !stricmp(tmp, tmpv))
						{
							cvarflags |= num;
							break;
						}
					}

					if (num > CVAR_LASTFLAG)
					{
						Con_Printf("Invalid cvar flag name\nUse cvarlist -h for help\n");
						return;
					}
				}
					break;
				case 'F':
					listflags |= CLF_FLAGS;
					break;
				case 'h':
showhelp:
					// list options
					Con_Printf("%s", cvarlist_help);

					for (num = 1; num <= CVAR_LASTFLAG; num <<= 1)
					{
						// no point caring about the content of var at this point
						var = Cvar_FlagToName(num);

						if (var)
							Con_Printf(" %s", var);
					}

					Con_Printf("\n\n");
					return;
				case '-':
					break;
				default:
					Con_Printf("Invalid option for cvarlist\nUse cvarlist -h for help\n");
					return;
				}
			}
		}
		else
			search = var;
	}

	// this is sane.. hopefully
	if (gsearch)
		Q_strlwr(gsearch);

	if (search)
		Q_strlwr(search);

	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		// list only cvars with group search substring
		if (gsearch)
		{
			Q_strncpyz(strtmp, grp->name, 512);
			Q_strlwr(strtmp);
			if (!wildcmp(gsearch, strtmp))
				continue;
		}

		gnum = 0;
		for (cmd=grp->cvars ; cmd ; cmd=cmd->next)
		{
			// list only non-restricted cvars
			if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
				continue;

			// list only cvars with search substring
			if (search)
			{
				Q_strncpyz(strtmp, cmd->name, 512);
				Q_strlwr(strtmp);

				if (!wildcmp(search, strtmp))
				{
					if (cmd->name2)
					{
						Q_strncpyz(strtmp, cmd->name2, 512);
						Q_strlwr(strtmp);
						if (!wildcmp(search, strtmp))
							continue;
					}
					else
						continue;
				}
			}

			// list only cvars with matching flags
			if ((listflags & CLF_FLAGMASK) && !(cmd->flags & cvarflags))
				continue;

			if ((listflags & CLF_CHANGEDONLY) && ((cmd->flags & (CVAR_NOSET|CVAR_NOSAVE)) || (cmd->defaultstr && !strcmp(cmd->string, cmd->defaultstr))))
				continue;

			// print cvar list header
			if (!(listflags & CLF_RAW) && !num)
				Con_TPrintf("^aCVar list:\n");

			// print group header
			if (!(listflags & CLF_RAW) && !gnum)
				Con_Printf("^a%s --\n", grp->name);

			// print restriction level
			if (listflags & CLF_LEVEL)
				Con_Printf("(%i) ", cmd->restriction);

			// print cvar name
			if (!cmd->defaultstr || !strcmp(cmd->string, cmd->defaultstr))
				col = S_COLOR_GREEN;	//cvar has default value, woo.
			else if (cmd->flags & CVAR_ARCHIVE)
				col = S_COLOR_RED;		//cvar will persist. oh noes.
			else
				col = S_COLOR_YELLOW;	//cvar is changed, but won't be saved to a config so w/e.
			if (cmd->flags & CVAR_NOUNSAFEEXPAND)
				Con_Printf("^[%s%s\\type\\%s\\tip\\<Current value is hidden>\n\n"S_COLOR_YELLOW"%s^]", col, cmd->name, cmd->name, cmd->description?cmd->description:"");	//cvar is changed, but won't be saved to a config so w/e.
			else
				Con_Printf("^[%s%s\\type\\%s %s\\tip\\Default: %s\nCurrent: %s\n\n"S_COLOR_YELLOW"%s^]", col, cmd->name, cmd->name,cmd->string, cmd->defaultstr,cmd->string, cmd->description?cmd->description:"");	//cvar is changed, but won't be saved to a config so w/e.
			total++;

			// print current value
			if (cmd->flags & CVAR_NOUNSAFEEXPAND)
				;
			else
			{
				if (listflags & CLF_VALUES)
				{
					if (*cmd->string)
						Con_Printf(" %s", cmd->string);
				}

				// print default value
				if (cmd->defaultstr && (listflags & CLF_DEFAULT))
					Con_Printf(", default \"%s\"", cmd->defaultstr);
			}

			// print alternate name
			if ((listflags & CLF_ALTNAME) && cmd->name2)
				Con_Printf(", alternate %s", cmd->name2);

			// print cvar flags
			if (listflags & CLF_FLAGS)
			{
				for (i = 1; i <= CVAR_LASTFLAG; i <<= 1)
				{
					if (i & cmd->flags)
					{
						var = Cvar_FlagToName(i);
						if (var)
							Con_Printf(" %s", var);
					}
				}
			}

			// print latched value
			if (listflags & CLF_LATCHES)
			{
				if (cmd->flags & CVAR_NOUNSAFEEXPAND)
					;
				else if (cmd->latched_string)
					Con_Printf(", latched as \"%s\"", cmd->latched_string);
			}

			// print new line to finish individual cvar
			Con_Printf("\n");

			num++;
			gnum++;
		}

		// print new line to seperate groups
		if (!(listflags & CLF_RAW) && gnum)
			Con_Printf("\n");
	}

	if (search && !strcmp(search, "*"))
		Con_Printf("%i cvars\n", total);
}

//default values are meant to be constants.
//sometimes that just doesn't make sense.
//so provide a safe way to change it (MUST be initialised to NULL so that it doesn't get freed)
void Cvar_SetEngineDefault(cvar_t *var, char *val)
{
	qboolean wasdefault = (var->defaultstr == var->enginevalue);
	Z_Free(var->enginevalue);
	if (val)
		var->enginevalue = Z_StrDup(val);
	else
		var->enginevalue = NULL;

	if (wasdefault)
		var->defaultstr = var->enginevalue;
}
void Cvar_LockDefaults_f(void)
{
	cvar_group_t *grp;
	cvar_t *cmd;
	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		for (cmd=grp->cvars ; cmd ; cmd=cmd->next)
		{
			if (cmd->flags & (CVAR_NOSET | CVAR_CHEAT))
				continue;

			if (strcmp(cmd->string, cmd->defaultstr))
			{
				if (cmd->defaultstr != cmd->enginevalue)
					Cvar_DefaultFree(cmd->defaultstr);
				cmd->defaultstr = Cvar_DefaultAlloc(cmd->string);
			}
		}
	}
}
void Cvar_PurgeDefaults_f(void)
{
	cvar_group_t *grp;
	cvar_t *cmd;
	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		for (cmd=grp->cvars ; cmd ; cmd=cmd->next)
		{
			if (!cmd->enginevalue)
				continue;	//can't reset the cvar's default if its an engine cvar.
			if (cmd->flags & CVAR_NOSET)
				continue;

			if (cmd->defaultstr != cmd->enginevalue)
			{
				Cvar_DefaultFree(cmd->defaultstr);
				cmd->defaultstr = cmd->enginevalue;
			}
		}
	}
}

//we're changing games. reset everything to engine defaults and kill all user cvars
static void Cvar_Free(cvar_t *tbf);
void Cvar_GamedirChange(void)
{
	cvar_t	*var, **link;
	cvar_group_t *grp;
	for (grp = cvar_groups; grp; grp = grp->next)
	{
		for (link = &grp->cvars; (var=*link); )
		{
			if (var->flags & CVAR_NORESET)
				;	//don't reset it (nor kill it).
			else if (var->enginevalue)
				Cvar_ForceSet(var, var->enginevalue);
			else if (var->flags & CVAR_POINTER)
			{
				*link = var->next;
				Z_Free(var->string);
				if (var->defaultstr != var->enginevalue)
					Cvar_DefaultFree(var->defaultstr);
				if (var->latched_string)
					Z_Free(var->latched_string);
				if (var->name)
					AHash_RemoveDataInsensitive(&cvar_hash, var->name, var);
				if (var->name2)
					AHash_RemoveDataInsensitive(&cvar_hash, var->name2, var);
				Z_Free(var);
				continue;
			}
			link = &(*link)->next;
		}
	}
}

void Cvar_ResetAll_f(void)
{
	cvar_group_t *grp;
	cvar_t *var;
	unsigned int bitmask;		//the bits to care about
	unsigned int bitmaskvalue;	//must match this value
	char *cmd = Cmd_Argv(0);
	if (!Q_strcasecmp(cmd, "cvar_resettodefaults_saveonly"))
		bitmask = CVAR_NORESET|CVAR_NOSET|CVAR_SAVE, bitmaskvalue = CVAR_SAVE;
	else if (!Q_strcasecmp(cmd, "cvar_resettodefaults_nosaveonly"))
		bitmask = CVAR_NORESET|CVAR_NOSET|CVAR_SAVE, bitmaskvalue = 0;
	else	//others...
		bitmask = CVAR_NORESET|CVAR_NOSET, bitmaskvalue = 0;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		for (var=grp->cvars ; var ; var=var->next)
		{
			if (!var->enginevalue)
				continue;	//can't reset the cvar's default if its an engine cvar.
			if ((var->flags & bitmask) != bitmaskvalue)
				continue;

			if (var->defaultstr != var->enginevalue)
			{
				Cvar_DefaultFree(var->defaultstr);
				var->defaultstr = var->enginevalue;
			}
		}
	}
}

#define CRF_ALTNAME 0x1
void Cvar_Reset_f (void)
{
	cvar_group_t *grp;
	cvar_t *cmd;
	int i, listflags=0, exclflags;
	char *var;
	char *search, *gsearch;
	char strtmp[512];
	char *resetval;
	char *pendingval;

	search = gsearch = NULL;
	exclflags = 0;

	// parse command line options
	for (i = 1; i < Cmd_Argc(); i++)
	{
		var = Cmd_Argv(i);
		if (*var == '-')
		{
			// short options
			for (var++; *var; var++)
			{
				switch (*var)
				{
				case 'a':
					listflags |= CRF_ALTNAME;
					break;
				case 'g':
					// fix this so we can search for multiple groups
					i++;
					if (i >= Cmd_Argc())
					{
						Con_Printf("Missing parameter for -g\nUse cvarlist -h for help\n");
						return;
					}

					gsearch = Cmd_Argv(i);
					break;
				case 'u':
					exclflags |= CVAR_USERCREATED;
				case 'h':
					Con_Printf("cvarreset resets all cvars to default values matching given parameters\n"
						"Syntax: cvarreset [-ahu] (-g group)/cvar\n"
						"  -a matches cvar against alternate cvar names\n"
						"  -g matches using wildcards in group\n"
						"  -h shows this help message\n"
						"  -u excludes user cvars\n"
						"  cvar indicates the cvars to reset, wildcards (*, ?) accepted\n"
						"A -g or cvar is required\n");
					return;
				default:
					Con_Printf("Invalid option for cvarreset\nUse cvarreset -h for help\n");
					return;
				}
			}
		}
		else
			search = var;
	}

	if (!search && !gsearch)
	{
		Con_Printf("No group or cvars given\nUse cvarreset -h for help\n");
		return;
	}

	// this should be sane.. hopefully
	if (search)
		Q_strlwr(search);
	if (gsearch)
		Q_strlwr(gsearch);

	if (search && !strcmp(search, "*"))
		search = NULL;
	if (gsearch && !strcmp(gsearch, "*"))
		gsearch = NULL;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		if (gsearch)
		{
			Q_strncpyz(strtmp, grp->name, 512);
			Q_strlwr(strtmp);
			if (!wildcmp(gsearch, strtmp))
				continue;
		}

		for (cmd=grp->cvars ; cmd ; cmd=cmd->next)
		{
			// reset only non-restricted cvars
			if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
				continue;

			// don't reset cvars with matched flags
			if (exclflags & cmd->flags)
				continue;

			// reset only cvars with search substring
			if (search)
			{
				Q_strncpyz(strtmp, cmd->name, 512);
				Q_strlwr(strtmp);

				if (!wildcmp(search, strtmp))
				{
					if ((listflags & CRF_ALTNAME) && cmd->name2)
					{
						Q_strncpyz(strtmp, cmd->name2, 512);
						Q_strlwr(strtmp);
						if (!wildcmp(search, strtmp))
							continue;
					}
					else
						continue;
				}
			}

			if ((cmd->flags & CVAR_NOSET) && !search)
				continue;
			if (cmd->flags & CVAR_NORESET)
				continue;

			// reset cvar to default only if its okay to do so
			if (cmd->defaultstr)
				resetval = cmd->defaultstr;
			else if (cmd->enginevalue)
				resetval = cmd->enginevalue;
			else
				continue;	//no idea what to reset it to.
			pendingval = cmd->string;
			if (cmd->latched_string)
				pendingval = cmd->latched_string;
			if (strcmp(resetval, pendingval))
				Cvar_Set(cmd, resetval);
		}
	}
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}

void Cvar_SetNamed (const char *var_name, const char *newvalue)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return;
	Cvar_Set(var, newvalue);
}

/*
============
Cvar_CompleteVariable
============
*/
/* moved to cmd_compleatevariable
char *Cvar_CompleteVariable (char *partial)
{
	cvar_group_t	*grp;
	cvar_t		*cvar;
	int			len;

	len = Q_strlen(partial);

	if (!len)
		return NULL;

	// check exact match
	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (cvar=grp->cvars ; cvar ; cvar=cvar->next)
		if (!strcmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (cvar=grp->cvars ; cvar ; cvar=cvar->next)
		if (!Q_strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}
*/

static qboolean cvar_archivedvaluechanged;
qboolean Cvar_UnsavedArchive(void)
{
	return cvar_archivedvaluechanged;
}
void Cvar_ConfigChanged(void)
{
	cvar_archivedvaluechanged = true;
}
void Cvar_Saved(void)
{
	cvar_archivedvaluechanged = false;
}

/*
============
Cvar_Set
============
*/
static cvar_t *Cvar_SetCore (cvar_t *var, const char *value, qboolean force)
{	//fixme: force should probably be a latch bitmask
	char *latch=NULL;
	qboolean changed;

	COM_AssertMainThread("Cvar_SetCore");

	if (!var)
		return NULL;

	if ((var->flags & CVAR_WATCHED) || cvar_watched == 2)
		Con_Printf("Cvar Set: %s to %s\n", var->name, value);

	if ((var->flags & CVAR_NOSET) && !force)
	{
		Con_Printf ("variable %s is readonly\n", var->name);
		return NULL;
	}

	if (!value)
		value = var->defaultstr;

	if (force)
		;
	else if (0)//var->flags & CVAR_SERVEROVERRIDE && !force)
		latch = "variable %s is under server control - latched\n";
	else if (var->flags & CVAR_MAPLATCH && (sv_state || cls_state))
		latch = "variable %s is latched and will be applied for the start of the next map\n";
//	else if (var->flags & CVAR_LATCHFLUSH)
//		latch = "variable %s is latched (type flush)\n";
#ifdef HAVE_CLIENT
	else if (var->flags & CVAR_VIDEOLATCH && qrenderer != QR_NONE)
		latch = "variable %s will be changed after a vid_restart\n";
	else if (var->flags & CVAR_RENDERERLATCH && qrenderer != QR_NONE)
		latch = "variable %s will be changed after a vid_reload\n";
	else if (var->flags & CVAR_RENDEREROVERRIDE && qrenderer != QR_NONE)
		latch = "variable %s is not supported by the current renderer/gpu/drivers\n";
#endif
	else if (var->flags & CVAR_RULESETLATCH)
		latch = "variable %s is latched due to current ruleset\n";
#ifdef HAVE_CLIENT
	else if (var->flags & CVAR_CHEAT && !cls.allow_cheats && cls.state)
		latch = "variable %s is a cheat variable - latched\n";
	else if (var->flags & CVAR_SEMICHEAT && !cls.allow_semicheats && cls.state)
		latch = "variable %s is a cheat variable - latched\n";
#endif

	if ((var->flags & CVAR_WARNONCHANGE) && cl_warncmd.ival)
	{
		if (var->defaultstr && strcmp(var->defaultstr, value))	//warn if its different from the default
			if (!var->enginevalue || strcmp(var->enginevalue, value))	//unless it matches the ENGINE's default, in which case its probably still okay.
				Con_Printf (CON_WARNING"WARNING: %s has been set to \"%s\". This is NOT recommended!\n", var->name, value);
	}

	if (latch)
	{
		if (cl_warncmd.value)
		{	//FIXME: flag that there's a latched cvar instead of spamming prints.
			//FIXME: apply pending rendererlatches vith a vid_reload when leaving the console/menu.
			if (var->latched_string)
			{	//already latched
				if (strcmp(var->latched_string, value))
					Con_Printf (latch, var->name);
			}
			else
			{	//new latch
				if (strcmp(var->string, value))
					Con_Printf (latch, var->name);
			}
		}

		if (var->latched_string && !strcmp(var->latched_string, value))	//no point, this would force the same
			return NULL;
		Cvar_ConfigChanged();
		if (var->latched_string)
			Z_Free(var->latched_string);
		if (!strcmp(var->string, value))	//latch to the original value? remove the latch.
		{
			var->latched_string = NULL;
			return NULL;
		}
		var->latched_string = (char*)Z_Malloc(strlen(value)+1);
		strcpy(var->latched_string, value);
		return NULL;
	}

#ifdef HAVE_SERVER
	if (var->flags & CVAR_SERVERINFO)
	{
		InfoBuf_SetKey (&svs.info, var->name, value);
	}
#endif
#ifdef HAVE_CLIENT
	if (var->flags & CVAR_SHADERSYSTEM)
	{
		if (var->string && value)
			if (strcmp(var->string, value))
				Shader_NeedReload(false);
	}
	if (var->flags & CVAR_USERINFO)
	{
		char *old = InfoBuf_ValueForKey(&cls.userinfo[0], var->name);
		const char *corruptval = TP_ParseFunChars(value);
		if (strcmp(old, corruptval))	//only spam the server if it actually changed
		{				//this helps with config execs
			InfoBuf_SetKey (&cls.userinfo[0], var->name, corruptval);
		}
	}
#endif

	latch = var->string;//save off the old value (so cvar_set(var, var->string) works)

	changed = (!latch) || strcmp(latch, value);

	var->flags &= ~CVAR_TEAMPLAYTAINT;
	if (changed)
	{
		var->string = Z_StrDup (value);
		var->value = Q_atof (var->string);
		var->ival = Q_atoi (var->string);

		{
			char *str = COM_Parse(var->string);
			var->vec4[0] = atof(com_token);
			str = COM_Parse(str);
			var->vec4[1] = atof(com_token);
			str = COM_Parse(str);
			var->vec4[2] = atof(com_token);
			if (!str || !*str)
				var->vec4[3] = 1;
			else
			{
				str = COM_Parse(str);
				var->vec4[3] = atof(com_token);
			}
		}

		if (latch)
		{	//don't do this on registration.
			var->modified=true;	//only modified if it changed.
			var->modifiedcount++;
			if (var->callback)
				var->callback(var, latch);

			if (var->flags & CVAR_TELLGAMECODE)
			{
#ifdef HAVE_SERVER
				SVQ1_CvarChanged(var);
#endif
#ifdef HAVE_CLIENT
#ifdef MENU_DAT
				MP_CvarChanged(var);
#endif
#ifdef CSQC_DAT
				CSQC_CvarChanged(var);
#endif
#endif
			}
		}

		if ((var->flags & CVAR_ARCHIVE) && !(var->flags & CVAR_SERVEROVERRIDE) && cl_warncmd.ival)
		{
			if (var->latched_string)
			{
				if (strcmp(var->latched_string, value))
					Cvar_ConfigChanged();
			}
			else
			{
				if (!latch || strcmp(latch, value))
					Cvar_ConfigChanged();
			}
		}

		Z_Free (latch);	// free the old value string

#ifdef HAVE_SERVER
		MSV_SendCvarChange(var);
#endif
	}

	if (var->latched_string)	//we may as well have this here.
	{
		Z_Free(var->latched_string);
		var->latched_string = NULL;
	}

	return var;
}

qboolean Cvar_ApplyLatchFlag(cvar_t *var, char *value, unsigned int newflag, unsigned int ignoreflags)
{
	qboolean result = true;

	char *latch;
	var->flags &= ~newflag;
	latch = var->latched_string;
	var->latched_string = NULL;
	if (!latch)
	{
#ifdef warningmsg
#pragma warningmsg("this means the callback will never be called")
#endif
		latch = Z_StrDup(var->string);
//		var->string = NULL;
	}

	ignoreflags = var->flags & ignoreflags;	//figure out which latching flags we're ignoring
	var->flags -= ignoreflags;	//and clear them
	Cvar_ForceSet(var, value);
	var->flags |= ignoreflags;	//give them back.

	if (var->latched_string)
	{	//something else latched it
		Z_Free(var->latched_string);
		var->latched_string = NULL;
		result = false;
	}
	else
		var->flags |= newflag;

	if (latch)
	{
		if (!strcmp(var->string, latch))
			Z_Free(latch);	//don't allow a latch to be the same as the current value
		else
			var->latched_string = latch;
	}

	return result;
}

#ifdef HAVE_CLIENT
void Cvar_ForceCheatVars(qboolean semicheats, qboolean absolutecheats)
{	//this either unlatches if the cheat type is allowed, or enforces a default for full cheats and blank for semicheats.
	//this is clientside only.
	//if a value is enforced, it is latched to the old value.
	cvar_group_t	*grp;
	cvar_t	*var;

	char *latch;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (var=grp->cvars ; var ; var=var->next)
	{
		if (!(var->flags & (CVAR_CHEAT|CVAR_SEMICHEAT)))
			continue;
		if (var->flags & CVAR_SERVEROVERRIDE)	//server has control over it. don't force it away.
			continue;

		latch = var->latched_string;
		var->latched_string = NULL;
		if (!latch)
			latch = Z_StrDup(var->string);

		if (var->flags & CVAR_CHEAT)
		{
			if (!absolutecheats)
			{
				if (var->enginevalue)
					Cvar_ForceSet(var, var->enginevalue);
				else
					Cvar_ForceSet(var, var->defaultstr);
			}
			else
				Cvar_ForceSet(var, latch);
		}
		else if (var->flags & CVAR_SEMICHEAT)
		{
			if (var->flags & CVAR_RULESETLATCH)
				;	//this is too problematic. the ruleset should cover it.
			else if (!semicheats)
				Cvar_ForceSet(var, "");
			else
				Cvar_ForceSet(var, latch);
		}

		if (latch)
		{
			if (!strcmp(var->string, latch))
				Z_Free(latch);
			else
				var->latched_string = latch;
		}
	}

	Validation_Apply_Ruleset();
}
#endif

int Cvar_ApplyLatches(int latchflag, qboolean clearflag)
{
	int result = 0;
	cvar_group_t	*grp;
	cvar_t	*var;
	int mask = ~0;
	int of;

	if (clearflag)	//these flags are cleared from cvars.
		mask = ~latchflag;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (var=grp->cvars ; var ; var=var->next)
	{
		if (var->flags & latchflag)
		{
			if (var->latched_string)
			{
				of = var->flags;
				var->flags &= ~latchflag;
				Cvar_Set(var, var->latched_string);
				var->flags = of;

				result++;
			}
			var->flags &= mask;
		}
	}

	return result;
}

cvar_t *Cvar_Set (cvar_t *var, const char *value)
{
	return Cvar_SetCore(var, value, false);
}
cvar_t *Cvar_ForceSet (cvar_t *var, const char *value)
{
	return Cvar_SetCore(var, value, true);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (cvar_t *var, float value)
{
	char	val[32];
	char *e, *p;

//	if (value == (int)value)
//		sprintf (val, "%i",(int)value);	//make it look nicer.
//	else
		sprintf (val, "%f",value);
	p = strchr(val, '.');
	if (p)
	{
		e = p + strlen(p);
		while (e > p && e[-1] == '0')
			e--;
		if (e[-1] == '.')
			e--;
		*e = 0;
	}
	Cvar_Set (var, val);
}
void Cvar_ForceSetValue (cvar_t *var, float value)
{
	char	val[32];

//	if (value == (int)value)
//		sprintf (val, "%i",(int)value);	//make it look nicer.
//	else
		sprintf (val, "%g",value);
	Cvar_ForceSet (var, val);
}

static void Cvar_Free(cvar_t *tbf)
{
	cvar_t *var;
	cvar_group_t *grp;
	if (!(tbf->flags & CVAR_POINTER))
		return;	//only freeable if it was a pointer to begin with.

	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		if (grp->cvars == tbf)
		{
			grp->cvars = tbf->next;
			goto unlinked;
		}
		if (grp->cvars)
		for (var=grp->cvars ; var->next ; var=var->next)
		{
			if (var->next == tbf)
			{
				var->next = tbf->next;
				goto unlinked;
			}
		}
	}
unlinked:
	Z_Free(tbf->string);
	if (tbf->defaultstr != tbf->enginevalue)
		Cvar_DefaultFree(tbf->defaultstr);
	if (tbf->latched_string)
		Z_Free(tbf->latched_string);
	AHash_RemoveDataInsensitive(&cvar_hash, tbf->name, tbf);
	if (tbf->name2)
		AHash_RemoveDataInsensitive(&cvar_hash, tbf->name2, tbf);
	Z_Free(tbf);
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/

qboolean Cvar_Register (cvar_t *variable, const char *groupname)
{
	cvar_t *old;
	cvar_group_t *group;
	char *initial;

	if (variable->defaultstr)
		initial = variable->defaultstr;
	else if (variable->enginevalue)
		initial = variable->enginevalue;
	else if (variable->string)
		initial = variable->string;
	else
		initial = "";

// check to see if it has already been defined
	old = Cvar_FindVar (variable->name);
	if (!old && variable->name2)
		old = Cvar_FindVar (variable->name2);
	if (old)
	{
		if ((old->flags & CVAR_POINTER) && !(variable->flags & CVAR_POINTER))
		{
			group = Cvar_GetGroup(groupname);

			variable->modified = old->modified;
			variable->modifiedcount = old->modifiedcount;
			variable->flags |= (old->flags & CVAR_ARCHIVE);

// link the variable in
			variable->next = group->cvars;
			variable->restriction = old->restriction;	//exe registered vars
			group->cvars = variable;

// make sure it can be zfreed
			variable->string = (char*)Z_Malloc (1);

//cheat prevention - engine set default is the one that stays.
			if (initial != variable->enginevalue)
				variable->defaultstr = Cvar_DefaultAlloc(initial);
			else
				variable->defaultstr = initial;

// set it through the function to be consistant
			if (old->latched_string)
				Cvar_SetCore (variable, old->latched_string, true);
			else
				Cvar_SetCore (variable, old->string, true);

			Cvar_Free(old);

			AHash_AddInsensitive(&cvar_hash, variable->name, variable);
			if (variable->name2)
				AHash_AddInsensitive(&cvar_hash, variable->name2, variable);

			return true;
		}

		Con_Printf ("Can't register variable %s, already defined\n", variable->name);
		return false;
	}

// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return false;
	}
	if (variable->name2 && (Cmd_Exists (variable->name2) || Cvar_FindVar (variable->name2)))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command/exists\n", variable->name2);
		variable->name2 = NULL;
	}

	group = Cvar_GetGroup(groupname);

// link the variable in
	variable->next = group->cvars;
	variable->restriction = 0;	//exe registered vars
	group->cvars = variable;

	AHash_AddInsensitive(&cvar_hash, variable->name, variable);
	if (variable->name2)
		AHash_AddInsensitive(&cvar_hash, variable->name2, variable);

	variable->string = NULL;

	if (initial != variable->enginevalue)
		variable->defaultstr = Cvar_DefaultAlloc(initial);
	else
		variable->defaultstr = initial;

// set it through the function to be consistant
	Cvar_SetCore (variable, initial, true);

	return true;
}

cvar_t *Cvar_Get2(const char *name, const char *defaultvalue, int flags, const char *description, const char *group)
{
	cvar_t *var;
	var = Cvar_FindVar(name);

	if (var)
	{
#ifdef HAVE_CLIENT
		if ((flags & CVAR_USERINFO) && !(var->flags & CVAR_USERINFO))
		{
			var->flags |= CVAR_USERINFO;
			InfoBuf_SetKey(&cls.userinfo[0], var->name, var->string);
		}
#endif
#ifdef HAVE_SERVER
		if ((flags & CVAR_SERVERINFO) && !(var->flags & CVAR_SERVERINFO))
		{
			var->flags |= CVAR_SERVERINFO;
			InfoBuf_SetKey (&svs.info, var->name, var->string);
		}
#endif
		return var;
	}
	if (!description || !*description)
		description = NULL;

	//don't allow cvars with certain funny chars in their name. ever. such things get really messy when saved in configs or whatever.
	if (!*name || strchr(name, '\"') || strchr(name, '^') || strchr(name, '$') || strchr(name, ' ') || strchr(name, '\t') || strchr(name, '\r') || strchr(name, '\n') || strchr(name, ';'))
		return NULL;

	var = (cvar_t*)Z_Malloc(sizeof(cvar_t)+strlen(name)+1+(description?(strlen(description)+1):0));
	var->name = (char *)(var+1);
	strcpy(var->name, name);
	var->string = (char*)defaultvalue;
	var->flags = flags|CVAR_POINTER|CVAR_USERCREATED;
	var->modifiedcount = 1;	//this counter always starts at 1, for q3 compat
	if (description)
	{
		char *desc = var->name+strlen(var->name)+1;
		strcpy(desc, description);
		var->description = desc;
	}

	if (!Cvar_Register(var, group))
	{
		Z_Free(var);
		return NULL;
	}

	return var;
}

//prevent the client from altering the cvar until they change map or the server resets the var to the default.
void Cvar_LockFromServer(cvar_t *var, const char *str)
{
	char *oldlatch;

	if (!(var->flags & CVAR_SERVEROVERRIDE))
	{
		Con_DPrintf("Server taking control of cvar %s (%s)\n", var->name, str);
		var->flags |= CVAR_SERVEROVERRIDE;
	}
	var->flags &= ~CVAR_RENDEREROVERRIDE;

	oldlatch = var->latched_string;
	if (oldlatch)	//maintaining control
		var->latched_string = NULL;
	else	//taking control
	{
		oldlatch = (char*)Z_Malloc(strlen(var->string)+1);
		strcpy(oldlatch, var->string);
	}

	Cvar_SetCore (var, str, true);	//will use all, quote included

	var->latched_string = oldlatch;	//keep track of the original value.
}
//not all renderers support all cvars. lets latch some of them if they're unavailable.
void Cvar_LockUnsupportedRendererCvar(cvar_t *var, const char *str)
{
	char *oldlatch;

	if (var->latched_string)
		return; //err... its not going to do anything anyway so just leave it.
	if (!(var->flags & CVAR_RENDEREROVERRIDE))
		var->flags |= CVAR_RENDEREROVERRIDE;

	oldlatch = (char*)Z_StrDup(var->string);
	Cvar_SetCore (var, str, true);	//will use all, quote included
	var->latched_string = oldlatch;	//keep track of the original value.
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (cvar_t *v, int level)
{
	char *str;
	char buffer[65536];
	int olev;

// check variables
	if (!v)
		return false;

	if (level==RESTRICT_TEAMPLAY || (v->restriction?v->restriction:rcon_level.ival) > level)
	{
		Con_TPrintf ("You do not have the priveledges for %s\n", v->name);
		return true;
	}

	if (v->flags & CVAR_NOTFROMSERVER && Cmd_IsInsecure())
	{
		Con_TPrintf ("Server tried setting %s cvar\n", v->name);
		return true;
	}

// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		if (v->latched_string)
		{
			if (v->flags & CVAR_MAPLATCH)
			{
				Con_TPrintf ("\"%s\" is currently \"%s\"\n", v->name, COM_QuotedString(v->string, buffer, sizeof(buffer), true));
				Con_TPrintf ("Will be changed to \"%s\" on the next map\n", COM_QuotedString(v->latched_string, buffer, sizeof(buffer), true));
			}
			else if (v->flags & CVAR_VIDEOLATCH)
			{
				Con_TPrintf ("\"%s\" is \"%s\"\n", v->name, COM_QuotedString(v->string, buffer, sizeof(buffer), true));
				Con_TPrintf ("Will be changed to \"%s\" on vid_restart\n", COM_QuotedString(v->latched_string, buffer, sizeof(buffer), true));
			}
			else if (v->flags & CVAR_RENDERERLATCH)
			{
				Con_TPrintf ("\"%s\" is \"%s\"\n", v->name, COM_QuotedString(v->string, buffer, sizeof(buffer), true));
				Con_TPrintf ("Will be changed to \"%s\" on vid_reload\n", COM_QuotedString(v->latched_string, buffer, sizeof(buffer), true));
			}
			else
			{
				Con_TPrintf ("\"%s\" is \"%s\"\n", v->name, COM_QuotedString(v->latched_string, buffer, sizeof(buffer), true));
				Con_TPrintf ("Effective value is \"%s\"\n", COM_QuotedString(v->string, buffer, sizeof(buffer), true));
			}
			if (v->defaultstr)
				Con_TPrintf("Default: \"%s\"\n", COM_QuotedString(v->defaultstr, buffer, sizeof(buffer), true));
		}
		else
		{
			if (v->defaultstr && !strcmp(v->string, v->defaultstr))
				Con_TPrintf ("\"%s\" is \"%s\" (default)\n", v->name, COM_QuotedString(v->string, buffer, sizeof(buffer), true));
			else
			{
				Con_TPrintf ("\"%s\" is \"%s\"\n", v->name, COM_QuotedString(v->string, buffer, sizeof(buffer), true));
				if (v->defaultstr)
					Con_TPrintf("Default: \"%s\"\n", COM_QuotedString(v->defaultstr, buffer, sizeof(buffer), true));
			}
		}
		return true;
	}

	if (Cmd_Argc() == 2)
		str = Cmd_Argv(1);
	else
		str = Cmd_Args();

	if (v->flags & CVAR_NOSET)
	{
		if (cl_warncmd.value || developer.value)
			Con_TPrintf ("Cvar %s may not be set via the console\n", v->name);
		return true;
	}

	olev = Cmd_ExecLevel;
	Cmd_ExecLevel = level;
#ifdef HAVE_CLIENT
	if (v->flags & CVAR_USERINFO)
	{
		int seat = CL_TargettedSplit(true);
		if (Cmd_FromGamecode() && cls.protocol == CP_QUAKEWORLD)
		{	//don't bother even changing the cvar locally, just update the server's version.
			//fixme: quake2/quake3 latching.
			CL_SendSeatClientCommand(true, seat, "setinfo %s %s", v->name, COM_QuotedString(str, buffer, sizeof(buffer), false));
		}
		else
			CL_SetInfo(seat, v->name, str);
		Cmd_ExecLevel = olev;
		return true;
	}

	if (v->flags & CVAR_SERVEROVERRIDE)
	{
		if (Cmd_FromGamecode())
		{
			if (!v->defaultstr || !strcmp(v->defaultstr, str))	//returning to default
			{
				v->flags &= ~CVAR_SERVEROVERRIDE;
				if (v->latched_string)
					str = v->latched_string;	//set to the latched
			}
			else
			{
				Cvar_LockFromServer(v, str);
				Cmd_ExecLevel = olev;
				return true;
			}
		}
		//let cvar_set latch if needed.
	}
	else if (Cmd_FromGamecode())
	{//it's not latched yet
		if (v->defaultstr && strcmp(v->defaultstr, str))
		{	//lock the cvar, unless it's going to it's default value.
			Cvar_LockFromServer(v, str);
			Cmd_ExecLevel = olev;
			return true;
		}
	}
#endif

	Cmd_ExecLevel = 0;//just to try to detect any bugs that could happen from this
	Cvar_Set (v, str);	//will use all, quote included
	Cmd_ExecLevel = olev;
	return true;
}

static char *Cvar_AddDescription(char *buffer, size_t sizeofbuffer, const char *line, const char *cvardesc)
{
	if (!cvardesc || !*cvardesc || strlen(line)+100 > sizeofbuffer)
		Q_snprintfz(buffer, sizeofbuffer, "%s\n", line);
	else
	{
		//fixme: de-funstring.
		char *out = buffer, *outend = out+sizeofbuffer-2;	// 2 for \n\0
		int i,pad;
		Q_snprintfz(out, outend-out, "%s", line);
		out += strlen(out);

		pad = (strlen(line)+8) & ~7;
		if (pad < 24)
			pad = 24;
		for(i = out-buffer; i < pad; i++)
			*out++ = ' ';
		*out++ = '/';
		*out++ = '/';

		for (;;)
		{
			if (!*cvardesc || out == outend)
			{
				*out++ = '\n';
				break;
			}
			if (*cvardesc == '\n')
			{
				cvardesc++;
				*out++ = '\n';
				if (out + pad + 2 >= outend)
					break;
				for(i = 0; i < pad; i++)
					*out++ = ' ';
				*out++ = '/';
				*out++ = '/';
			}
			else
				*out++ = *cvardesc++;
		}
		*out = 0;
	}
	return buffer;
}

/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (vfsfile_t *f, qboolean all, qboolean nohidden)
{
	qboolean writtengroupheader;
	cvar_group_t *grp;
	cvar_t	*var;
	char *val;
	char *s;
	char buffer[65536];

	for (grp=cvar_groups ; grp ; grp=grp->next)
	{
		writtengroupheader = false;
		for (var = grp->cvars ; var ; var = var->next)
			if (var->flags & CVAR_ARCHIVE || (all && var != &cl_warncmd))
			{
				//yeah, don't force-save readonly cvars.
				if (var->flags & (CVAR_NOSET|CVAR_NOSAVE))
					continue;
				if (nohidden && (var->flags & CVAR_NOUNSAFEEXPAND))
					continue;

				if (!writtengroupheader)
				{
					writtengroupheader = true;
					s = va("\n// %s\n", grp->name);
					VFS_WRITE(f, s, strlen(s));
				}

				val = var->string;	//latched vars should act differently.
				if (var->latched_string)
					val = var->latched_string;

				if (var->flags & CVAR_USERCREATED)
				{
					if (var->flags & CVAR_ARCHIVE)
						s = va("seta %s %s", var->name, COM_QuotedString(val, buffer, sizeof(buffer), false));
					else
						s = va("set %s %s", var->name, COM_QuotedString(val, buffer, sizeof(buffer), false));
				}
				else
					s = va("%s %s", var->name, COM_QuotedString(val, buffer, sizeof(buffer), false));
				s = Cvar_AddDescription(buffer, sizeof(buffer), s, var->description);
				VFS_WRITE(f, s, strlen(s));
			}
	}
}

void Cvar_Hook(cvar_t *cvar, void (QDECL *callback) (struct cvar_s *var, char *oldvalue))
{
	cvar->callback = callback;
}

void Cvar_Unhook(cvar_t *cvar)
{
	cvar->callback = NULL;
}

void Cvar_ForceCallback(cvar_t *var)
{
	if (var)
		if (var->callback)
			var->callback(var, var->string);
}

void Cvar_ApplyCallbacks(int callbackflag)
{
	cvar_group_t	*grp;
	cvar_t	*var;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (var=grp->cvars ; var ; var=var->next)
	{
		if (var->flags & callbackflag)
		{
			if (var->callback)
				var->callback(var, var->string);
		}
	}
}

// standard callbacks
void QDECL Cvar_Limiter_ZeroToOne_Callback(struct cvar_s *var, char *oldvalue)
{
	if (var->value > 1)
	{
		Cvar_ForceSet(var, "1");
		return;
	}
	else if (var->value < 0)
	{
		Cvar_ForceSet(var, "0");
		return;
	}
}

void Cvar_Init(void)
{
	memset(cvar_buckets, 0, sizeof(cvar_buckets));
	//Hash_InitTable(&cvar_hash, sizeof(cvar_buckets)/Hash_BytesForBuckets(1), cvar_buckets);
}

void Cvar_Shutdown(void)
{
	cvar_t	*var;
	cvar_group_t *grp;
	while(cvar_groups)
	{
		while(cvar_groups->cvars)
		{
			var = cvar_groups->cvars;
			cvar_groups->cvars = var->next;

			if (var->defaultstr != var->enginevalue)
			{
				Cvar_DefaultFree(var->defaultstr);
				var->defaultstr = NULL;
			}
			Z_Free(var->latched_string);
			Z_Free(var->string);

			AHash_RemoveDataInsensitive(&cvar_hash, var->name, var);
			if (var->name2)
				AHash_RemoveDataInsensitive(&cvar_hash, var->name2, var);

			if (var->flags & CVAR_POINTER)
				Z_Free(var);
			else
			{
				var->string = NULL;
				var->latched_string = NULL;
			}
		}

		grp = cvar_groups;
		cvar_groups = grp->next;
		Z_Free(grp);
	}
	AHash_Cleanup(&cvar_hash);
}
