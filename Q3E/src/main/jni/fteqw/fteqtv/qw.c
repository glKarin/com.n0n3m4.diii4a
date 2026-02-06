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
#include <string.h>
#include <time.h>

static const filename_t ConnectionlessModelList[] = {{""}, {"maps/start.bsp"}, {"progs/player.mdl"}, {""}};
static const filename_t ConnectionlessSoundList[] = {{""}, {""}};
const entity_state_t nullentstate = {0};
void SV_WriteDelta(int entnum, const entity_state_t *from, const entity_state_t *to, netmsg_t *msg, qboolean force, unsigned int pext);

const intermission_t nullstreamspot = {{544, 288, 64}, {0, 90, 0}};

void QTV_Say(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *message, qboolean noupwards);

void QTV_DefaultMovevars(movevars_t *vars)
{
	vars->gravity = 800;
	vars->maxspeed = 320;
	vars->spectatormaxspeed = 500;
	vars->accelerate = 10;
	vars->airaccelerate = 0.7f;
	vars->waterfriction = 4;
	vars->entgrav = 1;
	vars->stopspeed = 10;
	vars->wateraccelerate = 10;
	vars->friction = 4;
}


const usercmd_t nullcmd = {0};

#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)
void ReadDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = ReadByte (m);

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = (ReadShort (m)/(float)0x10000)*360;
	if (bits & CM_ANGLE2)
		move->angles[1] = (ReadShort (m)/(float)0x10000)*360;
	if (bits & CM_ANGLE3)
		move->angles[2] = (ReadShort (m)/(float)0x10000)*360;

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = ReadShort(m);
	if (bits & CM_SIDE)
		move->sidemove = ReadShort(m);
	if (bits & CM_UP)
		move->upmove = ReadShort(m);

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = ReadByte (m);

	if (bits & CM_IMPULSE)
		move->impulse = ReadByte (m);

// read time to run command
	move->msec = ReadByte (m);		// always sent
}

void WriteDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits = 0;

	if (move->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (move->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (move->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;

	if (move->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (move->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (move->upmove != from->upmove)
		bits |= CM_UP;

	if (move->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (move->impulse != from->impulse)
		bits |= CM_IMPULSE;


	WriteByte (m, bits);

// read current angles
	if (bits & CM_ANGLE1)
		WriteShort (m, (move->angles[0]/360.0)*0x10000);
	if (bits & CM_ANGLE2)
		WriteShort (m, (move->angles[1]/360.0)*0x10000);
	if (bits & CM_ANGLE3)
		WriteShort (m, (move->angles[2]/360.0)*0x10000);

// read movement
	if (bits & CM_FORWARD)
		WriteShort(m, move->forwardmove);
	if (bits & CM_SIDE)
		WriteShort(m, move->sidemove);
	if (bits & CM_UP)
		WriteShort(m, move->upmove);

// read buttons
	if (bits & CM_BUTTONS)
		WriteByte (m, move->buttons);

	if (bits & CM_IMPULSE)
		WriteByte (m, move->impulse);

// read time to run command
	WriteByte (m, move->msec);		// always sent
}












void BuildServerData(sv_t *tv, netmsg_t *msg, int servercount, viewer_t *viewer)
{
	movevars_t movevars;
	WriteByte(msg, svc_serverdata);
	if (viewer)
	{	//its for an actual viewer, tailor the extensions...
		if (viewer->pext1)
		{
			WriteLong(msg, PROTOCOL_VERSION_FTE);
			WriteLong(msg, viewer->pext1);
		}
		if (viewer->pext2)
		{
			WriteLong(msg, PROTOCOL_VERSION_FTE2);
			WriteLong(msg, viewer->pext2);
		}
	}
	else
	{	//we're just forwarding, use the same extensions our source used.
		if (tv->pext1)
		{
			WriteLong(msg, PROTOCOL_VERSION_FTE);
			WriteLong(msg, tv->pext1);
		}
		if (tv->pext2)
		{
			WriteLong(msg, PROTOCOL_VERSION_FTE2);
			WriteLong(msg, tv->pext2);
		}
		if (tv->pexte)
		{
			WriteLong(msg, PROTOCOL_VERSION_EZQUAKE1);
			WriteLong(msg, tv->pexte);
		}
	}
	WriteLong(msg, PROTOCOL_VERSION);
	WriteLong(msg, servercount);

	if (!tv)
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "qw");

		if (!viewer)
			WriteFloat(msg, 0);
		else
			WriteByte(msg, (MAX_CLIENTS-1) | (128));
		WriteString(msg, "FTEQTV Proxy");


		// get the movevars
		QTV_DefaultMovevars(&movevars);
		WriteFloat(msg, movevars.gravity);
		WriteFloat(msg, movevars.stopspeed);
		WriteFloat(msg, movevars.maxspeed);
		WriteFloat(msg, movevars.spectatormaxspeed);
		WriteFloat(msg, movevars.accelerate);
		WriteFloat(msg, movevars.airaccelerate);
		WriteFloat(msg, movevars.wateraccelerate);
		WriteFloat(msg, movevars.friction);
		WriteFloat(msg, movevars.waterfriction);
		WriteFloat(msg, movevars.entgrav);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, "\\*QTV\\"QTV_VERSION_STRING);
		WriteString(msg, "\"\n");

	}
	else
	{
		WriteString(msg, tv->map.gamedir);

		if (!viewer)
			WriteFloat(msg, 0);
		else
		{
			if (tv->controller == viewer)
				WriteByte(msg, viewer->thisplayer);
			else
				WriteByte(msg, viewer->thisplayer | 128);
		}
		WriteString(msg, tv->map.mapname);


		// get the movevars
		WriteFloat(msg, tv->map.movevars.gravity);
		WriteFloat(msg, tv->map.movevars.stopspeed);
		WriteFloat(msg, tv->map.movevars.maxspeed);
		WriteFloat(msg, tv->map.movevars.spectatormaxspeed);
		WriteFloat(msg, tv->map.movevars.accelerate);
		WriteFloat(msg, tv->map.movevars.airaccelerate);
		WriteFloat(msg, tv->map.movevars.wateraccelerate);
		WriteFloat(msg, tv->map.movevars.friction);
		WriteFloat(msg, tv->map.movevars.waterfriction);
		WriteFloat(msg, tv->map.movevars.entgrav);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, tv->map.serverinfo);
		WriteString(msg, "\"\n");
	}
}
void BuildNQServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int playernum)
{
	int i;
	WriteByte(msg, svc_serverdata);
	if (tv && tv->pext1 & PEXT_FLOATCOORDS)
	{
		WriteLong(msg, 999);
		WriteLong(msg, (1<<1)|(1<<4));	//short angles, float coords, same as PEXT_FLOATCOORDS
	}
	else
		WriteLong(msg, PROTOCOL_VERSION_NQ);
	WriteByte(msg, 16);	//MAX_CLIENTS
	WriteByte(msg, 1);	//game type

	if (!tv || tv->parsingconnectiondata )
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "FTEQTV Proxy");


		//modellist
		for (i = 1; *ConnectionlessModelList[i].name; i++)
		{
			WriteString(msg, ConnectionlessModelList[i].name);
		}
		WriteString(msg, "");

		//soundlist
		for (i = 1; *ConnectionlessSoundList[i].name; i++)
		{
			WriteString(msg, ConnectionlessSoundList[i].name);
		}
		WriteString(msg, "");

		WriteByte(msg, svc_cdtrack);
		WriteByte(msg, 0);	//two of them, yeah... weird, eh?
		WriteByte(msg, 0);

		WriteByte(msg, svc_nqsetview);
		WriteShort(msg, playernum+1);

		WriteByte(msg, svc_nqsignonnum);
		WriteByte(msg, 1);
	}
	else
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, tv->map.mapname);


		//modellist
		for (i = 1; *tv->map.modellist[i].name; i++)
		{
			WriteString(msg, tv->map.modellist[i].name);
		}
		WriteString(msg, "");

		//soundlist
		for (i = 1; *tv->map.soundlist[i].name; i++)
		{
			WriteString(msg, tv->map.soundlist[i].name);
		}
		WriteString(msg, "");

		WriteByte(msg, svc_cdtrack);
		WriteByte(msg, tv->map.cdtrack);	//two of them, yeah... weird, eh?
		WriteByte(msg, tv->map.cdtrack);

		WriteByte(msg, svc_nqsetview);
		WriteShort(msg, playernum+1);

		WriteByte(msg, svc_nqsignonnum);
		WriteByte(msg, 1);
	}
}


void QW_ClearViewerState(viewer_t *viewer)
{
	memset(viewer->currentstats, 0, sizeof(viewer->currentstats));
}

void SendServerData(sv_t *tv, viewer_t *viewer)
{
	netmsg_t msg;
	char buffer[MAX_MSGLEN];

	InitNetMsg(&msg, buffer, viewer->netchan.maxreliablelen);

	if (tv)
	{
		viewer->pext1 = tv->pext1;
		viewer->pext2 = tv->pext2;
	}
	else
		viewer->pext1 = viewer->pext2 = 0;

	if (tv && (tv->controller == viewer || !tv->controller))
		viewer->thisplayer = tv->map.thisplayer;
	else
		viewer->thisplayer = viewer->netchan.isnqprotocol?15:MAX_CLIENTS-1;
	if (viewer->netchan.isnqprotocol)
		BuildNQServerData(tv, &msg, false, viewer->thisplayer);
	else
		BuildServerData(tv, &msg, viewer->servercount, viewer);

	SendBufferToViewer(viewer, msg.data, msg.cursize, true);

	viewer->thinksitsconnected = false;
	if (tv && (tv->controller == viewer) && !viewer->netchan.isnqprotocol)
		viewer->thinksitsconnected = true;

	QW_ClearViewerState(viewer);
}

void SendNQSpawnInfoToViewer(cluster_t *cluster, viewer_t *viewer, netmsg_t *msg)
{
	char buffer[64];
	int i;
	int colours;
	sv_t *tv = viewer->server;
	WriteByte(msg, svc_nqtime);
	WriteFloat(msg, (cluster->curtime - (tv?tv->mapstarttime:0))/1000.0f);

	if (tv)
	{
		for (i=0; i<MAX_CLIENTS && i < 16; i++)
		{
			WriteByte (msg, svc_nqupdatename);
			WriteByte (msg, i);
			Info_ValueForKey(tv->map.players[i].userinfo, "name", buffer, sizeof(buffer));
			WriteString (msg, buffer);	//fixme

			WriteByte (msg, svc_updatefrags);
			WriteByte (msg, i);
			WriteShort (msg, tv->map.players[i].frags);

			Info_ValueForKey(tv->map.players[i].userinfo, "bottomcolor", buffer, sizeof(buffer));
			colours = atoi(buffer);
			Info_ValueForKey(tv->map.players[i].userinfo, "topcolor", buffer, sizeof(buffer));
			colours |= atoi(buffer)*16;
			WriteByte (msg, svc_nqupdatecolors);
			WriteByte (msg, i);
			WriteByte (msg, colours);
		}
	}
	else
	{
		for (i=0; i < 16; i++)
		{
			WriteByte (msg, svc_nqupdatename);
			WriteByte (msg, i);
			WriteString (msg, "");
			WriteByte (msg, svc_updatefrags);
			WriteByte (msg, i);
			WriteShort (msg, 0);
			WriteByte (msg, svc_nqupdatecolors);
			WriteByte (msg, i);
			WriteByte (msg, 0);
		}
	}

	WriteByte(msg, svc_nqsignonnum);
	WriteByte(msg, 3);
}

int SendCurrentUserinfos(sv_t *tv, int cursize, netmsg_t *msg, int i, int thisplayer)
{
	char name[1024];

	if (i < 0)
		return i;
	if (i >= MAX_CLIENTS)
		return i;

	for (; i < MAX_CLIENTS; i++)
	{
		if (i == thisplayer && (!tv || !(tv->controller || tv->proxyplayer)))
		{
			WriteByte(msg, svc_updateuserinfo);
			WriteByte(msg, i);
			WriteLong(msg, i+1);
			WriteString2(msg, "\\*spectator\\1\\name\\");

			// Print the number of people on QTV along with the hostname
			if (tv && tv->map.hostname[0])
			{
				if (tv->map.hostname[0])
					snprintf(name, sizeof(name), "[%d] %s", tv->numviewers, tv->map.hostname);
				else
					snprintf(name, sizeof(name), "[%d] FTEQTV", tv->numviewers);
			}
			else
				snprintf(name, sizeof(name), "FTEQTV");

			/*
			if (tv)
			{
				char tmp[MAX_QPATH];
				snprintf(tmp
				strlcat(name, itoa(tv->numviewers), sizeof(name));
				//snprintf(name, sizeof(name), "%s %d", name, tv->numviewers);
			}
			*/
			WriteString(msg, name);

			WriteByte(msg, svc_updatefrags);
			WriteByte(msg, i);
			WriteShort(msg, 9999);

			WriteByte(msg, svc_updateping);
			WriteByte(msg, i);
			WriteShort(msg, 0);

			WriteByte(msg, svc_updatepl);
			WriteByte(msg, i);
			WriteByte(msg, 0);

			continue;
		}
		if (!tv)
			continue;
		if (msg->cursize+cursize+strlen(tv->map.players[i].userinfo) > 768)
		{
			return i;
		}
		WriteByte(msg, svc_updateuserinfo);
		WriteByte(msg, i);
		WriteLong(msg, i+1);
		WriteString(msg, tv->map.players[i].userinfo);

		WriteByte(msg, svc_updatefrags);
		WriteByte(msg, i);
		WriteShort(msg, tv->map.players[i].frags);

		WriteByte(msg, svc_updateping);
		WriteByte(msg, i);
		WriteShort(msg, tv->map.players[i].ping);

		WriteByte(msg, svc_updatepl);
		WriteByte(msg, i);
		WriteByte(msg, tv->map.players[i].packetloss);
	}

	i++;

	return i;
}
void WriteEntityState(netmsg_t *msg, entity_state_t *es, unsigned int pext)
{
	int i;
	WriteByte(msg, es->modelindex);
	WriteByte(msg, es->frame);
	WriteByte(msg, es->colormap);
	WriteByte(msg, es->skinnum);
	for (i = 0; i < 3; i++)
	{
		WriteCoord(msg, es->origin[i], pext);
		WriteAngle(msg, es->angles[i], pext);
	}
}
int SendCurrentBaselines(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{

	if (i < 0 || i >= MAX_ENTITIES)
		return i;

	for (; i < MAX_ENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}

		if (tv->map.entity[i].baseline.modelindex)
		{
			if (tv->pext1 & PEXT_SPAWNSTATIC2)
			{
				WriteByte(msg, svcfte_spawnbaseline2);
				SV_WriteDelta(i, &nullentstate, &tv->map.entity[i].baseline, msg, true, tv->pext1);
			}
			else
			{
				WriteByte(msg, svc_spawnbaseline);
				WriteShort(msg, i);
				WriteEntityState(msg, &tv->map.entity[i].baseline, tv->pext1);
			}
		}
	}

	return i;
}
int SendCurrentLightmaps(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_LIGHTSTYLES)
		return i;

	for (; i < MAX_LIGHTSTYLES; i++)
	{
		if (msg->cursize+cursize+strlen(tv->map.lightstyle[i].name) > maxbuffersize)
		{
			return i;
		}
		WriteByte(msg, svc_lightstyle);
		WriteByte(msg, i);
		WriteString(msg, tv->map.lightstyle[i].name);
	}
	return i;
}
int SendStaticSounds(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICSOUNDS)
		return i;

	for (; i < MAX_STATICSOUNDS; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->map.staticsound[i].soundindex)
			continue;

		WriteByte(msg, svc_spawnstaticsound);
		WriteCoord(msg, tv->map.staticsound[i].origin[0], tv->pext1);
		WriteCoord(msg, tv->map.staticsound[i].origin[1], tv->pext1);
		WriteCoord(msg, tv->map.staticsound[i].origin[2], tv->pext1);
		WriteByte(msg, tv->map.staticsound[i].soundindex);
		WriteByte(msg, tv->map.staticsound[i].volume);
		WriteByte(msg, tv->map.staticsound[i].attenuation);
	}

	return i;
}
int SendStaticEntities(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICENTITIES)
		return i;

	for (; i < MAX_STATICENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->map.spawnstatic[i].modelindex)
			continue;

		if (tv->pext1 & PEXT_SPAWNSTATIC2)
		{
			WriteByte(msg, svcfte_spawnstatic2);
			SV_WriteDelta(i, &nullentstate, &tv->map.spawnstatic[i], msg, true, tv->pext1);
		}
		else
		{
			WriteByte(msg, svc_spawnstatic);
			WriteEntityState(msg, &tv->map.spawnstatic[i], tv->pext1);
		}
	}

	return i;
}

int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg)
{
	int i;

	WriteByte(msg, svc);
	WriteByte(msg, first);
	for (i = first+1; i < 256; i++)
	{
//		printf("write %i: %s\n", i, list[i].name);
		WriteString(msg, list[i].name);
		if (!*list[i].name)	//fixme: this probably needs testing for where we are close to the limit
		{	//no more
			WriteByte(msg, 0);
			return -1;
		}

		if (msg->cursize > 768)
		{	//truncate
			i--;
			break;
		}
	}
	WriteByte(msg, 0);
	WriteByte(msg, i);

	return i;
}

void QW_StreamPrint(cluster_t *cluster, sv_t *server, viewer_t *allbut, char *message)
{
	viewer_t *v;

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->server == server)
		{
			if (v == allbut)
				continue;
			QW_PrintfToViewer(v, "%s", message);
		}
	}
}


void QW_StreamStuffcmd(cluster_t *cluster, sv_t *server, char *fmt, ...)
{
	viewer_t *v;
	va_list		argptr;
	char buf[1024];
	char cmd[512];

	netmsg_t msg;

	va_start (argptr, fmt);
	vsnprintf (cmd, sizeof(cmd), fmt, argptr);
	va_end (argptr);

	InitNetMsg(&msg, buf, sizeof(buf));
	WriteByte(&msg, svc_stufftext);
	WriteString(&msg, cmd);


	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->server == server)
		{
			SendBufferToViewer(v, msg.data, msg.cursize, true);
		}
	}
}



