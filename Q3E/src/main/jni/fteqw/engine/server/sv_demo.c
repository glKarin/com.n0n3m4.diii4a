#include "quakedef.h"

#ifndef CLIENTONLY
#ifdef SERVER_DEMO_PLAYBACK

void NPP_MVDWriteByte(qbyte data, client_t *to, int broadcast);

void SV_New_f (void);

qboolean SV_GetPacket (void)
{
	return NET_GetPacket (NS_SERVER);
}








#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define	dem_single		4
#define dem_stats		5
#define dem_all			6




#define svd sv

//char empty[512];
qboolean SV_ReadMVD (void);

#ifdef SERVERONLY
float nextdemotime = 0;
float olddemotime = 0;
#else
extern float nextdemotime;
extern float olddemotime;
#endif
void SV_LoadClientDemo_f (void)
{
	int i;
	char demoname[MAX_OSPATH];
	client_t *ohc;
	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s <demoname>: play a server side multi-view demo\n", Cmd_Argv(0));
		return;
	}

	if (svd.demofile)
	{
		Con_Printf ("Ending old demo\n");
		VFS_CLOSE(svd.demofile);
		svd.demofile = NULL;

		SV_ReadMVD();
	}

	Q_strncpyz(demoname, Cmd_Argv(1), sizeof(demoname));

	if (!sv.state)
		Cmd_ExecuteString("map start\n", Cmd_ExecLevel);	//go for the start map
	if (!sv.state)
	{
		Con_Printf("Could not activate server\n");
		return;
	}

	svd.demofile = FS_OpenVFS(demoname, "rb", FS_GAME);
	if (!svd.demofile)	//try with a different path
		svd.demofile = FS_OpenVFS(va("demos/%s", demoname), "rb", FS_GAME);
	com_filesize = VFS_GETLEN(svd.demofile);

	if (!svd.demofile)
	{
		Con_Printf("Failed to open %s\n", demoname);
		return;
	}
	if (com_filesize <= 0)
	{
		Con_Printf("Failed to open %s\n", demoname);
		VFS_CLOSE(svd.demofile);
		svd.demofile = NULL;
		return;
	}

	if (sv.demostate)
	{
		sv.demostatevalid = false;
		memset(sv.demostate, 0, sizeof(entity_state_t)*MAX_EDICTS);
	}
/*
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		host_client = &svs.clients[i];
		if (host_client->state == cs_spawned)
			host_client->state = cs_connected;
	}
*/
//	SV_BroadcastCommand ("changing\n");

#ifndef SERVERONLY
	CL_Disconnect();
#endif

	svd.mvdplayback = true;
	Con_Printf("Playing from %s\n", demoname);

	for (i = 0; i < MAX_SIGNON_BUFFERS; i++)
		sv.demosignon_buffer_size[i] = 0;
	sv.demosignon.maxsize = sizeof(sv.demosignon_buffers[0]);
	sv.demosignon.data = sv.demosignon_buffers[0];
	sv.demosignon.cursize = 0;
	sv.num_demosignon_buffers = 1;
	sv.democausesreconnect = false;
	*sv.demname = '\0';

	svd.lasttype = dem_read;
	svd.realtime = realtime;
	nextdemotime = realtime-0.1; //cause read of the first 0.1 secs to get all spawn info.
	olddemotime = realtime;
	while (SV_ReadMVD())
	{
		sv.datagram.cursize = 0;
		sv.reliable_datagram.cursize = 0;
	}

	//if we did need reconnect, continue needing it cos I can't be bothered to play with multiple buffers etc.
//	if (memcmp(sv.demmodel_precache, sv.model_precache, sizeof(sv.model_precache)) || memcmp(sv.demsound_precache, sv.sound_precache, sizeof(sv.sound_precache)))
		sv.democausesreconnect = true;
	if (sv.democausesreconnect)
	{
		svs.spawncount++;
		SV_BroadcastCommand ("changing\n");	//but this arrives BEFORE the serverdata

		ohc = host_client;
		for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
		{
			if (host_client->state != cs_spawned)
				continue;
			host_client->state = cs_connected;
			host_client->istobeloaded = true;	//don't harm the ent
			SV_New_f ();
		}
		host_client = ohc;
	}
	return;
}

