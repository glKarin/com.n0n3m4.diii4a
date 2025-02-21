#include "q3common.h"
#include "shader.h"



#include "glquake.h"
#include "shader.h"
#include "cl_master.h"

//urm, yeah, this is more than just parse.

#ifdef Q3CLIENT

#include "clq3defs.h"

#define SHOWSTRING(s) if(cl_shownet_ptr->value==2)Con_Printf ("%s\n", s);
#define SHOWNET(x) if(cl_shownet_ptr->value==2)Con_Printf ("%3i:%s\n", msg->currentbit-8, x);
#define SHOWNET2(x, y) if(cl_shownet_ptr->value==2)Con_Printf ("%3i:%3i:%s\n", msg->currentbit-8, y, x);

void MSG_WriteBits(sizebuf_t *msg, int value, int bits);
static qboolean CLQ3_Netchan_Process(sizebuf_t *msg);


ClientConnectionState_t ccs;



qboolean CG_FillQ3Snapshot(int snapnum, snapshot_t *snapshot)
{
	int i;
	clientSnap_t	*snap;

	if (snapnum > ccs.serverMessageNum)
	{
		plugfuncs->EndGame("CG_FillQ3Snapshot: snapshotNumber > cl.snap.serverMessageNum");
	}

	if (ccs.serverMessageNum - snapnum >= Q3UPDATE_BACKUP)
	{
		return false; // too old
	}

	snap = &ccs.snapshots[snapnum & Q3UPDATE_MASK];
	if(!snap->valid || snap->serverMessageNum != snapnum)
	{
		return false; // invalid
	}

	memcpy(&snapshot->ps, &snap->playerstate, sizeof(snapshot->ps));
	snapshot->numEntities = snap->numEntities;
	for (i=0; i<snapshot->numEntities; i++)
	{
		memcpy(&snapshot->entities[i], &ccs.parseEntities[(snap->firstEntity+i) & Q3PARSE_ENTITIES_MASK], sizeof(snapshot->entities[0]));
	}

	memcpy( &snapshot->areamask, snap->areabits, sizeof( snapshot->areamask ) );

	snapshot->snapFlags = snap->snapFlags;
	snapshot->ping = snap->ping;

	snapshot->serverTime = snap->serverTime;

	snapshot->numServerCommands = snap->serverCommandNum;
	snapshot->serverCommandSequence = ccs.lastServerCommandNum;

	return true;
}