void QW_SetViewersServer(cluster_t *cluster, viewer_t *viewer, sv_t *sv)
{
	char buffer[1024];
	sv_t *oldserver;
	oldserver = viewer->server;
	if (viewer->server)
		viewer->server->numviewers--;
	viewer->server = sv;
	if (viewer->server)
		viewer->server->numviewers++;
	if (!sv || !sv->parsingconnectiondata)
	{
		if (sv != oldserver)
			QW_StuffcmdToViewer(viewer, "cmd new\n");
		viewer->thinksitsconnected = false;
	}
	viewer->servercount++;
	viewer->origin[0] = 0;
	viewer->origin[1] = 0;
	viewer->origin[2] = 0;

	if (sv != oldserver)
	{
		if (sv && oldserver)
		{
			snprintf(buffer, sizeof(buffer), "%cQTV%c%s leaves to watch %s (%i)\n", 91+128, 93+128, viewer->name, *sv->map.hostname?sv->map.hostname:sv->server, sv->streamid);
			QW_StreamPrint(cluster, oldserver, viewer, buffer);
		}
		snprintf(buffer, sizeof(buffer), "%cQTV%c%s joins the stream\n", 91+128, 93+128, viewer->name);
		QW_StreamPrint(cluster, sv, viewer, buffer);
	}
}

//fixme: will these want to have state?..
int NewChallenge(cluster_t *cluster, netadr_t *addr)
{
	unsigned int r = 0, l;
	unsigned char *digest;
	void *ctx;
	hashfunc_t *func = &hash_sha1;
	static time_t t;

	//reminder: Challenges exist so clients can't spoof their source address and waste our ram without us being able to ban them without banning everyone.
	size_t sz = 0;
	if (((struct sockaddr*)addr->sockaddr)->sa_family == AF_INET)
		sz = sizeof(struct sockaddr_in);
	else if (((struct sockaddr*)addr->sockaddr)->sa_family == AF_INET6)
		sz = sizeof(struct sockaddr_in6);
	//else error

	ctx = alloca(func->contextsize);
	func->init(ctx);
	if (!t)	//must be constant, so only do this if its still 0.
		t = time(NULL);
	func->process(ctx, addr, sz);		//hash their address primarily.
	func->process(ctx, cluster->turnkey, sizeof(cluster->turnkey));	//might not be set...
	func->process(ctx, &t, sizeof(t));	//extra privacy, sizeof doesn't matter as its only our process that cares
	//func->process(ctx, cluster, sizeof(cluster));	//a random pointer too, because zomgwtf

	digest = alloca(func->digestsize);
	func->terminate(digest, ctx);
	for (l = 0; l < func->digestsize; l++)
		r ^= digest[l]<<((l%sizeof(r))*8);
	return r;
}
qboolean ChallengePasses(cluster_t *cluster, netadr_t *addr, int challenge)
{
	if (challenge == NewChallenge(cluster, addr))
		return true;
	return false;
}

void NewClient(cluster_t *cluster, viewer_t *viewer)
{
	sv_t *initialserver;
	initialserver = NULL;

	if (viewer->netchan.remote_address.tcpcon)
	{
		tcpconnect_t *dest;
		for (dest = cluster->tcpconnects; dest; dest = dest->next)
		{
			if (dest == viewer->netchan.remote_address.tcpcon)
				break;
		}

		if (*dest->initialstreamname)
		{
			initialserver = QTV_NewServerConnection(cluster, 0, dest->initialstreamname, "", false, AD_WHENEMPTY, true, false);
			if (initialserver && initialserver->sourcetype == SRC_UDP)
				initialserver->controller = viewer;
		}
	}

	if (initialserver)
		;	//already picked it via websocket resources.
	else if (*cluster->autojoinadr)
	{
		initialserver = QTV_NewServerConnection(cluster, 0, cluster->autojoinadr, "", false, AD_WHENEMPTY, true, false);
		if (initialserver && initialserver->sourcetype == SRC_UDP)
			initialserver->controller = viewer;
	}
	else if (cluster->nouserconnects && cluster->numservers == 1)
	{
		initialserver = cluster->servers;
		if (!initialserver->map.modellist[1].name[0])
			initialserver = NULL;	//damn, that server isn't ready
	}

	QW_SetViewersServer(cluster, viewer, initialserver);

	viewer->userid = ++cluster->nextuserid;
	viewer->timeout = cluster->curtime + 15*1000;
	viewer->trackplayer = -1;

	viewer->menunum = -1;
	QW_SetMenu(viewer, MENU_NONE);


#ifndef LIBQTV
	QW_PrintfToViewer(viewer, "Welcome to FTEQTV %s\n", QTV_VERSION_STRING);
	QW_StuffcmdToViewer(viewer, "alias admin \"cmd admin\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:up\" \"say proxy:menu up\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:down\" \"say proxy:menu down\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:right\" \"say proxy:menu right\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:left\" \"say proxy:menu left\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:select\" \"say proxy:menu select\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:home\" \"say proxy:menu home\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:end\" \"say proxy:menu end\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:menu\" \"say proxy:menu\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:backspace\" \"say proxy:menu backspace\"\n");

		QW_StuffcmdToViewer(viewer, "alias \".help\" \"say .help\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".disconnect\" \"say .disconnect\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".menu\" \"say .menu\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".admin\" \"say .admin\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".reset\" \"say .reset\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".clients\" \"say .clients\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".qtv\" \"say .qtv\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".join\" \"say .join\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".observe\" \"say .observe\"\n");

	QW_PrintfToViewer(viewer, "Type admin for the admin menu\n");
#endif
}

void ParseUserInfo(cluster_t *cluster, viewer_t *viewer)
{
	char buf[1024];
	float rate;
	char temp[64];

	viewer->isproxy = false;

	Info_ValueForKey(viewer->userinfo, "*qtv", temp, sizeof(temp));
	if (*temp)
		viewer->isproxy = true;
	Info_ValueForKey(viewer->userinfo, "Qizmo", temp, sizeof(temp));
	if (*temp)
		viewer->isproxy = true;

	Info_ValueForKey(viewer->userinfo, "name", temp, sizeof(temp));

	if (!*temp)
		strcpy(temp, "unnamed");
	if (!*viewer->name)
		Sys_Printf(cluster, "Viewer %s connected\n", temp);

	if (strcmp(viewer->name, temp))
	{
		if (*viewer->name)
		{
			snprintf(buf, sizeof(buf), "%cQTV%c%s changed name to %cQTV%c%s\n",
					91+128, 93+128, viewer->name,
					91+128, 93+128, temp
					);
		}
		else
		{
			snprintf(buf, sizeof(buf), "%cQTV%c%s joins the stream\n",
					91+128, 93+128, temp
					);

		}

		if (!viewer->server || viewer->server->controller != viewer)
			QW_StreamPrint(cluster, viewer->server, NULL, buf);
	}

	strlcpy(viewer->name, temp, sizeof(viewer->name));

	Info_ValueForKey(viewer->userinfo, "rate", temp, sizeof(temp));
	rate = atof(temp);
	if (!rate)
		rate = 2500;
	if (rate < 250)
		rate = 250;
	if (rate > 10000)
		rate = 10000;
	viewer->netchan.rate = 1000.0f / rate;
}

void NewNQClient(cluster_t *cluster, netadr_t *addr)
{
//	sv_t *initialserver;
	int header;
	int len;
	int i;
	unsigned char buffer[64];
	viewer_t *viewer = NULL;


	if (cluster->numviewers >= cluster->maxviewers && cluster->maxviewers)
	{
		buffer[4] = CCREP_REJECT;
		strcpy((char*)buffer+5, "Sorry, proxy is full.\n");
		len = strlen((char*)buffer+5)+5;
	}
/*	else
	{
		buffer[4] = CCREP_REJECT;
		strcpy((char*)buffer+5, "NQ not supported yet\n");
		len = strlen((char*)buffer+5)+5;
	}*/
	else if (!(viewer = malloc(sizeof(viewer_t))))
	{
		buffer[4] = CCREP_REJECT;
		strcpy((char*)buffer+5, "Out of memory\n");
		len = strlen((char*)buffer+5)+5;
	}
	else
	{
		buffer[4] = CCREP_ACCEPT;
		buffer[5] = (cluster->qwlistenportnum&0x00ff)>>0;
		buffer[6] = (cluster->qwlistenportnum&0xff00)>>8;
		buffer[7] = 0;
		buffer[8] = 0;
		len = 4+1+4;
	}

	*(int*)buffer = NETFLAG_CTL | len;
	header = (buffer[0]<<24) + (buffer[1]<<16) + (buffer[2]<<8) + buffer[3];
	*(int*)buffer = header;

	NET_SendPacket (cluster, NET_ChooseSocket(cluster->qwdsocket, addr, *addr), len, buffer, *addr);

	if (!viewer)
		return;


	memset(viewer, 0, sizeof(*viewer));


	Netchan_Setup (NET_ChooseSocket(cluster->qwdsocket, addr, *addr), &viewer->netchan, *addr, 0, false);
	viewer->netchan.isnqprotocol = true;
	viewer->netchan.maxdatagramlen = MAX_NQDATAGRAM;
	viewer->netchan.maxreliablelen = MAX_NQMSGLEN;

	viewer->firstconnect = true;

	viewer->next = cluster->viewers;
	cluster->viewers = viewer;
	for (i = 0; i < ENTITY_FRAMES; i++)
		viewer->delta_frames[i] = -1;

	cluster->numviewers++;

	sprintf(viewer->userinfo, "\\name\\%s", "unnamed");

	ParseUserInfo(cluster, viewer);

	NewClient(cluster, viewer);

	if (!viewer->server)
		QW_StuffcmdToViewer(viewer, "cmd new\n");
}

void NewQWClient(cluster_t *cluster, netadr_t *addr, char *connectmessage)
{
//	sv_t *initialserver;
	viewer_t *viewer;

	char qport[32];
	char challenge[32];
	char infostring[1024];
	char prx[256];
	int i;

	connectmessage+=11;

	connectmessage = COM_ParseToken(connectmessage, qport, sizeof(qport), "");
	connectmessage = COM_ParseToken(connectmessage, challenge, sizeof(challenge), "");
	connectmessage = COM_ParseToken(connectmessage, infostring, sizeof(infostring), "");

	if (!ChallengePasses(cluster, addr, atoi(challenge)))
	{
		Netchan_OutOfBandPrint(cluster, *addr, "n" "Bad challenge");
		return;
	}

	Info_ValueForKey(infostring, "prx", prx,sizeof(prx));
	if (*prx)
	{
		Fwd_NewQWFwd(cluster, addr, prx);
		return;
	}


	viewer = malloc(sizeof(viewer_t));
	if (!viewer)
	{
		Netchan_OutOfBandPrint(cluster, *addr, "n" "Out of memory");
		return;
	}
	memset(viewer, 0, sizeof(viewer_t));

	Netchan_Setup (NET_ChooseSocket(cluster->qwdsocket, addr, *addr), &viewer->netchan, *addr, atoi(qport), false);
	viewer->netchan.message.maxsize = MAX_QWMSGLEN;
	viewer->netchan.maxdatagramlen = MAX_QWMSGLEN;
	viewer->netchan.maxreliablelen = MAX_QWMSGLEN;

	viewer->firstconnect = true;

	viewer->next = cluster->viewers;
	cluster->viewers = viewer;
	for (i = 0; i < ENTITY_FRAMES; i++)
		viewer->delta_frames[i] = -1;

	cluster->numviewers++;

	strlcpy(viewer->userinfo, infostring, sizeof(viewer->userinfo));
	ParseUserInfo(cluster, viewer);

	Netchan_OutOfBandPrint(cluster, *addr, "j");

	NewClient(cluster, viewer);
}

void QW_SetMenu(viewer_t *v, int menunum)
{
	if ((v->menunum==MENU_NONE) != (menunum==MENU_NONE))
	{
		if (v->isproxy)
		{
			if (menunum != MENU_NONE)
				QW_StuffcmdToViewer(v, "//set prox_inmenu 1\n");
			else
				QW_StuffcmdToViewer(v, "//set prox_inmenu 0\n");
		}
		else
		{
			if (menunum != MENU_NONE)
			{
				QW_StuffcmdToViewer(v, "//set prox_inmenu 1\n");

				QW_StuffcmdToViewer(v, "alias \"+proxjump\" \"say proxy:menu enter\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"say proxy:menu up\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxback\" \"say proxy:menu down\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"say proxy:menu left\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxright\" \"say proxy:menu right\"\n");

				QW_StuffcmdToViewer(v, "alias \"-proxjump\" \" \"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \" \"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxback\" \" \"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxleft\" \" \"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxright\" \" \"\n");
			}
			else
			{
				QW_StuffcmdToViewer(v, "//set prox_inmenu 0\n");

				QW_StuffcmdToViewer(v, "alias \"+proxjump\" \"+jump\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"+forward\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxback\" \"+back\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"+moveleft\"\n");
				QW_StuffcmdToViewer(v, "alias \"+proxright\" \"+moveright\"\n");

				QW_StuffcmdToViewer(v, "alias \"-proxjump\" \"-jump\"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \"-forward\"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxback\" \"-back\"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxleft\" \"-moveleft\"\n");
				QW_StuffcmdToViewer(v, "alias \"-proxright\" \"-moveright\"\n");
			}
			QW_StuffcmdToViewer(v, "-forward\n");
			QW_StuffcmdToViewer(v, "-back\n");
			QW_StuffcmdToViewer(v, "-moveleft\n");
			QW_StuffcmdToViewer(v, "-moveright\n");
		}
	}

	if (v->server && v->server->controller == v)
	{
		if (menunum==MENU_NONE || menunum==MENU_FORWARDING)
			v->server->proxyisselected = false;
		else
			v->server->proxyisselected = true;
	}

	v->menunum = menunum;
	v->menuop = 0;

	v->menuspamtime = 0;
}

void QTV_Rcon(cluster_t *cluster, char *message, netadr_t *from)
{
	char buffer[8192];

	char *command;
	int passlen;

	if (!*cluster->adminpassword)
	{
		Netchan_OutOfBandPrint(cluster, *from, "n" "Bad rcon_password.\n");
		return;
	}

	while(*message > '\0' && *message <= ' ')
		message++;

	command = strchr(message, ' ');
	passlen = command-message;
	if (passlen != strlen(cluster->adminpassword) || strncmp(message, cluster->adminpassword, passlen))
	{
		Netchan_OutOfBandPrint(cluster, *from, "n" "Bad rcon_password.\n");
		return;
	}

	Netchan_OutOfBandPrint(cluster, *from, "n%s", Rcon_Command(cluster, NULL, command, buffer, sizeof(buffer), false));
}

