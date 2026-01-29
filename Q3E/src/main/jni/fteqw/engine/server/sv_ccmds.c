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
#include "quakedef.h"
#include "pr_common.h"
#include "fs.h"
#include "netinc.h"

#ifndef CLIENTONLY

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif



int	sv_allow_cheats;
qboolean SV_MayCheat(void)
{
	if (sv_allow_cheats == 2)
		return sv.allocated_client_slots == 1;
	return sv_allow_cheats!=0;
}

#ifdef SUBSERVERS
cvar_t sv_autooffload = CVARD("sv_autooffload", "0", "Automatically start the server in a separate process, so that sporadic or persistent gamecode slowdowns do not affect visual framerates (equivelent to the mapcluster command). Note: Offloaded servers have separate cvar+command states which may complicate usage.");
#endif
extern cvar_t cl_warncmd;
cvar_t sv_cheats = CVARF("sv_cheats", "0", CVAR_MAPLATCH);
	extern		redirect_t	sv_redirected;

extern cvar_t sv_public;

static const struct banflags_s
{
	unsigned int banflag;
	const char *names[2];
} banflags[] =
{
	{BAN_BAN,		{"ban"}},
	{BAN_PERMIT,	{"safe",		"permit"}},
	{BAN_CUFF,		{"cuff"}},
	{BAN_MUTE,		{"mute"}},
	{BAN_VMUTE,		{"vmute"}},
	{BAN_CRIPPLED,	{"cripple"}},
	{BAN_DEAF,		{"deaf"}},
	{BAN_LAGGED,	{"lag",		"lagged"}},
	{BAN_VIP,		{"vip"}},
	{BAN_BLIND,		{"blind"}},
	{BAN_SPECONLY,	{"spec"}},
	{BAN_STEALTH,	{"stealth"}},
	{BAN_MAPPER,	{"mapper"}},

	{BAN_USER1,		{"user1"}},
	{BAN_USER2,		{"user2"}},
	{BAN_USER3,		{"user3"}},
	{BAN_USER4,		{"user4"}},
	{BAN_USER5,		{"user5"}},
	{BAN_USER6,		{"user6"}},
	{BAN_USER7,		{"user7"}},
	{BAN_USER8,		{"user8"}}
};

//generic helper function for naming players.
client_t *SV_GetClientForString(const char *name, int *id)
{
	int i;
	const char *s;
	char nicename[80];
	char niceclname[80];
	client_t *cl;

	int first=0;
	if (id && *id != -1)
		first = *id;
	if (first < 0)
	{
		if (id)
			*id=sv.allocated_client_slots;
		return NULL;
	}

	if (!strcmp(name, "*"))	//match with all
	{
		for (i = first, cl = svs.clients+first; i < sv.allocated_client_slots; i++, cl++)
		{
			if (cl->state<=cs_loadzombie)
				continue;

			if (id)
				*id=i+1;
			return cl;
		}
		if (id)
			*id=sv.allocated_client_slots;
		return NULL;
	}

	//check to make sure it's all an int

	for (s = name; *s; s++)
	{
		if (*s < '0' || *s > '9')
			break;
	}

	//we got to the end of the string and found only numbers. - it's a uid.
	if (!*s)
	{
		int uid = Q_atoi(name);
		for (i = first, cl = svs.clients+first; i < sv.allocated_client_slots; i++, cl++)
		{
			if (cl->state<=cs_loadzombie)
				continue;
			if (cl->userid == uid)
			{
				if (id)
					*id=sv.allocated_client_slots;
				return cl;
			}
		}

		return NULL;
	}

	for (i = first, cl = svs.clients+first; i < sv.allocated_client_slots; i++, cl++)
	{
		if (cl->state<=cs_loadzombie)
			continue;


		deleetstring(niceclname, cl->name);
		deleetstring(nicename, name);

		if (strstr(niceclname, nicename))
		{
			if (id)
				*id=i+1;
			return cl;
		}
	}

	return NULL;
}

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
SV_Quit_f
==================
*/
static void SV_Quit_f (void)
{
	if (sv.state >= ss_loading)
		SV_FinalMessage ("server shutdown\n");
	Con_TPrintf ("Shutting down.\n");
	SV_Shutdown ();
	Sys_Quit ();
}

