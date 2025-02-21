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
#ifdef SQL
#include "sv_sql.h"
#endif
#ifdef __GLIBC__
#include <malloc.h>	//for malloc_trim
#endif

#ifndef CLIENTONLY
extern int			total_loading_size, current_loading_size, loading_stage;
char *T_GetString(int num);

void SVQ2_Ents_Shutdown(void);
#define Q2EDICT_NUM(i) (q2edict_t*)((char *)ge->edicts+(i)*ge->edict_size)

server_static_t	svs;				// persistant server info
server_t		sv;					// local server

entity_state_t *sv_staticentities;
int sv_max_staticentities;
staticsound_state_t *sv_staticsounds;
int sv_max_staticsounds;

extern cvar_t	skill;
extern cvar_t	sv_cheats;
extern cvar_t	sv_bigcoords;
extern cvar_t	sv_gamespeed;
extern cvar_t	sv_csqc_progname;
extern cvar_t	sv_calcphs;
extern cvar_t	sv_playerslots, maxclients, maxspectators;
extern cvar_t	sv_nqplayerphysics; //auto setting needs updating on map changes

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (const char *name)
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i=1 ; i<MAX_PRECACHE_MODELS && sv.strings.model_precache[i] ; i++)
		if (!strcmp(sv.strings.model_precache[i], name))
			return i;
	if (i==MAX_PRECACHE_MODELS || !sv.strings.model_precache[i])
	{
		if (i!=MAX_PRECACHE_MODELS)
		{
#ifdef VM_Q1
			if (svs.gametype == GT_Q1QVM)
				sv.strings.model_precache[i] = name;
			else
#endif
				sv.strings.model_precache[i] = PR_AddString(svprogfuncs, name, 0, false);
			if (!strcmp(name + strlen(name) - 4, ".bsp"))
				sv.models[i] = Mod_FindName(Mod_FixName(sv.strings.model_precache[i], sv.strings.model_precache[1]));

			Con_DPrintf("WARNING: SV_ModelIndex: model %s not precached\n", name);

			if (sv.state != ss_loading)
			{
				MSG_WriteByte(&sv.reliable_datagram, svcfte_precache);
				MSG_WriteShort(&sv.reliable_datagram, i);
				MSG_WriteString(&sv.reliable_datagram, sv.strings.model_precache[i]);
#ifdef NQPROT
				MSG_WriteByte(&sv.nqreliable_datagram, svcdp_precache);
				MSG_WriteShort(&sv.nqreliable_datagram, i);
				MSG_WriteString(&sv.nqreliable_datagram, sv.strings.model_precache[i]);
#endif
			}
		}
	}
	return i;
}

//looks up a name->index without caching it
int SV_SafeModelIndex (char *name)
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i=1 ; i<MAX_PRECACHE_MODELS && sv.strings.model_precache[i] ; i++)
		if (!strcmp(sv.strings.model_precache[i], name))
			return i;
	if (i==MAX_PRECACHE_MODELS || !sv.strings.model_precache[i])
	{
		return 0;
	}
	return i;
}

/*
================
SV_FlushSignon

Moves to the next signon buffer if needed
This stops any chunk from getting too large, hopefully, but if the worst happens then hopefully network fragmentation will work.
================
*/
void SV_FlushSignon (qboolean force)
{	//flush only when it gets too big.
	if (sv.signon.cursize < MAX_DATAGRAM - 512)
	{
		if (!force || !sv.signon.cursize)
			return;
	}

	if (sv.signon.cursize)
	{
		sv.signon.data[-2] = (sv.signon.cursize>>0)&0xff;
		sv.signon.data[-1] = (sv.signon.cursize>>8)&0xff;
		sv.used_signon_space += 2+sv.signon.cursize;
	}

	sv.signon.data = sv.signon_buffer + sv.used_signon_space+2;
	sv.signon.maxsize = sizeof(sv.signon_buffer) - (sv.used_signon_space+2);
	sv.signon.cursize = 0;
	sv.signon.prim = svs.netprim;
}
#ifdef SERVER_DEMO_PLAYBACK
void SV_FlushDemoSignon (void)
{
	if (sv.demosignon.cursize < sv.demosignon.maxsize - 512)
		return;

	if (sv.num_demosignon_buffers == MAX_SIGNON_BUFFERS-1)
		SV_Error ("sv.num_demosignon_buffers == MAX_SIGNON_BUFFERS-1");

	sv.demosignon_buffer_size[sv.num_demosignon_buffers-1] = sv.demosignon.cursize;
	sv.demosignon.data = sv.demosignon_buffers[sv.num_demosignon_buffers];
	sv.num_demosignon_buffers++;
	sv.demosignon.cursize = 0;
}
#endif
/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
/*void SV_CreateBaseline (void)
{
	int			i;
	edict_t			*svent;
	int				entnum;

	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		// create baselines for all player slots,
		// and any other edict that has a visible model
		if (entnum > svs.allocated_client_slots && !svent->v->modelindex)
			continue;

	//
	// create entity baseline
	//
		VectorCopy (svent->v->origin, svent->baseline.origin);
		VectorCopy (svent->v->angles, svent->baseline.angles);
		svent->baseline.frame = svent->v->frame;
		svent->baseline.skinnum = svent->v->skin;
		if (entnum > 0 && entnum <= svs.allocated_client_slots)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl")&255;
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(PR_GetString(svent->v->model))&255;
		}
#ifdef PEXT_SCALE
		svent->baseline.scale = 1;
#endif
#ifdef PEXT_TRANS
		svent->baseline.trans = 1;
#endif

		//
		// flush the signon message out to a seperate buffer if
		// nearly full
		//
		SV_FlushSignon ();

		//
		// add to the message
		//
		MSG_WriteByte (&sv.signon,svc_spawnbaseline);
		MSG_WriteShort (&sv.signon,entnum);

		MSG_WriteByte (&sv.signon, svent->baseline.modelindex);
		MSG_WriteByte (&sv.signon, svent->baseline.frame);
		MSG_WriteByte (&sv.signon, svent->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->baseline.skinnum);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
			MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
		}
	}
}
*/

void SVQ1_CreateBaseline (void)
{
	edict_t			*svent;
	int				entnum;
	extern entity_state_t nullentitystate;

	int playermodel = SV_SafeModelIndex("progs/player.mdl");

	for (entnum = 0; entnum < sv.world.num_edicts ; entnum++)
	{
		svent = EDICT_NUM_PB(svprogfuncs, entnum);

		memcpy(&svent->baseline, &nullentitystate, sizeof(entity_state_t));
		svent->baseline.number = entnum;

		if (ED_ISFREE(svent))
			continue;
		// create baselines for all player slots,
		// and any other edict that has a visible model
		if (entnum > sv.allocated_client_slots && !svent->v->modelindex)
			continue;

	//
	// create entity baseline
	//
		SV_Snapshot_BuildStateQ1(&svent->baseline, svent, NULL, NULL);

		if (entnum > 0 && entnum <= sv.allocated_client_slots)
		{
			if (entnum > 0 && entnum <= 16)
				svent->baseline.colormap = entnum;
			else
				svent->baseline.colormap = 0;	//this would crash NQ.

			if (!svent->baseline.solidsize)
				svent->baseline.solidsize = ES_SOLID_HULL1;
			if (!svent->baseline.modelindex)
				svent->baseline.modelindex = playermodel;
		}
		svent->baseline.modelindex&=255;	//FIXME

		if (!svent->baseline.modelindex)
		{
			memcpy(&svent->baseline, &nullentitystate, sizeof(entity_state_t));
			svent->baseline.number = entnum;
		}
	}
}