void QTV_Status(cluster_t *cluster, netadr_t *from)
{
	int i;
	char buffer[8192];
	sv_t *sv;

	netmsg_t msg;
	char elem[256];
	InitNetMsg(&msg, buffer, sizeof(buffer));
	WriteLong(&msg, -1);
	WriteByte(&msg, 'n');

	WriteString2(&msg, "\\*QTV\\");
	WriteString2(&msg, QTV_VERSION_STRING);

	if (cluster->numservers==1)
	{	//show this server's info
		sv = cluster->servers;

		WriteString2(&msg, sv->map.serverinfo);
		WriteString2(&msg, "\n");

		for (i = 0;i < MAX_CLIENTS; i++)
		{
			if (i == sv->map.thisplayer)
				continue;
			if (!sv->map.players[i].active)
				continue;
			//userid
			sprintf(elem, "%i", i);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//frags
			sprintf(elem, "%i", sv->map.players[i].frags);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//time (minuites)
			sprintf(elem, "%i", 0);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//ping
			sprintf(elem, "%i", sv->map.players[i].ping);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//name
			Info_ValueForKey(sv->map.players[i].userinfo, "name", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");

			//skin
			Info_ValueForKey(sv->map.players[i].userinfo, "skin", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");
			WriteString2(&msg, " ");

			//tc
			Info_ValueForKey(sv->map.players[i].userinfo, "topcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//bc
			Info_ValueForKey(sv->map.players[i].userinfo, "bottomcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			WriteString2(&msg, "\n");
		}
	}
	else
	{
		WriteString2(&msg, "\\hostname\\");
		WriteString2(&msg, cluster->hostname);

		for (sv = cluster->servers, i = 0; sv; sv = sv->next, i++)
		{
			sprintf(elem, "\\%i\\", sv->streamid);
			WriteString2(&msg, elem);
			WriteString2(&msg, sv->server);
//			sprintf(elem, " (%s)", sv->serveraddress);
//			WriteString2(&msg, elem);
		}

		WriteString2(&msg, "\n");
	}

	WriteByte(&msg, 0);
	NET_SendPacket(cluster, NET_ChooseSocket(cluster->qwdsocket, from, *from), msg.cursize, msg.data, *from);
}
static void QTV_GetInfo(cluster_t *cluster, netadr_t *from, char *args)
{
	//ftemaster support
	char challenge[256], tmp[64];
	char protocolname[MAX_QPATH];
	char buffer[8192];
	netmsg_t msg;
	qboolean authed = false;
	InitNetMsg(&msg, buffer, sizeof(buffer));

	args = COM_ParseToken(args, challenge, sizeof(challenge), "");
	while((args = COM_ParseToken(args, tmp, sizeof(tmp), "")))
	{
		if (!strncmp(tmp, "c=",2) && !strcmp(tmp+2, cluster->chalkey))
			authed = true;	//they're able to read our outgoing packets. assume not intercepted (at least blocks spoofed packets). should really use (d)tls. this is more to protect our resources than anything else though, so doesn't need to be strong.
		else if (!strncmp(tmp, "a=",2) && authed)
		{
			netadr_t adr;
			if (NET_StringToAddr(tmp+2, &adr, 0))
			{	//master told us our IP. we can use that to report to turn clients
				if (((struct sockaddr*)&adr.sockaddr)->sa_family == AF_INET)
					memcpy(cluster->turn_ipv4, &((struct sockaddr_in*)&adr.sockaddr)->sin_addr, 4);
				else if (((struct sockaddr*)&adr.sockaddr)->sa_family == AF_INET6)
					memcpy(cluster->turn_ipv6, &((struct sockaddr_in6*)&adr.sockaddr)->sin6_addr, 16);
			}
		}
	}
	COM_ParseToken(cluster->protocolname?cluster->protocolname:"FTE-Quake", protocolname, sizeof(protocolname), "");	//we can only report one, so report the first.

	//response packet header
	WriteLong(&msg, ~0u);
//	if (fullstatus)
//		WriteString2(&msg, "statusResponse\n");
//	else
		WriteString2(&msg, "infoResponse\n");

	//first line contains the serverinfo, or some form of it
	WriteString2(&msg, "\\*QTV\\");				WriteString2(&msg, QTV_VERSION_STRING);
//	WriteString2(&msg, "\\*fp\\");				WriteString2(&msg, hash(cert));
	if (authed)
	{	//only reported to the master server to generate time-based auth tokens.
		tobase64(tmp,sizeof(tmp), cluster->turnkey, sizeof(cluster->turnkey));
		WriteString2(&msg, "\\_turnkey\\");			WriteString2(&msg, tmp);
	}
	WriteString2(&msg, "\\challenge\\");		WriteString2(&msg, challenge);
	WriteString2(&msg, "\\gamename\\");			WriteString2(&msg, protocolname);
	snprintf(tmp, sizeof(tmp), "%i%s", cluster->protocolname?cluster->protocolver:3, "t"); //'w':quakeworld, 'n'/'d':netquake, 'x':qe, 't':qtv, 'r':turnrelay, 'f':fwd
	WriteString2(&msg, "\\protocol\\");			WriteString2(&msg, tmp);
	WriteString2(&msg, "\\clients\\");			WriteString2(&msg, "0");
	WriteString2(&msg, "\\sv_maxclients\\");	WriteString2(&msg, "0");
	WriteString2(&msg, "\\modname\\");			WriteString2(&msg, "QTV");
	WriteString2(&msg, "\\mapname\\");			WriteString2(&msg, "QTV");
	WriteString2(&msg, "\\hostname\\");			WriteString2(&msg, cluster->hostname);
	snprintf(tmp, sizeof(tmp), "%i", cluster->tcplistenportnum);
	WriteString2(&msg, "\\sv_port_tcp\\");		WriteString2(&msg, tmp);

	/*if (fullstatus)
	{
		client_t *cl;
		char *start = resp;

		if (resp != response+sizeof(response))
		{
			resp[-1] = '\n';	//replace the null terminator that we already wrote

			//on the following lines we have an entry for each client
			for (i=0 ; i<svs.allocated_client_slots ; i++)
			{
				cl = &svs.clients[i];
				if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && !cl->spectator)
				{
					Q_strncpyz(resp, va(
									"%d %d \"%s\" \"%s\"\n"
									,
									cl->old_frags,
									SV_CalcPing(cl, false),
									cl->team,
									cl->name
									), sizeof(response) - (resp-response));
					resp += strlen(resp);
				}
			}

			*resp++ = 0;	//this might not be a null
			if (resp == response+sizeof(response))
			{
				//we're at the end of the buffer, it's full. bummer
				//replace 12 bytes with infoResponse
				memcpy(response+4, "infoResponse", 12);
				//move down by len(statusResponse)-len(infoResponse) bytes
				memmove(response+4+12, response+4+14, resp-response-(4+14));
				start -= 14-12; //fix this pointer

				resp = start;
				resp[-1] = 0;	//reset the \n
			}
		}
	}*/

	WriteByte(&msg, 0);

	NET_SendPacket(cluster, NET_ChooseSocket(cluster->qwdsocket, from, *from), msg.cursize, msg.data, *from);
}

void QTV_StatusResponse(cluster_t *cluster, char *msg, netadr_t *from)
{
	int p, tc, bc;
	char name[64], skin[64], token[64];
	sv_t *sv;

	char *eol;

	for (sv = cluster->servers; sv; sv = sv->next)
	{
		/*ignore connected streams*/
		if (sv->isconnected)
			continue;
		/*and only streams that we could have requested this from*/
		if (sv->autodisconnect != AD_STATUSPOLL)
			continue;

		if (Net_CompareAddress(&sv->serveraddress, from, 0, 1))
			break;
	}
	/*not a valid server... weird.*/
	if (!sv)
		return;

	/*skip the n directive*/
	msg++;
	eol = strchr(msg, '\n');
	if (!eol)
		return;
	*eol = 0;

	strlcpy(sv->map.serverinfo, msg, sizeof(sv->map.serverinfo));
	QTV_UpdatedServerInfo(sv);

//	Info_ValueForKey(sv->map.serverinfo, "map", sv->map.mapname, sizeof(sv->map.mapname));
	Info_ValueForKey(sv->map.serverinfo, "*gamedir", sv->map.gamedir, sizeof(sv->map.gamedir));
	if (!*sv->map.gamedir)
		strlcpy(sv->map.gamedir, "qw", sizeof(sv->map.gamedir));

	for(p = 0; p < MAX_CLIENTS; p++)
	{
		msg = eol+1;
		eol = strchr(msg, '\n');
		if (!eol)
			break;
		*eol = 0;

		sv->map.players[p].active = false;

		//userid
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);

		//frags
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);
		sv->map.players[p].frags = atoi(token);

		//time (minuites)
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);

		//ping
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);
		sv->map.players[p].ping = atoi(token);

		//name
		msg = COM_ParseToken(msg, name, sizeof(name), NULL);

		//skin
		msg = COM_ParseToken(msg, skin, sizeof(skin), NULL);

		//tc
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);
		tc = atoi(token);

		//bc
		msg = COM_ParseToken(msg, token, sizeof(token), NULL);
		bc = atoi(token);

		snprintf(sv->map.players[p].userinfo, sizeof(sv->map.players[p].userinfo), "\\name\\%s\\skin\\%s\\topcolor\\%i\\bottomcolor\\%i", name, skin, tc, bc);
	}
	for(; p < MAX_CLIENTS; p++)
	{
		sv->map.players[p].active = false;
		*sv->map.players[p].userinfo = 0;
	}
}

void ConnectionlessPacket(cluster_t *cluster, netadr_t *from, netmsg_t *m)
{
	char buffer[MAX_QWMSGLEN];
	int i;

	ReadLong(m);
	ReadString(m, buffer, sizeof(buffer));

	if (!strncmp(buffer, "n\\", 2))
	{
		QTV_StatusResponse(cluster, buffer, from);
		return;
	}
	if (!strncmp(buffer, "rcon ", 5))
	{
		QTV_Rcon(cluster, buffer+5, from);
		return;
	}
	if (!strncmp(buffer, "ping", 4))
	{	//ack
		Netchan_OutOfBandPrint(cluster, *from, "l");
		return;
	}
	if (!strncmp(buffer, "status", 6))
	{
		QTV_Status(cluster, from);
		return;
	}
	if (!strncmp(buffer, "getinfo", 7))
	{
		QTV_GetInfo(cluster, from, buffer+7);
		return;
	}
	if (!strncmp(buffer, "getchallenge", 12))
	{
		i = NewChallenge(cluster, from);
		if (!cluster->relayenabled)
			Netchan_OutOfBandPrint(cluster, *from, "c%i", i);
		else
		{	//special response to say we don't support dtls, but can proxy it, so use dtlsconnect without needing to send any private info until the final target is determined.
			snprintf(buffer, sizeof(buffer), "c%i%cDTLS\xff\xff\xff\xff", i, 0);	//PROTOCOL_VERSION_DTLSUPGRADE
			Netchan_OutOfBand(cluster, *from, strlen(buffer)+9, buffer);
		}
		return;
	}
	if (!strncmp(buffer, "connect 28 ", 11))
	{
		if (cluster->numviewers >= cluster->maxviewers && cluster->maxviewers)
			Netchan_OutOfBandPrint(cluster, *from, "n" "Sorry, proxy is full.\n");
		else
			NewQWClient(cluster, from, buffer);
		return;
	}
	if (!strncmp(buffer, "getserversExtResponse", 21) && cluster->pingtreeenabled)
	{	//q3-style serverlist response
		m->readpos = 4+21;
		Fwd_ParseServerList(cluster, m, -1);
		return;
	}
	if (!strncmp(buffer, "d\n", 2) && cluster->pingtreeenabled)
	{	//legacy qw serverlist response
		m->readpos = 4+2;
		Fwd_ParseServerList(cluster, m, AF_INET);
		return;
	}
	if (!strcmp(buffer, "l") && cluster->pingtreeenabled)
	{	//qw ping response
		Fwd_PingResponse(cluster, from);
		return;
	}
	if (!strncmp(buffer, "pingstatus", 10) && cluster->pingtreeenabled)
	{
		int ext = false;
		char arg[64];
		if (buffer[10] == ' ')
		{
			char *s = buffer + 11;
			while (*s)
			{
				s = COM_ParseToken(s, arg,sizeof(arg), "");	//
				if (!strcmp(arg, "ext"))
					ext = true;
			}
		}
		Fwd_PingStatus(cluster, from, ext);
		return;
	}
	if (!strncmp(buffer, "dtlsconnect ", 12) && cluster->relayenabled)
	{	//dtlsconnect challenge [finalip@middleip@targetip]
		char challenge[64];
		char *s = COM_ParseToken(buffer+12, challenge,sizeof(challenge), "");	//
		if (ChallengePasses(cluster, from, atoi(challenge)))
		{
			while(*s == ' ')
				s++;
			Fwd_NewQWFwd(cluster, from, s);	//will send a challenge to the target.
			//the relay code will pass the response to the client triggering a new dtlsconnect.
			//eventually punching all the way through to the target which will respond with a dtlsopened.
			//the client will then be free to send its dtls handshakes, with the server's certificate matched against the fingerprint reported by the master.
			//this should ensure there's no tampering.
			//note that we cannot read any disconnect hints when they're encrypted, so we'll be depending on timeouts (which also avoids malicious disconnect spoofs, yay?)
		}
		return;
	}
//	if (buffer[0] == 'l' && (!buffer[1] || buffer[1] == '\n'))
//	{
//		Sys_Printf(cluster, "Ack from %s\n", );
//	}
}


void SV_WriteDelta(int entnum, const entity_state_t *from, const entity_state_t *to, netmsg_t *msg, qboolean force, unsigned int pext)
{
	unsigned int i;
	unsigned int bits;

	bits = 0;

	if (entnum >= 2048)
	{	//panic
		if (!force)
			return;
		//erk! oh noes! woe is me!
	}
	else if (entnum >= 1024+512)
		bits |= UX_ENTITYDBL|UX_ENTITYDBL2;
	else if (entnum >= 1024)
		bits |= UX_ENTITYDBL2;
	else if (entnum >= 512)
		bits |= UX_ENTITYDBL;

	if (from->angles[0] != to->angles[0])
		bits |= U_ANGLE1;
	if (from->angles[1] != to->angles[1])
		bits |= U_ANGLE2;
	if (from->angles[2] != to->angles[2])
		bits |= U_ANGLE3;

	if (from->origin[0] != to->origin[0])
		bits |= U_ORIGIN1;
	if (from->origin[1] != to->origin[1])
		bits |= U_ORIGIN2;
	if (from->origin[2] != to->origin[2])
		bits |= U_ORIGIN3;

	if (from->colormap != to->colormap)
		bits |= U_COLORMAP;
	if (from->skinnum != to->skinnum)
		bits |= U_SKIN;
	if (from->modelindex != to->modelindex)
	{
		if (to->modelindex > 0xff)
		{
			if (to->modelindex <= 0x1ff)
				bits |= U_MODEL|UX_MODELDBL;	//0x100|byte
			else
				bits |= UX_MODELDBL;			//short
		}
		else
			bits |= U_MODEL;
	}
	if (from->frame != to->frame)
		bits |= U_FRAME;
	if ((from->effects&0xff) != (to->effects&0xff))
		bits |= U_EFFECTS;

	if ((from->effects&0xff00) != (to->effects&0xff00) &&
		pext & PEXT_DPFLAGS)
		bits |= UX_EFFECTS16;
	if (from->alpha != to->alpha &&
		pext & PEXT_TRANS)
		bits |= UX_ALPHA;
	if (from->scale != to->scale &&
		pext & PEXT_SCALE)
		bits |= UX_SCALE;
	if (from->fatness != to->fatness &&
		pext & PEXT_FATNESS)
		bits |= UX_FATNESS;
	if (from->drawflags != to->drawflags &&
		pext & PEXT_HEXEN2)
		bits |= UX_DRAWFLAGS;
	if (from->abslight != to->abslight &&
		pext & PEXT_HEXEN2)
		bits |= UX_ABSLIGHT;
	if ((from->colormod[0]!= to->colormod[0] ||
		from->colormod[1] != to->colormod[1] ||
		from->colormod[2] != to->colormod[2]) &&
		pext & PEXT_COLOURMOD)
		bits |= UX_COLOURMOD;
	if (from->dpflags != to->dpflags &&
		pext & PEXT_DPFLAGS)
		bits |= UX_DPFLAGS;
	if ((from->tagentity != to->tagentity ||
		from->tagindex != to->tagindex) &&
		pext & PEXT_SETATTACHMENT)
		bits |= UX_TAGINFO;
	if ((from->light[0] != to->light[0] ||
		from->light[1] != to->light[1] ||
		from->light[2] != to->light[2] ||
		from->light[3] != to->light[3] ||
		from->lightstyle != to->lightstyle ||
		from->lightpflags != to->lightpflags) &&
		pext & PEXT_DPFLAGS)
		bits |= UX_LIGHT;

	if (bits & 0xff000000)
		bits |= UX_YETMORE;
	if (bits & 0x00ff0000)
		bits |= UX_EVENMORE;
	if (bits & 0x000000ff)
		bits |= U_MOREBITS;



	if (!bits && !force)
		return;

	i = (entnum&511) | (bits&~511);
	WriteShort (msg, i);

	if (bits & U_MOREBITS)
		WriteByte (msg, bits&255);

	if (bits & UX_EVENMORE)
		WriteByte (msg, bits>>16);
	if (bits & UX_YETMORE)
		WriteByte (msg, bits>>24);

	if (bits & U_MODEL)
		WriteByte (msg,	to->modelindex&255);
	else if (bits & UX_MODELDBL)
		WriteShort(msg,	to->modelindex&0xffff);
	if (bits & U_FRAME)
		WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		WriteByte (msg, to->effects&0x00ff);
	if (bits & U_ORIGIN1)
		WriteCoord(msg, to->origin[0], pext);
	if (bits & U_ANGLE1)
		WriteAngle(msg, to->angles[0], pext);
	if (bits & U_ORIGIN2)
		WriteCoord(msg, to->origin[1], pext);
	if (bits & U_ANGLE2)
		WriteAngle(msg, to->angles[1], pext);
	if (bits & U_ORIGIN3)
		WriteCoord(msg, to->origin[2], pext);
	if (bits & U_ANGLE3)
		WriteAngle(msg, to->angles[2], pext);

	if (bits & UX_SCALE)
		WriteByte (msg, to->scale);
	if (bits & UX_ALPHA)
		WriteByte (msg, to->alpha);
	if (bits & UX_FATNESS)
		WriteByte (msg, to->fatness);
	if (bits & UX_DRAWFLAGS)
		WriteByte (msg, to->drawflags);
	if (bits & UX_ABSLIGHT)
		WriteByte (msg, to->abslight);
	if (bits & UX_COLOURMOD)
	{
		WriteByte (msg, to->colormod[0]);
		WriteByte (msg, to->colormod[1]);
		WriteByte (msg, to->colormod[2]);
	}
	if (bits & UX_DPFLAGS)
	{	// these are bits for the 'flags' field of the entity_state_t
		WriteByte (msg, to->dpflags);
	}
	if (bits & UX_TAGINFO)
	{
		WriteShort (msg, to->tagentity);
		WriteShort (msg, to->tagindex);
	}
	if (bits & UX_LIGHT)
	{
		WriteShort (msg, to->light[0]);
		WriteShort (msg, to->light[1]);
		WriteShort (msg, to->light[2]);
		WriteShort (msg, to->light[3]);
		WriteByte (msg, to->lightstyle);
		WriteByte (msg, to->lightpflags);
	}
	if (bits & UX_EFFECTS16)
		WriteByte (msg, to->effects>>8);
}

void SV_EmitPacketEntities (const sv_t *qtv, const viewer_t *v, const packet_entities_t *to, netmsg_t *msg)
{
	const entity_state_t *baseline;
	const packet_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;
	int		delta_frame;

	delta_frame = v->delta_frames[v->netchan.outgoing_sequence&(ENTITY_FRAMES-1)];

	// this is the frame that we are going to delta update from
	if (delta_frame != -1)
	{
		from = &v->frame[delta_frame & (ENTITY_FRAMES-1)];
		oldmax = from->numents;

		WriteByte (msg, svc_deltapacketentities);
		WriteByte (msg, delta_frame);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->numents || oldindex < oldmax)
	{
		newnum = newindex >= to->numents ? 9999 : to->entnum[newindex];
		oldnum = oldindex >= oldmax ? 9999 : from->entnum[oldindex];

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (newnum, &from->ents[oldindex], &to->ents[newindex], msg, false, qtv->pext1);

			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			baseline = &qtv->map.entity[newnum].baseline;
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (newnum, baseline, &to->ents[newindex], msg, true, qtv->pext1);

			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	WriteShort (msg, 0);	// end of packetentities
}

void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg)
{
	frame_t *frame;
	int i, entnum;
	WriteByte(msg, svc_packetentities);
	frame = &qtv->map.frame[qtv->netchan.incoming_sequence & (ENTITY_FRAMES-1)];
	for (i = 0; i < frame->numents; i++)
	{
		entnum = frame->entnums[i];
		SV_WriteDelta(entnum, &qtv->map.entity[entnum].baseline, &frame->ents[i], msg, true, qtv->pext1);
	}
	WriteShort(msg, 0);
}

static float InterpolateAngle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (move >= 180)
		move -= 360;
	else if (move <= -180)
		move += 360;

	return current + fraction * move;
}

void SendLocalPlayerState(sv_t *tv, viewer_t *v, int playernum, netmsg_t *msg)
{
	int flags;
	int j;
	unsigned int pext1 = tv?tv->pext1:0;

	WriteByte(msg, svc_playerinfo);
	WriteByte(msg, playernum);

	if (tv && tv->controller == v)
	{	//we're the one that is actually playing.
		v->trackplayer = tv->map.thisplayer;
		flags = 0;
		if (tv->map.players[tv->map.thisplayer].current.weaponframe)
			flags |= PF_WEAPONFRAME;
		if (tv->map.players[tv->map.thisplayer].current.effects)
			flags |= PF_EFFECTS;

		if (tv->map.players[tv->map.thisplayer].dead)
			flags |= PF_DEAD;
		if (tv->map.players[tv->map.thisplayer].gibbed)
			flags |= PF_GIB;

		for (j=0 ; j<3 ; j++)
			if (tv->map.players[tv->map.thisplayer].current.velocity[j])
				flags |= (PF_VELOCITY1<<j);

		WriteShort(msg, flags);
		WriteCoord(msg, tv->map.players[tv->map.thisplayer].current.origin[0], tv->pext1);
		WriteCoord(msg, tv->map.players[tv->map.thisplayer].current.origin[1], tv->pext1);
		WriteCoord(msg, tv->map.players[tv->map.thisplayer].current.origin[2], tv->pext1);
		WriteByte(msg, tv->map.players[tv->map.thisplayer].current.frame);

		for (j=0 ; j<3 ; j++)
			if (flags & (PF_VELOCITY1<<j) )
				WriteShort (msg, tv->map.players[tv->map.thisplayer].current.velocity[j]);

		if (flags & PF_MODEL)
			WriteByte(msg, tv->map.players[tv->map.thisplayer].current.modelindex);
		if (flags & PF_SKINNUM)
			WriteByte(msg, tv->map.players[tv->map.thisplayer].current.skinnum);
		if (flags & PF_EFFECTS)
			WriteByte(msg, tv->map.players[tv->map.thisplayer].current.effects);
		if (flags & PF_WEAPONFRAME)
			WriteByte(msg, tv->map.players[tv->map.thisplayer].current.weaponframe);
	}
	else
	{
		flags = 0;

		for (j=0 ; j<3 ; j++)
			if ((int)v->velocity[j])
				flags |= (PF_VELOCITY1<<j);

		WriteShort(msg, flags);
		WriteCoord(msg, v->origin[0], pext1);
		WriteCoord(msg, v->origin[1], pext1);
		WriteCoord(msg, v->origin[2], pext1);
		WriteByte(msg, 0);

		for (j=0 ; j<3 ; j++)
			if (flags & (PF_VELOCITY1<<j) )
				WriteShort (msg, v->velocity[j]);
	}
}

