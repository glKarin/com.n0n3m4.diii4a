/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "server.h"


/*
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	len = strlen(sv.configstrings[index]);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &sv.configstrings[index][sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"\n", index,
			sv.configstrings[index] );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index < MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && client->gentity &&
			(client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
			continue;
		}
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	int		i;
	client_t	*client;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_SetConfigstring: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevant clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}
		
			SV_SendConfigstring(client, index);
		}
	}
}

/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_GetConfigstring: bad index %i", index);
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_GetUserinfo: bad index %i", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
static void SV_CreateBaseline( void ) {
	sharedEntity_t *svent;
	int				entnum;	

	for ( entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->r.linked) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[entnum].baseline = svent->s;
	}
}


/*
===============
SV_BoundMaxClients

===============
*/
static void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "8", 0 );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}


/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
static void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = Z_Malloc (sizeof(client_t) * sv_maxclients->integer );
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		Cvar_Set( "sv_killserver", "0" );
	}

	Cvar_Set( "sv_running", "1" );
	
	// Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
	NET_JoinMulticast6();
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	Z_Free( svs.clients );

	// allocate new clients
	svs.clients = Z_Malloc ( sv_maxclients->integer * sizeof(client_t) );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof(client_t) );

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );
	
	// allocate new snapshot entities
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
}

/*
================
SV_ClearServer
================
*/
static void SV_ClearServer(void) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
		}
	}
	Com_Memset (&sv, 0, sizeof(sv));
}

