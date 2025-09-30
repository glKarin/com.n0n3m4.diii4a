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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] = {
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",	
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot"
};

void SHOWNET( msg_t *msg, char *s) {
	if ( cl_shownet->integer >= 2) {
		Com_Printf ("%3i:%s\n", msg->readcount-1, s);
	}
}


/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old,
					 qboolean unchanged) {
	entityState_t	*state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

	if ( unchanged ) {
		*state = *old;
	} else {
		MSG_ReadDeltaEntity( msg, old, state, newnum );
	}

	if ( state->number == (MAX_GENTITIES-1) ) {
		return;		// entity was delta removed
	}
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe) {
	int			newnum;
	entityState_t	*oldstate;
	int			oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = NULL;
	if (!oldframe) {
		oldnum = 99999;
	} else {
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while ( 1 ) {
		// read the entity index number
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum == (MAX_GENTITIES-1) ) {
			break;
		}

		if ( msg->readcount > msg->cursize ) {
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: end of message");
		}

		while ( oldnum < newnum ) {
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );
			
			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum) {
			// delta from previous state
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum ) {
			// delta from baseline
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[newnum], qfalse );
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != 99999 ) {
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->integer == 3 ) {
			Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );
		
		oldindex++;

		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}


/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot( msg_t *msg ) {
	int			len;
	clSnapshot_t	*old;
	clSnapshot_t	newSnap;
	int			deltaNum;
	int			oldMessageNum;
	int			i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset (&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = clc.serverCommandSequence;

	newSnap.serverTime = MSG_ReadLong( msg );

	// if we were just unpaused, we can only *now* really let the
	// change come into effect or the client hangs.
	cl_paused->modified = 0;

	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;		// uncompressed frame
		old = NULL;
		clc.demowaiting = qfalse;	// we can start recording now
	} else {
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if ( !old->valid ) {
			// should never happen
			Com_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		} else if ( old->messageNum != newSnap.deltaNum ) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf ("Delta frame too old.\n");
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
			Com_Printf ("Delta parseEntitiesNum too old.\n");
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );
	
	if(len > sizeof(newSnap.areamask))
	{
		Com_Error (ERR_DROP,"CL_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}
	
	MSG_ReadData( msg, &newSnap.areamask, len);

	// read playerinfo
	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid ) {
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}
	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ ) {
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) {
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if (cl_shownet->integer == 3) {
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
		cl.snap.deltaNum, cl.snap.ping );
	}

	cl.newSnapshots = qtrue;
}


//=====================================================================

int cl_connectedToPureServer;
int cl_connectedToCheatServer;

/*
================
TextDecode6Bit

Decode pk3 file names encoded usign 6 bit alphabet
================
*/

void TextDecode6Bit(unsigned char *buf, int blen, char *out, int olen)
{
 static char val2char[] = "\x00 !#%&'()+,-.0123456789;=@[]^_`abcdefghijklmnopqrstuvwxyz{}~";
 int i;
 int sh = 0;
 int shn = 0;
 int op = -1; // encode added space at start

 for(i=0;i<blen;i++) {
  int v = buf[i];
  if (v>=64) Com_Error (ERR_DROP,"TextDecode6Bit: Character value out of range (%d)\n",v);
  if (v<60) {
   v = val2char[v];
   if (op>=0) out[op] = v;
   op++;
   if (!v) return; // null terminated, we are done
  } else {
   switch (v-60) {
    case 0:
     if (op+5>olen) {
      buf[op]=0;
      Com_Printf("TextDecode6Bit: target buffer overflow!\n");
      return;
     }
     if (op>=0) out[op+0] = ' ';
     out[op+1] = 'u';
     out[op+2] = 't';
     out[op+3] = '4';
     out[op+4] = '_';
     op+=5;
     break;
    case 1:
     if (op+4>olen) {
      buf[op]=0;
      Com_Printf("TextDecode6Bit: target buffer overflow!\n");
      return;
     }
     if (op>=0) out[op+0] = ' ';
     out[op+1] = 'u';
     out[op+2] = 't';
     out[op+3] = '_';
     op+=4;
     break;
    case 2:
     if (op+5>olen) {
      buf[op]=0;
      Com_Printf("TextDecode6Bit: target buffer overflow!\n");
      return;
     }
     if (op>=0) out[op+0] = '_';
     out[op+1] = 'b';
     out[op+2] = 'e';
     out[op+3] = 't';
     out[op+4] = 'a';
     op+=5;
     break;
    case 3:
     if (op+5>olen) {
      buf[op]=0;
      Com_Printf("TextDecode6Bit: target buffer overflow!\n");
      return;
     }
     if (op>=0) out[op+0] = 'j';
     out[op+1] = 'u';
     out[op+2] = 'm';
     out[op+3] = 'p';
     out[op+4] = 's';
     op+=5;
     break;
   }
  }
  if (op==olen) {
   break;
  }
 }
}