/*
============
SV_Fraglogfile_f
============
*/
static void SV_Fraglogfile_f (void)
{
	char	name[MAX_OSPATH];
	int		i;

	if (sv_fraglogfile)
	{
		Con_TPrintf ("Frag file logging off.\n");
		VFS_CLOSE (sv_fraglogfile);
		sv_fraglogfile = NULL;
		return;
	}

	// find an unused name
	for (i=0 ; i<1000 ; i++)
	{
		sprintf (name, "frag_%i.log", i);
		sv_fraglogfile = FS_OpenVFS(name, "rb", FS_GAME);
		if (!sv_fraglogfile)
		{	// can't read it, so create this one
			sv_fraglogfile = FS_OpenVFS (name, "wb", FS_GAME);
			if (!sv_fraglogfile)
				i=1000;	// give error
			break;
		}
		VFS_CLOSE (sv_fraglogfile);
	}
	if (i==1000)
	{
		Con_TPrintf ("Can't open any logfiles.\n");
		sv_fraglogfile = NULL;
		return;
	}

	Con_TPrintf ("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
static qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;

	idnum = atoi(Cmd_Argv(1));

	for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->edict;
			return true;
		}
	}
	Con_TPrintf ("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
static void SV_God_f (void)
{
	if (!SV_MayCheat())
	{
		Con_TPrintf ("Please set sv_cheats 1 and restart the map first.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_LogPlayer(host_client, "god cheat");
	sv_player->v->flags = (int)sv_player->v->flags ^ FL_GODMODE;
	if ((int)sv_player->v->flags & FL_GODMODE)
		SV_ClientTPrintf (host_client, PRINT_HIGH, "godmode ON\n");
	else
		SV_ClientTPrintf (host_client, PRINT_HIGH, "godmode OFF\n");
}


static void SV_Noclip_f (void)
{
	if (!SV_MayCheat())
	{
		Con_TPrintf ("Please set sv_cheats 1 and restart the map first.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_LogPlayer(host_client, "noclip cheat");
	if (sv_player->v->movetype != MOVETYPE_NOCLIP)
	{
		sv_player->v->movetype = MOVETYPE_NOCLIP;
		SV_ClientTPrintf (host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		sv_player->v->movetype = MOVETYPE_WALK;
		SV_ClientTPrintf (host_client, PRINT_HIGH, "noclip OFF\n");
	}
}

#ifdef QUAKESTATS
/*
==================
SV_Give_f
==================
*/
static void SV_Give_f (void)
{
	char	*t = Cmd_Argv(2);
	int		v;

	if (!svprogfuncs)
		return;

	if (!strcmp(t, "damn"))
	{
		Con_TPrintf ("%s not given.\n", t);
		return;
	}

	if (!SV_MayCheat())
	{
		Con_TPrintf ("Please set sv_cheats 1 and restart the map first.\n");
		return;
	}

/*	if (developer.value)
	{
		int oldself;
		oldself = pr_global_struct->self;
		pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, sv.world.edicts);
		Con_Printf("Result: %s\n", svprogfuncs->EvaluateDebugString(svprogfuncs, Cmd_Args()));
		pr_global_struct->self = oldself;
	}
*/
	if (!SV_SetPlayer ())
	{
		return;
	}

	SV_LogPlayer(host_client, "give cheat");

	v = atoi (Cmd_Argv(3));

	switch ((t[1]==0)?t[0]:0)
	{
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		sv_player->v->items = (int)sv_player->v->items | IT_SHOTGUN<< (t[0] - '2');
		break;

	case 's':
		sv_player->v->ammo_shells = v;
		break;
	case 'n':
		sv_player->v->ammo_nails = v;
		break;
	case 'r':
		sv_player->v->ammo_rockets = v;
		break;
	case 'h':
		sv_player->v->health = v;
		break;
	case 'c':
		sv_player->v->ammo_cells = v;
		break;
/*	default:
		{
			int oldself;
			oldself = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, sv_player);
			Cmd_ShiftArgs(1, false);
			Con_TPrintf("Result: %s\n", svprogfuncs->EvaluateDebugString(svprogfuncs, Cmd_Args()));
			pr_global_struct->self = oldself;
		}
*/
	}
}
#endif


#if defined(HAVE_LEGACY) && defined(HAVE_SERVER)
static void SV_redundantcommand_f(void)
{
	if (cl_warncmd.ival)
		Con_Printf("%s is obsolete, redundant, or otherwise outdated.\n", Cmd_Argv(0));
}
#endif

static int QDECL ShowMapList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	searchpathfuncs_t **oldspath = parm;
	const char *levelshots[] =
	{
		"levelshots/%s.tga",
		"levelshots/%s.jpg",
		"levelshots/%s.png",
		"maps/%s.tga",
		"maps/%s.jpg",
		"maps/%s.png"
	};
	size_t u;
	char stripped[MAX_QPATH];
	char completed[256];
	const char *cmd = name+5; //the arg to pass to `map`
	const char *ext;
	flocation_t loc;
	if (name[5] == 'b' && name[6] == '_')	//skip box models
		return true;

	if (FS_FLocateFile(name, FSLF_IFFOUND, &loc))
	{
		if (loc.search->handle != spath)
			return true; //shadowed
	}
	else
		return true; //wtf?

	ext = COM_GetFileExtension (name+5, NULL);
	if (!strcmp(ext, ".gz") || !strcmp(ext, ".xz"))
		ext = COM_GetFileExtension (name+5, ext);	//.gz files should be listed too.

	if (!strcmp(ext, ".bsp") || !Q_strcasecmp(ext, ".d3dbsp") || !Q_strcasecmp(ext, ".cm"))
	{
		ext = "";	//hide it
		cmd = stripped;	//omit it, might as well. should give less confusing mapname serverinfo etc.
	}
	else if (!Q_strcasecmp(ext, ".bsp") || !Q_strcasecmp(ext, ".bsp.gz") || !Q_strcasecmp(ext, ".bsp.xz"))
		;
	else if (!Q_strcasecmp(ext, ".d3dbsp") || !Q_strcasecmp(ext, ".d3dbsp.gz") || !Q_strcasecmp(ext, ".d3dbsp.xz"))
		;	//cod2 compat. vile.
#ifdef TERRAIN
	else if (!Q_strcasecmp(ext, ".map") || !Q_strcasecmp(ext, ".map.gz") || !Q_strcasecmp(ext, ".hmp"))
		;
#endif
	else if (!Q_strcasecmp(ext, ".ent") && strchr(name+5, '#'))
	{	//FIXME hide if earlier that the .bsp
		ext = ""; //hide it.
		cmd = stripped;	//do NOT use the .ent extension here
	}
	else
		return true; //probably a .lit

	if (*oldspath != spath)
	{
		*oldspath = spath;
		Con_Printf(S_COLOR_GRAY"From %s\n", loc.search->purepath);
	}

	*completed = 0;
#ifdef HAVE_CLIENT
	{
		float besttime, fulltime, kills, secrets;
		if (Log_CheckMapCompletion(NULL, name, &besttime, &fulltime, &kills, &secrets))
		{
			if (kills || secrets)
				Q_snprintfz(completed, sizeof(completed), "^7 - ^2best: ^1%.1f^2, full: ^1%.1f^2 (^1%.0f^2 kills, ^1%.0f^2 secrets)", besttime, fulltime, kills, secrets);
			else
				Q_snprintfz(completed, sizeof(completed), "^7 - ^2best: ^1%.1f^2", besttime);
		}
	}
#endif

	COM_StripExtension(name+5, stripped, sizeof(stripped));
	for (u = 0; u < countof(levelshots); u++)
	{
		const char *ls = va(levelshots[u], stripped);
		if (COM_FCheckExists(ls))
		{
			Con_Printf("^[\\map\\%s\\img\\%s\\w\\64\\h\\48^]", cmd, ls);
			Con_Printf("^[[%s%s]%s\\map\\%s\\tipimg\\%s\\tip\\from %s/%s^]\n", stripped, ext, completed, cmd, ls, loc.search->logicalpath, name);
			return true;
		}
	}
	Con_Printf("^[[%s%s]%s\\map\\%s\\tip\\from %s/%s^]\n", stripped, ext, completed, cmd, loc.search->logicalpath, name);
	return true;
}
static void SV_MapList_f(void)
{
	searchpathfuncs_t *spath = NULL;
	COM_EnumerateFilesReverse("maps/*.*", ShowMapList, &spath);
	COM_EnumerateFilesReverse("maps/*/*.*", ShowMapList, &spath);
	COM_EnumerateFilesReverse("maps/*/*/*.*", ShowMapList, &spath);
}

static int QDECL CompleteMapList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	char stripped[64];
	if (name[5] == 'b' && name[6] == '_')	//skip box models
		return true;

	COM_StripExtension(name+5, stripped, sizeof(stripped));
	ctx->cb(stripped, NULL, NULL, ctx);
	return true;
}
static int QDECL CompleteMapListEnt (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	char stripped[64];
	char *modifier = strchr(name, '#');
	if (!modifier)	//skip non-modifiers.
		return true;
	if (modifier-name+4 > sizeof(stripped))	//too long...
		return true;

	//make sure we have its .bsp
	memcpy(stripped, name, modifier-name);
	strcpy(stripped+(modifier-name), ".bsp");
	if (!COM_FCheckExists(stripped))
		return true;

	COM_StripExtension(name+5, stripped, sizeof(stripped));
	ctx->cb(stripped, NULL, NULL, ctx);
	return true;
}

static int QDECL CompleteMapListExt (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	if (name[5] == 'b' && name[6] == '_')	//skip box models
		return true;

	ctx->cb(name+5, NULL, NULL, ctx);
	return true;
}
static void SV_Map_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		//FIXME: maps/mapname#modifier.ent
		COM_EnumerateFiles(va("maps/%s*.bsp", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*.d3dbsp", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*.bsp.gz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*.bsp.xz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*.map", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*.map.gz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*.cm", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*.hmp", partial), CompleteMapList, ctx);

		COM_EnumerateFiles(va("maps/%s*.ent", partial), CompleteMapListEnt, ctx);

		COM_EnumerateFiles(va("maps/%s*/*.bsp", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.d3dbsp", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.bsp.gz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.bsp.xz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.map", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.map.gz", partial), CompleteMapListExt, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.cm", partial), CompleteMapList, ctx);
		COM_EnumerateFiles(va("maps/%s*/*.hmp", partial), CompleteMapList, ctx);

#ifdef PACKAGEMANAGER
		PM_EnumerateMaps(partial, ctx);
#endif
	}
}

#ifdef WEBCLIENT
static char *uri_escape(const char *in, char *out, size_t outsize)
{
	static const char *hex = "0123456789ABCDEF";

	const unsigned char *s = in;
	unsigned char *o = out;
	while (*s && o < (unsigned char*)out+outsize-4)
	{
		//unreserved chars according to RFC3986
		if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9')
				|| *s == '.' || *s == '-' || *s == '_' || *s == '~')
			*o++ = *s++;
		else
		{
			*o++ = '%';
			*o++ = hex[*s>>4];
			*o++ = hex[*s&0xf];
			s++;
		}
	}
	*o = 0;
	return out;
}
static void SV_Map_DownloadCanceled(const char *mapname)
{
#ifdef HAVE_SERVER
	if (SSV_IsSubServer() && !sv.state)	//subservers don't leave defunct servers with no maps lying around.
		Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
	if (isDedicated && !sv.state && !COM_CheckParm("-allowmapless"))
		SV_Error (CON_ERROR"Couldn't download map %s.", mapname);
#endif
#ifdef HAVE_CLIENT
	SCR_SetLoadingStage(LS_NONE);
#endif
}
static void SV_Map_Downloaded(struct dl_download *dl)
{
	char buf[1024];
#ifdef HAVE_CLIENT
	SCR_SetLoadingStage(LS_NONE);
#endif
	if (dl->status == DL_FINISHED)
	{
		Cbuf_AddText(va("map %s\n", COM_QuotedString(dl->user_ctx, buf, sizeof(buf), false)), RESTRICT_LOCAL);
	}
	else
	{
		Con_Printf("Unable to download map %s\n", COM_QuotedString(dl->user_ctx, buf, sizeof(buf), false));
		SV_Map_DownloadCanceled(dl->user_ctx);
	}
	Z_Free(dl->user_ctx);
}
extern cvar_t cl_download_mapsrc;
static qboolean SV_Map_DownloadStart(char *mapname/*must be duped*/)
{
	char buf[512];
	struct dl_download *dl = HTTP_CL_Get(va("%s%s.bsp", cl_download_mapsrc.string, uri_escape(mapname, buf, sizeof(buf))), va("maps/%s.bsp", mapname), SV_Map_Downloaded);
	if (dl)
	{
		dl->user_ctx = mapname;
		DL_CreateThread(dl, NULL, NULL);	//allows it to run at its own rate. yay speedups.
		return true;
	}
	Z_Free(mapname);
	return false;
}
#ifdef HAVE_CLIENT
static void SV_Map_DownloadPrompted(void *ctx, promptbutton_t buttn)
{
	if (buttn == PROMPT_YES)
	{
		if (SV_Map_DownloadStart(ctx))
			return;
		SV_Map_DownloadCanceled(ctx);
	}
	else
	{
		SV_Map_DownloadCanceled(ctx);
		Z_Free(ctx);
	}
}
#endif
static qboolean SV_Map_DownloadPrompt(const char *mapname)
{
#ifdef HAVE_SERVER
	if (isDedicated)
	{
		return SV_Map_DownloadStart(Z_StrDup(mapname));
	}
#endif
#ifdef HAVE_CLIENT
	Menu_Prompt(SV_Map_DownloadPrompted, Z_StrDup(mapname), va(localtext("Download map %s from "S_COLOR_BLUE "%s" S_COLOR_WHITE"?"), mapname, cl_download_mapsrc.string), "Download", NULL, "Cancel", true);
	return true;
#else
	return false;
#endif
}
#endif

//static void gtcallback(struct cvar_s *var, char *oldvalue)
//{
//	Con_Printf("g_gametype changed\n");
//}

/*
======================
SV_Map_f

handle a
map <mapname>
command from the console or progs.

quirks:
a leading '*' means new unit, meaning all old map state is flushed regardless of startspot
a '+' means 'set nextmap cvar to the following value and otherwise ignore, for q2 compat. only applies if there's also a '.' and the specified bsp doesn't exist, for q1 compat.
just a '.' is taken to mean 'restart'. parms are not changed from their current values, startspot is also unchanged. Loads the last saved game instead when applicable.

variations:
'map' will change map, for most games. strips parms+serverflags+cache. note that vanilla NQ kicks everyone (NQ expects you to use changelevel for that).
'changelevel' will not flush the level cache, for h2 compat (won't save current level state in such a situation, as nq would prefer not)
'gamemap' will save the game to 'save0' after loading, for q2 compat
'spmap' is for q3 and sets 'gametype' to '2', otherwise identical to 'map'. all other map commands will reset it to '0' if its '2' at the time.
'spdevmap' forces sv_cheats 1, otherwise spmap
'devmap' forces sv_cheats 1, otherwise map
'map_restart' restarts the current map. Name is needed for q3 compat.
'restart' is an alias for 'map_restart'. Exists for NQ compat, but as an alias for QW mods that tried to use it for mod-specific things.

hexen2 fixme:
'restart restore' restarts the map, reloading from a saved game if applicable.
'restart' forgets the current map (potentially breaking the game). we don't care much for that behaviour (could make it a 'restart unit' I guess).

quake2:
'gamemap [*]foo.dm2[$spot][+nextserver]'
	* == new unit
	$ == start spot
	+ == value for nextserver cvar (used for cinematics).
'map' is always a new unit.

quake:
+ is used in certain map names. * cannot be, but $ potentially could be.

fte:
'map package:mapname' should download the specified map package and load up its maps.

mvdsv:
'map foo bar' should load 'maps/bar.ent' instead of the regular ent file. this 'bar' will usually be something like 'foo#modified'

======================
*/
void SV_Map_f (void)
{
	char	level[MAX_QPATH];
	char	spot[MAX_QPATH];
	char	expanded[MAX_QPATH+64];
	char	*nextserver = NULL;
	qboolean preserveplayers= false;
	qboolean isrestart		= false;	//don't hurt settings
#ifdef SAVEDGAMES
	qboolean newunit		= false;	//no hubcache
	qboolean q2savetos0		= false;
#endif
	qboolean flushparms		= false;	//flush parms+serverflags
	qboolean cinematic		= false;	//new map is .cin / .roq or something
#ifdef Q3SERVER
	qboolean q3singleplayer	= false;	//forces g_gametype to 2 (otherwise clears if it was 2).
#endif
	qboolean waschangelevel	= false;
	qboolean mapeditor		= false;
	qboolean forceCheats = false;
	int i;
	char *startspot;
	const char *cmd = Cmd_Argv(0);

#ifndef SERVERONLY
	if (!Renderer_Started() && !isDedicated)
	{
		Cbuf_AddText(va("wait;%s %s\n", cmd, Cmd_Args()), Cmd_ExecLevel);
		return;
	}
#endif

#ifdef SUBSERVERS
	//disconnect first if you want to stop your current server getting the command instead.
	if (sv.state == ss_clustermode && MSV_ForwardToAutoServer())
		return;
#endif

	if (!Q_strcasecmp(cmd, "map_restart"))
	{
		const char *arg = Cmd_Argv(1);

#ifdef Q3SERVER
		Cvar_ApplyLatches(CVAR_MAPLATCH, false);
		if (sv.state==ss_active && svs.gametype==GT_QUAKE3 && q3->sv.RestartGamecode())
		{
			sv.time = sv.world.physicstime;
			sv.starttime = Sys_DoubleTime() - sv.time;
			return;
		}
#endif

#ifdef SAVEDGAMES
		if (!strcmp(arg, "restore"))		//hexen2 reload-saved-game
			;
		else if (!strcmp(arg, "initial"))	//force initial, even if it breaks saved games.
			*sv.loadgame_on_restart = 0;
		else
#endif
		{
			float delay = atof(arg);
			if (delay)			//q3's restart-after-delay
				Con_DPrintf ("map_restart delay not implemented yet\n");
		}
		Q_strncpyz (level, ".", sizeof(level));
		startspot = NULL;
		isrestart = true;

		//FIXME: if precaches+statics don't change, don't do the whole networking thing.
	}
	else
	{
		if (Cmd_Argc() != 2 && Cmd_Argc() != 3)
		{
			if (Cmd_IsInsecure())
				return;
			Con_TPrintf ("Available maps:\n", Cmd_Argv(0));
			SV_MapList_f();
			return;
		}

#ifdef PACKAGEMANAGER
		if (Cmd_Argc() == 2)
		{
			char *mangled = Cmd_Argv(1);
			char *sep = strchr(mangled, ':');
			if (sep && strncmp(mangled, "file:", 5) && strncmp(mangled, "http:", 5) && strncmp(mangled, "https:", 5))
			{
				*sep++ = 0;
				if (Cmd_FromGamecode())
				{
					Con_TPrintf ("switching packages via %s command is blocked from gamecode, just in case.\n", Cmd_Argv(0));
					sv.mapchangelocked = false;
				}
				else
					PM_LoadMap(mangled, va("%s %s\n", Cmd_Argv(0), COM_QuotedString(sep, expanded, sizeof(expanded), false)));
				return;
			}
		}
#endif

		Q_strncpyz (level, Cmd_Argv(1), sizeof(level));
		startspot = ((Cmd_Argc() == 2)?NULL:Cmd_Argv(2));
	}

#ifdef Q3SERVER
	q3singleplayer = !strncmp(cmd, "sp", 2);
#endif
	if ((svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM) && progstype == PROG_QW)
		flushparms = !strncmp(cmd, "sp", 2);	//quakeworld's map command preserves spawnparms. q3 doesn't do parms, so we might as well reuse sp[dev]map to flush in qw
	else
		flushparms = !strcmp(cmd, "map") || !strncmp(cmd, "sp", 2); //[sp]map flushes in nq+h2+q2+etc
#ifdef SAVEDGAMES
	newunit = flushparms || (!strcmp(Cmd_Argv(0), "changelevel") && !startspot);
	q2savetos0 = !strcmp(cmd, "gamemap") && !isDedicated;	//q2
#endif
	mapeditor = !strcmp(Cmd_Argv(0), "mapedit");
	forceCheats = !strcmp(Cmd_Argv(0), "devmap");

	sv.mapchangelocked = false;

	if (!strcmp(level, "."))
		;//restart current - deprecated.
	else
	{
		snprintf (expanded, sizeof(expanded), "maps/%s.bsp", level); // this function and the if statement below, is a quake bugfix which stopped a map called "dm6++.bsp" from loading because of the + sign, quake2 map syntax interprets + character as "intro.cin+base1.bsp", to play a cinematic then load a map after
		if (!COM_FCheckExists (level) && !COM_FCheckExists (expanded))
		{
			nextserver = strchr(level, '+');
			if (nextserver)
			{
				*nextserver = '\0';
				nextserver++;
			}
		}
	}

	if (startspot)
	{
		strcpy(spot, startspot);
		startspot = spot;
	}
	else if ((startspot = strchr(level, '$')))
	{
		strcpy(spot, startspot+1);
		*startspot = '\0';
		startspot = spot;
	}
	else
		startspot = NULL;

	if (!strcmp(level, "."))	//restart current
	{
		//grab the current map name
		Q_strncpyz(level, svs.name, sizeof(level));
		isrestart = true;
		flushparms = false;
#ifdef SAVEDGAMES
		newunit = false;
		q2savetos0 = false;
#endif

		if (!*level)
		{
			sv.mapchangelocked = true;
			if (Cmd_AliasExist("startmap_dm", RESTRICT_LOCAL))
			{
				Cbuf_AddText("startmap_dm", Cmd_ExecLevel);
				return;
			}
			Q_strncpyz(level, "start", sizeof(level));
		}

		if (startspot && !strcmp(startspot, "."))
		{
			preserveplayers = true;
			startspot = NULL;
		}
		if (!startspot)
		{
			//revert the startspot if its not overridden
			Q_strncpyz(spot, InfoBuf_ValueForKey(&svs.info, "*startspot"), sizeof(spot));
			startspot = spot;
		}
	}

#ifdef SAVEDGAMES
	if (isrestart && *sv.loadgame_on_restart && SV_Loadgame(sv.loadgame_on_restart))
	{	//we managed to reload a saved game instead!
		//this is required in order to keep hub state consistent (dying mid-map would require saved games to store both current and start of map(not to be confused with initial state, which would be trivial))
		return;
	}
#endif

	// check to make sure the level exists
	if (*level == '*')
	{
		memmove(level, level+1, strlen(level));
#ifdef SAVEDGAMES
		newunit=true;
#endif
	}
#ifndef SERVERONLY
	SCR_ImageName(level);
	SCR_SetLoadingStage(LS_SERVER);
	SCR_SetLoadingFile("finalize server");
#else
	#define SCR_SetLoadingFile(s)
#endif

	COM_FlushFSCache(false, true);

#ifdef Q2SERVER
	if (strlen(level) > 4 &&
		(!strcmp(level + strlen(level)-4, ".cin") ||
		!strcmp(level + strlen(level)-4, ".roq") ||
		!strcmp(level + strlen(level)-4, ".ogv") ||
		!strcmp(level + strlen(level)-4, ".pcx") ||
		!strcmp(level + strlen(level)-4, ".avi")))
	{
		cinematic = true;
	}
	else
#endif
#ifdef TERRAIN
	//'map doesntexist.map' should just auto-generate that map or something
	if (!Q_strcasecmp("map", COM_FileExtension(level, expanded, sizeof(expanded))))
		;
	else
#endif
	{
		char *exts[] = {"%s", "maps/%s", "maps/%s.bsp", "maps/%s.bsp.gz", "maps/%s.bsp.xz", "maps/%s.d3dbsp", "maps/%s.cm", "maps/%s.hmp", /*"maps/%s.map",*/ /*"maps/%s.ent",*/ NULL};
		int i, j;

		for (i = 0; exts[i]; i++)
		{
			snprintf (expanded, sizeof(expanded), exts[i], level);
			if (COM_FCheckExists (expanded))
				break;
		}
		if (!exts[i])
		{	//try again.
			char *mod = strchr(level, '#');
			if (mod)
			{
				*mod = 0;
				for (i = 0; exts[i]; i++)
				{
					snprintf (expanded, sizeof(expanded), exts[i], level);
					if (COM_FCheckExists (expanded))
						break;
				}
				*mod = '#';
			}
		}
		if (!exts[i])
		{
			for (i = 0; exts[i]; i++)
			{
				//doesn't exist, so try lowercase. Q3 does this. really our fs_cache stuff should be handling this, but its possible its disabled.
				for (j = 0; j < sizeof(level) && level[j]; j++)
				{
					if (level[j] >= 'A' && level[j] <= 'Z')
						level[j] = level[j] - 'A' + 'a';
				}
				snprintf (expanded, sizeof(expanded), exts[i], level);
				if (COM_FCheckExists (expanded))
					break;
			}
			if (!exts[i])
			{
#ifdef HAVE_CLIENT
				SCR_SetLoadingStage(LS_NONE);
#endif
#ifdef WEBCLIENT
				if (*cl_download_mapsrc.string &&
						!strcmp(cmd, "map") && !startspot &&
						Cmd_ExecLevel==RESTRICT_LOCAL && !strchr(level, '.'))
				{
					if (SV_Map_DownloadPrompt(level))
						return;
				}
#endif

				// FTE is still a Quake engine so report BSP missing
				Con_TPrintf ("Can't find %s\n", COM_QuotedString(va("maps/%s.bsp", level), expanded, sizeof(expanded), false));

				if (SSV_IsSubServer() && !sv.state)	//subservers don't leave defunct servers with no maps lying around.
					Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
				return;
			}
		}
	}

#ifdef SUBSERVERS
	if (!isDedicated && sv_autooffload.ival && !sv.state && !SSV_IsSubServer() && (
			isrestart
			|| (!strcmp(Cmd_Argv(0), "map") && Cmd_Argc()==2)
		))
	{
		MSV_MapCluster_Setup(level, false, true);
		return;
	}
#endif

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
		SV_MVDStop_f();
#endif

#ifndef SERVERONLY
	if (!isDedicated)	//otherwise, info used on map loading isn't present
	{
		cl.haveserverinfo = true;
		InfoBuf_Clone(&cl.serverinfo, &svs.info);
		CL_CheckServerInfo();
	}

	if (!sv.state && cls.state)
		CL_Disconnect(NULL);
#endif

	if (!isrestart)
		SV_SaveSpawnparms ();

#ifdef SAVEDGAMES
	if (newunit)
		SV_FlushLevelCache();	//forget all on new unit
	else if (startspot && !isrestart && !newunit)
	{
#ifdef Q2SERVER
		if (ge)
		{
			qboolean savedinuse[MAX_CLIENTS];
			for (i=0 ; i<sv.allocated_client_slots; i++)
			{
				savedinuse[i] = svs.clients[i].q2edict->inuse;
				svs.clients[i].q2edict->inuse = false;
			}
			SV_SaveLevelCache(NULL, false);
			for (i=0 ; i<sv.allocated_client_slots; i++)
			{
				svs.clients[i].q2edict->inuse = savedinuse[i];
			}
		}
		else
#endif
			SV_SaveLevelCache(NULL, false);
	}
#endif

	if (forceCheats) {
		Cvar_ForceSet(&sv_cheats, "1");
	}

#ifdef Q3SERVER
	{
		cvar_t *var, *gametype;

		Cvar_ApplyLatches(CVAR_MAPLATCH, false);

		host_mapname.flags |= CVAR_SERVERINFO;

		var = Cvar_Get("nextmap", "", 0, "Q3 compatibility");
		Cvar_ForceSet(var, "map_restart 0");	//on every map change matches q3.

		gametype = Cvar_Get("g_gametype", "", CVAR_MAPLATCH|CVAR_SERVERINFO, "Q3 compatability");
//		gametype->callback = gtcallback;

		/* map_restart doesn't need to handle gametype changes - eukara */
		if (!isrestart)
		{
			if (q3singleplayer)
			{
				Cvar_ForceSet(gametype, "2");//singleplayer
				Cvar_ForceSet(&deathmatch, "0");//for non-q3 type stuff to not get confused..
			}
			else if (gametype->value == 2)
				Cvar_ForceSet(gametype, "");//force to ffa deathmatch
		}
	}
#endif

	Cvar_ForceSet(&host_mapname, level);

#ifdef HAVE_CLIENT
	Menu_PopAll();
#endif

	if (preserveplayers && svprogfuncs)
	{
		for (i=0 ; i<svs.allocated_client_slots ; i++)	//we need to drop all q2 clients. We don't mix q1w with q2.
		{
			char buffer[8192], *buf;
			size_t bufsize = 0;
			if (svs.clients[i].state>cs_connected)
			{
				buf = svprogfuncs->saveent(svprogfuncs, buffer, &bufsize, sizeof(buffer), svs.clients[i].edict);
				if (svs.clients[i].spawninfo)
					Z_Free(svs.clients[i].spawninfo);
				svs.clients[i].spawninfo = Z_Malloc(bufsize+1);
				memcpy(svs.clients[i].spawninfo, buf, bufsize+1);
				svs.clients[i].spawninfotime = sv.time;
			}
		}
	}
	else
	{
		for (i=0 ; i<svs.allocated_client_slots ; i++)	//we need to drop all q2 clients. We don't mix q1w with q2.
		{
			if (svs.clients[i].state>cs_connected)	//so that we don't send a datagram
				svs.clients[i].state=cs_connected;
		}
	}

#ifndef SERVERONLY
	S_StopAllSounds (true);
//	SCR_BeginLoadingPlaque();
	SCR_ImageName(level);
#endif

//	if (!preserveplayers)
	{
		for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
		{
			/*pass the new map's name as an extension, so appropriate loading screens can be shown*/
			if (host_client->controller == NULL)
			{
				if (ISNQCLIENT(host_client))
				{
					if (ISDPCLIENT(host_client))
					{
						//DP clients cannot cope with being told the next map's name
						SV_StuffcmdToClient(host_client, "reconnect\n");
					}
					else
						SV_StuffcmdToClient(host_client, va("reconnect \"%s\"\n", level));
				}
				else
					SV_StuffcmdToClient(host_client, va("changing \"%s\"\n", level));
			}
			host_client->prespawn_stage = PRESPAWN_INVALID;
			host_client->prespawn_idx = 0;
		}

#ifdef NQPROT
		if (dpcompat_nopreparse.ival)
		{	//wipe broadcasts here...
			sv.reliable_datagram.cursize = 0;
			sv.datagram.cursize = 0;
			sv.nqreliable_datagram.cursize = 0;
			sv.nqdatagram.cursize = 0;
		}
#endif

		SV_SendMessagesToAll ();

		if (flushparms)
			svs.serverflags = 0;
	}

	SCR_SetLoadingFile("spawnserver");
#ifdef SAVEDGAMES
	if (newunit || !startspot || cinematic || !SV_LoadLevelCache(NULL, level, startspot, false))
#endif
	{
		if (waschangelevel && !startspot)
			startspot = "";
		SV_SpawnServer (level, startspot, mapeditor, cinematic, 0);
	}
	SCR_SetLoadingFile("server spawned");

	//SV_BroadcastCommand ("cmd new\n");
	for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
	{	//this expanded code cuts out a packet when changing maps...
		//but more usefully, it stops dp(and probably nq too) from timing out.
		//make sure its all reset.
		host_client->sentents.num_entities = 0;
		host_client->ratetime = 0;
		if (host_client->pendingdeltabits)
			host_client->pendingdeltabits[0] = UF_SV_REMOVE;

		if (flushparms)
		{
			if (host_client->spawninfo)
				Z_Free(host_client->spawninfo);
			host_client->spawninfo = NULL;
			memset(host_client->spawn_parms, 0, sizeof(host_client->spawn_parms));
			if (host_client->state > cs_zombie)
				SV_GetNewSpawnParms(host_client);
		}

		if (preserveplayers && svprogfuncs && host_client->state == cs_spawned && host_client->spawninfo)
		{
			size_t j = 0;
			svprogfuncs->restoreent(svprogfuncs, host_client->spawninfo, &j, host_client->edict);
			host_client->istobeloaded = true;
			host_client->state=cs_connected;
			if (host_client->spectator)
				sv.spawned_observer_slots++;
			else
				sv.spawned_client_slots++;
		}

		if (host_client->controller)
			continue;
		if (host_client->state>=cs_connected)
		{
			if (host_client->protocol == SCP_QUAKE3)
				continue;
			if (host_client->protocol == SCP_BAD)
				continue;

#ifdef NQPROT
			if (ISNQCLIENT(host_client))
			{
				SVNQ_New_f();
				host_client->send_message = true;
			}
			else
#endif
				SV_New_f();
		}
	}

	if (!isrestart)
	{
		cvar_t *nsv;
		nsv = Cvar_Get("nextserver", "", 0, "");
		if (nextserver)
			Cvar_Set(nsv, va("gamemap \"%s\"", nextserver));
		else
			Cvar_Set(nsv, "");
	}

#ifdef SAVEDGAMES
	if (q2savetos0)
	{
		if (sv.state != ss_cinematic)	//too weird.
			SV_Savegame("s0", true);
	}
#endif

	if (isDedicated)
		Mod_Purge(MP_MAPCHANGED);
}

static void SV_KillServer_f(void)
{
	SV_UnspawnServer();
}


/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f (void)
{
	client_t	*cl;
	int clnum=-1;

	if (!sv.state)
		return;

	if (!strcmp(Cmd_Argv(1), "#"))
	{
		clnum = atoi(Cmd_Argv(2)) - 1;
		if (clnum >= 0 && clnum < sv.allocated_client_slots)
		{
			cl = &svs.clients[clnum];
			if (cl->state >= cs_connected)
			{
				SV_BroadcastTPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
				// print directly, because the dropped client won't get the
				// SV_BroadcastPrintf message
				SV_ClientTPrintf (cl, PRINT_HIGH, "You were kicked\n");

				SV_LogPlayer(cl, "kicked");
				SV_DropClient (cl);
			}
		}
		return;
	}

	while((cl = SV_GetClientForString(Cmd_Argv(1), &clnum)))
	{
		SV_BroadcastTPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
		// print directly, because the dropped client won't get the
		// SV_BroadcastPrintf message
		SV_ClientTPrintf (cl, PRINT_HIGH, "You were kicked\n");

		SV_LogPlayer(cl, "kicked");
		SV_DropClient (cl);
	}

	if (clnum == -1)
		Con_TPrintf ("Couldn't find user number %s\n", Cmd_Argv(1));
}

/*for q3's kick bot menu*/
static void SV_KickSlot_f (void)
{
	client_t	*cl;
	int clnum=atoi(Cmd_Argv(1));

	if (!sv.state)
		return;

	if (clnum < sv.allocated_client_slots && svs.clients[clnum].state)
	{
		cl = &svs.clients[clnum];

		SV_BroadcastTPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
		// print directly, because the dropped client won't get the
		// SV_BroadcastPrintf message
		SV_ClientTPrintf (cl, PRINT_HIGH, "You were kicked\n");

		SV_LogPlayer(cl, "kicked");
		SV_DropClient (cl);
	}
	else
		Con_Printf("Client %i is not active\n", clnum);
}

//ipv4ify if its an ipv6 ipv4-mapped address.
static netadr_t *NET_IPV4ify(netadr_t *a, netadr_t *tmp)
{
	if (a->type == NA_IPV6 &&
		!*(int*)&a->address.ip6[0] &&
		!*(int*)&a->address.ip6[4] &&
		!*(short*)&a->address.ip6[8] &&
		*(short*)&a->address.ip6[10]==(short)0xffff)
	{
		tmp->type = NA_IP;
		tmp->connum = a->connum;
		tmp->scopeid = a->scopeid;
		tmp->port = a->port;
		tmp->prot = a->prot;
		tmp->address.ip[0] = a->address.ip6[12];
		tmp->address.ip[1] = a->address.ip6[13];
		tmp->address.ip[2] = a->address.ip6[14];
		tmp->address.ip[3] = a->address.ip6[15];
		a = tmp;
	}
	return a;
}

//will kick clients if they got banned (without being safe)
void SV_EvaluatePenalties(client_t *cl)
{
	bannedips_t *banip;
	unsigned int penalties = 0, delta, p;
	char *penaltyreason[countof(banflags)] = {NULL};
	const char *activepenalties[countof(banflags)];
	char *reasons[countof(banflags)] = {NULL};
	int numpenalties = 0;
	int numreasons = 0;
	int i;
	netadr_t tmp, *a;

	if (cl->realip.type != NA_INVALID)
	{
		a = NET_IPV4ify(&cl->realip, &tmp);
		for (banip = svs.bannedips; banip; banip=banip->next)
		{
			if (NET_CompareAdrMasked(a, &banip->adr, &banip->adrmask))
			{
				for (i = 0; i < sizeof(penaltyreason)/sizeof(penaltyreason[0]); i++)
				{
					p = 1u<<i;
					if (banip->banflags & p)
					{
						if (!penaltyreason[i])
							penaltyreason[i] = banip->reason;
						penalties |= p;
					}
				}
			}
		}
	}
	a = NET_IPV4ify(&cl->netchan.remote_address, &tmp);
	for (banip = svs.bannedips; banip; banip=banip->next)
	{
		if (NET_CompareAdrMasked(a, &banip->adr, &banip->adrmask))
		{
			for (i = 0; i < sizeof(penaltyreason)/sizeof(penaltyreason[0]); i++)
			{
				p = 1u<<i;
				if (banip->banflags & p)
				{
					if (!penaltyreason[i])
						penaltyreason[i] = banip->reason;
					penalties |= p;
				}
			}
		}
	}

	delta = cl->penalties ^ penalties;
	cl->penalties = penalties;

	if ((penalties & (BAN_BAN | BAN_PERMIT)) == BAN_BAN)
	{
		//we should only reach here by a player getting banned mid-game.
		if (penaltyreason[BAN_BAN])
			SV_BroadcastPrintf(PRINT_HIGH, "%s was banned: %s\n", cl->name, penaltyreason[BAN_BAN]);
		else
			SV_BroadcastPrintf(PRINT_HIGH, "%s was banned\n", cl->name);
		cl->drop = true;
	}

	//don't announce these now.
	delta &= ~(BAN_BAN | BAN_PERMIT);

	//deaf+mute sees no (other) penalty messages
	if (((penalties|delta) & (BAN_MUTE|BAN_DEAF)) == (BAN_MUTE|BAN_DEAF))
		delta &= ~(BAN_MUTE|BAN_DEAF);

	if ((delta|penalties) & BAN_STEALTH)
		delta = 0;	//don't announce ANY.

	if (cl->controller)
		delta = 0;	//don't spam it for every player in a splitscreen client.

	if (delta & BAN_VIP)
	{
		delta &= ~BAN_VIP;	//don't refer to this as a penalty
		if (penalties & BAN_VIP)
			SV_PrintToClient(cl, PRINT_HIGH, "You are a VIP, apparently\n");
		else
			SV_PrintToClient(cl, PRINT_HIGH, "VIP expired\n");
	}

	for (i = 0; i < countof(banflags); i++)
	{
		p = banflags[i].banflag;
		if (delta & p)
		{
			if (penalties & p)
			{
				if (banflags[i].names[0])
					activepenalties[numpenalties++] = banflags[i].names[0];
				if (reasons[i] && *reasons[i])
					reasons[numreasons++] = reasons[i];
			}
			else
				SV_PrintToClient(cl, PRINT_HIGH, va("Penalty expired: %s\n", banflags[i].names[0]));
		}
	}

	if (numpenalties)
	{
		char penaltystring[1024];
		int i, j;
		Q_strncpyz(penaltystring, "You are penalised: ", sizeof(penaltystring));
		for (i = 0; i < numpenalties; i++)
		{
			if (i && i == numpenalties-1)
				Q_strncatz(penaltystring, " and ", sizeof(penaltystring));
			else if (i)
				Q_strncatz(penaltystring, ", ", sizeof(penaltystring));
			Q_strncatz(penaltystring, activepenalties[i], sizeof(penaltystring));
		}
		Q_strncatz(penaltystring, "\n", sizeof(penaltystring));
		SV_PrintToClient(cl, PRINT_HIGH, penaltystring);
		for (i = 0; i < numreasons; i++)
		{
			if (*reasons[i])
			{
				for(j = 0; j < i; j++)
					if (!strcmp(reasons[i], reasons[j]))
						break;
				if (i == j)
					SV_PrintToClient(cl, PRINT_HIGH, va("  %s\n", reasons[i]));
			}
		}
	}

	if (delta & BAN_VIP)
		InfoBuf_SetStarKey(&cl->userinfo, "*VIP", (cl->penalties & BAN_VIP)?"1":"");
	if (delta & BAN_MAPPER)
		InfoBuf_SetStarKey(&cl->userinfo, "*mapper", (cl->penalties & BAN_MAPPER)?"1":"");
}

static time_t reevaluatebantime;
static qboolean reevaluatebans;
//could use time(NULL) instead, but this avoids a system call.
static time_t SV_BanTime(void)
{
	static double bantimemark;
	static time_t banstarttime;
	if (!banstarttime)
	{
		banstarttime = time(NULL);
		bantimemark = realtime;
	}
	return banstarttime + (realtime - bantimemark);
}
//removes anything with an expiry time in the past.
//avoids walking the list if there's nothing changed.
//can be used to force penalty reevaluation.
void SV_KillExpiredBans(void)
{
	bannedips_t **link, *banip;
	time_t curtime = SV_BanTime();
	int i;
	if (reevaluatebantime && curtime > reevaluatebantime)
	{
		reevaluatebantime = 0;
		for(link = &svs.bannedips; (banip = *link) != NULL; )
		{
			if (banip->expiretime)
			{
				if (banip->expiretime < curtime)
				{
					reevaluatebans = true;
					*link = banip->next;
					Z_Free(banip);
					continue;
				}
				if (!reevaluatebantime || reevaluatebantime > banip->expiretime)
					reevaluatebantime = banip->expiretime+1;
			}
			link = &banip->next;
		}
	}

	if (reevaluatebans)
	{
		reevaluatebans = false;
		for (i = 0; i < svs.allocated_client_slots; i++)
		{
			if (svs.clients[i].state<=cs_loadzombie)
				continue;

			SV_EvaluatePenalties(&svs.clients[i]);
		}
	}
}

//adds a new ban/penalty.
//will remove old penalties if the new one has a longer duration, otherwise will ignore the add.
static qboolean SV_AddBanEntry(bannedips_t *proto, char *reason)
{
	bannedips_t *nb, **link;
	nb = svs.bannedips;
	while (nb)
	{
		if (NET_CompareAdr(&nb->adr, &proto->adr) && NET_CompareAdr(&nb->adrmask, &proto->adrmask))
		{
			//found a match, figure out which lasts longer
			//the shorter ban duration gets its effective banflags stripped.
			if ((proto->expiretime && proto->expiretime < nb->expiretime) || !nb->expiretime)
				proto->banflags &= ~nb->banflags;
			else
				nb->banflags &= ~proto->banflags;

			if (!proto->banflags)
			{
				//we should not have been able to strip a previous nb->banflags if this ban was duped later.
				return false;
			}
			if (!nb->banflags)
				reevaluatebantime = nb->expiretime = 1;	//make sure it expires 'soon'.
		}
		nb = nb->next;
	}

	link = &svs.bannedips;

	// add IP and mask to filter list
	nb = Z_Malloc(sizeof(bannedips_t) + strlen(reason));
	nb->adr = proto->adr;
	nb->adrmask = proto->adrmask;
	nb->banflags = proto->banflags;
	nb->expiretime = proto->expiretime;
	Q_strcpy(nb->reason, reason);

	nb->next = *link;
	*link = nb;

	reevaluatebans = true;	//make sure the new ban/penalty applies to the right IPs.
	if (nb->expiretime && (!reevaluatebantime || reevaluatebantime > nb->expiretime))
		reevaluatebantime = nb->expiretime;
	return true;
}

//slightly different logic.
//if duration is specified, just does an add instead.
//otherwise ignores durations.
//only really works with a single toggle. if any are found, will not add.
//returns 1 if added, 0 if removed, and -1 if tried to add and it already existed.
static int SV_ToggleBan(bannedips_t *proto, char *reason)
{
	qboolean found = false;
	bannedips_t *nb;
	if (proto->expiretime)
		return SV_AddBanEntry(proto, reason)?true:-1;

	nb = svs.bannedips;
	while (nb)
	{
		if (NET_CompareAdr(&nb->adr, &proto->adr) && NET_CompareAdr(&nb->adrmask, &proto->adrmask))
		{
			if (nb->banflags & proto->banflags)
			{
				found = true;
				nb->banflags &= ~proto->banflags;
				reevaluatebans = true;
				if (!nb->banflags)
					reevaluatebantime = nb->expiretime = 1;	//make sure it expires 'soon' (in the past).
			}
		}
		nb = nb->next;
	}

	if (found)
		return 0;
	return SV_AddBanEntry(proto, reason)?true:-1;
}

extern cvar_t filterban;
//returns a reason if the client is banned. ignores other penalties.
char *SV_BannedReason (netadr_t *a)
{
	char *reason = filterban.value?NULL:"";	//"" = banned with no explicit reason
	bannedips_t *banip;
	netadr_t tmp;

	if (NET_IsLoopBackAddress(a))
		return NULL; // never filter loopback

	a = NET_IPV4ify(a, &tmp);

	for (banip = svs.bannedips; banip; banip=banip->next)
	{
		if (NET_CompareAdrMasked(a, &banip->adr, &banip->adrmask))
		{
			if (banip->banflags & BAN_BAN)
				return banip->reason;	//banned, with reason.
			if (banip->banflags & BAN_PERMIT)
				return NULL;	//allowed
		}
	}
	return reason;
}

static void SV_FilterIP_f (void)
{
	bannedips_t proto;
	extern cvar_t filterban;
	char *s;
	int i;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s <address/mask|adress/maskbits> [flags] [+time] [reason]\n", Cmd_Argv(0));
		Con_Printf("allowed flags: %s", banflags[0].names[0]);
		for (i = 1; i < countof(banflags); i++)
			Con_Printf(",%s", banflags[i].names[0]);
		Con_Printf(". time is in seconds (omitting the plus will be taken to mean unix time).\n");
		return;
	}

	if (!NET_StringToAdrMasked(Cmd_Argv(1), true, &proto.adr, &proto.adrmask))
	{
		Con_Printf("invalid address or mask\n");
		return;
	}

	s = Cmd_Argv(2);
	proto.banflags = 0;
	while(*s)
	{
		s=COM_ParseToken(s,",");
		if (!Q_strcasecmp(com_token, ","))
			i = -1;
		else for (i = 0; i < countof(banflags); i++)
		{
			if (!Q_strcasecmp(com_token, banflags[i].names[0]) || (banflags[i].names[1] && !Q_strcasecmp(com_token, banflags[i].names[1])))
			{
				proto.banflags |= banflags[i].banflag;
				break;
			}
		}
		if (i == countof(banflags))
			Con_Printf("Unknown ban/penalty flag: %s. ignoring.\n", com_token);
	}
	//if no flags were specified,
	if (!proto.banflags)
	{
		if (!strcmp(Cmd_Argv(0), "ban"))
			proto.banflags = BAN_BAN;
		else
			proto.banflags = filterban.ival?BAN_BAN:BAN_PERMIT;
	}

	if (NET_IsLoopBackAddress(&proto.adr) && (proto.banflags & BAN_NOLOCALHOST))
	{	//do allow them to be muted etc, just not banned outright.
		Con_Printf("You're not allowed to filter loopback!\n");
		return;
	}

	s = Cmd_Argv(3);
	if (*s == '+')
	{
		time_t secs = strtoull(s+1, &s, 0);
		if (*s == ':')
		{
			secs*=60;
			secs+=strtoull(s+1, &s, 0);
		}
		proto.expiretime = SV_BanTime() + secs;
	}
	else
		proto.expiretime = strtoull(s, NULL, 0);

	//and then add it
	if (!SV_AddBanEntry(&proto, Cmd_Argv(4)))
		Con_Printf("addip: entry already exists\n");
}