/*
=====================
CLQ3_ParseServerCommand
=====================
*/
void CLQ3_ParseServerCommand(void)
 {
	int		number;
	char	*string;

	number = msgfuncs->ReadLong();
//	SHOWNET(va("%i", number));

	string = msgfuncs->ReadString();
	SHOWSTRING(string);

	if( number <= ccs.lastServerCommandNum )
	{
		return; // we have already received this command
	}

	ccs.lastServerCommandNum++;

	if( number > ccs.lastServerCommandNum+Q3TEXTCMD_MASK-1 )
	{
		Con_Printf("Warning: Lost %i reliable serverCommands\n",
			number - ccs.lastServerCommandNum );
	}

	// archive the command to be processed by cgame later
	Q_strncpyz( ccs.serverCommands[number & Q3TEXTCMD_MASK], string, sizeof( ccs.serverCommands[0] ) );
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
static void CLQ3_DeltaEntity( clientSnap_t *frame, int newnum, q3entityState_t *old, qboolean unchanged )
{
	q3entityState_t *state;

	state = &ccs.parseEntities[ccs.firstParseEntity & Q3PARSE_ENTITIES_MASK];

	if( unchanged )
	{
		memcpy( state, old, sizeof(*state) ); // don't read any bits
	}
	else
	{
		if (!MSG_Q3_ReadDeltaEntity(old, state, newnum)) // the entity present in oldframe is not in the current frame
			return;
	}

	ccs.firstParseEntity++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
static void CLQ3_ParsePacketEntities( clientSnap_t *oldframe, clientSnap_t *newframe )
{
	int				numentities;
	int				oldnum;
	int				newnum;
	q3entityState_t	*oldstate;

	oldstate = NULL;
	newframe->firstEntity = ccs.firstParseEntity;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	numentities = 0;
	if( !oldframe )
	{
		oldnum = 99999;
	}
	else if( oldframe->numEntities <= 0 )
	{
		oldnum = 99999;
	}
	else
	{
		oldstate = &ccs.parseEntities[oldframe->firstEntity & Q3PARSE_ENTITIES_MASK];
		oldnum = oldstate->number;
	}

	while( 1 )
	{
		newnum = msgfuncs->ReadBits( GENTITYNUM_BITS );
		if (newnum < 0)
			plugfuncs->EndGame("CLQ3_ParsePacketEntities: end of message");
		else if (newnum >= MAX_GENTITIES )
			plugfuncs->EndGame("CLQ3_ParsePacketEntities: bad number %i", newnum);

		// end of packetentities
		if( newnum == ENTITYNUM_NONE )
		{
			break;
		}

		while( oldnum < newnum )
		{
			// one or more entities from the old packet are unchanged
			SHOWSTRING( va( "unchanged: %i", oldnum ) );

			CLQ3_DeltaEntity( newframe, oldnum, oldstate, true );

			numentities++;

			if( numentities >= oldframe->numEntities )
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &ccs.parseEntities[(oldframe->firstEntity + numentities) & Q3PARSE_ENTITIES_MASK];
				oldnum = oldstate->number;
			}
		}

		if( oldnum == newnum )
		{
			// delta from previous state
			SHOWSTRING( va( "delta: %i", newnum ) );

			CLQ3_DeltaEntity( newframe, newnum, oldstate, false );

			numentities++;

			if( numentities >= oldframe->numEntities )
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &ccs.parseEntities[(oldframe->firstEntity + numentities) & Q3PARSE_ENTITIES_MASK];
				oldnum = oldstate->number;
			}
			continue;
		}

		if( oldnum > newnum )
		{
			// delta from baseline
			SHOWSTRING( va( "baseline: %i", newnum ) );

			CLQ3_DeltaEntity( newframe, newnum, &ccs.baselines[newnum], false );
		}
	}

	// any remaining entities in the old frame are copied over
	while( oldnum != 99999 )
	{
		// one or more entities from the old packet are unchanged
		SHOWSTRING( va( "unchanged: %i", oldnum ) );

		CLQ3_DeltaEntity( newframe, oldnum, oldstate, true );

		numentities++;

		if( numentities >= oldframe->numEntities )
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &ccs.parseEntities[(oldframe->firstEntity + numentities) & Q3PARSE_ENTITIES_MASK];
			oldnum = oldstate->number;
		}
	}
}

void CLQ3_ParseSnapshot(void)
{
	clientSnap_t	snap, *oldsnap;
	int				delta;
	int				len;
//	int				i;
//	outframe_t		*frame;
//	usercmd_t		*ucmd;
//	int				commandTime;

	memset(&snap, 0, sizeof(snap));
	snap.serverMessageNum = ccs.serverMessageNum;
	snap.serverCommandNum = ccs.lastServerCommandNum;
	snap.serverTime = msgfuncs->ReadLong();

	//so we can delta to it properly.
	clientfuncs->UpdateGameTime(snap.serverTime / 1000.0f);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed message
	delta = msgfuncs->ReadByte();
	if(delta)
	{
		snap.deltaFrame = ccs.serverMessageNum - delta;
		oldsnap = &ccs.snapshots[snap.deltaFrame & Q3UPDATE_MASK];

		if(!oldsnap->valid)
		{
			// should never happen
			Con_Printf( "Delta from invalid frame (not supposed to happen!).\n");
		}
		else if( oldsnap->serverMessageNum != snap.deltaFrame )
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Con_DPrintf( "Delta frame too old.\n" );
		}
		else if(ccs.firstParseEntity - oldsnap->firstEntity >
			Q3MAX_PARSE_ENTITIES - MAX_ENTITIES_IN_SNAPSHOT)
		{
			Con_DPrintf( "Delta parse_entities too old.\n" );
		}
		else
		{
			snap.valid = true; // valid delta parse
		}
	}
	else
	{
		oldsnap = NULL;
		snap.deltaFrame = -1;
		snap.valid = true; // uncompressed frame
	}

	// read snapFlags
	snap.snapFlags = msgfuncs->ReadByte();

	// read areabits
	len = msgfuncs->ReadByte();
	msgfuncs->ReadData(snap.areabits, len );

	// read playerinfo
	SHOWSTRING("playerstate");
	MSG_Q3_ReadDeltaPlayerstate(oldsnap ? &oldsnap->playerstate : NULL, &snap.playerstate);

	// read packet entities
	SHOWSTRING("packet entities");
	CLQ3_ParsePacketEntities(oldsnap, &snap);

	if (!snap.valid)
	{
		return;
	}