/*
================
SV_TouchCGame

  touch the cgame.vm so that a pure client can load it if it's in a separate pk3
================
*/
static void SV_TouchCGame(void) {
	fileHandle_t	f;
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", "cgame" );
	FS_FOpenFileRead( filename, &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

/*
================
TextEncode6Bit

Encodes text using 6 bit alphabet for map names
================
*/

int TextEncode6Bit(const char *nam, unsigned char *buf, int blen)
{
	static int char2val[256] = { // character mapping table
			0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			1,2,-1,3,-1,4,5,6,7,8,-1,9,10,11,12,-1,13,14,15,16,17,18,19,20,21,22,-1,23,-1,24,-1,-1,25,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,26,-1,27,28,29,30,31,32,
			33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,-1,58,59,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	};
	int ol;
	static char tmp[BIG_INFO_STRING * 4];
	char *tp = tmp+1;

	tmp[0]=' '; // add space at start to compress first name using " ut4_"
	while(*nam)  *(tp++) = tolower(*(nam++));

	tp = tmp;
	ol = 0;
	while(1) { // Use 4 values for most common words (based on analysis of 5000+ map filenames)
		int x;
		char ch = *tp;
		if (ch==' ' && tp[1]=='u' && tp[2]=='t' && tp[3]=='4' && tp[4]=='_') { // " ut4_"
			x = 60; tp+=5;
		} else
		if (ch==' ' && tp[1]=='u' && tp[2]=='t' && tp[3]=='_') { // " ut_"
			x = 61; tp+=4;
		} else
		if (ch=='_' && tp[1]=='b' && tp[2]=='e' && tp[3]=='t' && tp[4]=='a') { //"_beta"
			x = 62; tp+=5;
		} else
		if (ch=='j' && tp[1]=='u' && tp[2]=='m' && tp[3]=='p' && tp[4]=='s') { //"jumps"
			x = 63; tp+=5;
		} else {
			x = char2val[(unsigned int)ch];
			tp++;
		}
		if (x == -1) {
			Com_Printf("Invalid character in pk3 file name: '%c'\n",ch);
			x = char2val['~']; // replace with ~ character to not break everythign else
		}
		buf[ol++] = x;
		if (ol == blen) {
			Com_Printf("TextEncode6Bit: target buffer overflow!\n");
			buf[ol - 1] = 0;
			return ol;
		}
		if (!ch)
			return ol;
	}
}

/*
================
SV_MakeCompressedPureList

Fills the last N configstrings with compressed pure list
================
*/
void SV_MakeCompressedPureList(void) {

	unsigned char buf[PURE_COMPRESS_BUFFER];
	char tmp[1025];
	int i, l, bl, sh, shl, ol, csnr;
	const char *nam;
	msg_t msg = {qfalse};
	const char *err_chunk = "Too many pk3 files (compressed data doesn't fit into available space)";

	// Get raw checksums
	bl = FS_LoadedPakChecksumsBlob(buf+2,sizeof(buf)-2);
	if (!bl) Com_Error(ERR_DROP,"Too many pk3 files (size checksums > buffer)");
	// Com_Printf("CRC SIZE(%d)\n",bl);

	// Add number of files at start
	l = bl / 4; // num files
	buf[0] = (l) & 0xFF;
	buf[1] = (l >> 8) & 0xFF;

	// Get pak names
	nam = FS_LoadedPakNames();
#if 1
	msg.cursize = 2+bl;
	msg.cursize += TextEncode6Bit(nam,buf + msg.cursize, sizeof(buf) - msg.cursize);
#else
	msg.cursize = 2+bl+strlen(nam)+1;
 if (msg.cursize>sizeof(buf)) Com_Error(ERR_DROP,"Too many pk3 files (no space in buffer for names, need %d bytes)",msg.cursize-sizeof(buf));
 strcpy((char*)buf+2+bl,nam);
#endif
	Com_Printf("Pure filelist (%d files) size: %d\n",l,msg.cursize);

	// fprintf(stderr,"FNAMES: %s\n",nam);
	// Com_Printf("BUF(%d): ",msg.cursize);
	// for(i=0;i<msg.cursize;i++) Com_Printf("%02X",buf[i]);
	// Com_Printf("\n");

	// Huffman compress names
	msg.maxsize = sizeof(buf);
	msg.data = buf;
	Huff_Compress(&msg,2+bl);
	bl = msg.cursize;

	// fprintf(stderr,"COMPRESSED(%d): ",msg.cursize);
	// for(i=0;i<msg.cursize;i++) fprintf(stderr,"%02X",buf[i]);
	// fprintf(stderr,"\n");

	Com_Printf("Pure filelist compressed size: %d\n",bl);

	// Do 8bit to 7bit encoding, escaping \0 % " using @ char and original value + 1
	csnr = 0;
	sh = 0;
	shl = 0;
	ol = 0;
	for(i = 0; i < bl; i++) {
		sh = (sh << 8)|buf[i];
		shl += 8;
		//Com_Printf("IN:%02X\n",buf[i]);
		while(shl >= 7) {
			shl -= 7;
			int v = (sh >> shl) & 127;
			if (v == 0 || v=='"' || v=='%' || v=='@') {
				tmp[ol++] = '@';
				//Com_Printf("OUT:%02X\n",tmp[ol-1]);
				if (ol == sizeof(tmp)-1) {
					tmp[ol] = 0;
					if (csnr == PURE_COMPRESS_NUMCS)
						Com_Error(ERR_DROP, "%s", err_chunk);
					SV_SetConfigstring(MAX_CONFIGSTRINGS - PURE_COMPRESS_NUMCS + csnr, tmp);
					csnr++;
					ol = 0;
				}
				tmp[ol++] = v + 1;
			} else {
				tmp[ol++] = v;
			}
			//Com_Printf("OUT:%02X\n",tmp[ol-1]);
			if (ol == sizeof(tmp) - 1) {
				tmp[ol] = 0;
				if (csnr == PURE_COMPRESS_NUMCS)
					Com_Error(ERR_DROP, "%s", err_chunk);
				SV_SetConfigstring( MAX_CONFIGSTRINGS-PURE_COMPRESS_NUMCS+csnr, tmp);
				csnr++;
				ol=0;
			}
		}
	}
	// Com_Printf("[shl=%d sh=%X]\n",shl,sh);

	// Finish remaining bits if any
	if (shl > 0) {
		int v = (sh << (7 - shl)) & 127;
		if (v == 0 || v == '"' || v == '%' || v == '@') {
			tmp[ol++] = '@';
			if (ol == sizeof(tmp)-1) {
				tmp[ol] = 0;
				if (csnr == PURE_COMPRESS_NUMCS)
					Com_Error(ERR_DROP, "%s", err_chunk);
				SV_SetConfigstring(MAX_CONFIGSTRINGS - PURE_COMPRESS_NUMCS + csnr, tmp);
				csnr++;
				ol = 0;
			}
			tmp[ol++] = v+1;
		} else {
			tmp[ol++] = v;
		}
		//Com_Printf("OUT:%02X\n",tmp[ol-1]);
	}
	if (ol) {
		if (csnr == PURE_COMPRESS_NUMCS)
			Com_Error(ERR_DROP, "%s", err_chunk);
		tmp[ol] = 0;
		SV_SetConfigstring(MAX_CONFIGSTRINGS - PURE_COMPRESS_NUMCS + csnr, tmp);
	}
	Com_Printf("Using %d configstrings to store pure filelist (encoded using %d characters)\n", csnr + 1, ol + csnr * 1024);
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	char		systemInfo[16384];
	const char	*p;

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf ("------ Server Initialization ------\n");
	Com_Printf ("Server: %s\n",server);

#ifndef DEDICATED
	// clear pure status when loading the server since the map we're loading might not be on the
	// pure list and would fail to load
	FS_PureServerSetLoadedPaks("", "");
#endif

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll(qfalse);

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

	// clear collision map data
	CM_ClearMap();

	// init client structures and svs.numSnapshotEntities 
	if ( !Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences(0);

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set( "nextmap", "map_restart 0");
//	Cvar_Set( "nextmap", va("map %s", server) );

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// save when the server started for each client already connected
		if (svs.clients[i].state >= CS_CONNECTED) {
			svs.clients[i].oldServerTime = sv.time;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	sv.checksumFeed = ( ((unsigned int)rand() << 16) ^ (unsigned int)rand() ) ^ Com_Milliseconds();
	FS_SetMapName(server);
	FS_Restart( sv.checksumFeed );

	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	sv.checksumFeedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld ();
	
	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// don't allow a map_restart if game is modified
	sv_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for (i = 0;i < 3; i++)
	{
		VM_Call (gvm, GAME_RUN_FRAME, sv.time);
		SV_BotFrame (sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline ();
	
	// stop server-side demo (if any)
	if (com_dedicated->integer)
		Cbuf_ExecuteText(EXEC_NOW, "stopserverdemo all");

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				if ( killBots ) {
					SV_DropClient( &svs.clients[i], "" );
					continue;
				}
				isBot = qtrue;
			}
			else {
				isBot = qfalse;
			}

			// connect the client again
			denied = VM_ExplicitArgPtr( gvm, VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				if( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else {
					client_t		*client;
					sharedEntity_t	*ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = -1;
					client->lastSnapshotTime = 0;	// generate a snapshot immediately

					VM_Call( gvm, GAME_CLIENT_BEGIN, i );
				}
			}
		}
	}	

	// run another frame to allow things to look at all the players
	VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	SV_BotFrame (sv.time);
	sv.time += 100;
	svs.time += 100;

	if ( sv_pure->integer ) {
		// if a dedicated pure server we need to touch the cgame because it could be in a
		// separate pk3 file and the client will need to load the latest cgame.qvm
		if ( com_dedicated->integer ) {
			SV_TouchCGame();
		}

		// Update latched value
		Cvar_Get ("sv_extraPaks", "", CVAR_ARCHIVE | CVAR_LATCH);
		Cvar_Get ("sv_extraPure", "0", CVAR_ARCHIVE | CVAR_LATCH);

		if ( sv_extraPure->integer ) {
			FS_SetExtraPure(server, sv_extraPaks->string);
		}

		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		if (sv_newpurelist->integer) {
			Cvar_Set( "sv_paks", "*" );
			Cvar_Set( "sv_pakNames", "*" );
			SV_MakeCompressedPureList();
		} else {
			p = FS_LoadedPakChecksums();
			Cvar_Set( "sv_paks", p );
			if (!p[0]) {
				Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
			}
			p = FS_LoadedPakNames();
			Cvar_Set( "sv_pakNames", p );
		}
	}
	else {
		Cvar_Set( "sv_paks", "" );
		Cvar_Set( "sv_pakNames", "" );
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = FS_ReferencedPakChecksums();
	Cvar_Set( "sv_referencedPaks", p );
	p = FS_ReferencedPakNames();
	Cvar_Set( "sv_referencedPakNames", p );

	// save systeminfo and serverinfo strings
	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );

	{
		const char *t,*tt;
		int l,l2;

		t = Info_ValueForKey( systemInfo, "sv_paks" );
		l = 0;
		if (t) {
			tt = t;
			while(*tt) { if (*tt==' ') l++; tt++; }
		}

		t = Info_ValueForKey( systemInfo, "sv_pakNames" );
		l2 = 0;
		if (t) {
			tt = t;
			while(*tt) { if (*tt==' ') l2++; tt++; }
		}
		if (l != l2) {
			Com_Printf( "WARNING: Pure pak file list inconsistency (%d checksums, %d file names)\n",l,l2 );
		}
	}

	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Hunk_SetMark();

#ifndef DEDICATED
	if ( com_dedicated->integer ) {
		// restart renderer in order to show console for dedicated servers
		// launched through the regular binary
		CL_StartHunkUsers( qtrue );
	}
#endif

	Com_Printf ("-----------------------------------\n");
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init (void)
{
	int index;

	SV_AddOperatorCommands ();

	// serverinfo vars
	Cvar_Get ("dmflags", "0", CVAR_ARCHIVE);
	Cvar_Get ("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	sv_gametype = Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH );
	Cvar_Get ("sv_keywords", "", CVAR_SERVERINFO);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get ("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get ("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_maxclients = Cvar_Get ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);

	sv_minRate = Cvar_Get ("sv_minRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxRate = Cvar_Get ("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_minPing = Cvar_Get ("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get ("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get ("sv_floodProtect", "2", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_clientsPerIp = Cvar_Get ("sv_clientsPerIp", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_newpurelist = Cvar_Get ("sv_newpurelist", "0", CVAR_ARCHIVE );

	// systeminfo
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_pure = Cvar_Get ("sv_pure", "1", CVAR_SYSTEMINFO );
#ifdef USE_VOIP
	sv_voip = Cvar_Get("sv_voip", "1", CVAR_LATCH);
	Cvar_CheckRange(sv_voip, 0, 1, qtrue);
	sv_voipProtocol = Cvar_Get("sv_voipProtocol", sv_voip->integer ? "opus" : "", CVAR_SYSTEMINFO | CVAR_ROM );
#endif
	Cvar_Get ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get ("sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get ("sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get ("sv_timeout", "200", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get ("nextmap", "", CVAR_TEMP );
	Cvar_Get ("sv_cheatMode", "0", CVAR_TEMP );

	Cvar_Get ("sv_dlURL", "", CVAR_SERVERINFO | CVAR_ARCHIVE);
	
	sv_master[0] = Cvar_Get("sv_master1", MASTER_SERVER_NAME, 0);
	sv_master[1] = Cvar_Get("sv_master2", "master.ioquake3.org", 0);
	for(index = 2; index < MAX_MASTER_SERVERS; index++)
		sv_master[index] = Cvar_Get(va("sv_master%d", index + 1), "", CVAR_ARCHIVE);

	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get ("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get ("sv_lanForceRate", "1", CVAR_ARCHIVE );
	sv_banFile = Cvar_Get("sv_banFile", "serverbans.dat", CVAR_ARCHIVE);

	sv_demonotice = Cvar_Get ("sv_demonotice", "Smile! You're on camera!", CVAR_ARCHIVE);
	sv_demofolder = Cvar_Get ("sv_demofolder", "serverdemos", CVAR_ARCHIVE );
	sv_autoRecordDemo = Cvar_Get ("sv_autoRecordDemo", "0", CVAR_ARCHIVE );

	sv_sayprefix = Cvar_Get ("sv_sayprefix", "console: ", CVAR_ARCHIVE );
	sv_tellprefix = Cvar_Get ("sv_tellprefix", "console_tell: ", CVAR_ARCHIVE );
	sv_extraPaks = Cvar_Get ("sv_extraPaks", "", CVAR_ARCHIVE | CVAR_LATCH);
	sv_extraPure = Cvar_Get ("sv_extraPure", "0", CVAR_ARCHIVE | CVAR_LATCH);

#ifdef USE_AUTH
	sv_authServerIP = Cvar_Get("sv_authServerIP", "", CVAR_TEMP | CVAR_ROM);
	sv_auth_engine = Cvar_Get("sv_auth_engine", "1", CVAR_ROM);
#endif

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SV_BotInitBotLib();
	
	// Load saved bans
	Cbuf_AddText("rehashbans\n");
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;
	
	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\n\"\n", message );
					SV_SendServerCommand( cl, "disconnect \"%s\"", message );
				}
				// force a snapshot to be sent
				cl->lastSnapshotTime = 0;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg ) {
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown (%s) -----\n", finalmsg );

	NET_LeaveMulticast6();

	// stop server-side demos (if any)
	if (com_dedicated->integer)
		Cbuf_ExecuteText(EXEC_NOW, "stopserverdemo all");
	
	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ShutdownGameProgs();

	// free current level
	SV_ClearServer();

	// free server static data
	if(svs.clients)
	{
		int index;
		
		for(index = 0; index < sv_maxclients->integer; index++)
			SV_FreeClient(&svs.clients[index]);
		
		Z_Free(svs.clients);
	}
	Com_Memset( &svs, 0, sizeof( svs ) );

	Cvar_Set( "sv_running", "0" );
	Cvar_Set("ui_singlePlayerActive", "0");

	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	if( sv_killserver->integer != 2 )
		CL_Disconnect( qfalse );
}