void SV_SpawnParmsToQC(client_t *client)
{
	int i;
	// copy spawn parms out of the client_t
	for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
	{
		if (pr_global_ptrs->spawnparamglobals[i])
			*pr_global_ptrs->spawnparamglobals[i] = client->spawn_parms[i];
	}
	if (pr_global_ptrs->parm_string)
		*pr_global_ptrs->parm_string = client->spawn_parmstring?PR_TempString(sv.world.progs, client->spawn_parmstring):0;
}

void SV_SpawnParmsToClient(client_t *client)
{
	int i;
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	{
		if (pr_global_ptrs->spawnparamglobals[i])
			client->spawn_parms[i] = *pr_global_ptrs->spawnparamglobals[i];
		else
			client->spawn_parms[i] = 0;
	}
	Z_Free(client->spawn_parmstring);
	if (pr_global_ptrs->parm_string)
		client->spawn_parmstring = Z_StrDup(PR_GetString(sv.world.progs, *pr_global_ptrs->parm_string));
	else
		client->spawn_parmstring = NULL;
}

void SV_SaveSpawnparmsClient(client_t *client, float *transferparms)
{
	int j;
	eval_t *eval;
	SV_SpawnParmsToQC(client);

#ifdef VM_Q1
	if (svs.gametype == GT_Q1QVM)
	{
		pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, client->edict);
		Q1QVM_SetChangeParms();
	}
	else
#endif
		if (pr_global_ptrs->SetChangeParms)
	{
		func_t setparms = 0;
		if (transferparms)
		{
			setparms = PR_FindFunction(svprogfuncs, "SetTransferParms", PR_ANY);
			if (!setparms)
				setparms = pr_global_struct->SetChangeParms;
		}
		else
			setparms = pr_global_struct->SetChangeParms;
		pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, client->edict);
		PR_ExecuteProgram (svprogfuncs, setparms);
	}

	if (transferparms)
	{
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
		{
			if (pr_global_ptrs->spawnparamglobals[j])
				transferparms[j] = *pr_global_ptrs->spawnparamglobals[j];
		}
		return;
	}
	else
	{
		SV_SpawnParmsToClient(client);
	}

	// call the progs to get default spawn parms for the new client
	eval = PR_FindGlobal(svprogfuncs, "ClientReEnter", 0, NULL);
	if (eval && eval->function)
	{//oooh, evil.
		char buffer[65536*4];
		size_t bufsize = 0;
		char *buf;
//		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
//			client->spawn_parms[j] = 0;

		buf = svprogfuncs->saveent(svprogfuncs, buffer, &bufsize, sizeof(buffer), client->edict);

		if (client->spawninfo)
			Z_Free(client->spawninfo);
		client->spawninfo = Z_Malloc(bufsize+1);
		memcpy(client->spawninfo, buf, bufsize+1);
		client->spawninfotime = sv.time;
	}

#ifdef SVRANKING
	if (client->rankid)
	{
		rankstats_t rs;
		if (Rank_GetPlayerStats(client->rankid, &rs))
		{
			rs.timeonserver += realtime - client->stats_started;
			client->stats_started = realtime;
			rs.kills += client->kills;
			rs.deaths += client->deaths;
			client->kills=0;
			client->deaths=0;
			for (j=0 ; j<NUM_RANK_SPAWN_PARMS ; j++)
			{
				rs.parm[j] = client->spawn_parms[j];
			}
			Rank_SetPlayerStats(client->rankid, &rs);
		}
	}
#endif
}

/*
================
SV_SaveSpawnparms

Grabs the current state of the progs serverinfo flags
and each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms (void)
{
	int		i;

	if (!sv.state)
		return;		// no progs loaded yet

	if (!svprogfuncs)
		return;

	// serverflags is the only game related thing maintained
	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients ; i<sv.allocated_client_slots ; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
			continue;

		SV_SaveSpawnparmsClient(host_client, NULL);
	}
}

void SV_GetNewSpawnParms(client_t *cl)
{
	if (svprogfuncs)	//q2 dlls don't use parms in this manner. It's all internal to the dll.
	{
		// call the progs to get default spawn parms for the new client
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
			Q1QVM_SetNewParms();
		else
#endif
		{
			if (pr_global_ptrs->SetNewParms)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetNewParms);
		}

		SV_SpawnParmsToClient(cl);
	}
}

/*
================
SV_CalcPHS

Expands the PVS and calculates the PHS
(Potentially Hearable Set)
================
*/
void SV_CalcPHS (void)
{
	int		rowbytes, rowwords;
	int		i, j, k, l, index, num;
	int		bitbyte;
	unsigned	*dest, *src;
	qbyte	*scan, *pvs;
	int		count, vcount;
	model_t *model = sv.world.worldmodel;
	pvsbuffer_t buf;

	if (model->pvs || model->fromgame == fg_quake2 || model->fromgame == fg_quake3)
	{
		//PHS calcs are pointless with Q2 bsps
		return;
	}

	//FIXME: this can take a significant time on some maps, and should ideally be pushed to a worker thread.
	num = model->numclusters;
	rowbytes = model->pvsbytes;
	rowwords = rowbytes/sizeof(*dest);
	buf.buffersize = model->pvsbytes;

	if (!sv_calcphs.ival || (sv_calcphs.ival == 2 && (rowbytes*num >= 0x100000 || (!deathmatch.ival && !coop.ival))))
	{
		pvs = NULL;/*ZG_Malloc(&model->memgroup, rowbytes*num);
		scan = pvs;
		for (i=0 ; i<num ; i++, scan+=rowbytes)
		{
			buf.buffer = scan;
			model->funcs.ClusterPVS(model, i, &buf, PVM_REPLACE);
		}*/

		Con_DPrintf("Skipping PHS\n");
		model->pvs = pvs;
		model->phs = NULL;
		return;
	}

	pvs = ZG_Malloc(&model->memgroup, rowbytes*num);
	scan = pvs;
	vcount = 0;
	for (i=0 ; i<num ; i++, scan+=rowbytes)
	{
		buf.buffer = scan;
		model->funcs.ClusterPVS(model, i, &buf, PVM_REPLACE);
		if (i == 0)
			continue;
		for (j=0 ; j<num ; j++)
		{
			if ( scan[j>>3] & (1<<(j&7)) )
			{
				vcount++;
			}
		}
	}
	if (developer.value)
		Con_TPrintf ("Building PHS...\n");

	model->pvs = pvs;
	model->phs = ZG_Malloc (&model->memgroup, rowbytes*num);

	/*this routine takes an exponential amount of time, so cache it if its too big*/
	if (rowbytes*num >= 0x100000)
	{
		char hdr[8];
		vfsfile_t *f = FS_OpenVFS(va("maps/%s.phs", svs.name), "rb", FS_GAME);
		if (f)
		{
			VFS_READ(f, hdr, sizeof(hdr));
			if (!memcmp(hdr, "QPHS\1\0\0\0", 8) && VFS_GETLEN(f) == rowbytes*num + 8)
			{
				VFS_READ(f, model->phs, rowbytes*num);
				VFS_CLOSE(f);
				Con_DPrintf("Loaded cached PHS\n");
				return;
			}
			else
				Con_DPrintf("Stale cached PHS\n");
			VFS_CLOSE(f);
		}
	}

	count = 0;
	scan = pvs;
	dest = (unsigned *)model->phs;
	for (i=0 ; i<num ; i++, dest += rowwords, scan += rowbytes)
	{
		memcpy (dest, scan, rowbytes);
		for (j=0 ; j<rowbytes ; j++)
		{
			bitbyte = scan[j];
			if (!bitbyte)
				continue;
			for (k=0 ; k<8 ; k++)
			{
				if (! (bitbyte & (1<<k)) )
					continue;
				// or this pvs row into the phs
				// +1 because pvs is 1 based
				//except we now use clusters internally, which are 0-based (ie: leaf 0 is invalid and maps to cluster -1)
				index = ((j<<3)+k);
				if (index >= num)
					continue;
				src = (unsigned *)pvs + index*rowwords;
				for (l=0 ; l<rowwords ; l++)
					dest[l] |= src[l];
			}
		}

		if (i == 0)
			continue;
		for (j=0 ; j<num ; j++)
			if ( ((qbyte *)dest)[j>>3] & (1<<(j&7)) )
				count++;
	}

	if (rowbytes*num >= 0x100000)
	{
		vfsfile_t *f = FS_OpenVFS(va("maps/%s.phs", svs.name), "wb", FS_GAMEONLY);
		if (f)
		{
			VFS_WRITE(f, "QPHS\1\0\0\0", 8);
			VFS_WRITE(f, model->phs, rowbytes*num);
			VFS_CLOSE(f);
			Con_Printf("Written PHS cache (%u bytes)\n", rowbytes*num);
		}
	}

	if (num)
		if (developer.value)
			Con_TPrintf ("Average leafs visible / hearable / total: %i / %i / %i\n", vcount/num, count/num, num);
}

