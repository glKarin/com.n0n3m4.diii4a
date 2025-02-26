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

/*64bit cpu notes:
string_t is a 32bit quantity.
this datatype needs to have enough bits to express any address that contains a string.
in a 32bit build, this is fine. with a qvm, the offset between the vm base and the string is always less than 32bits so this is fine too.
HOWEVER...
native code uses a base address of 0. this needs a 48bit datatype for any userland address. 32 bits just ain't enough.
even worse: ktx defines string_t as a 'char*'. okay, its 64bit at last... but it means that the entire entity field structure is now the wrong size with the wrong offsets.
this means CRASH!
how to fix? good luck with that. seriously.
	the only sane way to fix it is to either define a better base address (say the dll base,
		and require that all string_t values are bss or data and not from malloc, which is problematic when loading dynamic stuff from a map)
	alternatively, you could create some string_t->pointer lookup. messy.
	either way, string_t cannot be a pointer.
	probably the best solution is to stop using string_t stuff completely. move all those string values somewhere else.
	netnames will still mess things up.
so just use qvms.
oh, wait, ktx no longer supports those properly.
*/

#include "quakedef.h"

#ifdef VM_Q1

#include "pr_common.h"

/*version changes:
13: 2009/june gamecode no longer aware of edict_t data (just 'qc' fields).
14: 2017/march gamedata_t.maxentities added
15: 2017/june for-64bit string indirection changes. added GAME_CLEAR_EDICT.
16: wasted_edict_t_size is finally 0, mod is responsible for querying all strings.
*/
#define	GAME_API_VERSION		16
#define	GAME_API_VERSION_MIN	8
#define MAX_Q1QVM_EDICTS	768 //according to ktx at api version 12 (fte's protocols go to 2048). removed in v14.
#define MAPNAME_LEN 64

void PR_SV_FillWorldGlobals(world_t *w);

static int qvm_api_version;
static size_t wasted_edict_t_size;

//===============================================================

//
// system traps provided by the main engine
//
typedef enum
{
	//============== general Quake services ==================

	G_GETAPIVERSION,		// ( void);	//0

	G_DPRINT,		// ( const char *string );	//1
	// print message on the local console

	G_ERROR,		// ( const char *string );	//2
	// abort the game
	G_GetEntityToken,		//3

	G_SPAWN_ENT,		//4
	G_REMOVE_ENT,		//5
	G_PRECACHE_SOUND,
	G_PRECACHE_MODEL,
	G_LIGHTSTYLE,
	G_SETORIGIN,
	G_SETSIZE,			//10
	G_SETMODEL,
	G_BPRINT,
	G_SPRINT,
	G_CENTERPRINT,
	G_AMBIENTSOUND,		//15
	G_SOUND,
	G_TRACELINE,
	G_CHECKCLIENT,
	G_STUFFCMD,
	G_LOCALCMD,			//20
	G_CVAR,
	G_CVAR_SET,
	G_FINDRADIUS,
	G_WALKMOVE,
	G_DROPTOFLOOR,		//25
	G_CHECKBOTTOM,
	G_POINTCONTENTS,
	G_NEXTENT,
	G_AIM,
	G_MAKESTATIC,		//30
	G_SETSPAWNPARAMS,
	G_CHANGELEVEL,
	G_LOGFRAG,
	G_GETINFOKEY,
	G_MULTICAST,		//35
	G_DISABLEUPDATES,
	G_WRITEBYTE,
	G_WRITECHAR,
	G_WRITESHORT,
	G_WRITELONG,		//40
	G_WRITEANGLE,
	G_WRITECOORD,
	G_WRITESTRING,
	G_WRITEENTITY,
	G_FLUSHSIGNON,		//45
	g_memset,
	g_memcpy,
	g_strncpy,
	g_sin,
	g_cos,				//50
	g_atan2,
	g_sqrt,
	g_floor,
	g_ceil,
	g_acos,				//55
	G_CMD_ARGC,
	G_CMD_ARGV,
	G_TraceBox,			//was G_TraceCapsule, which is a misnomer.
	G_FS_OpenFile,
	G_FS_CloseFile,		//60
	G_FS_ReadFile,
	G_FS_WriteFile,
	G_FS_SeekFile,
	G_FS_TellFile,
	G_FS_GetFileList,	//65
	G_CVAR_SET_FLOAT,
	G_CVAR_STRING,
	G_Map_Extension,
	G_strcmp,
	G_strncmp,			//70
	G_stricmp,
	G_strnicmp,
	G_Find,
	G_executecmd,
	G_conprint,			//75
	G_readcmd,
	G_redirectcmd,
	G_Add_Bot,
	G_Remove_Bot,
	G_SetBotUserInfo,	//80
	G_SetBotCMD,

	G_strftime,
	G_CMD_ARGS,
	G_CMD_TOKENIZE,
	G_strlcpy,		//85
	G_strlcat,
	G_MAKEVECTORS,
	G_NEXTCLIENT,

	G_PRECACHE_VWEP_MODEL,
	G_SETPAUSE,
	G_SETUSERINFO,
	G_MOVETOGOAL,
	G_VISIBLETO,


	G_MAX
} gameImport_t;


//
// functions exported by the game subsystem
//
typedef enum
{
	GAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.
	GAME_LOADENTS,
	GAME_SHUTDOWN,	// (void);

	GAME_CLIENT_CONNECT,	 	// ( int clientNum ,int isSpectator);
	GAME_PUT_CLIENT_IN_SERVER,

	GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum,int isSpectator );

	GAME_CLIENT_DISCONNECT,			// ( int clientNum,int isSpectator );

	GAME_CLIENT_COMMAND,			// ( int clientNum,int isSpectator );

	GAME_CLIENT_PRETHINK,
	GAME_CLIENT_THINK,				// ( int clientNum,int isSpectator );
	GAME_CLIENT_POSTTHINK,

	GAME_START_FRAME,					// ( int levelTime );
	GAME_SETCHANGEPARMS, //self
	GAME_SETNEWPARMS,
	GAME_CONSOLE_COMMAND,			// ( void );
	GAME_EDICT_TOUCH,                      //(self,other)
	GAME_EDICT_THINK,                      //(self,other=world,time)
	GAME_EDICT_BLOCKED,                     //(self,other)
	GAME_CLIENT_SAY, 		//(int isteam)
	GAME_PAUSED_TIC,		//(int milliseconds)

	GAME_CLEAR_EDICT,		//v15 (sets self.fields to safe values after they're cleared)

	GAME_EDICT_CSQCSEND=200,	//fte entrypoint, called when using SendEntity.
} q1qvmgameExport_t;


typedef enum
{
	F_INT,
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
//	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
//	F_ENTITY,			// index on disk, pointer in memory
//	F_ITEM,				// index on disk, pointer in memory
//	F_CLIENT,			// index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	quintptr_t	name;
	int			ofs;
	fieldtype_t	type;
//	int			flags;
} fieldN_t;

typedef struct
{
	unsigned int		name;
	int					ofs;
	fieldtype_t			type;
//	int					flags;
} field32_t;



typedef struct {
	int	pad[28];
	int	self;
	int	other;
	int	world;
	float	time;
	float	frametime;
	int	newmis;
	float	force_retouch;
	string_t	mapname;
	float	serverflags;
	float	total_secrets;
	float	total_monsters;
	float	found_secrets;
	float	killed_monsters;
	float	parm1;
	float	parm2;
	float	parm3;
	float	parm4;
	float	parm5;
	float	parm6;
	float	parm7;
	float	parm8;
	float	parm9;
	float	parm10;
	float	parm11;
	float	parm12;
	float	parm13;
	float	parm14;
	float	parm15;
	float	parm16;
	vec3_t	v_forward;
	vec3_t	v_up;
	vec3_t	v_right;
	float	trace_allsolid;
	float	trace_startsolid;
	float	trace_fraction;
	vec3_t	trace_endpos;
	vec3_t	trace_plane_normal;
	float	trace_plane_dist;
	int	trace_ent;
	float	trace_inopen;
	float	trace_inwater;
	int	msg_entity;
	func_t	main;
	func_t	StartFrame;
	func_t	PlayerPreThink;
	func_t	PlayerPostThink;
	func_t	ClientKill;
	func_t	ClientConnect;
	func_t	PutClientInServer;
	func_t	ClientDisconnect;
	func_t	SetNewParms;
	func_t	SetChangeParms;
} q1qvmglobalvars_t;


//this is not directly usable in 64bit to refer to a 32bit qvm (hence why we have two versions).
typedef struct
{
	unsigned int		APIversion;
	unsigned int		sizeofent;
	unsigned int		maxedicts;

	quintptr_t			global;
	quintptr_t			fields;
	quintptr_t			ents;
} gameDataPrivate_t;

typedef struct
{
	unsigned int	ents;
	int				sizeofent;
	unsigned int	global;
	unsigned int	fields;
	int				APIversion;
#if GAME_API_VERSION >= 14
	unsigned int	maxentities;
#endif
} gameData32_t;

typedef struct
{
	quintptr_t		ents;
	int				sizeofent;
	quintptr_t		global;
	quintptr_t		fields;
	int				APIversion;
#if GAME_API_VERSION >= 14
	unsigned int	maxentities;
#endif
} gameDataN_t;

typedef enum {
	FS_READ_BIN,
	FS_READ_TXT,
	FS_WRITE_BIN,
	FS_WRITE_TXT,
	FS_APPEND_BIN,
	FS_APPEND_TXT
} q1qvmfsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;








#define emufields \
		emufield(gravity,		F_FLOAT)	\
		emufield(maxspeed,		F_FLOAT)	\
		emufield(movement,		F_VECTOR)	\
		emufield(vw_index,		F_FLOAT)	\
		emufield(isBot,			F_INT)		\
		emufield(items2,		F_FLOAT)	\
		emufield(trackent,		F_INT)		/*network another player instead, but not entity because of an mvdsv bug. used during bloodfest.*/	\
		emufield(hideentity,	F_INT)		/*backward nodrawtoclient, used by race mode spectators*/											\
		emufield(hideplayers,	F_INT)		/*force other clients as invisible, for race mode*/
//		emufield(visclients,	F_INT)	/*bitfield of clients that can see this entity (borked with playerslots>32). used for 'cmd tpmsg foo', and bots.*/
//		emufield(teleported,	F_INT)	/*teleport angle twisting*/
//		emufield(brokenankle,	F_FLOAT) /*not actually in mvdsv after all*/
//		emufield(mod_admin,		F_INT)	/*enable 'cmd ban' etc when &2*/


static struct
{
#define emufield(n,t) int n;
	emufields
#undef emufield
} fofs;


static const char *q1qvmentstring;
static vm_t *q1qvm;
static pubprogfuncs_t q1qvmprogfuncs;

static q1qvmglobalvars_t *gvars;
static void *evars;	//pointer to the gamecodes idea of an edict_t
static quintptr_t vevars;	//offset into the vm base of evars

/*
static char *Q1QVMPF_AddString(pubprogfuncs_t *pf, char *base, int minlength)
{
	char *n;
	int l = strlen(base);
	Con_Printf("warning: string %s will not be readable from the qvm\n", base);
	l = l<minlength?minlength:l;
	n = Z_TagMalloc(l+1, VMFSID_Q1QVM);
	strcpy(n, base);
	return n;
}
*/

static edict_t *QDECL Q1QVMPF_EdictNum(pubprogfuncs_t *pf, unsigned int num)
{
	edict_t *e;

	if (/*num < 0 ||*/ num >= sv.world.max_edicts)
		return NULL;

	e = q1qvmprogfuncs.edicttable[num];
	if (!e)
	{
		e = q1qvmprogfuncs.edicttable[num] = Z_TagMalloc(sizeof(edict_t)+sizeof(extentvars_t), VMFSID_Q1QVM);
		e->v = (stdentvars_t*)((char*)evars + (num * sv.world.edict_size) + wasted_edict_t_size);
		e->xv = (extentvars_t*)(e+1);
		e->entnum = num;
	}
	return e;
}

static unsigned int QDECL Q1QVMPF_NumForEdict(pubprogfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}

static int QDECL Q1QVMPF_EdictToProgs(pubprogfuncs_t *pf, edict_t *e)
{	//sadly ktx still uses byte-offset-from-world :(
	return e->entnum*sv.world.edict_size;
}
static edict_t *QDECL Q1QVMPF_ProgsToEdict(pubprogfuncs_t *pf, int num)
{
	//sadly ktx still uses byte-offset-from-world :(
	if (num % sv.world.edict_size)
		Con_Printf("Edict To Progs with remainder\n");
	num /= sv.world.edict_size;

	return Q1QVMPF_EdictNum(pf, num);
}