static void SV_BanList_f (void)
{
	int bancount = 0;
	bannedips_t *nb;
	char adr[MAX_ADR_SIZE];
	char middlebit[256];
	time_t bantime = SV_BanTime();

	SV_KillExpiredBans();

	for (nb = svs.bannedips; nb; nb = nb->next)
	{
		if (nb->banflags & BAN_BAN)
		{
			*middlebit = 0;
			if (nb->expiretime)
				Q_strncatz(middlebit, va(",\t+%"PRIu64, (quint64_t)(nb->expiretime - bantime)), sizeof(middlebit));
			if (nb->reason[0])
				Q_strncatz(middlebit, ",\t", sizeof(middlebit));
			Con_Printf("%s%s%s\n", NET_AdrToStringMasked(adr, sizeof(adr), &nb->adr, &nb->adrmask), middlebit, nb->reason);
			bancount++;
		}
	}

	Con_Printf("%i total entries in ban list\n", bancount);
}

static void SV_FilterList_f (void)
{
	int filtercount = 0;
	bannedips_t *nb;
	char adr[MAX_ADR_SIZE];
	char banflagtext[1024];
	int i;
	time_t curtime = SV_BanTime();

	SV_KillExpiredBans();

	for (nb = svs.bannedips; nb; )
	{
		*banflagtext = 0;
		for (i = 0; i < countof(banflags); i++)
		{
			if (nb->banflags & banflags[i].banflag)
			{
				if (*banflagtext)
					Q_strncatz(banflagtext, ",", sizeof(banflagtext));
				Q_strncatz(banflagtext, banflags[i].names[0], sizeof(banflagtext));
			}
		}

		if (nb->expiretime)
		{
			time_t secs = nb->expiretime - curtime;
			Con_Printf("%s %s +%"PRIu64":%02u\n", NET_AdrToStringMasked(adr, sizeof(adr), &nb->adr, &nb->adrmask), banflagtext, (quint64_t)(secs/60), (unsigned int)(secs%60));
		}
		else
			Con_Printf("%s %s\n", NET_AdrToStringMasked(adr, sizeof(adr), &nb->adr, &nb->adrmask), banflagtext);
		filtercount++;
		nb = nb->next;
	}

	Con_Printf("%i total entries in filter list\n", filtercount);
}

