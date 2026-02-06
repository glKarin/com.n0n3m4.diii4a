#include "quakedef.h"
#include "pr_common.h"

#if !defined(CLIENTONLY) && defined(SAVEDGAMES)
#define CACHEGAME_VERSION_DEFAULT CACHEGAME_VERSION_VERBOSE

extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t teamplay;
extern cvar_t pr_enable_profiling;

cvar_t sv_savefmt = CVARFD("sv_savefmt", "", CVAR_SAVE, "Specifies the format used for the saved game.\n0=legacy.\n1=fte\n2=binary");
cvar_t sv_autosave = CVARFD("sv_autosave", "5", CVAR_SAVE, "Interval for autosaves, in minutes. Set to 0 to disable autosave.");
extern cvar_t pr_ssqc_memsize;

void SV_Savegame_f (void);


typedef struct
{
	char name[32];
	union
	{
		int i;
		float f;
	} parm[NUM_SPAWN_PARMS];
	char *parmstr;

	client_t *source;
} loadplayer_t;

struct loadinfo_s
{
	size_t numplayers;
	loadplayer_t *players;
};

//Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
void SV_SavegameComment (char *text, size_t textsize)
{
	int		i;
	char	kills[20];
	char	datetime[64];
	time_t	timeval;

	char *mapname = sv.mapname;

	for (i=0 ; i<textsize-1 ; i++)
		text[i] = ' ';
	text[textsize-1] = '\0';
	if (!mapname)
		strcpy( text, "Unnamed_Level");
	else
	{
		i = strlen(mapname);
		if (i > 22)
			i = 22;
		memcpy (text, mapname, i);
	}

	kills[0] = '\0';
#ifdef Q2SERVER
	if (ge)	//q2
	{
		kills[0] = '\0';
	}
	else
#endif
#ifdef HLSERVER
	if (svs.gametype == GT_HALFLIFE)
	{
		sprintf (kills,"");
	}
	else
#endif
	{
		if ((int)pr_global_struct->killed_monsters || (int)pr_global_struct->total_monsters)
			sprintf (kills,"kills:%3i/%3i", (int)pr_global_struct->killed_monsters, (int)pr_global_struct->total_monsters);
	}

	time(&timeval);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime( &timeval ));

	memcpy (text+22, kills, strlen(kills));
	if (textsize > 39)
	{
		Q_strncpyz(text+39, datetime, textsize-39);
	}
// convert space to _ to make stdio happy
	for (i=0 ; i<textsize-1 ; i++)
	{
		if (text[i] == ' ')
			text[i] = '_';
		else if (text[i] == '\n')
			text[i] = '\0';
	}
	text[textsize-1] = '\0';
}

pbool PDECL SV_ExtendedSaveData(pubprogfuncs_t *progfuncs, void *loadctx, const char **ptr)
{
	struct loadinfo_s *loadinfo = loadctx;
	char token[65536];
	com_tokentype_t tt;
	const char *l = *ptr;
	size_t idx;
	if (l[0] == 's' && l[1] == 'v' && l[2] == '.')
		l += 3;	//DPism

	do
	{
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);
	} while(tt == TTP_LINEENDING);
	if (tt != TTP_RAWTOKEN)return false;

	if (!strcmp(token, "lightstyle") || !strcmp(token, "lightstyles"))
	{	//lightstyle N "STYLESTRING" 1 1 1
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (idx >= sv.maxlightstyles)
		{
			if (idx >= MAX_NET_LIGHTSTYLES)
				return false; //unsupported index.
			Z_ReallocElements((void**)&sv.lightstyles, &sv.maxlightstyles, idx+1, sizeof(*sv.lightstyles));
		}
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;

		if (sv.lightstyles[idx].str)
			Z_Free((char*)sv.lightstyles[idx].str);
		sv.lightstyles[idx].str = Z_StrDup(token);
		sv.lightstyles[idx].colours[0] = sv.lightstyles[idx].colours[1] = sv.lightstyles[idx].colours[2] = 1.0;
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		sv.lightstyles[idx].colours[0] = atof(token);
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		sv.lightstyles[idx].colours[1] = atof(token);
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		sv.lightstyles[idx].colours[2] = atof(token);
	}
	else if (!strcmp(token, "model_precache") || !strcmp(token, "model"))
	{	//model_precache N "MODELNAME"
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (!idx || idx >= countof(sv.strings.model_precache))
			return false;	//unsupported index.
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;
		sv.strings.model_precache[idx] = PR_AddString(svprogfuncs, token, 0, false);
	}
#ifdef HAVE_LEGACY
	else if (!strcmp(token, "vwep"))
	{	//vwep N "MODELNAME"
		//0 IS valid, and frankly this stuff sucks.
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (idx >= countof(sv.strings.vw_model_precache))
			return false;	//unsupported index.
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;
		sv.strings.vw_model_precache[idx] = PR_AddString(svprogfuncs, token, 0, false);
	}
#endif
	else if (!strcmp(token, "sound_precache") || !strcmp(token, "sound"))
	{	//sound_precache N "MODELNAME"
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (!idx || idx >= countof(sv.strings.sound_precache))
			return false;	//unsupported index.
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;
		sv.strings.sound_precache[idx] = PR_AddString(svprogfuncs, token, 0, false);
	}
	else if (!strcmp(token, "particle_precache") || !strcmp(token, "particle"))
	{	//particle_precache N "MODELNAME"
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (!idx || idx >= countof(sv.strings.particle_precache))
			return false;	//unsupported index.
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;
		sv.strings.particle_precache[idx] = PR_AddString(svprogfuncs, token, 0, false);
	}
	else if (!strcmp(token, "serverflags"))
	{	//serverflags N  (for map_restart to work properly)
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		svs.serverflags = idx;
	}
	else if (!strcmp(token, "startspot"))
	{	//startspot "foo"  (for map_restart to work properly)
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		InfoBuf_SetStarKey(&svs.info, "*startspot", token);
	}
	else if (loadinfo && !strcmp(token, "spawnparm"))
	{	//spawnparm idx val  (for map_restart to work properly)
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
		idx = atoi(token);
		if (idx == 0)
		{	//the parmstr...
			l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return false;
			loadinfo->players[0].parmstr = Z_StrDup(token);
		}
		else if (idx >= 1 && idx <= countof(loadinfo->players->parm))
		{	//regular parm
			l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return false;
			loadinfo->players[0].parm[idx-1].f = atof(token);
		}
	}
	//strbuffer+hashtable+etc junk
	else if (PR_Common_LoadGame(svprogfuncs, token, &l))
		;
	else
		return false;
	*ptr = l;
	return true;
}

#ifndef QUAKETC
static qboolean SV_LegacySavegame (const char *savename, qboolean verbose)
{
	size_t len;
	char *s = NULL;
	client_t *cl;
	int clnum;

	int version = SAVEGAME_VERSION_FTE_LEG;

	char	native[MAX_OSPATH];
	char	name[MAX_QPATH];
	vfsfile_t	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];

	if (sv.state != ss_active)
	{
		if (verbose)
			Con_TPrintf("Can't apply: Server isn't running or is still loading\n");
		return false;
	}

	if (sv.allocated_client_slots != 1 || svs.clients->state != cs_spawned)
	{
		//we don't care about fte-format legacy.
		if (verbose)
			Con_TPrintf("Unable to use legacy savegame format to save multiplayer games\n");
		return false;
	}

	sprintf (name, "%s", savename);
	COM_RequireExtension (name, ".sav", sizeof(name));	//do NOT allow .pak etc
	if (!FS_DisplayPath(name, FS_GAMEONLY, native, sizeof(native)))
		return false;
	Con_TPrintf (U8("Saving game to %s...\n"), native);
	f = FS_OpenVFS(name, "wbp", FS_GAMEONLY);
	if (!f)
	{
		if (verbose)
			Con_TPrintf ("ERROR: couldn't open %s.\n", name);
		return false;
	}

	//if there are 1 of 1 players connected
	if (sv.allocated_client_slots == 1 && svs.clients->state == cs_spawned)
	{//try to go for nq/zq compatability as this is a single player game.
		s = PR_SaveEnts(svprogfuncs, NULL, &len, 0, 2);	//get the entity state now, so that we know if we can get the full state in a q1 format.
		if (s)
		{
			if (progstype == PROG_QW)
				version = SAVEGAME_VERSION_QW;
			else
				version = SAVEGAME_VERSION_NQ;
		}
	}


	VFS_PRINTF(f, "%i\n", version);
	SV_SavegameComment (comment, sizeof(comment));
	VFS_PRINTF(f, "%s\n", comment);

	if (version == SAVEGAME_VERSION_NQ || version == SAVEGAME_VERSION_QW)
	{
		//only 16 spawn parms.
		for (i=0; i < 16; i++)
			VFS_PRINTF(f, "%f\n", svs.clients->spawn_parms[i]);	//client 1.
		VFS_PRINTF(f, "%f\n", skill.value);
	}
	else
	{
		VFS_PRINTF(f, "%i\n", sv.allocated_client_slots);
		for (cl = svs.clients, clnum=0; clnum < sv.allocated_client_slots; cl++,clnum++)
		{
			if (cl->state < cs_loadzombie || !cl->spawned)	//don't save if they are still connecting
			{
				VFS_PRINTF(f, "\"\"\n");
				continue;
			}

			VFS_PRINTF(f, "\"%s\"\n", cl->name);
			for (i=0; i<NUM_SPAWN_PARMS ; i++)
				VFS_PRINTF(f, "%f\n", cl->spawn_parms[i]);
		}
		VFS_PRINTF(f, "%i\n", progstype);
		VFS_PRINTF(f, "%f\n", skill.value);
		VFS_PRINTF(f, "%f\n", deathmatch.value);
		VFS_PRINTF(f, "%f\n", coop.value);
		VFS_PRINTF(f, "%f\n", teamplay.value);
	}
	VFS_PRINTF(f, "%s\n", svs.name);
	VFS_PRINTF(f, "%f\n",sv.time);

// write the light styles (only 64 are saved in legacy saved games)
	for (i=0 ; i < 64; i++)
	{
		if (i < sv.maxlightstyles && sv.lightstyles[i].str && *sv.lightstyles[i].str)
			VFS_PRINTF(f, "%s\n", sv.lightstyles[i].str);
		else
			VFS_PRINTF(f,"m\n");
	}

	if (!s)
		s = PR_SaveEnts(svprogfuncs, NULL, &len, 0, 1);
	VFS_PUTS(f, s);
	VFS_PUTS(f, "\n");

