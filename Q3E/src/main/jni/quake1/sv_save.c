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
#include "prvm_cmds.h"
#include "sys.h"

/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5

void SV_Savegame_to(prvm_prog_t *prog, const char *name)
{
	qfile_t	*f;
	int		i, k, l, numbuffers, lightstyles = 64;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	char	line[MAX_INPUTLINE];
	qbool isserver;
	char	*s;

	// first we have to figure out if this can be saved in 64 lightstyles
	// (for Quake compatibility)
	for (i=64 ; i<MAX_LIGHTSTYLES ; i++)
		if (sv.lightstyles[i][0])
			lightstyles = i+1;

	isserver = prog == SVVM_prog;

	Con_Printf("Saving game to %s...\n", name);
	f = FS_OpenRealFile(name, "wb", false);
	if (!f)
	{
		Con_Print(CON_ERROR "ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", SAVEGAME_VERSION);

	memset(comment, 0, sizeof(comment));
	if(isserver)
		dpsnprintf(comment, sizeof(comment), "%-21.21s kills:%3i/%3i", PRVM_GetString(prog, PRVM_serveredictstring(prog->edicts, message)), (int)PRVM_serverglobalfloat(killed_monsters), (int)PRVM_serverglobalfloat(total_monsters));
	else
		dpsnprintf(comment, sizeof(comment), "(crash dump of %s progs)", prog->name);
	// convert space to _ to make stdio happy
	// LadyHavoc: convert control characters to _ as well
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (ISWHITESPACEORCONTROL(comment[i]))
			comment[i] = '_';
	comment[SAVEGAME_COMMENT_LENGTH] = '\0';

	FS_Printf(f, "%s\n", comment);
	if(isserver)
	{
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			FS_Printf(f, "%f\n", svs.clients[0].spawn_parms[i]);
		FS_Printf(f, "%d\n", current_skill);
		FS_Printf(f, "%s\n", sv.worldbasename);
		FS_Printf(f, "%f\n",sv.time);
	}
	else
	{
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			FS_Printf(f, "(dummy)\n");
		FS_Printf(f, "%d\n", 0);
		FS_Printf(f, "%s\n", "(dummy)");
		FS_Printf(f, "%f\n", host.realtime);
	}

	// write the light styles
	for (i=0 ; i<lightstyles ; i++)
	{
		if (isserver && sv.lightstyles[i][0])
			FS_Printf(f, "%s\n", sv.lightstyles[i]);
		else
			FS_Print(f,"m\n");
	}

	PRVM_ED_WriteGlobals (prog, f);
	for (i=0 ; i<prog->num_edicts ; i++)
	{
		FS_Printf(f,"// edict %d\n", i);
		//Con_Printf("edict %d...\n", i);
		PRVM_ED_Write (prog, f, PRVM_EDICT_NUM(i));
	}

#if 1
	FS_Printf(f,"/*\n");
	FS_Printf(f,"// DarkPlaces extended savegame\n");
	// darkplaces extension - extra lightstyles, support for color lightstyles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
		if (isserver && sv.lightstyles[i][0])
			FS_Printf(f, "sv.lightstyles %i %s\n", i, sv.lightstyles[i]);

	// darkplaces extension - model precaches
	for (i=1 ; i<MAX_MODELS ; i++)
		if (sv.model_precache[i][0])
			FS_Printf(f,"sv.model_precache %i %s\n", i, sv.model_precache[i]);

	// darkplaces extension - sound precaches
	for (i=1 ; i<MAX_SOUNDS ; i++)
		if (sv.sound_precache[i][0])
			FS_Printf(f,"sv.sound_precache %i %s\n", i, sv.sound_precache[i]);

	// darkplaces extension - save buffers
	numbuffers = (int)Mem_ExpandableArray_IndexRange(&prog->stringbuffersarray);
	for (i = 0; i < numbuffers; i++)
	{
		prvm_stringbuffer_t *stringbuffer = (prvm_stringbuffer_t*) Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, i);
		if(stringbuffer && (stringbuffer->flags & STRINGBUFFER_SAVED))
		{
			FS_Printf(f,"sv.buffer %i %i \"string\"\n", i, stringbuffer->flags & STRINGBUFFER_QCFLAGS);
			for(k = 0; k < stringbuffer->num_strings; k++)
			{
				if (!stringbuffer->strings[k])
					continue;
				// Parse the string a bit to turn special characters
				// (like newline, specifically) into escape codes
				s = stringbuffer->strings[k];
				for (l = 0;l < (int)sizeof(line) - 2 && *s;)
				{	
					if (*s == '\n')
					{
						line[l++] = '\\';
						line[l++] = 'n';
					}
					else if (*s == '\r')
					{
						line[l++] = '\\';
						line[l++] = 'r';
					}
					else if (*s == '\\')
					{
						line[l++] = '\\';
						line[l++] = '\\';
					}
					else if (*s == '"')
					{
						line[l++] = '\\';
						line[l++] = '"';
					}
					else
						line[l++] = *s;
					s++;
				}
				line[l] = '\0';
				FS_Printf(f,"sv.bufstr %i %i \"%s\"\n", i, k, line);
			}
		}
	}
	FS_Printf(f,"*/\n");
#endif

	FS_Close (f);
#ifdef __EMSCRIPTEN__
	js_syncFS(false);
#endif
	Con_Print("done.\n");
}