static void Q1QVMED_ClearEdict (edict_t *e, qboolean wipe)
{
	int num = e->entnum;
	if (wipe)
	{
		memset (e->v, 0, sv.world.edict_size - wasted_edict_t_size);
		memset (e->xv, 0, sizeof(*e->xv));
	}
	if (qvm_api_version >= 15)
	{
		int oself = pr_global_struct->self;
		pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, e);
		VM_Call(q1qvm, GAME_CLEAR_EDICT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		pr_global_struct->self = oself;
	}
	e->ereftype = ER_ENTITY;
	e->entnum = num;
}
static void QDECL Q1QVMPF_ClearEdict(pubprogfuncs_t *pf, edict_t *e)
{
	Q1QVMED_ClearEdict(e, true);
}

static void QDECL Q1QVMPF_EntRemove(pubprogfuncs_t *pf, edict_t *e, pbool instant)
{
	if (!ED_CanFree(e))
		return;
	e->ereftype = ER_FREE;
	e->freetime = instant?0:sv.time;
}

static edict_t *QDECL Q1QVMPF_EntAlloc(pubprogfuncs_t *pf, pbool object, size_t extrasize)
{
	int i;
	edict_t *e;
	for ( i=0 ; i<sv.world.num_edicts ; i++)
	{
		e = (edict_t*)EDICT_NUM_PB(pf, i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e || (ED_ISFREE(e) && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) ))
		{
			Q1QVMED_ClearEdict (e, true);

			ED_Spawned((struct edict_s *) e, false);
			return (struct edict_s *)e;
		}
	}

	if (i >= sv.world.max_edicts-1)	//try again, but use timed out ents.
	{
		for ( i=0 ; i<sv.world.num_edicts ; i++)
		{
			e = (edict_t*)EDICT_NUM_PB(pf, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || ED_ISFREE(e))
			{
				Q1QVMED_ClearEdict (e, true);

				ED_Spawned((struct edict_s *) e, false);
				return (struct edict_s *)e;
			}
		}

		if (i >= sv.world.max_edicts-1)
		{
			Sys_Error ("ED_Alloc: no free edicts");
		}
	}

	sv.world.num_edicts++;
	e = (edict_t*)Q1QVMPF_EdictNum(pf, i);

// new ents come ready wiped (unless 15 in which case we need to give the gamecode a chance to set safe defaults)
	if (qvm_api_version >= 15)
		Q1QVMED_ClearEdict (e, false);

	ED_Spawned((struct edict_s *) e, false);

	return (struct edict_s *)e;
}

static int QDECL Q1QVMPF_LoadEnts(pubprogfuncs_t *pf, const char *mapstring, void *ctx,
								  void (PDECL *memoryreset) (pubprogfuncs_t *progfuncs, void *ctx),
								  void (PDECL *ent_callback) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend),
								  pbool (PDECL *ext_callback)(pubprogfuncs_t *pf, void *ctx, const char **str))
{
	//the qvm calls the spawn functions itself.
	//no saved-games.
	q1qvmentstring = mapstring;
	VM_Call(q1qvm, GAME_LOADENTS, 0, 0, 0);
	q1qvmentstring = NULL;
	return sv.world.edict_size;
}

static int QDECL Q1QVMPF_QueryField(pubprogfuncs_t *prinst, unsigned int fieldoffset, etype_t *type, char const**name, evalc_t *fieldcache)
{
	*type = ev_void;
	*name = "?";

	fieldcache->varname = NULL;
	fieldcache->spare[0] = fieldoffset;
	return true;
}

static eval_t *QDECL Q1QVMPF_GetEdictFieldValue(pubprogfuncs_t *pf, edict_t *e, const char *fieldname, etype_t type, evalc_t *cache)
{
	if (cache && !cache->varname)
	{
		return (eval_t*)((char*)e->v + cache->spare[0]-wasted_edict_t_size);
	}
	if (!strcmp(fieldname, "message"))
	{
		return (eval_t*)&e->v->message;
	}
	return NULL;
}

static eval_t	*QDECL Q1QVMPF_FindGlobal		(pubprogfuncs_t *prinst, const char *name, progsnum_t num, etype_t *type)
{
	return NULL;
}

static globalvars_t *QDECL Q1QVMPF_Globals(pubprogfuncs_t *prinst, progsnum_t prnum)
{
	return NULL;
}

static string_t QDECL Q1QVMPF_StringToProgs(pubprogfuncs_t *prinst, const char *str)
{
	quintptr_t ret = (str - (char*)VM_MemoryBase(q1qvm));
	if (ret >= VM_MemoryMask(q1qvm))
		return 0;
	if (ret >= 0xffffffff)
		return 0;	//invalid string! blame 64bit.
	return ret;
}

static void *ASMCALL QDECL Q1QVMPF_PointerToNative(pubprogfuncs_t *prinst, quintptr_t str)
{
	void *ret;
	if (!str || (quintptr_t)str >= VM_MemoryMask(q1qvm))
		return NULL;	//null or invalid pointers.
	ret = (char*)VM_MemoryBase(q1qvm) + str;
	return ret;
}

static const char *ASMCALL QDECL Q1QVMPF_StringToNative(pubprogfuncs_t *prinst, string_t str)
{
	quintptr_t ref;
	if (str == ~0)
		return " ";	//models are weird. yes, this is a hack.
	if (qvm_api_version >= 15)
	{
		qboolean stringishacky = sizeof(quintptr_t) != sizeof(string_t) && qvm_api_version >= 15 && !VM_NonNative(q1qvm);	//silly bullshit. Really, native gamecode should have its own implementation of this builtin or something, especially as its pretty much only ever used for classnames.
		if (!str)
			return "";	//invalid...
		else if (stringishacky)
		{
			if (str >= 0 && str < sv.world.edict_size*sv.world.max_edicts - sizeof(quintptr_t))
				ref = *(quintptr_t*)((char*)evars + (int)str);	//extra indirection added in api 15.
			else
				return ""; //error
		}
		else
		{
			if (str >= 0 && str < sv.world.edict_size*sv.world.max_edicts - sizeof(quintptr_t))
				ref = *(string_t*)((char*)evars + (int)str);	//extra indirection added in api 15.
			else
				return ""; //error
		}
	}
	else
		ref = str;
	if (!ref || (quintptr_t)ref >= VM_MemoryMask(q1qvm))
		return "";	//null or invalid pointers.
	return (char*)VM_MemoryBase(q1qvm) + ref;
}
static void QDECL Q1QVMPF_SetStringField(pubprogfuncs_t *progfuncs, struct edict_s *ed, string_t *fld, const char *str, pbool str_is_static)
{
	if (!str_is_static)
		return;
	if (qvm_api_version >= 15)
	{
		qboolean stringishacky = sizeof(quintptr_t) != sizeof(string_t) && qvm_api_version >= 15 && !VM_NonNative(q1qvm);	//silly bullshit. Really, native gamecode should have its own implementation of this builtin or something, especially as its pretty much only ever used for classnames.
		quintptr_t nval = (str - (char*)VM_MemoryBase(q1qvm));
		if (nval >= VM_MemoryMask(q1qvm))
			return;

		if (!*fld)
			Con_DPrintf("Ignoring string set. mod pointer not set.\n");
		else if (stringishacky)
		{
			if (*fld >= 0 && *fld < sv.world.edict_size*sv.world.max_edicts - sizeof(quintptr_t))
				*(quintptr_t*)((char*)evars + *fld) = nval;
			else
				Con_DPrintf("Ignoring string set outside of progs VM\n");
		}
		else
		{
			if (nval >= 0xffffffff)
				return;	//invalid string! blame 64bit.
			if (*fld >= 0 && *fld < sv.world.edict_size*sv.world.max_edicts - sizeof(string_t))
				*(string_t*)((char*)evars + *fld) = (string_t)nval;
			else
				Con_DPrintf("Ignoring string set outside of progs VM\n");
		}
	}
	else
	{
		string_t newval = progfuncs->StringToProgs(progfuncs, str);
		if (newval || !str)
			*fld = newval;
		else if (!str)
			*fld = 0;
		else
		{
			*fld = ~0;
			//Con_DPrintf("Ignoring string set outside of progs VM\n");
		}
	}
}

static void Q1QVMPF_SetStringGlobal(pubprogfuncs_t *progfuncs, string_t *fld, const char *str, size_t copysize)
{
	if (qvm_api_version >= 15)
	{
		qboolean stringishacky = sizeof(quintptr_t) != sizeof(string_t) && qvm_api_version >= 15 && !VM_NonNative(q1qvm);	//silly bullshit. Really, native gamecode should have its own implementation of this builtin or something, especially as its pretty much only ever used for classnames.
		if (!*fld)
			Con_Printf("Q1QVM: string reference not set. Fix the mod.\n");
		else if (stringishacky)
		{	//quintptr_t
//			if (*fld >= 0 && *fld < sv.world.edict_size*sv.world.max_edicts - sizeof(intptr_t))
			{
				if (!*(quintptr_t*)((char*)gvars + *fld))
				{
					Con_DPrintf("String buffer not set. Hacking the data in instead.\n");
					*(quintptr_t*)((char*)gvars + *fld) = (str - (char*)VM_MemoryBase(q1qvm));
				}
				else
				{
					char *ptr = (char*)*(quintptr_t*)((char*)gvars + *fld);
					Q_strncpyz(ptr, str, copysize);
				}
			}
		}
		else
		{	//string_t
//			if (*fld >= 0 && *fld < sv.world.edict_size*sv.world.max_edicts - sizeof(string_t))
			{
				if (!*(quintptr_t*)((char*)gvars + *fld))
				{
					quintptr_t nval = (str - (char*)VM_MemoryBase(q1qvm));;
					if (nval > VM_MemoryMask(q1qvm))
					{
						Con_Printf("Q1QVM: String buffer not set. Data out of QVM memory space. Fix the mod.\n");
					}
					else
					{
						Con_DPrintf("String buffer not set. Hacking the data in instead.\n");
						*(quintptr_t*)((char*)gvars + *fld) = (str - (char*)VM_MemoryBase(q1qvm));;
					}
				}
				else
				{
					char *ptr = (char*)*(quintptr_t*)((char*)gvars + *fld);
					Q_strncpyz(ptr, str, copysize);
				}
			}
		}
	}
	else
	{
		if (!*fld)
		{
			quintptr_t nval = (str - (char*)VM_MemoryBase(q1qvm));
			if (nval > VM_MemoryMask(q1qvm))
			{
				Con_Printf("Q1QVM: String buffer not set. Data out of QVM memory space. Fix the mod.\n");
			}
			else
			{
				Con_DPrintf("String buffer not set. Hacking the data in instead.\n");
				*fld = (str - (char*)VM_MemoryBase(q1qvm));
			}
		}
		else
		{
			char *ptr = (char*)VM_MemoryBase(q1qvm) + *fld;
			Q_strncpyz(ptr, str, copysize);
		}
	}
}

static int WrapQCBuiltin(builtin_t func, void *offset, quintptr_t mask, const qintptr_t *arg, char *argtypes)
{
	globalvars_t gv;
	int argnum=0;
	while(*argtypes)
	{
		switch(*argtypes++)
		{
		case 'f':
			gv.param[argnum++].f = VM_FLOAT(*arg++);
			break;
		case 'i':
			gv.param[argnum++].f = VM_LONG(*arg++);	//vanilla qc does not support ints, but qvms do. this means ints need to be converted to floats for the builtin to understand them properly.
			break;
		case 'n':	//ent num
			gv.param[argnum++].i = EDICT_TO_PROG(svprogfuncs, Q1QVMPF_EdictNum(svprogfuncs, VM_LONG(*arg++)));
			break;
		case 'v':	//three seperate args -> 1 vector
			gv.param[argnum].vec[0] = VM_FLOAT(*arg++);
			gv.param[argnum].vec[1] = VM_FLOAT(*arg++);
			gv.param[argnum].vec[2] = VM_FLOAT(*arg++);
			argnum++;
			break;
		case 's':
			gv.param[argnum].i = VM_LONG(*arg++);
			argnum++;
			break;
		}
	}
	svprogfuncs->callargc = argnum;
	gv.ret.i = 0;
	func(svprogfuncs, &gv);
	return gv.ret.i;
}

#define VALIDATEPOINTER(o,l) if ((qintptr_t)o + l >= mask || VM_POINTER(o) < offset) SV_Error("Call to game trap passes invalid pointer\n");	//out of bounds.
static qintptr_t QVM_GetAPIVersion (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return qvm_api_version;
}

static qintptr_t QVM_DPrint (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	Con_DPrintf("%s", (char*)VM_POINTER(arg[0]));
	return 0;
}

static qintptr_t QVM_Error (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	SV_Error("Q1QVM: %s", (char*)VM_POINTER(arg[0]));
	return 0;
}