#define	UNQ_MOREBITS	(1<<0)
#define	UNQ_ORIGIN1	(1<<1)
#define	UNQ_ORIGIN2	(1<<2)
#define	UNQ_ORIGIN3	(1<<3)
#define	UNQ_ANGLE2	(1<<4)
#define	UNQ_NOLERP	(1<<5)		// don't interpolate movement
#define	UNQ_FRAME		(1<<6)
#define UNQ_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	UNQ_ANGLE1	(1<<8)
#define	UNQ_ANGLE3	(1<<9)
#define	UNQ_MODEL		(1<<10)
#define	UNQ_COLORMAP	(1<<11)
#define	UNQ_SKIN		(1<<12)
#define	UNQ_EFFECTS	(1<<13)
#define	UNQ_LONGENTITY	(1<<14)
#define UNQ_UNUSED	(1<<15)


#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)

void SendNQClientData(sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	playerinfo_t *pl;
	int bits;
	int i;

	if (!tv)
		return;

	if (v->trackplayer < 0)
	{
		WriteByte (msg, svc_nqclientdata);
		WriteShort (msg, SU_VIEWHEIGHT|SU_ITEMS);
		WriteByte (msg, 22); //viewheight
		WriteLong (msg, 0);	//items
		WriteShort (msg, 1000);	//health
		WriteByte (msg, 0);	//currentammo
		WriteByte (msg, 0);	//shells
		WriteByte (msg, 0);	//nails
		WriteByte (msg, 0);	//rockets
		WriteByte (msg, 0);	//cells
		WriteByte (msg, 0); //active weapon
		return;
	}
	else
		pl = &tv->map.players[v->trackplayer];

	bits = 0;

	if (!pl->dead)
		bits |= SU_VIEWHEIGHT;

	if (0)
		bits |= SU_IDEALPITCH;

	bits |= SU_ITEMS;

	if ( 0)
		bits |= SU_ONGROUND;

	if ( 0 )
		bits |= SU_INWATER;

	for (i=0 ; i<3 ; i++)
	{
		if (0)
			bits |= (SU_PUNCH1<<i);
		if (0)
			bits |= (SU_VELOCITY1<<i);
	}

	if (pl->current.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (pl->stats[STAT_ARMOR])
		bits |= SU_ARMOR;

//	if (pl->stats[STAT_WEAPON])
		bits |= SU_WEAPON;

// send the data

	WriteByte (msg, svc_nqclientdata);
	WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		WriteByte (msg, 22);

	if (bits & SU_IDEALPITCH)
		WriteByte (msg, 0);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			WriteByte (msg, 0);
		if (bits & (SU_VELOCITY1<<i))
			WriteByte (msg, 0);
	}

	if (bits & SU_ITEMS)
		WriteLong (msg, pl->stats[STAT_ITEMS]);

	if (bits & SU_WEAPONFRAME)
		WriteByte (msg, pl->current.weaponframe);
	if (bits & SU_ARMOR)
		WriteByte (msg, pl->stats[STAT_ARMOR]);
	if (bits & SU_WEAPON)
		WriteByte (msg, pl->stats[STAT_WEAPONMODELI]);

	WriteShort (msg, pl->stats[STAT_HEALTH]);
	WriteByte (msg, pl->stats[STAT_AMMO]);
	WriteByte (msg, pl->stats[STAT_SHELLS]);
	WriteByte (msg, pl->stats[STAT_NAILS]);
	WriteByte (msg, pl->stats[STAT_ROCKETS]);
	WriteByte (msg, pl->stats[STAT_CELLS]);

	WriteByte (msg, pl->stats[STAT_ACTIVEWEAPON]);
}

void SendNQPlayerStates(cluster_t *cluster, sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	int e;
	int i;
	usercmd_t to;
	float lerp;
	int bits;
	float org[3];
	entity_t *ent;
	playerinfo_t *pl;
	unsigned int pext1;

	memset(&to, 0, sizeof(to));

	if (tv)
	{
		pext1 = tv->pext1;
		WriteByte(msg, svc_nqtime);
		WriteFloat(msg, (tv->physicstime - tv->mapstarttime)/1000.0f);

		BSP_SetupForPosition(tv->map.bsp, v->origin[0], v->origin[1], v->origin[2]);

		lerp = ((tv->simtime - tv->oldpackettime)/1000.0f) / ((tv->nextpackettime - tv->oldpackettime)/1000.0f);
		lerp = 1;

//		if (tv->controller == v)
//			lerp = 1;
	}
	else
	{
		WriteByte(msg, svc_nqtime);
		WriteFloat(msg, (cluster->curtime)/1000.0f);

		lerp = 1;
		pext1 = 0;
	}

	SendNQClientData(tv, v, msg);

	if (tv)
	{
		if (v != tv->controller)
		{
			if (v->trackplayer >= 0)
			{
				WriteByte(msg, svc_nqsetview);
				WriteShort(msg, v->trackplayer+1);

				WriteByte(msg, svc_setangle);
				WriteAngle(msg, InterpolateAngle(tv->map.players[v->trackplayer].old.angles[0], tv->map.players[v->trackplayer].current.angles[0], lerp), tv->pext1);
				WriteAngle(msg, InterpolateAngle(tv->map.players[v->trackplayer].old.angles[1], tv->map.players[v->trackplayer].current.angles[1], lerp), tv->pext1);
				WriteAngle(msg, InterpolateAngle(tv->map.players[v->trackplayer].old.angles[2], tv->map.players[v->trackplayer].current.angles[2], lerp), tv->pext1);
			}
			else
			{
				WriteByte(msg, svc_nqsetview);
				WriteShort(msg, v->thisplayer+1);
			}
		}

		for (e = 0; e < MAX_CLIENTS; e++)
		{
			pl = &tv->map.players[e];
			ent = &tv->map.entity[e+1];

			if (e == v->thisplayer && v->trackplayer < 0)
			{
				bits = UNQ_ORIGIN1 | UNQ_ORIGIN2 | UNQ_ORIGIN3 | UNQ_COLORMAP;


  				if (e+1 > 255)
					bits |= UNQ_LONGENTITY;

				if (bits > 255)
					bits |= UNQ_MOREBITS;
				WriteByte (msg,bits | UNQ_SIGNAL);
				if (bits & UNQ_MOREBITS)
					WriteByte (msg, bits>>8);
				if (bits & UNQ_LONGENTITY)
					WriteShort (msg,e+1);
				else
					WriteByte (msg,e+1);

				if (bits & UNQ_MODEL)
					WriteByte (msg,	0);
				if (bits & UNQ_FRAME)
					WriteByte (msg, 0);
				if (bits & UNQ_COLORMAP)
					WriteByte (msg, 0);
				if (bits & UNQ_SKIN)
					WriteByte (msg, 0);
				if (bits & UNQ_EFFECTS)
					WriteByte (msg, 0);
				if (bits & UNQ_ORIGIN1)
					WriteCoord (msg, v->origin[0], tv->pext1);
				if (bits & UNQ_ANGLE1)
					WriteAngle(msg, -v->ucmds[2].angles[0], tv->pext1);
				if (bits & UNQ_ORIGIN2)
					WriteCoord (msg, v->origin[1], tv->pext1);
				if (bits & UNQ_ANGLE2)
					WriteAngle(msg, v->ucmds[2].angles[1], tv->pext1);
				if (bits & UNQ_ORIGIN3)
					WriteCoord (msg, v->origin[2], tv->pext1);
				if (bits & UNQ_ANGLE3)
					WriteAngle(msg, v->ucmds[2].angles[2], tv->pext1);
				continue;
			}

			if (!pl->active)
				continue;

			if (v != tv->controller && e != v->trackplayer)
				if (pl->current.modelindex >= tv->map.numinlines && !BSP_Visible(tv->map.bsp, pl->leafcount, pl->leafs))	//don't cull bsp objects, like nq...
					continue;

// send an update
			bits = 0;

			for (i=0 ; i<3 ; i++)
			{
				org[i] = (lerp)*pl->current.origin[i] + (1-lerp)*pl->old.origin[i];
				bits |= UNQ_ORIGIN1<<i;
			}

			if ( pl->current.angles[0] != ent->baseline.angles[0] )
				bits |= UNQ_ANGLE1;

			if ( pl->current.angles[1] != ent->baseline.angles[1] )
				bits |= UNQ_ANGLE2;

			if ( pl->current.angles[2] != ent->baseline.angles[2] )
				bits |= UNQ_ANGLE3;

//			if (pl->v.movetype == MOVETYPE_STEP)
//				bits |= UNQ_NOLERP;	// don't mess up the step animation

			if (ent->baseline.colormap != e+1 || ent->baseline.colormap > 15)
				bits |= UNQ_COLORMAP;

			if (ent->baseline.skinnum != pl->current.skinnum)
				bits |= UNQ_SKIN;

			if (ent->baseline.frame != pl->current.frame)
				bits |= UNQ_FRAME;

			if (ent->baseline.effects != pl->current.effects)
				bits |= UNQ_EFFECTS;

			if (ent->baseline.modelindex != pl->current.modelindex)
				bits |= UNQ_MODEL;

			if (e+1 > 255)
				bits |= UNQ_LONGENTITY;

			if (bits > 255)
				bits |= UNQ_MOREBITS;

		//
		// write the message
		//
			WriteByte (msg,bits | UNQ_SIGNAL);

			if (bits & UNQ_MOREBITS)
				WriteByte (msg, bits>>8);
			if (bits & UNQ_LONGENTITY)
				WriteShort (msg,e+1);
			else
				WriteByte (msg,e+1);

			if (bits & UNQ_MODEL)
				WriteByte (msg,	pl->current.modelindex);
			if (bits & UNQ_FRAME)
				WriteByte (msg, pl->current.frame);
			if (bits & UNQ_COLORMAP)
				WriteByte (msg, (e>=15)?0:(e+1));
			if (bits & UNQ_SKIN)
				WriteByte (msg, pl->current.skinnum);
			if (bits & UNQ_EFFECTS)
				WriteByte (msg, pl->current.effects);
			if (bits & UNQ_ORIGIN1)
				WriteCoord (msg, org[0], tv->pext1);
			if (bits & UNQ_ANGLE1)
				WriteAngle(msg, -pl->current.angles[0], tv->pext1);
			if (bits & UNQ_ORIGIN2)
				WriteCoord (msg, org[1], tv->pext1);
			if (bits & UNQ_ANGLE2)
				WriteAngle(msg, pl->current.angles[1], tv->pext1);
			if (bits & UNQ_ORIGIN3)
				WriteCoord (msg, org[2], tv->pext1);
			if (bits & UNQ_ANGLE3)
				WriteAngle(msg, pl->current.angles[2], tv->pext1);
		}


		{
			int newindex = 0;
			entity_state_t *newstate;
			int newnum;
			frame_t *topacket;
			int snapdist = 128;	//in quake units
			int miss;

			snapdist = snapdist*8;
			snapdist = snapdist*snapdist;

			topacket = &tv->map.frame[tv->netchan.incoming_sequence&(ENTITY_FRAMES-1)];

			for (newindex = 0; newindex < topacket->numents; newindex++)
			{
				//don't pvs cull bsp models
				//pvs cull everything else
				newstate = &topacket->ents[newindex];
				newnum = topacket->entnums[newindex];
				if (v != tv->controller)
					if (newstate->modelindex >= tv->map.numinlines && !BSP_Visible(tv->map.bsp, tv->map.entity[newnum].leafcount, tv->map.entity[newnum].leafs))
						continue;

				if (msg->cursize + 128 > msg->maxsize)
					break;

		// send an update
				bits = 0;

				for (i=0 ; i<3 ; i++)
				{
					miss = (newstate->origin[i]) - ent->baseline.origin[i];
					if ( miss <= -1/8.0 || miss >= 1/8.0 )
						bits |= UNQ_ORIGIN1<<i;
				}

				if (newstate->angles[0] != ent->baseline.angles[0])
					bits |= UNQ_ANGLE1;

				if (newstate->angles[1] != ent->baseline.angles[1])
					bits |= UNQ_ANGLE2;

				if (newstate->angles[2] != ent->baseline.angles[2])
					bits |= UNQ_ANGLE3;

		//			if (ent->v.movetype == MOVETYPE_STEP)
		//				bits |= UNQ_NOLERP;	// don't mess up the step animation

				if (newstate->colormap != ent->baseline.colormap || ent->baseline.colormap > 15)
					bits |= UNQ_COLORMAP;

				if (newstate->skinnum != ent->baseline.skinnum)
					bits |= UNQ_SKIN;

				if (newstate->frame != ent->baseline.frame)
					bits |= UNQ_FRAME;

				if (newstate->effects != ent->baseline.effects)
					bits |= UNQ_EFFECTS;

				if (newstate->modelindex != ent->baseline.modelindex)
					bits |= UNQ_MODEL;

				if (newnum >= 256)
					bits |= UNQ_LONGENTITY;

				if (bits >= 256)
					bits |= UNQ_MOREBITS;

			//
			// write the message
			//
				WriteByte (msg,bits | UNQ_SIGNAL);

				if (bits & UNQ_MOREBITS)
					WriteByte (msg, bits>>8);
				if (bits & UNQ_LONGENTITY)
					WriteShort (msg,newnum);
				else
					WriteByte (msg,newnum);

				if (bits & UNQ_MODEL)
					WriteByte (msg,	newstate->modelindex);
				if (bits & UNQ_FRAME)
					WriteByte (msg, newstate->frame);
				if (bits & UNQ_COLORMAP)
					WriteByte (msg, (newstate->colormap>15)?0:(newstate->colormap));
				if (bits & UNQ_SKIN)
					WriteByte (msg, newstate->skinnum);
				if (bits & UNQ_EFFECTS)
					WriteByte (msg, newstate->effects);
				if (bits & UNQ_ORIGIN1)
					WriteCoord (msg, newstate->origin[0], pext1);
				if (bits & UNQ_ANGLE1)
					WriteAngle(msg, newstate->angles[0], pext1);
				if (bits & UNQ_ORIGIN2)
					WriteCoord (msg, newstate->origin[1], pext1);
				if (bits & UNQ_ANGLE2)
					WriteAngle(msg, newstate->angles[1], pext1);
				if (bits & UNQ_ORIGIN3)
					WriteCoord (msg, newstate->origin[2], pext1);
				if (bits & UNQ_ANGLE3)
					WriteAngle(msg, newstate->angles[2], pext1);
			}
		}
	}
	else
	{
		WriteByte(msg, svc_nqsetview);
		WriteShort(msg, v->thisplayer+1);

		WriteShort (msg,UNQ_MOREBITS|UNQ_MODEL|UNQ_ORIGIN1 | UNQ_ORIGIN2 | UNQ_ORIGIN3 | UNQ_SIGNAL);
		WriteByte (msg, v->thisplayer+1);
		WriteByte (msg, 2);	//model
		WriteCoord (msg, v->origin[0], pext1);
		WriteCoord (msg, v->origin[1], pext1);
		WriteCoord (msg, v->origin[2], pext1);
	}
}

//returns self, or the final commentator
viewer_t *GetCommentator(viewer_t *v)
{
	viewer_t *orig = v;
	int runaway = 10;

	while(runaway-- > 0)
	{
		if (!v->commentator)
			break;
		if (v->commentator->thinksitsconnected == false)
			break;
		if (v->commentator->server != orig->server)
			break;
		v = v->commentator;
	}
	return v;
}