//	cl.adjustTimeDelta = true;

	// Find last usercmd server has processed and calculate snap.ping

	snap.ping = 3;
/*	for (i=ccs.netchan.outgoing_sequence-1 ; i>ccs.netchan.outgoing_sequence-Q3CMD_BACKUP ; i--)
	{
		frame = &cl.outframes[i & Q3CMD_MASK];
		if (frame->server_message_num == snap.deltaFrame)
		{
			snap.ping = plugfuncs->GetMilliseconds() - frame->client_time;
			break;
		}
	}*/

	memcpy(&ccs.snap, &snap, sizeof(snap));
	memcpy(&ccs.snapshots[ccs.serverMessageNum & Q3UPDATE_MASK], &snap, sizeof(snap));

	SHOWSTRING(va("snapshot:%i  delta:%i  ping:%i", snap.serverMessageNum, snap.deltaFrame, snap.ping));
}

#define MAXCHUNKSIZE 65536
void CLQ3_ParseDownload(void)
{
	qdownload_t *dl = ccs.download;
	unsigned int chunknum;
	unsigned int chunksize;
	unsigned char chunkdata[MAXCHUNKSIZE];
	int i;
	char *s;

	chunknum = (unsigned short) msgfuncs->ReadShort();
	chunknum |= ccs.downloadchunknum&~0xffff;		//add the chunk number, truncated by the network protocol.

	if (!chunknum)
	{
		dl->size = (unsigned int)msgfuncs->ReadLong();
		cvarfuncs->SetFloat( cvarfuncs->GetNVFDG("cl_downloadSize", "0", 0, NULL, "Q3 Compat")->name, dl->size );	//so the gamecode knows download progress.
	}

	if (dl->size == (unsigned int)-1)
	{	//the only downloads we should be getting is pk3s.
		//if they're advertised-but-failing then its probably due to permissions rather than file-not-found
		s = msgfuncs->ReadString();
		clientfuncs->DownloadFailed(dl->remotename, dl, DLFAIL_SERVERCVAR);
		plugfuncs->EndGame("%s", s);
		return;
	}

	chunksize = (unsigned short)msgfuncs->ReadShort();
	if (chunksize > MAXCHUNKSIZE)
		plugfuncs->EndGame("Server sent a download chunk of size %i (it's too damn big!)\n", chunksize);

	for (i = 0; i < chunksize; i++)
		chunkdata[i] = msgfuncs->ReadByte();

	if (ccs.downloadchunknum != chunknum)	//the q3 client is rather lame.
	{										//ccs.downloadchunknum holds the chunk number.
		Con_DPrintf("PACKETLOSS WHEN DOWNLOADING!!!!\n");
		return;	//let the server try again some time
	}
	ccs.downloadchunknum++;

	if (!dl || dl->method != DL_Q3)
	{
		Con_Printf("Server sending download, but no download was requested\n");
		CLQ3_SendClientCommand("stopdl");
		return;
	}

	if (!dl->file)
	{
		if (!clientfuncs->DownloadBegun(dl))
		{
			clientfuncs->DownloadFailed(dl->remotename, dl, DLFAIL_CLIENTFILE);
			return;
		}
	}

	Con_DPrintf("dl: chnk %u, size %u, csize %u\n", (unsigned int)chunknum, (unsigned int)dl->size, (unsigned int)chunksize);

	if (!chunksize)
	{
		clientfuncs->DownloadFinished(dl);

		ccs.servercount = -1;	//make sure the server resends us that vital gamestate.
		ccs.downloadchunknum = -1;
		return;
	}
	else
	{
		VFS_WRITE(dl->file, chunkdata, chunksize);
		dl->ratebytes += chunksize;
		chunksize=VFS_TELL(dl->file);
//		Con_Printf("Recieved %i\n", chunksize);

		dl->percent = (100.0 * chunksize) / dl->size;
	}


	CLQ3_SendClientCommand("nextdl %u", chunknum);
}

