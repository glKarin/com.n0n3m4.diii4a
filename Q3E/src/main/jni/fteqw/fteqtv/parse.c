/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "qtv.h"

#include "bsd_string.h"

#define ParseError(m) (m)->readpos = (m)->cursize+1	//

static const entity_state_t null_entity_state;

void SendBufferToViewer(viewer_t *v, const char *buffer, int length, qboolean reliable)
{
	if (reliable)
	{
		//try and put it in the normal reliable
		if (!v->backbuffered && v->netchan.message.cursize+length < v->netchan.message.maxsize)
			WriteData(&v->netchan.message, buffer, length);
		else if (v->backbuffered>0 && v->backbuf[v->backbuffered-1].cursize+length < v->backbuf[v->backbuffered-1].maxsize)	//try and put it in the current backbuffer
			WriteData(&v->backbuf[v->backbuffered-1], buffer, length);
		else if (v->backbuffered == MAX_BACK_BUFFERS)
		{
			v->netchan.message.cursize = 0;
			WriteByte(&v->netchan.message, svc_print);
			if (!v->netchan.isnqprotocol)
				WriteByte(&v->netchan.message, PRINT_HIGH);
			WriteString(&v->netchan.message, "backbuffer overflow\n");
			if (!v->drop)
				Sys_Printf(NULL, "%s backbuffers overflowed\n", v->name);	//FIXME
			v->drop = true;	//we would need too many backbuffers.
		}
		else
		{

			//create a new backbuffer
			if (!v->backbuf[v->backbuffered].data)
			{
				InitNetMsg(&v->backbuf[v->backbuffered], (unsigned char *)malloc(MAX_BACKBUF_SIZE), MAX_BACKBUF_SIZE);
			}
			v->backbuf[v->backbuffered].cursize = 0;	//make sure it's empty
			WriteData(&v->backbuf[v->backbuffered], buffer, length);
			v->backbuffered++;
		}
	}
}

void Multicast(sv_t *tv, void *buffer, int length, int to, unsigned int playermask, int suitablefor)
{
	viewer_t *v;
	switch(to)
	{
	case dem_multiple:
	case dem_single:
	case dem_stats:
		//check and send to them only if they're tracking this player(s).
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (v->thinksitsconnected||suitablefor&CONNECTING)
				if (v->server == tv)
					if (v->trackplayer>=0)
						if ((1<<v->trackplayer)&playermask)
						{
							if (suitablefor&(v->netchan.isnqprotocol?NQ:QW))
								SendBufferToViewer(v, buffer, length, true);	//FIXME: change the reliable depending on message type
						}
		}
		break;
	default:
		//send to all
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (v->thinksitsconnected||suitablefor&CONNECTING)
				if (v->server == tv)
					if (suitablefor&(v->netchan.isnqprotocol?NQ:QW))
						SendBufferToViewer(v, buffer, length, true);	//FIXME: change the reliable depending on message type
		}
		break;
	}
}
void Broadcast(cluster_t *cluster, void *buffer, int length, int suitablefor)
{
	viewer_t *v;
	for (v = cluster->viewers; v; v = v->next)
	{
		if (suitablefor&(v->netchan.isnqprotocol?NQ:QW))
			SendBufferToViewer(v, buffer, length, true);
	}
}

void ConnectionData(sv_t *tv, void *buffer, int length, int to, unsigned int playermask, int suitablefor)
{
	if (!tv->parsingconnectiondata)
		Multicast(tv, buffer, length, to, playermask, suitablefor);
	else if (tv->controller)
	{
		if (suitablefor&(tv->controller->netchan.isnqprotocol?NQ:QW))
			SendBufferToViewer(tv->controller, buffer, length, true);
	}
}