void SendPlayerStates(sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	bsp_t *bsp = (!tv || tv->controller == v ? NULL : tv->map.bsp);
	viewer_t *cv;
	packet_entities_t *e;
	int i;
	usercmd_t to;
	unsigned short flags;
	float interp;
	float lerp;
	int track;

	int snapdist = 128;	//in quake units

	snapdist = snapdist*8;
	snapdist = snapdist*snapdist;


	memset(&to, 0, sizeof(to));

	if (tv)
	{
		if (tv->physicstime != v->settime)// && tv->cluster->chokeonnotupdated)
		{
			WriteByte(msg, svc_updatestatlong);
			WriteByte(msg, STAT_TIME);
			WriteLong(msg, v->settime);

			v->settime = tv->physicstime;
		}

		BSP_SetupForPosition(bsp, v->origin[0], v->origin[1], v->origin[2]);

		lerp = ((tv->simtime - tv->oldpackettime)/1000.0f) / ((tv->nextpackettime - tv->oldpackettime)/1000.0f);
		if (lerp < 0)
			lerp = 0;
		if (lerp > 1)
			lerp = 1;

		if (tv->controller == v)
		{
			lerp = 1;
			track = tv->map.thisplayer;
			v->trackplayer = tv->map.thisplayer;
		}
		else
		{
			cv = GetCommentator(v);
			track = cv->trackplayer;

			if (cv != v && track < 0)
			{	//following a commentator
				track = MAX_CLIENTS-2;
			}

			if (v->trackplayer != track)
				QW_StuffcmdToViewer (v, "track %i\n", track);

			if (!v->commentator && track >= 0 && !v->backbuffered)
			{
				if (v->trackplayer != tv->map.trackplayer && tv->usequakeworldprotocols)
					if (!tv->map.players[v->trackplayer].active && tv->map.players[tv->map.trackplayer].active)
					{
						QW_StuffcmdToViewer (v, "track %i\n", tv->map.trackplayer);
					}
			}
		}

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (i == v->thisplayer)
			{
				SendLocalPlayerState(tv, v, i, msg);
				continue;
			}

			if (!tv->map.players[i].active && track != i)
				continue;

			//bsp cull. currently tracked player is always visible
			if (track != i && !BSP_Visible(bsp, tv->map.players[i].leafcount, tv->map.players[i].leafs))
				continue;

			flags = PF_COMMAND;
			if (track == i && tv->map.players[i].current.weaponframe)
				flags |= PF_WEAPONFRAME;
			if (tv->map.players[i].current.modelindex != tv->map.modelindex_player)
				flags |= PF_MODEL;
			if (tv->map.players[i].dead || !tv->map.players[i].active)
				flags |= PF_DEAD;
			if (tv->map.players[i].gibbed || !tv->map.players[i].active)
				flags |= PF_GIB;
			if (tv->map.players[i].current.effects != 0)
				flags |= PF_EFFECTS;
			if (tv->map.players[i].current.skinnum != 0)
				flags |= PF_SKINNUM;
			if (tv->map.players[i].current.velocity[0])
				flags |= PF_VELOCITY1;
			if (tv->map.players[i].current.velocity[1])
				flags |= PF_VELOCITY2;
			if (tv->map.players[i].current.velocity[2])
				flags |= PF_VELOCITY3;

			WriteByte(msg, svc_playerinfo);
			WriteByte(msg, i);
			WriteShort(msg, flags);

			if (!tv->map.players[i].active || !tv->map.players[i].oldactive ||
				(tv->map.players[i].current.origin[0] - tv->map.players[i].old.origin[0])*(tv->map.players[i].current.origin[0] - tv->map.players[i].old.origin[0]) > snapdist ||
				(tv->map.players[i].current.origin[1] - tv->map.players[i].old.origin[1])*(tv->map.players[i].current.origin[1] - tv->map.players[i].old.origin[1]) > snapdist ||
				(tv->map.players[i].current.origin[2] - tv->map.players[i].old.origin[2])*(tv->map.players[i].current.origin[2] - tv->map.players[i].old.origin[2]) > snapdist)
			{	//teleported (or respawned), so don't interpolate
				WriteCoord(msg, tv->map.players[i].current.origin[0], tv->pext1);
				WriteCoord(msg, tv->map.players[i].current.origin[1], tv->pext1);
				WriteCoord(msg, tv->map.players[i].current.origin[2], tv->pext1);
			}
			else
			{	//send interpolated angles
				interp = (lerp)*tv->map.players[i].current.origin[0] + (1-lerp)*tv->map.players[i].old.origin[0];
				WriteCoord(msg, interp, tv->pext1);
				interp = (lerp)*tv->map.players[i].current.origin[1] + (1-lerp)*tv->map.players[i].old.origin[1];
				WriteCoord(msg, interp, tv->pext1);
				interp = (lerp)*tv->map.players[i].current.origin[2] + (1-lerp)*tv->map.players[i].old.origin[2];
				WriteCoord(msg, interp, tv->pext1);
			}

			WriteByte(msg, tv->map.players[i].current.frame);


			if (flags & PF_MSEC)
			{
				WriteByte(msg, 0);
			}
			if (flags & PF_COMMAND)
			{
				if (!tv->map.players[i].active || !tv->map.players[i].oldactive)
				{
					to.angles[0] = tv->map.players[i].current.angles[0];
					to.angles[1] = tv->map.players[i].current.angles[1];
					to.angles[2] = tv->map.players[i].current.angles[2];
				}
				else
				{
					to.angles[0] = InterpolateAngle(tv->map.players[i].old.angles[0], tv->map.players[i].current.angles[0], lerp);
					to.angles[1] = InterpolateAngle(tv->map.players[i].old.angles[1], tv->map.players[i].current.angles[1], lerp);
					to.angles[2] = InterpolateAngle(tv->map.players[i].old.angles[2], tv->map.players[i].current.angles[2], lerp);
				}
				WriteDeltaUsercmd(msg, &nullcmd, &to);
			}
			//vel
			if (flags & PF_VELOCITY1)
				WriteShort(msg, tv->map.players[i].current.velocity[0]);
			if (flags & PF_VELOCITY2)
				WriteShort(msg, tv->map.players[i].current.velocity[1]);
			if (flags & PF_VELOCITY3)
				WriteShort(msg, tv->map.players[i].current.velocity[2]);
			//model
			if (flags & PF_MODEL)
				WriteByte(msg, tv->map.players[i].current.modelindex);
			//skin
			if (flags & PF_SKINNUM)
				WriteByte (msg, tv->map.players[i].current.skinnum);
			//effects
			if (flags & PF_EFFECTS)
				WriteByte (msg, tv->map.players[i].current.effects);
			//weaponframe
			if (flags & PF_WEAPONFRAME)
				WriteByte(msg, tv->map.players[i].current.weaponframe);
		}
	}
	else
	{
		lerp = 1;

		SendLocalPlayerState(tv, v, v->thisplayer, msg);
	}


	e = &v->frame[v->netchan.outgoing_sequence&(ENTITY_FRAMES-1)];
	e->numents = 0;
	if (tv)
	{
		int oldindex = 0, newindex = 0;
		entity_state_t *newstate;
		int newnum, oldnum;
		frame_t *frompacket, *topacket;
		topacket = &tv->map.frame[tv->netchan.incoming_sequence&(ENTITY_FRAMES-1)];
		if (tv->usequakeworldprotocols)
		{
			frompacket = &tv->map.frame[(topacket->oldframe)&(ENTITY_FRAMES-1)];
		}
		else
		{
			frompacket = &tv->map.frame[(tv->netchan.incoming_sequence-1)&(ENTITY_FRAMES-1)];
		}

		for (newindex = 0; newindex < topacket->numents; newindex++)
		{
			//don't pvs cull bsp models
			//pvs cull everything else
			newstate = &topacket->ents[newindex];
			newnum = topacket->entnums[newindex];
			if (newstate->modelindex >= tv->map.numinlines && !BSP_Visible(bsp, tv->map.entity[newnum].leafcount, tv->map.entity[newnum].leafs))
				continue;

			e->entnum[e->numents] = newnum;
			memcpy(&e->ents[e->numents], newstate, sizeof(entity_state_t));

			if (frompacket != topacket)	//optimisation for qw protocols
			{
				entity_state_t *oldstate;

				if (oldindex < frompacket->numents)
				{
					oldnum = frompacket->entnums[oldindex];

					while(oldnum < newnum)
					{
						oldindex++;
						if (oldindex >= frompacket->numents)
							break;	//no more
						oldnum = frompacket->entnums[oldindex];
					}
					if (oldnum == newnum)
					{
						//ent exists in old packet
						oldstate = &frompacket->ents[oldindex];
					}
					else
					{
						oldstate = newstate;
					}
				}
				else
				{	//reached end, definatly not in packet
					oldstate = newstate;
				}


				if ((newstate->origin[0] - oldstate->origin[0])*(newstate->origin[0] - oldstate->origin[0]) > snapdist ||
					(newstate->origin[1] - oldstate->origin[1])*(newstate->origin[1] - oldstate->origin[1]) > snapdist ||
					(newstate->origin[2] - oldstate->origin[2])*(newstate->origin[2] - oldstate->origin[2]) > snapdist)
				{	//teleported (or respawned), so don't interpolate
					e->ents[e->numents].origin[0] = newstate->origin[0];
					e->ents[e->numents].origin[1] = newstate->origin[1];
					e->ents[e->numents].origin[2] = newstate->origin[2];
				}
				else
				{
					e->ents[e->numents].origin[0] = (lerp)*newstate->origin[0] + (1-lerp)*oldstate->origin[0];
					e->ents[e->numents].origin[1] = (lerp)*newstate->origin[1] + (1-lerp)*oldstate->origin[1];
					e->ents[e->numents].origin[2] = (lerp)*newstate->origin[2] + (1-lerp)*oldstate->origin[2];
				}
			}

			e->numents++;

			if (e->numents == ENTS_PER_FRAME)
				break;
		}
	}

	SV_EmitPacketEntities(tv, v, e, msg);

	if (tv && tv->map.nailcount)
	{
		WriteByte(msg, svc_nails);
		WriteByte(msg, tv->map.nailcount);
		for (i = 0; i < tv->map.nailcount; i++)
		{
			WriteByte(msg, tv->map.nails[i].bits[0]);
			WriteByte(msg, tv->map.nails[i].bits[1]);
			WriteByte(msg, tv->map.nails[i].bits[2]);
			WriteByte(msg, tv->map.nails[i].bits[3]);
			WriteByte(msg, tv->map.nails[i].bits[4]);
			WriteByte(msg, tv->map.nails[i].bits[5]);
		}
	}
}

void UpdateStats(sv_t *qtv, viewer_t *v)
{
	viewer_t *cv;
	netmsg_t msg;
	char buf[6];
	int i;
	static const unsigned int nullstats[MAX_STATS] = {1000};

	const unsigned int *stats;

	InitNetMsg(&msg, buf, sizeof(buf));

	if (v->commentator && v->thinksitsconnected)
		cv = v->commentator;
	else
		cv = v;

	if (qtv && qtv->controller == cv)
		stats = qtv->map.players[qtv->map.thisplayer].stats;
	else if (cv->trackplayer == -1 || !qtv)
		stats = nullstats;
	else
		stats = qtv->map.players[cv->trackplayer].stats;

	for (i = 0; i < MAX_STATS; i++)
	{
		if (v->currentstats[i] != stats[i])
		{
			if (v->netchan.isnqprotocol)
			{	//nq only supports 32bit stats
				WriteByte(&msg, svc_updatestat);
				WriteByte(&msg, i);
				WriteLong(&msg, stats[i]);
			}
			else if (stats[i] < 256)
			{
				WriteByte(&msg, svc_updatestat);
				WriteByte(&msg, i);
				WriteByte(&msg, stats[i]);
			}
			else
			{
				WriteByte(&msg, svc_updatestatlong);
				WriteByte(&msg, i);
				WriteLong(&msg, stats[i]);
			}
			SendBufferToViewer(v, msg.data, msg.cursize, true);
			msg.cursize = 0;
			v->currentstats[i] = stats[i];
		}
	}
}

