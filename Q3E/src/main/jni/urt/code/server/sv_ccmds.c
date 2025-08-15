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
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
SV_GetPlayerByHandle

Returns the player with player id or name from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByHandle(void) {

	char      *s;
	char      name[MAX_NAME_LENGTH];
	int       num = 0;
	int       i, idnum;
	client_t  *cl;
	client_t  *matches[MAX_CLIENTS];

	// Make sure server is running.
	if (!com_sv_running->integer) {
		return NULL;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("No player specified.\n");
		return NULL;
	}

	s = Cmd_Argv(1);

	// Check whether this is a numeric player handle.
	for (i = 0; s[i] >= '0' && s[i] <= '9'; i++);

	if (!s[i]) {

		idnum = atoi(s);
		if ((idnum < 0) || (idnum >= sv_maxclients->integer)) {
			Com_Printf("Bad player slot: %i\n", idnum);
			return NULL;
		}

		cl = &svs.clients[idnum];

		if (!cl->state) {
			Com_Printf("Player %i is not active.\n", idnum);
			return NULL;
		}

		return cl;

	} else {

		for (i = 0; i < sv_maxclients->integer ; i++) {

			cl = &svs.clients[i];

			if (!cl->state) {
				continue;
			}

			Q_strncpyz(name, cl->name, sizeof(name));
			Q_CleanStr(name);

			// Check for exact match
			if (!Q_stricmp(name, s)) {
				matches[0] = &svs.clients[i];
				num = 1;
				break;
			}

			// check for substring match
			if (Q_stristr(name, s)) {
				matches[num] = &svs.clients[i];
				num++;
			}

		}

		if (!num) {
			Com_Printf("No player found matching %s.\n", s);
			return NULL;
		}

		if (num > 1) {
			Com_Printf("Found %d players matching %s:\n", num, s);
			for (i = 0; i < num; i++) {
				cl = matches[i];
				Q_strncpyz(name, cl->name, sizeof(name));
				Q_CleanStr(name);
				Com_Printf(" %2d: [%s]\n", (int)(cl - svs.clients), name);
			}
			return NULL;
		}

		return matches[0];

	}

}

/*
==================
SV_GetPlayerByNum

Returns the player with idnum from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByNum( void ) {

	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "No player specified.\n" );
		return NULL;
	}

	s = Cmd_Argv(1);

	for (i = 0; s[i]; i++) {
		if (s[i] < '0' || s[i] > '9') {
			Com_Printf( "Bad slot number: %s.\n", s);
			return NULL;
		}
	}
	idnum = atoi( s );
	if ( idnum < 0 || idnum >= sv_maxclients->integer ) {
		Com_Printf( "Bad player slot: %i.\n", idnum );
		return NULL;
	}

	cl = &svs.clients[idnum];
	if ( !cl->state ) {
		Com_Printf( "Player %i is not active.\n", idnum );
		return NULL;
	}
	return cl;
}

/*
==================
SV_AlphaSort

Array sorting comparison function (alphabetical sort).
==================
*/
static int QDECL SV_AlphaSort(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **) b);
}

/*
==================
SV_GetMapSoundingLike

Retrieve a full map name given a substring of it.
==================
*/
static void SV_GetMapSoundingLike(char *dest, const char *src, int destsize) {

	int  i;
	int  len = 0;
	int  num = 0;
	int  mapNum;
	char *ptr;
	char *matches[MAX_MAPLIST_SIZE];
	char expanded[MAX_QPATH];
	static char mapList[MAX_MAPLIST_STRING];

	Com_sprintf(expanded, sizeof(expanded), "maps/%s.bsp", src);
	if (FS_FOpenFileRead(expanded, NULL, qfalse) > 0) {
		Q_strncpyz(dest, src, destsize);
		return;
	}

	if (!(mapNum = FS_GetFileList("maps", ".bsp", mapList, sizeof(mapList)))) {
		Com_Printf("Failed to retrieve available maps!\n");
		*dest = 0;
		return;
	}

	ptr = mapList;
	for (i = 0; i < mapNum && num < MAX_MAPLIST_SIZE; i++, ptr += len + 1) {
		len = (int) strlen(ptr);
		COM_StripExtension(ptr, ptr, MAX_QPATH);
		if (Q_stristr(ptr, src)) {
			matches[num] = ptr;
			num++;
		}
	}

	if (!num) {
		Com_Printf("No map found matching %s.\n", src);
		*dest = 0;
		return;
	}

	if (num > 1) {

	qsort(matches, (size_t) num, sizeof(char *), SV_AlphaSort);

		Com_Printf("Found %d maps matching %s:\n", num, src);
		for (i = 0; i < num; i++) {
			Com_Printf(" - [%s]\n", matches[i]);
		}

		if (num > MAX_MAPLIST_SIZE) {
			Com_Printf("...and more.\n");
		}

		*dest = 0;
		return;

	}

	Q_strncpyz(dest, matches[0], destsize);

}


//=========================================================