/*
===============
SV_Savegame_f
===============
*/
void SV_Savegame_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char	name[MAX_QPATH];

	if (!sv.active)
	{
		Con_Print(CON_ERROR "Can't save - no server running.\n");
		return;
	}

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(cmd, 1), ".."))
	{
		Con_Print(CON_ERROR "Relative pathnames are not allowed.\n");
		return;
	}

	for (int i = 0; i < svs.maxclients; ++i)
		if (svs.clients[i].active && PRVM_serveredictfloat(svs.clients[i].edict, deadflag))
		{
			Con_Print(CON_ERROR "Can't savegame with a dead player\n");
			return;
		}

	// bones_was_here: intermission_running isn't declared in dpdefs, but it's used by
	// id1 Quake, all Quake mods (afaict), Nexuiz and Xonotic.
	if (PRVM_serverglobalfloat(intermission_running))
	{
		Con_Print(CON_ERROR "Can't save in intermission.\n");
		return;
	}

	if (SV_IsLocalServer() != 1)
		Con_Print(CON_WARN "Warning: saving a multiplayer game may have strange results when restored (to properly resume, all players must join in the same player slots and then the game can be reloaded).\n");

	dp_strlcpy (name, Cmd_Argv(cmd, 1), sizeof (name));
	FS_DefaultExtension (name, ".sav", sizeof (name));

	SV_Savegame_to(prog, name);
}