static void SV_Unfilter_f (void)
{
	qboolean found = false;
	qboolean all = false;
	bannedips_t **link;
	bannedips_t *nb;
	netadr_t unbanadr = {0};
	netadr_t unbanmask = {0};
	char adr[MAX_ADR_SIZE];
	unsigned int clearbanflags, nf;
	char *s;
	int i;

	SV_KillExpiredBans();

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s address/mask|address/maskbits|all [flags]\n", Cmd_Argv(0));
		return;
	}

	if (!Q_strcasecmp(Cmd_Argv(1), "all"))
	{
		Con_Printf("removing all filtered addresses\n");
		all = true;
	}
	else if (!NET_StringToAdrMasked(Cmd_Argv(1), true, &unbanadr, &unbanmask))
	{
		Con_Printf("invalid address or mask\n");
		return;
	}

	s = Cmd_Argv(2);
	clearbanflags = 0;
	while(*s)
	{
		s=COM_ParseToken(s,",");
		if (!Q_strcasecmp(com_token, ","))
			i = -1;
		else for (i = 0; i < countof(banflags); i++)
		{
			if (!Q_strcasecmp(com_token, banflags[i].names[0]) || (banflags[i].names[1] && !Q_strcasecmp(com_token, banflags[i].names[1])))
			{
				clearbanflags |= banflags[i].banflag;
				break;
			}
		}
		if (i == countof(banflags))
			Con_Printf("Unknown ban/penalty flag: %s. ignoring.\n", com_token);
	}
	//if no flags were specified, assume all
	if (!clearbanflags)
		clearbanflags = ~0u;

	for (link = &svs.bannedips ; (nb = *link) ; )
	{
		if ((nb->banflags & clearbanflags) && (all || (NET_CompareAdr(&nb->adr, &unbanadr) && NET_CompareAdr(&nb->adrmask, &unbanmask))))
		{
			found = true;
			if (!all)
				Con_Printf("unfiltered %s\n", NET_AdrToStringMasked(adr, sizeof(adr), &nb->adr, &nb->adrmask));

			nf = nb->banflags & clearbanflags;
			nb->banflags -= nf;
			if (!nb->banflags)
			{
				//this entry no longer has any flags
				*link = nb->next;
				Z_Free(nb);
			}
			else
				link = &(*link)->next;
		}
		else
		{
			link = &(*link)->next;
		}
	}

	if (!all && !found)
		Con_Printf("address was not filtered\n");

	if (found)
	{
		reevaluatebans = true;
		SV_KillExpiredBans();
	}
}
static void SV_PenaltyToggle (unsigned int banflag, char *penaltyname)
{
	char *clname = Cmd_Argv(1);
	char *duration = Cmd_Argv(2);
	char *reason = Cmd_Argv(3);
	bannedips_t proto = {0};
	client_t *cl;
	qboolean found = false;
	int clnum=-1;
	netadr_t tmp;

	proto.banflags = banflag;

	if (*duration == '+')
		proto.expiretime = SV_BanTime() + strtoull(duration+1, &duration, 0);
	else
		proto.expiretime = strtoull(duration, &duration, 0);

	//both of these should work
	//cuff foo "cos they're morons"
	//cuff foo +10 "cos they're morons"
	if (!*reason && *duration)
		reason = duration;

	memset(&proto.adrmask.address, 0xff, sizeof(proto.adrmask.address));
	while((cl = SV_GetClientForString(clname, &clnum)))
	{
		found = true;
		proto.adr = *NET_IPV4ify(&cl->netchan.remote_address, &tmp);
		proto.adr.port = 0;
		proto.adrmask.type = proto.adr.type;

		if (NET_IsLoopBackAddress(&proto.adr) && (proto.banflags & BAN_NOLOCALHOST))
		{
			Con_Printf("You're not allowed to filter loopback!\n");
			continue;
		}

		switch(SV_ToggleBan(&proto, reason))
		{
		case 1:
			Con_Printf("%s: %s is now %s\n", Cmd_Argv(0), cl->name, penaltyname);
			break;
		case 0:
			Con_Printf("%s: %s is no longer %s\n", Cmd_Argv(0), cl->name, penaltyname);
			break;
		default:
		case -1:
			Con_Printf("%s: %s already %s\n", Cmd_Argv(0), cl->name, penaltyname);
			break;
		}
	}
	if (!found)
		Con_Printf("%s: no clients\n", Cmd_Argv(0));
}