unsigned SV_CheckModel(char *mdl)
{
	size_t fsize;
	qbyte *buf;
	unsigned short crc;

	buf = (qbyte *)FS_LoadMallocFile (mdl, &fsize);
	if (!buf)
		return 0;
	crc = CalcHashInt(&hash_crc16, buf, fsize);
	BZ_Free(buf);
	return crc;
}

void SV_UnspawnServer (void)	//terminate the running server.
{
	int i;
	if (sv.state)
	{
		Con_TPrintf("Server ended\n");
		SV_FinalMessage("Server unspawned\n");

		PR_PreShutdown();

#ifdef SUBSERVERS
		if (sv.state == ss_clustermode && svs.allocated_client_slots == 1)
			MSV_Shutdown();
#endif

#ifdef MVD_RECORDING
		if (sv.mvdrecording)
			SV_MVDStop (MVD_CLOSE_STOPPED, false);
#endif

		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			if (svs.clients[i].state)
				SV_DropClient(&svs.clients[i]);
		}
		PR_Deinit();
#ifdef Q3SERVER
		if (q3)
			q3->sv.ShutdownGame(false);
#endif
#ifdef Q2SERVER
		SVQ2_ShutdownGameProgs();
		SVQ2_Ents_Shutdown();
#endif
#ifdef HLSERVER
		SVHL_ShutdownGame();
#endif
#ifdef VM_Q1
		Q1QVM_Shutdown(true);
#endif
		sv.world.worldmodel = NULL;
		sv.state = ss_dead;
		if (sv.csqcentversion)
		{
			BZ_Free(sv.csqcentversion);
			sv.csqcentversion = NULL;
		}
	}
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		if (svs.clients[i].frameunion.frames)
			Z_Free(svs.clients[i].frameunion.frames);
		svs.clients[i].frameunion.frames = NULL;
		svs.clients[i].pendingdeltabits = NULL;
		svs.clients[i].pendingcsqcbits = NULL;
		svs.clients[i].state = 0;
		*svs.clients[i].namebuf = '\0';
		svs.clients[i].name = NULL;
		InfoBuf_Clear(&svs.clients[i].userinfo, true);
	}
	free(svs.clients);
	svs.clients = NULL;
	svs.allocated_client_slots = 0;
#ifdef SAVEDGAMES
	SV_FlushLevelCache();
#endif
	NET_CloseServer ();
	SV_RunCmdCleanup();
}

void SV_UpdateMaxPlayers(int newmax)
{
	int i;
	if (newmax != svs.allocated_client_slots)
	{
		client_t *old = svs.clients;
		for (i = newmax; i < svs.allocated_client_slots; i++)
		{
			if (svs.clients[i].state)
				SV_DropClient(&svs.clients[i]);
			svs.clients[i].namebuf[0] = '\0';						//kill all bots
		}
		if (newmax)
			svs.clients = realloc(svs.clients, newmax*sizeof(*svs.clients));
		else
		{
			free(svs.clients);
			svs.clients = NULL;
		}
		for (i = svs.allocated_client_slots; i < newmax; i++)
		{
			memset(&svs.clients[i], 0, sizeof(svs.clients[i]));
			svs.clients[i].name = svs.clients[i].namebuf;
			svs.clients[i].team = svs.clients[i].teambuf;
		}
		for (i = 0; i < min(newmax, svs.allocated_client_slots); i++)
		{
			if (svs.clients[i].name == old[i].namebuf)
				svs.clients[i].name = svs.clients[i].namebuf;
			if (svs.clients[i].team == old[i].teambuf)
				svs.clients[i].team = svs.clients[i].teambuf;
			if (svs.clients[i].netchan.message.data)
				svs.clients[i].netchan.message.data = (qbyte*)&svs.clients[i] + (svs.clients[i].netchan.message.data - (qbyte*)&old[i]);
			if (svs.clients[i].datagram.data)
				svs.clients[i].datagram.data = (qbyte*)&svs.clients[i] + (svs.clients[i].datagram.data - (qbyte*)&old[i]);
			if (svs.clients[i].backbuf.data)
				svs.clients[i].backbuf.data = (qbyte*)&svs.clients[i] + (svs.clients[i].backbuf.data - (qbyte*)&old[i]);
			if (svs.clients[i].controlled)
				svs.clients[i].controlled = svs.clients + (svs.clients[i].controlled - old);
			if (svs.clients[i].controller)
				svs.clients[i].controller = svs.clients + (svs.clients[i].controller - old);
		}
		svs.allocated_client_slots = sv.allocated_client_slots = newmax;

		for (i = 0; i < svs.allocated_client_slots; i++)
		{
			InfoSync_Clear(&svs.clients[i].infosync);
			svs.clients[i].userinfo.ChangeCB = svs.info.ChangeCB;
			svs.clients[i].userinfo.ChangeCTX = &svs.clients[i].userinfo;
		}
	}
	sv.allocated_client_slots = svs.allocated_client_slots;
}

void SV_SetupNetworkBuffers(qboolean bigcoords)
{
	int i;

	//determine basic primitive sizes.
	svs.netprim.flags = 0;
	if (bigcoords)
	{
		if (svs.netprim.coordtype && svs.netprim.coordtype != COORDTYPE_FLOAT_32)
			Con_Printf("Switching to big coords\n");
		svs.netprim.coordtype = COORDTYPE_FLOAT_32;
		svs.netprim.anglesize = 2;
	}
	else
	{
		if (svs.netprim.coordtype && svs.netprim.coordtype != COORDTYPE_FIXED_13_3)
			Con_Printf("Switching to classic coords\n");
		svs.netprim.coordtype = COORDTYPE_FIXED_13_3;
		svs.netprim.anglesize = 1;
	}

	//FIXME: this should be part of sv_new_f or something instead, so that any angles sent by clients won't be invalid
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		svs.clients[i].netchan.netprim = svs.netprim;
		if (svs.clients[i].protocol == SCP_QUAKE2EX)
			svs.clients[i].netchan.netprim.coordtype = COORDTYPE_FLOAT_32; //forced to floats. we have multiple multicast buffers. woo.

		//make sure those are kept up to date too.
		svs.clients[i].datagram.prim =
		svs.clients[i].netchan.message.prim =
		svs.clients[i].backbuf.prim = svs.clients[i].netchan.netprim;
	}

	//
	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.data = sv.datagram_buf;
	sv.datagram.allowoverflow = true;
	sv.datagram.prim = svs.netprim;

	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.data = sv.reliable_datagram_buf;
	sv.reliable_datagram.prim = svs.netprim;

	sv.multicast.maxsize = sizeof(sv.multicast_buf);
	sv.multicast.data = sv.multicast_buf;
	sv.multicast.prim = svs.netprim;