/*
===============
SV_Loadgame_f
===============
*/
void SV_Loadgame_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char filename[MAX_QPATH];
	char mapname[MAX_QPATH];
	float time;
	const char *start;
	const char *end;
	const char *t;
	char *text;
	prvm_edict_t *ent;
	int i, k, numbuffers;
	int entnum;
	int version;
	float spawn_parms[NUM_SPAWN_PARMS];
	prvm_stringbuffer_t *stringbuffer;

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("load <savename> : load a game\n");
		return;
	}

	dp_strlcpy (filename, Cmd_Argv(cmd, 1), sizeof(filename));
	FS_DefaultExtension (filename, ".sav", sizeof (filename));

	Con_Printf("Loading game from %s...\n", filename);

	if(host.hook.Disconnect)
		host.hook.Disconnect(false, NULL);

	if(host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	cls.demonum = -1;		// stop demo loop in case this fails

	t = text = (char *)FS_LoadFile (filename, tempmempool, false, NULL);
	if (!text)
	{
		Con_Print(CON_ERROR "ERROR: couldn't open.\n");
		return;
	}

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: loading version\n");

	// version
	COM_ParseToken_Simple(&t, false, false, true);
	version = atoi(com_token);
	if (version != SAVEGAME_VERSION)
	{
		Mem_Free(text);
		Con_Printf(CON_ERROR "Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return;
	}

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: loading description\n");

	// description
	COM_ParseToken_Simple(&t, false, false, true);

	for (i = 0;i < NUM_SPAWN_PARMS;i++)
	{
		COM_ParseToken_Simple(&t, false, false, true);
		spawn_parms[i] = atof(com_token);
	}
	// skill
	COM_ParseToken_Simple(&t, false, false, true);
// this silliness is so we can load 1.06 save files, which have float skill values
	current_skill = (int)(atof(com_token) + 0.5);
	Cvar_SetValueQuick(&skill, (float)current_skill);

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: loading mapname\n");

	// mapname
	COM_ParseToken_Simple(&t, false, false, true);
	dp_strlcpy (mapname, com_token, sizeof(mapname));

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: loading time\n");

	// time
	COM_ParseToken_Simple(&t, false, false, true);
	time = atof(com_token);

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: spawning server\n");

	SV_SpawnServer (mapname);
	if (!sv.active)
	{
		Mem_Free(text);
		Con_Printf(CON_ERROR "Couldn't load map \"%s\"\n", mapname);
		return;
	}
	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: loading light styles\n");

// load the light styles

	// -1 is the globals
	entnum = -1;

	for (i = 0;i < MAX_LIGHTSTYLES;i++)
	{
		// light style
		start = t;
		COM_ParseToken_Simple(&t, false, false, true);
		// if this is a 64 lightstyle savegame produced by Quake, stop now
		// we have to check this because darkplaces may save more than 64
		if (com_token[0] == '{')
		{
			t = start;
			break;
		}
		dp_strlcpy(sv.lightstyles[i], com_token, sizeof(sv.lightstyles[i]));
	}

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: skipping until globals\n");

	// now skip everything before the first opening brace
	// (this is for forward compatibility, so that older versions (at
	// least ones with this fix) can load savegames with extra data before the
	// first brace, as might be produced by a later engine version)
	for (;;)
	{
		start = t;
		if (!COM_ParseToken_Simple(&t, false, false, true))
			break;
		if (com_token[0] == '{')
		{
			t = start;
			break;
		}
	}

	// unlink all entities
	World_UnlinkAll(&sv.world);

// load the edicts out of the savegame file
	end = t;
	for (;;)
	{
		start = t;
		while (COM_ParseToken_Simple(&t, false, false, true))
			if (!strcmp(com_token, "}"))
				break;
		if (!COM_ParseToken_Simple(&start, false, false, true))
		{
			// end of file
			break;
		}
		if (strcmp(com_token,"{"))
		{
			Mem_Free(text);
			Host_Error ("%s: first token isn't a brace", __func__);
		}

		if (entnum == -1)
		{
			if(developer_entityparsing.integer)
				Con_Printf("SV_Loadgame_f: loading globals\n");

			// parse the global vars
			PRVM_ED_ParseGlobals (prog, start);

			// restore the autocvar globals
			Cvar_UpdateAllAutoCvars(prog->console_cmd->cvars);
		}
		else
		{
			// parse an edict
			if (entnum >= MAX_EDICTS)
			{
				Mem_Free(text);
				Host_Error("%s: too many edicts in save file (reached MAX_EDICTS %i)", __func__, MAX_EDICTS);
			}
			while (entnum >= prog->max_edicts)
				PRVM_MEM_IncreaseEdicts(prog);
			ent = PRVM_EDICT_NUM(entnum);
			memset(ent->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
			ent->free = false;

			if(developer_entityparsing.integer)
				Con_Printf("SV_Loadgame_f: loading edict %d\n", entnum);

			PRVM_ED_ParseEdict (prog, start, ent, true);

			// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict(ent);
		}

		end = t;
		entnum++;
	}

	prog->num_edicts = entnum;
	sv.time = time;

	for (i = 0;i < NUM_SPAWN_PARMS;i++)
		svs.clients[0].spawn_parms[i] = spawn_parms[i];

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: skipping until extended data\n");

	// read extended data if present
	// the extended data is stored inside a /* */ comment block, which the
	// parser intentionally skips, so we have to check for it manually here
	if(end)
	{
		while (*end == '\r' || *end == '\n')
			end++;
		if (end[0] == '/' && end[1] == '*' && (end[2] == '\r' || end[2] == '\n'))
		{
			if(developer_entityparsing.integer)
				Con_Printf("SV_Loadgame_f: loading extended data\n");

			Con_Printf("Loading extended DarkPlaces savegame\n");
			t = end + 2;
			memset(sv.lightstyles[0], 0, sizeof(sv.lightstyles));
			memset(sv.model_precache[0], 0, sizeof(sv.model_precache));
			memset(sv.sound_precache[0], 0, sizeof(sv.sound_precache));
			BufStr_Flush(prog);

			while (COM_ParseToken_Simple(&t, false, false, true))
			{
				if (!strcmp(com_token, "sv.lightstyles"))
				{
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_LIGHTSTYLES)
						dp_strlcpy(sv.lightstyles[i], com_token, sizeof(sv.lightstyles[i]));
					else
						Con_Printf(CON_WARN "unsupported lightstyle %i \"%s\"\n", i, com_token);
				}
				else if (!strcmp(com_token, "sv.model_precache"))
				{
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_MODELS)
					{
						dp_strlcpy(sv.model_precache[i], com_token, sizeof(sv.model_precache[i]));
						sv.models[i] = Mod_ForName (sv.model_precache[i], true, false, sv.model_precache[i][0] == '*' ? sv.worldname : NULL);
					}
					else
						Con_Printf(CON_WARN "unsupported model %i \"%s\"\n", i, com_token);
				}
				else if (!strcmp(com_token, "sv.sound_precache"))
				{
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_SOUNDS)
						dp_strlcpy(sv.sound_precache[i], com_token, sizeof(sv.sound_precache[i]));
					else
						Con_Printf(CON_WARN "unsupported sound %i \"%s\"\n", i, com_token);
				}
				else if (!strcmp(com_token, "sv.buffer"))
				{
					if (COM_ParseToken_Simple(&t, false, false, true))
					{
						i = atoi(com_token);
						if (i >= 0)
						{
							k = STRINGBUFFER_SAVED;
							if (COM_ParseToken_Simple(&t, false, false, true))
								k |= atoi(com_token);
							if (!BufStr_FindCreateReplace(prog, i, k, "string"))
								Con_Printf(CON_ERROR "failed to create stringbuffer %i\n", i);
						}
						else
							Con_Printf(CON_WARN "unsupported stringbuffer index %i \"%s\"\n", i, com_token);
					}
					else
						Con_Printf(CON_WARN "unexpected end of line when parsing sv.buffer (expected buffer index)\n");
				}
				else if (!strcmp(com_token, "sv.bufstr"))
				{
					if (!COM_ParseToken_Simple(&t, false, false, true))
						Con_Printf(CON_WARN "unexpected end of line when parsing sv.bufstr\n");
					else
					{
						i = atoi(com_token);
						stringbuffer = BufStr_FindCreateReplace(prog, i, STRINGBUFFER_SAVED, "string");
						if (stringbuffer)
						{
							if (COM_ParseToken_Simple(&t, false, false, true))
							{
								k = atoi(com_token);
								if (COM_ParseToken_Simple(&t, false, false, true))
									BufStr_Set(prog, stringbuffer, k, com_token);
								else
									Con_Printf(CON_WARN "unexpected end of line when parsing sv.bufstr (expected string)\n");
							}
							else
								Con_Printf(CON_WARN "unexpected end of line when parsing sv.bufstr (expected strindex)\n");
						}
						else
							Con_Printf(CON_ERROR "failed to create stringbuffer %i \"%s\"\n", i, com_token);
					}
				}	
				// skip any trailing text or unrecognized commands
				while (COM_ParseToken_Simple(&t, true, false, true) && strcmp(com_token, "\n"))
					;
			}
		}
	}
	Mem_Free(text);

	// remove all temporary flagged string buffers (ones created with BufStr_FindCreateReplace)
	numbuffers = (int)Mem_ExpandableArray_IndexRange(&prog->stringbuffersarray);
	for (i = 0; i < numbuffers; i++)
	{
		if ( (stringbuffer = (prvm_stringbuffer_t *)Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, i)) )
			if (stringbuffer->flags & STRINGBUFFER_TEMP)
				BufStr_Del(prog, stringbuffer);
	}

	if(developer_entityparsing.integer)
		Con_Printf("SV_Loadgame_f: finished\n");

	// make sure we're connected to loopback
	if(sv.active && host.hook.ConnectLocal)
		host.hook.ConnectLocal();
}