static qintptr_t QVM_GetEntityToken (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	if (VM_OOB(arg[0], arg[1]) || !arg[1])
		return false;
	if (q1qvmentstring)
	{
		char *ret = VM_POINTER(arg[0]);
		q1qvmentstring = COM_Parse(q1qvmentstring);
		Q_strncpyz(ret, com_token, VM_LONG(arg[1]));
		return q1qvmentstring != NULL;
	}
	else
	{
		char *ret = VM_POINTER(arg[0]);
		*ret = '\0';
		return false;
	}
}

static qintptr_t QVM_Spawn_Ent (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return Q1QVMPF_EntAlloc(svprogfuncs, false, 0)->entnum;
}

static qintptr_t QVM_Remove_Ent (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	if (arg[0] >= sv.world.max_edicts)
		return false;
	Q1QVMPF_EntRemove(svprogfuncs, q1qvmprogfuncs.edicttable[arg[0]], false);
	return true;
}

static qintptr_t QVM_Precache_Sound (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return PF_precache_sound_Internal(svprogfuncs, VM_POINTER(arg[0]), false);
}
static qintptr_t QVM_Precache_Model (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return PF_precache_model_Internal(svprogfuncs, VM_POINTER(arg[0]), false);
}
static qintptr_t QVM_LightStyle (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	vec3_t rgb = {1,1,1};
	PF_applylightstyle(VM_LONG(arg[0]), VM_POINTER(arg[1]), rgb);
	return 0;
}
static qintptr_t QVM_SetOrigin (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, VM_LONG(arg[0]));
	if (!e || ED_ISFREE(e))
		return false;

	e->v->origin[0] = VM_FLOAT(arg[1]);
	e->v->origin[1] = VM_FLOAT(arg[2]);
	e->v->origin[2] = VM_FLOAT(arg[3]);
	World_LinkEdict (&sv.world, (wedict_t*)e, false);
	return true;
}
static qintptr_t QVM_SetSize (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, arg[0]);
	if (!e || ED_ISFREE(e))
		return false;

	e->v->mins[0] = VM_FLOAT(arg[1]);
	e->v->mins[1] = VM_FLOAT(arg[2]);
	e->v->mins[2] = VM_FLOAT(arg[3]);

	e->v->maxs[0] = VM_FLOAT(arg[4]);
	e->v->maxs[1] = VM_FLOAT(arg[5]);
	e->v->maxs[2] = VM_FLOAT(arg[6]);

	VectorSubtract (e->v->maxs, e->v->mins, e->v->size);
	World_LinkEdict (&sv.world, (wedict_t*)e, false);
	return true;
}
static qintptr_t QVM_SetModel (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, arg[0]);
	PF_setmodel_Internal(svprogfuncs, e, VM_POINTER(arg[1]));
	return 0;
}
static qintptr_t QVM_BPrint (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int flags = VM_LONG(arg[2]);
	if (qvm_api_version < 13)
		flags = 0;	//added mid-v12, resulting in undefined values with early-12 mods.
	SV_BroadcastPrint(flags, arg[0], (char*)VM_POINTER(arg[1]));
	return 0;
}
static qintptr_t QVM_SPrint (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int clnum = VM_LONG(arg[0])-1;
	int level = VM_LONG(arg[1]);
	const char *text = VM_POINTER(arg[2]);
	unsigned int flags = VM_LONG(arg[3]);
#define SPRINT_IGNOREINDEMO   (   1<<0) // do not put such message in mvd demo
	client_t *cl = &svs.clients[clnum];
	if (clnum >= sv.allocated_client_slots)
		return 0;
	if (qvm_api_version < 13)
		flags = 0;	//added mid-v12, resulting in undefined values with early-12 mods.

	if (flags & SPRINT_IGNOREINDEMO)
	{
		if (level >= cl->messagelevel)
			SV_PrintToClient(cl, level, text);
	}
	else
		SV_ClientPrintf(cl, level, "%s", text);
	return 0;
}
static qintptr_t QVM_CenterPrint (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	PF_centerprint_Internal(VM_LONG(arg[0]), false, VM_POINTER(arg[1]));
	return 0;
}
static qintptr_t QVM_AmbientSound (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	vec3_t pos;
	pos[0] = VM_FLOAT(arg[0]);
	pos[1] = VM_FLOAT(arg[1]);
	pos[2] = VM_FLOAT(arg[2]);
	PF_ambientsound_Internal(pos, VM_POINTER(arg[3]), VM_FLOAT(arg[4]), VM_FLOAT(arg[5]));
	return 0;
}
static qintptr_t QVM_Sound (void *offset, quintptr_t mask, const qintptr_t *arg)
{
//	( int edn, int channel, char *samp, float vol, float att )
	int channel = VM_LONG(arg[1]);
	int flags = 0;
	if (channel & 8)
	{	//based on quakeworld, remember
		channel = (channel & 7) | ((channel&~15)>>1);
		flags |= CF_SV_RELIABLE;
	}
	SVQ1_StartSound (NULL, (wedict_t*)Q1QVMPF_EdictNum(svprogfuncs, VM_LONG(arg[0])), channel, VM_POINTER(arg[2]), VM_FLOAT(arg[3])*255, VM_FLOAT(arg[4]), 0, 0, flags);
	return 0;
}
static qintptr_t QVM_TraceLine (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_svtraceline, offset, mask, arg, "vvin");
	return 0;
}
static qintptr_t QVM_CheckClient (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return PF_checkclient_Internal(svprogfuncs);
}
static qintptr_t QVM_StuffCmd (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	PF_stuffcmd_Internal(VM_LONG(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]));
	return 0;
}
static qintptr_t QVM_LocalCmd (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	Cbuf_AddText (VM_POINTER(arg[0]), RESTRICT_INSECURE);
	return 0;
}
static qintptr_t QVM_CVar (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	cvar_t *c;
	char *vname = VM_POINTER(arg[0]);

	//paused state is not a cvar in fte.
	if (!strcmp(vname, "sv_paused"))
		return VM_LONG(sv.paused);

	c = Cvar_Get(vname, "", 0, "Gamecode");
	return VM_LONG(c->value);
}
static qintptr_t QVM_CVar_Set (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	cvar_t *var;
	var = Cvar_Get(VM_POINTER(arg[0]), VM_POINTER(arg[1]), 0, "Gamecode variables");
	if (!var)
		return -1;
	Cvar_Set (var, VM_POINTER(arg[1]));
	return 0;
}