static void ParseServerData(sv_t *tv, netmsg_t *m, int to, unsigned int playermask)
{
	unsigned int protocol;
	unsigned int supported;
	viewer_t *v;

	//free the old map state
	QTV_CleanupMap(tv);

	tv->pext1 = 0;
	tv->pext2 = 0;
	tv->pexte = 0;

	//when it comes to QTV, the proxy 'blindly' forwards the data after parsing the header, so we need to support EVERYTHING the original server might.
	//and if we don't, then we might have troubles.
	for(;;)
	{
		protocol = ReadLong(m);
		switch (protocol)
		{
		case PROTOCOL_VERSION:
			break;
		case PROTOCOL_VERSION_FTE:
			protocol = ReadLong(m);
			tv->pext1 = protocol;

			//HAVE
			supported = PEXT_SETVIEW|PEXT_ACCURATETIMINGS; /*simple forwarding*/
			supported |= PEXT_256PACKETENTITIES|PEXT_VIEW2|PEXT_HLBSP|PEXT_Q2BSP|PEXT_Q3BSP;	//features other than the protocol (stats, simple limits etc)

			supported |= PEXT_FLOATCOORDS|PEXT_SPAWNSTATIC2;	//working
//			supported |= PEXT_CHUNKEDDOWNLOADS;					//shouldn't be relevant...
			supported |= PEXT_TRANS|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_SOUNDDBL;

			//replaced by replacementdeltas.
			supported |= PEXT_SCALE|PEXT_TRANS|PEXT_FATNESS|PEXT_COLOURMOD|PEXT_HEXEN2|PEXT_SETATTACHMENT|PEXT_DPFLAGS;

			//stuff that we ought to handle, but don't currently
			//PEXT_LIGHTSTYLECOL	- woo, fancy rgb colours
			//PEXT_CUSTOMTEMPEFFECTS - required for hexen2's effects. kinda messy.
			//PEXT_TE_BULLET		- implies nq tents too.

			//HARD...
			//PEXT_CSQC -- all bets are off if we receive a csqc ent update

			//totally optional... so will probably never be added...
			//PEXT_HULLSIZE			- bigger players... maybe. like anyone can depend on this... not supported with mvd players so w/e
			//PEXT_CHUNKEDDOWNLOADS	- not sure there's much point
			//PEXT_SPLITSCREEN		- irrelevant for mvds. might be useful as a qw client, but who cares. not enough servers have it active.
			//PEXT_SHOWPIC			- rare, lame, limited. just yuck.

			if (protocol & ~supported)
			{
				int i;
				const char *names[] = {
					"PEXT_SETVIEW",				"PEXT_SCALE",			"PEXT_LIGHTSTYLECOL",	"PEXT_TRANS",
					"PEXT_VIEW2",				"0x00000020",			"PEXT_ACCURATETIMINGS", "PEXT_SOUNDDBL",
					"PEXT_FATNESS",				"PEXT_HLBSP",			"PEXT_TE_BULLET",		"PEXT_HULLSIZE",
					"PEXT_MODELDBL",			"PEXT_ENTITYDBL",		"PEXT_ENTITYDBL2",		"PEXT_FLOATCOORDS",
					"0x00010000",				"PEXT_Q2BSP",			"PEXT_Q3BSP",			"PEXT_COLOURMOD",
					"PEXT_SPLITSCREEN",			"PEXT_HEXEN2",			"PEXT_SPAWNSTATIC2",	"PEXT_CUSTOMTEMPEFFECTS",
					"PEXT_256PACKETENTITIES",	"0x02000000",			"PEXT_SHOWPIC",			"PEXT_SETATTACHMENT",
					"0x10000000",				"PEXT_CHUNKEDDOWNLOADS","PEXT_CSQC",			"PEXT_DPFLAGS",
				};
				for (i = 0; i < sizeof(names)/sizeof(names[0]); i++)
				{
					if (protocol & ~supported & (1u<<i))
					{
						Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_FTE (%s) not supported\n", names[i]);
						supported |= (1u<<i);
					}
				}
				if (protocol & ~supported)
					Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_FTE (%x) not supported\n", protocol & ~supported);
			}
			continue;
		case PROTOCOL_VERSION_FTE2:
			protocol = ReadLong(m);
			tv->pext2 = protocol;
			supported = 0;
//			supported |= PEXT2_PRYDONCURSOR|PEXT2_VOICECHAT|PEXT2_SETANGLEDELTA|PEXT2_REPLACEMENTDELTAS|PEXT2_MAXPLAYERS;

			//FIXME: handle the svc and clc if they arrive.
			supported |= PEXT2_VOICECHAT;

			//WANT
			//PEXT2_SETANGLEDELTA
			//PEXT2_REPLACEMENTDELTAS
			//PEXT2_SETANGLEDELTA
			//PEXT2_PREDINFO
			//PEXT2_PRYDONCURSOR

			if (protocol & ~supported)
				Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_FTE2 (%x) not supported\n", protocol & ~supported);
			continue;
		case PROTOCOL_VERSION_EZQUAKE1:
			tv->pexte = protocol = ReadLong(m);
			supported = PEXTE_HIDDENMESSAGES;
			if (protocol & ~supported)
				Sys_Printf(tv->cluster, "ParseMessage: Unsupported MVD1 protocol flags %#x\n", protocol);
			continue;
		case PROTOCOL_VERSION_HUFFMAN:
			Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_HUFFMAN not supported\n");
			ParseError(m);
			return;
		case PROTOCOL_VERSION_VARLENGTH:
			{
				int len = ReadLong(m);
				if (len < 0 || len > 8192)
				{
					Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_VARLENGTH invalid\n");
					ParseError(m);
					return;
				}
				protocol = ReadLong(m);/*ident*/
				switch(protocol)
				{
				default:
					m->readpos += len;

					Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_VARLENGTH (%x) not supported\n", protocol);
					ParseError(m);
					return;
				}
			}
			continue;
		case PROTOCOL_VERSION_FRAGMENT:
			protocol = ReadLong(m);
			Sys_Printf(tv->cluster, "ParseMessage: PROTOCOL_VERSION_FRAGMENT not supported\n");
			ParseError(m);
			return;
		default:
			Sys_Printf(tv->cluster, "ParseMessage: Unknown protocol version %#x\n", protocol);
			ParseError(m);
			return;
		}
		break;
	}

	tv->mapstarttime = tv->parsetime;
	tv->parsingconnectiondata = true;

	tv->clservercount = ReadLong(m);	//we don't care about server's servercount, it's all reliable data anyway.

	tv->map.trackplayer = -1;

	ReadString(m, tv->map.gamedir, sizeof(tv->map.gamedir));
#define DEFAULTGAMEDIR "qw"
	if (strchr(tv->map.gamedir, ':'))	//nuke any multiple gamedirs - we need to read maps which would fail if its not a valid single path.
		*strchr(tv->map.gamedir, ';') = 0;
	if (!*tv->map.gamedir)
		strcpy(tv->map.gamedir, DEFAULTGAMEDIR);
	if (!*tv->map.gamedir
		|| *tv->map.gamedir == '.'
		|| !strcmp(tv->map.gamedir, ".")
		|| strstr(tv->map.gamedir, "..")
		|| strstr(tv->map.gamedir, "/")
		|| strstr(tv->map.gamedir, "\\")
		|| strstr(tv->map.gamedir, ":")
		)
	{
		QTV_Printf(tv, "Ignoring unsafe gamedir: \"%s\"\n", tv->map.gamedir);
		strcpy(tv->map.gamedir, DEFAULTGAMEDIR);
	}

	if (tv->usequakeworldprotocols)
		tv->map.thisplayer = ReadByte(m)&~128;
	else
	{
		tv->map.thisplayer = MAX_CLIENTS-1;
		/*tv->servertime =*/ ReadFloat(m);
	}
	if (tv->controller)
		tv->controller->thisplayer = tv->map.thisplayer;
	ReadString(m, tv->map.mapname, sizeof(tv->map.mapname));

	QTV_Printf(tv, "Gamedir: %s\n", tv->map.gamedir);
	QTV_Printf(tv, "---------------------\n");
	Sys_Printf(tv->cluster, "Stream %i: %s\n", tv->streamid, tv->map.mapname);
	QTV_Printf(tv, "---------------------\n");

	// get the movevars
	tv->map.movevars.gravity			= ReadFloat(m);
	tv->map.movevars.stopspeed			= ReadFloat(m);
	tv->map.movevars.maxspeed			= ReadFloat(m);
	tv->map.movevars.spectatormaxspeed	= ReadFloat(m);
	tv->map.movevars.accelerate			= ReadFloat(m);
	tv->map.movevars.airaccelerate		= ReadFloat(m);
	tv->map.movevars.wateraccelerate	= ReadFloat(m);
	tv->map.movevars.friction			= ReadFloat(m);
	tv->map.movevars.waterfriction		= ReadFloat(m);
	tv->map.movevars.entgrav			= ReadFloat(m);

	for (v = tv->cluster->viewers; v; v = v->next)
	{
		if (v->server == tv)
			v->thinksitsconnected = false;
	}

	if ((!tv->controller || tv->controller->netchan.isnqprotocol) && tv->usequakeworldprotocols)
	{
		tv->netchan.message.cursize = 0;	//mvdsv sucks
		SendClientCommand(tv, "soundlist %i 0\n", tv->clservercount);
	}
	else
		ConnectionData(tv, (void*)((char*)m->data+m->startpos), m->readpos - m->startpos, to, dem_read, QW);

	if (tv->controller)
	{
		QW_ClearViewerState(tv->controller);
		tv->controller->trackplayer = tv->map.thisplayer;
	}

	strcpy(tv->status, "Receiving soundlist\n");
}

/*called if the server changed the map.serverinfo, so we can corrupt it again*/
void QTV_UpdatedServerInfo(sv_t *tv)
{
	qboolean fromproxy;
	char text[1024];
	char value[256];

	Info_ValueForKey(tv->map.serverinfo, "*qtv", value, sizeof(value));
	if (*value)
	{
		fromproxy = true;
		tv->serverisproxy = fromproxy;
	}
	else
		fromproxy = false;

	//add on our extra infos
	Info_SetValueForStarKey(tv->map.serverinfo, "*qtv", QTV_VERSION_STRING, sizeof(tv->map.serverinfo));
	Info_SetValueForStarKey(tv->map.serverinfo, "*z_ext", Z_EXT_STRING, sizeof(tv->map.serverinfo));

	Info_ValueForKey(tv->map.serverinfo, "hostname", tv->map.hostname, sizeof(tv->map.hostname));

	//change the hostname (the qtv's hostname with the server's hostname in brackets)
	Info_ValueForKey(tv->map.serverinfo, "hostname", value, sizeof(value));
	if (fromproxy && strchr(value, '(') && value[strlen(value)-1] == ')')	//already has brackets
	{	//the fromproxy check is because it's fairly common to find a qw server with brackets after it's name.
		char *s;
		s = strchr(value, '(');	//so strip the parent proxy's hostname, and put our hostname first, leaving the origional server's hostname within the brackets
		snprintf(text, sizeof(text), "%s %s", tv->cluster->hostname, s);
	}
	else
	{
		if (tv->sourcefile)
			snprintf(text, sizeof(text), "%s (recorded from: %s)", tv->cluster->hostname, value);
		else
			snprintf(text, sizeof(text), "%s (live: %s)", tv->cluster->hostname, value);
	}
	Info_SetValueForStarKey(tv->map.serverinfo, "hostname", text, sizeof(tv->map.serverinfo));
}