void SV_AutoAddPenalty (client_t *cl, unsigned int banflag, int duration, char *reason)
{
	bannedips_t proto;

	proto.banflags = banflag;
	proto.expiretime = SV_BanTime() + duration;
	memset(&proto.adrmask.address, 0xff, sizeof(proto.adrmask.address));
	proto.adr = cl->netchan.remote_address;
	proto.adr.port = 0;
	proto.adrmask.type = proto.adr.type;

	SV_AddBanEntry(&proto, reason);

	for (cl = (cl->controller?cl->controller:cl); cl; cl = cl->controlled)
		SV_EvaluatePenalties(cl);
}
void SV_AutoBanSender (int duration, char *reason)
{
	bannedips_t proto;

	proto.banflags = BAN_BAN;
	proto.expiretime = SV_BanTime() + duration;
	memset(&proto.adrmask.address, 0xff, sizeof(proto.adrmask.address));
	proto.adr = net_from;
	proto.adr.port = 0;
	proto.adrmask.type = proto.adr.type;

	SV_AddBanEntry(&proto, reason);
}

static void SV_WriteIP_f (void)
{
	vfsfile_t	*f;
	char	name[MAX_OSPATH];
	bannedips_t *bi;
	char *s;
	char adr[MAX_ADR_SIZE];
	char banflagtext[1024];
	int i;

	SV_KillExpiredBans();

	strcpy (name, "listip.cfg");

	Con_Printf ("Writing %s.\n", name);

	f = FS_OpenVFS(name, "wb", FS_GAMEONLY);
	if (!f)
	{
		Con_Printf ("Couldn't open %s\n", name);
		return;
	}

	bi = svs.bannedips;
	while (bi)
	{
		*banflagtext = 0;
		for (i = 0; i < countof(banflags); i++)
		{
			if (bi->banflags & banflags[i].banflag)
			{
				if (*banflagtext)
					Q_strncatz(banflagtext, ",", sizeof(banflagtext));
				Q_strncatz(banflagtext, banflags[i].names[0], sizeof(banflagtext));
			}
		}
		if (bi->reason[0])
			s = va("addip %s %s %"PRIu64" \"%s\"\n", NET_AdrToStringMasked(adr, sizeof(adr), &bi->adr, &bi->adrmask), banflagtext, (quint64_t) bi->expiretime, bi->reason);
		else if (bi->expiretime)
			s = va("addip %s %s %"PRIu64"\n", NET_AdrToStringMasked(adr, sizeof(adr), &bi->adr, &bi->adrmask), banflagtext, (quint64_t) bi->expiretime);
		else
			s = va("addip %s %s\n", NET_AdrToStringMasked(adr, sizeof(adr), &bi->adr, &bi->adrmask), banflagtext);
		VFS_WRITE(f, s, strlen(s));
		bi = bi->next;
	}

	VFS_CLOSE (f);
}


static void SV_ForceName_f (void)
{
	client_t	*cl;
	int clnum=-1;

	while((cl = SV_GetClientForString(Cmd_Argv(1), &clnum)))
	{
		InfoBuf_SetKey(&cl->userinfo, "name", Cmd_Argv(2));
		SV_LogPlayer(cl, "name forced");
		SV_ExtractFromUserinfo(cl, true);
		Q_strncpyz(cl->name, Cmd_Argv(2), sizeof(cl->namebuf));
		SV_BroadcastUserinfoChange(cl, true, "name", cl->name);
		return;
	}

	if (clnum == -1)
		Con_TPrintf ("Couldn't find user number %s\n", Cmd_Argv(1));
}

static void SV_CripplePlayer_f (void)
{
	SV_PenaltyToggle(BAN_CRIPPLED, "crippled");
}

static void SV_Mute_f (void)
{
	SV_PenaltyToggle(BAN_MUTE, "muted");
}
static void SV_StealthMute_f (void)
{
	SV_PenaltyToggle(BAN_MUTE|BAN_STEALTH, "stealth-muted");
}

static void SV_Cuff_f (void)
{
	SV_PenaltyToggle(BAN_CUFF, "cuffed");
}

static void SV_BanClientIP_f (void)
{
	SV_PenaltyToggle(BAN_BAN, "banned");
}

static void SV_Floodprot_f(void)
{
	extern cvar_t sv_floodprotect;
	extern cvar_t sv_floodprotect_messages;
	extern cvar_t sv_floodprotect_interval;
	extern cvar_t sv_floodprotect_silencetime;

	if (Cmd_Argc() == 1)
	{
		if (sv_floodprotect_messages.value <= 0 || !sv_floodprotect.value)
			Con_Printf("Flood protection is off.\n");
		else
			Con_Printf("Current flood protection settings: \nAfter %g msgs for %g seconds, silence for %g seconds\n",
				sv_floodprotect_messages.value,
				sv_floodprotect_interval.value,
				sv_floodprotect_silencetime.value);
		return;
	}

	if (Cmd_Argc() != 4)
	{
		Con_Printf("Usage: %s <messagerate> <ratepersecond> <silencetime>\n", Cmd_Argv(0));
		return;
	}

	Cvar_SetValue(&sv_floodprotect_messages, atof(Cmd_Argv(1)));
	Cvar_SetValue(&sv_floodprotect_interval, atof(Cmd_Argv(2)));
	Cvar_SetValue(&sv_floodprotect_silencetime, atof(Cmd_Argv(3)));
}

static void SV_StuffToClient_f(void)
{	//with this we emulate the progs 'stuffcmds' builtin

	client_t	*cl;

	int clnum=-1;
	char *clientname = Cmd_Argv(1);
	char *str;
	char *c;
	char *key;

	if (Cmd_Argc() < 3)
	{
		Con_Printf("%s <clientname> <consolecommand>\n", Cmd_Argv(0));
		return;
	}

	Cmd_ShiftArgs(1, Cmd_ExecLevel==RESTRICT_LOCAL);
	if (!strcmp(Cmd_Argv(1), "bind"))
	{
		key = Z_Malloc(strlen(Cmd_Argv(2))+1);
		strcpy(key, Cmd_Argv(2));
		Cmd_ShiftArgs(2, Cmd_ExecLevel==RESTRICT_LOCAL);
	}
	else
		key = NULL;
	str = Cmd_Args();

	while(*str <= ' ')	//strim leading spaces
	{
		if (!*str)
			break;
		str++;
	}

	//a list of safe, allowed commands. Allows any extention of this.
	if (strchr(str, '\n') || strchr(str, ';') || (
		!strncmp(str, "setinfo", 7) &&
		!strncmp(str, "quit", 4) &&
		!strncmp(str, "gl_fb", 5) &&
		!strncmp(str, "r_fb", 4) &&
		!strncmp(str, "say", 3) &&	//note that the say parsing could be useful here.
		!strncmp(str, "echo", 4) &&
		!strncmp(str, "name", 4) &&
		!strncmp(str, "skin", 4) &&
		!strncmp(str, "color", 5) &&
		!strncmp(str, "cmd", 3) &&
		!strncmp(str, "fov", 3) &&
		!strncmp(str, "connect", 7) &&
		!strncmp(str, "rate", 4) &&
		!strncmp(str, "cd", 2) &&
		!strncmp(str, "easyrecord", 10) &&
		!strncmp(str, "leftisright", 11) &&
		!strncmp(str, "menu_", 5) &&
		!strncmp(str, "r_fullbright", 12) &&
		!strncmp(str, "toggleconsole", 13) &&
		!strncmp(str, "v_i", 3) &&	//idlescale vars
		!strncmp(str, "bf", 2) &&
		!strncmp(str, "+", 1) &&
		!strncmp(str, "-", 1) &&
		!strncmp(str, "impulse", 7) &&
		1))
	{
		Con_Printf("You're not allowed to stuffcmd that\n");

		if (key)
			Z_Free(key);
		return;
	}

	while((cl = SV_GetClientForString(clientname, &clnum)))
	{
		if (ISQ2CLIENT(cl))
			ClientReliableWrite_Begin (cl, svcq2_stufftext, 3+strlen(str) + (key?strlen(key)+6:0));
		else
			ClientReliableWrite_Begin (cl, svc_stufftext, 3+strlen(str) + (key?strlen(key)+6:0));

		if (key)
		{
			for (c = "bind "; *c; c++)
				ClientReliableWrite_Byte (cl, *c);

			for (c = key; *c; c++)
				ClientReliableWrite_Byte (cl, *c);

			ClientReliableWrite_Byte (cl, ' ');
		}

		for (c = str; *c; c++)
			ClientReliableWrite_Byte (cl, *c);
		ClientReliableWrite_Byte (cl, '\n');
		ClientReliableWrite_Byte (cl, '\0');
	}

	if (key)
		Z_Free(key);
}

static char *ShowTime(unsigned int seconds)
{
	char buf[1024];
	char *b = buf;
	*b = 0;

	if (seconds > 60)
	{
		if (seconds > 60*60)
		{
			if (seconds > 24*60*60)
			{
				strcpy(b, va("%id ", seconds/(24*60*60)));
				b += strlen(b);
				seconds %= 24*60*60;
			}

			strcpy(b, va("%ih ", seconds/(60*60)));
			b += strlen(b);
			seconds %= 60*60;
		}
		strcpy(b, va("%im ", seconds/60));
		b += strlen(b);
		seconds %= 60;
	}
	strcpy(b, va("%is", seconds));
	b += strlen(b);

	return va("%s", buf);
}