static qintptr_t QVM_FindRadius (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int start = ((char*)VM_POINTER(arg[0]) - (char*)evars) / sv.world.edict_size;
	edict_t *ed;
	vec3_t diff;
	float *org = VM_POINTER(arg[1]);
	float rad = VM_FLOAT(arg[2]);
	rad *= rad;
	for(start++; start < sv.world.num_edicts; start++)
	{
		ed = EDICT_NUM_PB(svprogfuncs, start);
		if (ED_ISFREE(ed))
			continue;
		VectorSubtract(ed->v->origin, org, diff);
		if (rad > DotProduct(diff, diff))
			return (qintptr_t)(vevars + start*sv.world.edict_size);
	}
	return 0;
}
static qintptr_t QVM_WalkMove (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	wedict_t *ed = WEDICT_NUM_UB(svprogfuncs, arg[0]);
	float yaw = VM_FLOAT(arg[1]);
	float dist = VM_FLOAT(arg[2]);
	vec3_t move;
	vec3_t axis[3];

	World_GetEntGravityAxis(ed, axis);

	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	return World_movestep(&sv.world, (wedict_t*)ed, move, axis, true, false, NULL);
}
static qintptr_t QVM_DropToFloor (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	edict_t		*ent;
	pvec3_t		end;
	pvec3_t		start;
	trace_t		trace;
	extern cvar_t pr_droptofloorunits;

	ent = EDICT_NUM_UB(svprogfuncs, arg[0]);

	VectorCopy (ent->v->origin, end);
	if (pr_droptofloorunits.value > 0)
		end[2] -= pr_droptofloorunits.value;
	else
		end[2] -= 256;

	VectorCopy (ent->v->origin, start);
	trace = World_Move (&sv.world, start, ent->v->mins, ent->v->maxs, end, MOVE_NORMAL, (wedict_t*)ent);

	if (trace.fraction == 1 || trace.allsolid)
		return false;
	else
	{
		VectorCopy (trace.endpos, ent->v->origin);
		World_LinkEdict (&sv.world, (wedict_t*)ent, false);
		ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
		ent->v->groundentity = EDICT_TO_PROG(svprogfuncs, trace.ent);
		return true;
	}
}
static qintptr_t QVM_CheckBottom (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	vec3_t up = {0,0,1};
	return World_CheckBottom(&sv.world, (wedict_t*)EDICT_NUM_UB(svprogfuncs, VM_LONG(arg[0])), up);
}
static qintptr_t QVM_PointContents (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	vec3_t v;
	v[0] = VM_FLOAT(arg[0]);
	v[1] = VM_FLOAT(arg[1]);
	v[2] = VM_FLOAT(arg[2]);
	return sv.world.worldmodel->funcs.PointContents(sv.world.worldmodel, NULL, v);
}
static qintptr_t QVM_NextEnt (void *offset, quintptr_t mask, const qintptr_t *arg)
{	//input output are entity numbers
	unsigned int i;
	edict_t	*ent;

	i = VM_LONG(arg[0]);
	while (1)
	{
		i++;
		if (i >= sv.world.num_edicts)
		{
			return 0;
		}
		ent = EDICT_NUM_PB(svprogfuncs, i);
		if (!ED_ISFREE(ent))
		{
			return i;
		}
	}
}
static qintptr_t QVM_Aim (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	Con_DPrintf("QVM_Aim: not implemented\n");
	return 0;	//not in mvdsv anyway
}
static qintptr_t QVM_MakeStatic (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_makestatic, offset, mask, arg, "n");
	return 0;
}
static qintptr_t QVM_SetSpawnParams (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_setspawnparms, offset, mask, arg, "n");
	return 0;
}
static qintptr_t QVM_ChangeLevel (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	const char *arg_mapname = VM_POINTER(arg[0]);
	const char *arg_entfilename = (qvm_api_version > 13)?(VM_POINTER(arg[1])):"";

	char newmap[MAX_QPATH];
	if (sv.mapchangelocked)
		return 0;

	if (arg_entfilename && *arg_entfilename)
	{
		int nl = strlen(arg_mapname);
		if (!strncmp(arg_mapname, arg_entfilename, nl) && arg_mapname[nl]=='#')
			arg_mapname = arg_entfilename;
		else
			Con_Printf(CON_ERROR"%s: named ent file does not match map\n", "QVM_ChangeLevel");
	}

	sv.mapchangelocked = true;
	COM_QuotedString(arg_mapname, newmap, sizeof(newmap), false);
	Cbuf_AddText (va("\nchangelevel %s\n", newmap), RESTRICT_LOCAL);
	return 1;
}
static qintptr_t QVM_ChangeLevelHub (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	const char *arg_mapname = VM_POINTER(arg[0]);
	const char *arg_entfilename = VM_POINTER(arg[1]);
	const char *arg_startspot = VM_POINTER(arg[2]);

	char newmap[MAX_QPATH];
	char startspot[MAX_QPATH];
	if (sv.mapchangelocked)
		return 0;

	if (arg_entfilename && *arg_entfilename)
	{
		int nl = strlen(arg_mapname);
		if (!strncmp(arg_mapname, arg_entfilename, nl) && arg_mapname[nl]=='#')
			arg_mapname = arg_entfilename;
		else
			Con_Printf(CON_ERROR"%s: named ent file does not match map\n", "QVM_ChangeLevelHub");
	}

	sv.mapchangelocked = true;
	COM_QuotedString(arg_mapname, newmap, sizeof(newmap), false);
	COM_QuotedString(arg_startspot, startspot, sizeof(startspot), false);
	Cbuf_AddText (va("\nchangelevel %s %s\n", newmap, startspot), RESTRICT_LOCAL);
	return 1;
}
static qintptr_t QVM_LogFrag (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_logfrag, offset, mask, arg, "nn");
	return 0;
}
static qintptr_t QVM_Precache_VWep_Model (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	const char *s = VM_POINTER(arg[0]);
	int i;

	if (!*s || strchr(s, '\"') || strchr(s, ';') || strchr(s, '\t') || strchr(s, '\n'))
		Con_Printf("QVM_Precache_VWep_Model: bad string\n");
	else
	{
		for (i = 0; i < sizeof(sv.strings.vw_model_precache)/sizeof(sv.strings.vw_model_precache[0]); i++)
		{
			if (!sv.strings.vw_model_precache[i])
			{
				if (sv.state != ss_loading)
				{
					Con_Printf("QVM_Precache_VWep_Model: not spawning\n");
					return 0;
				}
				sv.strings.vw_model_precache[i] = s;
				return i;
			}
			if (!strcmp(sv.strings.vw_model_precache[i], s))
				return i;
		}
		Con_Printf("QVM_Precache_VWep_Model: overflow\n");
	}
	return 0;
}
static qintptr_t QVM_GetInfoKey (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *v;
	if (VM_OOB(arg[2], arg[3]))
		return -1;
	v = PF_infokey_Internal(VM_LONG(arg[0]), VM_POINTER(arg[1]));
	Q_strncpyz(VM_POINTER(arg[2]), v, VM_LONG(arg[3]));
	return 0;
}
static qintptr_t QVM_Multicast (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_multicast, offset, mask, arg, "vi");
	return 0;
}
static qintptr_t QVM_DisableUpdates (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	//FIXME: remember to ask mvdsv people why this is useful
	Con_Printf("G_DISABLEUPDATES: not supported\n");
	return 0;
}
static qintptr_t QVM_WriteByte (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteByte, offset, mask, arg, "ii");
	return 0;
}
static qintptr_t QVM_WriteChar (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteChar, offset, mask, arg, "ii");
	return 0;
}
static qintptr_t QVM_WriteShort (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteShort, offset, mask, arg, "ii");
	return 0;
}
static qintptr_t QVM_WriteLong (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteLong, offset, mask, arg, "ii");
	return 0;
}
static qintptr_t QVM_WriteAngle (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteAngle, offset, mask, arg, "if");
	return 0;
}
static qintptr_t QVM_WriteCoord (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteCoord, offset, mask, arg, "if");
	return 0;
}
static qintptr_t QVM_WriteString (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	PF_WriteString_Internal(VM_LONG(arg[0]), VM_POINTER(arg[1]));
	return 0;
}
static qintptr_t QVM_WriteEntity (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_WriteEntity, offset, mask, arg, "in");
	return 0;
}
static qintptr_t QVM_FlushSignon (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	SV_FlushSignon (false);
	return 0;
}
static qintptr_t QVM_memset (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	void *dst = VM_POINTER(arg[0]);
	VALIDATEPOINTER(arg[0], arg[2]);
	memset(dst, arg[1], arg[2]);
	return arg[0];
}
static qintptr_t QVM_memcpy (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	void *dst = VM_POINTER(arg[0]);
	void *src = VM_POINTER(arg[1]);
	VALIDATEPOINTER(arg[0], arg[2]);
	memmove(dst, src, arg[2]);
	return arg[0];
}
static qintptr_t QVM_strncpy (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	VALIDATEPOINTER(arg[0], arg[2]);
	Q_strncpyS(VM_POINTER(arg[0]), VM_POINTER(arg[1]), arg[2]);
	return arg[0];
}
static qintptr_t QVM_sin (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = sin(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_cos (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = cos(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_atan2 (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = atan2(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]));
	return u.r;
}
static qintptr_t QVM_sqrt (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = sqrt(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_floor (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = floor(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_ceil (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = ceil(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_acos (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	union
	{
		qintptr_t r;
		float f;
	} u = {0};
	u.f = acos(VM_FLOAT(arg[0]));
	return u.r;
}
static qintptr_t QVM_Cmd_ArgC (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return Cmd_Argc();
}
static qintptr_t QVM_Cmd_ArgV (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *c;
	c = Cmd_Argv(VM_LONG(arg[0]));
	if (VM_OOB(arg[1], arg[2]))
		return -1;
	Q_strncpyz(VM_POINTER(arg[1]), c, VM_LONG(arg[2]));
	return 0;
}
static qintptr_t QVM_TraceBox (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_svtraceline, offset, mask, arg, "vvinvv");
	return 0;
}



typedef struct {
	vfsfile_t *file;
} vm_fopen_files_t;
static vm_fopen_files_t vm_fopen_files[64];

static qintptr_t QVM_FS_OpenFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	const char *name = VM_POINTER(arg[0]);
	int *handle = VM_POINTER(arg[1]);
	int fmode = VM_LONG(arg[2]);
	int fnum;
	static struct {
		const char *mode;
		enum fs_relative root;
	} mode[] = {
		/*FS_READ_BIN*/{"rb",FS_GAME},
		/*FS_READ_TXT*/{"rt",FS_GAME},
		/*FS_WRITE_BIN*/{"wb",FS_GAMEONLY},
		/*FS_WRITE_TXT*/{"wt",FS_GAMEONLY},
		/*FS_APPEND_BIN*/{"ab",FS_GAMEONLY},
		/*FS_APPEND_TXT*/{"at",FS_GAMEONLY},
	};
	if (fmode < 0 || fmode >= countof(mode))
		return -1;
	for (fnum = 0; fnum < countof(vm_fopen_files); fnum++)
		if (!vm_fopen_files[fnum].file)
			break;
	if (fnum == countof(vm_fopen_files))	//too many already open
		return -1;

	vm_fopen_files[fnum].file = FS_OpenVFS(name, mode[fmode].mode, mode[fmode].root);
	if (!vm_fopen_files[fnum].file)
		return -1;
	*handle = fnum+1;
	return VFS_GETLEN(vm_fopen_files[fnum].file);
}
static qintptr_t QVM_FS_CloseFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int fnum = VM_LONG(arg[0])-1;
	if (fnum >= 0 && fnum < countof(vm_fopen_files) && vm_fopen_files[fnum].file)
	{
		VFS_CLOSE(vm_fopen_files[fnum].file);
		vm_fopen_files[fnum].file = NULL;
		return 0;
	}
	return -1;
}
static void QVM_FS_CloseFileAll (void)
{
	size_t fnum;
	for (fnum = 0; fnum < countof(vm_fopen_files); fnum++)
		if (vm_fopen_files[fnum].file)
		{
			VFS_CLOSE(vm_fopen_files[fnum].file);
			vm_fopen_files[fnum].file = NULL;
		}
}
static qintptr_t QVM_FS_ReadFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	void *dest = VM_POINTER(arg[0]);
	int size = VM_LONG(arg[1]);
	int fnum = VM_LONG(arg[2])-1;
	if (VM_OOB(arg[0], size))
		return 0;
	if (fnum >= 0 && fnum < countof(vm_fopen_files) && vm_fopen_files[fnum].file && vm_fopen_files[fnum].file->ReadBytes)
		return VFS_READ(vm_fopen_files[fnum].file, dest, size);
	return 0;
}
static qintptr_t QVM_FS_WriteFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	void *dest = VM_POINTER(arg[0]);
	int size = VM_LONG(arg[1]);
	int fnum = VM_LONG(arg[2])-1;
	if (VM_OOB(arg[0], size))
		return 0;
	if (fnum >= 0 && fnum < countof(vm_fopen_files) && vm_fopen_files[fnum].file && vm_fopen_files[fnum].file->ReadBytes)
		return VFS_WRITE(vm_fopen_files[fnum].file, dest, size);
	return 0;
}
static qintptr_t QVM_FS_SeekFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int fnum = VM_LONG(arg[0])-1;
	quintptr_t foffset = arg[1];
	int seektype = VM_LONG(arg[2]);
	if (fnum >= 0 && fnum < countof(vm_fopen_files) && vm_fopen_files[fnum].file && vm_fopen_files[fnum].file->seekstyle != SS_UNSEEKABLE)
	{
		if (seektype == 0)	//cur
			foffset += VFS_TELL(vm_fopen_files[fnum].file);
		else if (seektype == 2)	//end
			foffset = VFS_GETLEN(vm_fopen_files[fnum].file) + (qintptr_t)foffset;
		return VFS_SEEK(vm_fopen_files[fnum].file, foffset);
	}
	return 0;
}
static qintptr_t QVM_FS_TellFile (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int fnum = VM_LONG(arg[0])-1;
	if (fnum >= 0 && fnum < countof(vm_fopen_files) && vm_fopen_files[fnum].file)
	{
		return VFS_TELL(vm_fopen_files[fnum].file);
	}
	return -1;
}




//filesystem searches result in a tightly-packed blob of null-terminated filenames (along with a count for how many entries)
typedef struct {
	char *initialbuffer;
	char *buffer;
	int found;
	int bufferleft;
	int skip;
} vmsearch_t;
static int QDECL VMEnum(const char *match, qofs_t size, time_t mtime, void *args, searchpathfuncs_t *spath)
{
	char *check;
	int newlen;
	match += ((vmsearch_t *)args)->skip;
	newlen = strlen(match)+1;
	if (newlen > ((vmsearch_t *)args)->bufferleft)
		return false;	//too many files for the buffer

	check = ((vmsearch_t *)args)->initialbuffer;
	while(check < ((vmsearch_t *)args)->buffer)
	{
		if (!Q_strcasecmp(check, match))
			return true;	//we found this one already
		check += strlen(check)+1;
	}

	memcpy(((vmsearch_t *)args)->buffer, match, newlen);
	((vmsearch_t *)args)->buffer+=newlen;
	((vmsearch_t *)args)->bufferleft-=newlen;
	((vmsearch_t *)args)->found++;
	return true;
}
static qintptr_t QVM_FS_GetFileList (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	vmsearch_t vms;
	const char *path = VM_POINTER(arg[0]);
	const char *ext = VM_POINTER(arg[1]);
	char *output = VM_POINTER(arg[2]);
	size_t buffersize = VM_LONG(arg[3]);
	if (VM_OOB(arg[2], buffersize))
		return 0;

	vms.initialbuffer = vms.buffer = output;
	vms.skip = strlen(path)+1;
	vms.bufferleft = buffersize;
	vms.found=0;
	if (*(const char *)ext == '.' || *(const char *)ext == '/')
		COM_EnumerateFiles(va("%s/*%s", path, ext), VMEnum, &vms);
	else
		COM_EnumerateFiles(va("%s/*.%s", path, ext), VMEnum, &vms);
	return vms.found;
}
static qintptr_t QVM_CVar_Set_Float (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	cvar_t *var;
	var = Cvar_Get(VM_POINTER(arg[0]), va("%f", VM_FLOAT(arg[1])), 0, "Gamecode variables");
	if (!var)
		return -1;
	Cvar_SetValue (var, VM_FLOAT(arg[1]));
	return 0;
}
static qintptr_t QVM_CVar_String (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *n = VM_POINTER(arg[0]);
	cvar_t *cv;
	if (VM_OOB(arg[1], arg[2]))
		return -1;
	if (!strcmp(n, "version"))
	{
		n = version_string();
		Q_strncpyz(VM_POINTER(arg[1]), n, VM_LONG(arg[2]));
	}
	else
	{
		cv = Cvar_Get(n, "", 0, "QC variables");
		if (cv)
			Q_strncpyz(VM_POINTER(arg[1]), cv->string, VM_LONG(arg[2]));
		else
			Q_strncpyz(VM_POINTER(arg[1]), "", VM_LONG(arg[2]));
	}
	return 0;
}
static qintptr_t QVM_strcmp (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *a = VM_POINTER(arg[0]);
	char *b = VM_POINTER(arg[1]);
	return strcmp(a, b);
}
static qintptr_t QVM_strncmp (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *a = VM_POINTER(arg[0]);
	char *b = VM_POINTER(arg[1]);
	return strncmp(a, b, VM_LONG(arg[2]));
}
static qintptr_t QVM_stricmp (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *a = VM_POINTER(arg[0]);
	char *b = VM_POINTER(arg[1]);
	return stricmp(a, b);
}
static qintptr_t QVM_strnicmp (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *a = VM_POINTER(arg[0]);
	char *b = VM_POINTER(arg[1]);
	return strnicmp(a, b, VM_LONG(arg[2]));
}
static qintptr_t QVM_Find (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	edict_t *e = VM_POINTER(arg[0]);
	int ofs = (VM_LONG(arg[1]) - wasted_edict_t_size);
	char *match = VM_POINTER(arg[2]);
	char *field;
	int first = e?((char*)e - (char*)evars)/sv.world.edict_size:0;
	int i;
	qboolean stringishacky = sizeof(quintptr_t) != sizeof(string_t) && qvm_api_version >= 15 && !VM_NonNative(q1qvm);	//silly bullshit. Really, native gamecode should have its own implementation of this builtin or something, especially as its pretty much only ever used for classnames.
	if (!match)
		match = "";
	for (i = first+1; i < sv.world.num_edicts; i++)
	{
		e = q1qvmprogfuncs.edicttable[i];
		if (stringishacky)
			field = VM_POINTER(*(quintptr_t*)((char*)e->v + ofs));
		else
			field = VM_POINTER(*(string_t*)((char*)e->v + ofs));
		if (field == NULL)
		{
			if (*match == '\0')
				return ((char*)e->v - (char*)offset)-wasted_edict_t_size;
		}
		else
		{
			if (!strcmp(field, match))
				return ((char*)e->v - (char*)offset)-wasted_edict_t_size;
		}
	}
	return 0;
}
static qintptr_t QVM_ExecuteCmd (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	WrapQCBuiltin(PF_ExecuteCommand, offset, mask, arg, "");
	return 0;
}
static qintptr_t QVM_ConPrint (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	Con_Printf("%s", (char*)VM_POINTER(arg[0]));
	return 0;
}
static qintptr_t QVM_ReadCmd (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	extern char sv_redirected_buf[];
	extern redirect_t sv_redirected;
	extern int sv_redirectedlang;
	redirect_t old;
	int oldl;
	int spawncount = svs.spawncount;

	char *s = VM_POINTER(arg[0]);
	char *output = VM_POINTER(arg[1]);
	int outputlen = VM_LONG(arg[2]);

	if (VM_OOB(arg[1], arg[2]))
		return -1;

	Cbuf_Execute();	//FIXME: this code is flawed
	if (svs.spawncount != spawncount || sv.state < ss_loading)
		Host_EndGame("QVM_ReadCmd: Map changed before reading");
	Cbuf_AddText (s, RESTRICT_LOCAL);

	old = sv_redirected;
	oldl = sv_redirectedlang;
	if (old != RD_NONE)
		SV_EndRedirect();

	SV_BeginRedirect(RD_OBLIVION, TL_FindLanguage(""));
	Cbuf_Execute();
	if (svs.spawncount != spawncount || sv.state < ss_loading)
	{
		SV_EndRedirect();
		Host_EndGame("QVM_ReadCmd: Map changed after reading");
	}
	Q_strncpyz(output, sv_redirected_buf, outputlen);
	SV_EndRedirect();

	if (old != RD_NONE)
		SV_BeginRedirect(old, oldl);

Con_DPrintf("PF_readcmd: %s\n%s", s, output);

	return 0;
}



static void QVM_RedirectCmdCallback(struct frameendtasks_s *task)
{	//called at the end of the frame when there's no gamecode running
	host_client = svs.clients + task->ctxint;
	if (host_client->state >= cs_connected)
	{
		SV_BeginRedirect(RD_CLIENT, host_client->language);
		Cmd_ExecuteString(task->data, RESTRICT_INSECURE);
		SV_EndRedirect();
	}
}
static qintptr_t QVM_RedirectCmd (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	struct frameendtasks_s *task, **link;
	unsigned int entnum = ((char*)VM_POINTER(arg[0]) - (char*)evars) / sv.world.edict_size;
	const char *s = VM_POINTER(arg[1]);
	if (entnum < 1 || entnum > sv.allocated_client_slots)
		SV_Error ("QVM_RedirectCmd: Parm 0 not a client");

	task = Z_Malloc(sizeof(*task)+strlen(s));
	task->callback = QVM_RedirectCmdCallback;
	strcpy(task->data, s);
	task->ctxint = entnum-1;
	for(link = &svs.frameendtasks; *link; link = &(*link)->next)
		;	//add them on the end, so they're execed in order
	*link = task;
	return 0;
}
static qintptr_t QVM_Add_Bot (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *name = VM_POINTER(arg[0]);
	int bottom = VM_LONG(arg[1]);
	int top = VM_LONG(arg[2]);
	char *skin = VM_POINTER(arg[3]);

#if 1
	extern int nextuserid;
	int i;
	for (i = 0; i < sv.allocated_client_slots; i++)
	{
		client_t *cl = &svs.clients[i];
		if (!*cl->name && !cl->protocol && cl->state == cs_free)
		{
			cl->userid = ++nextuserid;
			cl->protocol = SCP_BAD;	//marker for bots
			cl->state = cs_spawned;
			cl->connection_started = realtime;
			cl->spawned = true;
			sv.spawned_client_slots++;
			cl->netchan.message.allowoverflow = true;
			cl->netchan.message.maxsize = 0;
			cl->datagram.allowoverflow = true;
			cl->datagram.maxsize = 0;

			cl->edict = EDICT_NUM_PB(sv.world.progs, i+1);

			InfoBuf_SetKey(&cl->userinfo, "name", name);
			InfoBuf_SetKey(&cl->userinfo, "topcolor", va("%i", top));
			InfoBuf_SetKey(&cl->userinfo, "bottomcolor", va("%i", bottom));
			InfoBuf_SetKey(&cl->userinfo, "skin", skin);
			InfoBuf_SetStarKey(&cl->userinfo, "*bot", "1");
			SV_ExtractFromUserinfo(cl, true);
			SV_SetUpClientEdict (cl, cl->edict);

			SV_FullClientUpdate(cl, NULL);
			Q1QVM_ClientConnect(cl);

			return cl->edict->entnum;
		}
	}
#else
	//FIXME: not implemented, always returns failure.
	//the other bot functions only ever work on bots anyway, so don't need to be implemented until this one is

	//return WrapQCBuiltin(PF_spawnclient, offset, mask, arg, "");
	Con_DPrintf("QVM_Add_Bot: not implemented\n");
#endif
	return 0;
}
static qintptr_t QVM_Remove_Bot (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	//kicks NOW. which generally makes it unsafe for players (calling from StartFrame should be okay).
	int entnum = VM_LONG(arg[0]);
	if (entnum >= 1 && entnum <= svs.allocated_client_slots)
	{
		client_t *cl = &svs.clients[entnum-1];
		SV_DropClient(cl);
	}

	return 0;
}
static qintptr_t QVM_SetBotCMD (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	//this just queues the command for later, even in mvdsv. ignore the msecs value because its basically pointless
	int edn		= VM_LONG(arg[0]);
	int msec	= VM_LONG(arg[1]);
	float angles_x = VM_FLOAT(arg[2]);
	float angles_y = VM_FLOAT(arg[3]);
	float angles_z = VM_FLOAT(arg[4]);
	int forwardmove = VM_LONG(arg[5]);
	int sidemove = VM_LONG(arg[6]);
	int upmove = VM_LONG(arg[7]);
	int buttons = VM_LONG(arg[8]);
	int impulse = VM_LONG(arg[9]);

	if (edn >= 1 && edn <= svs.allocated_client_slots)
	{
		client_t *cl = &svs.clients[edn-1];
		cl->lastcmd.msec = msec;
		cl->lastcmd.angles[0] = ANGLE2SHORT(angles_x);
		cl->lastcmd.angles[1] = ANGLE2SHORT(angles_y);
		cl->lastcmd.angles[2] = ANGLE2SHORT(angles_z);
		cl->lastcmd.forwardmove = forwardmove;
		cl->lastcmd.sidemove = sidemove;
		cl->lastcmd.upmove = upmove;
		cl->lastcmd.buttons = buttons;
		cl->lastcmd.impulse = impulse;
	}
	return 0;
}
static qintptr_t QVM_SetUserInfo (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int ent = VM_LONG(arg[0]);
	const char *key = VM_POINTER(arg[1]);
	const char *val = VM_POINTER(arg[2]);
	if (*key == '*' && (VM_LONG(arg[3])&1))
		return -1;	//denied!
	return PF_ForceInfoKey_Internal(ent, key, val, strlen(val));
}
static qintptr_t QVM_SetBotUserInfo (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int ent = VM_LONG(arg[0]);
	const char *key = VM_POINTER(arg[1]);
	const char *val = VM_POINTER(arg[2]);

	return PF_ForceInfoKey_Internal(ent, key, val,  strlen(val));
}
static qintptr_t QVM_MoveToGoal (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return World_MoveToGoal(&sv.world, (wedict_t*)Q1QVMPF_ProgsToEdict(svprogfuncs, pr_global_struct->self), VM_FLOAT(arg[0]));
}
static qintptr_t QVM_strftime (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *out = VM_POINTER(arg[0]);
	char *fmt = VM_POINTER(arg[2]);
	time_t curtime;
	struct tm *local;
	if (VM_OOB(arg[0], arg[1]) || !out)
		return -1;	//please don't corrupt me
	time(&curtime);
	curtime += VM_LONG(arg[3]);
	local = localtime(&curtime);
	return strftime(out, VM_LONG(arg[1]), fmt, local);
}
static qintptr_t QVM_Cmd_ArgS (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *c;
	c = Cmd_Args();
	if (VM_OOB(arg[0], arg[1]))
		return -1;
	Q_strncpyz(VM_POINTER(arg[0]), c, VM_LONG(arg[1]));
	return arg[0];
}
static qintptr_t QVM_Cmd_Tokenize (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *str = VM_POINTER(arg[0]);
	Cmd_TokenizeString(str, false, false);
	return Cmd_Argc();
}
static qintptr_t QVM_strlcpy (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *dst = VM_POINTER(arg[0]);
	char *src = VM_POINTER(arg[1]);
	if (VM_OOB(arg[0], arg[2]) || VM_LONG(arg[2]) < 1)
		return -1;
	else if (!src)
	{
		*dst = 0;
		return 0;
	}
	else
	{
		Q_strncpyz(dst, src, VM_LONG(arg[2]));
		return strlen(src);
	}
}
static qintptr_t QVM_strlcat (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *dst = VM_POINTER(arg[0]);
	char *src = VM_POINTER(arg[1]);
	if (VM_OOB(arg[0], arg[2]))
		return -1;
	Q_strncatz(dst, src, VM_LONG(arg[2]));
	//WARNING: no return value
	return 0;
}
static qintptr_t QVM_MakeVectors (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	AngleVectors(VM_POINTER(arg[0]), P_VEC(v_forward), P_VEC(v_right), P_VEC(v_up));
	return 0;
}
static qintptr_t QVM_NextClient (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int start = ((char*)VM_POINTER(arg[0]) - (char*)evars) / sv.world.edict_size;
	while (start < sv.allocated_client_slots)
	{
		if (svs.clients[start].state == cs_spawned)
			return (qintptr_t)(vevars + (start+1) * sv.world.edict_size);
		start++;
	}
	return 0;
}
static qintptr_t QVM_SetPause (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int pause = VM_LONG(arg[0]);
	if ((sv.paused&PAUSE_EXPLICIT) == (pause&PAUSE_EXPLICIT))
		return !!(sv.paused&PAUSE_EXPLICIT);	//nothing changed, ignore it.
	sv.paused = pause;
	sv.pausedstart = Sys_DoubleTime();
	return !!(sv.paused&PAUSE_EXPLICIT);
}

static qintptr_t QVM_VisibleTo (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int viewernum = VM_LONG(arg[0]);
	unsigned int first = VM_LONG(arg[1]);
	unsigned int count = VM_LONG(arg[2]);
	qbyte		 *results = VM_POINTER(arg[3]);
	unsigned int e, last = first + count;
	unsigned int ret = 0;

	memset(results, 0, count);	//assume the worst...
	if (viewernum < sv.world.num_edicts && !VM_OOB(arg[3], count) && first < last && last <= sv.world.num_edicts)
	{
		edict_t *viewer = q1qvmprogfuncs.edicttable[viewernum];
		edict_t *ed;
		static pvsbuffer_t pvs;	//bit of a leak. but only one allocation.
		vec3_t org;
		int areas[] = {2,viewer->pvsinfo.areanum, viewer->pvsinfo.areanum2};

		if (ED_ISFREE(viewer))
			return ret;

		//FIXME: make a FatPVS that uses a pvscache_t
		VectorAdd(viewer->v->origin, viewer->v->view_ofs, org);
		sv.world.worldmodel->funcs.FatPVS(sv.world.worldmodel, org, &pvs, false);
		for (e = first; e < last; e++)
		{
			ed = q1qvmprogfuncs.edicttable[e];
			if (ED_ISFREE(ed))
				continue;	//free ents can't be visible (should not be linked, but oh well)
			if (e >= 1 && e <= sv.allocated_client_slots && svs.clients[e - 1].state != cs_spawned)
				continue;	//player ents are weird, skip them for mods that do weird stuff.

			if (sv.world.worldmodel->funcs.EdictInFatPVS(sv.world.worldmodel, &ed->pvsinfo, pvs.buffer, areas))
			{	//one of the viewer's clusters can see the viewee.
				results[e - first] = true;
				ret+=1;
				break;
			}
		}
	}
	return ret;
}
static qintptr_t QVM_NotYetImplemented (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	SV_Error("Q1QVM: Trap not implemented\n");
	return 0;
}

static int QVM_FindExtField(char *fname)
{
	extentvars_t *xv = NULL;
#define comfieldfloat(name,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
#define comfieldint(name,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
#define comfieldvector(name,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
#define comfieldentity(name,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
#define comfieldstring(name,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
#define comfieldfunction(name, typestr,desc) if (!strcmp(fname, #name)) return ((int*)&xv->name - (int*)xv);
comextqcfields
svextqcfields
#undef comfieldfloat
#undef comfieldint
#undef comfieldvector
#undef comfieldentity
#undef comfieldstring
#undef comfieldfunction
	return -1;	//unsupported
}
static qintptr_t QVM_SetExtField (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int entnum = ((char*)VM_POINTER(arg[0]) - (char*)evars)/sv.world.edict_size;
	int i = QVM_FindExtField(VM_POINTER(arg[1]));
	int value = VM_LONG(arg[2]);

	if (i >= 0 && entnum < q1qvmprogfuncs.edicttable_length && q1qvmprogfuncs.edicttable[entnum])
	{
		((int*)q1qvmprogfuncs.edicttable[entnum]->xv)[i] = value;
		return value;
	}
	return 0;
}
static qintptr_t QVM_GetExtField (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int entnum = ((char*)VM_POINTER(arg[0]) - (char*)evars)/sv.world.edict_size;
	int i = QVM_FindExtField(VM_POINTER(arg[1]));

	if (i >= 0 && entnum < q1qvmprogfuncs.edicttable_length && q1qvmprogfuncs.edicttable[entnum])
	{
		return ((int*)q1qvmprogfuncs.edicttable[entnum]->xv)[i];
	}
	return 0;
}

#ifdef WEBCLIENT
static void QVM_uri_query_callback(struct dl_download *dl)
{
	void *cb_context = dl->user_ctx;
	int cb_entry = dl->user_float;
	int selfnum = dl->user_num;

	if (svs.gametype != GT_Q1QVM || svs.spawncount != dl->user_sequence)
		return;	//the world moved on.
	//fixme: pointers might not still be valid if the map changed.

	*sv.world.g.self = selfnum;
	if (dl->file)
	{
		size_t len = VFS_GETLEN(dl->file);
		char *buffer = malloc(len+1);
		buffer[len] = 0;
		VFS_READ(dl->file, buffer, len);
		Cmd_Args_Set(buffer, strlen(buffer));
		free(buffer);
	}
	else
		Cmd_Args_Set(NULL, 0);
	VM_Call(q1qvm, cb_entry, cb_context, dl->replycode, 0, 0, 0);
}

//bool uri_get(char *uri, int cb_entry, void *cb_ctx, char *mime, void *data, unsigned datasize)
static qintptr_t QVM_uri_query (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	struct dl_download *dl;

	const unsigned char *url = VM_POINTER(arg[0]);
	int cb_entry = VM_LONG(arg[1]);
	void *cb_context = VM_POINTER(arg[2]);
	const char *mimetype = VM_POINTER(arg[3]);
	const char *data = VM_POINTER(arg[4]);
	size_t datasize = VM_LONG(arg[5]);
	extern cvar_t pr_enable_uriget;

	if (!pr_enable_uriget.ival)
	{
		Con_Printf("QVM_uri_query(\"%s\"): %s disabled\n", url, pr_enable_uriget.name);
		return 0;
	}

	if (mimetype && *mimetype)
	{
		VALIDATEPOINTER(arg[4],datasize);
		Con_DPrintf("QVM_uri_query(%s)\n", url);
		dl = HTTP_CL_Put(url, mimetype, data, datasize, QVM_uri_query_callback);
	}
	else
	{
		Con_DPrintf("QVM_uri_query(%s)\n", url);
		dl = HTTP_CL_Get(url, NULL, QVM_uri_query_callback);
	}
	if (dl)
	{
		dl->user_ctx = cb_context;
		dl->user_float = cb_entry;
		dl->user_num = *sv.world.g.self;
		dl->user_sequence = svs.spawncount;
		dl->isquery = true;
		return 1;
	}
	else
		return 0;
}
#endif

void QCBUILTIN PF_sv_trailparticles(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_sv_pointparticles(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_sv_particleeffectnum(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
static qintptr_t QVM_particleeffectnum (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int i = WrapQCBuiltin(PF_sv_particleeffectnum, offset, mask, arg, "s");
	return VM_FLOAT(i);
}
static qintptr_t QVM_trailparticles (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return WrapQCBuiltin(PF_sv_trailparticles, offset, mask, arg, "invv");
}
static qintptr_t QVM_pointparticles (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	return WrapQCBuiltin(PF_sv_pointparticles, offset, mask, arg, "ivvi");
}

static qintptr_t QVM_clientstat (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int num = VM_LONG(arg[0]);
	int type = VM_LONG(arg[1]);
	int fieldofs = VM_LONG(arg[2]);


//	SV_QCStatEval(type, "", &cache, NULL, num);

	SV_QCStatFieldIdx(type, fieldofs, num);
	return 0;
}
static qintptr_t QVM_pointerstat (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	int num = VM_LONG(arg[0]);
	int type = VM_LONG(arg[1]);
	void *ptr = VM_POINTER(arg[2]);
	SV_QCStatPtr(type, ptr, num);
	return 0;
}

//void(entity e, vector flags, entity target) setsendneeded
static qintptr_t QVM_SetSendNeeded(void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int subject = VM_LONG(arg[0]);
	quint64_t fl = arg[1] << SENDFLAGS_SHIFT;
	unsigned int to = VM_LONG(arg[2]);
	if (!to)
	{	//broadcast
		for (to = 0; to < sv.allocated_client_slots; to++)
			if (svs.clients[to].pendingcsqcbits && subject < svs.clients[to].max_net_ents)
				svs.clients[to].pendingcsqcbits[subject] |= fl;
	}
	else
	{
		to--;
		if (to >= sv.allocated_client_slots || !svs.clients[to].pendingcsqcbits || subject >= svs.clients[to].max_net_ents)
			;	//some kind of error.
		else
			svs.clients[to].pendingcsqcbits[subject] |= fl;
	}
	return 0;
}

static qintptr_t QVM_VisibleTo_FTE (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	unsigned int a0 = VM_LONG(arg[0]);
	unsigned int a1 = VM_LONG(arg[1]);
	if (a0 < sv.world.num_edicts && a1 < sv.world.num_edicts)
	{
		pvscache_t *viewer = &q1qvmprogfuncs.edicttable[a0]->pvsinfo;
		pvscache_t *viewee = &q1qvmprogfuncs.edicttable[a1]->pvsinfo;
		if (viewer->num_leafs && viewee->num_leafs)
		{
			unsigned int i;
			int areas[] = {2,viewer->areanum, viewer->areanum2};
			for (i = 0; i < viewer->num_leafs; i++)
			{
				qbyte *pvs = sv.world.worldmodel->funcs.ClusterPVS(sv.world.worldmodel, viewer->leafnums[i], NULL, PVM_FAST);
				if (sv.world.worldmodel->funcs.EdictInFatPVS(sv.world.worldmodel, viewee, pvs, areas))
					return true;	//viewer can see viewee
			}
		}
	}
	return 0;
}

static qintptr_t QVM_Map_Extension (void *offset, quintptr_t mask, const qintptr_t *arg);

typedef qintptr_t (*traps_t) (void *offset, quintptr_t mask, const qintptr_t *arg);
traps_t bitraps[G_MAX] =
{
	QVM_GetAPIVersion,
	QVM_DPrint,
	QVM_Error,
	QVM_GetEntityToken,
	QVM_Spawn_Ent,
	QVM_Remove_Ent,
	QVM_Precache_Sound,
	QVM_Precache_Model,
	QVM_LightStyle,
	QVM_SetOrigin,
	QVM_SetSize,			//10
	QVM_SetModel,
	QVM_BPrint,
	QVM_SPrint,
	QVM_CenterPrint,
	QVM_AmbientSound,		//15
	QVM_Sound,
	QVM_TraceLine,
	QVM_CheckClient,
	QVM_StuffCmd,
	QVM_LocalCmd,			//20
	QVM_CVar,
	QVM_CVar_Set,
	QVM_FindRadius,
	QVM_WalkMove,
	QVM_DropToFloor,		//25
	QVM_CheckBottom,
	QVM_PointContents,
	QVM_NextEnt,
	QVM_Aim,
	QVM_MakeStatic,		//30
	QVM_SetSpawnParams,
	QVM_ChangeLevel,
	QVM_LogFrag,
	QVM_GetInfoKey,
	QVM_Multicast,		//35
	QVM_DisableUpdates,
	QVM_WriteByte,
	QVM_WriteChar,
	QVM_WriteShort,
	QVM_WriteLong,		//40
	QVM_WriteAngle,
	QVM_WriteCoord,
	QVM_WriteString,
	QVM_WriteEntity,
	QVM_FlushSignon,		//45
	QVM_memset,
	QVM_memcpy,
	QVM_strncpy,
	QVM_sin,
	QVM_cos,				//50
	QVM_atan2,
	QVM_sqrt,
	QVM_floor,
	QVM_ceil,
	QVM_acos,				//55
	QVM_Cmd_ArgC,
	QVM_Cmd_ArgV,
	QVM_TraceBox,			//was G_TraceCapsule
	QVM_FS_OpenFile,
	QVM_FS_CloseFile,		//60
	QVM_FS_ReadFile,
	QVM_FS_WriteFile,
	QVM_FS_SeekFile,
	QVM_FS_TellFile,
	QVM_FS_GetFileList,	//65
	QVM_CVar_Set_Float,
	QVM_CVar_String,
	QVM_Map_Extension,
	QVM_strcmp,
	QVM_strncmp,			//70
	QVM_stricmp,
	QVM_strnicmp,
	QVM_Find,
	QVM_ExecuteCmd,
	QVM_ConPrint,			//75
	QVM_ReadCmd,
	QVM_RedirectCmd,
	QVM_Add_Bot,
	QVM_Remove_Bot,
	QVM_SetBotUserInfo,	//80
	QVM_SetBotCMD,

	QVM_strftime,
	QVM_Cmd_ArgS,
	QVM_Cmd_Tokenize,
	QVM_strlcpy,		//85
	QVM_strlcat,
	QVM_MakeVectors,
	QVM_NextClient,

	QVM_Precache_VWep_Model,
	QVM_SetPause,
	QVM_SetUserInfo,
	QVM_MoveToGoal,
	QVM_VisibleTo,
};

struct
{
	char *extname;
	traps_t trap;
} qvmextensions[] =
{
	{"SetExtField",			QVM_SetExtField},
	{"GetExtField",			QVM_GetExtField},
	{"ChangeLevelHub",		QVM_ChangeLevelHub},	//with start spot
#ifdef WEBCLIENT
	{"URI_Query",			QVM_uri_query},
#endif
	{"particleeffectnum",	QVM_particleeffectnum},
	{"trailparticles",		QVM_trailparticles},
	{"pointparticles",		QVM_pointparticles},
	{"clientstat",			QVM_clientstat},	//csqc extension
	{"pointerstat",			QVM_pointerstat},	//csqc extension
	{"setsendneeded",		QVM_SetSendNeeded},		//csqc extension
	{"VisibleTo",			QVM_VisibleTo_FTE},		//alternative to mvdsv's visclients hack. redundant now. FIXME: Remove.

	//sql?
	//model querying?
	//skeletal objects/tags?
	//heightmap / brush editing?
	//csqc ents
	{NULL, NULL}
};

traps_t traps[512];

static qintptr_t QVM_Map_Extension (void *offset, quintptr_t mask, const qintptr_t *arg)
{
	char *extname = VM_POINTER(arg[0]);
	unsigned int slot = VM_LONG(arg[1]);
	int i;

	if (slot >= countof(traps))
		return -2;	//invalid slot.

	if (!extname)
	{	//special handling for vauge compat with mvdsv, for testing how many 'known' builtins are implemented.
		if (slot < G_MAX)
			return -2;
		return -1;
	}

	//find the extension and map it to the slot if found.
	for (i = 0; qvmextensions[i].extname; i++)
	{
		if (!Q_strcasecmp(extname, qvmextensions[i].extname))
		{
			traps[slot] = qvmextensions[i].trap;
			return slot;
		}
	}
	return -1;	//extension not known
}

//============== general Quake services ==================
static int syscallqvm (void *offset, quintptr_t mask, int fn, const int *arg)
{
	if (sizeof(int) == sizeof(qintptr_t))
	{	//should allow the slow copy below to be optimised out
		if (fn >= countof(traps))
			return QVM_NotYetImplemented(offset, mask, (qintptr_t*)arg);
		return traps[fn](offset, mask, (qintptr_t*)arg);
	}
	else
	{
		qintptr_t args[13];
		int i;
		for (i = 0; i < countof(args); i++)
			args[i] = arg[i];
		if (fn >= countof(traps))
			return QVM_NotYetImplemented(offset, mask, args);
		return traps[fn](offset, mask, args);
	}
}

static qintptr_t EXPORT_FN syscallnative (qintptr_t arg, ...)
{
	qintptr_t args[13];
	va_list argptr;

	va_start(argptr, arg);
	args[0]=va_arg(argptr, qintptr_t);
	args[1]=va_arg(argptr, qintptr_t);
	args[2]=va_arg(argptr, qintptr_t);
	args[3]=va_arg(argptr, qintptr_t);
	args[4]=va_arg(argptr, qintptr_t);
	args[5]=va_arg(argptr, qintptr_t);
	args[6]=va_arg(argptr, qintptr_t);
	args[7]=va_arg(argptr, qintptr_t);
	args[8]=va_arg(argptr, qintptr_t);
	args[9]=va_arg(argptr, qintptr_t);
	args[10]=va_arg(argptr, qintptr_t);
	args[11]=va_arg(argptr, qintptr_t);
	args[12]=va_arg(argptr, qintptr_t);
	va_end(argptr);

	if (arg >= countof(traps))
		return QVM_NotYetImplemented(NULL, ~(quintptr_t)0, args);
	return traps[arg](NULL, ~(quintptr_t)0, args);
}

void Q1QVM_Shutdown(qboolean notifygame)
{
	int i;
	if (q1qvm)
	{
		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			if (svs.clients[i].name)
				Q_strncpyz(svs.clients[i].namebuf, svs.clients[i].name, sizeof(svs.clients[i].namebuf));
			svs.clients[i].name = svs.clients[i].namebuf;
		}
		if (notifygame && gvars)
			VM_Call(q1qvm, GAME_SHUTDOWN, 0, 0, 0);
		VM_Destroy(q1qvm);
		q1qvm = NULL;
		QVM_FS_CloseFileAll();
		if (svprogfuncs == &q1qvmprogfuncs)
			sv.world.progs = svprogfuncs = NULL;
		Z_FreeTags(VMFSID_Q1QVM);
		if (q1qvmprogfuncs.edicttable)
		{
			Z_Free(q1qvmprogfuncs.edicttable);
			q1qvmprogfuncs.edicttable = NULL;
		}
		vevars = 0;
	}
}

static void QDECL Q1QVM_Get_FrameState(world_t *w, wedict_t *ent, framestate_t *fstate)
{
	memset(fstate, 0, sizeof(*fstate));
	fstate->g[FS_REG].frame[0] = ent->v->frame;
	fstate->g[FS_REG].frametime[0] = ent->xv->frame1time;
	fstate->g[FS_REG].lerpweight[0] = 1;
	fstate->g[FS_REG].endbone = 0x7fffffff;

	fstate->g[FST_BASE].frame[0] = ent->xv->baseframe;
	fstate->g[FST_BASE].frametime[0] = ent->xv->/*base*/frame1time;
	fstate->g[FST_BASE].lerpweight[0] = 1;
	fstate->g[FST_BASE].endbone = ent->xv->basebone;

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	if (ent->xv->skeletonindex)
		skel_lookup(w, ent->xv->skeletonindex, fstate);
#endif
}

static void QDECL Q1QVM_Event_Touch(world_t *w, wedict_t *s, wedict_t *o, trace_t *trace)
{
	int oself = pr_global_struct->self;
	int oother = pr_global_struct->other;

	pr_global_struct->self = EDICT_TO_PROG(w->progs, s);
	pr_global_struct->other = EDICT_TO_PROG(w->progs, o);
	pr_global_struct->time = w->physicstime;
	VM_Call(q1qvm, GAME_EDICT_TOUCH, 0, 0, 0);

	pr_global_struct->self = oself;
	pr_global_struct->other = oother;
}

static void QDECL Q1QVM_Event_Think(world_t *w, wedict_t *s)
{
	pr_global_struct->self = EDICT_TO_PROG(w->progs, s);
	pr_global_struct->other = EDICT_TO_PROG(w->progs, w->edicts);
	VM_Call(q1qvm, GAME_EDICT_THINK, 0, 0, 0);
}

static qboolean QDECL Q1QVM_Event_ContentsTransition(world_t *w, wedict_t *ent, int oldwatertype, int newwatertype)
{
	return false;	//always do legacy behaviour
}

qboolean PR_LoadQ1QVM(void)
{
	static pint_t writable_int;
	static pvec_t writable;
	static pvec_t dimensionsend = 255;
	static pvec_t dimensiondefault = 255;
	static pvec_t physics_mode = 2;
	static pvec3_t defaultgravity = {0,0,-1};
	int i;
	gameDataPrivate_t gd;
	gameDataN_t *gdn;
	gameData32_t *gd32;
	qintptr_t ret;
	qintptr_t limit;
	extern cvar_t	pr_maxedicts;

	const char *fname = pr_ssqc_progs.string;
	if (!*fname)
		fname = "qwprogs";

	Q1QVM_Shutdown(true);

	q1qvm = VM_Create(fname, com_gamedirnativecode.ival?syscallnative:NULL, fname, syscallqvm);
	if (!q1qvm)
		q1qvm = VM_Create(fname, syscallnative, fname, NULL);
	if (!q1qvm)
	{
		if (!com_gamedirnativecode.ival && COM_FCheckExists(va("%s"ARCH_DL_POSTFIX, fname)))
			Con_Printf(CON_WARNING"%s"ARCH_DL_POSTFIX" exists, but is blocked from loading due to known bugs in other engines. If this is from a safe source then either ^aset com_gamedirnativecode 1^a or rename to eg %s%s_%s"ARCH_DL_POSTFIX"\n", fname, ((host_parms.binarydir && *host_parms.binarydir)?host_parms.binarydir:host_parms.basedir), fname, FS_GetGamedir(false));
		if (svprogfuncs == &q1qvmprogfuncs)
			sv.world.progs = svprogfuncs = NULL;
		return false;
	}

	for(i = 0; i < G_MAX; i++)
		traps[i] = bitraps[i];
	for(; i < countof(traps); i++)
		traps[i] = QVM_NotYetImplemented;


	memset(&fofs, 0, sizeof(fofs));

	progstype = PROG_QW;


	svprogfuncs = &q1qvmprogfuncs;


//	q1qvmprogfuncs.AddString = Q1QVMPF_AddString;	//using this breaks 64bit support, and is a 'bad plan' elsewhere too,
	q1qvmprogfuncs.EdictNum = Q1QVMPF_EdictNum;
	q1qvmprogfuncs.NumForEdict = Q1QVMPF_NumForEdict;
	q1qvmprogfuncs.EdictToProgs = Q1QVMPF_EdictToProgs;
	q1qvmprogfuncs.ProgsToEdict = Q1QVMPF_ProgsToEdict;
	q1qvmprogfuncs.EntAlloc = Q1QVMPF_EntAlloc;
	q1qvmprogfuncs.EntFree = Q1QVMPF_EntRemove;
	q1qvmprogfuncs.FindGlobal = Q1QVMPF_FindGlobal;
	q1qvmprogfuncs.load_ents = Q1QVMPF_LoadEnts;
	q1qvmprogfuncs.globals = Q1QVMPF_Globals;
	q1qvmprogfuncs.GetEdictFieldValue = Q1QVMPF_GetEdictFieldValue;
	q1qvmprogfuncs.QueryField = Q1QVMPF_QueryField;
	q1qvmprogfuncs.StringToProgs = Q1QVMPF_StringToProgs;
	q1qvmprogfuncs.StringToNative = Q1QVMPF_StringToNative;
	q1qvmprogfuncs.SetStringField = Q1QVMPF_SetStringField;
	q1qvmprogfuncs.EntClear = Q1QVMPF_ClearEdict;

	sv.world.Event_Touch = Q1QVM_Event_Touch;
	sv.world.Event_Think = Q1QVM_Event_Think;
	sv.world.Event_Sound = SVQ1_StartSound;
	sv.world.Event_ContentsTransition = Q1QVM_Event_ContentsTransition;
	sv.world.Get_CModel = SVPR_GetCModel;
	sv.world.Get_FrameState = Q1QVM_Get_FrameState;

	sv.world.num_edicts = 0;	//we're not ready for most of the builtins yet
	sv.world.max_edicts = 0;	//so clear these out, just in case
	sv.world.edict_size = 0;	//if we get a division by zero, then at least its a safe crash

	q1qvmprogfuncs.edicttable = NULL;

	q1qvmprogfuncs.stringtable = VM_MemoryBase(q1qvm);

	qvm_api_version = GAME_API_VERSION;

	ret = VM_Call(q1qvm, GAME_INIT, (qintptr_t)(sv.time*1000), rand(), 0, 0, 0);
	if (!ret)
	{
		Q1QVM_Shutdown(false);
		return false;
	}

	if (VM_NonNative(q1qvm))
	{	//when non native, this can only be a 32bit qvm in a 64bit server.
		gd32 = (gameData32_t*)((char*)VM_MemoryBase(q1qvm) + ret);	//qvm is 32bit

		gd.APIversion = gd32->APIversion;
		gd.sizeofent = gd32->sizeofent;

		gd.ents = gd32->ents;
		gd.global = gd32->global;
		gd.fields = gd32->fields;

		if (qvm_api_version >= 14)
			gd.maxedicts = gd32->maxentities;
		else
			gd.maxedicts = MAX_Q1QVM_EDICTS;
	}
	else
	{
		gdn = (gameDataN_t*)((char*)VM_MemoryBase(q1qvm) + ret);
		gd.APIversion = gdn->APIversion;
		gd.sizeofent = gdn->sizeofent;

		gd.ents = gdn->ents;
		gd.global = gdn->global;
		gd.fields = gdn->fields;

		if (qvm_api_version >= 14)
			gd.maxedicts = gdn->maxentities;
		else
			gd.maxedicts = MAX_Q1QVM_EDICTS;
	}
	gd.maxedicts = bound(1, pr_maxedicts.ival, gd.maxedicts);
	gd.maxedicts = bound(1, gd.maxedicts, MAX_EDICTS);

	qvm_api_version = gd.APIversion;
	if (!(GAME_API_VERSION_MIN <= qvm_api_version && qvm_api_version <= GAME_API_VERSION))
	{
		Con_Printf("QVM-API version %i not supported\n", qvm_api_version);
		Q1QVM_Shutdown(false);
		return false;
	}

	if (qvm_api_version >= 16)
	{	//version 16 finally removed the last remnant of the server's state from the qvm.
		wasted_edict_t_size = 0;
	}
	else if (qvm_api_version >= 13)
	{
		//in version 13, the actual edict_t struct is gone, and there's a pointer to it in its place (which is unusable, and changes depending on modes).
		wasted_edict_t_size = (VM_NonNative(q1qvm)?sizeof(int):sizeof(void*));
	}
	else
	{
		//fte/qclib has split edict_t and entvars_t.
		//older versions of the qvm api has them in one lump
		//so we need to bias the mod's entvars_t offsets a little.
		wasted_edict_t_size = 114;
	}

	sv.world.num_edicts = 1;
	sv.world.max_edicts = bound(64, gd.maxedicts, MAX_EDICTS);
	q1qvmprogfuncs.edicttable = Z_Malloc(sizeof(*q1qvmprogfuncs.edicttable) * sv.world.max_edicts);
	q1qvmprogfuncs.edicttable_length = sv.world.max_edicts;

	limit = VM_MemoryMask(q1qvm);
	if (gd.sizeofent > 0xffffffff / gd.maxedicts)
		gd.sizeofent = 0xffffffff / gd.maxedicts;
	if ((quintptr_t)gd.ents+(gd.sizeofent*gd.maxedicts) < (quintptr_t)gd.ents || (quintptr_t)gd.ents > (quintptr_t)limit)
		gd.ents = 0;
	if ((quintptr_t)(gd.global+1) < (quintptr_t)gd.global || (quintptr_t)gd.global > (quintptr_t)limit)
		gd.global = 0;
	if (/*(quintptr_t)gd.fields < (quintptr_t)gd.fields ||*/ (quintptr_t)gd.fields > limit)
		gd.fields = 0;

	sv.world.edict_size = gd.sizeofent;
	vevars = gd.ents;
	evars = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, vevars);
	gvars = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, gd.global);

	if (!evars || !gvars)
	{
		Q1QVM_Shutdown(false);
		return false;
	}

//WARNING: global is not remapped yet...
//This code is written evilly, but works well enough
#define globalint(required, name) pr_global_ptrs->name = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, (qintptr_t)&gvars->name - (qintptr_t)q1qvmprogfuncs.stringtable)	//the logic of this is somewhat crazy
#define globalfloat(required, name) pr_global_ptrs->name = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, (qintptr_t)&gvars->name - (qintptr_t)q1qvmprogfuncs.stringtable)
#define globalstring(required, name) pr_global_ptrs->name = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, (qintptr_t)&gvars->name - (qintptr_t)q1qvmprogfuncs.stringtable)
#define globalvec(required, name) pr_global_ptrs->name = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, (qintptr_t)&gvars->name - (qintptr_t)q1qvmprogfuncs.stringtable)
#define globalfunc(required, name) pr_global_ptrs->name = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, (qintptr_t)&gvars->name - (qintptr_t)q1qvmprogfuncs.stringtable)
#define globalnull(required, name) pr_global_ptrs->name = NULL
	globalint		(true, self);	//we need the qw ones, but any in standard quake and not quakeworld, we don't really care about.
	globalint		(true, other);
	globalint		(true, world);
	globalfloat		(true, time);
	globalfloat		(true, frametime);
	globalint		(false, newmis);	//not always in nq.
	globalfloat		(false, force_retouch);
	globalstring	(true, mapname);
	globalnull		(false, deathmatch);
	globalnull		(false, coop);
	globalnull		(false, teamplay);
	globalfloat		(true, serverflags);
	globalfloat		(true, total_secrets);
	globalfloat		(true, total_monsters);
	globalfloat		(true, found_secrets);
	globalfloat		(true, killed_monsters);
	globalvec		(true, v_forward);
	globalvec		(true, v_up);
	globalvec		(true, v_right);
	globalfloat		(true, trace_allsolid);
	globalfloat		(true, trace_startsolid);
	globalfloat		(true, trace_fraction);
	globalvec		(true, trace_endpos);
	globalvec		(true, trace_plane_normal);
	globalfloat		(true, trace_plane_dist);
	globalint		(true, trace_ent);
	globalfloat		(true, trace_inopen);
	globalfloat		(true, trace_inwater);
	globalnull		(false, trace_endcontentsf);
	globalnull		(false, trace_endcontentsi);
	globalnull		(false, trace_surfaceflagsf);
	globalnull		(false, trace_surfaceflagsi);
	globalnull		(false, cycle_wrapped);
	globalint		(false, msg_entity);
	globalfunc		(false, main);
	globalfunc		(true, StartFrame);
	globalfunc		(true, PlayerPreThink);
	globalfunc		(true, PlayerPostThink);
	globalfunc		(true, ClientKill);
	globalfunc		(true, ClientConnect);
	globalfunc		(true, PutClientInServer);
	globalfunc		(true, ClientDisconnect);
	globalfunc		(false, SetNewParms);
	globalfunc		(false, SetChangeParms);

	pr_global_ptrs->trace_surfaceflagsf = &writable;
	pr_global_ptrs->trace_surfaceflagsi = &writable_int;
	pr_global_ptrs->trace_endcontentsf = &writable;
	pr_global_ptrs->trace_endcontentsi = &writable_int;
	pr_global_ptrs->dimension_default = &dimensiondefault;
	pr_global_ptrs->dimension_send = &dimensionsend;
	pr_global_ptrs->physics_mode = &physics_mode;
	pr_global_ptrs->trace_brush_id = &writable_int;
	pr_global_ptrs->trace_brush_faceid = &writable_int;
	pr_global_ptrs->trace_surface_id = &writable_int;
	pr_global_ptrs->trace_bone_id = &writable_int;
	pr_global_ptrs->trace_triangle_id = &writable_int;
	pr_global_ptrs->global_gravitydir = &defaultgravity;

//	ensureglobal(input_timelength, input_timelength_default);
//	ensureglobal(input_impulse, input_impulse_default);
//	ensureglobal(input_angles, input_angles_default);
//	ensureglobal(input_movevalues, input_movevalues_default);
//	ensureglobal(input_buttons, input_buttons_default);

	dimensionsend = dimensiondefault = 255;
	for (i = 0; i < 16; i++)
		pr_global_ptrs->spawnparamglobals[i] = (float*)(&gvars->parm1 + i);
	for (; i < NUM_SPAWN_PARMS; i++)
		pr_global_ptrs->spawnparamglobals[i] = NULL;
	pr_global_ptrs->parm_string = NULL;

#define emufield(n,t) if (field[i].type == t && !strcmp(#n, fname)) {fofs.n = (field[i].ofs - wasted_edict_t_size)/sizeof(float); continue;}
	if (VM_NonNative(q1qvm))
	{
		field32_t *field = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, gd.fields);
		if (field)
		for (i = 0; field[i].name; i++)
		{
			const char *fname = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, field[i].name);
			emufields
			Con_DPrintf("Extension field %s is not supported\n", fname);
		}
	}
	else
	{
		fieldN_t *field = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, gd.fields);
		if (field)
		for (i = 0; field[i].name; i++)
		{
			const char *fname = Q1QVMPF_PointerToNative(&q1qvmprogfuncs, field[i].name);
			emufields
			Con_DPrintf("Extension field %s is not supported\n", fname);
		}
	}