qboolean SV_RunDemo (void)
{
	int		r, i;
	float	demotime;
	qbyte	c;
//	usercmd_t *pcmd;
//	usercmd_t emptycmd;
	qbyte	newtime;



readnext:

	// read the time from the packet
	if (svd.mvdplayback)
	{
		VFS_READ(svd.demofile, &newtime, sizeof(newtime));
		nextdemotime = olddemotime + newtime * (1/1000.0f);
		demotime = nextdemotime;

		if (nextdemotime > svd.realtime)
		{
			VFS_SEEK(svd.demofile, VFS_TELL(svd.demofile) - sizeof(newtime));
			return false;
		}
		else if (nextdemotime + 0.1 < svd.realtime)
			demotime = svd.realtime;	//we froze too long.. ?
	}
	else
	{
		VFS_READ(svd.demofile, &demotime, sizeof(demotime));
		demotime = LittleFloat(demotime);
		if (!nextdemotime)
			svd.realtime = nextdemotime = demotime;
	}

// decide if it is time to grab the next message
	if (!sv.paused)
	{	// always grab until fully connected
		if (!svd.mvdplayback)
		{
			if (svd.realtime + 1.0 < demotime) {
				// too far back
				svd.realtime = demotime - 1.0;
				// rewind back to time
				VFS_SEEK(svd.demofile, VFS_TELL(svd.demofile) - sizeof(demotime));
				return false;
			} else if (nextdemotime < demotime) {
				// rewind back to time
				VFS_SEEK(svd.demofile, VFS_TELL(svd.demofile) - sizeof(demotime));
				return false;		// don't need another message yet
			}
		}
	} else {
		svd.realtime = demotime; // we're warping
	}

	olddemotime = demotime;

	// get the msg type
	if ((r = VFS_READ(svd.demofile, &c, sizeof(c))) != sizeof(c))
	{
		Con_Printf ("Unexpected end of demo\n");
		VFS_CLOSE(svd.demofile);
		svd.demofile = NULL;
		return false;
//		SV_Error ("Unexpected end of demo");
	}

	switch (c & 7) {
	case dem_cmd :

		Con_Printf ("dem_cmd not supported\n");
		VFS_CLOSE(svd.demofile);
		svd.demofile = NULL;
		return false;


		// user sent input
//		i = svd.netchan.outgoing_sequence & UPDATE_MASK;
//		pcmd = &cl.frames[i].cmd;
	//	if ((r = fread (&emptycmd, sizeof(emptycmd), 1, svd.demofile)) != 1)
	//		SV_Error ("Corrupted demo");
/*
		// qbyte order stuff
		for (j = 0; j < 3; j++)
			pcmd->angles[j] = LittleFloat(pcmd->angles[j]);

		pcmd->forwardmove = LittleShort(pcmd->forwardmove);
		pcmd->sidemove    = LittleShort(pcmd->sidemove);
		pcmd->upmove      = LittleShort(pcmd->upmove);
		cl.frames[i].senttime = demotime;
		cl.frames[i].receivedtime = -1;		// we haven't gotten a reply yet
		svd.netchan.outgoing_sequence++;
*/
	//	fread (&emptycmd, 12, 1, svd.demofile);
/*		for (j = 0; j < 3; j++)
			 cl.viewangles[i] = LittleFloat (cl.viewangles[i]);
		if (cl.spectator)
			Cam_TryLock ();
*/
		goto readnext;

	case dem_read:
readit:
		// get the next message
		VFS_READ (svd.demofile, &net_message.cursize, 4);
		net_message.cursize = LittleLong (net_message.cursize);

		if (!svd.mvdplayback && net_message.cursize > MAX_QWMSGLEN + 8)
			SV_Error ("Demo message > MAX_MSGLEN + 8");
		else if (svd.mvdplayback && net_message.cursize > net_message.maxsize)
			SV_Error ("Demo message > MAX_UDP_PACKET");

		if ((r = VFS_READ(svd.demofile, net_message.data, net_message.cursize)) != net_message.cursize)
			SV_Error ("Corrupted demo");

/*		if (svd.mvdplayback) {
			tracknum = Cam_TrackNum();

			if (svd.lasttype == dem_multiple) {
				if (tracknum == -1)
					goto readnext;

				if (!(svd.lastto & (1 << (tracknum))))
					goto readnext;
			} else if (svd.lasttype == dem_single) {
				if (tracknum == -1 || svd.lastto != spec_track)
					goto readnext;
			}
		}
*/
		return true;

	case dem_set:
		VFS_READ(svd.demofile, &i, 4);
		VFS_READ(svd.demofile, &i, 4);
		goto readnext;

	case dem_multiple:
		if ((r = VFS_READ(svd.demofile, &i, 4)) != 1)
			SV_Error ("Corrupted demo");

		svd.lastto = LittleLong(i);
		svd.lasttype = dem_multiple;
		goto readit;

	case dem_single:
		svd.lastto = c >> 3;
		svd.lasttype = dem_single;
		goto readit;
	case dem_stats:
		svd.lastto = c >> 3;
		svd.lasttype = dem_stats;
		goto readit;
	case dem_all:
		svd.lastto = 0;
		svd.lasttype = dem_all;
		goto readit;
	default :
		SV_Error ("Corrupted demo");
		return false;
	}
}