/*
================
SV_Status_f
================
*/
const char *SV_ProtocolNameForClient(client_t *cl);
static void SV_Status_f (void)
{
	int			i;
	client_t	*cl;
	float		cpu;
	char		*s, *sec;
	const char	*p;
	char		adr[MAX_ADR_SIZE];
	float pi, po, bi, bo;

	int columns = 80;
	extern cvar_t sv_listen_qw;
#if defined(TCPCONNECT) && !defined(CLIENTONLY)
	#if defined(HAVE_SSL)
		extern cvar_t net_enable_tls;
	#endif
	#ifdef HAVE_HTTPSV
		extern cvar_t net_enable_http, net_enable_rtcbroker, net_enable_websockets;
	#endif
	extern cvar_t net_enable_qizmo, net_enable_qtv;
#endif
#ifdef NQPROT
	extern cvar_t sv_listen_nq, sv_listen_dp;
#endif
#ifdef QWOVERQ3
	extern cvar_t sv_listen_q3;
#endif

#ifndef SERVERONLY
	if (!sv.state && cls.state >= ca_connected && !cls.demoplayback && cls.protocol == CP_NETQUAKE)
	{	//nq can normally forward the request to the server.
		Cmd_ForwardToServer();
		return;
	}
#endif

	if (sv_redirected != RD_OBLIVION && (sv_redirected != RD_NONE
#ifndef SERVERONLY
		|| (vid.width < 68*8 && qrenderer != QR_NONE)
#endif
		))
		columns = 40;

	NET_PrintAddresses(svs.sockets);

	if (!sv.state)
	{
		Con_TPrintf("Server is not running\n");
		return;
	}

	if (Cmd_Argc()>1)
		columns = atoi(Cmd_Argv(1));

	cpu = (svs.stats.latched_active+svs.stats.latched_idle);
	if (cpu)
		cpu = 100*svs.stats.latched_active/cpu;

	Con_TPrintf("cpu utilization  : %3i%%\n",(int)cpu);
	if (sv.state == ss_active)
	{
		Con_TPrintf("avg response time: %i ms (%i max)\n",(int)(1000*svs.stats.latched_active/svs.stats.latched_count), (int)(1000*svs.stats.latched_maxresponse));
		Con_TPrintf("packets/frame    : %5.2f (%i max)\n", (float)svs.stats.latched_packets/svs.stats.latched_count, svs.stats.latched_maxpackets);	//not relevent as a limit.
	}
	if (NET_GetRates(svs.sockets, &pi, &po, &bi, &bo))
		Con_TPrintf("packets,bytes/sec: in: %g %g  out: %g %g\n", pi, bi, po, bo);	//not relevent as a limit.
	Con_TPrintf("server uptime    : %s\n", ShowTime(realtime));
	if (sv_public.ival < 0)
		s = "hidden";
	else if (sv_public.ival == 2)
		s = "hole punching";
	else if (sv_public.ival)
		s = "direct";
	else
		s = "private";
	Con_TPrintf("public           : %s\n", localtext(s));

	switch(svs.gametype)
	{
#ifdef Q3SERVER
	case GT_QUAKE3:
		Con_TPrintf("client types     :%s\n", sv_listen_qw.ival?" Q3":"");
		break;
#endif
#ifdef Q2SERVER
	case GT_QUAKE2:
		Con_TPrintf("client types     :%s\n", sv_listen_qw.ival?" Q2":"");
		break;
#endif

	default:
		Con_TPrintf("client types     :%s", sv_listen_qw.ival?" ^[QW\\tip\\This is "FULLENGINENAME"'s standard protocol.^]":"");
#ifdef NQPROT
		Con_TPrintf("%s%s", (sv_listen_nq.ival==2)?" ^[NQ+\\tip\\Allows 'Net'/'Normal' Quake clients to connect, with cookies and extensions that might confuse some old clients^]":(sv_listen_nq.ival?" ^[NQ(15)\\tip\\Vanilla/Normal Quake protocol with maximum compatibility^]":""), sv_listen_dp.ival?" ^[DP\\tip\\Explicitly recognise connection requests from DP clients, no handshakes.^]":"");
#endif
#ifdef QWOVERQ3
		if (sv_listen_q3.ival) Con_Printf(" Q3");
#endif
#ifdef HAVE_DTLS
		if (svs.sockets && svs.sockets->dtlsfuncs)
		{
			if (net_enable_dtls.ival >= 3)
				Con_TPrintf(" ^[DTLS-only\\tip\\Insecure clients (those without support for DTLS) will be barred from connecting.^]");
			else if (net_enable_dtls.ival)
				Con_TPrintf(" ^[DTLS\\tip\\Clients may optionally connect via DTLS for added security^]");
		}
#endif
		Con_Printf("\n");
#if defined(TCPCONNECT) && !defined(CLIENTONLY)
		Con_TPrintf("tcp services     :");

		if (svs.sockets)
		{
			int i, m;
			netadr_t	addr[64];
			struct ftenet_generic_connection_s			*con[sizeof(addr)/sizeof(addr[0])];
			int			flags[sizeof(addr)/sizeof(addr[0])];
			const char *params[sizeof(addr)/sizeof(addr[0])];
			m = NET_EnumerateAddresses(svs.sockets, con, flags, addr, params, sizeof(addr)/sizeof(addr[0]));
			for (i = 0; i < m; i++)
			{
				if (!con[i]->islisten)
					continue;	//wut?
				if (addr[i].prot == NP_STREAM)
					break;
			}
			if (i == m)
			{
				Con_Printf(S_COLOR_GRAY" <No TCP ports open>\n");
				break;
			}
		}
#if defined(HAVE_SSL)
		if (net_enable_tls.ival)
			Con_TPrintf(" ^[TLS\\tip\\Clients are able to connect with Transport Layer Security for the other services, allowing for the use of tls://, wss:// or https:// schemes when their underlaying protocol is enabled.^]");
#endif
#ifdef HAVE_HTTPSV
		if (net_enable_http.ival)
			Con_TPrintf(" ^[HTTP\\tip\\This server also acts as a web server. This might be useful to allow hosting demos or stats.^]");
		if (net_enable_rtcbroker.ival)
			Con_TPrintf(" ^[RTC\\tip\\This server is set up to act as a webrtc broker, allowing clients+servers to locate each other instead of playing on this server.^]");
		if (net_enable_websockets.ival)
			Con_TPrintf(" ^[WebSocket\\tip\\Clients can use the ws:// or possibly wss:// schemes to connect to this server, potentially from browser ports. This may be laggy.^]");
#endif
		if (net_enable_qizmo.ival)
			Con_TPrintf(" ^[Qizmo\\tip\\Compatible with the tcp connection feature of qizmo, equivelent to 'connect tcp://ip:port' in FTE.^]");
		if (net_enable_qtv.ival)
			Con_TPrintf(" ^[QTV\\tip\\Allows receiving streamed mvd data from this server.^]");
		Con_Printf("\n");
#endif
		break;
	}
#ifdef SUBSERVERS
	if (sv.state == ss_clustermode)
	{
		MSV_Status();
		return;
	}
#endif
	Con_TPrintf("map uptime       : %s\n", ShowTime(sv.world.physicstime));
	//show the current map+name (but hide name if its too long or would be ugly)
	if (columns >= 80 && *sv.mapname && strlen(sv.mapname) < 45 && !strchr(sv.mapname, '\n'))
		Con_TPrintf ("current map      : %s "S_COLOR_GRAY"(%s)\n", svs.name, sv.mapname);
	else
		Con_TPrintf ("current map      : %s\n", svs.name);

	if (svs.gametype == GT_PROGS)
	{
		int count = 0, i;
		edict_t *e;
		for (i = 0; i < sv.world.num_edicts; i++)
		{
			e = EDICT_NUM_PB(svprogfuncs, i);
			if (e && e->ereftype == ER_FREE && sv.time - e->freetime > 0.5)
				continue;	//free, and older than the zombie time
			count++;
		}
		Con_TPrintf("entities         : %i/%i/%i (mem: %.1f%%)\n", count, sv.world.num_edicts, sv.world.max_edicts, 100.0*(sv.world.progs->stringtablesize/(double)sv.world.progs->stringtablemaxsize));
		for (count = 1; count < MAX_PRECACHE_MODELS; count++)
			if (!sv.strings.model_precache[count])
				break;
		Con_TPrintf("models           : %i/%i\n", count, MAX_PRECACHE_MODELS);
		for (count = 1; count < MAX_PRECACHE_SOUNDS; count++)
			if (!sv.strings.sound_precache[count])
				break;
		Con_TPrintf("sounds           : %i/%i\n", count, MAX_PRECACHE_SOUNDS);

		for (count = 1; count < MAX_SSPARTICLESPRE; count++)
			if (!sv.strings.particle_precache[count])
				break;
		if (count!=1)
			Con_TPrintf("particles        : %i/%i\n", count, MAX_SSPARTICLESPRE);
	}
	if (!strcmp(FS_GetGamedir(true), InfoBuf_ValueForKey(&svs.info, "*gamedir")))
		Con_TPrintf("gamedir          : %s\n", FS_GetGamedir(true));
	else
		Con_TPrintf("gamedir          : %s"S_COLOR_GRAY" (%s)\n", FS_GetGamedir(true), InfoBuf_ValueForKey(&svs.info, "*gamedir"));
	if (sv_csqcdebug.ival)
		Con_TPrintf("csqc debug       : true\n");
#ifdef MVD_RECORDING
	SV_Demo_PrintOutputs();
#endif
	NET_PrintConnectionsStatus(svs.sockets);


// min fps lat drp
	if (columns < 80)
	{
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		Con_TPrintf (	"name               userid frags\n"
						"  address          rate ping drop\n"
						"  ---------------- ---- ---- -----\n");
		for (i=0,cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
		{
			if (!cl->state)
				continue;

			Con_Printf ("%-16.16s  ", cl->name);

			Con_Printf ("%6i %5i", cl->userid, (int)cl->old_frags);
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else
				Con_Printf("\n");

			if (cl->state == cs_loadzombie)
			{
				if (cl->istobeloaded)
					s = "LoadZombie";
				else
					s = "ParmZombie";
			}
			else if (cl->reversedns)
				s = cl->reversedns;
			else if (cl->state == cs_zombie && cl->netchan.remote_address.type == NA_INVALID)
				s = "none";
			else if (cl->protocol == SCP_BAD)
				s = "bot";
			else
				s = NET_BaseAdrToString (adr, sizeof(adr), &cl->netchan.remote_address);
			Con_Printf ("  %-16.16s", s);
			if (cl->state == cs_connected)
			{
				Con_TPrintf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie || cl->state == cs_loadzombie)
			{
				Con_TPrintf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %5.2f\n"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl, false)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence);
		}
	}
	else
	{
#define COLUMNS C_FRAGS C_USERID C_ADDRESS C_NAME C_RATE C_PING C_DROP C_DLP C_DLS C_PROT C_ADDRESS2
#define C_FRAGS		COLUMN(0, "frags", if (cl->spectator==1)Con_Printf("%-5s ", "spec"); else Con_Printf("%5i ", (int)cl->old_frags))
#define C_USERID	COLUMN(1, "userid", Con_Printf("%6i ", (int)cl->userid))
#define C_ADDRESS	COLUMN(2, "address        ", Con_Printf("%s%-16.16s", sec, s))
#define C_NAME		COLUMN(3, "name           ", Con_Printf("%-16.16s", cl->name))
#define C_RATE		COLUMN(4, "  hz", Con_Printf("%4i ", (cl->frameunion.frames&&cl->netchan.frame_rate>0)?(int)(0.5f+1/cl->netchan.frame_rate):0))
#define C_PING		COLUMN(5, "ping", Con_Printf("%4i ", (int)SV_CalcPing (cl, false)))
#define C_DROP		COLUMN(6, "drop", Con_Printf("%4.1f ", 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence))
#define C_DLP		COLUMN(7, "dl ", if (!cl->download||!cl->downloadsize)Con_Printf("    ");else Con_Printf("%3.0f ", (cl->downloadcount*100.0)/cl->downloadsize))
#define C_DLS		COLUMN(8, "dls", if (!cl->download)Con_Printf("    ");else Con_Printf("%3u ", (unsigned int)(cl->downloadsize/1024)))
#define C_PROT		COLUMN(9, "prot ", Con_Printf("%-6.5s", p))
#define C_MODELSKIN	COLUMN(11, "model/skin     ", Con_Printf("%s", s))
#define C_ADDRESS2	COLUMN(10, "address        ", Con_Printf("%s", s))

		int columns = (1<<4)-1;

		for (i=0,cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
		{
			if (!cl->state)
				continue;

			if (cl->netchan.drop_count)
				columns |= 1<<6;
			if (cl->download)
			{
				columns |= 1<<7;
				columns |= 1<<8;
			}
			if (cl->frameunion.frames&&cl->netchan.frame_rate>0)
				columns |= 1<<4;
			if (cl->netchan.remote_address.type > NA_LOOPBACK)
				columns |= 1<<5;
			if (cl->protocol != SCP_BAD && (cl->protocol >= SCP_NETQUAKE || cl->spectator || (cl->protocol == SCP_QUAKEWORLD && !(cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))))
				columns |= 1<<9;
			if ((cl->netchan.remote_address.type == NA_IPV6 && memcmp(cl->netchan.remote_address.address.ip6, "\0\0\0\0""\0\0\0\0""\0\0\xff\xff", 12))||cl->reversedns)
				columns |= (1<<10);
		}
		if (columns&(1<<10))	//if address2, remove the limited length addresses.
			columns &= ~(1<<2);

#define COLUMN(f,t,v) if (columns&(1<<f)) Con_Printf(t" ");
		COLUMNS
#undef  COLUMN
		Con_Printf("\n");
#define COLUMN(f,t,v)  if (columns&(1<<f)){for (i = 0; i < sizeof(t)-1; i++) Con_Printf("-"); Con_Printf(" ");}
		COLUMNS
#undef  COLUMN
		Con_Printf("\n");

//		Con_Printf ("frags userid name            rate ping drop "" dl%% dls"" address         \n");
//		Con_Printf ("----- ------ --------------- ---- ---- -----"" --- ---"" --------------- \n");
		for (i=0,cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
		{
			if (!cl->state)
				continue;


			if (cl->state == cs_loadzombie)
			{	//loadzombies have no specific address
				if (cl->istobeloaded)
					s = "LoadZombie";
				else
					s = "ParmZombie";
			}
			else if (cl->reversedns)
				s = cl->reversedns;
			else if (cl->state == cs_zombie && cl->netchan.remote_address.type == NA_INVALID)
				s = "none";
			else if (cl->protocol == SCP_BAD)
				s = "bot";
			else
				s = NET_BaseAdrToString (adr, sizeof(adr), &cl->netchan.remote_address);

			if (NET_IsLoopBackAddress(&cl->netchan.remote_address))
				sec = "";
			else if (NET_IsEncrypted(&cl->netchan.remote_address))
				sec = S_COLOR_GREEN;
			else
				sec = S_COLOR_RED;

			p = SV_ProtocolNameForClient(cl);
			if (cl->state == cs_connected && cl->protocol>=SCP_NETQUAKE)
				p = "nq";	//not actually known yet.
			else if (cl->state == cs_zombie || cl->state == cs_loadzombie)
				p = "zom";


#define COLUMN(f,t,v)  if (columns&(1<<f)){v;}
			COLUMNS
#undef  COLUMN

			Con_Printf("\n");
		}
	}
	Con_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	Q_strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[Q_strlen(p)-1] = 0;
	}

	Q_strcat(text, p);

	for (j = 0, client = svs.clients; j < svs.allocated_client_slots; j++, client++)
	{
		if (client->state == cs_free)
			continue;
		if (client->penalties & BAN_DEAF)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		sizebuf_t *msg;
		msg = MVDWrite_Begin (dem_all, 0, strlen(text)+4);
		MSG_WriteByte (msg, svc_print);
		MSG_WriteByte (msg, PRINT_CHAT);
		for (j = 0; text[j]; j++)
			MSG_WriteChar(msg, text[j]);
		MSG_WriteChar(msg, '\n');
		MSG_WriteChar(msg, 0);
	}
#endif
}

static void SV_ConSayOne_f (void)
{
	char	text[2048];
	client_t	*to;
	int i;
	char *s;
	int clnum=-1;

	if (Cmd_Argc () < 3)
		return;

	while((to = SV_GetClientForString(Cmd_Argv(1), &clnum)))
	{
		Q_strcpy (text, "{console}: ");

		for (i = 2; ; i++)
		{
			s = Cmd_Argv(i);
			if (!*s)
				break;

			if (strlen(text) + strlen(s) + 2 >= sizeof(text)-1)
				break;
			strcat(text, " ");
			strcat(text, s);
		}
		strcat(text, "\n");
		SV_ClientPrintf(to, PRINT_CHAT, "%s", text);
	}
	if (!clnum)
		Con_TPrintf("Couldn't find user number %s\n", Cmd_Argv(1));
}

/*
==================
SV_Heartbeat_f
==================
*/
static void SV_Heartbeat_f (void)
{
	SV_Master_ReResolve();
}

/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void)
{
	cvar_t	*var;
	char value[512];
	int i;

	if (Cmd_Argc() == 1)
	{
		Con_TPrintf ("Server info settings:\n");
		InfoBuf_Print (&svs.info, "");
		Con_Printf("[%u]\n", (unsigned int)svs.info.totalsize);
		return;
	}

	if (Cmd_Argc() < 3)
	{
		Con_TPrintf ("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		if (!strcmp(Cmd_Argv(1), "*"))
			if (!strcmp(Cmd_Argv(2), ""))
			{	//clear it out
				const char *k;
				for(i=0;;)
				{
					k = InfoBuf_KeyForNumber(&svs.info, i);
					if (!k)
						break;	//no more.
					else if (*k == '*')
						i++;	//can't remove * keys
					else if ((var = Cvar_FindVar(k)) && var->flags&CVAR_SERVERINFO)
						i++;	//this one is a cvar.
					else
						InfoBuf_RemoveKey(&svs.info, k);	//we can remove this one though, so yay.
				}

				return;
			}
		Con_TPrintf ("Can't set * keys\n");
		return;
	}

	if (!strcmp(Cmd_Argv(0), "serverinfoblob"))
	{
		qofs_t fsize;
		char *data = FS_MallocFile(Cmd_Argv(2), FS_GAME, &fsize);
		if (!data)
		{
			Con_TPrintf ("Unable to read %s\n", Cmd_Argv(2));
			return;
		}
		if (fsize > 64*1024*1024)
			Con_TPrintf ("File is over 64mb\n");
		else
			InfoBuf_SetStarBlobKey(&svs.info, Cmd_Argv(1), data, fsize);
		FS_FreeFile(data);
	}
	else
	{
		Q_strncpyz(value, Cmd_Argv(2), sizeof(value));
		value[sizeof(value)-1] = '\0';
		for (i = 3; i < Cmd_Argc(); i++)
		{
			strncat(value, " ", sizeof(value)-1);
			strncat(value, Cmd_Argv(i), sizeof(value)-1);
		}

		InfoBuf_SetValueForKey (&svs.info, Cmd_Argv(1), value);
	}

	// if this is a cvar, change it too
	var = Cvar_FindVar (Cmd_Argv(1));
	if (var)
	{
		Cvar_Set(var, value);
/*		Z_Free (var->string);	// free the old value string
		var->string = Z_StrDup (value);
		var->value = Q_atof (var->string);
*/	}
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
static void SV_Localinfo_f (void)
{
	char *old;

	if (Cmd_Argc() == 1)
	{
		Con_TPrintf ("Local info settings:\n");
		InfoBuf_Print (&svs.localinfo, "");
		Con_Printf("[%u]\n", (unsigned int)svs.localinfo.totalsize);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_TPrintf ("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		if (!strcmp(Cmd_Argv(1), "*"))
			if (!strcmp(Cmd_Argv(2), ""))
			{	//clear it out
				InfoBuf_Clear(&svs.localinfo, false);
				return;
			}
		Con_TPrintf ("Can't set * keys\n");
		return;
	}
	old = InfoBuf_ValueForKey(&svs.localinfo, Cmd_Argv(1));
	InfoBuf_SetValueForKey (&svs.localinfo, Cmd_Argv(1), Cmd_Argv(2));

	PR_LocalInfoChanged(Cmd_Argv(1), old, Cmd_Argv(2));

	Con_DPrintf("Localinfo %s changed (%s -> %s)\n", Cmd_Argv(1), old, Cmd_Argv(2));
}

void SV_SaveInfos(vfsfile_t *f)
{
	VFS_WRITE(f, "\n", 1);
	VFS_WRITE(f, "serverinfo * \"\"\n", 16);
	InfoBuf_WriteToFile(f, &svs.info, "serverinfo", CVAR_SERVERINFO);
	VFS_WRITE(f, "\n", 1);
	VFS_WRITE(f, "localinfo * \"\"\n", 15);
	InfoBuf_WriteToFile(f, &svs.localinfo, "localinfo", 0);
}

/*
void SV_ResetInfos(void)
{
	// TODO: add me
}
*/

/*
===========
SV_User_f

Examine a users info strings
===========
*/
void SV_User_f (void)
{
	double ftime, minf, maxf;
	int frames;
	client_t	*cl;
	int clnum=-1;
	unsigned int u;
	char buf[8192];
	qbyte digest[DIGEST_MAXSIZE];
	int certsize;
	extern cvar_t sv_userinfo_bytelimit, sv_userinfo_keylimit;
	static const char *pext1names[32] = {	"setview",		"scale",	"lightstylecol",	"trans",		"view2",		"builletens",	"accuratetimings",	"sounddbl",
											"fatness",		"hlbsp",	"bullet",			"hullsize",		"modeldbl",		"entitydbl",	"entitydbl2",		"floatcoords",
											"OLD vweap",	"q2bsp",	"q3bsp",			"colormod",		"splitscreen",	"hexen2",		"spawnstatic2",		"customtempeffects",
											"packents",		"UNKNOWN",	"showpic",			"setattachment","UNKNOWN",		"chunkeddls",	"csqc",				"dpflags"};
	static const char *pext2names[32] = {	"prydoncursor",	"voip",		"setangledelta",	"rplcdeltas",	"maxplayers",	"predinfo",		"sizeenc",			"infoblobs",
											"stunaware",	"vrinputs",	"UNKNOWN",			"UNKNOWN",		"UNKNOWN",		"UNKNOWN",		"UNKNOWN",			"UNKNOWN",
											"UNKNOWN",		"UNKNOWN",	"UNKNOWN",			"UNKNOWN",		"UNKNOWN",		"UNKNOWN",		"UNKNOWN",			"UNKNOWN",
											"UNKNOWN",		"UNKNOWN",	"UNKNOWN",			"UNKNOWN",		"UNKNOWN",		"UNKNOWN",		"UNKNOWN",			"UNKNOWN"};

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("Usage: info <userid>\n");
		return;
	}

	while((cl = SV_GetClientForString(Cmd_Argv(1), &clnum)))
	{
		Con_TPrintf("Userinfo (%i):\n", cl->userid);
		InfoBuf_Print (&cl->userinfo, "  ");
		Con_Printf("[%u/%i, %u/%i]\n", (unsigned)cl->userinfo.totalsize, sv_userinfo_bytelimit.ival, (unsigned)cl->userinfo.numkeys, sv_userinfo_keylimit.ival);
		safeswitch(cl->protocol)
		{
		case SCP_BAD:
			Con_Printf("protocol: bot/invalid\n");
			continue;
		case SCP_QUAKEWORLD:	//branding is everything...
			if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
				Con_Printf("protocol: fteqw-nack\n");
			else
				Con_Printf("protocol: quakeworld\n");
			break;
		case SCP_QUAKE2:
			Con_Printf("protocol: quake2\n");
			break;
		case SCP_QUAKE2EX:
			Con_Printf("protocol: quake2ex\n");
			break;
		case SCP_QUAKE3:
			Con_Printf("protocol: quake3\n");
			break;
		case SCP_NETQUAKE:
			if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
				Con_Printf("protocol: ftenq-nack\n");
			else
				Con_Printf("protocol: (net)quake\n");
			break;
		case SCP_BJP3:
			Con_Printf("protocol: bjp3\n");
			break;
		case SCP_FITZ666:
			if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
				Con_Printf("protocol: fte666-nack\n");
			else
				Con_Printf("protocol: fitzquake 666\n");
			break;
		case SCP_DARKPLACES6:
			Con_Printf("protocol: dpp6\n");
			break;
		case SCP_DARKPLACES7:
			Con_Printf("protocol: dpp7\n");
			break;
		safedefault:
			Con_Printf("protocol: other (fixme)\n");
			break;
		}

		if (cl->fteprotocolextensions)
		{
			unsigned int effective = cl->fteprotocolextensions;
			if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)	//these flags were made obsolete. don't list them.
				effective &= ~(PEXT_SCALE|PEXT_TRANS|PEXT_ACCURATETIMINGS|PEXT_FATNESS|PEXT_HULLSIZE|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_COLOURMOD|PEXT_SPAWNSTATIC2|PEXT_SETATTACHMENT|PEXT_DPFLAGS);
			Con_Printf("pext1:");
			for (u = 0; u < 32; u++)
				if (effective & (1u<<u))
						Con_Printf(" %s", pext1names[u]);
			Con_Printf("\n");
		}
		if (cl->fteprotocolextensions2)
		{
			Con_Printf("pext2:");
			for (u = 0; u < 32; u++)
				if (cl->fteprotocolextensions2 & (1u<<u))
						Con_Printf(" %s", pext2names[u]);
			Con_Printf("\n");
		}

		Con_Printf("ip: %s%s\n", NET_IsEncrypted(&cl->netchan.remote_address)?S_COLOR_GREEN:S_COLOR_RED, NET_AdrToString(buf, sizeof(buf), &cl->netchan.remote_address));
		certsize = NET_GetConnectionCertificate(svs.sockets, &cl->netchan.remote_address, QCERT_PEERCERTIFICATE, buf, sizeof(buf));
		if (certsize <= 0)
			strcpy(buf, "<no certificate>");
		else
			Base64_EncodeBlockURI(digest,CalcHash(&hash_certfp, digest, sizeof(digest), buf, certsize), buf, sizeof(buf));
		Con_Printf("fp: %s\n", buf);
		if (NET_GetConnectionCertificate(svs.sockets, &cl->netchan.remote_address, QCERT_PEERSUBJECT, buf, sizeof(buf)) < 0)
			strcpy(buf, "<unavailable>");
		Con_Printf("dn: %s\n", buf);
		switch(cl->realip_status)
		{
		case 1:
			Con_Printf("realip: %s ("CON_WARNING"unverified"CON_DEFAULT")\n", NET_AdrToString(buf, sizeof(buf), &cl->realip));
			break;
		case 2:
			Con_Printf("realip: %s ("CON_ERROR"unverifiable"CON_DEFAULT")\n", NET_AdrToString(buf, sizeof(buf), &cl->realip));
			break;
		case 3:
			Con_Printf("realip: %s (verified)\n", NET_AdrToString(buf, sizeof(buf), &cl->realip));
			break;
		}
		if (*cl->guid)
			Con_Printf("guid: %s\n", cl->guid);
		if (cl->download)
			Con_Printf ("download: \"%s\" %uk/%uk (%g%%)", cl->downloadfn, (unsigned int)(cl->downloadcount/1024), (unsigned int)(cl->downloadsize/1024), (cl->downloadcount*100.0)/cl->downloadsize);

		if (cl->penalties & BAN_CRIPPLED)
			Con_Printf("crippled\n");
		if (cl->penalties & BAN_CUFF)
			Con_Printf("cuffed\n");
		if (cl->penalties & BAN_DEAF)
			Con_Printf("deaf\n");
		if (cl->penalties & BAN_LAGGED)
			Con_Printf("lagged\n");
		if (cl->penalties & BAN_MUTE)
			Con_Printf("muted\n");
		if (cl->penalties & BAN_VIP)
			Con_Printf("vip\n");

		SV_CalcNetRates(cl, &ftime, &frames, &minf, &maxf);
		if (frames)
			Con_Printf("net: %gfps (min%g max %g), c2s: %ibps, s2c: %ibps\n", ftime/frames, minf, maxf, (int)cl->inrate, (int)cl->outrate);
		else
			Con_Printf("net: unknown framerate, c2s: %ibps, s2c: %ibps\n", (int)cl->inrate, (int)cl->outrate);
	}

	if (clnum == -1)
		Con_TPrintf ("Userid %i is not on the server\n", atoi(Cmd_Argv(1)));
}

/*
================
SV_Floodport_f

Sets the gamedir and path to a different directory.
================
*/

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
static void SV_Gamedir (void)
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_TPrintf ("Current gamedir: %s\n", InfoBuf_ValueForKey (&svs.info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_TPrintf ("%s should be a single filename, not a path\n", Cmd_Argv(0));
		return;
	}

	InfoBuf_SetValueForStarKey (&svs.info, "*gamedir", dir);
}

static int QDECL CompleteGamedirPath (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	char dirname[MAX_QPATH];
	if (*name)
	{
		size_t l = strlen(name)-1;
		if (l < countof(dirname) && name[l] == '/')
		{	//directories are marked with an explicit trailing slash. because we're weird.
			memcpy(dirname, name, l);
			dirname[l] = 0;
			ctx->cb(dirname, NULL, NULL, ctx);
		}
	}
	return true;
}
static void SV_Gamedir_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	extern qboolean	com_homepathenabled;
	if (argn == 1)
	{
		if (com_homepathenabled)
			Sys_EnumerateFiles(com_homepath, va("%s*", partial), CompleteGamedirPath, ctx, NULL);
		Sys_EnumerateFiles(com_gamepath, va("%s*", partial), CompleteGamedirPath, ctx, NULL);
	}
}

/*
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
FIXME: should block this if we're on a server at the time
================
*/
static void SV_Gamedir_f (void)
{
	char			*dir;
	int argc = Cmd_Argc();

	if (argc == 1)
	{
		Con_TPrintf ("Current gamedir: %s\n", FS_GetGamedir(true));
		return;
	}

	if (argc < 2)
	{
		Con_TPrintf ("Usage: gamedir <newgamedir>\n");
		return;
	}

	if (argc == 2)
		dir = Z_StrDup(Cmd_Argv(1));
	else
	{
		int i;
		size_t l = 1;
		for (i = 1; i < argc; i++)
			l += strlen(Cmd_Argv(i))+1;
		dir = Z_Malloc(l);
		for (i = 1; i < argc; i++)
		{	//disgusting hack for quakespasm's "game extendedgame -missionpack" crap.
			//games with a leading hypen are inserted before others, with the hyphen ignored.
			if (*Cmd_Argv(i) != '-')
				continue;
			if (*dir)
				Q_strncatz(dir, ";", l);
			Q_strncatz(dir, Cmd_Argv(i)+1, l);
		}
		for (i = 1; i < argc; i++)
		{
			if (*Cmd_Argv(i) == '-')
				continue;
			if (*dir)
				Q_strncatz(dir, ";", l);
			Q_strncatz(dir, Cmd_Argv(i), l);
		}
	}

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_TPrintf ("%s should be a single filename, not a path\n", Cmd_Argv(0));
	}
	else
		COM_Gamedir (dir, NULL);
	Z_Free(dir);
}

/*
================
SV_Snap
================
*/
static void SV_Snap (int uid)
{
	client_t *cl;
	char		pcxname[80];
	char		checkname[MAX_OSPATH];
	int			i;

	for (i = 0, cl = svs.clients; i < svs.allocated_client_slots; i++, cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == uid)
			break;
	}
	if (i >= svs.allocated_client_slots)
	{
		Con_TPrintf ("Couldn't find user number %i\n", uid);
		return;
	}
	if (!ISQWCLIENT(cl))
	{
		Con_Printf("Can only snap QW clients\n");
		return;
	}

	sprintf(pcxname, "%d-00.pcx", uid);

	strcpy(checkname, "snap");

	for (i=0 ; i<=99 ; i++)
	{
		pcxname[strlen(pcxname) - 6] = i/10 + '0';
		pcxname[strlen(pcxname) - 5] = i%10 + '0';
		Q_snprintfz (checkname, sizeof(checkname), "snap/%s", pcxname);
		if (!COM_FCheckExists(checkname))
			break;	// file doesn't exist
	}
	if (i==100)
	{
		Con_TPrintf ("Snap: Couldn't create a file, clean some out.\n");
		return;
	}
	strcpy(cl->uploadfn, checkname);

	memcpy(&cl->snap_from, &net_from, sizeof(net_from));
	if (sv_redirected != RD_NONE)
		cl->remote_snap = true;
	else
		cl->remote_snap = false;

	ClientReliableWrite_Begin (cl, svc_stufftext, 24);
	ClientReliableWrite_String (cl, "cmd snap\n");
	Con_TPrintf ("Requesting snap from user %d...\n", uid);
}