/*
================
CL_ParseCompressedPureList

Decode, decompress and set pure file list from compressed data
================
*/
void CL_ParseCompressedPureList(void)
{
 int i,esc,sh,shc,bl;
 static unsigned char buf[PURE_COMPRESS_BUFFER];
 int l;
 static char sums[16384];
 static char names[BIG_INFO_STRING*4];
 msg_t msg = {0};

 // Decode 7 bit encoding into 8 bit buffer
 esc = 0;
 shc = 0;
 sh = 0;
 bl = 0;
 for(i=0;(i<8)&&(bl<sizeof(buf));i++) {
  char *s = cl.gameState.stringData + cl.gameState.stringOffsets[ MAX_CONFIGSTRINGS-PURE_COMPRESS_NUMCS+i ];
  while(*s) {
   int v = *(s++);
   if (esc) {
    v--;
    esc=0;
   } else {
    if (v=='@') {
     esc=1;
     continue;
    }
   }
   sh=(sh<<7)|v;
   shc+=7;
   if (shc>=8) {
    shc-=8;
    v = (sh>>shc)&0xFF;
    buf[bl++] = v;
//    fprintf(stderr,"%02X",v);
   }
  }
 }
// fprintf(stderr,"\n");

 Com_Printf("Pure filelist compressed size: %d\n",bl);
 if (bl<2+4+1) {
  Com_Error (ERR_DROP,"CL_ParseCompressedPureList: CS data too short to be valid (%d bytes)\n",bl);
 }

 // Decompress huffman encoded filenames
 msg.maxsize = sizeof(buf);
 msg.cursize = bl;
 msg.data = buf;
 l = (buf[0]|(buf[1]<<8))*4;
 if ((l>msg.maxsize) || (l<4)) Com_Error(ERR_DROP,"Malformed compressed pure filelist size");
 Huff_Decompress(&msg,2+l);

 Com_Printf("Pure filelist (%d files) decompressed size: %d\n",l/4,bl);

// fprintf(stderr,"DECOMPRESSED(%d): ",msg.cursize);
// for(i=0;i<msg.cursize;i++) fprintf(stderr,"%02X",buf[i]);
// fprintf(stderr,"\n");

 // create checksums string
 sums[0]=0;
 sh=0;
 for(i=0;i<l;i+=4) {
  char tmp[16];
  int n;
  unsigned int chs = buf[2+i+0]|(buf[2+i+1]<<8)|(buf[2+i+2]<<16)|(buf[2+i+3]<<24);
  n = sprintf(tmp,"%i ",chs); // Can't use Com_sprintf, it doesn't return anything
  if (sh+n+1>=sizeof(sums)) {
   Com_Error(ERR_DROP,"Checksum buffer overflow");
   break;
  }
  strcpy(sums+sh,tmp);
  sh+=n;
//  Q_strcat( sums, sizeof( sums), va("%i ", chs ) ); // this is fugly
 }
 if (sh && sums[sh-1]==' ') sums[sh-1] = 0; // kill ending space

 // unpack pk3 filenames
 TextDecode6Bit(buf+2+l, msg.cursize-(2+l), names, sizeof(names));

// do simple integrity check, both should have same number of items
 sh=1; // number of spaces = number of names-1
 i=0;
 while(names[i]) { if (names[i]==' ') sh++; i++; }
 Com_DPrintf("FNAMES: %d  CHSUMS: %d\n",sh,l/4);
 if (abs(sh-l/4)>2) Com_Printf("WARNING: Compressed purelist inconsistency!! (FN:%d/CH:%d)\n",sh,l/4);

 if (com_developer->value) {
  // fprintf to stderr needed, these strings are huge and will overflow console buffer
  fprintf(stderr,"FNAMES: \"%s\"\n",names);
  fprintf(stderr,"CHSUMS: \"%s\"\n",sums);
 }
 FS_PureServerSetLoadedPaks( sums, names );
}

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_SystemInfoChanged( void ) {
	char			*systemInfo;
	const char		*s, *t;
	char			key[BIG_INFO_KEY];
	char			value[BIG_INFO_VALUE];
	qboolean		gameSet;

	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SYSTEMINFO ];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.serverId = atoi( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	// don't set any vars when playing a demo
	if ( clc.demoplaying ) {
		return;
	}

	s = Info_ValueForKey( systemInfo, "sv_cheats" );
	cl_connectedToCheatServer = atoi( s );
	if ( !cl_connectedToCheatServer ) {
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey( systemInfo, "sv_paks" );
	t = Info_ValueForKey( systemInfo, "sv_pakNames" );
	if (s[0]=='*' && s[1]==0 && t[0]=='*' && t[1]==0) {
		Com_Printf("Using compressed pure file list\n");
		CL_ParseCompressedPureList();
	} else {
		Com_Printf("Using standard pure file list\n");
		FS_PureServerSetLoadedPaks( s, t );
	}

	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	FS_PureServerSetReferencedPaks( s, t );

	gameSet = qfalse;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while ( s ) {
		int cvar_flags;
		
		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}
		
		// ehw!
		if (!Q_stricmp(key, "fs_game"))
		{
			if(FS_CheckDirTraversal(value))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: Server sent invalid fs_game value %s\n", value);
				continue;
			}
				
			gameSet = qtrue;
		}

		if((cvar_flags = Cvar_Flags(key)) == CVAR_NONEXISTENT)
			Cvar_Get(key, value, CVAR_SERVER_CREATED | CVAR_ROM);
		else
		{
			// If this cvar may not be modified by a server discard the value.
			if(!(cvar_flags & (CVAR_SYSTEMINFO | CVAR_SERVER_CREATED)))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: server is not allowed to set %s=%s\n", key, value);
				continue;
			}

			Cvar_Set(key, value);
		}
	}
	// if game folder should not be set and it is set at the client side
	if ( !gameSet && *Cvar_VariableString("fs_game") ) {
		Cvar_Set( "fs_game", "" );
	}
	cl_connectedToPureServer = Cvar_VariableValue( "sv_pure" );
}