#ifdef NQPROT
	sv.nqdatagram.maxsize = sizeof(sv.nqdatagram_buf);
	sv.nqdatagram.data = sv.nqdatagram_buf;
	sv.nqdatagram.allowoverflow = true;
	sv.nqdatagram.prim = svs.netprim;

	sv.nqreliable_datagram.maxsize = sizeof(sv.nqreliable_datagram_buf);
	sv.nqreliable_datagram.data = sv.nqreliable_datagram_buf;
	sv.nqreliable_datagram.prim = svs.netprim;

	sv.nqmulticast.maxsize = sizeof(sv.nqmulticast_buf);
	sv.nqmulticast.data = sv.nqmulticast_buf;
	sv.nqmulticast.prim = svs.netprim;
#endif

#ifdef Q2SERVER
	sv.q2multicast[0].maxsize = sizeof(sv.q2multicast_lcbuf);
	sv.q2multicast[0].data = sv.q2multicast_lcbuf;
	sv.q2multicast[0].prim = svs.netprim;
	sv.q2multicast[1].maxsize = sizeof(sv.q2multicast_bcbuf);
	sv.q2multicast[1].data = sv.q2multicast_bcbuf;
	sv.q2multicast[1].prim = svs.netprim;
	sv.q2multicast[1].prim.coordtype = COORDTYPE_FLOAT_32;
#endif

	sv.master.maxsize = sizeof(sv.master_buf);
	sv.master.data = sv.master_buf;
	sv.master.prim = msg_nullnetprim;

	sv.signon.data = sv.signon_buffer+2;
	sv.used_signon_space = 0;
	sv.signon.prim = svs.netprim;
	sv.signon.maxsize = sizeof(sv.signon_buffer)-sv.used_signon_space;
}

void SV_WipeServerState(void)
{
	if (sv.stringsalloced)
	{
		unsigned int i;
		char **ptrs = (char**)&sv.strings;
		for (i = 0; i < sizeof(sv.strings) / sizeof(sv.strings.ptrs[0]); i++)
			Z_Free(ptrs[i]);
	}
#ifdef SQL
	SQL_KillServers(&sv);
#endif
	memset (&sv, 0, sizeof(sv));
	sv.logindatabase = -1;
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

This is only called from the SV_Map_f() function.
================
*/
void SV_SpawnServer (const char *server, const char *startspot, qboolean noents, qboolean usecinematic, int playerslots)
{
	extern cvar_t allow_download_refpackages;
	func_t f;
	const char *file, *csprogsname;

	gametype_e newgametype;

	edict_t		*ent;
#ifdef Q2SERVER
	q2edict_t		*q2ent;
#endif
	int			i, j;
	extern int sv_allow_cheats;
	size_t fsz;

#ifndef SERVERONLY
	if (!isDedicated && qrenderer == QR_NONE)
	{
		R_RestartRenderer_f();

		if (qrenderer == QR_NONE)
		{
			Sys_Error("No renderer set when map restarted\n");
			return;
		}
	}
#endif

	Con_DPrintf ("SpawnServer: %s\n",server);

#ifndef SERVERONLY
	total_loading_size = 100;
	current_loading_size = 0;
	SCR_SetLoadingStage(LS_SERVER);
//	SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif

	PR_PreShutdown();

	svs.spawncount++;		// any partially connected client will be restarted
	sv.world.spawncount = svs.spawncount;

	sv.state = ss_dead;

	if (sv.gamedirchanged)
	{
		sv.gamedirchanged = false;
#ifndef SERVERONLY
		Wads_Flush();	//server code is responsable for flushing old state
#endif
#ifdef SVRANKING
		Rank_Flush();
#endif

		for (i = 0; i < svs.allocated_client_slots; i++)
		{
#ifdef SVRANKING
			if (svs.clients[i].state && ISQWCLIENT(&svs.clients[i]))
				ReloadRanking(&svs.clients[i], svs.clients[i].name);
#endif

			if (svs.clients[i].spawninfo)	//don't remember this stuff.
				Z_Free(svs.clients[i].spawninfo);
			svs.clients[i].spawninfo = NULL;
		}
#ifdef HEXEN2
		T_FreeStrings();
#endif
	}

	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		svs.clients[i].nextservertimeupdate = 0;
		if (!svs.clients[i].state)	//bots with the net_preparse module.
			InfoBuf_Clear(&svs.clients[i].userinfo, true);	//clear the userinfo to clear the name

		if (svs.clients[i].netchan.remote_address.type == NA_LOOPBACK)
		{	//forget this client's message buffers, so that any shared client/server network state persists (eg: float coords)
			svs.clients[i].num_backbuf = 0;
			svs.clients[i].datagram.cursize = 0;
		}
		svs.clients[i].csqcactive = false;
	}

	VoteFlushAll();
#ifndef SERVERONLY
	cl.worldmodel = NULL;
	r_worldentity.model = NULL;
//	if (0)
//	cls.state = ca_connected;
	Surf_PreNewMap();
#endif

#ifdef Q3SERVER
	if (svs.gametype == GT_QUAKE3)
		q3->sv.ShutdownGame(false);	//botlib kinda mandates this. :(
#endif

	Mod_ClearAll ();
#ifndef SERVERONLY
	r_regsequence++;
#endif

	PR_Deinit();

	if (sv.csqcentversion)
		BZ_Free(sv.csqcentversion);

	// wipe the entire per-level structure
	SV_WipeServerState();

	SV_SetupNetworkBuffers(sv_bigcoords.ival);

	if (allow_download_refpackages.ival)
		FS_ReferenceControl(1, 1);

	Q_strncpyz (svs.name, server, sizeof(svs.name));
#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
	SCR_SetLoadingFile("map");
#else
	#define SCR_SetLoadingFile(s)
#endif

	Cvar_ApplyLatches(CVAR_MAPLATCH, false);

//work out the gamespeed
//reset the server time.
	sv.time = 0.01;	//some progs don't like time starting at 0.
					//cos of spawn funcs like self.nextthink = time...
					//NQ uses 1, QW uses 0. Awkward.
	sv.starttime = Sys_DoubleTime();

	COM_FlushTempoaryPacks();

	if (sv_cheats.ival)
	{
		sv_allow_cheats = true;
		InfoBuf_SetStarKey(&svs.info, "*cheats", "ON");
	}
	else
	{
		sv_allow_cheats = 2;
		InfoBuf_SetStarKey(&svs.info, "*cheats", "");
	}
#ifndef SERVERONLY
	//This fixes a bug where the server advertises cheats, the internal client connects, and doesn't think cheats are allowed.
	//this applies to anything that can affect the content that is loaded by the server, but cheats is the only special one (because of the *)
	InfoBuf_Clone(&cl.serverinfo, &svs.info);
	if (!isDedicated)
		CL_CheckServerInfo();
#endif

	sv.state = ss_loading;
#if defined(Q2BSPS)
	if (usecinematic)
	{
		qboolean QDECL Mod_LoadQ2BrushModel (model_t *mod, void *buffer, size_t fsize);

		Q_strncpyz (svs.name, server, sizeof(svs.name));
		Q_strncpyz (sv.modelname, "", sizeof(sv.modelname));

		sv.world.worldmodel = Mod_FindName (sv.modelname);
		if (Mod_LoadQ2BrushModel (sv.world.worldmodel, NULL, 0))
			sv.world.worldmodel->loadstate = MLS_LOADED;
		else
			sv.world.worldmodel->loadstate = MLS_FAILED;
	}
	else
#endif
	{
		//.map is commented out because quite frankly, they're a bit annoying when the engine loads the gpled start.map when really you wanted to just play the damn game intead of take it apart.
		//if you want to load a .map, just use 'map foo.map' instead.
		char *exts[] = {"%s", "maps/%s", "maps/%s.bsp", "maps/%s.d3dbsp", "maps/%s.cm", "maps/%s.hmp", "maps/%s.bsp.gz", "maps/%s.bsp.xz", /*"maps/%s.map",*/ NULL}, *e;
		int depth, bestdepth = FDEPTH_MISSING;
		flocation_t loc;
		time_t filetime;
		char *mod = NULL;
		Q_snprintfz (sv.modelname, sizeof(sv.modelname), exts[1], server);	// `map foo.map` can bypass earlier checks, so don't get too screwed up by that.
		if (bestdepth == FDEPTH_MISSING)
		{	//not an exact name, scan the maps subdir.
			for (i = 0; exts[i]; i++)
			{
				depth = COM_FDepthFile(va(exts[i], server), false);
				if (depth < bestdepth)
				{
					bestdepth = depth;
					Q_snprintfz (sv.modelname, sizeof(sv.modelname), exts[i], server);
				}
			}
		}
		if (bestdepth == FDEPTH_MISSING)
		{
			mod = strchr(server, '#');
			if (mod)
			{
				*mod = 0;
				bestdepth = COM_FDepthFile(server, false);
				if (bestdepth != FDEPTH_MISSING)
					Q_snprintfz (sv.modelname, sizeof(sv.modelname), "%s", server);
				else
				{	//not an exact name, scan the maps subdir.
					for (i = 0; exts[i]; i++)
					{
						depth = COM_FDepthFile(va(exts[i], server), false);
						if (depth < bestdepth)
						{
							bestdepth = depth;
							Q_snprintfz (sv.modelname, sizeof(sv.modelname), exts[i], server);
						}
					}
				}
				*mod = '#';
				if (bestdepth == FDEPTH_MISSING)
					mod = NULL;
			}
		}
		if (!strncmp(sv.modelname, "maps/", 5))
			Q_strncpyz (svs.name, sv.modelname+5, sizeof(svs.name));
		else
			Q_strncpyz (svs.name, sv.modelname, sizeof(svs.name));
		e = (char*)COM_GetFileExtension(svs.name, NULL);
		if (!strcmp(e, ".gz") || !strcmp(e, ".xz"))
		{
			*e = 0;
			e = (char*)COM_GetFileExtension(svs.name, NULL);
		}
		if (!strcmp(e, ".bsp"))
			*e = 0;

		Mod_SetModifier(mod);

		sv.world.worldmodel = Mod_ForName (sv.modelname, MLV_ERROR);

		if (FS_FLocateFile(sv.modelname,FSLF_IFFOUND, &loc) && FS_GetLocMTime(&loc, &filetime))
		{
			if (filetime > sv.world.worldmodel->mtime && sv.world.worldmodel->mtime)
			{
				COM_WorkerFullSync();	//sync all the workers, just in case.
				Mod_PurgeModel(sv.world.worldmodel, MP_RESET);	//nuke it now
				sv.world.worldmodel = Mod_ForName (sv.modelname, MLV_ERROR);	//and we can reload it now
			}
			sv.world.worldmodel->mtime = filetime;
		}

		if (!sv.world.worldmodel || sv.world.worldmodel->loadstate != MLS_LOADED)
			Sys_Error("\"%s\" is missing or corrupt\n", sv.modelname);
//	if (sv.world.worldmodel->type != mod_brush && sv.world.worldmodel->type != mod_heightmap)
		if (!sv.world.worldmodel->funcs.NativeTrace && !sv.world.worldmodel->funcs.PointContents)
			Sys_Error("\"%s\" is not a bsp model\n", sv.modelname);
		else if (!Mod_GetEntitiesString(sv.world.worldmodel))
			Sys_Error("\"%s\" has no entity data\n", sv.modelname);
	}

	sv.state = ss_dead;

	//make sure our map's package is loaded.
	if (sv.world.worldmodel)
		FS_LoadMapPackFile(sv.world.worldmodel->name, sv.world.worldmodel->archive);

	//reset the map's areaportal state... it might be dirty from a restart or so.
	if (sv.world.worldmodel->funcs.LoadAreaPortalBlob)
		sv.world.worldmodel->funcs.LoadAreaPortalBlob(sv.world.worldmodel, NULL, 0);

#ifndef SERVERONLY
	current_loading_size+=10;
//	SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
	SCR_SetLoadingFile("phs");
#endif
	SV_CalcPHS ();
#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
	SCR_SetLoadingFile("gamecode");
#endif

	if (sv.world.worldmodel->type != mod_brush)
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "");
	else if (sv.world.worldmodel->fromgame == fg_doom)
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "1");
	else if (sv.world.worldmodel->fromgame == fg_halflife)
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "30");
	else if (sv.world.worldmodel->fromgame == fg_quake2)
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "38");
	else if (sv.world.worldmodel->fromgame == fg_quake3)
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "46");
	else
		InfoBuf_SetStarKey(&svs.info, "*bspversion", "");
	InfoBuf_SetStarKey(&svs.info, "*startspot", (startspot?startspot:""));

	//
	// init physics interaction links
	//
	World_ClearWorld (&sv.world, false);

	//do we allow csprogs?