/*
================
SV_Snap_f
================
*/
static void SV_Snap_f (void)
{
	int			uid;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("Usage:  snap <userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	SV_Snap(uid);
}

/*
================
SV_Snap
================
*/
static void SV_SnapAll_f (void)
{
	client_t *cl;
	int			i;

	for (i = 0, cl = svs.clients; i < svs.allocated_client_slots; i++, cl++)
	{
		if (cl->state < cs_connected || cl->spectator)
			continue;
		SV_Snap(cl->userid);
	}
}

static float mytimer;
static float lasttimer;
static int ticsleft;
static float timerinterval;
static int timerlevel;
static cvar_t *timercommand;
void SV_CheckTimer(void)
{
	float ctime = Sys_DoubleTime();
//	if (ctime < lasttimer) //new map? (shouldn't happen)
//		mytimer = ctime+5;	//trigger in a few secs
	lasttimer = ctime;

	if (ticsleft)
	{
		if (mytimer < ctime)
		{
			mytimer += timerinterval;
			if (ticsleft > 0)
				ticsleft--;

			if (timercommand)
			{
				Cbuf_AddText(timercommand->string, timerlevel);
				Cbuf_AddText("\n", timerlevel);
			}
		}
	}
}

static void SV_SetTimer_f(void)
{
	int count;
	float interval;
	char *command;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s <count> <interval> <command>\n", Cmd_Argv(0));
		return;
	}

	count = atoi(Cmd_Argv(1));
	interval = atof(Cmd_Argv(2));

	if (!count && Cmd_Argc() == 2)
	{
		ticsleft = 0;
		return;
	}

	if (interval <= 0 || (count <= 0 && count != -1))	//makes sure the args are right. :)
	{
		Con_Printf("%s count interval command\n", Cmd_Argv(0));
		return;
	}

	Cmd_ShiftArgs(2, Cmd_ExecLevel==RESTRICT_LOCAL);	//strip the two vars
	command = Cmd_Args();

	timercommand = Cvar_Get("sv_timer", "", CVAR_NOSET, NULL);
	Cvar_ForceSet(timercommand, command);

	mytimer = Sys_DoubleTime() + interval;
	ticsleft = count;
	timerinterval = interval;

	timerlevel = Cmd_ExecLevel;
}

static void SV_SendGameCommand_f(void)
{
#ifdef Q3SERVER
	if (q3)
		if (q3->sv.PrefixedConsoleCommand())
			return;
#endif

#ifdef VM_Q1
	if (Q1QVM_GameConsoleCommand())
		return;
#endif

	if (PR_ConsoleCmd(Cmd_Args()))
		return;

#ifdef Q2SERVER
	if (ge)
	{
		ge->ServerCommand();
	}
	else
#endif
		Con_Printf("Mod-specific command \"%s\" not known\n", Cmd_Argv(1));
}