/*
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f( void ) {

	char		*cmd;
	qboolean	killBots, cheat;
	char		mapname[MAX_QPATH];
	int			sv_cheatMode;

	if (Cmd_Argc() < 2) {
		return;
	}

	SV_GetMapSoundingLike(mapname, Cmd_Argv(1), sizeof(mapname));
	if (!mapname[0]) {
		return;
	}

	// force latched values to get set
	Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH );

	cmd = Cmd_Argv(0);
	if( Q_stricmpn( cmd, "sp", 2 ) == 0 ) {
		Cvar_SetValue( "g_gametype", GT_SINGLE_PLAYER );
		Cvar_SetValue( "g_doWarmup", 0 );
		// may not set sv_maxclients directly, always set latched
		Cvar_SetLatched( "sv_maxclients", "8" );
		cmd += 2;
		if (!Q_stricmp( cmd, "devmap" ) ) {
			cheat = qtrue;
		} else {
			cheat = qfalse;
		}
		killBots = qtrue;
	}
	else {
		if ( !Q_stricmp( cmd, "devmap" ) ) {
			cheat = qtrue;
			killBots = qtrue;
		} else {
			cheat = qfalse;
			killBots = qfalse;
		}
		if( sv_gametype->integer == GT_SINGLE_PLAYER ) {
			Cvar_SetValue( "g_gametype", GT_FFA );
		}
	}

	// start up the map
	SV_SpawnServer( mapname, killBots );

	// set the cheat value
	// If the level was started with "devmap <levelname>"
	// or if sv_cheatMode was set to 1 or 2 then cheats will be allowed.
	// if not, the cheats will not be allowed.

	sv_cheatMode = Cvar_VariableIntegerValue("sv_cheatMode");

	if ( cheat || sv_cheatMode ) {
		Cvar_Set( "sv_cheats", "1" );

		if ( sv_cheatMode != 2 ) {
			Cvar_Set( "sv_cheatMode", "0");
		}
	} else {
		Cvar_Set( "sv_cheats", "0" );
		Cvar_Set( "sv_cheatMode", "0");
	}
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f( void ) {
	int			i;
	client_t	*client;
	char		*denied;
	qboolean	isBot;
	int			delay;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId ) {
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( sv.restartTime ) {
		return;
	}

	if (Cmd_Argc() > 1 ) {
		delay = atoi( Cmd_Argv(1) );
	}
	else {
		delay = 5;
	}
	if( delay && !Cvar_VariableValue("g_doWarmup") ) {
		sv.restartTime = sv.time + delay * 1000;
		SV_SetConfigstring( CS_WARMUP, va("%i", sv.restartTime) );
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified || sv_gametype->modified ) {
		char	mapname[MAX_QPATH];

		Com_Printf( "variable change -- restarting.\n" );
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );

		SV_SpawnServer( mapname, qfalse );
		return;
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid	
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// if a map_restart occurs while a client is changing maps, we need
	// to give them the correct time so that when they finish loading
	// they don't violate the backwards time check in cl_cgame.c
	for (i=0 ; i<sv_maxclients->integer ; i++) {
		if (svs.clients[i].state == CS_PRIMED) {
			svs.clients[i].oldServerTime = sv.restartTime;
		}
	}

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.restarting = qtrue;

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		VM_Call (gvm, GAME_RUN_FRAME, sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	sv.state = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for (i=0 ; i<sv_maxclients->integer ; i++) {
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if ( client->state < CS_CONNECTED) {
			continue;
		}

		if ( client->netchan.remoteAddress.type == NA_BOT ) {
			isBot = qtrue;
		} else {
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = VM_ExplicitArgPtr( gvm, VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );
		if ( denied ) {
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, denied );
			Com_Printf( "SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i );
			continue;
		}

		if(client->state == CS_ACTIVE)
			SV_ClientEnterWorld(client, &client->lastUsercmd);
		else
		{
			// If we don't reset client->lastUsercmd and are restarting during map load,
			// the client will hang because we'll use the last Usercmd from the previous map,
			// which is wrong obviously.
			SV_ClientEnterWorld(client, NULL);
		}
	}	

	// run another frame to allow things to look at all the players
	VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	sv.time += 100;
	svs.time += 100;
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f( void ) {

	int			i;
	client_t	*cl;
	char        *reason = "was kicked";

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ((Cmd_Argc() < 2) || (Cmd_Argc() > 3)) {
		Com_Printf("Usage: kick <player> [<reason>]\n"
				   "	   kick all [<reason>] = kick everyone\n"
				   "	   kick allbots = kick all bots\n");
		return;
	}

	if (Cmd_Argc() == 3) {
		reason = va("was kicked: %s", Cmd_Argv(2));
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		if ( !Q_stricmp(Cmd_Argv(1), "all") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
					continue;
				}
				SV_DropClient( cl, reason );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		else if ( !Q_stricmp(Cmd_Argv(1), "allbots") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type != NA_BOT ) {
					continue;
				}
				SV_DropClient( cl, reason );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		Com_Printf("Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, reason );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
==================
SV_KickBots_f

Kick all bots off of the server
==================
*/
static void SV_KickBots_f( void ) {

	int			i;
	client_t	*cl;
	char        *reason = "was kicked";

	// make sure server is running
	if( !com_sv_running->integer ) {
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() == 2) {
		reason = va("was kicked: %s", Cmd_Argv(1));
	}

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if( !cl->state ) {
			continue;
		}
		if( cl->netchan.remoteAddress.type != NA_BOT ) {
			continue;
		}
		SV_DropClient( cl, reason );
		cl->lastPacketTime = svs.time; // in case there is a funny zombie
	}
}
/*
==================
SV_KickAll_f

Kick all users off of the server
==================
*/
static void SV_KickAll_f( void ) {

	int			i;
	client_t	*cl;
	char        *reason = "was kicked";

	// make sure server is running
	if( !com_sv_running->integer ) {
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() == 2) {
		reason = va("was kicked: %s", Cmd_Argv(1));
	}

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if( !cl->state ) {
			continue;
		}
		if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
			continue;
		}
		SV_DropClient( cl, reason );
		cl->lastPacketTime = svs.time; // in case there is a funny zombie
	}
}