qboolean SV_ReadMVD (void)
{
	int i, c;
	client_t *cl;

	int oldsc = svs.spawncount;

	if (!svd.demofile)
	{
		if (sv.demostate)
			BZ_Free(sv.demostate);
		sv.demostate=NULL;
		sv.demostatevalid = false;
		if (sv.democausesreconnect)
		{
			sv.democausesreconnect = false;
			svs.spawncount++;

			for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
			{
				if (host_client->state != cs_spawned)
					continue;
				host_client->state = cs_connected;
				host_client->istobeloaded = true;	//don't harm the ent
				SV_New_f ();
			}
		}
		nextdemotime = realtime;
		return false;
	}

	svd.realtime = realtime;

	if (!SV_RunDemo())
	{
		if (!svd.demofile)
		{	//demo ended.
			for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
			{
				cl->sendinfo = true;
			}
		}
		return false;
	}

	if (!svd.mvdplayback)	//broadcast all.
	{
		for (c = 0; c < net_message.cursize; c++)
			NPP_MVDWriteByte(net_message.data[c], NULL, true);

		NPP_MVDForceFlush();
	}
	else
	{
		switch(svd.lasttype)
		{
		default:
			Con_Printf("Bad sv.lasttype %i\n", sv.lasttype);
			break;


		case dem_set:	//Unknown stuff. (Got to work out what this is for)
		case dem_read:	//baseline stuff
		case dem_stats:	//contains info read by server
		case dem_all:	//broadcast things (like userinfo)
		case dem_multiple:	//treat these as broadcast (tempents should be treated correctly)
			for (c = 0; c < net_message.cursize; c++)
				NPP_MVDWriteByte(net_message.data[c], NULL, true);

			NPP_MVDForceFlush();
			break;
//				case dem_read:	//baseline stuff
		case dem_single:
			for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
			{
				if (!cl->state)
					continue;
	//			if (!(1 >> 3 & svd.lastto))
				if (!cl->spec_track)
					continue;
				if (!(cl->spec_track >> 3 & svd.lastto))
					continue;

				for (c = 0; c < net_message.cursize; c++)
					NPP_MVDWriteByte(net_message.data[c], cl, false);

				NPP_MVDForceFlush();
			}
			break;
		}
	}

	if (oldsc != svs.spawncount)
	{
		VFS_CLOSE(svd.demofile);
		svd.demofile = NULL;

		for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
		{
			if (host_client->state != cs_spawned)
				continue;
			host_client->state = cs_connected;
			host_client->istobeloaded = true;	//don't harm the ent
			SV_New_f ();
		}
		return true;
	}

	return true;
}




void SV_Demo_Init(void)
{
	Cmd_AddCommand("playmvd", SV_LoadClientDemo_f);
	Cmd_AddCommand("mvdplay", SV_LoadClientDemo_f);
	Cmd_AddCommand("svplay", SV_PlayDemo_f);
	Cmd_AddCommand("svrecord", SV_RecordDemo_f);
}

#endif //SERVER_DEMO_PLAYBACK
#endif //CLIENTONLY