static qboolean CLQ3_SendDownloads(char *rc, char *rn)
{
	while(rn)
	{
		qdownload_t *dl;
		char localname[MAX_QPATH];
		char tempname[MAX_QPATH];
		char filename[MAX_QPATH];
		char crc[64];
		vfsfile_t *f;
		rc = cmdfuncs->ParseToken(rc, crc, sizeof(crc), NULL);
		rn = cmdfuncs->ParseToken(rn, filename, sizeof(filename), NULL);
		if (!*filename)
			break;

		if (!strchr(filename, '/'))	//don't let some muppet tell us to download quake3.exe
			break;

		//as much as I'd like to use COM_FCheckExists, this stuf is relative to root, not the gamedir.
		f = fsfuncs->OpenVFS(va("%s.pk3", filename), "rb", FS_ROOT);
		if (f)
		{
			VFS_CLOSE(f);
			continue;
		}
		if (!fsfuncs->GenCachedPakName(va("%s.pk3", filename), crc, localname, sizeof(localname)))
			continue;
		f = fsfuncs->OpenVFS(localname, "rb", FS_ROOT);
		if (f)
		{
			VFS_CLOSE(f);
			continue;
		}

		if (!fsfuncs->GenCachedPakName(va("%s.tmp", filename), crc, tempname, sizeof(tempname)))
			continue;

		if (!cvarfuncs->GetFloat("cl_downloads"))
		{
			Con_Printf(CON_WARNING "Need to download %s.pk3, but downloads are disabled\n", filename);
			continue;
		}

		//fixme: request to download it
		Con_Printf("Sending request to download %s.pk3\n", filename);
		CLQ3_SendClientCommand("download %s.pk3", filename);
		ccs.downloadchunknum = 0;
		dl = Z_Malloc(sizeof(*dl));
		//q3's downloads are relative to root, but they do at least force a pk3 extension.
		Q_snprintfz(dl->localname, sizeof(dl->localname), "package/%s", localname);
		Q_snprintfz(dl->tempname, sizeof(dl->tempname), "package/%s", tempname);
		dl->prefixbytes = 8;
		dl->fsroot = FS_ROOT;

		Q_snprintfz(dl->remotename, sizeof(dl->remotename), "%s.pk3", filename);
		dl->method = DL_Q3;
		dl->percent = 0;
		ccs.download = dl;
		return true;
	}
	return false;
}

qboolean CLQ3_SystemInfoChanged(const char *str)
{
	qboolean usingpure, usingcheats;
	char *value;
	char *pc, *pn;
	char *rc, *rn;

	Con_Printf("Server's sv_pure: \"%s\"\n", worldfuncs->GetInfoKey(str, "sv_pure"));
	usingpure = atoi(worldfuncs->GetInfoKey(str, "sv_pure"));
	usingcheats = atoi(worldfuncs->GetInfoKey(str, "sv_cheats"));
	clientfuncs->ForceCheatVars(usingpure||usingcheats, usingcheats);

//	if (atoi(value))
//		Host_EndGame("Unable to connect to Q3 Pure Servers\n");
	value = worldfuncs->GetInfoKey(str, "fs_game");

	if (!*value)
	{
		value = "baseq3";
	}

	rc = worldfuncs->GetInfoKey(str, "sv_referencedPaks");	//the ones that we should download.
	rn = worldfuncs->GetInfoKey(str, "sv_referencedPakNames");
	if (CLQ3_SendDownloads(rc, rn))
		return false;

	pc = worldfuncs->GetInfoKey(str, "sv_paks");		//the ones that we are allowed to use (in order!)
	pn = worldfuncs->GetInfoKey(str, "sv_pakNames");
	fsfuncs->PureMode(value, usingpure?2:0, pn, pc, rn, rc, ccs.fs_key);

	return true;	//yay, we're in
}