/*
==================
CL_ParseServerInfo
==================
*/
static void CL_ParseServerInfo(void)
{
	const char *serverInfo;

	serverInfo = cl.gameState.stringData
		+ cl.gameState.stringOffsets[ CS_SERVERINFO ];

	clc.sv_allowDownload = atoi(Info_ValueForKey(serverInfo,
		"sv_allowDownload"));
	Q_strncpyz(clc.sv_dlURL,
		Info_ValueForKey(serverInfo, "sv_dlURL"),
		sizeof(clc.sv_dlURL));
	Q_strncpyz(clc.mapname,
    Info_ValueForKey(serverInfo, "mapname"),
		sizeof(clc.mapname));
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate( msg_t *msg ) {
	int				i;
	entityState_t	*es;
	int				newnum;
	entityState_t	nullstate;
	int				cmd;
	char			*s;

	Con_Close();

	clc.connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	cl.gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}
		
		if ( cmd == svc_configstring ) {
			int		len;

			i = MSG_ReadShort( msg );
			if ( i < 0 || i >= MAX_CONFIGSTRINGS ) {
				Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
			}
			s = MSG_ReadBigString( msg );
			len = strlen( s );

			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
				Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
			}

			// append it to the gameState string buffer
			cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "Baseline number out of range: %i", newnum );
			}
			Com_Memset (&nullstate, 0, sizeof(nullstate));
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		} else {
			Com_Error( ERR_DROP, "CL_ParseGamestate: bad command byte" );
		}
	}

	clc.clientNum = MSG_ReadLong(msg);
	// read the checksum feed
	clc.checksumFeed = MSG_ReadLong( msg );

	// parse useful values out of CS_SERVERINFO
	CL_ParseServerInfo();

	// parse serverId and other cvars
	CL_SystemInfoChanged();

	// stop recording now so the demo won't have an unnecessary level load at the end.
	if(cl_autoRecordDemo->integer && clc.demorecording)
		CL_StopRecord_f();
	
	// reinitialize the filesystem if the game directory has changed
	FS_ConditionalRestart( clc.checksumFeed );

	if (dangerousPaksFound) {
		char PakList[MAX_STRING_CHARS];
		for (i = 0; i < dangerousPaksFound; i++) {
			Q_strcat(PakList, sizeof(PakList), va("%s.pk3, ", dangerousPakNames[i]));
		}

		PakList[strlen(PakList) - 2] = 0;

		Cvar_Set("com_errorMessage", va(
			"^1WARNING! ^7Dangerous file(s) found in downloaded pk3%s:\n\n%s\n\n"
			"You should go delete %s immediately. %s could lead to malicious code execution.",
			dangerousPaksFound == 1 ? "" : "s",
			PakList,
			dangerousPaksFound == 1 ? "that file" : "those files",
			dangerousPaksFound == 1 ? "It" : "They"));

		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
		return;
	}

	// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
	// cgame
	CL_InitDownloads();

	// make sure the game starts
	Cvar_Set( "cl_paused", "0" );
}