static void ParseCDTrack(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	char nqversion[3];
	tv->map.cdtrack = ReadByte(m);

	ConnectionData(tv, (void*)((char*)m->data+m->startpos), m->readpos - m->startpos, to, mask, QW);

	nqversion[0] = svc_cdtrack;
	nqversion[1] = tv->map.cdtrack;
	nqversion[2] = tv->map.cdtrack;
	ConnectionData(tv, nqversion, 3, to, mask, NQ);
}
static void ParseStufftext(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	viewer_t *v;
	char text[1024];

	ReadString(m, text, sizeof(text));
//	Sys_Printf(tv->cluster, "stuffcmd: %s", text);
	if (!strcmp(text, "say proxy:menu\n"))
	{	//qizmo's 'previous proxy' message
		tv->proxyisselected = true;
		if (tv->controller)
			QW_SetMenu(tv->controller, MENU_MAIN);
		tv->serverisproxy = true;	//FIXME: Detect this properly on qizmo
	}
	else if (!strncmp(text, "//I am a proxy", 14))
		tv->serverisproxy = true;
	else if (!strncmp(text, "//set prox_inmenu ", 18))
	{
		if (tv->controller)
			QW_SetMenu(tv->controller, atoi(text+18)?MENU_FORWARDING:MENU_NONE);
	}
//	else if (!strncmp(text, "//set protocolname ", 19))
//	else if (!strncmp(text, "//set recorddate ", 17))	//reports when the demo was originally recorded, without needing to depend upon metadata.
//	else if (!strncmp(text, "//paknames ", 11))
//	else if (!strncmp(text, "//paks ", 7))
//	else if (!strncmp(text, "//vwep ", 7))
	else if (strstr(text, "screenshot"))
	{
		if (tv->controller)
		{	//let it through to the controller
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
		}
		return;	//this was generating far too many screenshots when watching demos
	}
	else if (!strcmp(text, "skins\n"))
	{
		const char newcmd[10] = {svc_stufftext, 'c', 'm', 'd', ' ', 'n','e','w','\n','\0'};
		tv->parsingconnectiondata = false;

		strcpy(tv->status, "On server\n");

		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (v->server == tv && (v != tv->controller || v->netchan.isnqprotocol))
			{
				v->servercount++;
				SendBufferToViewer(v, newcmd, sizeof(newcmd), true);
			}
		}

		if (tv->controller && !tv->controller->netchan.isnqprotocol)
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
		else if (tv->usequakeworldprotocols)
			SendClientCommand(tv, "begin %i\n", tv->clservercount);
		return;
	}
	else if (!strncmp(text, "fullserverinfo ", 15))
	{
		/*strip newline*/
		text[strlen(text)-1] = '\0';
		/*strip trailing quote*/
		text[strlen(text)-1] = '\0';

		//copy over the server's serverinfo
		strlcpy(tv->map.serverinfo, text+16, sizeof(tv->map.serverinfo));

		QTV_UpdatedServerInfo(tv);

		if (tv->controller && (tv->controller->netchan.isnqprotocol == false))
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
		return;
	}
	else if (!strncmp(text, "cmd prespawn ", 13))
	{
		if (tv->usequakeworldprotocols)
			SendClientCommand(tv, "%s", text+4);
		return;	//commands the game server asked for are pointless.
	}
	else if (!strncmp(text, "cmd spawn ", 10))
	{
		if (tv->usequakeworldprotocols)
			SendClientCommand(tv, "%s", text+4);

		return;	//commands the game server asked for are pointless.
	}
	else if (!strncmp(text, "cmd ", 4))
	{
		if (tv->controller)
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
		else if (tv->usequakeworldprotocols)
			SendClientCommand(tv, "%s", text+4);
		return;	//commands the game server asked for are pointless.
	}
	else if (!strncmp(text, "reconnect", 9))
	{
		if (tv->controller)
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
		else if (tv->usequakeworldprotocols)
			SendClientCommand(tv, "new\n");
		return;
	}
	else if (!strncmp(text, "packet ", 7))
	{
		if (tv->controller)
		{	//if we're acting as a proxy, forward the realip packets, and ONLY to the controller
			//quakeworld proxies are usually there for routing or protocol advantages, NOT privacy
			//(client can always ignore it themselves, but a server might ban you, but at least they'll be less inclined to ban the proxy).
			SendBufferToViewer(tv->controller, (char*)m->data+m->startpos, m->readpos - m->startpos, true);
			return;
		}
		if(tv->usequakeworldprotocols)
		{//eeeevil hack for proxy-spectating
			char *ptr;
			char arg[3][ARG_LEN];
			netadr_t adr;
			ptr = text;
			ptr = COM_ParseToken(ptr, arg[0], ARG_LEN, "");
			ptr = COM_ParseToken(ptr, arg[1], ARG_LEN, "");
			ptr = COM_ParseToken(ptr, arg[2], ARG_LEN, "");
			NET_StringToAddr(arg[1], &adr, PROX_DEFAULTSERVERPORT);
			Netchan_OutOfBandSocket(tv->cluster, tv->sourcesock, &adr, strlen(arg[2]), arg[2]);

			//this is an evil hack
			SendClientCommand(tv, "new\n");
			return;
		}
		Sys_Printf(tv->cluster, "packet stuffcmd in an mvd\n");	//shouldn't ever happen, try ignoring it.
		return;
	}
	else if (tv->usequakeworldprotocols && !strncmp(text, "setinfo ", 8))
	{
		Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, Q1);
		if (!tv->controller)
			SendClientCommand(tv, "%s", text);
	}
	else
	{
		Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, Q1);
		return;
	}
}

static void ParseSetInfo(sv_t *tv, netmsg_t *m)
{
	int pnum;
	char key[64];
	char value[256];
	pnum = ReadByte(m);
	ReadString(m, key, sizeof(key));
	ReadString(m, value, sizeof(value));

	if (pnum < MAX_CLIENTS)
		Info_SetValueForStarKey(tv->map.players[pnum].userinfo, key, value, sizeof(tv->map.players[pnum].userinfo));

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, dem_all, (unsigned)-1, QW);
}

static void ParseServerinfo(sv_t *tv, netmsg_t *m)
{
	char key[64];
	char value[256];
	ReadString(m, key, sizeof(key));
	ReadString(m, value, sizeof(value));

	if (strcmp(key, "hostname"))	//don't allow the hostname to change, but allow the server to change other serverinfos.
		Info_SetValueForStarKey(tv->map.serverinfo, key, value, sizeof(tv->map.serverinfo));

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, dem_all, (unsigned)-1, QW);
}

static void ParsePrint(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	char text[1024];
	char nqbuffer[1024];
	int level;

	level = ReadByte(m);
	ReadString(m, text, sizeof(text)-2);

	if (level == 3)
	{
		//FIXME: that number shouldn't be hard-coded
		if (!strncmp(text, "#0:qtv_say:#", 12) || !strncmp(text, "#0:qtv_say_game:#", 17) || !strncmp(text, "#0:qtv_say_team_game:#", 22))
		{
			char *colon;
			colon = strchr(text, ':');
			colon = strchr(colon+1, ':');
			colon = strchr(colon+1, ':');
			if (colon)
			{
				//de-fuck qqshka's extra gibberish.
				snprintf(nqbuffer, sizeof(nqbuffer), "%c%c[QTV]%s\n", svc_print, 3, colon+1);
				Multicast(tv, nqbuffer, strlen(nqbuffer)+1, to, mask, QW|CONNECTING);
				snprintf(nqbuffer, sizeof(nqbuffer), "%c%c[QTV]%s\n", svc_print, 1, colon+1);
				Multicast(tv, nqbuffer, strlen(nqbuffer)+1, to, mask, NQ|CONNECTING);
				return;
			}
		}
		strlcpy(nqbuffer+2, text, sizeof(nqbuffer)-2);
		nqbuffer[1] = 1;	//nq chat is prefixed with a 1
	}
	else
	{
		strlcpy(nqbuffer+1, text, sizeof(nqbuffer)-1);
	}
	nqbuffer[0] = svc_print;

	if ((to&dem_mask) == dem_all || to == dem_read)
	{
		if (level > 1)
		{
			QTV_Printf(tv, "%s", text);
		}
	}

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW|CONNECTING);
	Multicast(tv, nqbuffer, strlen(nqbuffer)+1, to, mask, NQ|CONNECTING);
}
static void ParseCenterprint(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	viewer_t *v;
	char text[1024];
	ReadString(m, text, sizeof(text));




	switch(to)
	{
	case dem_multiple:
	case dem_single:
	case dem_stats:
		//check and send to them only if they're tracking this player(s).
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (!v->menunum || v->menunum == MENU_FORWARDING)
			if (v->thinksitsconnected)
				if (v->server == tv)
					if (v->trackplayer>=0)
						if ((1<<v->trackplayer)&mask)
						{
							SendBufferToViewer(v, (char*)m->data+m->startpos, m->readpos - m->startpos, true);	//FIXME: change the reliable depending on message type
						}
		}
		break;
	default:
		//send to all
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (!v->menunum || v->menunum == MENU_FORWARDING)
			if (v->thinksitsconnected)
				if (v->server == tv)
					SendBufferToViewer(v, (char*)m->data+m->startpos, m->readpos - m->startpos, true);	//FIXME: change the reliable depending on message type
		}
		break;
	}
}
static int ParseList(sv_t *tv, netmsg_t *m, filename_t *list, int to, unsigned int mask, qboolean big)
{
	int first;

	if (big)
		first = ReadShort(m)+1;
	else
		first = ReadByte(m)+1;
	for (; first < MAX_LIST; first++)
	{
		ReadString(m, list[first].name, sizeof(list[first].name));
//		printf("read %i: %s\n", first, list[first].name);
		if (!*list[first].name)
			break;
//		printf("%i: %s\n", first, list[first].name);
	}

	return ReadByte(m);
}