void CLQ3_ParseGameState(sizebuf_t *msg)
{
	int		c;
	int		index;
	char	*configString;

//
// wipe the client_state_t struct
//
	clientfuncs->ClearClientState();
	CG_ClearGameState();
	ccs.firstParseEntity = 0;
	memset(ccs.parseEntities, 0, sizeof(ccs.parseEntities));
	memset(ccs.baselines, 0, sizeof(ccs.baselines));

	ccs.lastServerCommandNum = msgfuncs->ReadLong();

	for(;;)
	{
		c = msgfuncs->ReadByte();

		if(c < 0)
		{
			plugfuncs->EndGame("CLQ3_ParseGameState: read past end of server message");
			break;
		}

		if(c == svcq3_eom)
		{
			break;
		}

		SHOWNET(va("%i", c));

		switch(c)
		{
		default:
			plugfuncs->EndGame("CLQ3_ParseGameState: bad command byte %i", c);
			break;

		case svcq3_configstring:
			index = msgfuncs->ReadBits(16);
			if (index < 0 || index >= MAX_Q3_CONFIGSTRINGS)
			{
				plugfuncs->EndGame("CLQ3_ParseGameState: configString index %i out of range", index);
			}
			configString = msgfuncs->ReadString();

			CG_InsertIntoGameState(index, configString);
			break;

		case svcq3_baseline:
			index = msgfuncs->ReadBits(GENTITYNUM_BITS);
			if (index < 0 || index >= MAX_GENTITIES)
			{
				plugfuncs->EndGame("CLQ3_ParseGameState: baseline index %i out of range", index);
			}
			MSG_Q3_ReadDeltaEntity(NULL, &ccs.baselines[index], index);
			break;
		}
	}

	ccs.playernum = msgfuncs->ReadLong();
	ccs.fs_key = msgfuncs->ReadLong();

	if (!CLQ3_SystemInfoChanged(CG_GetConfigString(CFGSTR_SYSINFO)))
	{
		UI_Restart_f();
		return;
	}

	scenefuncs->NewMap(NULL);

	CG_Restart();
	UI_Restart_f();

	if (!ccs.worldmodel)
		plugfuncs->EndGame("CGame didn't set a map.\n");

	clientfuncs->SetLoadingState(false);

	ccs.state = ca_active;

	{
		char buffer[2048];
		strcpy(buffer, va("cp %i ", ccs.servercount));
		fsfuncs->GenerateClientPacksList(buffer, sizeof(buffer), ccs.fs_key);
		CLQ3_SendClientCommand("%s", buffer); // warning: format not a string literal and no format arguments
	}

	// load cgame, etc
//	CL_ChangeLevel();

	cvarfuncs->ForceSetString("cl_paused", "0");
}

int CLQ3_ParseServerMessage (sizebuf_t *msg)
{
	int cmd;
	if (!CLQ3_Netchan_Process(msg))
		return ccs.state;	//was a fragment.

	if (cl_shownet_ptr->value == 1)
		Con_Printf ("%i ", msg->cursize);
	else if (cl_shownet_ptr->value == 2)
		Con_Printf ("------------------\n");

	msgfuncs->BeginReading(msg, msg_nullnetprim);
	ccs.serverMessageNum = msgfuncs->ReadLong();
	msg->packing = SZ_HUFFMAN;	//the rest is huffman compressed.

	// read last client command number server received
	ccs.lastClientCommandNum = msgfuncs->ReadLong();
	if( ccs.lastClientCommandNum <= ccs.numClientCommands - Q3TEXTCMD_BACKUP )
	{
		ccs.lastClientCommandNum = ccs.numClientCommands - Q3TEXTCMD_BACKUP + 1;
	}
	else if( ccs.lastClientCommandNum > ccs.numClientCommands )
	{
		ccs.lastClientCommandNum = ccs.numClientCommands;
	}

//
// parse the message
//
	for(;;)
	{
		cmd = msgfuncs->ReadByte();

		if(cmd < 0)	//hm, we have an eom, so only stop when the message is bad.
		{
			plugfuncs->EndGame("CLQ3_ParseServerMessage: read past end of server message");
			break;
		}

		if(cmd == svcq3_eom)
		{
			SHOWNET2("END OF MESSAGE", 2);
			break;
		}

		SHOWNET(va("%i", cmd));

	// other commands
		switch(cmd)
		{
		default:
			plugfuncs->EndGame("CLQ3_ParseServerMessage: Illegible server message");
			break;
		case svcq3_nop:
			break;
		case svcq3_gamestate:
			CLQ3_ParseGameState(msg);
			break;
		case svcq3_serverCommand:
			CLQ3_ParseServerCommand();
			break;
		case svcq3_download:
			CLQ3_ParseDownload();
			break;
		case svcq3_snapshot:
			CLQ3_ParseSnapshot();
			break;
		}
	}
	return ccs.state;
}




static qboolean CLQ3_Netchan_Process(sizebuf_t *msg)
{
	if(Netchan_ProcessQ3(&ccs.netchan, msg))
	{
#ifndef Q3_NOENCRYPT
		int		sequence;
		int		lastClientCommandNum;
		qbyte	bitmask;
		qbyte	c;
		int		i, j;
		char	*string;
		int		readcount;

		msgfuncs->BeginReading(msg, msg_nullnetprim);
		sequence = msgfuncs->ReadLong();
		msg->packing = SZ_HUFFMAN;
		readcount = msg->currentbit>>3;
		lastClientCommandNum = msgfuncs->ReadLong();

		// calculate bitmask
		bitmask = (sequence ^ ccs.challenge) & 0xff;
		string = ccs.clientCommands[lastClientCommandNum & Q3TEXTCMD_MASK];

		// decrypt the packet
		for(i=readcount+4,j=0 ; i<msg->cursize ; i++,j++)
		{
			if(!string[j])
				j = 0; // another way around
			c = string[j];
			if(c > 127 || c == '%')
				c = '.';
			bitmask ^= c << ((i-readcount) & 1);
			msg->data[i] ^= bitmask;
		}
		msg->packing = SZ_RAWBITS;	//first bit was plain...
#endif
		return true;	//all good
	}
	return false;	//its bad dude, bad.
}