/*
==================
SV_KickNum_f

Kick a user off of the server
==================
*/
static void SV_KickNum_f( void ) {

	client_t	*cl;
	char        *reason = "was kicked";

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ((Cmd_Argc() < 2) || (Cmd_Argc() > 3)) {
		Com_Printf ("Usage: %s <client number> [<reason>]\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 3) {
		reason = va("was kicked: %s", Cmd_Argv(2));
	}

	cl = SV_GetPlayerByNum();
	if ( !cl ) {
		return;
	}

	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		Com_Printf("Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, reason );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
==================
SV_RehashBans_f

Load saved bans from file.
==================
*/
static void SV_RehashBans_f(void)
{
	int index, filelen;
	fileHandle_t readfrom;
	char *textbuf, *curpos, *maskpos, *newlinepos, *endpos;
	char filepath[MAX_QPATH];
	
	// make sure server is running
	if ( !com_sv_running->integer ) {
		return;
	}
	
	serverBansCount = 0;
	
	if(!sv_banFile->string || !*sv_banFile->string)
		return;

	Com_sprintf(filepath, sizeof(filepath), "%s/%s", FS_GetCurrentGameDir(), sv_banFile->string);

	if((filelen = FS_SV_FOpenFileRead(filepath, &readfrom)) >= 0)
	{
		if(filelen < 2)
		{
			// Don't bother if file is too short.
			FS_FCloseFile(readfrom);
			return;
		}

		curpos = textbuf = Z_Malloc(filelen);
		
		filelen = FS_Read(textbuf, filelen, readfrom);
		FS_FCloseFile(readfrom);
		
		endpos = textbuf + filelen;
		
		for(index = 0; index < SERVER_MAXBANS && curpos + 2 < endpos; index++)
		{
			// find the end of the address string
			for(maskpos = curpos + 2; maskpos < endpos && *maskpos != ' '; maskpos++);
			
			if(maskpos + 1 >= endpos)
				break;

			*maskpos = '\0';
			maskpos++;
			
			// find the end of the subnet specifier
			for(newlinepos = maskpos; newlinepos < endpos && *newlinepos != '\n'; newlinepos++);
			
			if(newlinepos >= endpos)
				break;
			
			*newlinepos = '\0';
			
			if(NET_StringToAdr(curpos + 2, &serverBans[index].ip, NA_UNSPEC))
			{
				serverBans[index].isexception = (curpos[0] != '0');
				serverBans[index].subnet = atoi(maskpos);
				
				if(serverBans[index].ip.type == NA_IP &&
				   (serverBans[index].subnet < 1 || serverBans[index].subnet > 32))
				{
					serverBans[index].subnet = 32;
				}
				else if(serverBans[index].ip.type == NA_IP6 &&
					(serverBans[index].subnet < 1 || serverBans[index].subnet > 128))
				{
					serverBans[index].subnet = 128;
				}
			}
			
			curpos = newlinepos + 1;
		}
			
		serverBansCount = index;
		
		Z_Free(textbuf);
	}
}

/*
==================
SV_WriteBans

Save bans to file.
==================
*/
static void SV_WriteBans(void)
{
	int index;
	fileHandle_t writeto;
	char filepath[MAX_QPATH];
	
	if(!sv_banFile->string || !*sv_banFile->string)
		return;
	
	Com_sprintf(filepath, sizeof(filepath), "%s/%s", FS_GetCurrentGameDir(), sv_banFile->string);

	if((writeto = FS_SV_FOpenFileWrite(filepath)))
	{
		char writebuf[128];
		serverBan_t *curban;
		
		for(index = 0; index < serverBansCount; index++)
		{
			curban = &serverBans[index];
			
			Com_sprintf(writebuf, sizeof(writebuf), "%d %s %d\n",
				    curban->isexception, NET_AdrToString(curban->ip), curban->subnet);
			FS_Write(writebuf, strlen(writebuf), writeto);
		}

		FS_FCloseFile(writeto);
	}
}

/*
==================
SV_DelBanEntryFromList

Remove a ban or an exception from the list.
==================
*/

static qboolean SV_DelBanEntryFromList(int index)
{
	if(index == serverBansCount - 1)
		serverBansCount--;
	else if(index < ARRAY_LEN(serverBans) - 1)
	{
		memmove(serverBans + index, serverBans + index + 1, (serverBansCount - index - 1) * sizeof(*serverBans));
		serverBansCount--;
	}
	else
		return qtrue;

	return qfalse;
}

/*
==================
SV_ParseCIDRNotation

Parse a CIDR notation type string and return a netadr_t and suffix by reference
==================
*/

static qboolean SV_ParseCIDRNotation(netadr_t *dest, int *mask, char *adrstr)
{
	char *suffix;
	
	suffix = strchr(adrstr, '/');
	if(suffix)
	{
		*suffix = '\0';
		suffix++;
	}

	if(!NET_StringToAdr(adrstr, dest, NA_UNSPEC))
		return qtrue;

	if(suffix)
	{
		*mask = atoi(suffix);
		
		if(dest->type == NA_IP)
		{
			if(*mask < 1 || *mask > 32)
				*mask = 32;
		}
		else
		{
			if(*mask < 1 || *mask > 128)
				*mask = 128;
		}
	}
	else if(dest->type == NA_IP)
		*mask = 32;
	else
		*mask = 128;
	
	return qfalse;
}

/*
==================
SV_AddBanToList

Ban a user from being able to play on this server based on his ip address.
==================
*/

static void SV_AddBanToList(qboolean isexception)
{
	char *banstring;
	char addy2[NET_ADDRSTRMAXLEN];
	netadr_t ip;
	int index, argc, mask;
	serverBan_t *curban;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	argc = Cmd_Argc();
	
	if(argc < 2 || argc > 3)
	{
		Com_Printf ("Usage: %s (ip[/subnet] | clientnum [subnet])\n", Cmd_Argv(0));
		return;
	}

	if(serverBansCount >= ARRAY_LEN(serverBans))
	{
		Com_Printf ("Error: Maximum number of bans/exceptions exceeded.\n");
		return;
	}

	banstring = Cmd_Argv(1);
	
	if(strchr(banstring, '.') || strchr(banstring, ':'))
	{
		// This is an ip address, not a client num.
		
		if(SV_ParseCIDRNotation(&ip, &mask, banstring))
		{
			Com_Printf("Error: Invalid address %s\n", banstring);
			return;
		}
	}
	else
	{
		client_t *cl;
		
		// client num.
		
		cl = SV_GetPlayerByNum();

		if(!cl)
		{
			Com_Printf("Error: Playernum %s does not exist.\n", Cmd_Argv(1));
			return;
		}
		
		ip = cl->netchan.remoteAddress;
		
		if(argc == 3)
		{
			mask = atoi(Cmd_Argv(2));
			
			if(ip.type == NA_IP)
			{
				if(mask < 1 || mask > 32)
					mask = 32;
			}
			else
			{
				if(mask < 1 || mask > 128)
					mask = 128;
			}
		}
		else
			mask = (ip.type == NA_IP6) ? 128 : 32;
	}

	if(ip.type != NA_IP && ip.type != NA_IP6)
	{
		Com_Printf("Error: Can ban players connected via the internet only.\n");
		return;
	}

	// first check whether a conflicting ban exists that would supersede the new one.
	for(index = 0; index < serverBansCount; index++)
	{
		curban = &serverBans[index];
		
		if(curban->subnet <= mask)
		{
			if((curban->isexception || !isexception) && NET_CompareBaseAdrMask(curban->ip, ip, curban->subnet))
			{
				Q_strncpyz(addy2, NET_AdrToString(ip), sizeof(addy2));
				
				Com_Printf("Error: %s %s/%d supersedes %s %s/%d\n", curban->isexception ? "Exception" : "Ban",
					   NET_AdrToString(curban->ip), curban->subnet,
					   isexception ? "exception" : "ban", addy2, mask);
				return;
			}
		}
		if(curban->subnet >= mask)
		{
			if(!curban->isexception && isexception && NET_CompareBaseAdrMask(curban->ip, ip, mask))
			{
				Q_strncpyz(addy2, NET_AdrToString(curban->ip), sizeof(addy2));
			
				Com_Printf("Error: %s %s/%d supersedes already existing %s %s/%d\n", isexception ? "Exception" : "Ban",
					   NET_AdrToString(ip), mask,
					   curban->isexception ? "exception" : "ban", addy2, curban->subnet);
				return;
			}
		}
	}

	// now delete bans that are superseded by the new one
	index = 0;
	while(index < serverBansCount)
	{
		curban = &serverBans[index];
		
		if(curban->subnet > mask && (!curban->isexception || isexception) && NET_CompareBaseAdrMask(curban->ip, ip, mask))
			SV_DelBanEntryFromList(index);
		else
			index++;
	}

	serverBans[serverBansCount].ip = ip;
	serverBans[serverBansCount].subnet = mask;
	serverBans[serverBansCount].isexception = isexception;
	
	serverBansCount++;
	
	SV_WriteBans();

	Com_Printf("Added %s: %s/%d\n", isexception ? "ban exception" : "ban",
		   NET_AdrToString(ip), mask);
}

/*
==================
SV_DelBanFromList

Remove a ban or an exception from the list.
==================
*/

static void SV_DelBanFromList(qboolean isexception)
{
	int index, count = 0, todel, mask;
	netadr_t ip;
	char *banstring;
	
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}
	
	if(Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: %s (ip[/subnet] | num)\n", Cmd_Argv(0));
		return;
	}

	banstring = Cmd_Argv(1);
	
	if(strchr(banstring, '.') || strchr(banstring, ':'))
	{
		serverBan_t *curban;
		
		if(SV_ParseCIDRNotation(&ip, &mask, banstring))
		{
			Com_Printf("Error: Invalid address %s\n", banstring);
			return;
		}
		
		index = 0;
		
		while(index < serverBansCount)
		{
			curban = &serverBans[index];
			
			if(curban->isexception == isexception		&&
			   curban->subnet >= mask 			&&
			   NET_CompareBaseAdrMask(curban->ip, ip, mask))
			{
				Com_Printf("Deleting %s %s/%d\n",
					   isexception ? "exception" : "ban",
					   NET_AdrToString(curban->ip), curban->subnet);
					   
				SV_DelBanEntryFromList(index);
			}
			else
				index++;
		}
	}
	else
	{
		todel = atoi(Cmd_Argv(1));

		if(todel < 1 || todel > serverBansCount)
		{
			Com_Printf("Error: Invalid ban number given\n");
			return;
		}
	
		for(index = 0; index < serverBansCount; index++)
		{
			if(serverBans[index].isexception == isexception)
			{
				count++;
			
				if(count == todel)
				{
					Com_Printf("Deleting %s %s/%d\n",
					   isexception ? "exception" : "ban",
					   NET_AdrToString(serverBans[index].ip), serverBans[index].subnet);

					SV_DelBanEntryFromList(index);

					break;
				}
			}
		}
	}
	
	SV_WriteBans();
}


/*
==================
SV_ListBans_f

List all bans and exceptions on console
==================
*/

static void SV_ListBans_f(void)
{
	int index, count;
	serverBan_t *ban;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}
	
	// List all bans
	for(index = count = 0; index < serverBansCount; index++)
	{
		ban = &serverBans[index];
		if(!ban->isexception)
		{
			count++;

			Com_Printf("Ban #%d: %s/%d\n", count,
				    NET_AdrToString(ban->ip), ban->subnet);
		}
	}
	// List all exceptions
	for(index = count = 0; index < serverBansCount; index++)
	{
		ban = &serverBans[index];
		if(ban->isexception)
		{
			count++;

			Com_Printf("Except #%d: %s/%d\n", count,
				    NET_AdrToString(ban->ip), ban->subnet);
		}
	}
}

/*
==================
SV_FlushBans_f

Delete all bans and exceptions.
==================
*/

static void SV_FlushBans_f(void)
{
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	serverBansCount = 0;
	
	// empty the ban file.
	SV_WriteBans();
	
	Com_Printf("All bans and exceptions have been deleted.\n");
}

static void SV_BanAddr_f(void)
{
	SV_AddBanToList(qfalse);
}

static void SV_ExceptAddr_f(void)
{
	SV_AddBanToList(qtrue);
}

static void SV_BanDel_f(void)
{
	SV_DelBanFromList(qfalse);
}

static void SV_ExceptDel_f(void)
{
	SV_DelBanFromList(qtrue);
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f(void) {

	int           i, j, l;
	client_t      *cl;
	playerState_t *ps;
	const char    *s;
	int           ping;
	char          name[MAX_NAME_LENGTH];

	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}

	Com_Printf("map: %s\n", sv_mapname->string);
	Com_Printf("num score ping name            lastmsg address               qport rate \n");
	Com_Printf("--- ----- ---- --------------- ------- --------------------- ----- -----\n");

	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {

		if (!cl->state) {
			continue;
		}

		Com_Printf("%3i ", i);
		ps = SV_GameClientNum(i);
		Com_Printf("%5i ", ps->persistant[PERS_SCORE]);

		if (cl->state == CS_CONNECTED) {
			Com_Printf("CNCT ");
		} else if (cl->state == CS_ZOMBIE) {
			Com_Printf("ZMBI ");
		} else {
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf("%4i ", ping);
		}

		// Remove color codes not to break the padding.
		Q_strncpyz(name, cl->name, sizeof(name));
		Q_CleanStr(name);

		Com_Printf("%s", name);

		// TTimo adding a ^7 to reset the color
		Com_Printf("^7");
		l = (int) (16 - strlen(name));
		for (j = 0; j < l; j++) {
			Com_Printf(" ");
		}

		Com_Printf("%7i ", svs.time - cl->lastPacketTime);
		s = NET_AdrToStringwPort(cl->netchan.remoteAddress);
		Com_Printf("%s", s);
		l = (int) (22 - strlen(s));
		for (j = 0; j < l; j++) {
			Com_Printf(" ");
		}

		Com_Printf("%5i", cl->netchan.qport);
		Com_Printf(" %5i", cl->rate);
		Com_Printf("\n");
	}

	Com_Printf("\n");

}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f(void) {
	char	*p;
	char	text[1024];

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc () < 2 ) {
		return;
	}

	strcpy (text, sv_sayprefix->string);
	p = Cmd_Args();

	if ( *p == '"' ) {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	Com_Printf("%s\n", text);
	SV_SendServerCommand(NULL, "chat \"%s\"", text);
}

/*
==================
SV_ConTell_f
==================
*/
static void SV_ConTell_f(void) {

	char	*p;
	char	text[1024];
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() < 3 ) {
		Com_Printf ("Usage: tell <player> <text>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		return;
	}

	strcpy (text, sv_tellprefix->string);
	p = Cmd_ArgsFrom(2);

	if ( *p == '"' ) {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	Com_Printf("%s\n", text);
	SV_SendServerCommand(cl, "chat \"%s\"", text);
}


/*
==================
	Com_Printf("%s\n", text);
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f( void ) {
	svs.nextHeartbeatTime = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f( void ) {
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	Com_Printf ("Server info settings:\n");
	Info_Print ( Cvar_InfoString( CVAR_SERVERINFO ) );
}


/*
===========
SV_Systeminfo_f

Examine the systeminfo string
===========
*/
static void SV_Systeminfo_f( void ) {
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	Com_Printf ("System info settings:\n");
	Info_Print ( Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
static void SV_DumpUser_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: dumpuser <player>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		return;
	}

	Com_Printf( "userinfo\n" );
	Com_Printf( "--------\n" );
	Info_Print( cl->userinfo );
}


/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f( void ) {
	SV_Shutdown( "killserver" );
}

//===========================================================

/*
Start a server-side demo.

This does it all, create the file and adjust the demo-related
stuff in client_t.

This is mostly ripped from sv_client.c/SV_SendClientGameState
and cl_main.c/CL_Record_f.
*/
static void SVD_StartDemoFile(client_t *client, const char *path) {

    int             i, len;
    entityState_t   *base, nullstate;
    msg_t           msg;
    byte            buffer[MAX_MSGLEN];
    fileHandle_t    file;
#ifdef USE_DEMO_FORMAT_42
    char            *s;
    int             v, size;
#endif

    Com_DPrintf("SVD_StartDemoFile\n");
    assert(!client->demo_recording);

    // create the demo file and write the necessary header
    file = FS_FOpenFileWrite(path);
    assert(file != 0);

    /* File_write_header_demo // ADD this fx */
    /* HOLBLIN  entete demo */
#ifdef USE_DEMO_FORMAT_42
    //@Barbatos: get the mod version from the server
    s = Cvar_VariableString("g_modversion");

    size = strlen(s);
    len = LittleLong(size);
    FS_Write(&len, 4, file);
    FS_Write(s, size, file);

    v = LittleLong(DEMO_VERSION);
    FS_Write (&v, 4, file);

    len = 0;
    len = LittleLong(len);
    FS_Write(&len, 4, file);
    FS_Write(&len, 4, file);
#endif
    /* END HOLBLIN  entete demo */

    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_Bitstream(&msg); // XXX server code doesn't do this, client code does
    MSG_WriteLong(&msg, client->lastClientCommand); // TODO: or is it client->reliableSequence?
    MSG_WriteByte(&msg, svc_gamestate);
    MSG_WriteLong(&msg, client->reliableSequence);

    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if (sv.configstrings[i][0]) {
            MSG_WriteByte(&msg, svc_configstring);
            MSG_WriteShort(&msg, i);
            MSG_WriteBigString(&msg, sv.configstrings[i]);
        }
    }

    Com_Memset(&nullstate, 0, sizeof(nullstate));
    for (i = 0 ; i < MAX_GENTITIES; i++) {
        base = &sv.svEntities[i].baseline;
        if (!base->number) {
            continue;
        }
        MSG_WriteByte(&msg, svc_baseline);
        MSG_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
    }

    MSG_WriteByte(&msg, svc_EOF);
    MSG_WriteLong(&msg, client - svs.clients);
    MSG_WriteLong(&msg, sv.checksumFeed);
    MSG_WriteByte(&msg, svc_EOF); // XXX server code doesn't do this, SV_Netchan_Transmit adds it!

    len = LittleLong(client->netchan.outgoingSequence - 1);
    FS_Write(&len, 4, file);

    len = LittleLong (msg.cursize);
    FS_Write(&len, 4, file);
    FS_Write(msg.data, msg.cursize, file);

#ifdef USE_DEMO_FORMAT_42
    // add size of packet in the end for backward play /* holblin */
    FS_Write(&len, 4, file);
#endif

    FS_Flush(file);

    // adjust client_t to reflect demo started
    client->demo_recording = qtrue;
    client->demo_file = file;
    client->demo_waiting = qtrue;
    client->demo_backoff = 1;
    client->demo_deltas = 0;
}

/*
Write a message to a server-side demo file.
*/
void SVD_WriteDemoFile(const client_t *client, const msg_t *msg) {

    int len;
    msg_t cmsg;
    byte cbuf[MAX_MSGLEN];
    fileHandle_t file = client->demo_file;

    if (*(int *)msg->data == -1) { // TODO: do we need this?
        Com_DPrintf("Ignored connectionless packet, not written to demo!\n");
        return;
    }

    // TODO: we only copy because we want to add svc_EOF; can we add it and then
    // "back off" from it, thus avoiding the copy?
    MSG_Copy(&cmsg, cbuf, sizeof(cbuf), (msg_t*) msg);
    MSG_WriteByte(&cmsg, svc_EOF); // XXX server code doesn't do this, SV_Netchan_Transmit adds it!

    // TODO: the headerbytes stuff done in the client seems unnecessary
    // here because we get the packet *before* the netchan has it's way
    // with it; just not sure that's really true :-/

    len = LittleLong(client->netchan.outgoingSequence);
    FS_Write(&len, 4, file);

    len = LittleLong(cmsg.cursize);
    FS_Write(&len, 4, file);

    FS_Write(cmsg.data, cmsg.cursize, file); // XXX don't use len!

#ifdef USE_DEMO_FORMAT_42
    // add size of packet in the end for backward play /* holblin */
    FS_Write(&len, 4, file);
#endif

    FS_Flush(file);
}

/*
Stop a server-side demo.

This finishes out the file and clears the demo-related stuff
in client_t again.
*/
static void SVD_StopDemoFile(client_t *client) {

    int marker = -1;
    fileHandle_t file = client->demo_file;

    Com_DPrintf("SVD_StopDemoFile\n");
    assert(client->demo_recording);

    // write the necessary trailer and close the demo file
    FS_Write(&marker, 4, file);
    FS_Write(&marker, 4, file);
    FS_Flush(file);
    FS_FCloseFile(file);

    // adjust client_t to reflect demo stopped
    client->demo_recording = qfalse;
    client->demo_file = -1;
    client->demo_waiting = qfalse;
    client->demo_backoff = 1;
    client->demo_deltas = 0;
}

/*
Clean up player name to be suitable as path name.
Similar to Q_CleanStr() but tweaked.
*/
static void SVD_CleanPlayerName(char *name) {

    char *src = name, *dst = name, c;

    while ((c = *src)) {
        // note Q_IsColorString(src++) won't work since it's a macro
        if (Q_IsColorString(src)) {
            src++;
        } else if (c == ':' || c == '\\' || c == '/' || c == '*' || c == '?') {
            *dst++ = '%';
        } else if (c > ' ' && c < 0x7f) {
            *dst++ = c;
        }
        src++;
    }

    *dst = '\0';

    if (strlen(name) == 0) {
        strcpy(name, "UnnamedPlayer");
    }
}

/*
Generate unique name for a new server demo file.
(We pretend there are no race conditions.)
*/
static void SV_NameServerDemo(char *filename, int length, const client_t *client, char *fn) {

    qtime_t time;
    char playername[32];
    char demoName[64]; //@Barbatos

    Com_DPrintf("SV_NameServerDemo\n");

    Com_RealTime(&time);
    Q_strncpyz(playername, client->name, sizeof(playername));
    SVD_CleanPlayerName(playername);

    if (fn != NULL) {

        Q_strncpyz(demoName, fn, sizeof(demoName));

#ifdef USE_DEMO_FORMAT_42
        Com_sprintf(filename, length-1, "%s/%s.urtdemo", sv_demofolder->string, demoName );
        if (FS_FileExists(filename)) {
            Com_sprintf(filename, length-1, "%s/%s_%d.urtdemo", sv_demofolder->string, demoName, Sys_Milliseconds() );
        }
#else
        Com_sprintf(filename, length-1, "%s/%s.dm_%d", sv_demofolder->string, demoName , PROTOCOL_VERSION );
        if (FS_FileExists(filename)) {
            Com_sprintf(filename, length-1, "%s/%s_%d.dm_%d", sv_demofolder->string, demoName, Sys_Milliseconds() , PROTOCOL_VERSION );
        }
#endif
    } else {
#ifdef USE_DEMO_FORMAT_42
        Com_sprintf(
            filename, length-1, "%s/%.4d-%.2d-%.2d_%.2d-%.2d-%.2d_%s_%d.urtdemo",
            sv_demofolder->string, time.tm_year+1900, time.tm_mon + 1, time.tm_mday,
            time.tm_hour, time.tm_min, time.tm_sec,
            playername,
            Sys_Milliseconds()
        );
#else
        Com_sprintf(
            filename, length-1, "%s/%.4d-%.2d-%.2d_%.2d-%.2d-%.2d_%s_%d.dm_%d",
            sv_demofolder->string, time.tm_year+1900, time.tm_mon + 1, time.tm_mday,
            time.tm_hour, time.tm_min, time.tm_sec,
            playername,
            Sys_Milliseconds(),
            PROTOCOL_VERSION
        );
#endif
        filename[length-1] = '\0';

        if (FS_FileExists(filename)) {
            filename[0] = 0;
            return;
        }
    }
}

void SV_StartRecordOne(client_t *client, char *filename) {

    char path[MAX_OSPATH];

    Com_DPrintf("SV_StartRecordOne\n");

    if (client->demo_recording) {
        Com_Printf("startserverdemo: %s is already being recorded\n", client->name);
        return;
    }

    if (client->state != CS_ACTIVE) {
        Com_Printf("startserverdemo: %s is not active\n", client->name);
        return;
    }

    if (client->netchan.remoteAddress.type == NA_BOT) {
        Com_Printf("startserverdemo: %s is a bot\n", client->name);
        return;
    }

    SV_NameServerDemo(path, sizeof(path), client, filename);
    SVD_StartDemoFile(client, path);

    if(sv_demonotice->string) {
        SV_SendServerCommand(client, "print \"%s\"\n", sv_demonotice->string);
    }

    Com_Printf("startserverdemo: recording %s to %s\n", client->name, path);
}

static void SV_StartRecordAll(void) {

    int slot;
    client_t *client;

    Com_DPrintf("SV_StartRecordAll\n");

    for (slot = 0, client = svs.clients; slot < sv_maxclients->integer; slot++, client++) {
        // filter here to avoid lots of bogus messages from SV_StartRecordOne()
        if (client->netchan.remoteAddress.type == NA_BOT
            || client->state != CS_ACTIVE
            || client->demo_recording) {
            continue;
        }
        SV_StartRecordOne(client, NULL);
    }
}

static void SV_StopRecordOne(client_t *client) {

    Com_DPrintf("SV_StopRecordOne\n");

    if (!client->demo_recording) {
        Com_Printf("stopserverdemo: %s is not being recorded\n", client->name);
        return;
    }

    if (client->state != CS_ACTIVE) { // disconnects are handled elsewhere
        Com_Printf("stopserverdemo: %s is not active\n", client->name);
        return;
    }

    if (client->netchan.remoteAddress.type == NA_BOT) {
        Com_Printf("stopserverdemo: %s is a bot\n", client->name);
        return;
    }

    SVD_StopDemoFile(client);
    Com_Printf("stopserverdemo: stopped recording %s\n", client->name);


}

static void SV_StopRecordAll(void) {

    int slot;
    client_t *client;

    Com_DPrintf("SV_StopRecordAll\n");

    for (slot=0, client=svs.clients; slot < sv_maxclients->integer; slot++, client++) {
        // filter here to avoid lots of bogus messages from SV_StopRecordOne()
        if (client->netchan.remoteAddress.type == NA_BOT
            || client->state != CS_ACTIVE // disconnects are handled elsewhere
            || !client->demo_recording) {
            continue;
        }
        SV_StopRecordOne(client);
    }

}

/*
==================
SV_StartServerDemo_f

Record a server-side demo for given player/slot. The demo
will be called "YYYY-MM-DD_hh-mm-ss_playername_id.urtdemo",
in the "demos" directory under your game directory. Note
that "startserverdemo all" will start demos for all players
currently in the server. Players who join later require a
new "startserverdemo" command. If you are already recording
demos for some players, another "startserverdemo all" will
start new demos only for players not already recording. Note
that bots will never be recorded, not even if "all" is given.
The server-side demos will stop when "stopserverdemo" is issued
or when the server restarts for any reason (such as a new map
loading).
==================
*/
static void SV_StartServerDemo_f(void) {

    client_t *client;

    Com_DPrintf("SV_StartServerDemo_f\n");

    if (!com_sv_running->integer) {
        Com_Printf("startserverdemo: Server not running\n");
        return;
    }

    if (Cmd_Argc() < 2) {
        Com_Printf("Usage: startserverdemo <client-or-all> [<optional-demo-name>]\n");
        return;
    }

    if (!Q_stricmp(Cmd_Argv(1), "all")) {

        SV_StartRecordAll();

    } else {

        client = SV_GetPlayerByHandle();
        if (!client) {
            return;
        }

        if (Cmd_Argc() > 2) {
            SV_StartRecordOne(client, Cmd_ArgsFrom(2));
        } else {
            SV_StartRecordOne(client, NULL);
        }

    }
}

/*
==================
SV_StopServerDemo_f

Stop a server-side demo for given player/slot. Note that
"stopserverdemo all" will stop demos for all players in
the server.
==================
*/
static void SV_StopServerDemo_f(void)
{
	client_t *client;

	Com_DPrintf("SV_StopServerDemo_f\n");

	if (!com_sv_running->integer) {
		Com_Printf("stopserverdemo: Server not running\n");
		return;
	}

	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: stopserverdemo <client-or-all>\n");
		return;
	}

	if (!Q_stricmp(Cmd_Argv(1), "all")) {

		SV_StopRecordAll();
	}
	else {

	    client = SV_GetPlayerByHandle();

	    if (!client) {
	        return;
	    }

		SV_StopRecordOne(client);
	}

}