#if 1
	/* Extended save info
	** This should also be compatible with both DP and QSS.
	** WARNING: this does NOT protect against models/sounds being precached in different/random orders (statics/baselines/ambients will be wrong).
	**          the only protection we get is from late precaches.
	**          theoretically the loader could make it work by rewriting the various tables, but that would not necessarily be reliable.
	*/
	VFS_PUTS(f, "/*\n");
	VFS_PUTS(f, "// FTE extended savegame\n");
	for (i=0 ; i < sv.maxlightstyles; i++)
	{	//yes, repeat styles 0-63 again, but only list ones that are not empty.
		if (sv.lightstyles[i].str)
			VFS_PRINTF(f, "sv.lightstyles %i %s\n", i, sv.lightstyles[i].str);
	}
	for (i=1 ; i < countof(sv.strings.model_precache); i++)
	{
		if (sv.strings.model_precache[i])
			VFS_PRINTF(f, "sv.model_precache %i %s\n", i, sv.strings.model_precache[i]);
	}
	for (i=1 ; i < countof(sv.strings.sound_precache); i++)
	{
		if (sv.strings.sound_precache[i])
			VFS_PRINTF(f, "sv.sound_precache %i %s\n", i, sv.strings.sound_precache[i]);
	}
	for (i=1 ; i < countof(sv.strings.particle_precache); i++)
	{
		if (sv.strings.particle_precache[i])
			VFS_PRINTF(f, "sv.particle_precache %i %s\n", i, sv.strings.particle_precache[i]);
	}
	VFS_PRINTF(f, "sv.serverflags %i\n", svs.serverflags);	//zomg! a fix for losing runes on load;restart!
//	VFS_PRINTF(f, "sv.startspot %s\n", InfoBuf_ValueForKey(&svs.info, "*startspot")); //startspot, for restarts.
	if (svs.clients->spawn_parmstring)
	{
		size_t maxlen = strlen(svs.clients->spawn_parmstring)*2+4 + 1;
		char *buffer = BZ_Malloc(maxlen);
		VFS_PRINTF(f, "spawnparm 0 %s\n", COM_QuotedString(svs.clients->spawn_parmstring, buffer, sizeof(maxlen), false));
		BZ_Free(buffer);
	}
	if (version == SAVEGAME_VERSION_NQ || version == SAVEGAME_VERSION_QW)
		for (i=16 ; i < countof(svs.clients->spawn_parms); i++)
			VFS_PRINTF(f, "spawnparm %i %g\n", i+1, svs.clients->spawn_parms[i]);
//	sv.buffer %i %i "string"
//	sv.bufstr %i %i "%s"
	VFS_PUTS(f, "*/\n");
#endif
	svprogfuncs->parms->memfree(s);

	VFS_CLOSE(f);

	FS_FlushFSHashWritten(name);

	Q_strncpyz(sv.loadgame_on_restart, savename, sizeof(sv.loadgame_on_restart));
	return true;
}
#endif

void SV_FlushLevelCache(void)
{
	levelcache_t *cache;

	while(svs.levcache)
	{
		cache = svs.levcache->next;
		Z_Free(svs.levcache);
		svs.levcache = cache;
	}
}

void LoadModelsAndSounds(vfsfile_t *f)
{
	char	str[32768];
	int i;

	sv.strings.model_precache[0] = PR_AddString(svprogfuncs, "", 0, false);
	for (i=1; i < MAX_PRECACHE_MODELS; i++)
	{
		VFS_GETS(f, str, sizeof(str));
		if (!*str)
			break;

		sv.strings.model_precache[i] = PR_AddString(svprogfuncs, str, 0, false);
	}
	if (i == MAX_PRECACHE_MODELS)
	{
		VFS_GETS(f, str, sizeof(str));
		if (*str)
			SV_Error("Too many model precaches in loadgame cache");
	}
	for (; i < MAX_PRECACHE_MODELS; i++)
		sv.strings.model_precache[i] = NULL;

	sv.strings.sound_precache[0] = PR_AddString(svprogfuncs, "", 0, false);
	for (i=1; i < MAX_PRECACHE_SOUNDS; i++)
	{
		VFS_GETS(f, str, sizeof(str));
		if (!*str)
			break;

		sv.strings.sound_precache[i] = PR_AddString(svprogfuncs, str, 0, false);
	}
	if (i == MAX_PRECACHE_SOUNDS)
	{
		VFS_GETS(f, str, sizeof(str));
		if (*str)
			SV_Error("Too many sound precaches in loadgame cache");
	}
	for (; i < MAX_PRECACHE_SOUNDS; i++)
		sv.strings.sound_precache[i] = NULL;
}

static void PDECL SV_SaveMemoryReset (pubprogfuncs_t *progfuncs, void *ctx)
{
	size_t i;
	//model names are pointers to vm-accessible memory. as that memory is going away, we need to destroy and recreate, which requires preserving them.
	for (i = 1; i < MAX_PRECACHE_MODELS; i++)
	{
		if (!sv.strings.model_precache[i])
			break;
		sv.strings.model_precache[i] = PR_AddString(svprogfuncs, sv.strings.model_precache[i], 0, false);
	}
	for (i = 1; i < MAX_PRECACHE_SOUNDS; i++)
	{
		if (!sv.strings.sound_precache[i])
			break;
		sv.strings.sound_precache[i] = PR_AddString(svprogfuncs, sv.strings.sound_precache[i], 0, false);
	}
}