void CL_Netchan_Transmit(struct ftenet_connections_s *socket, int length, const qbyte *data )
{
#ifndef Q3_NOENCRYPT
	sizebuf_t msg;
	char msgdata[MAX_OVERALLMSGLEN];
	int			serverid;
	int			lastSequence;
	int			lastServerCommandNum;
	qbyte		bitmask;
	qbyte		c;
	int			i, j;
	char		*string;

	msgfuncs->BeginWriting(&msg, msg_nullnetprim, msgdata, sizeof(msgdata));
	msgfuncs->WriteData(&msg, data, length);

	if (msg.overflowed)
	{
		plugfuncs->EndGame("Client message overflowed");
	}

	msgfuncs->BeginReading(&msg, msg_nullnetprim);
	msg.packing = SZ_HUFFMAN;

	serverid = msgfuncs->ReadLong();
	lastSequence = msgfuncs->ReadLong();
	lastServerCommandNum = msgfuncs->ReadLong();

	// calculate bitmask
	bitmask = (lastSequence ^ serverid ^ ccs.challenge) & 0xff;
	string = ccs.serverCommands[lastServerCommandNum & Q3TEXTCMD_MASK];

	// encrypt the packet
	for (i=12,j=0 ; i<msg.cursize ; i++,j++)
	{
		if (!string[j])
		{
			j = 0; // another way around
		}
		c = string[j];
		if (c > 127 || c == '%')
		{
			c = '.';
		}
		bitmask ^= c << (i & 1);
		msg.data[i] ^= bitmask;
	}
	data = msg.data;
	length = msg.cursize;
#endif
	Netchan_TransmitQ3(socket, &ccs.netchan, length, data);
}




static void MSG_WriteDeltaKey( sizebuf_t *msg, int key, int from, int to, int bits )
{
	if( from == to )
	{
		msgfuncs->WriteBits( msg, 0, 1 );
		return; // unchanged
	}

	msgfuncs->WriteBits( msg, 1, 1 );
	msgfuncs->WriteBits( msg, to ^ key, bits );
}

void MSG_Q3_WriteDeltaUsercmd( sizebuf_t *msg, int key, const usercmd_t *from, const usercmd_t *to )
{
	// figure out how to pack serverTime
	if( to->servertime - from->servertime < 255 )
	{
		msgfuncs->WriteBits(msg, 1, 1);
		msgfuncs->WriteBits(msg, to->servertime - from->servertime, 8);
	}
	else
	{
		msgfuncs->WriteBits( msg, 0, 1 );
		msgfuncs->WriteBits( msg, to->servertime, 32);
	}

	if( !memcmp( (qbyte *)from + 4, (qbyte *)to + 4, sizeof( usercmd_t ) - 4 ) )
	{
		msgfuncs->WriteBits(msg, 0, 1);
		return; // nothing changed
	}
	key ^= to->servertime;
	msgfuncs->WriteBits(msg, 1, 1);
	MSG_WriteDeltaKey(msg, key, from->angles[0],		to->angles[0],		16);
	MSG_WriteDeltaKey(msg, key, from->angles[1],		to->angles[1],		16);
	MSG_WriteDeltaKey(msg, key, from->angles[2],		to->angles[2],		16);
	MSG_WriteDeltaKey(msg, key, from->forwardmove,		to->forwardmove,	 8);
	MSG_WriteDeltaKey(msg, key, from->sidemove,			to->sidemove,		 8);
	MSG_WriteDeltaKey(msg, key, from->upmove,			to->upmove,			 8);
	MSG_WriteDeltaKey(msg, key, from->buttons,			to->buttons,		16);
	MSG_WriteDeltaKey(msg, key, from->weapon,			to->weapon,			 8);
}