#undef emufield


	sv.world.progs = &q1qvmprogfuncs;
	sv.world.edicts = (wedict_t*)Q1QVMPF_EdictNum(svprogfuncs, 0);
	sv.world.usesolidcorpse = true;

	if (qvm_api_version >= 15)
	{
		int e;
		for (e = 0; e <= 32; e++)
		{
			pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, Q1QVMPF_EdictNum(svprogfuncs, e));
			VM_Call(q1qvm, GAME_CLEAR_EDICT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
		pr_global_struct->self = 0;

		if (qvm_api_version == 15)
			Q1QVMPF_SetStringGlobal(sv.world.progs, &gvars->mapname, svs.name, MAPNAME_LEN);
	}
	else
	{
		if ((quintptr_t)gvars->mapname && (quintptr_t)gvars->mapname+MAPNAME_LEN < VM_MemoryMask(q1qvm))
			Q_strncpyz((char*)VM_MemoryBase(q1qvm) + gvars->mapname, svs.name, MAPNAME_LEN);
		else
			gvars->mapname = Q1QVMPF_StringToProgs(sv.world.progs, svs.name);
	}

	PR_SV_FillWorldGlobals(&sv.world);
	return true;
}




void Q1QVM_ClientConnect(client_t *cl)
{
	if (qvm_api_version >= 16)
	{
		Q_strncpyz(cl->namebuf, cl->name, sizeof(cl->namebuf));
		cl->name = cl->namebuf;
	}
	else if (qvm_api_version >= 15 && !VM_NonNative(q1qvm))
	{
		Q_strncpyz(cl->namebuf, cl->name, sizeof(cl->namebuf));
		Q1QVMPF_SetStringField(sv.world.progs, cl->edict, &cl->edict->v->netname, cl->namebuf, true);
	}
	else if (cl->edict->v->netname)
	{
		char *name;
		char *base = VM_MemoryBase(q1qvm);
		quintptr_t mask = VM_MemoryMask(q1qvm);
		name = (char*)Q1QVMPF_StringToNative(svprogfuncs, cl->edict->v->netname);
		if (cl->name > base && cl->name < base+mask)
		{
			Q_strncpyz(cl->namebuf, name, sizeof(cl->namebuf));
			strcpy(name, cl->namebuf);
			cl->name = name;	//so the gamecode can do changes if it wants.
		}
		else
			Con_Printf("WARNING: Mod provided no netname buffer. Player names will not be set properly.\n");
	}
	else if (!VM_NonNative(q1qvm))
	{
		Q_strncpyz(cl->namebuf, cl->name, sizeof(cl->namebuf));
		cl->name = cl->namebuf;
		cl->edict->v->netname = Q1QVMPF_StringToProgs(svprogfuncs, cl->namebuf);

//		Con_DPrintf("WARNING: Mod provided no netname buffer and will not function correctly when compiled as a qvm.\n");
	}
	else
		Con_Printf("WARNING: Mod provided no netname buffer. Player names will not be set properly.\n");

	if (fofs.gravity)
		((float*)cl->edict->v)[fofs.gravity] = cl->edict->xv->gravity;
	if (fofs.maxspeed)
		((float*)cl->edict->v)[fofs.maxspeed] = cl->edict->xv->maxspeed;
	if (fofs.isBot)
		((float*)cl->edict->v)[fofs.isBot] = !cl->protocol;

	// call the spawn function
	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_CLIENT_CONNECT, cl->spectator, 0, 0, 0);

	// actually spawn the player
	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_PUT_CLIENT_IN_SERVER, cl->spectator, 0, 0, 0);
}