/*ignoreplayers - says to not tell gamecode (a loadgame rather than a level change)*/
qboolean SV_LoadLevelCache(const char *savename, const char *level, const char *startspot, qboolean isloadgame)
{
	eval_t *eval, *e2;

	char	name[MAX_OSPATH];
	vfsfile_t	*f;
	char	mapname[MAX_QPATH];
	float	time;
	char	str[32768];
	int		i;
	size_t	j;
	edict_t	*ent;
	int		version;

	int current_skill;

	int pt;

	int modelpos;

	qofs_t filelen, filepos;
	char *file;
	gametype_e gametype;

	levelcache_t *cache;
	int numstyles;

	if (isloadgame)
	{
		gametype = svs.gametype;
	}
	else
	{
		cache = svs.levcache;
		while(cache)
		{
			if (!strcmp(cache->mapname, level))
				break;

			cache = cache->next;
		}
		if (!cache)
			return false;	//not visited yet. Ignore the existing caches as fakes.

		gametype = cache->gametype;
	}

	if (savename)
		Q_snprintfz (name, sizeof(name), "saves/%s/%s.lvc", savename, level);
	else
		Q_snprintfz (name, sizeof(name), "saves/%s.lvc", level);

//	Con_TPrintf ("Loading game from %s...\n", name);

#ifdef Q2SERVER
	if (gametype == GT_QUAKE2)
	{
		char *s;
		flocation_t loc;
		SV_SpawnServer (level, startspot, false, false, 0);

		World_ClearWorld(&sv.world, false);
		if (!ge)
		{
			Con_Printf("Incorrect gamecode type.\n");
			return false;
		}

		if (!FS_FLocateFile(name, FSLF_IFFOUND, &loc))
		{
			Con_Printf("Couldn't find %s.\n", name);
			return false;
		}
		if (!*loc.rawname || loc.offset)
		{
			Con_Printf("%s is inside a package and cannot be used by the quake2 gamecode.\n", name);
			return false;
		}

		if (savename)
			Q_snprintfz (name, sizeof(name), "saves/%s/%s.lvx", savename, level);
		else
			Q_snprintfz (name, sizeof(name), "saves/%s.lvx", level);
		file = FS_MallocFile(name, FS_GAME, &filelen);
		if (file)
		{
			s = file;
			//Read config strings
			for (i = 0; i < countof(sv.strings.configstring) && s < file+filelen; i++)
			{
				Z_Free((char*)sv.strings.configstring[i]);
				sv.strings.configstring[i] = Z_StrDup(s);
				s += strlen(s)+1;
			}
			for (i = 0; s < file+filelen; i++)
			{
				if (!*s)
					break;
				if (i < countof(sv.strings.q2_extramodels))
				{
					Z_Free((char*)sv.strings.q2_extramodels[i]);
					sv.strings.q2_extramodels[i] = Z_StrDup(s);
				}
				s += strlen(s)+1;
			}
			for (; i < countof(sv.strings.q2_extramodels); i++)
			{
				Z_Free((char*)sv.strings.q2_extramodels[i]);
				sv.strings.q2_extramodels[i] = NULL;
			}
			for (i = 0; s < file+filelen; i++)
			{
				if (!*s)
					break;
				if (i < countof(sv.strings.q2_extrasounds))
				{
					Z_Free((char*)sv.strings.q2_extrasounds[i]);
					sv.strings.q2_extrasounds[i] = Z_StrDup(s);
				}
				s += strlen(s)+1;
			}
			for (; i < countof(sv.strings.q2_extrasounds); i++)
			{
				Z_Free((char*)sv.strings.q2_extrasounds[i]);
				sv.strings.q2_extrasounds[i] = NULL;
			}
			//Read portal state
			sv.world.worldmodel->funcs.LoadAreaPortalBlob(sv.world.worldmodel, s, (file+filelen)-s);
			FS_FreeFile(file);
		}

		ge->ReadLevel(loc.rawname);

		for (i=0 ; i<100 ; i++)	//run for 10 secs to iron out a few bugs.
			ge->RunFrame ();
		return true;
	}
#endif

// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	f = FS_OpenVFS(name, "rb", FS_GAME);
	if (!f)
	{
		if (isloadgame)
			Con_Printf ("ERROR: Couldn't load \"%s\"\n", name);
		return false;
	}

	VFS_GETS(f, str, sizeof(str));
	version = atoi(str);
	if (version != CACHEGAME_VERSION_OLD && version != CACHEGAME_VERSION_VERBOSE && version != CACHEGAME_VERSION_MODSAVED)
	{
		VFS_CLOSE (f);
		Con_TPrintf ("Savegame is version %i, not %i\n", version, CACHEGAME_VERSION_DEFAULT);
		return false;
	}
	VFS_GETS(f, str, sizeof(str));	//comment

	SV_SendMessagesToAll();

	if (version == CACHEGAME_VERSION_OLD)
	{
		VFS_GETS(f, str, sizeof(str));
		pt = atof(str);

	// this silliness is so we can load 1.06 save files, which have float skill values
		VFS_GETS(f, str, sizeof(str));
		current_skill = (int)(atof(str) + 0.1);
		Cvar_Set (&skill, va("%i", current_skill));

		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (&deathmatch, atof(str));
		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (&coop, atof(str));
		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (&teamplay, atof(str));

		VFS_GETS(f, mapname, sizeof(mapname));
		VFS_GETS(f, str, sizeof(str));
		time = atof(str);
	}
	else
	{
		time = 0;
		pt = PROG_UNKNOWN;
		while (VFS_GETS(f, str, sizeof(str)))
		{
			char *s = str;
			cvar_t *var;
			s = COM_Parse(s);
			if (!strcmp(com_token, "map"))
			{	//map "foo": terminates the preamble.
				COM_ParseOut(s, mapname, sizeof(mapname));
				break;
			}
			else if (!strcmp(com_token, "cvar"))
			{
				s = COM_Parse(s);
				var = Cvar_FindVar(com_token);
				s = COM_Parse(s);
				if (var)
					Cvar_Set(var, com_token);
			}
			else if (!strcmp(com_token, "time"))
			{
				s = COM_Parse(s);
				time = atof(com_token);
			}
			else if (!strcmp(com_token, "vmmode"))
			{
				s = COM_Parse(s);
				if (!strcmp(com_token, "NONE")) pt = PROG_NONE;
				else if (!strcmp(com_token, "QW")) pt = PROG_QW;
				else if (!strcmp(com_token, "NQ")) pt = PROG_NQ;
				else if (!strcmp(com_token, "H2")) pt = PROG_H2;
				else if (!strcmp(com_token, "PREREL")) pt = PROG_PREREL;
				else if (!strcmp(com_token, "TENEBRAE")) pt = PROG_TENEBRAE;
				else if (!strcmp(com_token, "UNKNOWN")) pt = PROG_UNKNOWN;
				else pt = PROG_UNKNOWN;
			}
			else
				Con_TPrintf ("Unknown savegame directive %s\n", com_token);
		}
	}

	//NOTE: This sets up the default baselines+statics+ambients.
	//FIXME: if any model names changed, then we're screwed.
	SV_SpawnServer (mapname, startspot, false, false, svs.allocated_client_slots);
	sv.time = time;
	if (svs.gametype != gametype)
	{
		VFS_CLOSE (f);
		Con_Printf("Incorrect gamecode type. Cannot load game.\n");
		return false;
	}
	if (sv.state != ss_active)
	{
		VFS_CLOSE (f);
		Con_TPrintf ("Couldn't load map\n");
		return false;
	}

	if (version == CACHEGAME_VERSION_MODSAVED)
	{
		const char *line;
		void *pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
		func_t restorefunc = PR_FindFunction(svprogfuncs, "SV_PerformLoad", PR_ANY);
		com_tokentype_t tt;
		if (!restorefunc)
		{
			VFS_CLOSE (f);
			Con_TPrintf ("SV_PerformLoad missing, unable to load game\n");
			return false;
		}

		//shove it into a buffer to make the next bit easier.
		filepos = VFS_TELL(f);
		filelen = VFS_GETLEN(f);
		filelen -= filepos;
		file = BZ_Malloc(filelen+1);
		memset(file, 0, filelen+1);
		filelen = VFS_READ(f, file, filelen);
		VFS_CLOSE (f);
		if ((int)filelen < 0)
			filelen = 0;
		file[filelen]='\0';

		//look for possible engine commands followed by a 'moddata' line (with no args)
		line = file;
		while(line && *line)
		{
			if (SV_ExtendedSaveData(svprogfuncs, NULL, &line))
				continue;
			line = COM_ParseTokenOut(line, NULL, com_token, sizeof(com_token), &tt);
			if (!strcmp(com_token, "moddata"))
			{
				//loop till end of line
				while (line && tt != TTP_LINEENDING)
					line = COM_ParseTokenOut(line, NULL, com_token, sizeof(com_token), &tt);
				break;	//terminates the postamble
			}

			//loop till end of line
			while (line && tt != TTP_LINEENDING)
				line = COM_ParseTokenOut(line, NULL, com_token, sizeof(com_token), &tt);
		}
		if (!line)
		{
			BZ_Free(file);
			Con_TPrintf ("unsupported saved game\n");
			return false;
		}

//		for(i = sv.allocated_client_slots+1; i < sv.world.num_edicts; i++)
//			svprogfuncs->EntFree (svprogfuncs, EDICT_NUM_PB(svprogfuncs, i), false);

		//and now we can stomp on everything. yay.
		sv.world.edicts[0].readonly = false;
		G_FLOAT(OFS_PARM0) = PR_QCFile_From_Buffer(svprogfuncs, name, file, line-file, filelen);
		G_FLOAT(OFS_PARM1) = sv.world.num_edicts;
		G_FLOAT(OFS_PARM2) = sv.allocated_client_slots;
		PR_ExecuteProgram(svprogfuncs, restorefunc);
		sv.world.edicts[0].readonly = true;

		//in case they forgot to setorigin everything after blindly loading its fields...
		World_ClearWorld (&sv.world, true);

		//let time jump to match whatever time the mod wanted.
		sv.time = sv.world.physicstime = pr_global_struct->time = time;
		sv.starttime = Sys_DoubleTime() - sv.time;
		return true;
	}

// load the edicts out of the savegame file
// the rest of the file is sent directly to the progs engine.

	/*hexen2's gamecode doesn't have SAVE set on all variables, in which case we must clobber them, and run the risk that they were set at map load time, but clear in the savegame.*/
	if (progstype != PROG_H2)
	{
		Q_SetProgsParms(false);
		PR_Configure(svprogfuncs, PR_ReadBytesString(pr_ssqc_memsize.string), MAX_PROGS, pr_enable_profiling.ival);
		PR_RegisterFields();
		PR_InitEnts(svprogfuncs, sv.world.max_edicts);
	}

	if (version == CACHEGAME_VERSION_OLD)
	{
		// load the light styles
		VFS_GETS(f, str, sizeof(str));
		numstyles = atoi(str);
		if (numstyles > MAX_NET_LIGHTSTYLES)
		{
			VFS_CLOSE (f);
			Con_Printf ("load failed - invalid number of lightstyles\n");
			return false;
		}
		for (i = 0; i<sv.maxlightstyles ; i++)
		{
			if (sv.lightstyles[i].str)
				BZ_Free((void*)sv.lightstyles[i].str);
			sv.lightstyles[i].str = NULL;
		}
		Z_ReallocElements((void**)&sv.lightstyles, &sv.maxlightstyles, numstyles, sizeof(*sv.lightstyles));

		for (i=0 ; i<numstyles ; i++)
		{
			VFS_GETS(f, str, sizeof(str));
			sv.lightstyles[i].str = Z_StrDup(str);
		}
		for ( ; i<sv.maxlightstyles ; i++)
		{
			sv.lightstyles[i].str = Z_StrDup("");
		}

		modelpos = VFS_TELL(f);
		LoadModelsAndSounds(f);
	}
	else
		modelpos = 0;

	filepos = VFS_TELL(f);
	filelen = VFS_GETLEN(f);
	filelen -= filepos;
	file = BZ_Malloc(filelen+1);
	memset(file, 0, filelen+1);
	VFS_READ(f, file, filelen);
	file[filelen]='\0';
	sv.world.edict_size=svprogfuncs->load_ents(svprogfuncs, file, NULL, SV_SaveMemoryReset, NULL, SV_ExtendedSaveData);
	BZ_Free(file);

	progstype = pt;

	PR_LoadGlabalStruct(false);

	pr_global_struct->time = sv.time = sv.world.physicstime = time;
	sv.starttime = Sys_DoubleTime() - sv.time;

	if (modelpos != 0)
	{
		VFS_SEEK(f, modelpos);
		LoadModelsAndSounds(f);
	}

	VFS_CLOSE(f);

	PF_InitTempStrings(svprogfuncs);

	World_ClearWorld (&sv.world, true);

	for (i=0 ; i<svs.allocated_client_slots ; i++)
	{
		if (i < sv.allocated_client_slots)
			ent = EDICT_NUM_PB(svprogfuncs, i+1);
		else
			ent = NULL;
		svs.clients[i].edict = ent;
		ent->ereftype = ER_ENTITY;	//should have already been allocated.

		svs.clients[i].name = PR_AddString(svprogfuncs, svs.clients[i].namebuf, sizeof(svs.clients[i].namebuf), false);
		svs.clients[i].team = PR_AddString(svprogfuncs, svs.clients[i].teambuf, sizeof(svs.clients[i].teambuf), false);

		//svs.clients[i].spawned = (svs.clients[i].state == cs_loadzombie);
#ifdef HEXEN2
		if (ent)
			svs.clients[i].playerclass = ent->xv->playerclass;
		else
			svs.clients[i].playerclass = 0;
#endif
	}

	if (!isloadgame)
	{
		eval = PR_FindGlobal(svprogfuncs, "startspot", 0, NULL);
		if (eval) eval->_int = (int)PR_NewString(svprogfuncs, startspot);

		eval = PR_FindGlobal(svprogfuncs, "ClientReEnter", 0, NULL);
		if (eval)
		for (i=0 ; i<sv.allocated_client_slots ; i++)
		{
			if (svs.clients[i].spawninfo)
			{
				globalvars_t *pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
				ent = svs.clients[i].edict;
				j = strlen(svs.clients[i].spawninfo);
				svprogfuncs->restoreent(svprogfuncs, svs.clients[i].spawninfo, &j, ent);

				e2 = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, "stats_restored", ev_float, NULL);
				if (e2)
					e2->_float = 1;
				SV_SpawnParmsToQC(host_client);
				pr_global_struct->time = sv.world.physicstime;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);
				World_UnlinkEdict((wedict_t*)ent);
				G_FLOAT(OFS_PARM0) = sv.time-host_client->spawninfotime;
				PR_ExecuteProgram(svprogfuncs, eval->function);

//				if (svs.clients[i].state == cs_loadzombie)
//					svs.clients[i].istobeloaded = 1;
//				else
//					svs.clients[i].state = cs_spawned;	//don't do a separate ClientConnect.
			}
		}
		pr_global_struct->serverflags = svs.serverflags;
	}

	pr_global_struct->time = sv.world.physicstime;

	for (i=0 ; i<sv.world.num_edicts ; i++)
	{
		ent = EDICT_NUM_PB(svprogfuncs, i);
		if (ED_ISFREE(ent))
			continue;

		/*hexen2 instead overwrites ents, which can theoretically be unreliable (ents with this flag are not saved in the first place, and thus are effectively reset instead of reloaded).
		  fte purges all ents beforehand in a desperate attempt to remain sane.
		  this behaviour does not match exactly, but is enough for vanilla hexen2/POP.
		*/
		if ((unsigned int)ent->v->flags & FL_HUBSAVERESET)
		{
			func_t f;
			/*set some minimal fields*/
			ent->v->solid = SOLID_NOT;
			ent->v->use = 0;
			ent->v->touch = 0;
			ent->v->think = 0;
			ent->v->nextthink = 0;
			/*reinvoke the spawn function*/
			pr_global_struct->time = 0.1;
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);
			f = PR_FindFunction(svprogfuncs, PR_GetString(svprogfuncs, ent->v->classname), PR_ANY);

			if (f)
				PR_ExecuteProgram(svprogfuncs, f);
		}
	}

	return true;	//yay
}