void VARGS CLQ3_SendClientCommand(const char *fmt, ...)
{
	va_list	argptr;
	char	command[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	vsnprintf( command, sizeof(command), fmt, argptr );
	va_end( argptr );

	// create new clientCommand
	ccs.numClientCommands++;

	// check if server will lose some of our clientCommands
	if(ccs.numClientCommands - ccs.lastClientCommandNum >= Q3TEXTCMD_BACKUP)
		plugfuncs->EndGame("Client command overflow");

	Q_strncpyz(ccs.clientCommands[ccs.numClientCommands & Q3TEXTCMD_MASK], command, sizeof(ccs.clientCommands[0]));
	Con_DPrintf("Sending %s\n", command);
}

void CLQ3_SendCmd(struct ftenet_connections_s *socket, usercmd_t *cmd, unsigned int movesequence, double gametime)
{
	char *string;
	int i;
	char data[MAX_OVERALLMSGLEN];
	sizebuf_t msg;
//	outframe_t *frame;
	unsigned int oldsequence;
	int cmdcount, key;
	const usercmd_t *to, *from;
	static usercmd_t nullcmd;

	//reuse the q1 array
	cmd->servertime = gametime*1000;
	cmd->weapon = ccs.selected_weapon;


	cmd->forwardmove *= 127/400.0f;
	cmd->sidemove *= 127/400.0f;
	cmd->upmove *= 127/400.0f;
	if (cmd->forwardmove > 127)
		cmd->forwardmove = 127;
	if (cmd->forwardmove < -127)
		cmd->forwardmove = -127;
	if (cmd->sidemove > 127)
		cmd->sidemove = 127;
	if (cmd->sidemove < -127)
		cmd->sidemove = -127;
	if (cmd->upmove > 127)
		cmd->upmove = 127;
	if (cmd->upmove < -127)
		cmd->upmove = -127;

	if (cmd->buttons & 2)	//jump
	{
		cmd->upmove = 100;
		cmd->buttons &= ~2;
	}
	if (inputfuncs->GetKeyDest() & ~kdm_game)
		cmd->buttons |= 2;	//add in the 'at console' button

	//FIXME: q3 generates a new command every video frame, but a new packet at a more limited rate.
	//FIXME: we should return here if its not yet time for a network frame.

/*	frame = &cl.outframes[ccs.netchan.outgoing_sequence & Q3CMD_MASK];
	frame->cmd_sequence = movesequence;
	frame->server_message_num = ccs.serverMessageNum;
	frame->server_time = gametime;
	frame->client_time = plugfuncs->GetMilliseconds();
*/
	memset(&msg, 0, sizeof(msg));
	msg.maxsize = sizeof(data);
	msg.data = data;
	msg.packing = SZ_HUFFMAN;

	msgfuncs->WriteBits(&msg, ccs.servercount, 32);
	msgfuncs->WriteBits(&msg, ccs.serverMessageNum, 32);
	msgfuncs->WriteBits(&msg, ccs.lastServerCommandNum, 32);

	// write clientCommands not acknowledged by server yet
	for (i=ccs.lastClientCommandNum+1; i<=ccs.numClientCommands; i++)
	{
		msgfuncs->WriteBits(&msg, clcq3_clientCommand, 8);
		msgfuncs->WriteBits(&msg, i, 32);
		string = ccs.clientCommands[i & Q3TEXTCMD_MASK];
		while(*string)
			msgfuncs->WriteBits(&msg, *string++, 8);
		msgfuncs->WriteBits(&msg, 0, 8);
	}

	i = ccs.netchan.outgoing_sequence;
	i -= bound(0, cl_c2sdupe_ptr->ival, 5); //extra age, if desired
	i--;
	if (i < ccs.netchan.outgoing_sequence-Q3CMD_MASK)
		i = ccs.netchan.outgoing_sequence-Q3CMD_MASK;
	oldsequence = movesequence-1;//cl.outframes[i & Q3CMD_MASK].cmd_sequence;
	cmdcount = movesequence - oldsequence;
	if (cmdcount > Q3CMD_MASK)
		cmdcount = Q3CMD_MASK;

	// begin a client move command, if any
	if (cmdcount)
	{
		if(cl_nodelta_ptr->value || !ccs.snap.valid || ccs.snap.serverMessageNum != ccs.serverMessageNum)
			msgfuncs->WriteBits(&msg, clcq3_nodeltaMove, 8); // no compression
		else
			msgfuncs->WriteBits(&msg, clcq3_move, 8);

		// write cmdcount
		msgfuncs->WriteBits(&msg, cmdcount, 8);

		// calculate key
		string = ccs.serverCommands[ccs.lastServerCommandNum & Q3TEXTCMD_MASK];
		key = ccs.fs_key ^ ccs.serverMessageNum ^ StringKey(string, 32);

		//note that q3 uses timestamps so sequences are not important
		//we can also send dupes without issue.
		from = &nullcmd;
		for (i = movesequence-cmdcount; i < movesequence; i++)
		{
			to = inputfuncs->GetMoveEntry(i);
			if (!to)
				to = from;
			MSG_Q3_WriteDeltaUsercmd( &msg, key, from, to );
			from = to;
		}
	}

	msgfuncs->WriteBits(&msg, clcq3_eom, 8);

	CL_Netchan_Transmit(socket, msg.cursize, msg.data );
	while(ccs.netchan.reliable_length)
		Netchan_TransmitNextFragment(socket, &ccs.netchan);
}

void CLQ3_SendAuthPacket(struct ftenet_connections_s *socket, netadr_t *gameserver)
{
#ifdef HAVE_PACKET
	char data[2048];
	sizebuf_t msg;

//send the auth packet
//this should be the right code, but it doesn't work.
	if (gameserver->type == NA_IP && gameserver->prot == NP_DGRAM)
	{
		char *key = cvarfuncs->GetNVFDG("cl_cdkey", "", CVAR_ARCHIVE, "Quake3 auth", "Q3 Compat")->string;
		netadr_t authaddr;
#define	Q3_AUTHORIZE_SERVER_NAME	"authorize.quake3arena.com:27952"
		if (*key)
		{
			Con_Printf("Resolving %s\n", Q3_AUTHORIZE_SERVER_NAME);
			if (masterfuncs->StringToAdr(Q3_AUTHORIZE_SERVER_NAME, 0, &authaddr, 1, NULL))
			{
				msgfuncs->BeginWriting(&msg, msg_nullnetprim, data, sizeof(data));
				msgfuncs->WriteLong(&msg, -1);
				msgfuncs->WriteString(&msg, "getKeyAuthorize 0 ");
				msg.cursize--;
				while(*key)
				{
					if ((*key >= 'a' && *key <= 'z') || (*key >= 'A' && *key <= 'Z') || (*key >= '0' && *key <= '9'))
						msgfuncs->WriteByte(&msg, *key);
					key++;
				}
				msgfuncs->WriteByte(&msg, 0);

				msgfuncs->SendPacket(socket, msg.cursize, msg.data, &authaddr);
			}
			else
				Con_Printf("    failed\n");
		}
	}
#endif
}

void CLQ3_SendConnectPacket(struct ftenet_connections_s *socket, netadr_t *to, int challenge, int qport, infobuf_t *userinfo)
{
	char infostr[1024];
	char data[2048];
	sizebuf_t msg;
	static const char *priorityq3[] = {"*", "name", NULL};
	static const char *nonq3[] = {"challenge", "qport", "protocol", "ip", "chat", NULL};
	int protocol = cvarfuncs->GetFloat("com_protocolversion");

	memset(&ccs, 0, sizeof(ccs));
	ccs.servercount = -1;
	ccs.challenge = challenge;
	Netchan_SetupQ3(NCF_CLIENT, &ccs.netchan, to, qport);

	worldfuncs->IBufToInfo(userinfo, infostr, sizeof(infostr), priorityq3, nonq3, NULL, NULL,/*&cls.userinfosync,*/ userinfo);

	msgfuncs->BeginWriting(&msg, msg_nullnetprim, data, sizeof(data));
	msgfuncs->WriteLong(&msg, -1);
	msgfuncs->WriteString(&msg, va("connect \"\\challenge\\%i\\qport\\%i\\protocol\\%i%s\"", challenge, qport, protocol, infostr));
#ifdef HUFFNETWORK
	if (msgfuncs->Huff_EncryptPacket)
		msgfuncs->Huff_EncryptPacket(&msg, 12);
	if (!msgfuncs->Huff_CompressionCRC || !msgfuncs->Huff_CompressionCRC(HUFFCRC_QUAKE3))
	{
		Con_Printf("Huffman compression error\n");
		return;
	}
#endif
	msgfuncs->SendPacket (socket, msg.cursize, msg.data, to);
}

void CLQ3_Established(void)
{
	ccs.state = ca_connected;
}

void CLQ3_Disconnect(struct ftenet_connections_s *socket)
{
	ccs.state = ca_disconnected;
}
#endif