qboolean Q1QVM_GameConsoleCommand(void)
{
	int oldself, oldother;
	if (!q1qvm)
		return false;

	//FIXME: if an rcon command from someone on the server, mvdsv sets self to match the ip of that player
	//this is not required (broken by proxies anyway) but is a nice handy feature

	pr_global_struct->time = sv.world.physicstime;
	oldself = pr_global_struct->self;	//these are usually useless
	oldother = pr_global_struct->other;	//but its possible that someone makes a mod that depends on the 'mod' command working via redirectcmd+co
						//this at least matches mvdsv
	pr_global_struct->self = 0;
	pr_global_struct->other = 0;

	VM_Call(q1qvm, GAME_CONSOLE_COMMAND, 0, 0, 0);	//mod uses Cmd_Argv+co to get args

	pr_global_struct->self = oldself;
	pr_global_struct->other = oldother;
	return true;
}

qboolean Q1QVM_ClientSay(edict_t *player, qboolean team)
{
	qboolean washandled;
	if (!q1qvm)
		return false;

	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, player);
	washandled = VM_Call(q1qvm, GAME_CLIENT_SAY, team, 0, 0, 0);

	return washandled;
}

qboolean Q1QVM_UserInfoChanged(edict_t *player, qboolean after)
{	//mod will use G_CMD_ARGV to get argv1+argv2 to read the info that is changing.
	if (!q1qvm)
		return false;

	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, player);
	return VM_Call(q1qvm, GAME_CLIENT_USERINFO_CHANGED, after, 0, 0);
}