static void ParseEntityState(sv_t *tv, entity_state_t *es, netmsg_t *m)	//for baselines/static entities
{
	int i;

	es->modelindex = ReadByte(m);
	es->frame = ReadByte(m);
	es->colormap = ReadByte(m);
	es->skinnum = ReadByte(m);
	for (i = 0; i < 3; i++)
	{
		es->origin[i] = ReadCoord(m, tv->pext1);
		es->angles[i] = ReadAngle(m, tv->pext1);
	}
}

static void ParseStaticSound(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	if (tv->map.staticsound_count == MAX_STATICSOUNDS)
	{
		tv->map.staticsound_count--;	// don't be fatal.
		Sys_Printf(tv->cluster, "Too many static sounds\n");
	}

	tv->map.staticsound[tv->map.staticsound_count].origin[0] = ReadCoord(m, tv->pext1);
	tv->map.staticsound[tv->map.staticsound_count].origin[1] = ReadCoord(m, tv->pext1);
	tv->map.staticsound[tv->map.staticsound_count].origin[2] = ReadCoord(m, tv->pext1);
	tv->map.staticsound[tv->map.staticsound_count].soundindex = ReadByte(m);
	tv->map.staticsound[tv->map.staticsound_count].volume = ReadByte(m);
	tv->map.staticsound[tv->map.staticsound_count].attenuation = ReadByte(m);

	tv->map.staticsound_count++;

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, Q1);
}

static void ParseIntermission(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	ReadShort(m);
	ReadShort(m);
	ReadShort(m);
	ReadByte(m);
	ReadByte(m);
	ReadByte(m);

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

extern const usercmd_t nullcmd;
static void ParsePlayerInfo(sv_t *tv, netmsg_t *m, qboolean clearoldplayers)
{
	usercmd_t nonnullcmd;
	int flags;
	int num;
	int i;

	if (clearoldplayers)
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{	//hide players
			//they'll be sent after this packet.
			tv->map.players[i].oldactive = tv->map.players[i].active;
			tv->map.players[i].active = false;
		}
	}

	num = ReadByte(m);
	if (num >= MAX_CLIENTS)
	{
		num = 0;	// don't be fatal.
		Sys_Printf(tv->cluster, "Too many svc_playerinfos, wrapping\n");
	}
	tv->map.players[num].old = tv->map.players[num].current;

	if (tv->usequakeworldprotocols)
	{
		flags = (unsigned short)ReadShort (m);

		tv->map.players[num].current.origin[0] = ReadCoord (m, tv->pext1);
		tv->map.players[num].current.origin[1] = ReadCoord (m, tv->pext1);
		tv->map.players[num].current.origin[2] = ReadCoord (m, tv->pext1);

		tv->map.players[num].current.frame = ReadByte(m);

		if (flags & PF_MSEC)
			ReadByte (m);

		if (flags & PF_COMMAND)
		{
			ReadDeltaUsercmd(m, &nullcmd, &nonnullcmd);
			tv->map.players[num].current.angles[0] = nonnullcmd.angles[0];
			tv->map.players[num].current.angles[1] = nonnullcmd.angles[1];
			tv->map.players[num].current.angles[2] = nonnullcmd.angles[2];
		}
		else
		{	//the only reason we'd not get a command is if it's us.
			if (tv->controller)
			{
				tv->map.players[num].current.angles[0] = tv->controller->ucmds[2].angles[0];
				tv->map.players[num].current.angles[1] = tv->controller->ucmds[2].angles[1];
				tv->map.players[num].current.angles[2] = tv->controller->ucmds[2].angles[2];
			}
			else
			{
				tv->map.players[num].current.angles[0] = tv->proxyplayerangles[0];
				tv->map.players[num].current.angles[1] = tv->proxyplayerangles[1];
				tv->map.players[num].current.angles[2] = tv->proxyplayerangles[2];
			}
		}

		for (i=0 ; i<3 ; i++)
		{
			if (flags & (PF_VELOCITY1<<i) )
				tv->map.players[num].current.velocity[i] = ReadShort(m);
			else
				tv->map.players[num].current.velocity[i] = 0;
		}

		tv->map.players[num].gibbed = !!(flags & PF_GIB);
		tv->map.players[num].dead = !!(flags & PF_DEAD);

		if (flags & PF_MODEL)
			tv->map.players[num].current.modelindex = ReadByte (m);
		else
			tv->map.players[num].current.modelindex = tv->map.modelindex_player;

		if (flags & PF_SKINNUM)
			tv->map.players[num].current.skinnum = ReadByte (m);
		else
			tv->map.players[num].current.skinnum = 0;

		if (flags & PF_EFFECTS)
			tv->map.players[num].current.effects = ReadByte (m);
		else
			tv->map.players[num].current.effects = 0;

		if (flags & PF_WEAPONFRAME)
			tv->map.players[num].current.weaponframe = ReadByte (m);
		else
			tv->map.players[num].current.weaponframe = 0;

		tv->map.players[num].active = true;
	}
	else
	{
		flags = ReadShort(m);
		tv->map.players[num].gibbed = !!(flags & DF_GIB);
		tv->map.players[num].dead = !!(flags & DF_DEAD);
		tv->map.players[num].current.frame = ReadByte(m);

		for (i = 0; i < 3; i++)
		{
			if (flags & (DF_ORIGIN << i))
				tv->map.players[num].current.origin[i] = ReadCoord (m, tv->pext1);
		}

		for (i = 0; i < 3; i++)
		{
			if (flags & (DF_ANGLES << i))
			{
				tv->map.players[num].current.angles[i] = (ReadShort(m)/(float)0x10000)*360;
			}
		}

		if (flags & DF_MODEL)
			tv->map.players[num].current.modelindex = ReadByte (m);

		if (flags & DF_SKINNUM)
			tv->map.players[num].current.skinnum = ReadByte (m);

		if (flags & DF_EFFECTS)
			tv->map.players[num].current.effects = ReadByte (m);

		if (flags & DF_WEAPONFRAME)
			tv->map.players[num].current.weaponframe = ReadByte (m);

		tv->map.players[num].active = true;

	}

	tv->map.players[num].leafcount = BSP_SphereLeafNums(tv->map.bsp,	MAX_ENTITY_LEAFS, tv->map.players[num].leafs,
														tv->map.players[num].current.origin[0],
														tv->map.players[num].current.origin[1],
														tv->map.players[num].current.origin[2], 32);
}

static int readentitynum(netmsg_t *m, unsigned int *retflags)
{
	int entnum;
	unsigned int flags;
	flags = ReadShort(m);
	if (!flags)
	{
		*retflags = 0;
		return 0;
	}

	entnum = flags&511;
	flags &= ~511;

	if (flags & U_MOREBITS)
	{
		flags |= ReadByte(m);

		if (flags & UX_EVENMORE)
			flags |= ReadByte(m)<<16;
		if (flags & UX_YETMORE)
			flags |= ReadByte(m)<<24;
	}

	if (flags & UX_ENTITYDBL)
		entnum += 512;
	if (flags & UX_ENTITYDBL2)
		entnum += 1024;

	*retflags = flags;

	return entnum;
}