void SV_SaveLevelCache(const char *savedir, qboolean dontharmgame)
{
	size_t len;
	char *s;
	client_t *cl;
	int clnum;

	char	name[256];
	vfsfile_t	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	levelcache_t *cache;
	int version = CACHEGAME_VERSION_DEFAULT;
	func_t func;

	if (!sv.state)
		return;

	if (!dontharmgame)
	{
		cache = svs.levcache;
		while(cache)
		{
			if (!strcmp(cache->mapname, svs.name))
				break;

			cache = cache->next;
		}
		if (!cache)	//not visited yet. Let us know that we went there.
		{
			cache = Z_Malloc(sizeof(levelcache_t)+strlen(svs.name)+1);
			cache->mapname = (char *)(cache+1);
			strcpy(cache->mapname, svs.name);

			cache->gametype = svs.gametype;
			cache->next = svs.levcache;
			svs.levcache = cache;
		}
	}

	if (savedir)
		Q_snprintfz (name, sizeof(name), "saves/%s/%s.lvc", savedir, svs.name);
	else
		Q_snprintfz (name, sizeof(name), "saves/%s.lvc", svs.name);

	FS_CreatePath(name, FS_GAMEONLY);

	if (!dontharmgame)	//save game in progress
		Con_TPrintf ("Saving game to %s...\n", name);

#ifdef Q2SERVER
	if (ge)
	{
		char	syspath[MAX_OSPATH];

		if (!FS_SystemPath(name, FS_GAMEONLY, syspath, sizeof(syspath)))
			return;
		ge->WriteLevel(syspath);

		if (savedir)
			Q_snprintfz (name, sizeof(name), "saves/%s/%s.lvx", savedir, svs.name);
		else
			Q_snprintfz (name, sizeof(name), "saves/%s.lvx", svs.name);
		//write configstrings
		f = FS_OpenVFS (name, "wbp", FS_GAMEONLY);
		if (f)
		{
			size_t portalblobsize;
			void *portalblob = NULL;
			for (i = 0; i < countof(sv.strings.configstring); i++)
			{
				if (sv.strings.configstring[i])
					VFS_WRITE(f, sv.strings.configstring[i], strlen(sv.strings.configstring[i])+1);
				else
					VFS_WRITE(f, "", 1);
			}

			for (i = 0; i < countof(sv.strings.q2_extramodels); i++)
			{
				if (!sv.strings.q2_extramodels[i])
					break;
				VFS_WRITE(f, sv.strings.q2_extramodels[i], strlen(sv.strings.q2_extramodels[i])+1);
			}
			VFS_WRITE(f, "", 1);

			for (i = 0; i < countof(sv.strings.q2_extrasounds); i++)
			{
				if (!sv.strings.q2_extrasounds[i])
					break;
				VFS_WRITE(f, sv.strings.q2_extrasounds[i], strlen(sv.strings.q2_extrasounds[i])+1);
			}
			VFS_WRITE(f, "", 1);

			portalblobsize = sv.world.worldmodel->funcs.SaveAreaPortalBlob(sv.world.worldmodel, &portalblob);
			VFS_WRITE(f, portalblob, portalblobsize);

			VFS_CLOSE(f);
		}


		FS_FlushFSHashFull();
		return;
	}
#endif

#ifdef HLSERVER
	if (svs.gametype == GT_HALFLIFE)
	{
		SVHL_SaveLevelCache(name);
		FS_FlushFSHashFull();
		return;
	}
#endif

	func = PR_FindFunction(svprogfuncs, "SV_PerformSave", PR_ANY);
	if (func)
		version = CACHEGAME_VERSION_MODSAVED;

	f = FS_OpenVFS (name, "wbp", FS_GAMEONLY);
	if (!f)
	{
		Con_TPrintf ("ERROR: couldn't open %s.\n", name);
		return;
	}

	VFS_PRINTF (f, "%i\n", version);
	SV_SavegameComment (comment, sizeof(comment));
	VFS_PRINTF (f, "%s\n", comment);

	if (!dontharmgame)
	{
		//map-change caches require the players to have been de-spawned
		//saved games require players to retain their fields.
		//probably this should happen elsewhere.
		for (cl = svs.clients, clnum=0; clnum < sv.allocated_client_slots; cl++,clnum++)//fake dropping
		{
			if (cl->state < cs_connected)
				continue;
			else if (progstype == PROG_H2)
				cl->edict->ereftype = ER_FREE;	//hexen2 has some annoying prints. it never formally dropped clients on map changes (we'll reset this later, so they'll just not appear in the saved game).
			else if (!cl->spawned)	//don't drop if they are still connecting
				cl->edict->v->solid = 0;
			else
				SV_DespawnClient(cl);
		}
	}

	if (version >= CACHEGAME_VERSION_VERBOSE)
	{
		char buf[8192];
		char *mode = "?";
		switch(progstype)
		{
		case PROG_NONE:							break;
		case PROG_QW:		mode = "QW";		break;
		case PROG_NQ:		mode = "NQ";		break;
		case PROG_H2:		mode = "H2";		break;
		case PROG_PREREL:	mode = "PREREL";	break;
		case PROG_TENEBRAE: mode = "TENEBRAE";	break;
		case PROG_UNKNOWN:	mode = "UNKNOWN";	break;
		}
		VFS_PRINTF (f, "vmmode %s\n",			COM_QuotedString(mode, buf, sizeof(buf), false));
		VFS_PRINTF (f, "cvar skill %s\n",		COM_QuotedString(skill.string, buf, sizeof(buf), false));
		VFS_PRINTF (f, "cvar deathmatch %s\n",	COM_QuotedString(deathmatch.string, buf, sizeof(buf), false));
		VFS_PRINTF (f, "cvar coop %s\n",		COM_QuotedString(coop.string, buf, sizeof(buf), false));
		VFS_PRINTF (f, "cvar teamplay %s\n",	COM_QuotedString(teamplay.string, buf, sizeof(buf), false));
		VFS_PRINTF (f, "time %f\n",				sv.time);
		VFS_PRINTF (f, "map %s\n",				COM_QuotedString(svs.name, buf, sizeof(buf), false));
	}
	else
	{
		VFS_PRINTF (f, "%d\n", progstype);
		VFS_PRINTF (f, "%f\n", skill.value);
		VFS_PRINTF (f, "%f\n", deathmatch.value);
		VFS_PRINTF (f, "%f\n", coop.value);
		VFS_PRINTF (f, "%f\n", teamplay.value);
		VFS_PRINTF (f, "%s\n", svs.name);
		VFS_PRINTF (f, "%f\n", sv.time);

// write the light styles
		VFS_PRINTF (f, "%u\n",(unsigned)sv.maxlightstyles);
		for (i=0 ; i<sv.maxlightstyles ; i++)
		{
			VFS_PRINTF (f, "%s\n", sv.lightstyles[i].str?sv.lightstyles[i].str:"");
		}

		for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
		{
			if (sv.strings.model_precache[i] && *sv.strings.model_precache[i])
				VFS_PRINTF (f, "%s\n", sv.strings.model_precache[i]);
			else
				break;
		}
		VFS_PRINTF (f,"\n");
		for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
		{
			if (sv.strings.sound_precache[i] && *sv.strings.sound_precache[i])
				VFS_PRINTF (f, "%s\n", sv.strings.sound_precache[i]);
			else
				break;
		}
		VFS_PRINTF (f,"\n");
	}

	if (version == CACHEGAME_VERSION_MODSAVED)
	{
		struct globalvars_s *pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
		func_t func = PR_FindFunction(svprogfuncs, "SV_PerformSave", PR_ANY);

		//FIXME: save precaches here.
		VFS_PRINTF (f, "moddata\n");
		G_FLOAT(OFS_PARM0) = PR_QCFile_From_VFS(svprogfuncs, name, f, true);
		G_FLOAT(OFS_PARM1) = sv.world.num_edicts;
		G_FLOAT(OFS_PARM2) = sv.allocated_client_slots;
		PR_ExecuteProgram(svprogfuncs, func);
	}
	else
	{
		/*if (version >= CACHEGAME_VERSION_BINARY)
		{
			VFS_PUTS(f, va("%i\n", svprogfuncs->stringtablesize));
			VFS_WRITE(f, svprogfuncs->stringtable, svprogfuncs->stringtablesize);
		}
		else*/
		{
			s = PR_SaveEnts(svprogfuncs, NULL, &len, 0, 1);
			VFS_PUTS(f, s);
			VFS_PUTS(f, "\n");
			svprogfuncs->parms->memfree(s);
		}

		if (version >= CACHEGAME_VERSION_VERBOSE)
		{
			char buf[8192];
			for (i=0 ; i<sv.maxlightstyles ; i++)
				if (sv.lightstyles[i].str)
					VFS_PRINTF (f, "lightstyle %i %s %f %f %f\n", i, COM_QuotedString(sv.lightstyles[i].str, buf, sizeof(buf), false), sv.lightstyles[i].colours[0], sv.lightstyles[i].colours[1], sv.lightstyles[i].colours[2]);
			for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
				if (sv.strings.model_precache[i] && *sv.strings.model_precache[i])
					VFS_PRINTF (f, "model %i %s\n", i, COM_QuotedString(sv.strings.model_precache[i], buf, sizeof(buf), false));
			for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
				if (sv.strings.sound_precache[i] && *sv.strings.sound_precache[i])
					VFS_PRINTF (f, "sound %i %s\n", i, COM_QuotedString(sv.strings.sound_precache[i], buf, sizeof(buf), false));
			for (i=1 ; i<MAX_SSPARTICLESPRE ; i++)
				if (sv.strings.particle_precache[i] && *sv.strings.particle_precache[i])
					VFS_PRINTF (f, "particle %i %s\n", i, COM_QuotedString(sv.strings.particle_precache[i], buf, sizeof(buf), false));
#ifdef HAVE_LEGACY
			for (i = 0; i < sizeof(sv.strings.vw_model_precache)/sizeof(sv.strings.vw_model_precache[0]); i++)
				if (sv.strings.vw_model_precache[i])
					VFS_PRINTF (f, "vwep %i %s\n", i, COM_QuotedString(sv.strings.vw_model_precache[i], buf, sizeof(buf), false));
#endif

			PR_Common_SaveGame(f, svprogfuncs, false);//, version >= CACHEGAME_VERSION_BINARY);

			//FIXME: string buffers
			//FIXME: hash tables
			//FIXME: skeletal objects?
			//FIXME: static entities
			//FIXME: midi track
			//FIXME: custom temp-ents?
			//FIXME: pending uri_gets? (if only just to report fails on load)
			//FIXME: routing calls?
			//FIXME: sql queries?
			//FIXME: frik files?
			//FIXME: qc threads?

			//	portalblobsize = CM_WritePortalState(sv.world.worldmodel, &portalblob);
			//	VFS_WRITE(f, portalblob, portalblobsize);
		}

		VFS_CLOSE (f);
	}


	if (!dontharmgame)
	{
		for (clnum=0; clnum < sv.allocated_client_slots; clnum++)
		{
			edict_t *ed = EDICT_NUM_PB(svprogfuncs, clnum+1);
			ed->ereftype = ER_ENTITY;
		}
	}

	FS_FlushFSHashWritten(name);
}