void Q1QVM_PlayerPreThink(void)
{
	if (fofs.movement)
	{
		sv_player->xv->movement[0] = ((float*)sv_player->v)[fofs.movement+0];
		sv_player->xv->movement[1] = ((float*)sv_player->v)[fofs.movement+1];
		sv_player->xv->movement[2] = ((float*)sv_player->v)[fofs.movement+2];
	}
	if (fofs.gravity)
		sv_player->xv->gravity = ((float*)sv_player->v)[fofs.gravity];
	if (fofs.maxspeed)
		sv_player->xv->maxspeed = ((float*)sv_player->v)[fofs.maxspeed];

	VM_Call(q1qvm, GAME_CLIENT_PRETHINK, host_client->spectator, 0, 0, 0);
}

void Q1QVM_RunPlayerThink(void)
{
	VM_Call(q1qvm, GAME_EDICT_THINK, 0, 0, 0);
	VM_Call(q1qvm, GAME_CLIENT_THINK, host_client->spectator, 0, 0, 0);
}

void Q1QVM_PostThink(void)
{
	VM_Call(q1qvm, GAME_CLIENT_POSTTHINK, host_client->spectator, 0, 0, 0);

	if (fofs.vw_index)
		sv_player->xv->vw_index = ((float*)sv_player->v)[fofs.vw_index];
	if (fofs.items2)
		sv_player->xv->items2 = ((float*)sv_player->v)[fofs.items2];
	if (fofs.trackent)
		host_client->viewent = ((int*)sv_player->v)[fofs.trackent];
	if (fofs.hideplayers)
		host_client->hideplayers = ((int*)sv_player->v)[fofs.hideplayers];
	if (fofs.hideentity)
		host_client->hideentity = ((int*)sv_player->v)[fofs.hideentity];
}