//returns the next prespawn 'buffer' number to use, or -1 if no more
//FIXME: viewer may support fewer/different extensions vs the the stream.
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer)
{
	int r, ni;
	r = bufnum;

	ni = SendCurrentUserinfos(qtv, curmsgsize, msg, bufnum, thisplayer);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_CLIENTS;

	ni = SendCurrentBaselines(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_ENTITIES;

	ni = SendCurrentLightmaps(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_LIGHTSTYLES;

	ni = SendStaticSounds(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICSOUNDS;

	ni = SendStaticEntities(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICENTITIES;

	if (bufnum == 0)
		return -1;

	return r;
}

void PMove(viewer_t *v, usercmd_t *cmd)
{
	sv_t *qtv;
	pmove_t pmove;
	if (v->server && v->server->controller == v)
	{
		v->origin[0] = v->server->map.players[v->server->map.thisplayer].current.origin[0];
		v->origin[1] = v->server->map.players[v->server->map.thisplayer].current.origin[1];
		v->origin[2] = v->server->map.players[v->server->map.thisplayer].current.origin[2];

		v->velocity[0] = v->server->map.players[v->server->map.thisplayer].current.velocity[0];
		v->velocity[1] = v->server->map.players[v->server->map.thisplayer].current.velocity[1];
		v->velocity[2] = v->server->map.players[v->server->map.thisplayer].current.velocity[2];
		return;
	}
	pmove.origin[0] = v->origin[0];
	pmove.origin[1] = v->origin[1];
	pmove.origin[2] = v->origin[2];

	pmove.velocity[0] = v->velocity[0];
	pmove.velocity[1] = v->velocity[1];
	pmove.velocity[2] = v->velocity[2];

	pmove.cmd = *cmd;
	qtv = v->server;
	if (qtv)
	{
		pmove.movevars = qtv->map.movevars;
	}
	else
	{
		QTV_DefaultMovevars(&pmove.movevars);
	}
	PM_PlayerMove(&pmove);

	v->origin[0] = pmove.origin[0];
	v->origin[1] = pmove.origin[1];
	v->origin[2] = pmove.origin[2];

	v->velocity[0] = pmove.velocity[0];
	v->velocity[1] = pmove.velocity[1];
	v->velocity[2] = pmove.velocity[2];
}

void QW_SetCommentator(cluster_t *cluster, viewer_t *v, viewer_t *commentator)
{
//	if (v->commentator == commentator)
//		return;

	WriteByte(&v->netchan.message, svc_setinfo);
	WriteByte(&v->netchan.message, MAX_CLIENTS-2);
	WriteString(&v->netchan.message, "name");
	if (commentator)
	{
		WriteString(&v->netchan.message, commentator->name);
		QW_StuffcmdToViewer(v, "track %i\n", MAX_CLIENTS-2);
		QW_PrintfToViewer(v, "Following commentator %s\n", commentator->name);

		if (v->server != commentator->server)
			QW_SetViewersServer(cluster, v, commentator->server);
	}
	else
	{
		WriteString(&v->netchan.message, "");
		if (v->commentator )
			QW_PrintfToViewer(v, "Commentator disabled\n");
	}
	v->commentator = commentator;
}

void QTV_SayCommand(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *fullcommand)
{
	char command[256];
	char *args;
	args = COM_ParseToken(fullcommand, command, sizeof(command), NULL);
	if (!args)
		args = "";
	while(*args && *args <= ' ')
		args++;

#pragma message("fixme: These all need testing")
	if (!strcmp(command, "help"))
	{
		QW_PrintfToViewer(v,	"Website: "PROXYWEBSITE"\n"
					"Commands:\n"
					".bind\n"
					"  Bind your keys to drive the menu.\n"
					".clients\n"
					"  Lists the users connected to this\n"
					"  proxy.\n"
					".qtvinfo\n"
					"  Print info about the current QTV\n"
					"  you're on.\n"
					".demo gamedir/demoname.mvd \n"
					"  Start a new stream on the specified\n"
					"  demo.\n"
					".disconnect\n"
					"  Disconnect from any server or\n"
					"  stream you're on.\n"
					".join qwserver:port\n"
					"  Play on the specified server.\n"
					".observe qwserver:port\n"
					"  Spectate on the specified server.\n"
					".qtv tcpserver:port\n"
					"  Start a new stream on the specified\n"
					"  server.\n"
					".guimenu\n"
					"  Bring up the GUI-based menu\n"
					"  interface.\n"
					".tuimenu\n"
					"  Bring up the text-based menu\n"
					"  interface.\n"
					".menu\n"
					"  Automatically chooses an interface\n"
					"  that your client supports.\n"
					".admin\n"
					"  Log in to administrate this QTV\n"
					"  proxy.\n"
				);
	}
	else if (!strcmp(command, "qtvinfo"))
	{
		char buf[256];
		netadr_t addr;
		unsigned char *ip;
		gethostname(buf, sizeof(buf));	//ask the operating system for the local dns name
		NET_StringToAddr(buf, &addr, 0);	//look that up
		ip = (unsigned char*)&((struct sockaddr_in *)&addr)->sin_addr;
		QW_PrintfToViewer(v, "[QuakeTV] %s | %i.%i.%i.%i\n", cluster->hostname, ip[0], ip[1], ip[2], ip[3]);
	}
	else if (!strcmp(command, "menu"))
	{
		v->menuspamtime = cluster->curtime-1;

		COM_ParseToken(args, command, sizeof(command), NULL);
		if (!strcmp(command, "up"))
		{
			v->menuop -= 1;
		}
		else if (!strcmp(command, "down"))
		{
			v->menuop += 1;
		}
		else if (!strcmp(command, "enter"))
		{
			Menu_Enter(cluster, v, 0);
		}
		else if (!strcmp(command, "use"))
		{
			Menu_Enter(cluster, v, 0);
		}
		else if (!strcmp(command, "right"))
		{
			Menu_Enter(cluster, v, 1);
		}
		else if (!strcmp(command, "left"))
		{
			Menu_Enter(cluster, v, -1);
		}
		else if (!strcmp(command, "select"))
		{
			Menu_Enter(cluster, v, 0);
		}
		else if (!strcmp(command, "home"))
		{
			v->menuop -= 100000;
		}
		else if (!strcmp(command, "end"))
		{
			v->menuop += 100000;
		}
		else if (!strcmp(command, "back"))
		{
			QW_SetMenu(v, MENU_DEFAULT);
		}
		else if (!strcmp(command, "enter"))
		{
			if (v->menunum)
				Menu_Enter(cluster, v, 0);
			else
				QW_SetMenu(v, MENU_SERVERS);
		}
		else if (!strcmp(command, "bind") || !strcmp(command, "bindstd"))
		{
			QW_StuffcmdToViewer(v, "bind uparrow \"say proxy:menu up\"\n");
			QW_StuffcmdToViewer(v, "bind downarrow \"say proxy:menu down\"\n");
			QW_StuffcmdToViewer(v, "bind rightarrow \"say proxy:menu right\"\n");
			QW_StuffcmdToViewer(v, "bind leftarrow \"say proxy:menu left\"\n");

			QW_StuffcmdToViewer(v, "bind enter \"say proxy:menu select\"\n");

			QW_StuffcmdToViewer(v, "bind home \"say proxy:menu home\"\n");
			QW_StuffcmdToViewer(v, "bind end \"say proxy:menu end\"\n");
			QW_StuffcmdToViewer(v, "bind pause \"say proxy:menu\"\n");
			QW_StuffcmdToViewer(v, "bind backspace \"say proxy:menu back\"\n");

			QW_PrintfToViewer(v, "All keys bound\n");
		}
		else if (!*command)
		{
			if (v->menunum)
				QW_SetMenu(v, MENU_NONE);
			else if (v->conmenussupported)
				goto guimenu;
			else
				goto tuimenu;
		}
		else
			QW_PrintfToViewer(v, "\"menu %s\" not recognised\n", command);
	}

	else if (!strcmp(command, "tuimenu"))
	{
tuimenu:
		if (v->menunum)
			QW_SetMenu(v, MENU_NONE);
		else
			QW_SetMenu(v, MENU_MAIN);
	}
	else if (!strcmp(command, "guimenu"))
	{
		sv_t *sv;
		int y;
		qboolean shownheader;

guimenu:

		QW_SetMenu(v, MENU_NONE);

		shownheader = false;

/*
I've removed the following from this function as it covered the menu (~Moodles):
			"menupic 0 4 gfx/qplaque.lmp\n"
			"menupic 96 4 gfx/p_option.lmp\n"
*/
		QW_StuffcmdToViewer(v,

			"alias menucallback\n"
			"{\n"
				"menuclear\n"
				"if (option == \"OBSERVE\")\n"
					"{\necho Spectating server $_server\nsay .observe $_server\n}\n"
				"if (option == \"QTV\")\n"
					"{\necho Streaming from qtv at $_server\nsay .qtv $_server\n}\n"
				"if (option == \"JOIN\")\n"
					"{\necho Joining game at $_server\nsay .join $_server\n}\n"
				"if (option == \"ADMIN\")\n"
					"{\nsay .guiadmin\n}\n"
				"if (option == \"DEMOS\")\n"
					"{\nsay .demos\n}\n"
				"if (\"stream \" isin option)\n"
					"{\necho Changing stream\nsay .$option\n}\n"
			"}\n"

			"conmenu menucallback\n"

			"menuedit 48 36 \"^aServer:\" \"_server\"\n"

			"menutext 48 52 \"Demos\" DEMOS\n"

			"menutext 104 52 \"Join\" JOIN\n"

			"menutext 152 52 \"Observe\" OBSERVE\n"

			"menutext 224 52 \"QTV\" QTV\n"



			"menutext 48 84 \"Admin\" ADMIN\n"

			"menutext 48 92 \"Close Menu\" cancel\n"



			"menutext 48 116 \"Type in a server address and\"\n"
			"menutext 48 124 \"click join to play in the game,\"\n"
			"menutext 48 132 \"observe(udp) to watch, or qtv(tcp)\"\n"
			"menutext 48 140 \"to connect to a stream or proxy.\"\n"
			);

		y = 140+16;
		for (sv = cluster->servers; sv; sv = sv->next)
		{
			if (!shownheader)
			{
				shownheader = true;

				QW_StuffcmdToViewer(v, "menutext 72 %i \"^aActive Games:\"\n", y);
				y+=8;
			}
			QW_StuffcmdToViewer(v, "menutext 32 %i \"%30s\" \"stream %i\"\n", y, *sv->map.hostname?sv->map.hostname:sv->server, sv->streamid);
			y+=8;
		}
		if (!shownheader)
			QW_StuffcmdToViewer(v, "menutext 72 %i \"There are no active games\"\n", y);

	}

	else if (!strcmp(command, "demos"))
	{
		if (v->conmenussupported)
			goto guidemos;
		else
			goto tuidemos;
	}
	else if (!strcmp(command, "guidemos"))
	{
		int maxshowndemos;
		char sizestr[13];
		int start;
		int i;

guidemos:
		maxshowndemos = 12;

		if (!*args)
			Cluster_BuildAvailableDemoList(cluster);

		start = atoi(args);	//FIXME
		QW_SetMenu(v, MENU_NONE);

/*
I've removed the following from this function as it covered the menu (~Moodles):
			"menupic 0 4 gfx/qplaque.lmp\n"
			"menupic 96 4 gfx/p_option.lmp\n"
*/
		QW_StuffcmdToViewer(v,

			"alias menucallback\n"
			"{\n"
				"menuclear\n"
				"if (option == \"PREV\")\n"
					"{\nsay .demos %i\n}\n"
				"if (option == \"NEXT\")\n"
					"{\nsay .demos %i\n}\n"
				"if (\"demo \" isin option)\n"
					"{\necho Changing stream\nsay .$option\n}\n"
			"}\n"

			"conmenu menucallback\n",
			start - maxshowndemos, start + maxshowndemos
		);

		if (start < 0)
			start = 0;

		if (start-maxshowndemos >= 0)
			QW_StuffcmdToViewer(v, "menutext 48 52 \"Prev\" \"PREV\"\n");
		if (start+maxshowndemos <= cluster->availdemoscount)
			QW_StuffcmdToViewer(v, "menutext 152 52 \"Next\" \"NEXT\"\n");

		for (i = start; i < start+maxshowndemos; i++)
		{
			if (i >= cluster->availdemoscount)
				break;
			if (cluster->availdemos[i].size < 1024)
				snprintf(sizestr, sizeof(sizestr), "%4ib", cluster->availdemos[i].size);
			else if (cluster->availdemos[i].size < 1024*1024)
				snprintf(sizestr, sizeof(sizestr), "%4ikb", cluster->availdemos[i].size/1024);
			else if (cluster->availdemos[i].size < 1024*1024*1024)
				snprintf(sizestr, sizeof(sizestr), "%4imb", cluster->availdemos[i].size/(1024*1024));
			else// if (cluster->availdemos[i].size < 1024*1024*1024*1024)
				snprintf(sizestr, sizeof(sizestr), "%4igb", cluster->availdemos[i].size/(1024*1024*1024));
//			else
//				*sizestr = 0;
			QW_StuffcmdToViewer(v, "menutext 32 %i \"%6s %-30s\" \"demo %s\"\n", (i-start)*8 + 52+16, sizestr, cluster->availdemos[i].name, cluster->availdemos[i].name);
		}
	}
	else if (!strncmp(command, ".tuidemos", 9))
	{
tuidemos:
		if (!*args)
			Cluster_BuildAvailableDemoList(cluster);

		if (v->menunum == MENU_DEMOS)
			QW_SetMenu(v, MENU_NONE);
		else
			QW_SetMenu(v, MENU_DEMOS);
	}

	else if (!strcmp(command, "admin"))
	{
		if (v->conmenussupported)
			goto guiadmin;
		else
			goto tuiadmin;
	}

	else if (!strcmp(command, "guiadmin"))
	{
guiadmin:
		if (!*cluster->adminpassword)
		{
/*
I've removed the following from this function as it covered the menu (~Moodles):
			"menupic 16 4 gfx/qplaque.lmp\n"
			"menupic - 4 gfx/p_option.lmp\n"
*/
			QW_StuffcmdToViewer(v,

				"alias menucallback\n"
				"{\n"
					"menuclear\n"
				"}\n"

				"conmenu menucallback\n"

				"menutext 72 48 \"No admin password is set\"\n"
				"menutext 72 56 \"Admin access is prohibited\"\n"
				);
		}
		else if (v->isadmin)
			//already an admin, so don't show admin login screen
			QW_SetMenu(v, MENU_ADMIN);
		else
		{
/*
I've removed the following from this function as it covered the menu (~Moodles):
				"menupic 16 4 gfx/qplaque.lmp\n"
				"menupic - 4 gfx/p_option.lmp\n"
*/
			QW_StuffcmdToViewer(v,

				"alias menucallback\n"
				"{\n"
					"menuclear\n"
					"if (option == \"log\")\n"
						"{\nsay $_password\n}\n"
					"set _password \"\"\n"
				"}\n"

				"conmenu menucallback\n"

				"menuedit 16 32 \"        Password\" \"_password\"\n"

				"menutext 72 48 \"Log in QW\" log\n"
				"menutext 192 48 \"Cancel\" cancel\n"
				);

			strcpy(v->expectcommand, "admin");
		}
	}

	else if (!strcmp(command, "tuiadmin"))
	{
tuiadmin:
		if (!*cluster->adminpassword)
		{
			/*if (Netchan_IsLocal(v->netchan.remote_address))
			{
				Sys_Printf(cluster, "Local player %s logs in as admin\n", v->name);
				QW_SetMenu(v, MENU_ADMIN);
				v->isadmin = true;
			}
			else*/
				QW_PrintfToViewer(v, "There is no admin password set\nYou may not log in.\n");
		}
		else if (v->isadmin)
			QW_SetMenu(v, MENU_ADMIN);
		else
		{
			strcpy(v->expectcommand, "admin");
			QW_StuffcmdToViewer(v, "echo Please enter the rcon password\nmessagemode\n");
		}
	}

	else if (!strcmp(command, "reset"))
	{
		QW_SetCommentator(cluster, v, NULL);
		QW_SetViewersServer(cluster, v, NULL);
		QW_SetMenu(v, MENU_SERVERS);
	}
	else if (!strcmp(command, "connect") || !strcmp(command, "qw") || !strcmp(command, "observe") || !strcmp(command, "join"))
	{
		char buf[256];
		int isjoin = false;

		if (!strcmp(command, "join") || !strcmp(command, "connect"))
			isjoin = true;

		snprintf(buf, sizeof(buf), "udp:%s", args);
		qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_WHENEMPTY, !isjoin, false);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			if (isjoin)
				qtv->controller = v;
			QW_PrintfToViewer(v, "Connected to %s\n", qtv->server);
		}
		else if (cluster->nouserconnects)
			QW_PrintfToViewer(v, "you may not do that here\n");
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", buf);
	}
	else if (!strcmp(command, "qtv"))
	{
		char buf[256];

		snprintf(buf, sizeof(buf), "tcp:%s", args);
		qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_WHENEMPTY, true, false);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Connected to %s\n", qtv->server);
		}
		else if (cluster->nouserconnects)
			QW_PrintfToViewer(v, "Ask an admin to connect first\n");
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", buf);
	}
	else if (!strcmp(command, "qtvinfo"))
	{
		char buf[256];

		snprintf(buf, sizeof(buf), "[QuakeTV] %s\n", qtv->server);
		// Print a short line with info about the server
		QW_PrintfToViewer(v, "%s", buf);
	}
	else if (!strcmp(command, "stream"))
	{
		int id;
		id = atoi(args);
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (qtv->streamid == id)
			{
				break;
			}
		}
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Watching to %s\n", qtv->server);
		}
		else
		{
			QW_PrintfToViewer(v, "Stream \"%s\" not recognised. Stream id is invalid or terminated.\n", args);
		}
	}
	else if (!strcmp(command, "demo"))
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "file:%s", args);
		qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_WHENEMPTY, true, false);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Streaming from %s\n", qtv->server);
		}
		else
			QW_PrintfToViewer(v, "Demo \"%s\" does not exist on proxy\n", buf);
	}
	else if (!strcmp(command, "disconnect"))
	{
		QW_SetMenu(v, MENU_SERVERS);
		QW_SetViewersServer(cluster, v, NULL);
		QW_PrintfToViewer(v, "Connected\n");
	}
	else if (!strcmp(command, "clients"))
	{
		viewer_t *ov;
		int skipfirst = 0;
		int printable = 30;
		int remaining = 0;
		for (ov = cluster->viewers; ov; ov = ov->next)
		{
			if (skipfirst > 0)
			{
				skipfirst--;
			}
			else if (printable > 0)
			{
				printable--;
				if (ov->server)
				{
					if (ov->server->controller == ov)
						QW_PrintfToViewer(v, "%i: %s: *%s\n", ov->userid, ov->name, ov->server->server);
					else
						QW_PrintfToViewer(v, "%i: %s: %s\n", ov->userid, ov->name, ov->server->server);
				}
				else
					QW_PrintfToViewer(v, "%i: %s: %s\n", ov->userid, ov->name, "None");
			}
			else
				remaining++;
		}
		if (remaining)
			QW_PrintfToViewer(v, "%i clients not shown\n", remaining);
	}
	else if (!strcmp(command, "followid"))
	{
		int id = atoi(args);
		viewer_t *cv;

		for (cv = cluster->viewers; cv; cv = cv->next)
		{
			if (cv->userid == id)
			{
				QW_SetCommentator(cluster, v, cv);
				return;
			}
		}
		QW_PrintfToViewer(v, "Couldn't find that player\n");
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strcmp(command, "follow"))
	{
		int id = atoi(args);
		viewer_t *cv;

		for (cv = cluster->viewers; cv; cv = cv->next)
		{
			if (!strcmp(cv->name, args))
			{
				QW_SetCommentator(cluster, v, cv);
				return;
			}
		}
		if (id)
		{
			for (cv = cluster->viewers; cv; cv = cv->next)
			{
				if (cv->userid == id)
				{
					QW_SetCommentator(cluster, v, cv);
					return;
				}
			}
		}
		QW_PrintfToViewer(v, "Couldn't find that player\n");
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strcmp(command, "follow"))
	{
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strcmp(command, "bind"))
	{
		QW_StuffcmdToViewer(v, "bind uparrow +proxfwd\n");
		QW_StuffcmdToViewer(v, "bind downarrow +proxback\n");
		QW_StuffcmdToViewer(v, "bind rightarrow +proxright\n");
		QW_StuffcmdToViewer(v, "bind leftarrow +proxleft\n");
		QW_PrintfToViewer(v, "Keys bound\n");
	}
	else if (!strcmp(command, "bsay"))
	{
		char buf[1024];
		netmsg_t msg;

		viewer_t *ov;
		if (cluster->notalking)
			return;

		for (ov = cluster->viewers; ov; ov = ov->next)
		{
			InitNetMsg(&msg, buf, sizeof(buf));

			WriteByte(&msg, svc_print);

			if (ov->netchan.isnqprotocol)
				WriteByte(&msg, 1);
			else
			{
				if (ov->conmenussupported)
				{
					WriteByte(&msg, 3);	//PRINT_CHAT
					WriteString2(&msg, "[^sBQTV^s]^s^5");
				}
				else
				{
					WriteByte(&msg, 2);	//PRINT_HIGH
					WriteByte(&msg, 91+128);
					WriteString2(&msg, "BQTV");
					WriteByte(&msg, 93+128);
					WriteByte(&msg, 0);

					WriteByte(&msg, svc_print);
					WriteByte(&msg, 3);	//PRINT_CHAT

				}
			}

			WriteString2(&msg, v->name);
			WriteString2(&msg, ": ");
//				WriteString2(&msg, "\x8d ");
			WriteString2(&msg, args);
			WriteString(&msg, "\n");

			if (msg.maxsize == msg.cursize)
				return;
			SendBufferToViewer(ov, msg.data, msg.cursize, true);
		}
	}
	else
	{
		QW_PrintfToViewer(v, "QTV Proxy command not recognised\n");
	}
}

static void QTV_DoSay(cluster_t *cluster, sv_t *qtv, const char *viewername, char *message)
{
	char buf[1024];
	netmsg_t msg;
	viewer_t *ov;

	if (cluster->notalking)
		return;

	if (qtv)
		SV_SayToUpstream(qtv, message);

	for (ov = cluster->viewers; ov; ov = ov->next)
	{
		if (ov->server != qtv)
			continue;

		InitNetMsg(&msg, buf, sizeof(buf));

		WriteByte(&msg, svc_print);

		if (ov->netchan.isnqprotocol)
		{
			WriteByte(&msg, 1);
			WriteByte(&msg, '[');
			WriteString2(&msg, "QTV");
			WriteByte(&msg, ']');
		}
		else
		{
			if (ov->conmenussupported)
			{
				WriteByte(&msg, 2);	//PRINT_HIGH
				WriteByte(&msg, 91+128);
				WriteString2(&msg, "QTV");
				WriteByte(&msg, 93+128);
				WriteString2(&msg, "^5");
			}
			else
			{
				WriteByte(&msg, 2);	//PRINT_HIGH
				WriteByte(&msg, 91+128);
				WriteString2(&msg, "QTV");
				WriteByte(&msg, 93+128);
				WriteByte(&msg, 0);

				WriteByte(&msg, svc_print);
				WriteByte(&msg, 3);	//PRINT_CHAT

			}
		}

		WriteString2(&msg, viewername);
		WriteString2(&msg, ": ");
//		WriteString2(&msg, "\x8d ");
		if (ov->conmenussupported)
			WriteString2(&msg, "^s");
		WriteString2(&msg, message);
		WriteString(&msg, "\n");

		SendBufferToViewer(ov, msg.data, msg.cursize, true);
	}
}

void QTV_Say(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *message, qboolean noupwards)
{
	char buf[1024];

	if (message[strlen(message)-1] == '\"')
		message[strlen(message)-1] = '\0';

	if (*v->expectcommand)
	{
		buf[sizeof(buf)-1] = '\0';
		if (!strcmp(v->expectcommand, "hostname"))
		{
			strlcpy(cluster->hostname, message, sizeof(cluster->hostname));
		}
		else if (!strcmp(v->expectcommand, "master"))
		{
			strlcpy(cluster->master, message, sizeof(cluster->master));
			if (!strcmp(cluster->master, "."))
				*cluster->master = '\0';
			cluster->mastersendtime = cluster->curtime;
		}
		else if (!strcmp(v->expectcommand, "addserver"))
		{
			snprintf(buf, sizeof(buf), "tcp:%s", message);
			qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_NO, false, false);
			if (qtv)
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Connected to \"%s\"\n", message);
			}
			else
				QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
		}
		else if (!strcmp(v->expectcommand, "admin"))
		{
			if (!strcmp(message, cluster->adminpassword))
			{
				QW_SetMenu(v, MENU_ADMIN);
				v->isadmin = true;
				Sys_Printf(cluster, "Player %s logs in as admin\n", v->name);
			}
			else
			{
				QW_PrintfToViewer(v, "Admin password incorrect\n");
				Sys_Printf(cluster, "Player %s gets incorrect admin password\n", v->name);
			}
		}
		else if (!strcmp(v->expectcommand, "insecadddemo"))
		{
			snprintf(buf, sizeof(buf), "file:%s", message);
			qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_NO, false, false);
			if (!qtv)
				QW_PrintfToViewer(v, "Failed to play demo \"%s\"\n", message);
			else
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Opened demo file \"%s\".\n", message);
			}
		}

		else if (!strcmp(v->expectcommand, "adddemo"))
		{
			snprintf(buf, sizeof(buf), "file:%s", message);
			qtv = QTV_NewServerConnection(cluster, 0, buf, "", false, AD_NO, false, false);
			if (!qtv)
				QW_PrintfToViewer(v, "Failed to play demo \"%s\"\n", message);
			else
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Opened demo file \"%s\".\n", message);
			}
		}
		else if (!strcmp(v->expectcommand, "setmvdport"))
		{
			int newp = atoi(message);

			Net_TCPListen(cluster, newp, true);
			Net_TCPListen(cluster, newp, false);
			cluster->tcplistenportnum = newp;
		}
		else
		{
			QW_PrintfToViewer(v, "Command %s was not recognised\n", v->expectcommand);
		}

		*v->expectcommand = '\0';
		return;
	}

	if (*message == '.')
	{
		if (message[1] == '.')	//double it up to say it
			message++;
		else
		{
			//this is always execed (. is local server)
			QTV_SayCommand(cluster, qtv, v, message+1);
			return;
		}
	}
	else if (!strncmp(message, "proxy:", 6))
	{
		//this is execed on the 'active' server
		if (qtv && (qtv->controller == v && !qtv->proxyisselected))
			SendClientCommand(qtv, "say \"%s\"", message);
		else
			QTV_SayCommand(cluster, qtv, v, message+6);
		return;
	}
	else if (*message == ',')
	{
		if (message[1] == ',')	//double up to say it
			message++;
		else
		{
			if (qtv && (qtv->controller == v && qtv->serverisproxy))
				SendClientCommand(qtv, "say \"%s\"", message);
			else
				QTV_SayCommand(cluster, qtv, v, message+1);
			return;
		}
	}



	if (!strncmp(message, ".", 1))
		message++;
	*v->expectcommand = '\0';

	if (qtv && qtv->usequakeworldprotocols && !noupwards)
	{
		if (qtv->controller == v)
		{
			SendClientCommand(qtv, "say \"%s\"", message);

			if (cluster->notalking)
				return;
		}
		else
		{
			if (cluster->notalking)
				return;
			SendClientCommand(qtv, "say \"[%s]: %s\"", v->name, message);
		}

		//FIXME: we ought to broadcast this to everyone not watching that qtv.
	}
	else
	{
		// If the current viewer is the player, pass on the say_team
		if (qtv && qtv->controller == v)
		{
			SendClientCommand(qtv, "say_team \"%s\"", message);
			return;
		}

		QTV_DoSay(cluster, v->server, v->name, message);
	}
}