static void ParseEntityDelta(sv_t *tv, netmsg_t *m, const entity_state_t *old, entity_state_t *new, unsigned int flags, entity_t *ent, qboolean forcerelink)
{
	memcpy(new, old, sizeof(entity_state_t));

	if (flags & U_MODEL)
	{
		if (flags & UX_MODELDBL)
			new->modelindex = ReadByte(m)|0x100;	//doubled limit...
		else
			new->modelindex = ReadByte(m);
	}
	else if (flags & UX_MODELDBL)
		new->modelindex = ReadShort(m);	//more sane path...
	if (flags & U_FRAME)
		new->frame = ReadByte(m);
	if (flags & U_COLORMAP)
		new->colormap = ReadByte(m);
	if (flags & U_SKIN)
		new->skinnum = ReadByte(m);
	if (flags & U_EFFECTS)
		new->effects = (new->effects&0xff00)|ReadByte(m);

	if (flags & U_ORIGIN1)
		new->origin[0] = ReadCoord(m, tv->pext1);
	if (flags & U_ANGLE1)
		new->angles[0] = ReadAngle(m, tv->pext1);
	if (flags & U_ORIGIN2)
		new->origin[1] = ReadCoord(m, tv->pext1);
	if (flags & U_ANGLE2)
		new->angles[1] = ReadAngle(m, tv->pext1);
	if (flags & U_ORIGIN3)
		new->origin[2] = ReadCoord(m, tv->pext1);
	if (flags & U_ANGLE3)
		new->angles[2] = ReadAngle(m, tv->pext1);

	if (flags & UX_SCALE)
		new->scale = ReadByte(m);
	if (flags & UX_ALPHA)
		new->alpha = ReadByte(m);
	if (flags & UX_FATNESS)
		new->fatness = (signed char)ReadByte(m);
	if (flags & UX_DRAWFLAGS)
		new->drawflags = ReadByte(m);
	if (flags & UX_ABSLIGHT)
		new->abslight = ReadByte(m);
	if (flags & UX_COLOURMOD)
	{
		new->colormod[0] = ReadByte(m);
		new->colormod[1] = ReadByte(m);
		new->colormod[2] = ReadByte(m);
	}
	if (flags & UX_DPFLAGS)
	{	// these are bits for the 'flags' field of the entity_state_t
		new->dpflags = ReadByte(m);
	}
	if (flags & UX_TAGINFO)
	{
		new->tagentity = ReadShort(m);
		new->tagindex = ReadShort(m);
	}
	if (flags & UX_LIGHT)
	{
		new->light[0] = ReadShort(m);
		new->light[1] = ReadShort(m);
		new->light[2] = ReadShort(m);
		new->light[3] = ReadShort(m);
		new->lightstyle = ReadByte(m);
		new->lightpflags = ReadByte(m);
	}
	if (flags & UX_EFFECTS16)
		new->effects = (new->effects&0x00ff)|(ReadByte(m)<<8);


	if (forcerelink || (flags & (U_ORIGIN1|U_ORIGIN2|U_ORIGIN3|U_MODEL)))
	{
		if (ent)
			ent->leafcount = 
					BSP_SphereLeafNums(tv->map.bsp, MAX_ENTITY_LEAFS, ent->leafs,
					new->origin[0],
					new->origin[1],
					new->origin[2], 32);
	}
}

static int ExpandFrame(unsigned int newmax, frame_t *frame)
{
	entity_state_t *newents;
	unsigned short *newnums;

	if (newmax < frame->maxents)
		return true;

	newmax += 16;

	newents = malloc(sizeof(*newents) * newmax);
	if (!newents)
		return false;
	newnums = malloc(sizeof(*newnums) * newmax);
	if (!newnums)
	{
		free(newents);
		return false;
	}

	memcpy(newents, frame->ents, sizeof(*newents) * frame->maxents);
	memcpy(newnums, frame->entnums, sizeof(*newnums) * frame->maxents);

	if (frame->ents)
		free(frame->ents);
	if (frame->entnums)
		free(frame->entnums);
	
	frame->ents = newents;
	frame->entnums = newnums;
	frame->maxents = newmax;
	return true;
}

static void ParsePacketEntities(sv_t *tv, netmsg_t *m, int deltaframe)
{
	frame_t *newframe;
	frame_t *oldframe;
	int oldcount;
	int newnum, oldnum;
	int newindex, oldindex;
	unsigned int flags;

	viewer_t *v;

	tv->map.nailcount = 0;

	tv->physicstime = tv->curtime;

	if (tv->cluster->chokeonnotupdated)
	{
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (v->server == tv)
				v->chokeme = false;
		}
		for (v = tv->cluster->viewers; v; v = v->next)
		{
			if (v->server == tv && v->netchan.isnqprotocol)
				v->maysend = true;
		}
	}


	if (deltaframe != -1)
		deltaframe &= (ENTITY_FRAMES-1);

	if (tv->usequakeworldprotocols)
	{
		newframe = &tv->map.frame[tv->netchan.incoming_sequence & (ENTITY_FRAMES-1)];

		if (tv->netchan.outgoing_sequence - tv->netchan.incoming_sequence >= ENTITY_FRAMES - 1)
		{
			//should drop it
			Sys_Printf(tv->cluster, "Outdated frames\n");
		}
		else if (deltaframe != -1 && newframe->oldframe != deltaframe)
			Sys_Printf(tv->cluster, "Mismatching delta frames\n");
	}
	else
	{
		deltaframe = tv->netchan.incoming_sequence & (ENTITY_FRAMES-1);
		tv->netchan.incoming_sequence++;
		newframe = &tv->map.frame[tv->netchan.incoming_sequence & (ENTITY_FRAMES-1)];
	}
	if (deltaframe != -1)
	{
		oldframe = &tv->map.frame[deltaframe];
		oldcount = oldframe->numents;
	}
	else
	{
		oldframe = NULL;
		oldcount = 0;
	}

	oldindex = 0;
	newindex = 0;

//printf("frame\n");

	for(;;)
	{
		newnum = readentitynum(m, &flags);
		if (!newnum)
		{
			//end of packet
			//any remaining old ents need to be copied to the new frame
			while (oldindex < oldcount)
			{
//printf("Propogate (spare)\n");
				if (!ExpandFrame(newindex, newframe))
					break;

				memcpy(&newframe->ents[newindex], &oldframe->ents[oldindex], sizeof(entity_state_t));
				newframe->entnums[newindex] = oldframe->entnums[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}

		if (oldindex >= oldcount)
			oldnum = 0xffff;
		else
			oldnum = oldframe->entnums[oldindex];
		while(newnum > oldnum)
		{
//printf("Propogate (unchanged)\n");
			if (!ExpandFrame(newindex, newframe))
				break;

			memcpy(&newframe->ents[newindex], &oldframe->ents[oldindex], sizeof(entity_state_t));
			newframe->entnums[newindex] = oldframe->entnums[oldindex];
			newindex++;
			oldindex++;

			if (oldindex >= oldcount)
				oldnum = 0xffff;
			else
				oldnum = oldframe->entnums[oldindex];
		}

		if (newnum < oldnum)
		{	//this ent wasn't in the last packet
//printf("add\n");
			if (flags & U_REMOVE)
			{	//remove this ent... just don't copy it across.
				//printf("add\n");
				continue;
			}

			if (!ExpandFrame(newindex, newframe))
				break;
			ParseEntityDelta(tv, m, &tv->map.entity[newnum].baseline, &newframe->ents[newindex], flags, &tv->map.entity[newnum], true);
			newframe->entnums[newindex] = newnum;
			newindex++;
		}
		else if (newnum == oldnum)
		{
			if (flags & U_REMOVE)
			{	//remove this ent... just don't copy it across.
				//printf("add\n");
				oldindex++;
				continue;
			}
//printf("Propogate (changed)\n");
			if (!ExpandFrame(newindex, newframe))
				break;
			ParseEntityDelta(tv, m, &oldframe->ents[oldindex], &newframe->ents[newindex], flags, &tv->map.entity[newnum], false);
			newframe->entnums[newindex] = newnum;
			newindex++;
			oldindex++;
		}

	}

	newframe->numents = newindex;
return;

/*

	//luckilly, only updated entities are here, so that keeps cpu time down a bit.
	for (;;)
	{
		flags = ReadShort(m);
		if (!flags)
			break;

		entnum = flags & 511;
		if (tv->maxents < entnum)
			tv->maxents = entnum;
		flags &= ~511;
		memcpy(&tv->entity[entnum].old, &tv->entity[entnum].current, sizeof(entity_state_t));	//ow.
		if (flags & U_REMOVE)
		{
			tv->entity[entnum].current.modelindex = 0;
			continue;
		}
		if (!tv->entity[entnum].current.modelindex)	//lerp from baseline
		{
			memcpy(&tv->entity[entnum].current, &tv->entity[entnum].baseline, sizeof(entity_state_t));
			forcerelink = true;
		}
		else
			forcerelink = false;

		if (flags & U_MOREBITS)
			flags |= ReadByte(m);
		if (flags & U_MODEL)
			tv->entity[entnum].current.modelindex = ReadByte(m);
		if (flags & U_FRAME)
			tv->entity[entnum].current.frame = ReadByte(m);
		if (flags & U_COLORMAP)
			tv->entity[entnum].current.colormap = ReadByte(m);
		if (flags & U_SKIN)
			tv->entity[entnum].current.skinnum = ReadByte(m);
		if (flags & U_EFFECTS)
			tv->entity[entnum].current.effects = ReadByte(m);

		if (flags & U_ORIGIN1)
			tv->entity[entnum].current.origin[0] = ReadShort(m);
		if (flags & U_ANGLE1)
			tv->entity[entnum].current.angles[0] = ReadByte(m);
		if (flags & U_ORIGIN2)
			tv->entity[entnum].current.origin[1] = ReadShort(m);
		if (flags & U_ANGLE2)
			tv->entity[entnum].current.angles[1] = ReadByte(m);
		if (flags & U_ORIGIN3)
			tv->entity[entnum].current.origin[2] = ReadShort(m);
		if (flags & U_ANGLE3)
			tv->entity[entnum].current.angles[2] = ReadByte(m);

		tv->entity[entnum].updatetime = tv->curtime;
		if (!tv->entity[entnum].old.modelindex)	//no old state
			memcpy(&tv->entity[entnum].old, &tv->entity[entnum].current, sizeof(entity_state_t));	//copy the new to the old, so we don't end up with interpolation glitches


		if ((flags & (U_ORIGIN1 | U_ORIGIN2 | U_ORIGIN3)) || forcerelink)
			tv->entity[entnum].leafcount = BSP_SphereLeafNums(tv->bsp, MAX_ENTITY_LEAFS, tv->entity[entnum].leafs,
															tv->entity[entnum].current.origin[0],
															tv->entity[entnum].current.origin[1],
															tv->entity[entnum].current.origin[2], 32);
	}
*/
}

void ParseSpawnStatic(sv_t *tv, netmsg_t *m, int to, unsigned int mask, qboolean delta)
{
	if (tv->map.spawnstatic_count == MAX_STATICENTITIES)
	{
		tv->map.spawnstatic_count--;	// don't be fatal.
		Sys_Printf(tv->cluster, "Too many static entities\n");
	}

	if (delta)
	{
		unsigned int flags;
		readentitynum(m, &flags);
		ParseEntityDelta(tv, m, &null_entity_state, &tv->map.spawnstatic[tv->map.spawnstatic_count], flags, NULL, false);
	}
	else
		ParseEntityState(tv, &tv->map.spawnstatic[tv->map.spawnstatic_count], m);

	tv->map.spawnstatic_count++;

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, Q1);
}