#ifdef PEXT_CSQC
	fsz = 0;
	if (noents)
		csprogsname = "csaddon.dat";
	else
		csprogsname = sv_csqc_progname.string;
	if (*csprogsname)
		file = COM_LoadTempFile(csprogsname, 0, &fsz);
	else
		file = NULL;
	if (file)
	{
		char text[64];
		sv.csqcchecksum = CalcHashInt(&hash_md4, file, fsz);
		sprintf(text, "0x%x", sv.csqcchecksum);
		InfoBuf_SetValueForStarKey(&svs.info, "*csprogs", text);
		sprintf(text, "0x%x", (unsigned int)fsz);
		InfoBuf_SetValueForStarKey(&svs.info, "*csprogssize", text);
		if (strcmp(csprogsname, "csprogs.dat"))
			InfoBuf_SetValueForStarKey(&svs.info, "*csprogsname", csprogsname);
		else
			InfoBuf_SetValueForStarKey(&svs.info, "*csprogsname", "");
	}
	else
	{
		sv.csqcchecksum = 0;
		InfoBuf_SetValueForStarKey(&svs.info, "*csprogs", "");
		InfoBuf_SetValueForStarKey(&svs.info, "*csprogssize", "");
		InfoBuf_SetValueForStarKey(&svs.info, "*csprogsname", "");
	}
#endif

	if (svs.gametype == GT_PROGS)
	{
		if (svprogfuncs)	//we don't want the q1 stuff anymore.
		{
			svprogfuncs->Shutdown(svprogfuncs);
			sv.world.progs = svprogfuncs = NULL;
		}
	}

	sv.state = ss_loading;

MSV_OpenUserDatabase();

	sv.world.max_edicts = pr_maxedicts.value;
	if (sv.world.max_edicts > MAX_EDICTS)
		sv.world.max_edicts = MAX_EDICTS;
#ifdef PEXT_CSQC
	sv.csqcentversion = BZ_Malloc(sizeof(*sv.csqcentversion) * sv.world.max_edicts);
	for (i=0 ; i<sv.world.max_edicts ; i++)
		sv.csqcentversion[i] = 1;	//force all csqc edicts to start off as version 1