//mapchange is true for Q2's map-change autosaves.
void SV_Savegame (const char *savename, qboolean mapchange)
{
	client_t *cl;
	int clnum;
	char	comment[(SAVEGAME_COMMENT_LENGTH+1)*2];
	vfsfile_t *f;
	int len;
	levelcache_t *cache;
	char *savefilename;

	if (!sv.state || sv.state == ss_clustermode)
	{
		Con_Printf("Server is not active - unable to save\n");
		return;
	}
	if (sv.state == ss_cinematic)
	{
		Con_Printf("Server is playing a cinematic - unable to save\n");
		return;
	}

#ifndef QUAKETC
	{
		int savefmt = sv_savefmt.ival;
		if (!*sv_savefmt.string && (svs.gametype != GT_PROGS || progstype == PROG_H2 || svs.levcache || (progstype == PROG_QW && strcmp(pr_ssqc_progs.string, "spprogs")) || (svs.gametype==GT_PROGS && PR_FindFunction(svprogfuncs, "SV_PerformSave", PR_ANY))))
			savefmt = 1;	//hexen2+q2/etc must not use the legacy format by default. can't use it when using any kind of hub system either (harder to detect upfront, which might give confused saved game naming but will at least work).
		else
			savefmt = sv_savefmt.ival;
		if (!savefmt && !mapchange)
		{
			if (SV_LegacySavegame(savename, *sv_savefmt.string))
				return;
			if (*sv_savefmt.string)
				Con_Printf("Unable to use legacy saved game format\n");
		}
	}
#endif

	switch(svs.gametype)
	{
	default:
	case GT_Q1QVM:
#ifdef VM_LUA
	case GT_LUA:
#endif
		Con_Printf("gamecode doesn't support saving\n");
		return;
	case GT_PROGS:
	case GT_QUAKE2:
		break;
	}

	if (sv.allocated_client_slots == 1 && svs.gametype == GT_PROGS)
	{
		if (svs.clients->state > cs_connected && svs.clients[0].edict->v->health <= 0)
		{
			Con_Printf("Refusing to save while dead.\n");
			return;
		}
	}
	//FIXME: we should probably block saving during intermission too.

	/*catch invalid names*/
	if (!*savename || strstr(savename, ".."))
		savename = "quick";

	savefilename = va("saves/%s/info.fsv", savename);
	FS_CreatePath(savefilename, FS_GAMEONLY);
	f = FS_OpenVFS(savefilename, "wbp", FS_GAMEONLY);
	if (!f)
	{
		Con_Printf("Couldn't open file saves/%s/info.fsv\n", savename);
		return;
	}
	SV_SavegameComment(comment, sizeof(comment));
	VFS_PRINTF (f, "%d\n", SAVEGAME_VERSION_FTE_HUB+svs.gametype);
	VFS_PRINTF (f, "%s\n", comment);

	VFS_PRINTF(f, "%i\n", sv.allocated_client_slots);
	for (cl = svs.clients, clnum=0; clnum < sv.allocated_client_slots; cl++,clnum++)
	{
		//FIXME: the qc is only told about the new client when the client finally sends a begin.
		//		 this means that if we save a client that is still connecting, the loading code HAS to assume that the qc already knows about the player.
		//		 this means that such players would not be loaded properly anyway, and would bug out the server.
		//		 so its best to not bother saving them at all. pro-top: don't save shortly after a map change in coop/sp.
		//istobeloaded means that the qc has already been told about the client from a previous saved game, regardless of the fact that they're still technically connecting (this may even be a zombie with no actual client connection).
		//note that autosave implies that we're saving on a map boundary. this is for q2 gamecode. q1 can't cope.
		if (cl->state < cs_spawned && !cl->istobeloaded)	//don't save if they are still connecting
		{
			VFS_PRINTF(f, "\n");
			continue;
		}
		VFS_PRINTF(f, "%s\n", cl->name);

		if (*cl->name)
		{
			char tmp[65536];
			VFS_PRINTF(f, "{\n");
			for (len = 0; len < NUM_SPAWN_PARMS; len++)
				VFS_PRINTF(f, "\tparm%i 0x%x //%.9g\n", len, *(int*)&cl->spawn_parms[len], cl->spawn_parms[len]);	//write hex as not everyone passes a float in the parms.
			VFS_PRINTF(f, "\tparm_string %s\n", COM_QuotedString(cl->spawn_parmstring?cl->spawn_parmstring:"", tmp, sizeof(tmp), false));
			/*if (cl->spawninfo)
			{
				VFS_PRINTF(f, "\tspawninfo %s\n", COM_QuotedString(cl->spawninfo, tmp, sizeof(tmp), false));
				VFS_PRINTF(f, "\tspawninfotime %9g\n", cl->spawninfotime);
			}*/
			VFS_PRINTF(f, "}\n");	//write ints as not everyone passes a float in the parms.
		}
	}

	InfoBuf_WriteToFile(f, &svs.info, NULL, 0);
	VFS_PUTS(f, "\n");
	InfoBuf_WriteToFile(f, &svs.localinfo, NULL, 0);

	VFS_PRINTF (f, "\n{\n");	//all game vars. FIXME: Should save the ones that have been retrieved/set by progs.
	VFS_PRINTF (f, "skill			\"%s\"\n",	skill.string);
	VFS_PRINTF (f, "deathmatch		\"%s\"\n",	deathmatch.string);
	VFS_PRINTF (f, "coop			\"%s\"\n",	coop.string);
	VFS_PRINTF (f, "teamplay		\"%s\"\n",	teamplay.string);

	VFS_PRINTF (f, "nomonsters		\"%s\"\n",	nomonsters.string);
	VFS_PRINTF (f, "gamecfg\t		\"%s\"\n",	gamecfg.string);
	VFS_PRINTF (f, "scratch1		\"%s\"\n",	scratch1.string);
	VFS_PRINTF (f, "scratch2		\"%s\"\n",	scratch2.string);
	VFS_PRINTF (f, "scratch3		\"%s\"\n",	scratch3.string);
	VFS_PRINTF (f, "scratch4		\"%s\"\n",	scratch4.string);
	VFS_PRINTF (f, "savedgamecfg\t	\"%s\"\n",	savedgamecfg.string);
	VFS_PRINTF (f, "saved1			\"%s\"\n",	saved1.string);
	VFS_PRINTF (f, "saved2			\"%s\"\n",	saved2.string);
	VFS_PRINTF (f, "saved3			\"%s\"\n",	saved3.string);
	VFS_PRINTF (f, "saved4			\"%s\"\n",	saved4.string);
	VFS_PRINTF (f, "temp1			\"%s\"\n",	temp1.string);
	VFS_PRINTF (f, "noexit			\"%s\"\n",	noexit.string);
	VFS_PRINTF (f, "pr_maxedicts\t	\"%s\"\n",	pr_maxedicts.string);
	VFS_PRINTF (f, "progs			\"%s\"\n",	pr_ssqc_progs.string);
	VFS_PRINTF (f, "set nextserver		\"%s\"\n",	Cvar_Get("nextserver", "", 0, "")->string);
	VFS_PRINTF (f, "}\n");

	SV_SaveLevelCache(savename, true);	//add the current level.

	cache = svs.levcache;	//state from previous levels - just copy it all accross.
	VFS_PRINTF(f, "{\n");
	while(cache)
	{
		VFS_PRINTF(f, "%s\n", cache->mapname);
		if (strcmp(cache->mapname, svs.name))
		{
			FS_Copy(va("saves/%s.lvc", cache->mapname), va("saves/%s/%s.lvc", savename, cache->mapname), FS_GAME, FS_GAME);
		}
		cache = cache->next;
	}
	VFS_PRINTF(f, "}\n");

	VFS_PRINTF (f, "%s\n", svs.name);

	VFS_PRINTF (f, "%i\n", svs.serverflags);

	VFS_CLOSE(f);

#ifdef HAVE_CLIENT
	//try to save screenshots automagically.
	Q_snprintfz(comment, sizeof(comment), "saves/%s/screeny.%s", savename, "tga");//scr_sshot_type.string);
	savefilename = comment;
	FS_Remove(savefilename, FS_GAMEONLY);
	if (cls.state == ca_active && qrenderer > QR_NONE && qrenderer != QR_VULKAN/*FIXME*/)
	{
		int stride;
		int width = vid.pixelwidth;
		int height = vid.pixelheight;
		image_t *img;
		uploadfmt_t format;
		void *rgbbuffer = SCR_ScreenShot_Capture(width, height, &stride, &format, false, false);
		if (rgbbuffer)
		{
//			extern cvar_t	scr_sshot_type;
			SCR_ScreenShot(savefilename, FS_GAMEONLY, &rgbbuffer, 1, stride, width, height, format, false);
			BZ_Free(rgbbuffer);

			//if any menu code has the shader loaded, we want to avoid them using a cache.
			//hopefully the menu code will unload as it goes, because these screenshots could be truely massive, as they're taken at screen resolution.
			//should probably use a smaller fbo or something, but whatever.
			img = Image_FindTexture(va("saves/%s/screeny.%s", savename, "tga"), NULL, 0);
			if (img)
			{
				if (Image_UnloadTexture(img))
				{
					//and then reload it so that any shaders using it don't get confused
					Image_GetTexture(va("saves/%s/screeny.%s", savename, "tga"), NULL, 0, NULL, NULL, 0, 0, TF_INVALID);
				}
			}
		}
	}
#endif

#ifdef Q2SERVER
	//save the player's inventory and other map-persistant state that is owned by the gamecode.
	if (ge)
	{
		char syspath[256];
		if (!FS_SystemPath(va("saves/%s/game.gsv", savename), FS_GAMEONLY, syspath, sizeof(syspath)))
			return;
		ge->WriteGame(syspath, mapchange);
		FS_FlushFSHashFull();
	}
	else
#endif
	{
		//fixme
		FS_FlushFSHashFull();
	}

	Q_strncpyz(sv.loadgame_on_restart, savename, sizeof(sv.loadgame_on_restart));
}