void PIN_LoadMessages(void);
void PIN_SaveMessages(void);
void PIN_DeleteOldestMessage(void);
void PIN_MakeMessage(char *from, char *msg);

static void SV_Pin_Save_f(void)
{
	PIN_SaveMessages();
}
static void SV_Pin_Reload_f(void)
{
	PIN_LoadMessages();
}
static void SV_Pin_Delete_f(void)
{
	PIN_DeleteOldestMessage();
}
static void SV_Pin_Add_f(void)
{
	PIN_MakeMessage(Cmd_Argv(1), Cmd_Argv(2));
}

/*
void SV_ReallyEvilHack_f(void)
{
	int clnum = -1;
	client_t *cl;
	while((cl = SV_GetClientForString(Cmd_Argv(1), &clnum)))
	if (cl)
	{
		//kick them back to map selection, ish.
		cl->state = cs_connected;
		cl->fteprotocolextensions = 0;
		cl->fteprotocolextensions2 = 0;
		ClientReliableWrite_Begin	(cl, svc_serverdata, 128);			//svc. dur.
		ClientReliableWrite_Long	(cl, PROTOCOL_VERSION_QW);			//protocol
		ClientReliableWrite_Long	(cl, svs.spawncount);				//servercount
		ClientReliableWrite_String	(cl, ".");						//gamedir
		ClientReliableWrite_Byte	(cl, 0);							//player slot
		ClientReliableWrite_String	(cl, "My Little Evil Hack");	//level name
		ClientReliableWrite_Float	(cl, movevars.gravity);
		ClientReliableWrite_Float	(cl, movevars.stopspeed);
		ClientReliableWrite_Float	(cl, movevars.maxspeed);
		ClientReliableWrite_Float	(cl, movevars.spectatormaxspeed);
		ClientReliableWrite_Float	(cl, movevars.accelerate);
		ClientReliableWrite_Float	(cl, movevars.airaccelerate);
		ClientReliableWrite_Float	(cl, movevars.wateraccelerate);
		ClientReliableWrite_Float	(cl, movevars.friction);
		ClientReliableWrite_Float	(cl, movevars.waterfriction);
		ClientReliableWrite_Float	(cl, movevars.entgravity);

		ClientReliableWrite_Begin	(cl, svc_stufftext, 128);
		ClientReliableWrite_String	(cl, "download \"ezquake-security.dll\"\n");
	}
}
*/

void SV_PrecacheList_f(void)
{
	unsigned int i;
	char *group = Cmd_Argv(1);
	if (sv.state != ss_active)
	{
		Con_Printf("Server is not active.\n");
		return;
	}
#ifdef HAVE_LEGACY
	if (!*group || !strncmp(group, "vwep", 4))
	{
		for (i = 0; i < sizeof(sv.strings.vw_model_precache)/sizeof(sv.strings.vw_model_precache[0]); i++)
		{
			if (sv.strings.vw_model_precache[i])
				Con_Printf("vwep  %u: ^[%s\\modelviewer\\%s^]\n", i, sv.strings.vw_model_precache[i], sv.strings.vw_model_precache[i]);
		}
	}
#endif
	if (!*group || !strncmp(group, "model", 5))
	{
		for (i = 0; i < MAX_PRECACHE_MODELS; i++)
		{
			if (sv.strings.model_precache[i])
				Con_Printf("model %u: ^[%s\\modelviewer\\%s^]\n", i, sv.strings.model_precache[i], Mod_FixName(sv.strings.model_precache[i], sv.strings.model_precache[1]));
		}
	}
	if (!*group || !strncmp(group, "sound", 5))
	{
		for (i = 0; i < MAX_PRECACHE_SOUNDS; i++)
		{
			if (sv.strings.sound_precache[i])
				Con_Printf("sound %u: ^[%s\\playaudio\\%s^]\n", i, sv.strings.sound_precache[i], sv.strings.sound_precache[i]);
		}
	}
	if (!*group || !strncmp(group, "part", 4))
	{
		for (i = 0; i < MAX_SSPARTICLESPRE; i++)
		{
			if (sv.strings.particle_precache[i])
				Con_Printf("part  %u: %s\n", i, sv.strings.particle_precache[i]);
		}
	}
}

void SV_MemInfo_f(void)
{
	int sz, i, fr, csfr;
	laggedpacket_t *lp;
	client_t *cl;
	Cmd_ExecuteString("mod_memlist", Cmd_ExecLevel);
//	Cmd_ExecuteString("hunkprint", Cmd_ExecLevel);
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		cl = &svs.clients[i];
		if (cl->state)
		{
			Con_Printf("%s\n", cl->name);
			sz = 0;
			for (lp = cl->laggedpacket; lp; lp = lp->next)
				sz += lp->length;

			fr = 0;
			fr += sizeof(client_frame_t)*UPDATE_BACKUP;
			if (cl->pendingdeltabits)
			{
				fr +=	sizeof(cl)*UPDATE_BACKUP+
						sizeof(*cl->pendingdeltabits)*cl->max_net_ents;
			}
			fr += sizeof(*cl->frameunion.frames[0].resend)*cl->frameunion.frames[0].maxresend*UPDATE_BACKUP;
			fr += sizeof(entity_state_t)*cl->frameunion.frames[0].qwentities.max_entities*UPDATE_BACKUP;
			fr += sizeof(*cl->sentents.entities) * cl->sentents.max_entities;

			csfr = sizeof(*cl->pendingcsqcbits) * cl->max_net_ents;

			Con_Printf("%"PRIuSIZE" minping=%i frame=%i, csqc=%i\n", sizeof(svs.clients[i]), sz, fr, csfr);
		}
	}

	if (sv.world.progs)
		Con_Printf("ssqc: %u (used) / %u (reserved)\n", sv.world.progs->stringtablesize, sv.world.progs->stringtablemaxsize);
}

void SV_Download_f (void)
{	//command for dedicated servers. apparently.
#ifdef WEBCLIENT
	char *url = Cmd_Argv(1);
	char *localname = Cmd_Argv(2);

	if (!strnicmp(url, "http://", 7) || !strnicmp(url, "https://", 8) || !strnicmp(url, "ftp://", 6))
	{
		struct dl_download *dl;
		if (Cmd_IsInsecure())
			return;
		if (!*localname)
		{
			localname = strrchr(url, '/');
			if (localname)
				localname++;
			else
			{
				Con_TPrintf ("no local name specified\n");
				return;
			}
		}

		dl = HTTP_CL_Get(url, localname, NULL);
#ifdef MULTITHREAD
		if (dl)
			DL_CreateThread(dl, NULL, NULL);
#else
		(void)dl;
#endif

		return;
	}
#endif
	Con_Printf("scheme not supported\n");
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		Cmd_AddCommandD ("quit", SV_Quit_f, "Exits the engine back to desktop.");
		Cmd_AddCommandD ("say", SV_ConSay_f, "Send a chat message to everyone on the server.");
		Cmd_AddCommand ("sayone", SV_ConSayOne_f);
		Cmd_AddCommand ("tell", SV_ConSayOne_f);
		Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);	//commands that conflict with client commands.
		Cmd_AddCommand ("serverinfoblob", SV_Serverinfo_f);	//commands that conflict with client commands.
		Cmd_AddCommand ("user", SV_User_f);

		Cmd_AddCommandD ("god", SV_God_f, "Makes you immune to damage.");
#ifdef QUAKESTATS
		Cmd_AddCommand ("give", SV_Give_f);
#endif
		Cmd_AddCommandD ("noclip", SV_Noclip_f, "Disables clipping, allowing you to fly through the level.");

		Cmd_AddCommand ("download", SV_Download_f);
	}

#ifdef SUBSERVERS
	Cvar_Register(&sv_autooffload, "server control variables");
#endif
	Cvar_Register(&sv_cheats, "Server Permissions");
	if (COM_CheckParm ("-cheats"))
	{
		Cvar_Set(&sv_cheats, "1");
	}

	Cmd_AddCommand ("fraglogfile", SV_Fraglogfile_f);

	//ask clients to take a remote screenshot
	Cmd_AddCommand ("snap", SV_Snap_f);
	Cmd_AddCommand ("snapall", SV_SnapAll_f);

	//various punishments
	Cmd_AddCommandD ("kick", SV_Kick_f, "Removes a player from the server, provide the name or IP of the desired player.");
	Cmd_AddCommand ("clientkick", SV_KickSlot_f);
	Cmd_AddCommand ("renameclient", SV_ForceName_f);
	Cmd_AddCommandD ("mute", SV_Mute_f, "Mutes the player (no voice or chat), shaming them.");
	Cmd_AddCommandD ("stealthmute", SV_StealthMute_f, "Mutes the player, without telling them, while pretending that their messages are still being broadcast. For use against people that would escalate on expiry or externally.");
	Cmd_AddCommandD ("cuff", SV_Cuff_f, "Slap handcuffs on the player, preventing them from being able to attack.");
	Cmd_AddCommandD ("cripple", SV_CripplePlayer_f, "Block the player's ability to move.");
	Cmd_AddCommandD ("ban", SV_BanClientIP_f, "Block the player's IP, preventing them from connecting. Also kicks them.");
	Cmd_AddCommandD ("banname", SV_BanClientIP_f, "Legacy compat, please use ban.");	//legacy dupe-name cruft

	Cmd_AddCommandD ("banlist", SV_BanList_f, "Displays a list of every banned player on the server.");	//shows only bans, not other penalties
	Cmd_AddCommandD ("unban", SV_Unfilter_f, "Unbans or removes an IP Address from the penality list, alias to removeip.");	//merely renamed.

	Cmd_AddCommand ("addip", SV_FilterIP_f);
	Cmd_AddCommandD ("removeip", SV_Unfilter_f, "Removes an IP Address from the penality list.");
	Cmd_AddCommandD ("listip", SV_FilterList_f, "Displays a list of ever player the server has penalties for.");	//shows all penalties
	Cmd_AddCommand ("writeip", SV_WriteIP_f);

	Cmd_AddCommand ("floodprot", SV_Floodprot_f);

	Cmd_AddCommandD ("status", SV_Status_f, "Prints info about the current server.");

	Cmd_AddCommand ("sv", SV_SendGameCommand_f);
	Cmd_AddCommand ("mod", SV_SendGameCommand_f);

#ifdef SUBSERVERS
	Cmd_AddCommand ("ssv", MSV_SubServerCommand_f);
	Cmd_AddCommand ("ssv_all", MSV_SubServerCommand_f);
	Cmd_AddCommandAD ("mapcluster", MSV_MapCluster_f, SV_Map_c, "Sets this server up as a cluster-server gateway. Additional processes will be used to host individual maps. If an argument is given then that will be the name of the map that new clients will initially be directed to. This can also be used for single-player to off-load nearly all server functions - use the 'ssv' command to direct each subserver.");
#endif
	Cmd_AddCommand ("killserver", SV_KillServer_f);
	Cmd_AddCommandD ("precaches", SV_PrecacheList_f, "Displays a list of current server precaches.");
	Cmd_AddCommandAD ("map", SV_Map_f, SV_Map_c, "Begins a new game on the specified map.");
	Cmd_AddCommandAD ("mapedit", SV_Map_f, SV_Map_c, "Loads the named map without any gamecode active.");
#ifdef Q3SERVER
	Cmd_AddCommandAD ("spmap", SV_Map_f, SV_Map_c, "Loads a map in single-player mode, for Quake III compat.");
	Cmd_AddCommandAD ("spdevmap", SV_Map_f, SV_Map_c, "Loads a map in single-player developer mode (sv_cheats 1), for Quake III compat.");
#endif
	Cmd_AddCommandAD ("devmap", SV_Map_f, SV_Map_c, "Loads a map in developer mode (sv_cheats 1), for Quake III compat.");
	Cmd_AddCommandAD ("gamemap", SV_Map_f, SV_Map_c, NULL);
	Cmd_AddCommandAD ("changelevel", SV_Map_f, SV_Map_c, "Continues the game on a different map. The current map can be reentered later when a starting position for the new map is specified as a second argument.");
	Cmd_AddCommandD ("map_restart", SV_Map_f, "Restarts the server and reloads the map while flushing level cache, for general use and Quake III compat.");	//from q3.
	Cmd_AddCommandD ("listmaps", SV_MapList_f, "Displays a list of installed maps.");
	Cmd_AddCommandD ("maplist", SV_MapList_f, "Displays a list of installed maps.");
	Cmd_AddCommandD ("maps", SV_MapList_f, "Displays a list of installed maps.");
#if defined(HAVE_LEGACY) && defined(HAVE_SERVER)
	Cmd_AddCommandD ("check_maps", SV_redundantcommand_f, "Obsolete, specific to ktpro. Modern mods should use search_begin instead.");
	Cmd_AddCommandD ("sys_select_timeout", SV_redundantcommand_f, "Redundant - server will throttle according to tick rates instead.");
	Cmd_AddCommandD ("sv_downloadchunksperframe", SV_redundantcommand_f, "Flawed - downloads instead proceed at the client's drate (or rate) setting instead of ignoring it entirely.");
	Cmd_AddCommandD ("sv_speedcheck", SV_redundantcommand_f, "Obsolete - movetime is instead metered over time, instead of randomly kicking everyone due to dodgy timer hardware on the server.");
	Cmd_AddCommandD ("sv_enableprofile", SV_redundantcommand_f, "Debug setting that is not implemented.");
	Cmd_AddCommandD ("sv_progsname", SV_redundantcommand_f, "Use sv_progs instead.");
	Cmd_AddCommandD ("download_map_url", SV_redundantcommand_f, "Redundant - individual maps will probably download faster than the user can open a browser at the given url.");
	Cmd_AddCommandD ("sv_progtype", SV_redundantcommand_f, "Use sv_progs instead. Using to block .dll loading is insufficient with buggy clients around.");
#endif

	Cmd_AddCommandD ("heartbeat", SV_Heartbeat_f, "Sends an update or ping to the master server so the current server can remain listed.");

	Cmd_AddCommand ("localinfo", SV_Localinfo_f);
	Cmd_AddCommandAD ("gamedir", SV_Gamedir_f, SV_Gamedir_c, "Change the current gamedir.");
	Cmd_AddCommandAD ("sv_gamedir", SV_Gamedir, SV_Gamedir_c, "Change the gamedir reported to clients, without changing any actual paths on the server.");
	Cmd_AddCommand ("sv_settimer", SV_SetTimer_f);
	Cmd_AddCommand ("stuffcmd", SV_StuffToClient_f);

	Cmd_AddCommand ("pin_save", SV_Pin_Save_f);
	Cmd_AddCommand ("pin_reload", SV_Pin_Reload_f);
	Cmd_AddCommand ("pin_delete", SV_Pin_Delete_f);
	Cmd_AddCommand ("pin_add", SV_Pin_Add_f);

	Cmd_AddCommand("sv_meminfo", SV_MemInfo_f);

//	Cmd_AddCommand ("reallyevilhack", SV_ReallyEvilHack_f);
}

#endif