//=====================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload ( msg_t *msg ) {
	int		size;
	unsigned char data[MAX_MSGLEN];
	uint16_t block;

	if (!*clc.downloadTempName) {
		Com_Printf("Server sending download, but no download was requested\n");
		CL_AddReliableCommand( "stopdl" );
		return;
	}

	// read the data
	block = MSG_ReadShort ( msg );

	if(!block && !clc.downloadBlock)
	{
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong ( msg );

		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if (clc.downloadSize < 0)
		{
			Com_Error(ERR_DROP, MSG_ReadString( msg ) );
			return;
		}
	}

	size = MSG_ReadShort ( msg );
	if (size < 0 || size > sizeof(data))
	{
		Com_Error(ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size);
		return;
	}
	
	MSG_ReadData(msg, data, size);

	if((clc.downloadBlock & 0xFFFF) != block)
	{
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n", (clc.downloadBlock & 0xFFFF), block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_SV_FOpenFileWrite( clc.downloadTempName );

		if (!clc.download) {
			Com_Printf( "Could not create %s\n", clc.downloadTempName );
			CL_AddReliableCommand( "stopdl" );
			CL_NextDownload();
			return;
		}
	}

	if (size)
		FS_Write( data, size, clc.download );

	CL_AddReliableCommand( va("nextdl %d", clc.downloadBlock) );
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );

	if (!size) { // A zero length block means EOF
		if (clc.download) {
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename ( clc.downloadTempName, clc.downloadName, qfalse );
		}
		*clc.downloadTempName = *clc.downloadName = 0;
		Cvar_Set( "cl_downloadName", "" );

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload ();
	}
}

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString( msg_t *msg ) {
	char	*s;
	int		seq;
	int		index;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed stored it off
	if ( clc.serverCommandSequence >= seq ) {
		return;
	}
	clc.serverCommandSequence = seq;

	index = seq & (MAX_RELIABLE_COMMANDS-1);
	Q_strncpyz( clc.serverCommands[ index ], s, sizeof( clc.serverCommands[ index ] ) );
}


/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( msg_t *msg ) {
	int			cmd;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf ("%i ",msg->cursize);
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf ("------------------\n");
	}

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong( msg );
	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	//
	// parse the message
	//
	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			Com_Error (ERR_DROP,"CL_ParseServerMessage: read past end of server message");
			break;
		}

		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF) {
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( !svc_strings[cmd] ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount-1, cmd );
			} else {
				SHOWNET( msg, svc_strings[cmd] );
			}
		}
	
	// other commands
		switch ( cmd ) {
		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;			
		case svc_nop:
			break;
		case svc_serverCommand:
			CL_ParseCommandString( msg );
			break;
		case svc_gamestate:
			CL_ParseGamestate( msg );
			break;
		case svc_snapshot:
			CL_ParseSnapshot( msg );
			break;
		case svc_download:
			CL_ParseDownload( msg );
			break;
		}
	}
}