static void ParseBaseline(sv_t *tv, netmsg_t *m, int to, unsigned int mask, qboolean delta)
{
	unsigned int entnum;
	if (delta)
	{
		entity_state_t es;
		unsigned int flags;
		entnum = readentitynum(m, &flags);
		ParseEntityDelta(tv, m, &null_entity_state, &es, flags, NULL, false);

		if (entnum >= MAX_ENTITIES)
		{
			ParseError(m);
			return;
		}
		tv->map.entity[entnum].baseline = es;
	}
	else
	{
		entnum = ReadShort(m);
		if (entnum >= MAX_ENTITIES)
		{
			ParseError(m);
			return;
		}
		ParseEntityState(tv, &tv->map.entity[entnum].baseline, m);
	}
	
	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, Q1);
}

static void ParseUpdatePing(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum;
	int ping;
	pnum = ReadByte(m);
	ping = ReadShort(m);

	if (pnum < MAX_CLIENTS)
		tv->map.players[pnum].ping = ping;
	else
		Sys_Printf(tv->cluster, "svc_updateping: invalid player number\n");

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

static void ParseUpdateFrags(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum;
	int frags;
	pnum = ReadByte(m);
	frags = (signed short)ReadShort(m);

	if (pnum < MAX_CLIENTS)
		tv->map.players[pnum].frags = frags;
	else
		Sys_Printf(tv->cluster, "svc_updatefrags: invalid player number\n");

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, (pnum < 16)?Q1:QW);
}