#endif

	newgametype = svs.gametype;
	if (noents)
	{
		newgametype = GT_PROGS;	//let's just hope this loads.
		Q_InitProgs(INITPROGS_EDITOR);
	}
#ifdef HLSERVER
	else if (SVHL_InitGame())
		newgametype = GT_HALFLIFE;
#endif
#ifdef Q3SERVER
	else if (q3 && q3->sv.InitGame(&svs, &sv, false))
		newgametype = GT_QUAKE3;
#endif
#ifdef Q2SERVER
	else if ((sv.world.worldmodel->fromgame == fg_quake2 || sv.world.worldmodel->fromgame == fg_quake3) && sv.world.worldmodel->funcs.AreasConnected && !*pr_ssqc_progs.string && SVQ2_InitGameProgs())	//these are the rules for running a q2 server
		newgametype = GT_QUAKE2;	//we loaded the dll
#endif
#ifdef VM_LUA
	else if (PR_LoadLua())
		newgametype = GT_LUA;
#endif
#ifdef VM_Q1
	else if (PR_LoadQ1QVM())
		newgametype = GT_Q1QVM;
#endif
	else
	{
		newgametype = GT_PROGS;	//let's just hope this loads.
		Q_InitProgs(usecinematic?INITPROGS_REQUIRE:INITPROGS_NORMAL);
	}

//	if ((sv.worldmodel->fromgame == fg_quake2 || sv.worldmodel->fromgame == fg_quake3) && !*progs.string && SVQ2_InitGameProgs())	//full q2 dll decision in one if statement

	if (newgametype != svs.gametype)
	{
#ifdef HLSERVER
		if (newgametype != GT_HALFLIFE)
			SVHL_ShutdownGame();
#endif
#ifdef Q3SERVER
		if (newgametype != GT_QUAKE3 && q3)
			q3->sv.ShutdownGame(false);
#endif
#ifdef Q2SERVER
		if (newgametype != GT_QUAKE2)	//we don't want the q2 stuff anymore.
			SVQ2_ShutdownGameProgs ();
#endif
#ifdef VM_Q1
		if (newgametype != GT_Q1QVM)
			Q1QVM_Shutdown(true);
#endif

		SV_UpdateMaxPlayers(0);
	}
	svs.gametype = newgametype;
	Cvar_ForceCallback(&sv_nqplayerphysics);

	sv.models[1] = sv.world.worldmodel;
#ifdef VM_Q1
	if (svs.gametype == GT_Q1QVM)
	{
		int subs;
		sv.strings.sound_precache[0] = "";
		sv.strings.model_precache[0] = "";

		subs = sv.world.worldmodel->numsubmodels;
		if (subs > MAX_PRECACHE_MODELS-2)
		{
			Con_Printf("Warning: worldmodel has too many submodels\n");
			subs = MAX_PRECACHE_MODELS-2;
		}

		sv.strings.model_precache[1] = sv.modelname;	//the qvm doesn't have access to this array
		for (i=1 ; i<subs ; i++)
		{
			char *z, *s = va("*%u", i);
			z = Z_TagMalloc(strlen(s)+1, VMFSID_Q1QVM);
			strcpy(z, s);
			sv.strings.model_precache[1+i] = z;
			sv.models[i+1] = Mod_ForName (Mod_FixName(z, sv.modelname), MLV_WARN);
		}

		//check player/eyes models for hacks
		sv.model_player_checksum = SV_CheckModel("progs/player.mdl");
		sv.eyes_player_checksum = SV_CheckModel("progs/eyes.mdl");
	}
	else
#endif
	if (svs.gametype == GT_PROGS
#ifdef VM_LUA
		|| svs.gametype == GT_LUA
#endif
		)
	{
		int subs;
		sv.strings.model_precache[0] = PR_AddString(svprogfuncs, "", 0, false);
		sv.strings.model_precache[1] = PR_AddString(svprogfuncs, sv.modelname, 0, false);

		subs = sv.world.worldmodel->numsubmodels;
		if (subs > MAX_PRECACHE_MODELS-2)
		{
			Con_Printf("Warning: worldmodel has too many submodels\n");
			subs = MAX_PRECACHE_MODELS-2;
		}
		for (i=1 ; i<subs ; i++)
		{
			sv.strings.model_precache[1+i] = PR_AddString(svprogfuncs, va("*%u", i), 0, false);
			sv.models[i+1] = Mod_ForName (Mod_FixName(sv.strings.model_precache[1+i], sv.modelname), MLV_WARN);
		}

		//check player/eyes models for hacks
		sv.model_player_checksum = SV_CheckModel("progs/player.mdl");
		sv.eyes_player_checksum = SV_CheckModel("progs/eyes.mdl");
	}
#ifdef Q2SERVER
	else if (svs.gametype == GT_QUAKE2)
	{
		int subs;
		extern cvar_t sv_airaccelerate;

		sv.stringsalloced = true;
		memset(&sv.strings, 0, sizeof(sv.strings));

		if (deathmatch.value)
			sv.strings.configstring[Q2CS_AIRACCEL] = Z_StrDupf("%g", sv_airaccelerate.value);
		else
			sv.strings.configstring[Q2CS_AIRACCEL] = Z_StrDup("0");

		// init map checksum config string but only for Q2/Q3 maps
		sv.strings.configstring[Q2CS_MAPCHECKSUM] = Z_StrDupf("%i %i", sv.world.worldmodel->checksum, sv.world.worldmodel->checksum2);

		subs = sv.world.worldmodel->numsubmodels;
		if (subs > MAX_PRECACHE_MODELS-1)
		{
			Con_Printf("Warning: worldmodel has too many submodels\n");
			subs = MAX_PRECACHE_MODELS-1;
		}

		sv.strings.configstring[Q2CS_MODELS+1] = Z_StrDup(sv.modelname);
		for (i=1; i<subs && i < Q2MAX_MODELS-2; i++)
		{
			sv.strings.configstring[Q2CS_MODELS+1+i] = Z_StrDupf("*%u", i);
			sv.models[i+1] = Mod_ForName (Mod_FixName(sv.strings.configstring[Q2CS_MODELS+1+i], sv.modelname), MLV_WARN);
		}
		for ( ; i<subs; i++)
		{
			sv.strings.q2_extramodels[1+i] = Z_StrDupf("*%u", i);
			sv.models[i+1] = Mod_ForName (Mod_FixName(sv.strings.q2_extramodels[1+i], sv.modelname), MLV_WARN);
		}
	}
#endif



#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
	SCR_SetLoadingFile("clients");