static int QDECL CompleteSaveList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	char trimmed[256];
	size_t l;
	char timetext[128];
	char desc[256];
	flocation_t loc;
	if (FS_FLocateFile(name, FSLF_QUIET|FSLF_DONTREFERENCE, &loc) && loc.search->handle != spath)
		;	//found in some other gamedir. don't show the dupe.
	else
	{
		Q_strncpyz(trimmed, name+6, sizeof(trimmed));
		l = strlen(trimmed);
		if (l >= 9 && !Q_strcasecmp(trimmed+l-9, "/info.fsv"))
		{
			trimmed[l-9] = 0;

			strftime(timetext, sizeof(timetext), "%a "S_COLOR_MAGENTA"%Y-%m-%d "S_COLOR_WHITE"%H:%M:%S", localtime(&mtime));
			Q_snprintfz(desc, sizeof(desc), "Modified %s\n^[\\h\\64\\img\\saves/%s/screeny.tga^]", timetext, trimmed);
			ctx->cb(trimmed, desc, NULL, ctx);
		}
	}
	return true;
}
#ifndef QUAKETC
static int QDECL CompleteSaveListLegacy (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	char stripped[64];
	char timetext[128];
	char desc[256];
	flocation_t loc;
	if (FS_FLocateFile(name, FSLF_QUIET|FSLF_DONTREFERENCE, &loc) && loc.search->handle != spath)
		;	//found in some other gamedir. don't show the dupe.
	else
	{
		COM_StripExtension(name, stripped, sizeof(stripped));
		strftime(timetext, sizeof(timetext), "%a "S_COLOR_MAGENTA"%Y-%m-%d "S_COLOR_WHITE"%H:%M:%S", localtime(&mtime));
		Q_snprintfz(desc, sizeof(desc), "%s, Modified %s", name, timetext);
		ctx->cb(stripped, desc, NULL, ctx);
	}
	return true;
}
#endif
void SV_Savegame_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		COM_EnumerateFiles(va("saves/%s*/info.fsv", partial), CompleteSaveList, ctx);
#ifndef QUAKETC
		COM_EnumerateFiles(va("%s*.sav", partial), CompleteSaveListLegacy, ctx);
#endif
	}
}

void SV_Savegame_f (void)
{
	if (sv.state == ss_clustermode && MSV_ForwardToAutoServer())
		;
	else if (Cmd_Argc() <= 2)
	{
		const char *savename = Cmd_Argv(1);
		if (strstr(savename, ".."))
		{
			Con_TPrintf ("Relative pathnames are not allowed\n");
			return;
		}
		//make sure the name is valid, eg if its omitted.
		if (!*savename || strstr(savename, ".."))
			savename = "quick";
#ifndef QUAKETC
		if (!Q_strcasecmp(Cmd_Argv(0), "savegame_legacy"))
		{
			if (SV_LegacySavegame(savename, true))
				return;
			Con_Printf("Unable to use legacy save format\n");
			return;
		}
#endif
		SV_Savegame(savename, false);
	}
	else
		Con_Printf("%s: invalid number of arguments\n", Cmd_Argv(0));
}

void SV_AutoSave(void)
{
#ifndef NOBUILTINMENUS
#ifndef SERVERONLY
	const char *autosavename;
	int i;
	if (sv_autosave.value <= 0)
		return;
	if (sv.state != ss_active)
		return;
	switch(svs.gametype)
	{
	default:	//probably broken. don't ever try.
		return;

	case GT_Q1QVM:
	case GT_PROGS:
		//don't bother to autosave multiplayer games.
		//this may be problematic with splitscreen, but coop rules tend to apply there anyway.
		if (sv.allocated_client_slots != 1)
			return;

		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			if (svs.clients[i].state == cs_spawned)
			{
				if (svs.clients[i].edict->v->health <= 0)
					return;	//autosaves with a dead player are just cruel.

				if ((int)svs.clients[i].edict->v->flags & (FL_GODMODE | FL_NOTARGET))
					return;	//autosaves to highlight cheaters is also just spiteful.

				if (svs.clients[i].edict->v->movetype != MOVETYPE_WALK)
					return;	//noclip|fly are cheaters, toss|bounce are bad at playing. etc.

				if (!((int)svs.clients[i].edict->v->flags & FL_ONGROUND))
					return;	//autosaves while people are jumping are awkward.

				if (svs.clients[i].edict->v->velocity[0] || svs.clients[i].edict->v->velocity[1] || svs.clients[i].edict->v->velocity[2])
					return;	//people running around are likely to result in poor saves
			}
		}
		break;
	}

	autosavename = M_ChooseAutoSave();
	Con_DPrintf("Autosaving to %s\n", autosavename);
	SV_Savegame(autosavename, false);

	sv.autosave_time = sv.time + sv_autosave.value * 60;
#endif
#endif
}

static void SV_SwapPlayers(client_t *a, client_t *b)
{
	size_t i;
	client_t tmp;
	if (a==b)
		return;	//o.O
	tmp = *a;
	*a = *b;
	*b = tmp;

	//swap over pointers for splitscreen.
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		if (svs.clients[i].controller == a)
			svs.clients[i].controller = b;
		else if (svs.clients[i].controller == b)
			svs.clients[i].controller = a;
		if (svs.clients[i].controlled == a)
			svs.clients[i].controlled = b;
		else if (svs.clients[i].controlled == b)
			svs.clients[i].controlled = a;
	}

	//undo some damage...
	b->edict = a->edict;
	a->edict = tmp.edict;

	if (a->name == b->namebuf)	a->name = a->namebuf;
	if (b->name == a->namebuf)	b->name = b->namebuf;
	if (a->team == b->teambuf)	a->team = a->teambuf;
	if (b->team == a->teambuf)	b->team = b->teambuf;

	if (a->netchan.message.data)
		a->netchan.message.data += (qbyte*)a-(qbyte*)b;
	if (a->datagram.data)
		a->datagram.data += (qbyte*)a-(qbyte*)b;
	if (a->backbuf.data)
		a->backbuf.data += (qbyte*)a-(qbyte*)b;

	if (b->netchan.message.data)
		b->netchan.message.data += (qbyte*)b-(qbyte*)a;
	if (b->datagram.data)
		b->datagram.data += (qbyte*)b-(qbyte*)a;
	if (b->backbuf.data)
		b->backbuf.data += (qbyte*)b-(qbyte*)a;
}
void SV_LoadPlayers(loadplayer_t *lp, size_t slots)
{	//loading games is messy as fuck
	//we need to reorder players to the order in the saved game.
	//swapping players around is really rather messy...

	client_t *cl;
	size_t clnum, p, p2;
	int to[255];

	//kick any splitscreen seats. they'll get re-added after load (filling slots like connecting players would)
	for (clnum = 0; clnum < svs.allocated_client_slots; clnum++)
	{
		cl = &svs.clients[clnum];
		cl->controlled = NULL;	//kill the links.
		if (cl->controller)	//its a slave
		{
			//unlink it
			cl->controller = NULL;

			//make it into a pseudo-bot so the kicking doesn't do weird stuff.
			cl->netchan.remote_address.type = NA_INVALID;	//so the remaining client doesn't get the kick too.
			cl->protocol = SCP_BAD;	//make it a bit like a bot, so we don't try sending any datagrams/reliables at someone that isn't able to receive anything.

			//okay, it can get lost now.
			cl->drop = true;
		}
	}

	//despawn any entity data, and try to find the loaded player to move them to
	for (clnum = 0; clnum < svs.allocated_client_slots; clnum++)	//clear the server for the level change.
	{
		to[clnum] = -1;
		cl = &svs.clients[clnum];
		SV_DespawnClient(cl);
		if (cl->state <= cs_loadzombie)
			continue;
		if (cl->state == cs_spawned)
			cl->state = cs_connected;

		//FIXME: try to match by guids (but we don't have saved guid info)

		//try to match the player to a slot by name.
		for (p = 0; p < slots; p++)
			if (*lp[p].name)
			{
				if (!strcmp(cl->name, lp[p].name) || slots == 1)
				{	//this player matched matched...
					to[clnum] = p;
					lp[p].source = cl;
					break;
				}
			}
	}
	//for loaded players that don't have a client go and find a player to spawn there, to try to deal with players that renamed themselves.
	for (p = 0; p < slots; p++)
	{
		if (!*lp[p].name || lp[p].source)
			continue;
		for (clnum = 0; clnum < svs.allocated_client_slots; clnum++)
		{
			cl = &svs.clients[clnum];
			if (cl->state <= cs_loadzombie)
				continue;
			if (to[clnum] >= 0)
				continue;	//was already mapped
			if (cl->spectator)
				continue;	//spectators shouldn't be pulled into a player against their will. it may still happen though.
			to[clnum] = p;
			lp[p].source = cl;

			SV_BroadcastPrintf(PRINT_HIGH, "%s reprises %s\n", cl->name, lp[p].name);
			break;
		}
	}

	//we walk the list in order, pulling from the appropriate slot.
	//we're swapping each time, so uninteresting players will bubble to the end instead of breaking our finalised list..
	//if we're swapping from an earlier slot then that slot wasn't relevant anyway.
	for (p = 0; p < slots; p++)
	{
		if (lp[p].source && lp[p].source!=&svs.clients[p])
		{
			SV_SwapPlayers(&svs.clients[p], lp[p].source);
			for (p2 = 0; p2 < slots; p2++)
			{
				if (p == p2)
					continue;
				if (lp[p2].source == &svs.clients[p])
					lp[p2].source = lp[p].source;
				else if (lp[p2].source == lp[p].source)
					lp[p2].source = &svs.clients[p];
			}
		}
	}

	if (slots > svs.allocated_client_slots)	//will be trimmed later
		SV_UpdateMaxPlayers(slots);
	for (cl = svs.clients, clnum=0; clnum < slots; cl++,clnum++)
	{
		if (*lp[clnum].name)
		{	//okay so we have a player ready for this slot.
			for (p = 0; p < NUM_SPAWN_PARMS; p++)
				cl->spawn_parms[p] = lp[clnum].parm[p].f;
			cl->spawn_parmstring = lp[clnum].parmstr;
			continue;
		}
		else if (cl->state > cs_zombie)
			SV_DropClient(cl);
/*
		Q_strncpyz(cl->namebuf, lp[clnum].name, sizeof(cl->namebuf));
		cl->name = cl->namebuf;
		if (*cl->namebuf)
		{
			cl->state = cs_loadzombie;
			cl->connection_started = realtime+20;
			cl->istobeloaded = true;	//the parms are known
			cl->userid = 0;

			memset(&cl->netchan, 0, sizeof(cl->netchan));

			for (p = 0; p < NUM_SPAWN_PARMS; p++)
				cl->spawn_parms[p] = lp[clnum].parm[p].f;
			cl->spawn_parmstring = lp[clnum].parmstr;
		}*/
	}
}