static void ParseUpdateStat(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;
	int statnum;

	statnum = ReadByte(m);
	value = ReadByte(m);

	if (statnum < MAX_STATS)
	{
		for (pnum = 0; pnum < MAX_CLIENTS; pnum++)
		{
			if (mask & (1<<pnum))
				tv->map.players[pnum].stats[statnum] = value;
		}
	}
	else
		Sys_Printf(tv->cluster, "svc_updatestat: invalid stat number\n");

//	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}
static void ParseUpdateStatLong(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;
	int statnum;

	statnum = ReadByte(m);
	value = ReadLong(m);

	if (statnum < MAX_STATS)
	{
		for (pnum = 0; pnum < MAX_CLIENTS; pnum++)
		{
			if (mask & (1<<pnum))
				tv->map.players[pnum].stats[statnum] = value;
		}
	}
	else
		Sys_Printf(tv->cluster, "svc_updatestatlong: invalid stat number\n");

//	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

static void ParseUpdateUserinfo(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum;
	pnum = ReadByte(m);
	ReadLong(m);
	if (pnum < MAX_CLIENTS)
		ReadString(m, tv->map.players[pnum].userinfo, sizeof(tv->map.players[pnum].userinfo));
	else
	{
		Sys_Printf(tv->cluster, "svc_updateuserinfo: invalid player number\n");
		while (ReadByte(m))	//suck out the message.
		{
		}
	}

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

static void ParsePacketloss(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;

	pnum = ReadByte(m)%MAX_CLIENTS;
	value = ReadByte(m);

	if (pnum < MAX_CLIENTS)
		tv->map.players[pnum].packetloss = value;
	else
		Sys_Printf(tv->cluster, "svc_updatepl: invalid player number\n");

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

static void ParseUpdateEnterTime(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	float value;

	pnum = ReadByte(m)%MAX_CLIENTS;
	value = ReadFloat(m);

	if (pnum < MAX_CLIENTS)
		tv->map.players[pnum].entertime = value;
	else
		Sys_Printf(tv->cluster, "svc_updateentertime: invalid player number\n");

	ConnectionData(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

static void ParseSound(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
#define	SND_VOLUME		(1<<15)		// a qbyte
#define	SND_ATTENUATION	(1<<14)		// a qbyte

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0
	int i;
	int channel;
	unsigned char vol;
	unsigned char atten;
	unsigned char sound_num;
	float org[3];
	int ent;


	netmsg_t nqversion;
	unsigned char nqbuffer[64];
	InitNetMsg(&nqversion, nqbuffer, sizeof(nqbuffer));

	channel = (unsigned short)ReadShort(m);


	if (channel & SND_VOLUME)
		vol = ReadByte (m);
	else
		vol = DEFAULT_SOUND_PACKET_VOLUME;

	if (channel & SND_ATTENUATION)
		atten = ReadByte (m) / 64.0;
	else
		atten = DEFAULT_SOUND_PACKET_ATTENUATION;

	sound_num = ReadByte (m);

	ent = (channel>>3)&1023;
	channel &= 7;

	for (i=0 ; i<3 ; i++)
		org[i] = ReadCoord (m, tv->pext1);

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);


	WriteByte(&nqversion, svc_sound);
	i = 0;
	if (vol != DEFAULT_SOUND_PACKET_VOLUME)
		i |= 1;
	if (atten != DEFAULT_SOUND_PACKET_ATTENUATION)
		i |= 2;
	if (ent > 8191 || channel > 7)
		i |= 8;
	if (sound_num > 255)
		i |= 16;
	WriteByte(&nqversion, i);
	if (i & 1)
		WriteByte(&nqversion, vol);
	if (i & 2)
		WriteByte(&nqversion, atten*64);
	if (i & 8)
	{
		WriteShort(&nqversion, ent);
		WriteByte(&nqversion, channel);
	}
	else
		WriteShort(&nqversion, (ent<<3) | channel);
	if (i & 16)
		WriteShort(&nqversion, sound_num);
	else
		WriteByte(&nqversion, sound_num);
	WriteCoord(&nqversion, org[0], tv->pext1);
	WriteCoord(&nqversion, org[1], tv->pext1);
	WriteCoord(&nqversion, org[2], tv->pext1);

	Multicast(tv, nqversion.data, nqversion.cursize, to, mask, NQ);
}

static void ParseDamage(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	ReadByte (m);
	ReadByte (m);
	ReadCoord (m, tv->pext1);
	ReadCoord (m, tv->pext1);
	ReadCoord (m, tv->pext1);
	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, QW);
}

enum {
	TE_SPIKE			= 0,
	TE_SUPERSPIKE		= 1,
	TE_GUNSHOT			= 2,
	TE_EXPLOSION		= 3,
	TE_TAREXPLOSION		= 4,
	TE_LIGHTNING1		= 5,
	TE_LIGHTNING2		= 6,
	TE_WIZSPIKE			= 7,
	TE_KNIGHTSPIKE		= 8,
	TE_LIGHTNING3		= 9,
	TE_LAVASPLASH		= 10,
	TE_TELEPORT			= 11,

	TE_BLOOD			= 12,
	TE_LIGHTNINGBLOOD	= 13,
};
static void ParseTempEntity(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int i;
	int dest = QW;
	char nqversion[64];
	int nqversionlength=0;

	i = ReadByte (m);
	switch(i)
	{
	case TE_SPIKE:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_SUPERSPIKE:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_GUNSHOT:
		ReadByte (m);

		nqversion[0] = svc_temp_entity;
		nqversion[1] = TE_GUNSHOT;
		if (tv->pext1 & PEXT_FLOATCOORDS)
			nqversionlength = 2+3*4;
		else
			nqversionlength = 2+3*2;
		for (i = 2; i < nqversionlength; i++)
			nqversion[i] = ReadByte (m);
		break;
	case TE_EXPLOSION:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_TAREXPLOSION:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_LIGHTNING1:
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		ReadShort (m);

		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);

		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_WIZSPIKE:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_KNIGHTSPIKE:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_LAVASPLASH:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_TELEPORT:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		dest |= NQ;
		break;
	case TE_BLOOD:
		ReadByte (m);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		//FIXME: generate svc_particle for nq
		break;
	case TE_LIGHTNINGBLOOD:
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		ReadCoord (m, tv->pext1);
		//FIXME: generate svc_particle for nq
		break;
	default:
		Sys_Printf(tv->cluster, "temp entity %i not recognised\n", i);
		return;
	}

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, to, mask, dest);

	if (nqversionlength)
		Multicast(tv, nqversion, nqversionlength, to, mask, NQ);
}

void ParseLightstyle(sv_t *tv, netmsg_t *m)
{
	int style;
	style = ReadByte(m);
	if (style < MAX_LIGHTSTYLES)
		ReadString(m, tv->map.lightstyle[style].name, sizeof(tv->map.lightstyle[style].name));
	else
	{
		Sys_Printf(tv->cluster, "svc_lightstyle: invalid lightstyle index (%i)\n", style);
		while (ReadByte(m))	//suck out the message.
		{
		}
	}

	Multicast(tv, (char*)m->data+m->startpos, m->readpos - m->startpos, dem_read, (unsigned)-1, Q1);
}

void ParseNails(sv_t *tv, netmsg_t *m, qboolean nails2)
{
	int count;
	int i;
	count = (unsigned char)ReadByte(m);
	while(count > sizeof(tv->map.nails) / sizeof(tv->map.nails[0]))
	{//they sent too many, suck it out.
		count--;
		if (nails2)
			ReadByte(m);
		for (i = 0; i < 6; i++)
			ReadByte(m);
	}

	tv->map.nailcount = count;
	while(count-- > 0)
	{
		if (nails2)
			tv->map.nails[count].number = ReadByte(m);
		else
			tv->map.nails[count].number = count;
		for (i = 0; i < 6; i++)
			tv->map.nails[count].bits[i] = ReadByte(m);
	}
}

void ParseDownload(sv_t *tv, netmsg_t *m)
{
//warning this needs looking at (controller downloads)
	int size, b;
	unsigned int percent;
	char buffer[2048];

	size = (signed short)ReadShort(m);
	percent = ReadByte(m);

	if (size < 0)
	{
		Sys_Printf(tv->cluster, "Downloading failed\n");
		if (tv->downloadfile)
			fclose(tv->downloadfile);
		tv->downloadfile = NULL;
		tv->errored = ERR_PERMANENT;
		QW_StreamPrint(tv->cluster, tv, NULL, "Map download failed\n");
		return;
	}

	for (b = 0; b < size; b++)
		buffer[b] = ReadByte(m);

	if (!tv->downloadfile)
	{
		Sys_Printf(tv->cluster, "Not downloading anything\n");
		tv->errored = ERR_PERMANENT;
		return;
	}
	fwrite(buffer, 1, size, tv->downloadfile);

	if (percent == 100)
	{
		fclose(tv->downloadfile);
		tv->downloadfile = NULL;

		snprintf(buffer, sizeof(buffer), "%s/%s", (*tv->map.gamedir)?tv->map.gamedir:"id1", tv->map.modellist[1].name);
		rename(tv->downloadname, buffer);

		Sys_Printf(tv->cluster, "Download complete\n");

		tv->map.bsp = BSP_LoadModel(tv->cluster, tv->map.gamedir, tv->map.modellist[1].name);
		if (!tv->map.bsp)
		{
			Sys_Printf(tv->cluster, "Failed to read BSP\n");
			tv->errored = ERR_PERMANENT;
		}
		else
		{
			SendClientCommand(tv, "prespawn %i 0 %i\n", tv->clservercount, LittleLong(BSP_Checksum(tv->map.bsp)));
			strcpy(tv->status, "Prespawning\n");
		}
	}
	else
	{
		snprintf(tv->status, sizeof(tv->status), "Downloading map, %i%%\n", percent);
		SendClientCommand(tv, "nextdl\n");
	}
}

void ParseMessage(sv_t *tv, void *buffer, int length, int to, int mask)
{
	int lastsvc;
	int svc = -1;
	int i;
	netmsg_t buf;
	qboolean clearoldplayers = true;
	buf.cursize = length;
	buf.maxsize = length;
	buf.readpos = 0;
	buf.data = buffer;
	buf.startpos = 0;
	while(buf.readpos < buf.cursize)
	{
		lastsvc = svc;
		if (buf.readpos > buf.cursize)
		{
			Sys_Printf(tv->cluster, "Read past end of parse buffer\n, last was %i\n", lastsvc);
			return;
		}
		buf.startpos = buf.readpos;
		svc = ReadByte(&buf);
//		printf("%i\n", svc);
		switch (svc)
		{
		case svc_bad:
			ParseError(&buf);
			Sys_Printf(tv->cluster, "ParseMessage: svc_bad, last was %i\n", lastsvc);
			return;
		case svc_nop:	//quakeworld isn't meant to send these.
			QTV_Printf(tv, "nop\n");
			break;

		case svc_disconnect:
			//mvdsv safely terminates it's mvds with an svc_disconnect.
			//the client is meant to read that and disconnect without reading the intentionally corrupt packet following it.
			//however, our demo playback is chained and looping and buffered.
			//so we've already found the end of the source file and restarted parsing.
			
			//in fte at least, the server does give the packet the correct length
			//I hope mvdsv is the same
			if (tv->sourcetype != SRC_DEMO)
			{
#ifndef _MSC_VER
	#warning QTV is meant to disconnect when servers tells it to.
#endif
				QTV_Printf(tv, "Maliciously ignoring svc_disconnect from upstream...\n");	//ideally the client would be the one to close() the socket first so its the one that gets stuck in TIME_WAIT instead of the server.
				// FIXME: Servers are today sending the svc_disconnect in a non-standard way, which makes QTV drop when it shouldn't.
				// Tell the server developers to fix the servers.
				//tv->drop = true;
			}
			else
			{
				while(ReadByte(&buf))
					;
			}
			return;

		case svc_updatestat:
			ParseUpdateStat(tv, &buf, to, mask);
			break;

//#define	svc_version			4	// [long] server version
		case svc_nqsetview:
			ReadShort(&buf);
//no actual handling is done!
			break;
		case svc_sound:
			ParseSound(tv, &buf, to, mask);
			break;
		case svc_nqtime:
			ReadFloat(&buf);
//no actual handling is done!
			break;

		case svc_print:
			ParsePrint(tv, &buf, to, mask);
			break;

		case svc_stufftext:
			ParseStufftext(tv, &buf, to, mask);
			break;

		case svc_setangle:
			if (!tv->usequakeworldprotocols)
				ReadByte(&buf);
			tv->proxyplayerangles[0] = ReadAngle(&buf, tv->pext1);
			tv->proxyplayerangles[1] = ReadAngle(&buf, tv->pext1);
			tv->proxyplayerangles[2] = ReadAngle(&buf, tv->pext1);

			if (tv->usequakeworldprotocols && tv->controller)
				SendBufferToViewer(tv->controller, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, true);

			/*{
				char nq[7];
				nq[0] = svc_setangle;
				nq[1] = tv->proxyplayerangles[0];
				nq[2] = tv->proxyplayerangles[1];
				nq[3] = tv->proxyplayerangles[2];
//				Multicast(tv, nq, 4, to, mask, Q1);
			}*/
			break;

		case svc_serverdata:
			ParseServerData(tv, &buf, to, mask);
			break;

		case svc_lightstyle:
			ParseLightstyle(tv, &buf);
			break;

//#define	svc_updatename		13	// [qbyte] [string]

		case svc_updatefrags:
			ParseUpdateFrags(tv, &buf, to, mask);
			break;

//#define	svc_clientdata		15	// <shortbits + data>
//#define	svc_stopsound		16	// <see code>
//#define	svc_updatecolors	17	// [qbyte] [qbyte] [qbyte]

		case svc_particle:
			ReadCoord(&buf, tv->pext1);
			ReadCoord(&buf, tv->pext1);
			ReadCoord(&buf, tv->pext1);
			ReadByte(&buf);
			ReadByte(&buf);
			ReadByte(&buf);
			ReadByte(&buf);
			ReadByte(&buf);
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, dem_read, (unsigned)-1, Q1);
			break;

		case svc_damage:
			ParseDamage(tv, &buf, to, mask);
			break;

		case svc_spawnstatic:
			ParseSpawnStatic(tv, &buf, to, mask, false);
			break;

		case svcfte_spawnstatic2:
			if (tv->pext1 & PEXT_SPAWNSTATIC2)
				ParseSpawnStatic(tv, &buf, to, mask, true);
			else
				goto badsvc;
			break;

		case svc_spawnbaseline:
			ParseBaseline(tv, &buf, to, mask, false);
			break;
		case svcfte_spawnbaseline2:
			if (tv->pext1 & PEXT_SPAWNSTATIC2)
				ParseBaseline(tv, &buf, to, mask, true);
			else
				goto badsvc;
			break;

		case svc_temp_entity:
			ParseTempEntity(tv, &buf, to, mask);
			break;

		case svc_setpause:	// [qbyte] on / off
			tv->map.ispaused = ReadByte(&buf);
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, dem_read, (unsigned)-1, Q1);
			break;

//#define	svc_signonnum		25	// [qbyte]  used for the signon sequence

		case svc_centerprint:
			ParseCenterprint(tv, &buf, to, mask);
			break;

		case svc_spawnstaticsound:
			ParseStaticSound(tv, &buf, to, mask);
			break;

		case svc_intermission:
			ParseIntermission(tv, &buf, to, mask);
			break;

		case svc_finale:
			while(ReadByte(&buf))
				;
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, dem_read, (unsigned)-1, Q1);
			break;

		case svc_cdtrack:
			ParseCDTrack(tv, &buf, to, mask);
			break;

		case svc_sellscreen:
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, dem_read, (unsigned)-1, Q1);
			break;

//#define svc_cutscene		34	//hmm... nq only... added after qw tree splitt?

		case svc_smallkick:
		case svc_bigkick:
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, QW);
			break;

		case svc_updateping:
			ParseUpdatePing(tv, &buf, to, mask);
			break;

		case svc_updateentertime:
			ParseUpdateEnterTime(tv, &buf, to, mask);
			break;

		case svc_updatestatlong:
			ParseUpdateStatLong(tv, &buf, to, mask);
			break;

		case svc_muzzleflash:
			ReadShort(&buf);
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, QW);
			break;

		case svc_updateuserinfo:
			ParseUpdateUserinfo(tv, &buf, to, mask);
			break;

		case svc_download:	// [short] size [size bytes]
			ParseDownload(tv, &buf);
			break;

		case svc_playerinfo:
			ParsePlayerInfo(tv, &buf, clearoldplayers);
			clearoldplayers = false;
			break;

		case svc_nails:
			ParseNails(tv, &buf, false);
			break;
		case svc_chokecount:
			ReadByte(&buf);
			break;

		case svcfte_modellistshort:
		case svc_modellist:
			i = ParseList(tv, &buf, tv->map.modellist, to, mask, svc==svcfte_modellistshort);
			if (!i)
			{
				int j;
				if (tv->map.bsp)
					BSP_Free(tv->map.bsp);

				if (tv->cluster->nobsp)// || !tv->usequkeworldprotocols)
					tv->map.bsp = NULL;
				else
					tv->map.bsp = BSP_LoadModel(tv->cluster, tv->map.gamedir, tv->map.modellist[1].name);

				tv->map.numinlines = 0;
				for (j = 2; j < 256; j++)
				{
					if (*tv->map.modellist[j].name != '*')
						break;
					tv->map.numinlines = j;
				}

				tv->map.modelindex_player = 0;
				tv->map.modelindex_spike = 0;
				for (j = 2; j < 256; j++)
				{
					if (!*tv->map.modellist[j].name)
						break;
					if (!strcmp(tv->map.modellist[j].name, "progs/player.mdl"))
						tv->map.modelindex_player = j;
					if (!strcmp(tv->map.modellist[j].name, "progs/spike.mdl"))
						tv->map.modelindex_spike = j;
				}
				strcpy(tv->status, "Prespawning\n");
			}
			ConnectionData(tv, (void*)((char*)buf.data+buf.startpos), buf.readpos - buf.startpos, to, mask, QW);
			if ((!tv->controller || tv->controller->netchan.isnqprotocol) && tv->usequakeworldprotocols)
			{
				if (i)
					SendClientCommand(tv, "modellist %i %i\n", tv->clservercount, i);
				else if (!tv->map.bsp && !tv->cluster->nobsp)
				{
					if (tv->downloadfile)
					{
						fclose(tv->downloadfile);
						unlink(tv->downloadname);
						Sys_Printf(tv->cluster, "Was already downloading %s\nOld download canceled\n", tv->downloadname);
						tv->downloadfile = NULL;
					}
					snprintf(tv->downloadname, sizeof(tv->downloadname), "%s/%s.tmp", (*tv->map.gamedir)?tv->map.gamedir:"id1", tv->map.modellist[1].name);
					QTV_mkdir(tv->downloadname);
					tv->downloadfile = fopen(tv->downloadname, "wb");
					if (!tv->downloadfile)
					{
						Sys_Printf(tv->cluster, "Couldn't open temporary file %s\n", tv->downloadname);

						SendClientCommand(tv, "prespawn %i 0 %i\n", tv->clservercount, LittleLong(BSP_Checksum(tv->map.bsp)));
					}
					else
					{
						char buffer[512];

						strcpy(tv->status, "Downloading map\n");
						Sys_Printf(tv->cluster, "Attempting download of %s\n", tv->downloadname);
						SendClientCommand(tv, "download %s\n", tv->map.modellist[1].name);

						snprintf(buffer, sizeof(buffer), "[QTV] Attempting map download (%s)\n", tv->map.modellist[1].name);
						QW_StreamPrint(tv->cluster, tv, NULL, buffer);
					}
				}
				else
				{
					SendClientCommand(tv, "prespawn %i 0 %i\n", tv->clservercount, LittleLong(BSP_Checksum(tv->map.bsp)));
				}
			}
			break;
		case svcfte_soundlistshort:
		case svc_soundlist:
			i = ParseList(tv, &buf, tv->map.soundlist, to, mask, svc==svcfte_soundlistshort);
			if (!i)
				strcpy(tv->status, "Receiving modellist\n");
			ConnectionData(tv, (void*)((char*)buf.data+buf.startpos), buf.readpos - buf.startpos, to, mask, QW);
			if ((!tv->controller || tv->controller->netchan.isnqprotocol) && tv->usequakeworldprotocols)
			{
				if (i)
					SendClientCommand(tv, "soundlist %i %i\n", tv->clservercount, i);
				else
					SendClientCommand(tv, "modellist %i 0\n", tv->clservercount);
			}
			break;

		case svc_packetentities:
//			FlushPacketEntities(tv);
			ParsePacketEntities(tv, &buf, -1);
			break;
		case svc_deltapacketentities:
			ParsePacketEntities(tv, &buf, ReadByte(&buf));
			break;

		case svc_entgravity:		// gravity change, for prediction
			ReadFloat(&buf);
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, QW);
			break;
		case svc_maxspeed:			// maxspeed change, for prediction
			ReadFloat(&buf);
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, QW);
			break;
		case svc_setinfo:
			ParseSetInfo(tv, &buf);
			break;
		case svc_serverinfo:
			ParseServerinfo(tv, &buf);
			break;
		case svc_updatepl:
			ParsePacketloss(tv, &buf, to, mask);
			break;
		case svc_nails2:
			ParseNails(tv, &buf, true);
			break;

		case svc_killedmonster:
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, Q1);
			break;
		case svc_foundsecret:
			Multicast(tv, (char*)buf.data+buf.startpos, buf.readpos - buf.startpos, to, mask, Q1);
			break;

		default:
		badsvc:
			buf.readpos = buf.startpos;
			Sys_Printf(tv->cluster, "Can't handle svc %i, last was %i\n", (unsigned int)ReadByte(&buf), lastsvc);
			return;
		}
	}
}