void Q1QVM_StartFrame(qboolean botsarespecialsnowflakes)
{
	if (botsarespecialsnowflakes && qvm_api_version < 15)
		return; //this stupidity brought to you with api 15!
	VM_Call(q1qvm, GAME_START_FRAME, (qintptr_t)(sv.time*1000), botsarespecialsnowflakes, 0, 0);
}

qboolean Q1QVM_SendEntity(quint64_t sendflags)
{
	return VM_Call(q1qvm, GAME_EDICT_CSQCSEND, sendflags, 0, 0, 0) > 0;
}

void Q1QVM_Blocked(void)
{
	VM_Call(q1qvm, GAME_EDICT_BLOCKED, 0, 0, 0);
}

void Q1QVM_SetNewParms(void)
{
	VM_Call(q1qvm, GAME_SETNEWPARMS, 0, 0, 0);
}

void Q1QVM_SetChangeParms(void)
{
	VM_Call(q1qvm, GAME_SETCHANGEPARMS, 0, 0, 0);
}

qboolean Q1QVM_ClientCommand(void)
{
	return VM_Call(q1qvm, GAME_CLIENT_COMMAND, 0, 0, 0);
}

void Q1QVM_GameCodePausedTic(float pausedduration)
{
	VM_Call(q1qvm, GAME_PAUSED_TIC, (qintptr_t)(pausedduration*1000), 0, 0, 0);
}

void Q1QVM_DropClient(client_t *cl)
{
	if (cl->name)
		Q_strncpyz(cl->namebuf, cl->name, sizeof(cl->namebuf));

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_CLIENT_DISCONNECT, 0, 0, 0);
	cl->name = cl->namebuf;
}

void Q1QVM_ChainMoved(void)
{
}
void Q1QVM_EndFrame(void)
{
}

#endif