static void SV_GameLoaded(loadplayer_t *lp, size_t slots, const char *savename)
{
	size_t clnum;
	client_t *cl;

	//make sure autosave doesn't save too early.
	sv.autosave_time = sv.time + sv_autosave.value*60;

	//let the restart command know the name of the saved game to reload.
	Q_strncpyz(sv.loadgame_on_restart, savename, sizeof(sv.loadgame_on_restart));

	slots = min(slots, svs.allocated_client_slots);

	//make sure the player state is set up properly.
	for (clnum = 0; clnum < slots; clnum++)
	{
		cl = &svs.clients[clnum];
		cl->spawned = !!*lp[clnum].name;
		if (cl->spawned)
		{
			sv.spawned_client_slots++;
			Q_strncpyz(cl->namebuf, lp[clnum].name, sizeof(cl->namebuf));
		}

		if (svprogfuncs)
		{
			cl->name = PR_AddString(svprogfuncs, cl->namebuf, sizeof(cl->namebuf), false);
			cl->team = PR_AddString(svprogfuncs, cl->teambuf, sizeof(cl->teambuf), false);
			cl->edict = EDICT_NUM_PB(svprogfuncs, clnum+1);

#ifdef HEXEN2
			if (cl->edict)
				cl->playerclass = cl->edict->xv->playerclass;
			else
				cl->playerclass = 0;
#endif
#ifdef HAVE_LEGACY
			cl->edict->xv->clientcolors = cl->playercolor;
#endif
		}

		if (cl->state == cs_spawned)	//shouldn't have gotten past SV_SpawnServer, but just in case...
			cl->state = cs_connected;	//client needs new serverinfo.
		if (cl->spawned && cl->state < cs_connected)	//make sure the player slot is active when the gamecode thinks it was (with a loadzombie if needed)
		{
			cl->state = cs_loadzombie;
			cl->connection_started = realtime+20;
			cl->istobeloaded = true;	//the parms are known
			cl->userid = 0;
			memset(&cl->netchan, 0, sizeof(cl->netchan));
		}

		if (cl->controller)
			continue;
		if (cl->state>=cs_connected)
		{
			if (cl->protocol == SCP_QUAKE3)
				continue;
			if (cl->protocol == SCP_BAD)
				continue;

			host_client = cl;
#ifdef NQPROT
			if (ISNQCLIENT(host_client))
				SVNQ_New_f();
			else
#endif
				SV_New_f();
		}
	}
	host_client = NULL;

	for (clnum = 0; clnum < slots; clnum++) {		
		cl = &svs.clients[clnum];

		/* ensure angles are respected -eukara */
		if (svprogfuncs) {
			cl->edict->v->fixangle = 1;
		}

		//make sure userinfos match any renamed players.
		if (cl->state >= cs_connected) {
			SV_ExtractFromUserinfo(&svs.clients[clnum], true);
		}
	}
}

#ifndef QUAKETC

//expects the version to have already been parsed
static qboolean SV_Loadgame_Legacy(const char *savename, const char *filename, vfsfile_t *f, int version)
{
	//FIXME: Multiplayer save probably won't work with spectators.
	char	mapname[MAX_QPATH];
	float	time;
	char	str[32768];
	int		i;
	int pt;
	int lstyles;

	int slots;

	int clnum;
	char plname[32];

	int filelen, filepos;
	char *file;

	char *modelnames[MAX_PRECACHE_MODELS];
	char *soundnames[MAX_PRECACHE_SOUNDS];
	loadplayer_t lp[255];
	struct loadinfo_s loadinfo;

	loadinfo.numplayers = countof(lp);
	loadinfo.players = lp;

	if (version != SAVEGAME_VERSION_FTE_LEG && version != SAVEGAME_VERSION_NQ && version != SAVEGAME_VERSION_QW)
	{
		VFS_CLOSE (f);
		Con_TPrintf ("Unable to load savegame of version %i\n", version);
		return false;
	}
	VFS_GETS(f, str, sizeof(str));	//discard comment.
	Con_Printf("loading legacy game from %s...\n", filename);

	if (version == SAVEGAME_VERSION_NQ || version == SAVEGAME_VERSION_QW)
	{
		slots = 1;

#ifdef SERVERONLY
		Q_strncpyz(lp[0].name, "", sizeof(lp[0].name));
#else
		Q_strncpyz(lp[0].name, name.string, sizeof(lp[0].name));
#endif
		lp[0].parmstr = NULL;
		lp[0].source = NULL;

		for (i=0 ; i<16 ; i++)
		{
			VFS_GETS(f, str, sizeof(str));
			lp[0].parm[i].f = atof(str);
		}
		for (; i < countof(lp[0].parm); i++)
			lp[0].parm[i].i = 0;
	}
	else	//fte saves ALL the clients on the server.
	{
		VFS_GETS(f, str, strlen(str));
		slots = atoi(str);
		if (!slots || slots >= countof(lp))	//err
		{
			VFS_CLOSE(f);
			Con_Printf ("Corrupted save game");
			return false;
		}
		for (clnum = 0; clnum < slots; clnum++)
		{
			VFS_GETS(f, plname, sizeof(plname));
			COM_Parse(plname);
			Q_strncpyz(lp[clnum].name, com_token, sizeof(lp[clnum].name));
			lp[clnum].parmstr = NULL;
			lp[clnum].source = NULL;

			if (!*com_token)
				continue;

			//probably should be 32, rather than NUM_SPAWN_PARMS(64)
			for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			{
				VFS_GETS(f, str, sizeof(str));
				lp[clnum].parm[i].f = atof(str);
			}
			for (; i < countof(lp[clnum].parm); i++)
				lp[clnum].parm[i].i = 0;
		}
	}
	SV_LoadPlayers(lp, slots);

	if (version == SAVEGAME_VERSION_NQ || version == SAVEGAME_VERSION_QW)
	{
		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (Cvar_FindVar("skill"), atof(str));
		Cvar_SetValue (Cvar_FindVar("deathmatch"), 0);
		Cvar_SetValue (Cvar_FindVar("coop"), 0);
		Cvar_SetValue (Cvar_FindVar("teamplay"), 0);

		if (version == SAVEGAME_VERSION_NQ)
		{
			progstype = PROG_NQ;
			Cvar_Set (&pr_ssqc_progs, "progs.dat");	//NQ's progs.
		}
		else
		{
			progstype = PROG_QW;
			Cvar_Set (&pr_ssqc_progs, "spprogs");	//zquake's single player qw progs.
		}
		pt = 0;
	}
	else
	{
		VFS_GETS(f, str, sizeof(str));
		pt = atoi(str);

		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (Cvar_FindVar("skill"), atof(str));

		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (Cvar_FindVar("deathmatch"), atof(str));
		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (Cvar_FindVar("coop"), atof(str));
		VFS_GETS(f, str, sizeof(str));
		Cvar_SetValue (Cvar_FindVar("teamplay"), atof(str));
	}
	VFS_GETS(f, mapname, sizeof(mapname));
	VFS_GETS(f, str, sizeof(str));
	time = atof(str);

	SV_SpawnServer (mapname, NULL, false, false, slots);	//always inits MAX_CLIENTS slots. That's okay, because we can cut the max easily.
	if (sv.state != ss_active)
	{
		VFS_CLOSE (f);
		Con_TPrintf ("Couldn't load map\n");
		return false;
	}

	if (sv.allocated_client_slots != slots)
	{
		Con_TPrintf ("Player count changed\n");
		return false;
	}

// load the light styles

	lstyles = 64;
	if (lstyles > sv.maxlightstyles)
		Z_ReallocElements((void**)&sv.lightstyles, &sv.maxlightstyles, lstyles, sizeof(*sv.lightstyles));
	for (i=0 ; i<lstyles ; i++)
	{
		VFS_GETS(f, str, sizeof(str));
		if (sv.lightstyles[i].str)
			Z_Free((char*)sv.lightstyles[i].str);
		sv.lightstyles[i].str = Z_StrDup(str);
		sv.lightstyles[i].colours[0] = sv.lightstyles[i].colours[1] = sv.lightstyles[i].colours[2] = 1;
	}
	for (; i < sv.maxlightstyles; i++)
	{
		if (sv.lightstyles[i].str)
			Z_Free((char*)sv.lightstyles[i].str);
		sv.lightstyles[i].str = NULL;
	}

	//model names are pointers to vm-accessible memory. as that memory is going away, we need to destroy and recreate, which requires preserving them.
	for (i = 1; i < MAX_PRECACHE_MODELS; i++)
	{
		if (!sv.strings.model_precache[i])
		{
			modelnames[i] = NULL;
			break;
		}
		modelnames[i] = Z_StrDup(sv.strings.model_precache[i]);
	}
	for (i = 1; i < MAX_PRECACHE_SOUNDS; i++)
	{
		if (!sv.strings.sound_precache[i])
		{
			soundnames[i] = NULL;
			break;
		}
		soundnames[i] = Z_StrDup(sv.strings.sound_precache[i]);
	}

// load the edicts out of the savegame file
// the rest of the file is sent directly to the progs engine.

	if (version == SAVEGAME_VERSION_NQ || version == SAVEGAME_VERSION_QW)
		;//Q_InitProgs();	//reinitialize progs entirly.
	else
	{
		Q_SetProgsParms(false);
		svs.numprogs = 0;

		PR_Configure(svprogfuncs, PR_ReadBytesString(pr_ssqc_memsize.string), MAX_PROGS, pr_enable_profiling.ival);
		PR_RegisterFields();
		PR_InitEnts(svprogfuncs, sv.world.max_edicts);	//just in case the max edicts isn't set.
		progstype = pt;	//presumably the progs.dat will be what they were before.
	}

	//reload model names.
	for (i = 1; i < MAX_PRECACHE_MODELS; i++)
	{
		if (!modelnames[i])
			break;
		sv.strings.model_precache[i] = PR_AddString(svprogfuncs, modelnames[i], 0, false);
		Z_Free(modelnames[i]);
	}
	for (i = 1; i < MAX_PRECACHE_SOUNDS; i++)
	{
		if (!soundnames[i])
			break;
		sv.strings.sound_precache[i] = PR_AddString(svprogfuncs, soundnames[i], 0, false);
		Z_Free(soundnames[i]);
	}

	filepos = VFS_TELL(f);
	filelen = VFS_GETLEN(f);
	filelen -= filepos;
	file = BZ_Malloc(filelen+1+8);
	memset(file, 0, filelen+1+8);
	strcpy(file, "loadgame");
	clnum=VFS_READ(f, file+8, filelen);
	file[filelen+8]='\0';
	sv.world.edict_size=svprogfuncs->load_ents(svprogfuncs, file, &loadinfo, SV_SaveMemoryReset, NULL, SV_ExtendedSaveData);
	BZ_Free(file);

	PR_LoadGlabalStruct(false);

	pr_global_struct->time = sv.world.physicstime = sv.time = time;
	sv.starttime = Sys_DoubleTime() - sv.time;

	VFS_CLOSE(f);

	//FIXME: QSS+DP saved games have some / *\nkey values\nkey values\n* / thing in them to save precaches and stuff

	World_ClearWorld(&sv.world, true);

	SV_GameLoaded(lp, slots, savename);
	return true;
}
#endif