/*
==================
SV_CompleteMapName
==================
*/
static void SV_CompleteMapName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "maps", "bsp", qtrue, qtrue, qtrue );
	}
}

/*
==================
SV_CompletePlayerName
==================
*/
static void SV_CompletePlayerName( char *args, int argNum ) {
	if( argNum == 2 ) {
		char		names[MAX_CLIENTS][MAX_NAME_LENGTH];
		const char	*namesPtr[MAX_CLIENTS];
		client_t	*cl;
		int			i;
		int			nameCount;
		int			clientCount;

		nameCount = 0;
		clientCount = sv_maxclients->integer;

		for ( i=0, cl=svs.clients ; i < clientCount; i++,cl++ ) {
			if ( !cl->state ) {
				continue;
			}
			if( i >= MAX_CLIENTS ) {
				break;
			}
			Q_strncpyz( names[nameCount], cl->name, sizeof(names[nameCount]) );
			Q_CleanStr( names[nameCount] );

			namesPtr[nameCount] = names[nameCount];

			nameCount++;
		}
		qsort( (void*)namesPtr, nameCount, sizeof( namesPtr[0] ), Com_strCompare );

		Field_CompletePlayerName( namesPtr, nameCount );
	}
}

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands( void ) {
	static qboolean	initialized;

	if ( initialized ) {
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("kickbots", SV_KickBots_f);
	Cmd_AddCommand ("kickall", SV_KickAll_f);
	Cmd_AddCommand ("kicknum", SV_KickNum_f);
	Cmd_AddCommand ("clientkick", SV_KickNum_f); // Legacy command
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("systeminfo", SV_Systeminfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);
	Cmd_AddCommand ("map_restart", SV_MapRestart_f);
	Cmd_AddCommand ("sectorlist", SV_SectorList_f);
	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "map", SV_CompleteMapName );
#ifndef PRE_RELEASE_DEMO
	Cmd_AddCommand ("devmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "devmap", SV_CompleteMapName );
	Cmd_AddCommand ("spmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "spmap", SV_CompleteMapName );
	Cmd_AddCommand ("spdevmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "spdevmap", SV_CompleteMapName );
#endif
	Cmd_AddCommand ("killserver", SV_KillServer_f);
	if( com_dedicated->integer ) {
		Cmd_AddCommand ("say", SV_ConSay_f);
		Cmd_AddCommand ("tell", SV_ConTell_f);
		Cmd_SetCommandCompletionFunc( "tell", SV_CompletePlayerName );
		Cmd_AddCommand ("sayto", SV_ConTell_f);
		Cmd_SetCommandCompletionFunc( "sayto", SV_CompletePlayerName );
		Cmd_AddCommand("startserverdemo", SV_StartServerDemo_f);
		Cmd_AddCommand("stopserverdemo", SV_StopServerDemo_f);
	}
	
	Cmd_AddCommand("rehashbans", SV_RehashBans_f);
	Cmd_AddCommand("listbans", SV_ListBans_f);
	Cmd_AddCommand("banaddr", SV_BanAddr_f);
	Cmd_AddCommand("exceptaddr", SV_ExceptAddr_f);
	Cmd_AddCommand("bandel", SV_BanDel_f);
	Cmd_AddCommand("exceptdel", SV_ExceptDel_f);
	Cmd_AddCommand("flushbans", SV_FlushBans_f);
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands( void ) {
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand ("heartbeat");
	Cmd_RemoveCommand ("kick");
	Cmd_RemoveCommand ("kicknum");
	Cmd_RemoveCommand ("clientkick");
	Cmd_RemoveCommand ("kickall");
	Cmd_RemoveCommand ("kickbots");
	Cmd_RemoveCommand ("status");
	Cmd_RemoveCommand ("serverinfo");
	Cmd_RemoveCommand ("systeminfo");
	Cmd_RemoveCommand ("dumpuser");
	Cmd_RemoveCommand ("map_restart");
	Cmd_RemoveCommand ("sectorlist");
	Cmd_RemoveCommand ("say");
	Cmd_RemoveCommand ("startserverdemo");
	Cmd_RemoveCommand ("stopserverdemo");
#endif
}