viewer_t *QW_IsOn(cluster_t *cluster, char *name)
{
	viewer_t *v;
	for (v = cluster->viewers; v; v = v->next)
		if (!stricmp(v->name, name))		//this needs to allow dequakified names.
			return v;

	return NULL;
}

void QW_PrintfToViewer(viewer_t *v, char *format, ...)
{
	int pos = 0;
	va_list		argptr;
	char buf[1024];

	buf[pos++] = svc_print;
	if (!v->netchan.isnqprotocol)
		buf[pos++] = 2;	//PRINT_HIGH

	va_start (argptr, format);
	vsnprintf (buf+pos, sizeof(buf)-pos, format, argptr);
	va_end (argptr);

	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}


void QW_StuffcmdToViewer(viewer_t *v, char *format, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, format);
	vsnprintf (buf+1, sizeof(buf)-1, format, argptr);
	va_end (argptr);

	buf[0] = svc_stufftext;
	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}

void QW_PositionAtIntermission(sv_t *qtv, viewer_t *v)
{
	netmsg_t msg;
	char buf[7];
	const intermission_t *spot;
	unsigned int pext1;


	if (qtv)
	{
		spot = BSP_IntermissionSpot(qtv->map.bsp);
		pext1 = qtv->pext1;
	}
	else
	{
		spot = &nullstreamspot;
		pext1 = 0;
	}


	v->origin[0] = spot->pos[0];
	v->origin[1] = spot->pos[1];
	v->origin[2] = spot->pos[2];

	InitNetMsg(&msg, buf, sizeof(buf));

	WriteByte (&msg, svc_setangle);
	WriteAngle(&msg, spot->angle[0], pext1);
	WriteAngle(&msg, spot->angle[1], pext1);
	WriteAngle(&msg, 0, pext1);

	SendBufferToViewer(v, msg.data, msg.cursize, true);
}