//Attempts to load a named saved game.
qboolean SV_Loadgame (const char *unsafe_savename)
{
	levelcache_t *cache;
	unsigned char str[MAX_LOCALINFO_STRING+1], *trim;
	unsigned char savename[MAX_QPATH];
	vfsfile_t *f;
	unsigned char filename[MAX_OSPATH];
	int version;
	int clnum;
	int slots;
	int p;
	client_t *cl;
	gametype_e gametype;
	loadplayer_t lp[255];

	struct
	{
		char *pattern;
		flocation_t loc;
	} savefiles[] =
	{
		{"saves/%s/info.fsv"},
#ifndef QUAKETC
		{"%s.sav"},
#endif
		{"%s"}
	};
	int bestd=0x7fffffff,best=0;
	time_t bestt=0,t;

	Q_strncpyz(savename, unsafe_savename, sizeof(savename));
	if (!*savename || strstr(savename, ".."))
	{	//if no args, or its invalid, try to pick the last one that was saved (of those listed in the menu)
		size_t n;
		static char *autoload[] = {	"quick", "a0", "a1", "a2",
									"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9"};
		strcpy(savename, "quick");	//default...

		for (n = 0; n < countof(autoload); n++)
		{
			for (p = 0; p < countof(savefiles)-1; p++)
			{
				int d = FS_FLocateFile(va(savefiles[p].pattern, autoload[n]), FSLF_DONTREFERENCE, &savefiles[p].loc);
				if (!d)
					continue;
				FS_GetLocMTime(&savefiles[p].loc, &t);
				if (d < bestd || (bestd==d&&t>bestt))
				{
					bestd = d;
					bestt = t;
					best = p;

					strcpy(savename, autoload[n]);
				}
			}
		}
	}

	for (p = 0; p < countof(savefiles); p++)
	{
		int d = FS_FLocateFile(va(savefiles[p].pattern, savename), FSLF_DONTREFERENCE, &savefiles[p].loc);
		if (!d)
			continue;
		FS_GetLocMTime(&savefiles[p].loc, &t);
		if (d < bestd || (bestd==d&&t>bestt))
		{
			bestd = d;
			bestt = t;
			best = p;
		}
	}

	Q_snprintfz (filename, sizeof(filename), savefiles[best].pattern, savename);
	f = FS_OpenReadLocation(filename, &savefiles[best].loc);
	if (!f)
	{
		Con_TPrintf ("ERROR: couldn't open %s.\n", filename);
		return false;
	}

#if defined(MENU_DAT) && !defined(SERVERONLY)
	MP_Toggle(0);
#endif

	VFS_GETS(f, str, sizeof(str)-1);
	version = atoi(str);
	if (version < SAVEGAME_VERSION_FTE_HUB || version >= SAVEGAME_VERSION_FTE_HUB+GT_MAX)
	{
#ifdef QUAKETC
		VFS_CLOSE (f);
		Con_TPrintf ("Unable to load savegame of version %i\n", version);
		return false;
#else
		return SV_Loadgame_Legacy(savename, filename, f, version);
#endif
	}

	gametype = version - SAVEGAME_VERSION_FTE_HUB;
	VFS_GETS(f, str, sizeof(str)-1);
#ifndef SERVERONLY
	if (!cls.state)
#endif
		Con_TPrintf ("Loading game from %s...\n", filename);



	VFS_GETS(f, str, sizeof(str)-1);
	slots = atoi(str);

	if (slots < 1 || slots > countof(lp))
	{
		VFS_CLOSE (f);
		Con_Printf ("invalid player count in saved game\n");
		return false;
	}

	for (cl = svs.clients, clnum=0; clnum < slots; cl++,clnum++)
	{
		VFS_GETS(f, str, sizeof(str)-1);
		str[sizeof(cl->namebuf)-1] = '\0';
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		strcpy(lp[clnum].name, str);
		lp[clnum].parmstr = NULL;
		lp[clnum].source = NULL;

		if (*str)
		{
			VFS_GETS(f, str, sizeof(str)-1);
			if (*str == '{')
			{
				while(VFS_GETS(f, str, sizeof(str)-1))
				{
					if (*str == '}')
						break;
					trim = COM_Parse(str);
					if (!strcmp(com_token, "parm_string"))
					{
						COM_Parse(trim);
						Z_Free(lp[clnum].parmstr);
						lp[clnum].parmstr = Z_StrDup(com_token);
					}
					else if (!strncmp(com_token, "parm", 4))
					{
						unsigned int parm = atoi(com_token+4);
						COM_Parse(trim);
						if (parm < NUM_SPAWN_PARMS)
						{
							if (!strncmp(com_token, "0x", 2))
								lp[clnum].parm[parm].i = strtoul(com_token, NULL, 16);
							else
								lp[clnum].parm[parm].f = strtod(com_token, NULL);
						}
					}
					else
						Con_Printf("Unknown player data: %s\n", com_token);
				}
			}
			else
			{	//we used to have N integers, where N was some random outdated constant.
				VFS_CLOSE (f);
				Con_Printf ("Incompatible saved game\n");
				return false;
			}
		}
	}

	SV_LoadPlayers(lp, slots);


	VFS_GETS(f, str, sizeof(str)-1);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	Info_RemovePrefixedKeys(str, '*');	//just in case
	InfoBuf_Clear(&svs.info, false);
	InfoBuf_FromString(&svs.info, str, true);

	VFS_GETS(f, str, sizeof(str)-1);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	Info_RemovePrefixedKeys(str, '*');	//just in case
	InfoBuf_Clear(&svs.localinfo, false);
	InfoBuf_FromString(&svs.localinfo, str, true);

	VFS_GETS(f, str, sizeof(str)-1);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	if (strcmp(str, "{"))
		SV_Error("Corrupt saved game\n");
	while(1)
	{
		if (!VFS_GETS(f, str, sizeof(str)-1))
			SV_Error("Corrupt saved game\n");
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		if (!strcmp(str, "}"))
			break;
		else if (*str)
			Cmd_ExecuteString(str, RESTRICT_RCON);
	}

	SV_FlushLevelCache();

	VFS_GETS(f, str, sizeof(str)-1);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;
	if (strcmp(str, "{"))
		SV_Error("Corrupt saved game\n");
	while(1)
	{
		if (!VFS_GETS(f, str, sizeof(str)-1))
			SV_Error("Corrupt saved game\n");
		for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
			*trim='\0';
		for (trim = str; *trim <= ' ' && *trim; trim++)
			;
		if (!strcmp(str, "}"))
			break;
		if (!*str)
			continue;

		cache = Z_Malloc(sizeof(levelcache_t)+strlen(str)+1);
		cache->mapname = (char *)(cache+1);
		strcpy(cache->mapname, str);
		cache->gametype = gametype;

		cache->next = svs.levcache;


		FS_Copy(va("saves/%s/%s.lvc", savename, cache->mapname), va("saves/%s.lvc", cache->mapname), FS_GAME, FS_GAMEONLY);

		svs.levcache = cache;
	}

	VFS_GETS(f, str, sizeof(str)-1);
	for (trim = str+strlen(str)-1; trim>=str && *trim <= ' '; trim--)
		*trim='\0';
	for (trim = str; *trim <= ' ' && *trim; trim++)
		;

	//serverflags is reset on restart, so we need to read the value as it was at the start of the current map.
	VFS_GETS(f, filename, sizeof(filename)-1);
	svs.serverflags = atof(filename);

	VFS_CLOSE(f);

#ifdef Q2SERVER
	if (gametype == GT_QUAKE2)
	{
		flocation_t loc;
		char *name = va("saves/%s/game.gsv", savename);
		if (!FS_FLocateFile(name, FSLF_IFFOUND, &loc))
			Con_Printf("Couldn't find %s.\n", name);
		else if (!*loc.rawname || loc.offset)
			Con_Printf("%s is inside a package and cannot be used by the quake2 gamecode.\n", name);
		else
		{
			SVQ2_InitGameProgs();
			if (ge)
				ge->ReadGame(loc.rawname);
		}
	}
#endif

	svs.gametype = gametype;
	SV_LoadLevelCache(savename, str, "", true);

	SV_GameLoaded(lp, slots, savename);
	return true;
}

void SV_Loadgame_f (void)
{
#ifdef HAVE_CLIENT
	if (!Renderer_Started() && !isDedicated)
	{
		Cbuf_AddText(va("wait;%s %s\n", Cmd_Argv(0), Cmd_Args()), Cmd_ExecLevel);
		return;
	}
	if (sv.state == ss_dead)
		CL_Disconnect(NULL);
#endif

	if (sv.state == ss_clustermode && MSV_ForwardToAutoServer())
		;
	else
		SV_Loadgame(Cmd_Argv(1));
}

#include "fs.h"
void SV_DeleteSavegame_f (void)
{
	const char *savename = Cmd_Argv(1);

	//either saves/$FOO/info.fsv (rmtree) or $FOO.sav (single file)
	//extensions are strictly implicit to limit damage.

	const char *fname;
	flocation_t loc;

	if (!*savename || *savename == '.' || strchr(savename, '/') || strchr(savename, '\\'))
	{
		Con_Printf("\"%s\" is not a valid saved game name to delete\n", savename);
		return;
	}

	fname = va("saves/%s/info.fsv", savename);
	if (FS_FLocateFile(fname, FSLF_IGNORELINKS|FSLF_DONTREFERENCE, &loc))
	{
		fname = va("saves/%s/", savename);
		if (FS_RemoveTree(loc.search->handle, fname))
			Con_Printf("Removed %s\n", fname);
		else
			Con_Printf("Unable to remove %s\n", fname);
	}

#ifndef QUAKETC
	fname = va("%s.sav", savename);
	if (FS_FLocateFile(fname, FSLF_IGNORELINKS|FSLF_DONTREFERENCE, &loc))
	{
		if (loc.search->handle->RemoveFile && loc.search->handle->RemoveFile(loc.search->handle, fname))
			Con_Printf("Removed %s\n", fname);
		else
			Con_Printf("Unable to remove %s\n", fname);
	}
#endif
}
#endif