#endif

	for (i=0 ; i<svs.allocated_client_slots ; i++)
	{
		svs.clients[i].spawned = false;
		svs.clients[i].edict = NULL;
		svs.clients[i].name = svs.clients[i].namebuf;
		svs.clients[i].team = svs.clients[i].teambuf;
		InfoSync_Clear(&svs.clients[i].infosync);	//we'll mark all the info as dirty at some point while connecting.
	}

	switch (svs.gametype)
	{
	default:
		SV_Error("bad gametype");
		break;
#ifdef VM_LUA
	case GT_LUA:
#endif
	case GT_Q1QVM:
	case GT_PROGS:
		ent = EDICT_NUM_PB(svprogfuncs, 0);
		ent->ereftype = ER_ENTITY;

#ifndef SERVERONLY
		/*force coop 1 if splitscreen and not deathmatch*/
		{
		if (cl_splitscreen.value && !deathmatch.value && !coop.value)
			Cvar_Set(&coop, "1");
		}
#endif
		if (sv_playerslots.ival > 0)
			i = sv_playerslots.ival;
		else
		{
			/*only make one slot for single-player (ktx sucks)*/
			if (!isDedicated && !deathmatch.value && !coop.value && svs.gametype != GT_Q1QVM)
				i = 1;
			else
			{
				i = maxclients.ival + maxspectators.ival;
				if (i < QWMAX_CLIENTS)
					i = QWMAX_CLIENTS;
			}
		}
		if (playerslots)
			i = playerslots;	//saved game? force it.
		if (i > MAX_CLIENTS)
			i = MAX_CLIENTS;
		SV_UpdateMaxPlayers(i);

		// leave slots at start for clients only
		for (i=0 ; i<sv.allocated_client_slots ; i++)
		{
			svs.clients[i].viewent = 0;

			ent = ED_Alloc(svprogfuncs, false, 0);//EDICT_NUM(i+1);
			svs.clients[i].edict = ent;
			ent->ereftype = ER_ENTITY;
	//ZOID - make sure we update frags right
			svs.clients[i].old_frags = 0;

			if (!svs.clients[i].state && svs.clients[i].name[0])	//this is a bot.
				svs.clients[i].name[0] = '\0';						//make it go away

#ifdef VM_Q1
			if (svs.gametype == GT_Q1QVM)
			{	//we'll fix it up later anyway
				svs.clients[i].name = svs.clients[i].namebuf;
				svs.clients[i].team = svs.clients[i].teambuf;
			}
			else
#endif
			{
				svs.clients[i].name = PR_AddString(svprogfuncs, svs.clients[i].namebuf, sizeof(svs.clients[i].namebuf), false);
				svs.clients[i].team = PR_AddString(svprogfuncs, svs.clients[i].teambuf, sizeof(svs.clients[i].teambuf), false);
			}
		}
		break;
#ifdef Q2SERVER
	case GT_QUAKE2:
		SV_UpdateMaxPlayers(svq2_maxclients);
		for (i=0 ; i<sv.allocated_client_slots ; i++)
		{
			q2ent = Q2EDICT_NUM(i+1);
			q2ent->s.number = i+1;
			svs.clients[i].q2edict = q2ent;
		}
		break;
#endif
#ifdef Q3SERVER
	case GT_QUAKE3:
		Cvar_LockFromServer(&maxclients, maxclients.string);
		SV_UpdateMaxPlayers(playerslots?playerslots:max(8,maxclients.ival));
		break;
#endif
#ifdef HLSERVER
	case GT_HALFLIFE:
		SVHL_SetupGame();
		SV_UpdateMaxPlayers(32);
		break;
#endif
	}
	//fixme: is this right?

	for (i=0 ; i<sv.allocated_client_slots ; i++)
	{
		Q_strncpyz(svs.clients[i].name, InfoBuf_ValueForKey(&svs.clients[i].userinfo, "name"), sizeof(svs.clients[i].namebuf));
		Q_strncpyz(svs.clients[i].team, InfoBuf_ValueForKey(&svs.clients[i].userinfo, "team"), sizeof(svs.clients[i].teambuf));
	}

#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif

	NET_InitServer();

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;

	if (svprogfuncs)
	{
		//world entity is hackily spawned
		extern cvar_t coop, pr_imitatemvdsv;
		ent = EDICT_NUM_PB(svprogfuncs, 0);
		ent->ereftype = ER_ENTITY;
#ifdef VM_Q1
		if (svs.gametype != GT_Q1QVM)	//we cannot do this with qvm
#endif
			svprogfuncs->SetStringField(svprogfuncs, ent, &ent->v->model, sv.strings.model_precache[1], true);
		ent->v->modelindex = 1;		// world model
		ent->v->solid = SOLID_BSP;
		ent->v->movetype = MOVETYPE_PUSH;
		VectorCopy(sv.world.worldmodel->mins, ent->v->mins);
		VectorCopy(sv.world.worldmodel->maxs, ent->v->maxs);
		VectorCopy(sv.world.worldmodel->mins, ent->v->absmin);
		VectorCopy(sv.world.worldmodel->maxs, ent->v->absmax);

		if (progstype == PROG_QW && pr_imitatemvdsv.value>0)
		{
#ifdef VM_Q1
			if (svs.gametype != GT_Q1QVM)	//we cannot do this with qvm
#endif
			{
				svprogfuncs->SetStringField(svprogfuncs, ent, &ent->v->targetname, "mvdsv", true);
				svprogfuncs->SetStringField(svprogfuncs, ent, &ent->v->netname, version_string(), false);
			}
			ent->v->impulse = 0;//QWE_VERNUM;
			ent->v->items = 103;
		}


#ifdef VM_Q1
		if (svs.gametype != GT_Q1QVM)	//we cannot do this with qvm
#endif
			svprogfuncs->SetStringField(svprogfuncs, NULL, &pr_global_struct->mapname, svs.name, true);

		// serverflags are for cross level information (sigils)
		pr_global_struct->serverflags = svs.serverflags;
		pr_global_struct->time = 1.0;	//match nq behaviour

#ifdef HEXEN2
		if (progstype == PROG_H2)
		{
			eval_t *eval;
			cvar_t *cv;
			if (coop.value)
			{
				eval = PR_FindGlobal(svprogfuncs, "coop", 0, NULL);
				if (eval) eval->_float = coop.value;
			}
			else
			{
				eval = PR_FindGlobal(svprogfuncs, "deathmatch", 0, NULL);
				if (eval) eval->_float = deathmatch.value;
			}
			cv = Cvar_Get("randomclass", "0", CVAR_MAPLATCH, "Hexen2");
			eval = PR_FindGlobal(svprogfuncs, "randomclass", 0, NULL);
			if (eval && cv) eval->_float = cv->value;

			cv = Cvar_Get("cl_playerclass", "1", CVAR_USERINFO|CVAR_ARCHIVE, "Hexen2");
			eval = PR_FindGlobal(svprogfuncs, "cl_playerclass", 0, NULL);
			if (eval && cv) eval->_float = cv->value;
		}
		else
#endif
		{
			if (pr_global_ptrs->coop && coop.value)
				pr_global_struct->coop = coop.value;
			else if (pr_global_ptrs->deathmatch)
				pr_global_struct->deathmatch = deathmatch.value;
		}

		if (svs.gametype != GT_Q1QVM) //we cannot do this with qvm
		{
			for (i = 0; i < svs.numprogs; i++)	//do this AFTER precaches have been played with...
			{
				f = PR_FindFunction (svprogfuncs, "initents", svs.progsnum[i]);
				if (f)
				{
					PR_ExecuteProgram(svprogfuncs, f);
				}
			}
		}
		if (progstype == PROG_QW)
			// run the frame start qc function to let progs check cvars
			SV_ProgStartFrame ();	//prydon gate seems to fail because of this allowance
	}

	// load and spawn all other entities
	SCR_SetLoadingFile("entities");
	if (!deathmatch.value && !*skill.string)	//skill was left blank so it doesn't polute serverinfo on deathmatch servers. in single player, we ensure that it gets a proper value.
		Cvar_Set(&skill, "1");
//do this and get the precaches/start up the game
	if (sv.world.worldmodel->entitiescrc)
	{
		char crc[12];
		sprintf(crc, "%i", sv.world.worldmodel->entitiescrc);
		InfoBuf_SetValueForStarKey(&svs.info, "*entfile", crc);
	}
	else
		InfoBuf_SetValueForStarKey(&svs.info, "*entfile", "");

	if (usecinematic)
		file = NULL;
	else
		file = Mod_GetEntitiesString(sv.world.worldmodel);
	if (!file)
		file = "";

	switch(svs.gametype)
	{
	default:
		PR_SpawnInitialEntities(file);
		break;
#ifdef Q2SERVER
	case GT_QUAKE2:
		ge->SpawnEntities(svs.name, file, startspot?startspot:"");
		break;
#endif
	case GT_QUAKE3:
		break;
#ifdef HLSERVER
	case GT_HALFLIFE:
		SVHL_SpawnEntities(file);
		break;
#endif
	}