void ParseNQC(cluster_t *cluster, sv_t *qtv, viewer_t *v, netmsg_t *m)
{
	char buf[MAX_NQMSGLEN];
	netmsg_t msg;
	int mtype;

	while (m->readpos < m->cursize)
	{
		switch ((mtype=ReadByte(m)))
		{
		case clc_nop:
			break;
		case clc_stringcmd:
			ReadString (m, buf, sizeof(buf));
			printf("stringcmd: %s\n", buf);

			if (!strcmp(buf, "new"))
			{
				if (qtv && qtv->parsingconnectiondata)
					QW_StuffcmdToViewer(v, "cmd new\n");
				else
				{
					SendServerData(qtv, v);
				}
			}
			else if (!strncmp(buf, "prespawn", 8))
			{
				msg.data = buf;
				msg.maxsize = sizeof(buf);
				msg.cursize = 0;
				msg.overflowed = 0;

				if (qtv)
				{
					SendCurrentBaselines(qtv, 64, &msg, msg.maxsize, 0);
					SendCurrentLightmaps(qtv, 64, &msg, msg.maxsize, 0);

					SendStaticSounds(qtv, 64, &msg, msg.maxsize, 0);

					SendStaticEntities(qtv, 64, &msg, msg.maxsize, 0);
				}
				WriteByte (&msg, svc_nqsignonnum);
				WriteByte (&msg, 2);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}

			else if (!strncmp(buf, "setinfo", 5))
			{
				#define TOKENIZE_PUNCTUATION ""

				int i;
				char arg[3][ARG_LEN];
				char *command = buf;

				for (i = 0; i < 3; i++)
				{
					command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
				}

				Info_SetValueForStarKey(v->userinfo, arg[1], arg[2], sizeof(v->userinfo));
				ParseUserInfo(cluster, v);
//				Info_ValueForKey(v->userinfo, "name", v->name, sizeof(v->name));

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}

			else if (!strncmp(buf, "name ", 5))
			{
				Info_SetValueForStarKey(v->userinfo, "name", buf+5, sizeof(v->userinfo));
				ParseUserInfo(cluster, v);

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "setinfo name \"%s\"", v->name);
			}
			else if (!strncmp(buf, "color ", 6))
			{
				/*
				fixme
				*/
			}
			else if (!strncmp(buf, "spawn", 5))
			{
				msg.data = buf;
				msg.maxsize = sizeof(buf);
				msg.cursize = 0;
				msg.overflowed = 0;
				SendNQSpawnInfoToViewer(cluster, v, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);

				QW_PositionAtIntermission(qtv, v);

				v->thinksitsconnected = true;
			}
			else if (!strncmp(buf, "begin", 5))
			{
				int oldmenu;
				v->thinksitsconnected = true;

				oldmenu = v->menunum;
				QW_SetMenu(v, MENU_NONE);
				QW_SetMenu(v, oldmenu);

				if (!v->server)
					QTV_Say(cluster, v->server, v, ".menu", false);
			}

			else if (!strncmp(buf, "say \".", 6))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say .", 5))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (v->server && v == v->server->controller)
				SendClientCommand(v->server, "%s", buf);

	//		else if (!strcmp(buf, "pause"))
	//			qtv->errored = ERR_PAUSED;

			else if (!strncmp(buf, "say \"", 5))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say ", 4))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (!strncmp(buf, "say_team \"", 10))
				QTV_Say(cluster, qtv, v, buf+10, true);
			else if (!strncmp(buf, "say_team ", 9))
				QTV_Say(cluster, qtv, v, buf+9, true);


			else
			{
				QW_PrintfToViewer(v, "Command not recognised\n");
				Sys_Printf(cluster, "NQ client sent unrecognized stringcmd %s\n", buf);
			}
			break;
		case clc_disconnect:
			if (!v->drop)
				Sys_Printf(cluster, "NQ viewer %s disconnects\n", v->name);
			v->drop = true;
			return;
		case clc_move:
			v->ucmds[0] = v->ucmds[1];
			v->ucmds[1] = v->ucmds[2];
			ReadFloat(m);	//time, for pings
			//three angles
			{
				unsigned int pext1;
				if (v->server)
					pext1 = v->server->pext1;
				else
					pext1 = 0;
				v->ucmds[2].angles[0] = ReadAngle(m, pext1);
				v->ucmds[2].angles[1] = ReadAngle(m, pext1);
				v->ucmds[2].angles[2] = ReadAngle(m, pext1);
			}
			//three direction values
			v->ucmds[2].forwardmove = ReadShort(m);
			v->ucmds[2].sidemove = ReadShort(m);
			v->ucmds[2].upmove = ReadShort(m);

			//one button
			v->ucmds[2].buttons = ReadByte(m);
			//one impulse
			v->ucmds[2].impulse = ReadByte(m);

			v->ucmds[2].msec = cluster->curtime - v->lasttime;
			v->lasttime = cluster->curtime;

			if (v->menunum)
			{
				int mb = 0;
				if (v->ucmds[2].forwardmove > 0)	mb = MBTN_UP;
				if (v->ucmds[2].forwardmove < 0)	mb = MBTN_DOWN;
				if (v->ucmds[2].sidemove > 0)		mb = MBTN_RIGHT;
				if (v->ucmds[2].sidemove < 0)		mb = MBTN_LEFT;
				if (v->ucmds[2].buttons & 2)		mb = MBTN_ENTER;
				if (mb & ~v->menubuttons & MBTN_UP)		v->menuop -= 1;
				if (mb & ~v->menubuttons & MBTN_DOWN)	v->menuop += 1;
				if (mb & ~v->menubuttons & MBTN_RIGHT)	Menu_Enter(cluster, v, 1);
				if (mb & ~v->menubuttons & MBTN_LEFT)	Menu_Enter(cluster, v, -1);
				if (mb & ~v->menubuttons & MBTN_ENTER)	Menu_Enter(cluster, v, 0);
				if (v->menubuttons != mb)
					v->menuspamtime = cluster->curtime-1;
				v->ucmds[2].forwardmove = 0;
				v->ucmds[2].sidemove = 0;
				v->ucmds[2].buttons = 0;
				v->menubuttons = mb;
			}
			else
				v->menubuttons = ~0;	//so nothing gets instantly flagged once we enter a menu.

			if (v->server && v->server->controller == v)
				return;

			PMove(v, &v->ucmds[2]);

			if ((v->ucmds[1].buttons&1) != (v->ucmds[2].buttons&1) && (v->ucmds[2].buttons&1))
			{
				if(v->server)
				{
					int t;

					for (t = v->trackplayer+1; t < MAX_CLIENTS; t++)
					{
						if (v->server->map.players[t].active)
						{
							break;
						}
					}/*
					if (t == MAX_CLIENTS)
					for (t = 0; t <= v->trackplayer; t++)
					{
						if (v->server->players[t].active)
						{
							break;
						}
					}
					*/
					if (t >= MAX_CLIENTS)
					{
						if (v->trackplayer >= 0)
							QW_PrintfToViewer(v, "Stopped tracking\n");
						else
							QW_PrintfToViewer(v, "Not tracking\n");
						v->trackplayer = -1;	//no trackable players found
					}
					else
					{
						v->trackplayer = t;
						Info_ValueForKey(v->server->map.players[t].userinfo, "name", buf, sizeof(buf));
						QW_PrintfToViewer(v, "Now tracking: %s\n", buf);
					}
				}
			}
			if ((v->ucmds[1].buttons&2) != (v->ucmds[2].buttons&2) && (v->ucmds[2].buttons&2))
			{
				if (!v->server && !v->menunum)
					QW_SetMenu(v, MENU_DEFAULT);

				if(v->server)
				{
					int t;
					if (v->trackplayer < 0)
					{
						for (t = MAX_CLIENTS-1; t >= v->trackplayer; t--)
						{
							if (v->server->map.players[t].active)
							{
								break;
							}
						}
					}
					else
					{
						for (t = v->trackplayer-1; t >= 0; t--)
						{
							if (v->server->map.players[t].active)
							{
								break;
							}
						}
					}

					if (t < 0)
					{
						v->trackplayer = -1;	//no trackable players found
						QW_PrintfToViewer(v, "Not tracking\n");
					}
					else
					{
						v->trackplayer = t;
						Info_ValueForKey(v->server->map.players[t].userinfo, "name", buf, sizeof(buf));
						QW_PrintfToViewer(v, "Now tracking: %s\n", buf);
					}
				}
			}

			if (v->trackplayer > -1 && v->server)
			{
				v->origin[0] = v->server->map.players[v->trackplayer].current.origin[0];
				v->origin[1] = v->server->map.players[v->trackplayer].current.origin[1];
				v->origin[2] = v->server->map.players[v->trackplayer].current.origin[2];
			}

			break;
		default:
			Sys_Printf(cluster, "Bad message type %i\n", mtype);
			return;
		}
	}
}
void ParseQWC(cluster_t *cluster, sv_t *qtv, viewer_t *v, netmsg_t *m)
{
//	usercmd_t	oldest, oldcmd, newcmd;
	char buf[1024];
	netmsg_t msg;
	int i;
	int iscont;

	v->delta_frames[v->netchan.incoming_sequence & (ENTITY_FRAMES-1)] = -1;

	while (m->readpos < m->cursize)
	{
		i = ReadByte(m);
		switch (i)
		{
		case clc_nop:
			return;
		case clc_delta:
			v->delta_frames[v->netchan.incoming_sequence & (ENTITY_FRAMES-1)] = ReadByte(m);
			break;
		case clc_stringcmd:
			ReadString (m, buf, sizeof(buf));

			iscont = v->server && v->server->controller == v;

			if (!strncmp(buf, "cmd ", 4))
			{
				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf+4);
			}
			else if (iscont && !strncmp(buf, "pext ", 5))
			{	//FIXME: include ones we can parse properly...
				SendClientCommand(v->server, "pext");
			}
			else if (!iscont && !strcmp(buf, "new"))
			{
				if (qtv && qtv->parsingconnectiondata)
					QW_StuffcmdToViewer(v, "cmd new\n");
				else
				{
//					QW_StuffcmdToViewer(v, "//querycmd conmenu\n");
					SendServerData(qtv, v);
				}
			}
			else if (!iscont && !strncmp(buf, "modellist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessModelList, svc_modellist, &msg);
				else
					SendList(qtv, first, qtv->map.modellist, svc_modellist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!iscont && !strncmp(buf, "soundlist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessSoundList, svc_soundlist, &msg);
				else
					SendList(qtv, first, qtv->map.soundlist, svc_soundlist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!iscont && !strncmp(buf, "prespawn", 8))
			{
				char skin[128];

				if (atoi(buf + 9) != v->servercount)
					SendServerData(qtv, v);	//we're old.
				else
				{
					int crc;
					int r;
					char *s;
					s = buf+8;
					while(*s == ' ')
						s++;
					while((*s >= '0' && *s <= '9') || *s == '-')
						s++;
					while(*s == ' ')
						s++;
					r = atoi(s);

					if (r == 0)
					{
						while((*s >= '0' && *s <= '9') || *s == '-')
							s++;
						while(*s == ' ')
							s++;
						crc = atoi(s);

						if (qtv && qtv->controller == v)
						{
							if (!qtv->map.bsp)
							{
							//warning do we still actually need to do this ourselves? Or can we just forward what the user stated?
								QW_PrintfToViewer(v, "QTV doesn't have that map (%s), sorry.\n", qtv->map.modellist[1].name);
								qtv->errored = ERR_DROP;
							}
							else if (crc != BSP_Checksum(qtv->map.bsp))
							{
								QW_PrintfToViewer(v, "QTV's map (%s) does not match the servers\n", qtv->map.modellist[1].name);
								qtv->errored = ERR_DROP;
							}
						}
					}

					InitNetMsg(&msg, buf, sizeof(buf));

					if (qtv)
					{
						r = Prespawn(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}
					else
					{
						r = SendCurrentUserinfos(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						if (r > MAX_CLIENTS)
							r = -1;
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}

					if (r < 0)
						sprintf(skin, "%ccmd spawn\n", svc_stufftext);
					else
						sprintf(skin, "%ccmd prespawn %i %i\n", svc_stufftext, v->servercount, r);

					SendBufferToViewer(v, skin, strlen(skin)+1, true);
				}
			}
			else if (!iscont && !strncmp(buf, "spawn", 5))
			{
				char skin[64];
				sprintf(skin, "%cskins\n", svc_stufftext);
				SendBufferToViewer(v, skin, strlen(skin)+1, true);

				QW_PositionAtIntermission(qtv, v);
			}
			else if (iscont && !strncmp(buf, "begin", 5))
			{	//the client made it!
				v->thinksitsconnected = true;
				qtv->parsingconnectiondata = false;
				SendClientCommand(v->server, "%s", buf);
			}
			else if (!iscont && !strncmp(buf, "begin", 5))
			{
				int oldmenu;
				viewer_t *com;
				if (atoi(buf+6) != v->servercount)
					SendServerData(qtv, v);	//this is unfortunate!
				else
				{
					v->thinksitsconnected = true;
					if (qtv && qtv->map.ispaused)
					{
						char msgb[] = {svc_setpause, 1};
						SendBufferToViewer(v, msgb, sizeof(msgb), true);
					}

					oldmenu = v->menunum;
					QW_SetMenu(v, MENU_NONE);
					QW_SetMenu(v, oldmenu);


					com = v->commentator;
					v->commentator = NULL;
					QW_SetCommentator(cluster, v, com);

					if (v->firstconnect)
					{
						QW_StuffcmdToViewer(v, "f_qtv\n");
						v->firstconnect = false;
					}

					if (!v->server)
						QTV_Say(cluster, v->server, v, ".menu", false);
				}
			}
			else if (!strncmp(buf, "download", 8))
			{
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_download);
				WriteShort(&m, -1);
				WriteByte(&m, 0);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else if (!strncmp(buf, "drop", 4))
			{
				if (!v->drop)
					Sys_Printf(cluster, "QW viewer %s disconnects\n", v->name);
				v->drop = true;
			}
			else if (!strcmp(buf, "pause"))
			{
				if (qtv->errored == ERR_PAUSED)
					qtv->errored = ERR_NONE;
				else if (qtv->sourcetype == SRC_DEMO && (1 || v->isadmin))
					qtv->errored = ERR_PAUSED;
				else
					QW_PrintfToViewer(v, "You may not pause this stream\n");
			}
			else if (!strncmp(buf, "ison", 4))
			{
				viewer_t *other;
				if ((other = QW_IsOn(cluster, buf+5)))
				{
					if (!other->server)
						QW_PrintfToViewer(v, "%s is on the proxy, but not yet watching a game\n", other->name);
					else
						QW_PrintfToViewer(v, "%s is watching %s\n", buf+5, other->server->server);
				}
				else
					QW_PrintfToViewer(v, "%s is not on the proxy, sorry\n", buf+5);	//the apology is to make the alternatives distinct.
			}
			else if (!strncmp(buf, "ptrack ", 7))
			{
				v->trackplayer = atoi(buf+7);
//				if (v->trackplayer != MAX_CLIENTS-2)
//					QW_SetCommentator(v, NULL);
			}
			else if (!strncmp(buf, "ptrack", 6))
			{
				v->trackplayer = -1;
				QW_SetCommentator(cluster, v, NULL);	//clicking out will stop the client from tracking thier commentator
			}
			else if (!iscont && !strncmp(buf, "pings", 5))
			{
			}
			else if (!strncmp(buf, "say \"", 5))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say ", 4))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (!strncmp(buf, "say_team \"", 10))
				QTV_Say(cluster, qtv, v, buf+10, true);
			else if (!strncmp(buf, "say_team ", 9))
				QTV_Say(cluster, qtv, v, buf+9, true);

			else if (!strncmp(buf, "servers", 7))
			{
				QW_SetMenu(v, MENU_SERVERS);
			}

			else if (!strncmp(buf, "setinfo", 5))
			{
				#define TOKENIZE_PUNCTUATION ""

				int i;
				char arg[3][ARG_LEN];
				char *command = buf;

				for (i = 0; i < 3; i++)
				{
					command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
				}

				Info_SetValueForStarKey(v->userinfo, arg[1], arg[2], sizeof(v->userinfo));
				ParseUserInfo(cluster, v);
//				Info_ValueForKey(v->userinfo, "name", v->name, sizeof(v->name));

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}
			else if (!strncmp(buf, "cmdsupported ", 13))
			{
				if (!strcmp(buf+13, "conmenu"))
					v->conmenussupported = true;
				else if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}
			else if (!qtv)
			{
				//all the other things need an active server.
				QW_PrintfToViewer(v, "Choose a server first    DEBUG:(%s)\n", buf);
			}
			else if (!strncmp(buf, "serverinfo", 5))
			{
				char *key, *value, *end;
				int len;
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_print);
				WriteByte(&m, 2);
				end = qtv->map.serverinfo;
				for(;;)
				{
					if (!*end)
						break;
					key = end;
					value = strchr(key+1, '\\');
					if (!value)
						break;
					end = strchr(value+1, '\\');
					if (!end)
						end = value+strlen(value);

					len = value-key;

					key++;
					while(*key != '\\' && *key)
						WriteByte(&m, *key++);

					for (; len < 20; len++)
						WriteByte(&m, ' ');

					value++;
					while(*value != '\\' && *value)
						WriteByte(&m, *value++);
					WriteByte(&m, '\n');
				}
				WriteByte(&m, 0);

//				WriteString(&m, qtv->serverinfo);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else
			{
				if (v->server && v->server->controller == v)
				{
					SendClientCommand(v->server, "%s", buf);
				}
				else
					Sys_Printf(cluster, "Client sent unknown string command: %s\n", buf);
			}

			break;

		case clc_move:
			v->lost = ReadByte(m);
			ReadByte(m);
			ReadDeltaUsercmd(m, &nullcmd, &v->ucmds[0]);
			ReadDeltaUsercmd(m, &v->ucmds[0], &v->ucmds[1]);
			ReadDeltaUsercmd(m, &v->ucmds[1], &v->ucmds[2]);

			PMove(v, &v->ucmds[2]);

			if (v->ucmds[0].buttons & 2)
			{
				if (!v->server && !v->menunum)
					QW_SetMenu(v, MENU_DEFAULT);
			}
			break;
		case clc_tmove:
			v->origin[0] = ReadCoord(m, v->pext1);
			v->origin[1] = ReadCoord(m, v->pext1);
			v->origin[2] = ReadCoord(m, v->pext1);
			break;

		case clc_upload:
			Sys_Printf(cluster, "Client uploads are not supported from %s\n", v->name);
			v->drop = true;
			return;

		default:
			Sys_Printf(cluster, "bad clc from %s\n", v->name);
			v->drop = true;
			return;
		}
	}
}

static const char dropcmd[] = {svc_stufftext, 'd', 'i', 's', 'c', 'o', 'n', 'n', 'e', 'c', 't', '\n', '\0'};

void QW_FreeViewer(cluster_t *cluster, viewer_t *viewer)
{
	char buf[1024];
	viewer_t *oview;
	int i;
	//note: unlink them yourself.

	snprintf(buf, sizeof(buf), "%cQTV%c%s leaves the proxy\n", 91+128, 93+128, viewer->name);
	QW_StreamPrint(cluster, viewer->server, NULL, buf);

	Sys_Printf(cluster, "Dropping viewer %s\n", viewer->name);

	//spam them thrice, then forget about them
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);

	for (i = 0; i < MAX_BACK_BUFFERS; i++)
	{
		if (viewer->backbuf[i].data)
			free(viewer->backbuf[i].data);
	}

	if (viewer->server)
	{
		if (viewer->server->controller == viewer)
		{
			if (viewer->server->autodisconnect == AD_WHENEMPTY)
				viewer->server->errored = ERR_DROP;
			else
				viewer->server->controller = NULL;
		}

		viewer->server->numviewers--;
	}

	for (oview = cluster->viewers; oview; oview = oview->next)
	{
		if (oview->commentator == viewer)
			QW_SetCommentator(cluster, oview, NULL);
	}

	free(viewer);

	cluster->numviewers--;
}

void SendViewerPackets(cluster_t *cluster, viewer_t *v)
{
	char buffer[MAX_MSGLEN];
	netmsg_t m;
	int read;
	sv_t *useserver;

	v->drop |= v->netchan.drop;

	if (v->timeout < cluster->curtime)
	{
		Sys_Printf(cluster, "Viewer %s timed out\n", v->name);
		v->drop = true;
	}

	if (v->netchan.isnqprotocol && (v->server == NULL || v->server->parsingconnectiondata))
	{
		v->maysend = (v->nextpacket < cluster->curtime);
	}
	if (!Netchan_CanPacket(&v->netchan))
	{
		return;
	}
	if (v->maysend)	//don't send incompleate connection data.
	{
//		printf("maysend (%i, %i)\n", cluster->curtime, v->nextpacket);
		v->nextpacket = v->nextpacket + 1000/NQ_PACKETS_PER_SECOND;
		if (v->nextpacket < cluster->curtime)
			v->nextpacket = cluster->curtime;
		if (v->nextpacket > cluster->curtime+1000/NQ_PACKETS_PER_SECOND)
			v->nextpacket = cluster->curtime+1000/NQ_PACKETS_PER_SECOND;

		useserver = v->server;
		if (useserver && useserver->controller == v)
			v->netchan.outgoing_sequence = useserver->netchan.incoming_sequence;
		else
		{
			if (useserver && useserver->parsingconnectiondata)
				useserver = NULL;
		}

		v->maysend = false;
		InitNetMsg(&m, buffer, v->netchan.maxdatagramlen);
		m.cursize = 0;
		if (v->thinksitsconnected)
		{
			if (v->netchan.isnqprotocol)
				SendNQPlayerStates(cluster, useserver, v, &m);
			else
				SendPlayerStates(useserver, v, &m);
			UpdateStats(useserver, v);
		}
		if (v->menunum)
			Menu_Draw(cluster, v);
		else if (v->server && v->server->parsingconnectiondata && v->server->controller != v)
		{
			WriteByte(&m, svc_centerprint);
			WriteString(&m, v->server->status);
		}

		if (v->server && v->server->controller == v)
		{
			int saved;
			saved = v->netchan.incoming_sequence;
			v->netchan.incoming_sequence = v->server->netchan.incoming_sequence;
			Netchan_Transmit(cluster, &v->netchan, m.cursize, m.data);
			v->netchan.incoming_sequence = saved;
		}
		else
			Netchan_Transmit(cluster, &v->netchan, m.cursize, m.data);

		if (!v->netchan.message.cursize && v->backbuffered)
		{//shift the backbuffers around
			memcpy(v->netchan.message.data, v->backbuf[0].data,  v->backbuf[0].cursize);
			v->netchan.message.cursize = v->backbuf[0].cursize;
			for (read = 0; read < v->backbuffered; read++)
			{
				if (read == v->backbuffered-1)
				{
					v->backbuf[read].cursize = 0;
				}
				else
				{
					memcpy(v->backbuf[read].data, v->backbuf[read+1].data,  v->backbuf[read+1].cursize);
					v->backbuf[read].cursize = v->backbuf[read+1].cursize;
				}
			}
			v->backbuffered--;
		}
	}
//	else
//		printf("maynotsend (%i, %i)\n", cluster->curtime, v->nextpacket);
}

void QW_ProcessUDPPacket(cluster_t *cluster, netmsg_t *m, netadr_t from)
{
	char tempbuffer[256];
	int qport;

	viewer_t *v;
	sv_t *useserver;

	if (*(int*)m->data == -1)
	{	//connectionless message
		if (TURN_IsRequest(cluster, m, &from))
			return;
		m->readpos = 0;

		ConnectionlessPacket(cluster, &from, m);
		return;
	}

	if (m->cursize < 10)	//otherwise it's a runt or bad.
	{
		qport = 0;
	}
	else
	{
		//read the qport
		ReadLong(m);
		ReadLong(m);
		qport = ReadShort(m);
	}

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->netchan.isnqprotocol)
		{
			if (Net_CompareAddress(&v->netchan.remote_address, &from, 0, 1))
			{
				if (NQNetchan_Process(cluster, &v->netchan, m))
				{
					useserver = v->server;
					if (useserver && useserver->parsingconnectiondata)
						useserver = NULL;

					v->timeout = cluster->curtime + 15*1000;

					ParseNQC(cluster, useserver, v, m);

					if (v->server && v->server->controller == v)
					{
						QTV_Run(v->server);
					}
				}
				return;
			}
		}
		else
		{
			if (Net_CompareAddress(&v->netchan.remote_address, &from, v->netchan.qport, qport))
			{
				if (v->server && v->server->controller == v && v->maysend)
					SendViewerPackets(cluster, v);	//do this before we read the new sequences

				if (Netchan_Process(&v->netchan, m))
				{
					useserver = v->server;
					if (useserver && useserver->parsingconnectiondata && useserver->controller != v)
						useserver = NULL;

					v->timeout = cluster->curtime + 15*1000;

					if (v->server && v->server->controller == v)
					{
//						v->maysend = true;
						v->server->maysend = true;
					}
					else
					{
						v->netchan.outgoing_sequence = v->netchan.incoming_sequence;	//compensate for client->server packetloss.
						if (!v->server)
							v->maysend = true;
						else if (!v->chokeme || !cluster->chokeonnotupdated)
						{
							v->maysend = true;
							v->chokeme = cluster->chokeonnotupdated;
						}
					}

					ParseQWC(cluster, useserver, v, m);

					if (v->server && v->server->controller == v)
					{
						QTV_Run(v->server);
					}
				}
				return;
			}
		}
	}
	m->readpos = 0;

	if (TURN_IsRequest(cluster, m, &from))
		return;
	m->readpos = 0;

	if (cluster->allownqclients)
	{
		unsigned int ctrl;
		//NQ connectionless packet?
		ctrl = ReadLong(m);
		ctrl = SwapLong(ctrl);
		if (ctrl & NETFLAG_CTL)
		{	//looks hopeful
			switch(ReadByte(m))
			{
			case CCREQ_SERVER_INFO:
				ReadString(m, tempbuffer, sizeof(tempbuffer));
				if (!strcmp(tempbuffer, NQ_NETCHAN_GAMENAME))
				{
					m->cursize = 0;
					WriteLong(m, 0);
					WriteByte(m, CCREP_SERVER_INFO);
					WriteString(m, "??");
					WriteString(m, cluster->hostname);
					WriteString(m, "Quake TV");
					WriteByte(m, cluster->numviewers>255?255:cluster->numviewers);
					WriteByte(m, cluster->maxviewers>255?255:cluster->maxviewers);
					WriteByte(m, NQ_NETCHAN_VERSION);
					*(int*)m->data = BigLong(NETFLAG_CTL | m->cursize);
					NET_SendPacket(cluster, NET_ChooseSocket(cluster->qwdsocket, &from, from), m->cursize, m->data, from);
				}
				break;
			case CCREQ_CONNECT:
				ReadString(m, tempbuffer, sizeof(tempbuffer));
				if (!strcmp(tempbuffer, NQ_NETCHAN_GAMENAME))
				{
					if (ReadByte(m) == NQ_NETCHAN_VERSION)
					{
						//proquake extensions
						/*int mod =*/ ReadByte(m);
						/*int modver =*/ ReadByte(m);
						/*int flags =*/ ReadByte(m);
						/*int passwd =*/ ReadLong(m);

						//fte extension, sent so that dual-protocol servers will not create connections for dual-protocol clients
						//the connectnq command disables this (as well as the qw hand shake) if you really want to use nq protocols with fte clients
						ReadString(m, tempbuffer, sizeof(tempbuffer));
						if (!strncmp(tempbuffer, "getchallenge", 12))
							break;

						//drop any old nq clients from this address
						for (v = cluster->viewers; v; v = v->next)
						{
							if (v->netchan.isnqprotocol)
							{
								if (Net_CompareAddress(&v->netchan.remote_address, &from, 0, 1))
								{
									Sys_Printf(cluster, "Dup connect from %s\n", v->name);
									v->drop = true;
								}
							}
						}
						NewNQClient(cluster, &from);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

void QW_TCPConnection(cluster_t *cluster, oproxy_t *sock, char *initialstreamname)
{
	int alen;
	tcpconnect_t *tc;

	//clean up the pending source a bit...
	if (sock->srcfile) fclose(sock->srcfile), sock->srcfile = NULL;

	if (sock->drop)
		tc = NULL;	//FIXME
	else
		tc = malloc(sizeof(*tc));
	if (!tc)
	{
		closesocket(sock->sock);
		free(initialstreamname);
	}
	else
	{	//okay, we're adding this as a client
		//try and disable nagle, we don't really want to be wasting time not sending anything.
		int _true = 1;
		setsockopt(sock->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&_true, sizeof(_true));

		tc->sock = sock->sock;
		tc->websocket = sock->websocket;	//copy it over

		tc->inbuffersize = sock->inbuffersize;
		memcpy(tc->inbuffer, sock->inbuffer, tc->inbuffersize);
		tc->outbuffersize = sock->buffersize;
		memcpy(tc->outbuffer, sock->buffer+sock->bufferpos, tc->outbuffersize);

		memset(&tc->peeraddr, 0, sizeof(tc->peeraddr));
		tc->peeraddr.tcpcon = tc;

		alen = sizeof(tc->peeraddr.sockaddr);
		getpeername(sock->sock, (struct sockaddr*)&tc->peeraddr.sockaddr, &alen);

		tc->initialstreamname = initialstreamname;
		tc->next = cluster->tcpconnects;
		cluster->tcpconnects = tc;
	}

	//okay, we're done with it.
	free(sock);
	cluster->numproxies--;
}

void QW_UpdateUDPStuff(cluster_t *cluster)
{
	char buffer[MAX_MSGLEN];	//contains read info
	netadr_t from;
	int fromsize = sizeof(from.sockaddr);
	int read;
	netmsg_t m;
	int socketno;
	tcpconnect_t *tc, **l;

	viewer_t *v, *f;

	if (*cluster->master && (cluster->curtime > cluster->mastersendtime || cluster->mastersendtime > cluster->curtime + 4*1000*60))	//urm... time wrapped?
	{
		if (NET_StringToAddr(cluster->master, &from, 27000))
		{
			if (cluster->turnenabled)
				sprintf(buffer, "\377\377\377\377""heartbeat FTEMaster c=%s\n", cluster->chalkey);	//fill buffer with a heartbeat
			else if (cluster->protocolname)
				sprintf(buffer, "\377\377\377\377""heartbeat Darkplaces\n");	//older, broader compatibility.
			else
				sprintf(buffer, "a\n%i\n0\n", cluster->mastersequence++);	//fill buffer with a heartbeat
//why is there no \xff\xff\xff\xff ?..
			NET_SendPacket(cluster, NET_ChooseSocket(cluster->qwdsocket, &from, from), strlen(buffer), buffer, from);
		}
		else
			Sys_Printf(cluster, "Cannot resolve master %s\n", cluster->master);

		cluster->mastersendtime = cluster->curtime + 3*1000*60;	//3 minuites.
	}

	/* initialised for reading */
	InitNetMsg(&m, buffer, sizeof(buffer));

	socketno = 0;
	for (;;)
	{
		if (cluster->qwdsocket[socketno] == INVALID_SOCKET)
		{
			socketno++;
			if (socketno >= SOCKETGROUPS)
				break;
			continue;
		}
		memset(&from, 0, sizeof(from));
		read = recvfrom(cluster->qwdsocket[socketno], buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&from.sockaddr, (unsigned*)&fromsize);

		if (read < 0)	//it's bad.
		{
			socketno++;
			if (socketno >= SOCKETGROUPS)
				break;
			continue;
		}

		if (read <= 5)	//otherwise it's a runt or bad.
		{
			if (read == 1 && *buffer == 'l')
			{	//ffs. easier to just fix it up here.
				buffer[0] =
				buffer[1] =
				buffer[2] =
				buffer[3] = 0xff;
				buffer[4] = 'l';
				read = 5;
			}
			else
				continue;
		}

		m.cursize = read;
		m.data = buffer;
		m.readpos = 0;

		buffer[m.cursize] = 0;	//make sure its null terminated.
		QW_ProcessUDPPacket(cluster, &m, from);
	}

	for (tc = cluster->tcpconnects; tc; tc = tc->next)
	{
		if (tc->outbuffersize)
		{
			int clen = send(tc->sock, tc->outbuffer, tc->outbuffersize, 0);
			if (clen > 0)
			{
				memmove(tc->outbuffer, tc->outbuffer+clen, tc->outbuffersize-clen);
				tc->outbuffersize-=clen;
			}
		}
	}

	for (l = &cluster->tcpconnects; *l; )
	{
		int clen;
		tc = *l;
		read = sizeof(tc->inbuffer) - tc->inbuffersize;
		read = NET_WebSocketRecv(tc->sock, &tc->websocket, tc->inbuffer+tc->inbuffersize, read, &clen);
		if (read > 0)
			tc->inbuffersize += read;
		if (read == 0 || read < 0)
		{
			if (read == 0 || qerrno != NET_EWOULDBLOCK)
			{
				*l = tc->next;
				if (tc->sock != INVALID_SOCKET)
					closesocket(tc->sock);
				free(tc->initialstreamname);
				free(tc);
				continue;
			}
		}
		
		if (clen >= 0)
		{
			/*if it really is a webclient connection, then the stream will be packetized already
			so we don't waste extra space*/
			m.data = tc->inbuffer;
			m.readpos = 0;
			m.cursize = read;

			QW_ProcessUDPPacket(cluster, &m, tc->peeraddr);

			memmove(tc->inbuffer, tc->inbuffer+read, tc->inbuffersize - (read));
			tc->inbuffersize -= read;
			continue;	//ask to read the next packet
		}
		else
		{
			while (tc->inbuffersize >= 2)
			{
				read = (tc->inbuffer[0]<<8) | tc->inbuffer[1];
				if (tc->inbuffersize >= 2+read)
				{
					m.data = tc->inbuffer+2;
					m.readpos = 0;
					m.cursize = read;

					QW_ProcessUDPPacket(cluster, &m, tc->peeraddr);

					memmove(tc->inbuffer, tc->inbuffer+2+read, tc->inbuffersize - (2+read));
					tc->inbuffersize -= 2+read;
				}
				else
					break;
			}
		}


		l = &(*l)->next;
	}

	if (cluster->viewers && cluster->viewers->drop)
	{
//		Sys_Printf(cluster, "Dropping viewer %s\n", v->name);
		f = cluster->viewers;
		cluster->viewers = f->next;

		QW_FreeViewer(cluster, f);
	}

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->next && v->next->drop)
		{	//free the next/
//			Sys_Printf(cluster, "Dropping viewer %s\n", v->name);
			f = v->next;
			v->next = f->next;

			QW_FreeViewer(cluster, f);
		}

		SendViewerPackets(cluster, v);
	}
}