#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif

	Q_strncpyz(sv.mapname, svs.name, sizeof(sv.mapname));
	if (svprogfuncs)
	{
		eval_t *val;
		ent = EDICT_NUM_PB(svprogfuncs, 0);
		ent->v->angles[0] = ent->v->angles[1] = ent->v->angles[2] = 0;
		if ((val = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, "message", ev_string, NULL)))
			snprintf(sv.mapname, sizeof(sv.mapname), "%s", PR_GetString(svprogfuncs, val->string));
#ifdef HEXEN2
		else if (progstype == PROG_H2 && (val = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, "message", ev_float, NULL)))
		{	//hexen2 uses a float string index for message.
			//if its 0 or negative, fall back on netname instead (for custom maps).
			if (val->_float <= 0)
				snprintf(sv.mapname, sizeof(sv.mapname), "%s", PR_GetString(svprogfuncs, ent->v->netname));
			else
				snprintf(sv.mapname, sizeof(sv.mapname), "%s", T_GetString(val->_float-1));
		}
#endif
		else
			snprintf(sv.mapname, sizeof(sv.mapname), "%s", svs.name);
		if (Cvar_Get("sv_readonlyworld", "1", 0, "DP compatability")->value)
		{
			ent->readonly = true;	//lock it down!

			if (ent->v->origin[0] != 0 || ent->v->origin[1] != 0 || ent->v->origin[2] != 0 || ent->v->angles[0] != 0 || ent->v->angles[1] != 0 || ent->v->angles[2] != 0)
				Con_Printf("Warning: The world has moved. Alert your nearest reputable news agency.\n");

		}

		// look up some model indexes for specialized message compression
		SV_FindModelNumbers ();
	}

#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif
	// run two frames to allow everything to settle
	//these frames must be at 1.0 then 1.1 (and 0.1 frametime)
	//(bug: starting less than that gives time for the scrag to fall on end)
	//hexen2: if you're looking here for the coop-invincible-riders bug, then that's a hexenc bug, not an fte one, and is also present in vanilla hexen2.
	realtime += 0.1;
	sv.world.physicstime = 1.0;
	sv.time = 1.1;
	SV_Physics ();
#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif
	realtime += 0.1;
//	sv.world.physicstime = 1.1;
	sv.time += 0.1;
	SV_Physics ();
	sv.time += 0.1;

#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif

	// save movement vars
	SV_SetMoveVars();

	// create a baseline for more efficient communications
//	SV_CreateBaseline ();
	if (svprogfuncs)
		SVQ1_CreateBaseline();
#ifdef Q2SERVER
	SVQ2_BuildBaselines();
#endif

	SV_FlushSignon(true);

	// all spawning is completed, any further precache statements
	// or prog writes to the signon message are errors
	if (usecinematic)
		sv.state = ss_cinematic;
	else
		sv.state = ss_active;

	SV_GibFilterInit();
	SV_FilterImpulseInit();

	InfoBuf_SetValueForKey (&svs.info, "map", svs.name);
	if (sv.allocated_client_slots != 1)
		Con_TPrintf ("Server spawned.\n");	//misc filenotfounds can be misleading.

	if (!startspot)
	{
#ifdef SAVEDGAMES
		SV_FlushLevelCache();	//to make sure it's caught
#endif
		/*for (i=0 ; i<sv.allocated_client_slots ; i++)
		{
			if (svs.clients[i].spawninfo)
				Z_Free(svs.clients[i].spawninfo);
			svs.clients[i].spawninfo = NULL;
		}*/
	}

	if (svprogfuncs && startspot)
	{
		eval_t *eval;
		eval = PR_FindGlobal(svprogfuncs, "startspot", 0, NULL);
		if (eval && svs.gametype != GT_Q1QVM)	//we cannot do this with qvm
			svprogfuncs->SetStringField(svprogfuncs, NULL, &eval->string, startspot, false);
	}

	if (Cmd_AliasExist("f_svnewmap", RESTRICT_LOCAL))
		Cbuf_AddText("f_svnewmap\n", RESTRICT_LOCAL);

#ifndef SERVERONLY
	current_loading_size+=10;
	//SCR_BeginLoadingPlaque();
	SCR_ImageName(server);
#endif

	/*world is now spawned. switch to big coords if there are entities outside the bounds of the map*/
	if (!*sv_bigcoords.string && svprogfuncs)
	{
		float extent = 0, ne;
		//fixme: go off bsp extents instead?
		for(i = 1; i < sv.world.num_edicts; i++)
		{
			ent = EDICT_NUM_PB(svprogfuncs, i);
			for (j = 0; j < 3; j++)
			{
				ne = fabs(ent->v->origin[j]);
				if (extent < ne)
					extent = ne;
			}
		}
		if (extent > (1u<<15)/8 
#ifdef TERRAIN
			|| sv.world.worldmodel->terrain
#endif
			)
		{
			if (sv.used_signon_space || sv.signon.cursize)
				Con_Printf("Cannot auto-enable extended coords as the init buffer was used\n");
			else
			{
				Con_Printf("Switching to extended coord sizes\n");
				SV_SetupNetworkBuffers(true);
			}
		}
	}

	/*DP_BOTCLIENT bots should move over to the new map too*/
	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
	{
		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			host_client = &svs.clients[i];
			if (host_client->state == cs_connected && host_client->protocol == SCP_BAD)
			{
				if (svs.gametype == GT_Q1QVM)
				{	//ktx expects its bots to drop for each map change.
					SV_DropClient(host_client);
					continue;
				}
				sv_player = host_client->edict;
				SV_ExtractFromUserinfo(host_client, true);
				SV_SpawnParmsToQC(host_client);
				SV_SetUpClientEdict(host_client, sv_player);
#ifdef HAVE_LEGACY
				sv_player->xv->clientcolors = host_client->playercolor;
#endif

				// call the spawn function
				sv.skipbprintclient = host_client;
				pr_global_struct->time = sv.world.physicstime;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, sv_player);
				if (pr_global_ptrs->ClientConnect)
					PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientConnect);
				sv.skipbprintclient = NULL;

				// actually spawn the player
				pr_global_struct->time = sv.world.physicstime;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, sv_player);
				if (pr_global_ptrs->PutClientInServer)
					PR_ExecuteProgram (svprogfuncs, pr_global_struct->PutClientInServer);
				sv.spawned_client_slots++;

				// send notification to all clients
				host_client->sendinfo = true;

				host_client->state = cs_spawned;
				host_client->spawned = true;

				SV_UpdateToReliableMessages();	//so that we don't flood too much with 31 bots and one player.
			}
		}
	}
#ifdef Q3SERVER
	if (svs.gametype == GT_QUAKE3)
	{
		q3->sv.NewMapConnects();
	}
#endif

	FS_ReferenceControl(0, 0);

#ifdef MVD_RECORDING
	SV_MVD_SendInitialGamestate(NULL);
#endif

	SSV_UpdateAddresses();

	//some mods stuffcmd these, and it would be a shame if they didn't work. we still need the earlier call in case the mod does extra stuff.
	SV_SetMoveVars();

	sv.starttime = Sys_DoubleTime() - sv.time;
#ifdef SAVEDGAMES
	sv.autosave_time = sv.time + sv_autosave.value*60;
#endif

#ifdef HAVE_CLIENT
	//there's a whole load of ugly debug crap there. make sure it stays hidden.
	Con_ClearNotify();
#endif

#ifdef __GLIBC__
	if (isDedicated)
		malloc_trim(0);
#endif
}

#endif

