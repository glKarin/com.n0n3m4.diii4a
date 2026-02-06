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
// sv_main.c -- server main program

#include "quakedef.h"
#include "pr_common.h"

#ifndef CLIENTONLY

#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

extern cvar_t sv_showpredloss;
extern cvar_t sv_gravity, sv_friction, sv_waterfriction, sv_gamespeed, sv_stopspeed, sv_spectatormaxspeed, sv_accelerate, sv_airaccelerate, sv_wateraccelerate, pm_edgefriction, sv_reliable_sound;
extern cvar_t  dpcompat_stats;

/*
=============================================================================

Con_Printf redirection

=============================================================================
*/

char	sv_redirected_buf[countof(sv_redirected_buf)];

redirect_t	sv_redirected;
int sv_redirectedlang;

extern cvar_t sv_phs;

/*
==================
SV_FlushRedirect
==================
*/
void SV_FlushRedirect (void)
{
	int totallen;
	char	send[sizeof(sv_redirected_buf)+6];

	if (!*sv_redirected_buf)
		return;

	if (sv_redirected == RD_PACKET || sv_redirected == RD_PACKET_LOG)
	{
		//log it to the rcon log if its not just a status response
		if (sv_redirected == RD_PACKET_LOG)
			Log_String(LOG_RCON, sv_redirected_buf);

		send[0] = 0xff;
		send[1] = 0xff;
		send[2] = 0xff;
		send[3] = 0xff;
		send[4] = A2C_PRINT;
		memcpy (send+5, sv_redirected_buf, strlen(sv_redirected_buf)+1);

		NET_SendPacket (svs.sockets, strlen(send)+1, send, &net_from);
	}
#ifdef SUBSERVERS
	else if (sv_redirected == RD_MASTER)
	{
		sizebuf_t s;

		memset(&s, 0, sizeof(s));
		s.data = send;
		s.maxsize = sizeof(send);
		s.cursize = 2;

		MSG_WriteByte(&s, ccmd_print);
		MSG_WriteString(&s, sv_redirected_buf);
		SSV_InstructMaster(&s);
	}
#endif
	else if (sv_redirected == RD_CLIENT)
	{
		int chop;
		char spare;
		char *s = sv_redirected_buf;
		totallen = strlen(s)+3;
		while (sizeof(host_client->backbuf_data[0])/2 < totallen)
		{
			chop = sizeof(host_client->backbuf_data[0]) / 2;
			spare = s[chop];
			s[chop] = '\0';

			ClientReliableWrite_Begin (host_client, host_client->protocol==SCP_QUAKE2?svcq2_print:svc_print, chop+3);
			ClientReliableWrite_Byte (host_client, PRINT_HIGH);
			ClientReliableWrite_String (host_client, s);

			s += chop;
			totallen -= chop;
			s[0] = spare;
		}
		ClientReliableWrite_Begin (host_client, host_client->protocol==SCP_QUAKE2?svcq2_print:svc_print, strlen(s)+3);
		ClientReliableWrite_Byte (host_client, PRINT_HIGH);
		ClientReliableWrite_String (host_client, s);
	}

	// clear it
	sv_redirected_buf[0] = 0;
}


/*
==================
SV_BeginRedirect

  Send Con_Printf data to the remote client
  instead of the console
==================
*/
void SV_BeginRedirect (redirect_t rd, int lang)
{
	SV_FlushRedirect();

	sv_redirected = rd;
	sv_redirectedlang = lang;
	sv_redirected_buf[0] = 0;
}

void SV_EndRedirect (void)
{
	SV_FlushRedirect ();
	sv_redirectedlang = 0;	//clenliness rather than functionality. Shouldn't be needed.
	sv_redirected = RD_NONE;
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

//Directly print to a client without translating nor printing into an mvd. generally for error messages due to the lack of mvd thing.
void SV_PrintToClient(client_t *cl, int level, const char *string)
{
	if (cl->controller)
		cl = cl->controller;

	switch (cl->protocol)
	{
	case SCP_BAD:	//bot
		break;
	case SCP_QUAKE2:
	case SCP_QUAKE2EX:
#ifdef Q2SERVER
		ClientReliableWrite_Begin (cl, svcq2_print, strlen(string)+3);
		ClientReliableWrite_Byte (cl, level);
		ClientReliableWrite_String (cl, string);
#endif
		break;
	case SCP_QUAKE3:
		break;
	case SCP_QUAKEWORLD:
		ClientReliableWrite_Begin (cl, svc_print, strlen(string)+3);
		ClientReliableWrite_Byte (cl, level);
		ClientReliableWrite_String (cl, string);
		break;
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
#ifdef NQPROT
		ClientReliableWrite_Begin (cl, svc_print, strlen(string)+3);
		if (level == PRINT_CHAT)
			ClientReliableWrite_Byte (cl, 1);
		ClientReliableWrite_String (cl, string);
#endif
		break;
	}
}
//translate it, but avoid 'public' mvd prints.
void SV_TPrintToClient(client_t *cl, int level, const char *string)
{
	string = langtext(string, cl->language);
	SV_PrintToClient(cl, level, string);
}

void SV_StuffcmdToClient(client_t *cl, const char *string)
{
	switch (cl->protocol)
	{
	case SCP_BAD:	//bot
		break;
	case SCP_QUAKE2:
	case SCP_QUAKE2EX:
#ifdef Q2SERVER
		ClientReliableWrite_Begin (cl, svcq2_stufftext, strlen(string)+3);
		ClientReliableWrite_String (cl, string);
#endif
		break;
	case SCP_QUAKE3:
		break;
	case SCP_QUAKEWORLD:
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
		if (cl->controller)
		{	//this is a slave client.
			//find the right number and send.
			int pnum = 0;
			client_t *sp;
			for (sp = cl->controller; sp; sp = sp->controlled)
			{
				if (sp == cl)
					break;
				pnum++;
			}
			sp = cl->controller;

			ClientReliableWrite_Begin (sp, svcfte_choosesplitclient, 4 + strlen(string));
			ClientReliableWrite_Byte (sp, pnum);
			ClientReliableWrite_Byte (sp, svc_stufftext);
			ClientReliableWrite_String (sp, string);
		}
		else
		{
			ClientReliableWrite_Begin (cl, svc_stufftext, strlen(string)+3);
			ClientReliableWrite_String (cl, string);
		}
		break;
	}
}
void SV_StuffcmdToClient_Unreliable(client_t *cl, const char *string)
{
	switch (cl->protocol)
	{
	case SCP_BAD:	//bot
		break;
	case SCP_QUAKE2:
	case SCP_QUAKE2EX:
#ifdef Q2SERVER
		ClientReliableWrite_Begin (cl, svcq2_stufftext, strlen(string)+3);
		ClientReliableWrite_String (cl, string);
#endif
		break;
	case SCP_QUAKE3:
		break;
	case SCP_QUAKEWORLD:
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
		if (cl->controller)
		{	//this is a slave client.
			//find the right number and send.
			int pnum = 0;
			client_t *sp;
			for (sp = cl->controller; sp; sp = sp->controlled)
			{
				if (sp == cl)
					break;
				pnum++;
			}
			sp = cl->controller;

			MSG_WriteByte (&sp->datagram, svcfte_choosesplitclient);
			MSG_WriteByte (&sp->datagram, pnum);
			MSG_WriteByte (&sp->datagram, svc_stufftext);
			MSG_WriteString (&sp->datagram, string);
		}
		else
		{
			MSG_WriteByte(&cl->datagram, svc_stufftext);
			MSG_WriteString(&cl->datagram, string);
		}
		break;
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
Is included in mvds.
=================
*/
void VARGS SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if (level < cl->messagelevel)
		return;

	va_start (argptr,fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_ClientPrintf: Buffer stomped\n");

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		sizebuf_t *msg = MVDWrite_Begin (dem_single, cl - svs.clients, strlen(string)+3);
		MSG_WriteByte (msg, svc_print);
		MSG_WriteByte (msg, level);
		MSG_WriteString (msg, string);
	}
#endif

	SV_PrintToClient(cl, level, string);
}

void VARGS SV_ClientTPrintf (client_t *cl, int level, translation_t stringnum, ...)
{
	va_list		argptr;
	char		string[1024];
	const char *fmt = langtext(stringnum, cl->language);

	if (level < cl->messagelevel)
		return;

	va_start (argptr,stringnum);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_ClientTPrintf: Buffer stomped\n");

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		sizebuf_t *msg = MVDWrite_Begin (dem_single, cl - svs.clients, strlen(string)+3);
		MSG_WriteByte (msg, svc_print);
		MSG_WriteByte (msg, level);
		MSG_WriteString (msg, string);
	}
#endif

	SV_PrintToClient(cl, level, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrint (unsigned int flags, int level, const char *string)
{
	client_t	*cl;
	int			i;

	if (!(flags & BPRINT_IGNORECONSOLE))
	{
		//pretend to print on the server, but not to the client's console
		Sys_Printf ("%s", string);	// print to the system console
		Log_String(LOG_CONSOLE, string);	//dump into log
	}

	if (!(flags & BPRINT_IGNORECLIENTS))
	{
		for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
		{
			if (level < cl->messagelevel)
				continue;
			if (!cl->state)
				continue;
			if (cl->protocol == SCP_BAD)
				continue;

			if (cl == sv.skipbprintclient)	//silence bprints about the player in ClientConnect. NQ completely wipes the buffer after clientconnect, which is what traditionally hides it.
				continue;

			if (cl->controller)
				continue;

			SV_PrintToClient(cl, level, string);
		}
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording && !(flags&BPRINT_IGNOREINDEMO))
	{
		sizebuf_t *msg = MVDWrite_Begin (dem_all, 0, strlen(string)+3);
		MSG_WriteByte (msg, svc_print);
		MSG_WriteByte (msg, level);
		MSG_WriteString (msg, string);
	}
#endif
}

void SV_BroadcastPrint_QexLoc (unsigned int flags, int level, const char **arg, int args)
{
	client_t	*cl;
	int			i;

	char string[1024];

	if (!(flags & BPRINT_IGNORECONSOLE))
	{
		//pretend to print on the server, but not to the client's console
		TL_Reformat(com_language, string, sizeof(string), args, arg);
		Sys_Printf ("%s", string);	// print to the system console
		Log_String(LOG_CONSOLE, string);	//dump into log
	}

	if (!(flags & BPRINT_IGNORECLIENTS))
	{
		for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
		{
			if (level < cl->messagelevel)
				continue;
			if (!cl->state)
				continue;
			if (cl->protocol == SCP_BAD)
				continue;

			if (cl == sv.skipbprintclient)	//silence bprints about the player in ClientConnect. NQ completely wipes the buffer after clientconnect, which is what traditionally hides it.
				continue;

			if (cl->controller)
				continue;

			if (cl->qex)
			{	//get the damn client to do it.
				int size = 3;
				for (i = 0; i < args; i++)
					size += strlen(arg[i])+1;
				ClientReliableWrite_Begin (cl, svcqex_locprint, size);
				ClientReliableWrite_Short (cl, args);
				for (i = 0; i < args; i++)
					ClientReliableWrite_String (cl, arg[i]);
			}
			else
			{
				TL_Reformat(cl->language, string, sizeof(string), args, arg);
				SV_PrintToClient(cl, level, string);
			}
		}
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording && !(flags&BPRINT_IGNOREINDEMO))
	{
		sizebuf_t *msg;
		TL_Reformat(com_language, string, sizeof(string), args, arg);
		msg = MVDWrite_Begin (dem_all, 0, strlen(string)+3);
		MSG_WriteByte (msg, svc_print);
		MSG_WriteByte (msg, level);
		MSG_WriteString (msg, string);
	}
#endif
}

void VARGS SV_BroadcastPrintf (int level, const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	va_start (argptr,fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);
	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");
	SV_BroadcastPrint(0, level, string);
}


void VARGS SV_BroadcastTPrintf (int level, translation_t stringnum, ...)
{
	va_list		argptr;
	char		string[1024];
	client_t	*cl;
	int			i;
	int oldlang=-1;
	const char *fmt = langtext(stringnum, oldlang=com_language);

	va_start (argptr,stringnum);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");

	//pretend to print on the server, but not to the client's console
	Sys_Printf ("%s", string);	// print to the console
	Log_String(LOG_CONSOLE, string);	//dump into log

	for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (!cl->state)
			continue;
		if (cl->controller)
			continue;

		if (oldlang!=cl->language)
		{
			fmt = langtext(stringnum, oldlang=cl->language);

			va_start (argptr,stringnum);
			vsnprintf (string,sizeof(string)-1, fmt,argptr);
			va_end (argptr);

			if(strlen(string) >= sizeof(string))
				Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");
		}

		SV_PrintToClient(cl, level, string);
	}
}


/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void VARGS SV_BroadcastCommand (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int i;
	client_t *cl;

	if (!sv.state)
		return;
	va_start (argptr,fmt);
	vsnprintf (string,sizeof(string), fmt,argptr);
	va_end (argptr);

	for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
	{
		if (cl->controller)
			continue;
		if (cl->state>=cs_connected)
		{
			if (ISQWCLIENT(cl) || ISNQCLIENT(cl))
			{
				ClientReliableWrite_Begin(cl, svc_stufftext, strlen(string)+2);
				ClientReliableWrite_String (cl, string);
			}
			else if (ISQ2CLIENT(cl))
			{
				ClientReliableWrite_Begin(cl, svcq2_stufftext, strlen(string)+2);
				ClientReliableWrite_String (cl, string);
			}
		}
	}
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org

MULTICAST_ONE	sent to a single client.
MULTICAST_INIT	sent to clients when they first connect. for completeness.
=================
*/
void SV_MulticastProtExt(vec3_t origin, multicast_t to, int dimension_mask, int with, int without)
{
	client_t	*client;
	qbyte		*mask;
	int			cluster, area1, area2;
	int			j;
	qboolean	reliable;
	client_t	*oneclient = NULL, *split;
	int seat;
	qboolean	andspecs = false;
	extern cvar_t sv_nopvs;

	if (!sv.multicast.cursize 
#ifdef NQPROT
		&& !sv.nqmulticast.cursize
#endif
#ifdef Q2SERVER
		&& !sv.q2multicast[0].cursize
#endif
		)
		return;

	if (to == MULTICAST_INIT)
	{
		//we only have one signon buffer. make sure you don't put non-identical protocols in the buffer
		SV_FlushSignon(false);
		SZ_Write (&sv.signon, sv.multicast.data, sv.multicast.cursize);

		//and send to players that are already on
		to = MULTICAST_ALL_R;
	}

//	to = MULTICAST_ALL;
	//don't let things crash if the world model went away. can happen in broadcasts when reloading video with the map no longer available causing the server to die with the resulting broadcast messages about players dropping or gib effects appearing
	if (sv.world.worldmodel->loadstate != MLS_LOADED || !sv.world.worldmodel->nodes)
	{
		switch(to)
		{
		case MULTICAST_PHS_R:
		case MULTICAST_PVS_R:
			to = MULTICAST_ALL_R;
			break;
		case MULTICAST_PHS:
		case MULTICAST_PVS:
			to = MULTICAST_ALL;
			break;
		default:
			break;
		}
	}

	{
		reliable = false;
		area1=-1;
		area2=-1;

		switch (to)
		{
		case MULTICAST_ALL_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_ALL:
			mask = NULL;
			break;

		case MULTICAST_PHS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PHS:
			if (!sv.world.worldmodel->phs || sv_nopvs.ival)	/*broadcast if no pvs*/
				mask = NULL;
			else
			{
				cluster = sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, origin, &area1);
				if (cluster >= 0)
					mask = sv.world.worldmodel->funcs.ClusterPHS(sv.world.worldmodel, cluster, NULL);
				else
					mask = NULL;
			}
			break;

		case MULTICAST_PVS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PVS:
			if (sv_nopvs.ival)
				mask = NULL;
			else
			{
				cluster = sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, origin, &area1);
				if (cluster >= 0)
					mask = sv.world.worldmodel->funcs.ClusterPVS(sv.world.worldmodel, cluster, NULL, PVM_FAST);
				else
					mask = NULL;
			}
			break;

		case MULTICAST_ONE_R_NOSPECS:
		case MULTICAST_ONE_R_SPECS:
			reliable = true;
		case MULTICAST_ONE_NOSPECS:
		case MULTICAST_ONE_SPECS:
			if (svprogfuncs)
			{
				edict_t *ent = PROG_TO_EDICT(svprogfuncs, pr_global_struct->msg_entity);
				oneclient = svs.clients + NUM_FOR_EDICT(svprogfuncs, ent) - 1;
			}
			else
				oneclient = NULL;
			mask = NULL;
			andspecs = (to==MULTICAST_ONE_R_SPECS||to==MULTICAST_ONE_SPECS);
			break;

		default:
			mask = NULL;
			SV_Error ("SV_Multicast: bad to:%i", to);
		}

		// send the data to all relevent clients
		for (j = 0, client = svs.clients; j < sv.allocated_client_slots; j++, client++)
		{
			if (client->state != cs_spawned)
				continue;

			if (client->controller)
				continue;

			for (split = client, seat = 0; split; split = split->controlled, seat++)
			{
				//if (split->protocol == SCP_QUAKEWORLD)
				{
					if (split->fteprotocolextensions & without)
					{
			//			Con_Printf ("Version supressed multicast - without pext\n");
						continue;
					}
					if (!(split->fteprotocolextensions & with) && with)
					{
			//			Con_Printf ("Version supressed multicast - with pext\n");
						continue;
					}
				}

				if (split->penalties & BAN_BLIND)
					continue;

				if (oneclient)
				{
					if (oneclient != split)
					{
						if (andspecs && split->spectator && split->spec_track > 0 && oneclient == &svs.clients[split->spec_track - 1])
							;
						else
							continue;
					}
				}
				else
				{
					vec3_t pos;
					if (svprogfuncs)
					{
						if (!((int)split->edict->xv->dimension_see & dimension_mask))
							continue;
						VectorAdd(split->edict->v->origin, split->edict->v->view_ofs, pos);
					}
#ifdef Q2SERVER
					else if (ge)
						VectorCopy(split->q2edict->s.origin, pos);
#endif
					else
						continue;	//no idea where the player is...

					if (!mask)	//no pvs? broadcast.
						break;

					if (to == MULTICAST_PHS_R || to == MULTICAST_PHS)
					{	//always in range if within 1024 units (consistent with quakeworld).
						vec3_t delta;
						VectorSubtract(origin, pos, delta);
						if (DotProduct(delta, delta) <= 1024*1024)
							break;
					}

					cluster = sv.world.worldmodel->funcs.ClusterForPoint (sv.world.worldmodel, pos, &area2);
					if (cluster>= 0 && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ||
						(sv.world.worldmodel->funcs.AreasConnected && !sv.world.worldmodel->funcs.AreasConnected (sv.world.worldmodel, area1, area2))))
					{
		//				Con_Printf ("PVS supressed multicast\n");
						continue;
					}
				}
				break;
			}
			if (!split)
				continue;

			safeswitch (client->protocol)
			{
			case SCP_BAD:
				continue;	//a bot.
			safedefault:
				SV_Error("multicast: Client is using a bad protocol");

			case SCP_QUAKE3:
				Con_Printf("Skipping multicast for q3 client\n");
				break;

			case SCP_NETQUAKE:
			case SCP_BJP3:
			case SCP_FITZ666:
			case SCP_DARKPLACES6:
			case SCP_DARKPLACES7:	//extra prediction stuff
#ifdef NQPROT
				if (reliable)
				{
					ClientReliableCheckBlock(client, sv.nqmulticast.cursize);
					ClientReliableWrite_SZ(client, sv.nqmulticast.data, sv.nqmulticast.cursize);
				}
				else
					SZ_Write (&client->datagram, sv.nqmulticast.data, sv.nqmulticast.cursize);
#endif
				break;
			case SCP_QUAKE2:
#ifdef Q2SERVER
				if (reliable)
				{
					ClientReliableCheckBlock(client, sv.q2multicast[0].cursize);
					ClientReliableWrite_SZ(client, sv.q2multicast[0].data, sv.q2multicast[0].cursize);
				}
				else
					SZ_Write (&client->datagram, sv.q2multicast[0].data, sv.q2multicast[0].cursize);
#endif
				break;
			case SCP_QUAKE2EX:
#ifdef Q2SERVER
				if (reliable)
				{
					ClientReliableCheckBlock(client, sv.q2multicast[1].cursize);
					ClientReliableWrite_SZ(client, sv.q2multicast[1].data, sv.q2multicast[1].cursize);
				}
				else
					SZ_Write (&client->datagram, sv.q2multicast[1].data, sv.q2multicast[1].cursize);
#endif
				break;
			case SCP_QUAKEWORLD:
				if (reliable)
				{
					if (oneclient && seat)
					{
						ClientReliableCheckBlock(client, 2+sv.multicast.cursize);
						ClientReliableWrite_Byte(client, svcfte_choosesplitclient);
						ClientReliableWrite_Byte(client, seat);
					}
					else
						ClientReliableCheckBlock(client, sv.multicast.cursize);

					ClientReliableWrite_SZ(client, sv.multicast.data, sv.multicast.cursize);
				}
				else
				{
					if (oneclient && seat)
					{
						MSG_WriteByte (&client->datagram, svcfte_choosesplitclient);
						MSG_WriteByte (&client->datagram, seat);
					}
					SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);
				}
				break;
			}
		}
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording && ((demo.recorder.fteprotocolextensions & with) == with) && !(demo.recorder.fteprotocolextensions & without))
	{
		sizebuf_t *msg;

		switch(to)
		{
		//mvds have no idea where the receiver's camera will be.
		//as such, they cannot have any support for pvs/phs
		case MULTICAST_INIT:
		default:
		case MULTICAST_ALL_R:
		case MULTICAST_PHS_R:
		case MULTICAST_PVS_R:
			msg = MVDWrite_Begin(dem_all, 0, sv.multicast.cursize);
			break;
		case MULTICAST_ALL:
		case MULTICAST_PHS:
		case MULTICAST_PVS:
			msg = &demo.datagram;
			break;

		case MULTICAST_ONE_R_NOSPECS:
		case MULTICAST_ONE_NOSPECS:
			msg = &demo.datagram;
			sv.multicast.cursize = 0;
			break;

		//mvds are all reliables really.
		case MULTICAST_ONE_R_SPECS:
		case MULTICAST_ONE_SPECS:
			{
				int pnum;
				if (svprogfuncs)
				{
					edict_t *ent = PROG_TO_EDICT(svprogfuncs, pr_global_struct->msg_entity);
					pnum = NUM_FOR_EDICT(svprogfuncs, ent) - 1;
				}
				else
				{
					pnum = 0;	//FIXME
					Con_Printf("SV_MulticastProtExt: unsupported unicast\n");
				}
				msg = MVDWrite_Begin(dem_single, pnum, sv.multicast.cursize);
			}
			break;
		}
		SZ_Write(msg, sv.multicast.data, sv.multicast.cursize);
	}
#endif

#ifdef NQPROT
	SZ_Clear (&sv.nqmulticast);
#endif
#ifdef Q2SERVER
	SZ_Clear (&sv.q2multicast[0]);
	SZ_Clear (&sv.q2multicast[1]);
#endif
	SZ_Clear (&sv.multicast);
}

void SV_MulticastCB(vec3_t origin, multicast_t to, const char *reliableinfokey, int dimension_mask, void (*callback)(client_t *cl, sizebuf_t *msg, void *ctx), void *ctx)
{
	qboolean reliable = false, doreliable;

	client_t	*client;
	qbyte		*mask;
	int			cluster;
	int			j;
	client_t	*oneclient = NULL, *split;
	qboolean	andspecs = false;

	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_ALL:
		mask = NULL;
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PHS:
		if (!sv.world.worldmodel->phs)	/*broadcast if no pvs*/
			mask = NULL;
		else
		{
			cluster = sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, origin, NULL);
			if (cluster >= 0)
				mask = sv.world.worldmodel->phs + cluster * sv.world.worldmodel->pvsbytes;
			else
				mask = NULL;
		}
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PVS:
		cluster = sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, origin, NULL);
		if (cluster >= 0)
			mask = sv.world.worldmodel->funcs.ClusterPVS(sv.world.worldmodel, cluster, NULL, PVM_FAST);
		else
			mask = NULL;
		break;

	case MULTICAST_ONE_R_NOSPECS:
	case MULTICAST_ONE_R_SPECS:
		reliable = true;

	case MULTICAST_ONE_NOSPECS:
	case MULTICAST_ONE_SPECS:
		if (svprogfuncs)
		{
			edict_t *ent = PROG_TO_EDICT(svprogfuncs, pr_global_struct->msg_entity);
			oneclient = svs.clients + NUM_FOR_EDICT(svprogfuncs, ent) - 1;
		}
		else
			oneclient = NULL;
		mask = NULL;
		andspecs = (to == MULTICAST_ONE_R_SPECS || to == MULTICAST_ONE_SPECS);
		break;

	default:
		mask = NULL;
		SV_Error ("SV_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < sv.allocated_client_slots; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;

		if (client->controller)
			continue;
		if (client->protocol == SCP_BAD)
			continue;	//a bot.

		for (split = client; split; split = split->controlled)
		{
			if (split->penalties & BAN_BLIND)
				continue;

			if (oneclient)
			{
				if (oneclient != split)
				{
					if (andspecs && split->spectator && split->spec_track >= 0 && oneclient == &svs.clients[split->spec_track])
						;
					else
						continue;
				}
			}
			else if (svprogfuncs)
			{
				if (!((int)split->edict->xv->dimension_see & dimension_mask))
					continue;

				if (!mask)	//no pvs? broadcast.
					break;

				if (to == MULTICAST_PHS_R || to == MULTICAST_PHS)
				{	//phs is always 'visible' within 1024qu
					vec3_t delta;
					VectorSubtract(origin, split->edict->v->origin, delta);
					if (DotProduct(delta, delta) <= 1024*1024)
						break;
				}

				{
					vec3_t pos;
					VectorAdd(split->edict->v->origin, split->edict->v->view_ofs, pos);
					cluster = sv.world.worldmodel->funcs.ClusterForPoint (sv.world.worldmodel, pos, NULL);
					if (cluster>= 0 && !(mask[cluster>>3] & (1<<(cluster&7)) ) )
					{
		//				Con_Printf ("PVS supressed multicast\n");
						continue;
					}
				}
			}
			break;
		}
		if (!split)
			continue;

		doreliable = reliable;
		if (reliableinfokey)
		{	//allow the user to override reliable state according to a userinfo key (primarily "rsnd" right now, but hey).
			const char *v = InfoBuf_ValueForKey(&client->userinfo, reliableinfokey);
			if (*v)
				doreliable = atoi(v);
		}

		if (doreliable)
		{
			char msgbuf[8192];
			sizebuf_t msg = {0};
			msg.data = msgbuf;
			msg.maxsize = sizeof(msgbuf);
			msg.prim = client->datagram.prim;
			callback(client, &msg, ctx);
			if (msg.cursize)
			{
				ClientReliableCheckBlock(client, msg.cursize);
				ClientReliableWrite_SZ(client, msg.data, msg.cursize);
			}
		}
		else
			callback(client, &client->datagram, ctx);
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		sizebuf_t *msg;
		unsigned int maxsize = 1024;

		switch(to)
		{
		//mvds have no idea where the receiver's camera will be.
		//as such, they cannot have any support for pvs/phs
		case MULTICAST_INIT:
		default:
		case MULTICAST_ALL_R:
		case MULTICAST_PHS_R:
		case MULTICAST_PVS_R:
			msg = MVDWrite_Begin(dem_all, 0, maxsize);
			break;
		case MULTICAST_ALL:
		case MULTICAST_PHS:
		case MULTICAST_PVS:
			msg = &demo.datagram;
			break;

		case MULTICAST_ONE_R_NOSPECS:
		case MULTICAST_ONE_NOSPECS:
			return;	//demos count as spectators.

		//mvds are all reliables really.
		case MULTICAST_ONE_R_SPECS:
		case MULTICAST_ONE_SPECS:
			{
				int pnum = -1;
				if (svprogfuncs)
				{
					edict_t *ent = PROG_TO_EDICT(svprogfuncs, pr_global_struct->msg_entity);
					pnum = NUM_FOR_EDICT(svprogfuncs, ent) - 1;
					if (pnum < 0 || pnum >= sv.allocated_client_slots)
					{
						Con_Printf(CON_WARNING"SV_Multicast: entity %i is not a client (\"%s\")\n", pnum+1, PR_GetString(svprogfuncs, ent->v->classname));
						return;
					}
				}
				else
				{
					Con_Printf(CON_WARNING"SV_Multicast: unsupported unicast\n");
					return;
				}
				msg = MVDWrite_Begin(dem_single, pnum, maxsize);
			}
			break;
		}
		callback(&demo.recorder, msg, ctx);
	}
#endif
}

//version does all the work now
void VARGS SV_Multicast (vec3_t origin, multicast_t to)
{
	SV_MulticastProtExt(origin, to, FULLDIMENSIONMASK, 0, 0);
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/
struct startsoundcontext_s
{
	float *org;
	float *vel;
	unsigned int ent;
	unsigned int chan;
	unsigned int sampleidx;
	unsigned int volume;
	float attenuation;
	float ratemul;
	unsigned int chflags;
	int timeofs;
};
static void SV_SoundMulticast(client_t *client, sizebuf_t *msg, void *vctx)
{
	int i;
	struct startsoundcontext_s *ctx = vctx;
	unsigned int field_mask = 0;

	if (ctx->ent >= client->max_net_ents)
		return;

	field_mask |= (ctx->chflags & CF_NETWORKED) << 8;
	if (ctx->volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= NQSND_VOLUME;
	if (ctx->attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= NQSND_ATTENUATION;
	if (ctx->ent >= 8192 || ctx->chan >= 8)
		field_mask |= NQSND_LARGEENTITY;
	if (ctx->sampleidx > 0xff && client->protocol != SCP_BJP3)
		field_mask |= NQSND_LARGESOUND;
	if (client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
	{
		if (ctx->ratemul && (ctx->ratemul != 1))
			field_mask |= FTESND_PITCHADJ;
		if (ctx->timeofs != 0)
			field_mask |= FTESND_TIMEOFS;
		if (ctx->vel)
			field_mask |= FTESND_VELOCITY;

		if (field_mask > 0xff)
			field_mask |= FTESND_MOREFLAGS;
	}
	if (client->protocol == SCP_DARKPLACES7)
	{	//dpp7 clients get slightly higher precision
		if (ctx->ratemul && (ctx->ratemul != 1))
		{
			field_mask |= DPSND_SPEEDUSHORT4000;
			field_mask &= ~FTESND_PITCHADJ;
		}
	}

	if (ISNQCLIENT(client) || ctx->chan >= 8 || ctx->ent >= 2048 || (field_mask & ~(NQSND_VOLUME|NQSND_ATTENUATION)))
	{
		//if any of the above conditions evaluates to true, then we can't use standard qw protocols
		if (ISNQCLIENT(client))
			MSG_WriteByte (msg, svc_sound);
		else
		{
			if (!(client->fteprotocolextensions & PEXT_SOUNDDBL) && !(client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
				return;
			MSG_WriteByte (msg, svcfte_soundextended);
		}
		MSG_WriteByte (msg, field_mask&0xff);
		if (field_mask & FTESND_MOREFLAGS)
			MSG_WriteUInt64 (msg, field_mask>>8);
		if (field_mask & NQSND_VOLUME)
			MSG_WriteByte (msg, bound(0, ctx->volume, 255));
		if (field_mask & NQSND_ATTENUATION)
			MSG_WriteByte (msg, bound(0, ctx->attenuation*64, 255));
		if (field_mask & FTESND_PITCHADJ)
			MSG_WriteByte (msg, bound(1, ctx->ratemul*100, 255));
		if (field_mask & FTESND_TIMEOFS)
			MSG_WriteShort (msg, bound(-32768, ctx->timeofs*1000, 32767));
		if (field_mask & FTESND_VELOCITY)
		{
			MSG_WriteShort (msg, bound(-32767, ctx->vel[0]*8, 32767));
			MSG_WriteShort (msg, bound(-32767, ctx->vel[1]*8, 32767));
			MSG_WriteShort (msg, bound(-32767, ctx->vel[2]*8, 32767));
		}
		if (field_mask & DPSND_SPEEDUSHORT4000)
			MSG_WriteShort (msg, bound(1, ctx->ratemul*4000, 65535));
		if (field_mask & NQSND_LARGEENTITY)
		{
			MSG_WriteEntity (msg, ctx->ent);
			MSG_WriteByte (msg, ctx->chan);
		}
		else
			MSG_WriteShort (msg, (ctx->ent<<3) | ctx->chan);
		if ((field_mask & NQSND_LARGESOUND) || client->protocol == SCP_BJP3)
			MSG_WriteShort (msg, ctx->sampleidx);
		else
			MSG_WriteByte (msg, ctx->sampleidx);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, ctx->org[i]);
	}
	else
	{
		unsigned short qwflags = (ctx->ent<<3) | ctx->chan;

		if (ctx->volume != DEFAULT_SOUND_PACKET_VOLUME)
			qwflags |= QWSND_VOLUME;
		if (ctx->attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
			qwflags |= QWSND_ATTENUATION;

		MSG_WriteByte (msg, svc_sound);
		MSG_WriteShort (msg, qwflags);
		if (qwflags & QWSND_VOLUME)
			MSG_WriteByte (msg, ctx->volume);
		if (qwflags & QWSND_ATTENUATION)
			MSG_WriteByte (msg, bound(0, ctx->attenuation*64, 255));
		MSG_WriteByte (msg, ctx->sampleidx&0xff);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, ctx->org[i]);
	}
}
void SV_StartSound (int ent, vec3_t origin, float *velocity, int seenmask, int channel, const char *sample, int volume, float attenuation, float ratemul, float timeofs, unsigned int chflags)
{
	qboolean	use_phs;
	qboolean	reliable = chflags & CF_SV_RELIABLE;
	struct startsoundcontext_s ctx;

	if (volume < 0 || volume > 255)
	{
		Con_Printf ("SV_StartSound: volume = %i", volume);
		return;
	}

	if (attenuation < 0 || attenuation >= 4)
		Con_DPrintf ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 255)
	{
		Con_Printf ("SV_StartSound: channel = %i", channel);
		return;
	}

	ctx.attenuation = attenuation;
	ctx.chan = channel;
	ctx.ent = ent;
	ctx.chflags = chflags;
	ctx.org = origin;
	ctx.vel = velocity;
	ctx.ratemul = ratemul;
	ctx.timeofs = timeofs;
	ctx.volume = volume;

	if (velocity && (!velocity[0] && !velocity[1] && !velocity[2]))
		ctx.vel = NULL;

// find precache number for sound
	if (!sample)
		ctx.sampleidx = 0;
	else if (!*sample)
		return;
	else
	{
		for (ctx.sampleidx=1 ; ctx.sampleidx<MAX_PRECACHE_SOUNDS
			&& sv.strings.sound_precache[ctx.sampleidx] ; ctx.sampleidx++)
			if (!strcmp(sample, sv.strings.sound_precache[ctx.sampleidx]))
				break;

		if ( ctx.sampleidx >= MAX_PRECACHE_SOUNDS || !sv.strings.sound_precache[ctx.sampleidx] )
		{
			if (ctx.sampleidx < MAX_PRECACHE_SOUNDS)
			{
				Con_Printf("WARNING: SV_StartSound: sound %s not precached\n", sample);
				//late precache it. use multicast to ensure that its sent NOW (and to all). normal reliables would mean it would arrive after the svc_sound
				sv.strings.sound_precache[ctx.sampleidx] = PR_AddString(svprogfuncs, sample, 0, false);
				Con_DPrintf("Delayed sound precache: %s\n", sample);
				MSG_WriteByte(&sv.multicast, svcfte_precache);
				MSG_WriteShort(&sv.multicast, ctx.sampleidx+PC_SOUND);
				MSG_WriteString(&sv.multicast, sample);
		#ifdef NQPROT
				MSG_WriteByte(&sv.nqmulticast, svcdp_precache);
				MSG_WriteShort(&sv.nqmulticast, ctx.sampleidx+PC_SOUND);
				MSG_WriteString(&sv.nqmulticast, sample);
		#endif
				SV_MulticastProtExt(NULL, MULTICAST_ALL_R, FULLDIMENSIONMASK, PEXT_CSQC, 0);

				reliable = true;	//try to make sure it doesn't arrive before the precache!
			}
			else
			{
				Con_DPrintf ("SV_StartSound: %s not precached\n", sample);
				return;
			}
		}
	}

	if (reliable || !sv_phs.value || !attenuation)	// no PHS flag
		use_phs = false;
	else
		use_phs = attenuation!=0;

	if (chflags & CF_SV_UNICAST)
	{
		SV_MulticastCB(origin, (reliable||sv_reliable_sound.ival) ? MULTICAST_ONE_R_SPECS : MULTICAST_ONE_SPECS, reliable?NULL:"rsnd", seenmask, SV_SoundMulticast, &ctx);
	}
	else
	{
		if (use_phs)
			SV_MulticastCB(origin, (reliable||sv_reliable_sound.ival) ? MULTICAST_PHS_R : MULTICAST_PHS, reliable?NULL:"rsnd", seenmask, SV_SoundMulticast, &ctx);
		else
			SV_MulticastCB(origin, (reliable||sv_reliable_sound.ival) ? MULTICAST_ALL_R : MULTICAST_ALL, reliable?NULL:"rsnd", seenmask, SV_SoundMulticast, &ctx);
	}
}

void QDECL SVQ1_StartSound (float *origin, wedict_t *wentity, int channel, const char *sample, int volume, float attenuation, float pitchadj, float timeofs, unsigned int chflags)
{
	edict_t *entity = (edict_t*)wentity;
	int i, solid;
	vec3_t originbuf, velocity={0,0,0};
	if (!origin)
	{
		origin = originbuf;
		solid = entity->v->solid;
		if (solid == SOLID_BSP || solid == SOLID_BSPTRIGGER)
		{
			for (i=0 ; i<3 ; i++)
				origin[i] = entity->v->origin[i]+0.5*(entity->v->mins[i]+entity->v->maxs[i]);

			//add the reliable flag for bsp objects.
			//these sounds are often looped, and if the start is in the phs and the end isn't/gets dropped, then you end up with an annoying infinitely looping sample.
			//making them all reliable avoids packetloss and phs issues.
			//this applies only to pushers. you won't get extra latency on player actions because of this.
			//be warned that it does mean you might be able to hear people triggering stuff on the other side of the map however.
			chflags |= CF_SV_RELIABLE;
		}
		else if (progstype == PROG_QW)
		{	//quakeworld puts the sound ONLY at the entity's actual origin. this is annoying and stupid. I'm not really sure what to do here. it seems wrong.
			VectorCopy (entity->v->origin, origin);
		}
		else
		{	//nq (and presumably h2) always put the sound in the middle of the ent's bbox. this is needed to avoid triggers breaking (like trigger_secret).
			for (i=0 ; i<3 ; i++)
				origin[i] = entity->v->origin[i]+0.5*(entity->v->mins[i]+entity->v->maxs[i]);
		}

		if (chflags & CF_SV_SENDVELOCITY)
			VectorCopy(entity->v->velocity, velocity);
	}

	SV_StartSound(NUM_FOR_EDICT(svprogfuncs, entity), origin, velocity, entity->xv->dimension_seen, channel, sample, volume, attenuation, pitchadj, timeofs, chflags);
}



void SV_SendLightstyle(client_t *cl, sizebuf_t *forcemsg, int style, qboolean initial)
{
	sizebuf_t *msg;
	const char *stylestring = (style < sv.maxlightstyles)?sv.lightstyles[style].str:NULL;
	float *stylecolor = (style < sv.maxlightstyles)?sv.lightstyles[style].colours:vec3_origin;
	int flags = 0;
	int sz;

	//don't crash old clients unless there's a good reason to do so.
	//new clients are expected to reinitialise their styles to empty on map changes.
	if (style >= MAX_STANDARDLIGHTSTYLES && initial && !stylestring)
		return;

	if (style > 255)
		flags |= 0x40;
	if (stylestring && (stylecolor[0]!=1||stylecolor[1]!=1||stylecolor[2]!=1))
	{
		if (stylecolor[0]!=0)
		{
			flags |= 1;
			if (stylecolor[0]!=1)
				flags |= 0x80|7;
		}
		if (stylecolor[1]!=0)
		{
			flags |= 2;
			if (stylecolor[1]!=1)
				flags |= 0x80|7;
		}
		if (stylecolor[2]!=0)
		{
			flags |= 4;
			if (stylecolor[2]!=1)
				flags |= 0x80|7;
		}
	}
	else
		flags |= 7;
	//flags |= 0x08;
	//flags |= 0x10;
	//flags |= 0x20;

	if (!(cl->fteprotocolextensions & PEXT_LIGHTSTYLECOL))
	{	//if they don't support it then just drop the extra colours, so long as it still makes sense.
		if ((flags & ~0x87u) && (ISNQCLIENT(cl) && !ISDPCLIENT(cl) && cl->fteprotocolextensions2))
		{
			char *text = va("//ls %i \"%s\" %g %g %g\n", style, sv.lightstyles[style].str, sv.lightstyles[style].colours[0], sv.lightstyles[style].colours[1], sv.lightstyles[style].colours[2]);
			if (forcemsg)
				msg = forcemsg;
			else
				msg = ClientReliable_StartWrite(cl, 2+strlen(text));
			MSG_WriteByte (msg, svc_stufftext);
			MSG_WriteString (msg, text);
			if (!forcemsg)
				ClientReliable_FinishWrite(cl);
			return;	//erk, can't handle this!
		}
		if (style >= ((cl->fteprotocolextensions2||ISDPCLIENT(cl))?255:64))
			return; //client probably doesn't support this lightstyle.
		flags = 7;	//force vanilla protocol as fallback.
	}

	if (forcemsg)
		msg = forcemsg;
	else
	{
		sz = 2;
		if (flags != 7)
			sz+=1;
		if (flags & 0x40)
			sz+=1;
		if (flags & 0x80u)
			sz+=3*2;	//rough overestimate
		sz += (stylestring?strlen(stylestring):0) + 1;
		msg = ClientReliable_StartWrite(cl, sz);
	}

	MSG_WriteByte(msg, (flags != 7)?svcfte_lightstylecol:svc_lightstyle);
	MSG_WriteByte (msg, style&0xffu);
	if (flags != 7)
		MSG_WriteByte(msg, flags);
	if (flags & 0x40)	//16bit style indexes
		MSG_WriteByte (msg, style>>8);
	if (flags & 0x80u)
	{	//rich style tints
		if (flags & 1)
			MSG_WriteShort (msg, bound(-0x7fff, stylecolor[0]*1024, 0x7fff));
		if (flags & 2)
			MSG_WriteShort (msg, bound(-0x7fff, stylecolor[1]*1024, 0x7fff));
		if (flags & 4)
			MSG_WriteShort (msg, bound(-0x7fff, stylecolor[2]*1024, 0x7fff));
	}
	MSG_WriteString (msg, stylestring);

	if (!forcemsg)
		ClientReliable_FinishWrite(cl);
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

int		sv_nailmodel, sv_supernailmodel, sv_playermodel;

void SV_FindModelNumbers (void)
{
	int		i;

	sv_nailmodel = -1;
	sv_supernailmodel = -1;
	sv_playermodel = -1;

	for (i=0 ; i<MAX_PRECACHE_MODELS ; i++)
	{
		if (!sv.strings.model_precache[i])
			break;
		if (!strcmp(sv.strings.model_precache[i],"progs/spike.mdl") && sv.multicast.prim.coordtype == COORDTYPE_FIXED_13_3)
			sv_nailmodel = i;
		if (!strcmp(sv.strings.model_precache[i],"progs/s_spike.mdl") && sv.multicast.prim.coordtype == COORDTYPE_FIXED_13_3)
			sv_supernailmodel = i;
		if (!strcmp(sv.strings.model_precache[i],"progs/player.mdl"))
			sv_playermodel = i;
	}
}

void SV_SendFixAngle(client_t *client, sizebuf_t *msg, int fixtype, qboolean roll)
{
	unsigned i;
	client_t *controller = client->controller?client->controller:client;
	edict_t *ent = client->edict;
	pvec_t *ang;
	if (!ent || ISQ2CLIENT(client))
		return;
	ang = ent->v->fixangle?ent->v->angles:ent->v->v_angle;	//angles is just WEIRD for mdls, but then quake sucks.
	if (ent->v->movetype == MOVETYPE_6DOF)
		roll = true;

	if (fixtype == FIXANGLE_AUTO)
	{
		if (client->lockanglesseq<controller->netchan.incoming_acknowledged && controller->delta_sequence != -1 && !client->viewent && !sv_nqplayerphysics.ival)
			fixtype = FIXANGLE_DELTA;
		else
			fixtype = FIXANGLE_FIXED;
	}
	else
		roll = true;

	if (controller->fteprotocolextensions2 & PEXT2_VRINPUTS)
	{
		if (fixtype == FIXANGLE_DELTA)
		{
			//fiddle with the base angle, server will see future moves with that change already applied.
			vec3_t diff;
			for (i = 0; i < 3; i++)
				diff[i] = ANGLE2SHORT(ang[i]) - client->lastcmd.angles[i];
			if (!roll)
				diff[2] = 0;
			VectorAdd(client->baseangles, diff, client->baseangles);
		}
		else
		{
			client->baseangles[0] = ANGLE2SHORT(ang[0]);
			client->baseangles[1] = ANGLE2SHORT(ang[1]);
			client->baseangles[2] = ANGLE2SHORT(ang[2]);
			client->baseanglelock++;
		}
	}
	else
	{
		if (fixtype == FIXANGLE_DELTA && !(controller->fteprotocolextensions2 & PEXT2_SETANGLEDELTA))
			fixtype = FIXANGLE_FIXED;	//sorry, can't do it.

		if (client->lockanglesseq>=controller->netchan.incoming_acknowledged && controller->netchan.message.cursize < controller->netchan.message.maxsize/2)
			msg = NULL;	//try to keep them vaugely reliable, where feasable.
		if (!msg)
			msg = ClientReliable_StartWrite(client, 10);
		else if (client->seat)
		{
			MSG_WriteByte(msg, svcfte_choosesplitclient);
			MSG_WriteByte(msg, client->seat);
		}
		if (fixtype == FIXANGLE_DELTA && (controller->fteprotocolextensions2 & PEXT2_SETANGLEDELTA))
		{
			MSG_WriteByte (msg, svcfte_setangledelta);
			for (i=0 ; i < 3 ; i++)
			{
				int newa = ang[i] - SHORT2ANGLE(client->lastcmd.angles[i]);
				MSG_WriteAngle16 (msg, newa);
				client->lastcmd.angles[i] = ANGLE2SHORT(ang[i]);
			}
		}
		else
		{
			MSG_WriteByte (msg, svc_setangle);
			if (client->ezprotocolextensions1 & EZPEXT1_SETANGLEREASON)
				MSG_WriteByte (msg, (fixtype == FIXANGLE_DELTA)?2:0);	//shitty backwards incompatible protocol extension that breaks from writebytes.
			for (i=0 ; i < 3 ; i++)
				MSG_WriteAngle (msg, (i==2&&!roll)?0:ang[i]);
		}
	}
	ClientReliable_FinishWrite(client);
	client->lockanglesseq = controller->netchan.outgoing_sequence+1;	//so that spammed fixangles use absolute values, locking the camera in place.
}

void SV_WriteEntityDataToMessage (client_t *client, sizebuf_t *msg, int pnum)
{
	edict_t	*other;
	edict_t	*ent;
	int i;

	ent = client->edict;

	if (!ent)
		return;

	// send a damage message if the player got hit this frame
	if (ent->v->dmg_take || ent->v->dmg_save)
	{
		other = PROG_TO_EDICT(svprogfuncs, ent->v->dmg_inflictor);
		if (pnum)
		{
			MSG_WriteByte(msg, svcfte_choosesplitclient);
			MSG_WriteByte(msg, pnum);
		}
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, bound(0, ent->v->dmg_save, 255));
		MSG_WriteByte (msg, bound(0, ent->v->dmg_take, 255));
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, other->v->origin[i] + 0.5*(other->v->mins[i] + other->v->maxs[i]));

		//FIXME: flood to spectators.

		ent->v->dmg_take = 0;
		ent->v->dmg_save = 0;
	}

	// a fixangle might get lost in a dropped packet.  Oh well.
	if (client->spectator && ISNQCLIENT(client) && client->spec_track > 0)
	{
		edict_t *ed = EDICT_NUM_UB(svprogfuncs, client->spec_track);
		MSG_WriteByte(msg, svc_setangle);
		MSG_WriteAngle(msg, ed->v->v_angle[0]);
		MSG_WriteAngle(msg, ed->v->v_angle[1]);
		MSG_WriteAngle(msg, ed->v->v_angle[2]);
		VectorCopy(ed->v->origin, client->edict->v->origin);
	}
	else if (ent->v->fixangle)
	{
		SV_SendFixAngle(client, msg, ent->v->fixangle, true);
		ent->v->fixangle = FIXANGLE_NO;
	}
}

/*sends the a centerprint string directly to the client*/
void SV_WriteCenterPrint(client_t *cl, char *s)
{
	if (cl->controller)
	{	//this is a slave client.
		//find the right number and send.
		int pnum = 0;
		client_t *sp;
		for (sp = cl->controller; sp; sp = sp->controlled)
		{
			if (sp == cl)
				break;
			pnum++;
		}
		cl = cl->controller;

		ClientReliableWrite_Begin (cl, svcfte_choosesplitclient, 4 + strlen(s));
		ClientReliableWrite_Byte (cl, pnum);
		ClientReliableWrite_Byte (cl, svc_centerprint);
	}
	else
		ClientReliableWrite_Begin (cl, svc_centerprint, 2 + strlen(s));
	ClientReliableWrite_String (cl, s);

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		sizebuf_t *msg = MVDWrite_Begin (dem_single, cl - svs.clients, 2 + strlen(s));
		MSG_WriteByte (msg, svc_centerprint);
		MSG_WriteString (msg, s);
	}
#endif
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg)
{
#ifdef NQPROT
	int		i;
	int bits, items;
	edict_t	*ent;
	qboolean nqjunk = true;
	int weaponmodelindex = 0;
#endif
	client_t *split;
	int pnum=0;

	// send the chokecount for r_netgraph
	if (ISQWCLIENT(client))
	if (client->chokecount)
	{
		MSG_WriteByte (msg, svc_chokecount);
		MSG_WriteByte (msg, bound(0, client->chokecount, 255));
		client->chokecount = 0;
	}

	for (split = client; split; split=split->controlled, pnum++)
	{
		SV_WriteEntityDataToMessage(split, msg, pnum);

		if (split->prompt.active)
			SV_Prompt_Resend(split);

		if (split->centerprintstring && ! client->num_backbuf)
		{
			SV_WriteCenterPrint(split, split->centerprintstring);
			Z_Free(split->centerprintstring);
			split->centerprintstring = NULL;
		}
	}
/*
	MSG_WriteByte (msg, svc_time);
	MSG_WriteFloat(msg, sv.physicstime);
	client->nextservertimeupdate = sv.physicstime;
*/

#ifdef HLSERVER
	if (svs.gametype == GT_HALFLIFE)
		return;
#endif

#ifdef NQPROT
	ent = client->edict;
	if (client->spectator && client->spec_track)
		ent = EDICT_NUM_UB(svprogfuncs, client->spec_track);
	if (progstype != PROG_QW)
	{
		if (ISQWCLIENT(client) && !(client->fteprotocolextensions2 & PEXT2_PREDINFO))
		{
			//quakeworld clients drop the punch angle themselves.
			while (ent->xv->punchangle[0] < -3)
			{
				ent->xv->punchangle[0] += 4;
				MSG_WriteByte (msg, svc_bigkick);
			}
			while (ent->xv->punchangle[0] < -1)
			{
				ent->xv->punchangle[0] += 2;
				MSG_WriteByte (msg, svc_smallkick);
			}
			ent->xv->punchangle[1] = 0;
			ent->xv->punchangle[2] = 0;
		}
	}

	if (ISQWCLIENT(client))
		return;

	if (!(client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
	{
		MSG_WriteByte (msg, svc_time);
		MSG_WriteFloat(msg, sv.world.physicstime);

		if (client->fteprotocolextensions2 & PEXT2_PREDINFO)
			MSG_WriteShort(msg, client->last_sequence);
		else if (client->qex && client->last_sequence)
		{
			MSG_WriteByte(msg, svcqex_seq);
			MSG_WriteULEB128(msg, client->last_sequence);
		}

//		Con_Printf("%f\n", sv.world.physicstime);
	}

	//predinfo extension reworks stats, making svc_clientdata redundant.
	if (client->fteprotocolextensions2 & PEXT2_PREDINFO)
		return;

	if (client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7)
		nqjunk = false;
	else
		nqjunk = true;

	bits = 0;

	if (ent->v->view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;

	if (ent->xv->idealpitch)
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
	if (sv.haveitems2)
		items = (int)ent->v->items | ((int)ent->xv->items2 << 23);
	else
		items = (int)ent->v->items | ((int)pr_global_struct->serverflags << 28);


	if (nqjunk)
		bits |= SU_ITEMS;

	if ( (int)ent->v->flags & FL_ONGROUND)
		bits |= SU_ONGROUND;

	if ( ent->v->waterlevel >= 2)
		bits |= SU_INWATER;

	for (i=0 ; i<3 ; i++)
	{
		if (ent->xv->punchangle[i])
			bits |= (SU_PUNCH1<<i);
//		if ((client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7) && ent->xv->punchvector[i])
//			bits |= (DPSU_PUNCHVEC1<<i);
		if (ent->v->velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}

	if (nqjunk)
	{
		nqjunk = true;

		if (ent->v->weaponframe)
			bits |= SU_WEAPONFRAME;

		if (ent->v->armorvalue)
			bits |= SU_ARMOR;

		weaponmodelindex = SV_ModelIndex(PR_GetString(svprogfuncs, ent->v->weaponmodel));

		if (weaponmodelindex)
			bits |= SU_WEAPONMODEL;

		if (client->protocol == SCP_FITZ666)
		{
			if (weaponmodelindex & 0xff00)
				bits |= FITZSU_WEAPONMODEL2;
			if ((int)ent->v->armorvalue & 0xff00)
				bits |= FITZSU_ARMOR2;
			if ((int)ent->v->currentammo & 0xff00)
				bits |= FITZSU_AMMO2;
			if ((int)ent->v->ammo_shells & 0xff00)
				bits |= FITZSU_SHELLS2;
			if ((int)ent->v->ammo_nails & 0xff00)
				bits |= FITZSU_NAILS2;
			if ((int)ent->v->ammo_rockets & 0xff00)
				bits |= FITZSU_ROCKETS2;
			if ((int)ent->v->ammo_cells & 0xff00)
				bits |= FITZSU_CELLS2;
			if ((int)ent->v->weaponframe & 0xff00)
				bits |= FITZSU_WEAPONFRAME2;
			if (ent->xv->alpha && ent->xv->alpha < 1)
				bits |= FITZSU_WEAPONALPHA;

			if (client->qex)
			{
				if (ent->v->flags)
					bits |= QEX_SU_ENTFLAGS;
				if (bits & (SU_VELOCITY1|SU_VELOCITY2|SU_VELOCITY3))
					bits |= QEX_SU_FLOATCOORDS;
			}
		}
	}

	if (bits >= (1u<<16))
		bits |= SU_EXTEND1;
	if (bits >= (1u<<24))
		bits |= SU_EXTEND2;
	if (bits >= ((quint64_t)1u<<32))
		bits |= SU_EXTEND3;

// send the data

	MSG_WriteByte (msg, svcnq_clientdata);
	MSG_WriteShort (msg, bits & 0xffff);

	if (bits & SU_EXTEND1)
		MSG_WriteByte(msg, (bits>>16)&0xff);
	if (bits & SU_EXTEND2)
		MSG_WriteByte(msg, bits>>24);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v->view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ent->xv->idealpitch);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
		{
			if (client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7)
				MSG_WriteAngle16 (msg, ent->xv->punchangle[i]);
			else
				MSG_WriteChar (msg, ent->xv->punchangle[i]);
		}
//		if ((client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7) && (bits & (DPSU_PUNCHVEC1<<i)))
//			Msg_WriteCoord(msg, ent->xv->punchvector);
		if (bits & (SU_VELOCITY1<<i))
		{
			if (client->qex && (bits & QEX_SU_ENTFLAGS))
				MSG_WriteFloat(msg, ent->v->velocity[i]);
			else if (client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7)
				MSG_WriteFloat(msg, ent->v->velocity[i]);
			else
				MSG_WriteChar (msg, bound(-128, ent->v->velocity[i]/16, 127));
		}
	}

	if (bits & SU_ITEMS)
		MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v->weaponframe);
	if (bits & SU_ARMOR)
	{
		if (ent->v->armorvalue < 0)
			MSG_WriteByte (msg, 0);
		else if (ent->v->armorvalue>255 && !(bits & FITZSU_ARMOR2))
			MSG_WriteByte (msg, 255);
		else
			MSG_WriteByte (msg, (int)ent->v->armorvalue&0xff);
	}
	if (bits & SU_WEAPONMODEL)
	{
		if (client->protocol == SCP_BJP3)
			MSG_WriteShort (msg, weaponmodelindex&0xffff);
		else
			MSG_WriteByte (msg, weaponmodelindex&0xff);
	}

	if (nqjunk)
	{
		if (client->spectator && !client->spec_track)
			MSG_WriteShort (msg, 1000);
		else
			MSG_WriteShort (msg, ent->v->health);
		if (client->protocol == SCP_FITZ666)
		{
			MSG_WriteByte (msg, (int)ent->v->currentammo & 0xff);
			MSG_WriteByte (msg, (int)ent->v->ammo_shells & 0xff);
			MSG_WriteByte (msg, (int)ent->v->ammo_nails & 0xff);
			MSG_WriteByte (msg, (int)ent->v->ammo_rockets & 0xff);
			MSG_WriteByte (msg, (int)ent->v->ammo_cells & 0xff);
		}
		else
		{
			MSG_WriteByte (msg, min(ent->v->currentammo, 255));
			MSG_WriteByte (msg, min(ent->v->ammo_shells, 255));
			MSG_WriteByte (msg, min(ent->v->ammo_nails, 255));
			MSG_WriteByte (msg, min(ent->v->ammo_rockets, 255));
			MSG_WriteByte (msg, min(ent->v->ammo_cells, 255));
		}

		if (standard_quake)
		{
			MSG_WriteByte (msg, (unsigned int)ent->v->weapon & 0xff);
		}
		else
		{
			for(i=0;i<32;i++)
			{
				if ( ((int)ent->v->weapon) & (1<<i) )
				{
					MSG_WriteByte (msg, i);
					break;
				}
			}
		}
	}

	if (client->protocol == SCP_FITZ666)
	{
		if (bits & FITZSU_WEAPONMODEL2)	MSG_WriteByte (msg, weaponmodelindex >> 8);
		if (bits & FITZSU_ARMOR2)		MSG_WriteByte (msg, (int)ent->v->armorvalue >> 8);
		if (bits & FITZSU_AMMO2)		MSG_WriteByte (msg, (int)ent->v->currentammo >> 8);
		if (bits & FITZSU_SHELLS2)		MSG_WriteByte (msg, (int)ent->v->ammo_shells >> 8);
		if (bits & FITZSU_NAILS2)		MSG_WriteByte (msg, (int)ent->v->ammo_nails >> 8);
		if (bits & FITZSU_ROCKETS2)		MSG_WriteByte (msg, (int)ent->v->ammo_rockets >> 8);
		if (bits & FITZSU_CELLS2)		MSG_WriteByte (msg, (int)ent->v->ammo_cells >> 8);
		if (bits & FITZSU_WEAPONFRAME2)	MSG_WriteByte (msg, (int)ent->v->weaponframe >> 8);
		if (bits & FITZSU_WEAPONALPHA)	MSG_WriteByte (msg, ent->xv->alpha*255);

		if (client->qex)
		{
			if (bits & QEX_SU_ENTFLAGS)	MSG_WriteULEB128 (msg, ent->v->flags);
		}
	}
#endif
}

typedef struct {
	int type;	//negative means a global.
	char name[64];
	union {
		evalc_t c;
		eval_t *g;	//just store a pointer to it.
	} eval;
	int statnum;
} qcstat_t;
static qcstat_t qcstats[MAX_CL_STATS];
static unsigned int numqcstats;
static unsigned int highestqcstat;
static void SV_QCStatEval(int type, const char *name, evalc_t *field, eval_t *global, int statnum)
{
	int i;
	if (numqcstats == sizeof(qcstats)/sizeof(qcstats[0]))
	{
		Con_Printf("Too many stat types\n");
		return;
	}

	for (i = 0; i < numqcstats; i++)
	{
		//strings use a different namespace.
		if (qcstats[i].statnum == statnum && ((qcstats[i].type == ev_string||qcstats[i].type == -ev_string) == (type == ev_string||type == -ev_string)))
			break;
	}
	if (i == numqcstats)
	{
		if (i == sizeof(qcstats)/sizeof(qcstats[0]))
		{
			Con_Printf("Too many stats specified for csqc\n");
			return;
		}
		numqcstats++;
	}

	qcstats[i].type = type;
	qcstats[i].statnum = statnum;
	Q_strncpyz(qcstats[i].name, name, sizeof(qcstats[i].name));
	memset(&qcstats[i].eval, 0, sizeof(qcstats[i].eval));
	if (type <= 0)
		qcstats[i].eval.g = global;
	else if (field)
		memcpy(&qcstats[i].eval.c, field, sizeof(evalc_t));
	else
		qcstats[i].type = ev_void;
}

void SV_QCStatGlobal(int type, const char *globalname, int statnum)
{
	eval_t *glob;

	if (type < 0)
		return;

	glob = svprogfuncs->FindGlobal(svprogfuncs, globalname, PR_ANY, NULL);
	if (!glob)
	{
		Con_Printf("couldn't find named global for csqc stat (%s)\n", globalname);
		return;
	}
	SV_QCStatEval(-type, globalname, NULL, glob, statnum);
}

void SV_QCStatPtr(int type, void *ptr, int statnum)
{
	SV_QCStatEval(-type, "", NULL, ptr, statnum);
}

void SV_QCStatName(int type, char *name, int statnum)
{
	evalc_t cache;
	if (type < 0)
		return;

	memset(&cache, 0, sizeof(cache));
	if (!svprogfuncs->GetEdictFieldValue(svprogfuncs, NULL, name, 0, &cache))
		return;

	SV_QCStatEval(type, name, &cache, NULL, statnum);
}

void SV_QCStatFieldIdx(int type, unsigned int fieldindex, int statnum)
{
	evalc_t cache;
	const char *name;
	etype_t ftype;

	if (type < 0)
		return;

	if (!svprogfuncs->QueryField(svprogfuncs, fieldindex, &ftype, &name, &cache))
	{
		Con_Printf("invalid field for csqc stat\n");
		return;
	}
	SV_QCStatEval(type, name, &cache, NULL, statnum);
}

void SV_ClearQCStats(void)
{
	numqcstats = 0;
	highestqcstat = MAX_QW_STATS;
}

extern cvar_t dpcompat_stats;
void SV_UpdateQCStats(edict_t	*ent, int *statsi, char const** statss, float *statsf)
{
	const char *s;
	int i;
	int t;

	for (i = 0; i < numqcstats; i++)
	{
		eval_t *eval;
		t = qcstats[i].type;
		if (t <= 0)
		{
			t = -t;
			eval = qcstats[i].eval.g;
		}
		else
		{
			eval = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, qcstats[i].name, 0, &qcstats[i].eval.c);
		}
		if (!eval)
			continue;

		switch(t)
		{
		case ev_float:
			statsf[qcstats[i].statnum] = eval->_float;
			break;
		case ev_double:
			statsf[qcstats[i].statnum] = eval->_double;	//FIXME: precision loss.
			break;
		case ev_vector:	//split over 3 stats.
			statsf[qcstats[i].statnum+0] = eval->_vector[0];
			statsf[qcstats[i].statnum+1] = eval->_vector[1];
			statsf[qcstats[i].statnum+2] = eval->_vector[2];
			break;
		case ev_integer:
		case ev_uint:
			statsi[qcstats[i].statnum] = eval->_int;
			break;
		case ev_int64:
		case ev_uint64:	//split over 2 stats.
			statsi[qcstats[i].statnum] = eval->u64&0xffffffff;
			statsi[qcstats[i].statnum+1] = eval->u64>>32;
			break;
		case ev_entity:
			statsi[qcstats[i].statnum] = NUM_FOR_EDICT(svprogfuncs, PROG_TO_EDICT(svprogfuncs, eval->edict));
			break;
		case ev_string:
			s = PR_GetString(svprogfuncs, eval->string);
			statss[qcstats[i].statnum] = s;
//			statsi[qcstats[i].statnum+0] = LittleLong(((int*)s)[0]);	//so the network is sent out correctly as a string.
//			statsi[qcstats[i].statnum+1] = LittleLong(((int*)s)[1]);
//			statsi[qcstats[i].statnum+2] = LittleLong(((int*)s)[2]);
//			statsi[qcstats[i].statnum+3] = LittleLong(((int*)s)[3]);
			break;
		}
	}
}

/*this function calculates the current stat values for the given client*/
static unsigned int SV_CalcClientStats(client_t *client, int statsi[MAX_CL_STATS], float statsf[MAX_CL_STATS], const char **statss)
{
	unsigned int m = highestqcstat;
	edict_t *ent;
	ent = client->edict;
	memset (statsi, 0, sizeof(int)*MAX_CL_STATS);
	memset (statsf, 0, sizeof(float)*MAX_CL_STATS);
	memset ((void*)statss, 0, sizeof(char const*)*MAX_CL_STATS);	//cast needed to get msvc to behave.

	// if we are a spectator and we are tracking a player, we get his stats
	// so our status bar reflects his
	if (client->spectator && client->spec_track > 0)
		ent = EDICT_NUM_UB(svprogfuncs, client->spec_track);

#ifdef HLSERVER
	if (svs.gametype == GT_HALFLIFE)
	{
		SVHL_BuildStats(client, statsi, statsf, statss);
	}
	else
#endif
	{
#ifdef QUAKESTATS
		if (client->spectator && !client->spec_track && ISNQCLIENT(client))
		{
			statsf[STAT_HEALTH] = 1000;
			statsf[STAT_ARMOR] = 1000;
			statsf[STAT_AMMO] = 1000;
		}
		else
		{
			statsf[STAT_HEALTH] = ent->v->health;	//sorry, but mneh
			statsi[STAT_WEAPONMODELI] = SV_ModelIndex(PR_GetString(svprogfuncs, ent->v->weaponmodel));
			if ((unsigned)statsi[STAT_WEAPONMODELI] >= client->maxmodels)
				statsi[STAT_WEAPONMODELI] = 0;	//play it safe, try not to crash unsuspecting clients
			statsf[STAT_AMMO] = ent->v->currentammo;
			statsf[STAT_ARMOR] = ent->v->armorvalue;
			statsf[STAT_SHELLS] = ent->v->ammo_shells;
			statsf[STAT_NAILS] = ent->v->ammo_nails;
			statsf[STAT_ROCKETS] = ent->v->ammo_rockets;
			statsf[STAT_CELLS] = ent->v->ammo_cells;
			statsf[STAT_ACTIVEWEAPON] = ent->v->weapon;
			if ((client->csqcactive && !(client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)) || client->protocol != SCP_QUAKEWORLD || (client->fteprotocolextensions2 & PEXT2_PREDINFO))
	//		if ((client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) || client->protocol != SCP_QUAKEWORLD)
				statsf[STAT_WEAPONFRAME] = ent->v->weaponframe;		//weapon frame is sent differently with classic quakeworld protocols.

			// stuff the sigil bits into the high bits of items for sbar
			if (sv.haveitems2)
				statsi[STAT_ITEMS] = (int)ent->v->items | ((int)ent->xv->items2 << 23);
			else
				statsi[STAT_ITEMS] = (int)ent->v->items | ((int)pr_global_struct->serverflags << 28);
		}

		statsf[STAT_VIEWHEIGHT] = ent->v->view_ofs[2];
		statsf[STAT_IDEALPITCH] = ent->xv->idealpitch;

		statsf[STAT_PUNCHANGLE_X] = ent->xv->punchangle[0];
		statsf[STAT_PUNCHANGLE_Y] = ent->xv->punchangle[1];
		statsf[STAT_PUNCHANGLE_Z] = ent->xv->punchangle[2];

//		statsf[STAT_PUNCHORIGIN_X] = ent->xv->punchvector[0];
//		statsf[STAT_PUNCHORIGIN_Y] = ent->xv->punchvector[1];
//		statsf[STAT_PUNCHORIGIN_Z] = ent->xv->punchvector[2];

	#ifdef PEXT_VIEW2
		if (ent->xv->view2)
			statsi[STAT_VIEW2] = NUM_FOR_EDICT(svprogfuncs, PROG_TO_EDICT(svprogfuncs, ent->xv->view2));
		else
			statsi[STAT_VIEW2] = 0;
	#endif

		if (!ent->xv->viewzoom)
			statsf[STAT_VIEWZOOM] = STAT_VIEWZOOM_SCALE;
		else
			statsf[STAT_VIEWZOOM] = max(1,ent->xv->viewzoom*STAT_VIEWZOOM_SCALE);
#endif

#ifdef NQPROT
		if (client->protocol == SCP_DARKPLACES7 || (client->fteprotocolextensions2 & PEXT2_PREDINFO))
		{
			extern cvar_t sv_stepheight;
			float	*statsfi;
			if (client->fteprotocolextensions2 & PEXT2_PREDINFO)
				statsfi = statsf;
			else
			{
				statsfi = (float*)statsi;	/*dp requires a union of ints and floats, which is rather hideous...*/

				statsfi[STAT_FRAGLIMIT] = fraglimit.value;
				statsfi[STAT_TIMELIMIT] = timelimit.value;
			}
//commented out things are basically for xonotic's use. they're not implemented by the server's movement stuff, not in dp, not in fte.
//that's not to say the client shouldn't support them (when mods have hacked up velocity stuff and no willingness to implement the same thing in csqc too).
//			statsfi[STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR]	= 0;
//			statsfi[STAT_MOVEVARS_AIRCONTROL_PENALTY]			= 0;
//			statsfi[STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW]			= 0;
//			statsfi[STAT_MOVEVARS_AIRSTRAFEACCEL_QW]			= 0;
//			statsfi[STAT_MOVEVARS_AIRCONTROL_POWER]				= 2;
			statsi [STAT_MOVEFLAGS]								= MOVEFLAG_VALID|MOVEFLAG_QWCOMPAT;
//			statsfi[STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL]	= 0;
//			statsfi[STAT_MOVEVARS_WARSOWBUNNY_ACCEL]			= 0;
//			statsfi[STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED]			= 0;
//			statsfi[STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL]		= 0;
//			statsfi[STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO]	= 0;
//			statsfi[STAT_MOVEVARS_AIRSTOPACCELERATE]			= 0;
//			statsfi[STAT_MOVEVARS_AIRSTRAFEACCELERATE]			= 0;
//			statsfi[STAT_MOVEVARS_MAXAIRSTRAFESPEED]			= 0;
//			statsfi[STAT_MOVEVARS_AIRCONTROL]					= 0;
//			statsfi[STAT_MOVEVARS_WALLFRICTION]					= 0;
			statsfi[STAT_MOVEVARS_FRICTION]						= sv_friction.value;
			statsfi[STAT_MOVEVARS_WATERFRICTION]				= sv_waterfriction.value;
			statsfi[STAT_MOVEVARS_TICRATE]						= sv_mintic.value?sv_mintic.value:(1.0/72);
			statsfi[STAT_MOVEVARS_TIMESCALE]					= sv_gamespeed.value;
			statsfi[STAT_MOVEVARS_GRAVITY]						= sv_gravity.value;
			statsfi[STAT_MOVEVARS_STOPSPEED]					= sv_stopspeed.value;
			statsfi[STAT_MOVEVARS_MAXSPEED]						= client->maxspeed;
			statsfi[STAT_MOVEVARS_SPECTATORMAXSPEED]			= sv_spectatormaxspeed.value;
			statsfi[STAT_MOVEVARS_ACCELERATE]					= sv_accelerate.value;
			statsfi[STAT_MOVEVARS_AIRACCELERATE]				= sv_airaccelerate.value;
			statsfi[STAT_MOVEVARS_WATERACCELERATE]				= sv_wateraccelerate.value;
			statsfi[STAT_MOVEVARS_ENTGRAVITY]					= client->entgravity/sv_gravity.value;
			statsfi[STAT_MOVEVARS_JUMPVELOCITY]					= 270;//sv_jumpvelocity.value;	//bah
			statsfi[STAT_MOVEVARS_EDGEFRICTION]					= pm_edgefriction.value;
			statsfi[STAT_MOVEVARS_MAXAIRSPEED]					= 30;	//max speed before airaccel cuts out. this is hardcoded in qw pmove
			statsfi[STAT_MOVEVARS_STEPHEIGHT]					= *sv_stepheight.string?sv_stepheight.value:PM_DEFAULTSTEPHEIGHT;
			statsfi[STAT_MOVEVARS_AIRACCEL_QW]					= 1;		//we're a quakeworld engine...
			statsfi[STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION]	= 0;

			if (m < 256)
				m = 256;
		}
#endif

		SV_UpdateQCStats(ent, statsi, statss, statsf);
	}
	return m;
}

/*
=======================
SV_UpdateClientStats

Performs a delta update of the stats array.  This should only be performed
when a reliable message can be delivered this frame.
=======================
*/
void SV_UpdateClientStats (client_t *client, int pnum, sizebuf_t *msg, client_frame_t *frame)
{
	int		statsi[MAX_CL_STATS];
	float	statsf[MAX_CL_STATS];
	const char	*statss[MAX_CL_STATS];
	unsigned int		i, m;

	/*figure out what the stat values should be*/
	m = SV_CalcClientStats(client, statsi, statsf, statss);
	if ((client->fteprotocolextensions & (PEXT_HEXEN2|PEXT_CSQC)) || client->protocol == SCP_DARKPLACES6 || client->protocol == SCP_DARKPLACES7)
		m = min(m,256);
	else
		m = min(m,MAX_QW_STATS);

	if (client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
	{
		//diff numerical stats first
		for (i=0 ; i<m ; i++)
		{
			if (client->statsi[i] != statsi[i] || client->statsf[i] != statsf[i])
			{
				client->statsi[i] = statsi[i];
				client->statsf[i] = statsf[i];
				client->pendingstats[i>>5u] |= 1u<<(i&0x1f);
			}
		}
		//diff string stats.
		for (i=0 ; i<m ; i++)
		{
			const char *o=client->statss[i], *n=statss[i];
			if (o != n)
			{
				if (!o)
					o = "";
				if (!n)
					n = "";
				if (strcmp(o, n))
					client->pendingstats[(i+MAX_CL_STATS)>>5u] |= 1u<<((i+MAX_CL_STATS)&0x1f);
				//FIXME: we could always just run the QCGC on the player's string stats too. wouldn't need string compares that way
				if (client->statss[i])
					Z_Free((void*)client->statss[i]);
				client->statss[i] = (statss[i]&&*statss[i])?Z_StrDup(statss[i]):NULL;
			}
		}

		for (i=0 ; i<m ; i++)
		{
			if (client->pendingstats[i>>5u] & (1u<<(i&0x1f)))
			{
				float fval = client->statsf[i];
				int ival = client->statsi[i];

				//would overflow
				if (msg->cursize+8 >= msg->maxsize)
					break;
				//can't track it
				if (frame->numresendstats >= sizeof(frame->resendstats)/sizeof(frame->resendstats[0]))
					break;
				//we're going for it.
				client->pendingstats[i>>5u] &= ~(1u<<(i&0x1f));	//doesn't need resending any more
				frame->resendstats[frame->numresendstats++] = i | (pnum<<12);
				if (fval && fval != (float)(int)fval && !dpcompat_stats.ival)
				{
					if (pnum)
					{
						MSG_WriteByte(msg, svcfte_choosesplitclient);
						MSG_WriteByte(msg, pnum);
					}
					MSG_WriteByte(msg, svcfte_updatestatfloat);
					MSG_WriteByte(msg, i);
					MSG_WriteFloat(msg, fval);
				}
				else
				{
					if (fval)
						ival = fval;
					if (ival >= 0 && ival <= 255)
					{
						if (pnum)
						{
							MSG_WriteByte(msg, svcfte_choosesplitclient);
							MSG_WriteByte(msg, pnum);
						}
						MSG_WriteByte(msg, ISNQCLIENT(client)?svcdp_updatestatbyte:svcqw_updatestatbyte);
						MSG_WriteByte(msg, i);
						MSG_WriteByte(msg, ival);
					}
					else
					{
						if (pnum)
						{
							MSG_WriteByte(msg, svcfte_choosesplitclient);
							MSG_WriteByte(msg, pnum);
						}
						MSG_WriteByte(msg, ISNQCLIENT(client)?svcnq_updatestatlong:svcqw_updatestatlong);
						MSG_WriteByte(msg, i);
						MSG_WriteLong(msg, ival);
					}
				}
			}
		}

		for (i=0 ; i<m ; i++)
		{
			if (client->pendingstats[(i+MAX_CL_STATS)>>5u] & (1u<<((i+MAX_CL_STATS)&0x1f)))
			{
				const char *s = client->statss[i];
				if (!s)
					s = "";

				//would overflow
				if (msg->cursize+4+strlen(s) >= msg->maxsize)
					break;
				//can't track it
				if (frame->numresendstats >= sizeof(frame->resendstats)/sizeof(frame->resendstats[0]))
					break;
				//we're going for it.
				client->pendingstats[(i+MAX_CL_STATS)>>5u] &= ~(1u<<((i+MAX_CL_STATS)&0x1f));	//doesn't need resending any more
				frame->resendstats[frame->numresendstats++] = (i+MAX_CL_STATS) | (pnum<<12);
				if (pnum)
				{
					MSG_WriteByte(msg, svcfte_choosesplitclient);
					MSG_WriteByte(msg, pnum);
				}
				MSG_WriteByte(msg, svcfte_updatestatstring);
				MSG_WriteByte(msg, i);
				MSG_WriteString(msg, s);
			}
		}
	}
	else

	for (i=0 ; i<m ; i++)
	{
#ifdef SERVER_DEMO_PLAYBACK
		if (sv.demofile)
		{
			if (!client->spec_track)
			{
				statsf[i] = 0;
				if (i == STAT_HEALTH)
					statsf[i] = 100;
			}
			else
			{
				statsf[i] = sv.recordedplayer[client->spec_track - 1].stats[i];
				statsi[i] = sv.recordedplayer[client->spec_track - 1].stats[i];
			}
		}
#endif
		if (!ISQWCLIENT(client))
		{
			if (!statsi[i])
				statsi[i] = statsf[i];
			if (statsi[i] != client->statsi[i])
			{
				client->statsi[i] = statsi[i];
				ClientReliableWrite_Begin(client, svcnq_updatestatlong, 6);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Long(client, statsi[i]);
			}
		}
		else
		{
#ifdef PEXT_CSQC
			if (client->fteprotocolextensions & PEXT_CSQC)
			{
				if (statss[i] || client->statss[i])
				if (strcmp(statss[i]?statss[i]:"", client->statss[i]?client->statss[i]:""))
				{
					if (client->statss[i])
						Z_Free((void*)client->statss[i]);
					if (statss[i] && *statss[i])
						client->statss[i] = Z_StrDup(statss[i]);
					else
						client->statss[i] = NULL;

					if (pnum)
					{
						ClientReliableWrite_Begin(client->controller, svcfte_choosesplitclient, 5+strlen(statss[i]));
						ClientReliableWrite_Byte(client->controller, pnum);
						ClientReliableWrite_Byte(client->controller, svcfte_updatestatstring);
						ClientReliableWrite_Byte(client->controller, i);
						ClientReliableWrite_String(client->controller, statss[i]);
					}
					else
					{
						ClientReliableWrite_Begin(client, svcfte_updatestatstring, 3+strlen(statss[i]));
						ClientReliableWrite_Byte(client, i);
						ClientReliableWrite_String(client, statss[i]);
					}
				}
			}
			if (dpcompat_stats.ival)
			{
				if (statsf[i])
				{
					statsi[i] = statsf[i];
					statsf[i] = 0;
				}
			}
#endif

			if (statsf[i])
			{
				if (client->fteprotocolextensions & PEXT_CSQC)
				{
					if (statsf[i] != client->statsf[i])
					{
						if (statsf[i] - (float)(int)statsf[i] == 0 && statsf[i] >= 0 && statsf[i] <= 255)
						{
							if (pnum)
							{
								ClientReliableWrite_Begin(client->controller, svcfte_choosesplitclient, 5);
								ClientReliableWrite_Byte(client->controller, pnum);
								ClientReliableWrite_Byte(client->controller, svcqw_updatestatbyte);
								ClientReliableWrite_Byte(client->controller, i);
								ClientReliableWrite_Byte(client->controller, statsf[i]);
							}
							else
							{
								ClientReliableWrite_Begin(client, svcqw_updatestatbyte, 3);
								ClientReliableWrite_Byte(client, i);
								ClientReliableWrite_Byte(client, statsf[i]);
							}
						}
						else
						{
							if (pnum)
							{
								ClientReliableWrite_Begin(client->controller, svcfte_choosesplitclient, 8);
								ClientReliableWrite_Byte(client->controller, pnum);
								ClientReliableWrite_Byte(client->controller, svcfte_updatestatfloat);
								ClientReliableWrite_Byte(client->controller, i);
								ClientReliableWrite_Float(client->controller, statsf[i]);
							}
							else
							{
								ClientReliableWrite_Begin(client, svcfte_updatestatfloat, 6);
								ClientReliableWrite_Byte(client, i);
								ClientReliableWrite_Float(client, statsf[i]);
							}
						}
						client->statsf[i] = statsf[i];
						/*make sure statsf is correct*/
						client->statsi[i] = statsf[i];
					}
					continue;
				}
				else
				{
					statsi[i] = statsf[i];
				}
			}
			if (statsi[i] != client->statsi[i])
			{
				client->statsi[i] = statsi[i];
				client->statsf[i] = statsi[i];

				if (statsi[i] >=0 && statsi[i] <= 255)
				{
					if (pnum)
					{
						ClientReliableWrite_Begin(client->controller, svcfte_choosesplitclient, 5);
						ClientReliableWrite_Byte(client->controller, pnum);
						ClientReliableWrite_Byte(client->controller, svcqw_updatestatbyte);
						ClientReliableWrite_Byte(client->controller, i);
						ClientReliableWrite_Byte(client->controller, statsi[i]);
					}
					else
					{
						ClientReliableWrite_Begin(client, svcqw_updatestatbyte, 3);
						ClientReliableWrite_Byte(client, i);
						ClientReliableWrite_Byte(client, statsi[i]);
					}
				}
				else
				{
					if (pnum)
					{
						ClientReliableWrite_Begin(client->controller, svcfte_choosesplitclient, 8);
						ClientReliableWrite_Byte(client->controller, pnum);
						ClientReliableWrite_Byte(client->controller, svcqw_updatestatlong);
						ClientReliableWrite_Byte(client->controller, i);
						ClientReliableWrite_Long(client->controller, statsi[i]);
					}
					else
					{
						ClientReliableWrite_Begin(client, svcqw_updatestatlong, 6);
						ClientReliableWrite_Byte(client, i);
						ClientReliableWrite_Long(client, statsi[i]);
					}
				}
			}
		}
	}
}

qboolean SV_CanTrack(client_t *client, int entity)
{
	if (entity <= 0 || entity > sv.allocated_client_slots)
		return false;
	if (svs.clients[entity-1].spectator)
		return false;
	if (svs.clients[entity-1].state == cs_spawned || (svs.clients[entity-1].state == cs_free && svs.clients[entity-1].userinfo.numkeys))
		return true;
	return false;
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	qbyte		buf[MAX_OVERALLMSGLEN-64/*play safe*/];
	sizebuf_t	msg;
	size_t		clientlimit;
	unsigned int sentbytes;
	unsigned int outframeseq = client->netchan.incoming_sequence;	//this is so weird... but at least covers nq/qw sequence vs unreliables weirdness...

	if (ISQWCLIENT(client) || ISNQCLIENT(client))
	{
		client_frame_t *frame = &client->frameunion.frames[outframeseq & UPDATE_MASK];
		frame->numresendstats = 0;
	}

	msg.data = buf;
	msg.maxsize = sizeof(buf)-50;
	msg.cursize = 0;
	msg.allowoverflow = true;
	msg.overflowed = false;
	msg.prim = client->datagram.prim;

	if (client->spec_track && !SV_CanTrack(client, client->spec_track))
	{
		client->spec_track = 0;
		client->edict->v->goalentity = 0;
	}

	clientlimit = Netchan_GetMaxUnreliable(&client->netchan);
	if (clientlimit < 1024)
		clientlimit = 1024;	//ignore what the netchan wants. it'll have to fragment it.

	if (client->protocol == SCP_NETQUAKE && clientlimit > MAX_NQDATAGRAM && client->netchan.remote_address.type != NA_LOOPBACK && !client->pextknown)
		clientlimit = MAX_NQDATAGRAM;	//vanilla clients are limited. don't crash them.

	if (clientlimit > countof(buf))
		clientlimit = countof(buf);
	msg.maxsize = clientlimit - client->datagram.cursize;
	if (msg.maxsize <= 0)
		msg.maxsize = clientlimit;	//its going to overflow. favour ents over unreliables. its a little less fatal

	if (sv.world.worldmodel && !client->controller)
	{
#ifdef Q2SERVER
		if (ISQ2CLIENT(client))
		{
			SVQ2_BuildClientFrame (client);

			// send over all the relevant entity_state_t
			// and the player_state_t
			SVQ2_WriteFrameToClient (client, &msg);
		}
		else
#endif
		{

			if (!ISQ2CLIENT(client) && ((client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) || Netchan_CanReliable (&client->netchan, SV_RateForClient(client))))
			{
				int pnum=1;
				client_t *c;
				client_frame_t *frame = &client->frameunion.frames[outframeseq & UPDATE_MASK];
				SV_UpdateClientStats (client, 0, &msg, frame);

				for (c = client->controlled; c; c = c->controlled,pnum++)
					SV_UpdateClientStats(c, pnum, &msg, frame);
			}

			// add the client specific data to the datagram
			SV_WriteClientdataToMessage (client, &msg);

			// send over all the objects that are in the PVS
			// this will include clients, a packetentities, and
			// possibly a nails update
			SV_WriteEntitiesToClient (client, &msg, false);
		}
#ifdef VOICECHAT
		SV_VoiceSendPacket(client, &msg);
#endif
	}

	msg.maxsize = clientlimit;
	// copy the accumulated multicast datagram
	// for this client out to the message
	if (!client->datagram.overflowed && !msg.overflowed && msg.cursize + client->datagram.cursize <= clientlimit)
	{
		SZ_Write (&msg, client->datagram.data, client->datagram.cursize);
		SZ_Clear (&client->datagram);
	}

	if (msg.overflowed)
	{
		Con_Printf ("WARNING: msg overflowed for %s\n", client->name);
		SZ_Clear (&msg);
	}

#ifdef NQPROT
	SV_DarkPlacesDownloadChunk(client, &msg);
#endif

	// send the datagram
	sentbytes = Netchan_Transmit (&client->netchan, msg.cursize, buf, SV_RateForClient(client));
	if (ISNQCLIENT(client))
	{
		client_frame_t *frame = &client->frameunion.frames[outframeseq & UPDATE_MASK];
		frame->packetsizeout += sentbytes;
		frame->ping_time = -1;
		frame->senttime = realtime;
	}
	else if (ISQWCLIENT(client))
	{
		client_frame_t *frame = &client->frameunion.frames[client->netchan.outgoing_sequence & UPDATE_MASK];
		frame->packetsizeout += sentbytes;
	}

	if (ISNQCLIENT(client) && (client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
	{
		if (!client->datagram.overflowed && client->datagram.cursize)
		{
			SZ_Clear (&msg);
			SZ_Write (&msg, client->datagram.data, client->datagram.cursize);
			SZ_Clear (&client->datagram);

			sentbytes = Netchan_Transmit (&client->netchan, msg.cursize, buf, SV_RateForClient(client));
			if (ISQWCLIENT(client) || ISNQCLIENT(client))
			{
				client_frame_t *frame = &client->frameunion.frames[client->netchan.outgoing_sequence & UPDATE_MASK];
				frame->packetsizeout += sentbytes;
			}
		}
	}

	if (client->datagram.cursize)
	{
		Con_Printf ("WARNING: datagram overflowed for %s\n", client->name);
		SZ_Clear (&client->datagram);
	}
	return true;
}

client_t *SV_SplitClientDest(client_t *client, qbyte first, int size)
{
	client_t *sp;
	if (client->controller)
	{	//this is a slave client.
		//find the right number and send.
		int pnum = 0;
		for (sp = client->controller; sp; sp = sp->controlled)
		{
			if (sp == client)
				break;
			pnum++;
		}
		sp = client->controller;

		ClientReliableWrite_Begin (sp, svcfte_choosesplitclient, size+2);
		ClientReliableWrite_Byte (sp, pnum);
		ClientReliableWrite_Byte (sp, first);
		return sp;
	}
	else
	{
		ClientReliableWrite_Begin (client, first, size);
		return client;
	}
}

void SV_FlushBroadcasts (void)
{
	client_t *client;
	int j;
	// append the broadcast messages to each client messages
	for (j=0, client = svs.clients ; j<svs.allocated_client_slots ; j++, client++)
	{
		if (client->state < cs_connected)
			continue;	// reliables go to all connected or spawned
		if (client->controller)
			continue;	//splitscreen

		if (client->protocol == SCP_BAD)
			continue;	//botclient

#ifdef NQPROT
		if (ISNQCLIENT(client))
		{
			if (client->pextknown)	//hacky check - means 'has sent an svc_serverdata'
			{
				ClientReliableCheckBlock(client, sv.nqreliable_datagram.cursize);
				ClientReliableWrite_SZ(client, sv.nqreliable_datagram.data, sv.nqreliable_datagram.cursize);
			}
			if (client->state != cs_spawned)
				continue;	// datagrams only go to spawned
			SZ_Write (&client->datagram
				, sv.nqdatagram.data
				, sv.nqdatagram.cursize);
		}
		else
#endif
		{
			ClientReliableCheckBlock(client, sv.reliable_datagram.cursize);
			ClientReliableWrite_SZ(client, sv.reliable_datagram.data, sv.reliable_datagram.cursize);

			if (client->state != cs_spawned)
				continue;	// datagrams only go to spawned
			SZ_Write (&client->datagram
				, sv.datagram.data
				, sv.datagram.cursize);
		}
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
		SV_MVD_WriteReliables(true);
#endif

	SZ_Clear (&sv.reliable_datagram);
	SZ_Clear (&sv.datagram);
#ifdef NQPROT
	SZ_Clear (&sv.nqreliable_datagram);
	SZ_Clear (&sv.nqdatagram);
#endif
}

static qboolean SV_SyncInfoBuf(client_t *client)
{
	const char *key = client->infosync.keys[0].name;
	infobuf_t *info = client->infosync.keys[0].context;
	size_t bloboffset = client->infosync.keys[0].syncpos;
	//unsigned int seat = info - cls.userinfo;
	qboolean large;
	size_t blobsize;
	const char *blobdata = InfoBuf_BlobForKey(info, key, &blobsize, &large);
	size_t sendsize;
	size_t bufferspace;

	qboolean final;
	sizebuf_t *buf;

	if (ISQ2CLIENT(client))
	{	//q2 gamecode is fully responsible for networking this via configstrings.
		InfoSync_Clear(&client->infosync);
		return false;
	}

	if (client->num_backbuf)
		return false;
	if (client->netchan.message.cursize >= MAX_BACKBUFLEN/2)
		return false;	//don't bother trying to send anything.

	if (!large)
	{	//vanilla-compatible info.
		if (!blobdata)
			blobdata = "";

Con_DLPrintf(2, "%s: info %u:%s\n", client->name, (info == &svs.info)?0:(unsigned int)((client_t*)((char*)info-(char*)&((client_t*)NULL)->userinfo)-svs.clients), key);
		if (ISNQCLIENT(client))
		{	//except that nq never had any userinfo
			const char *s;
			if (info == &svs.info)
				s = va("//svi \"%s\" \"%s\"\n", key, blobdata);
			else
			{
				int playerslot = (client_t*)((char*)info-(char*)&((client_t*)NULL)->userinfo)-svs.clients;
				s = va("//ui %i \"%s\" \"%s\"\n", playerslot, key, blobdata);
			}
			buf = ClientReliable_StartWrite(client, 2+strlen(s));
			MSG_WriteByte(buf, svc_stufftext);
			MSG_WriteString(buf, s);
			ClientReliable_FinishWrite(client);
		}
		else if (ISQWCLIENT(client))
		{
			buf = ClientReliable_StartWrite(client, 2+strlen(key)+1+strlen(blobdata)+1);
			if (info == &svs.info)
				MSG_WriteByte(buf, svc_serverinfo);
			else
			{
				MSG_WriteByte(buf, svc_setinfo);
				MSG_WriteByte(buf, (client_t*)((char*)info-(char*)&((client_t*)NULL)->userinfo)-svs.clients);
			}
			MSG_WriteString(buf, key);
			MSG_WriteString(buf, blobdata);
			ClientReliable_FinishWrite(client);
		}
	}
	else if (client->fteprotocolextensions2 & PEXT2_INFOBLOBS)
	{
		char enckey[2048];
		unsigned int pl;
		if (info == &svs.info)
			pl = 0;	//players are 1-based. 0 is used for serverinfo.
		else
			pl = 1+((client_t*)((char*)info-(char*)&((client_t*)NULL)->userinfo)-svs.clients);

		if (!InfoBuf_EncodeString(key, strlen(key), enckey, sizeof(enckey)))
		{
			InfoSync_Remove(&client->infosync, 0);
			return false;
		}
		if (!blobdata)
			bloboffset = 0;	//wiped or something? I dunno, don't bug out though.

		sendsize = blobsize - bloboffset;
		bufferspace = MAX_BACKBUFLEN - client->netchan.message.cursize;
		bufferspace -= 8 + strlen(enckey) + 1;	//extra overhead
		sendsize = min(bufferspace, sendsize);
		final = (bloboffset+sendsize >= blobsize);

Con_DLPrintf(2, "%s: blob %u:%s@%u-%u\n", client->name, pl, key, (unsigned int)bloboffset, (unsigned int)(bloboffset+sendsize));
		buf = ClientReliable_StartWrite(client, 8+strlen(enckey)+1+sendsize);
		MSG_WriteByte(buf, svcfte_setinfoblob);
		MSG_WriteByte(buf, pl);
		MSG_WriteString(buf, enckey);
		MSG_WriteLong(buf, (final?0x80000000:0)|bloboffset);
		MSG_WriteShort(buf, sendsize);
		SZ_Write(buf, blobdata+bloboffset, sendsize);
		ClientReliable_FinishWrite(client);

		if (!final)
		{
			client->infosync.keys[0].syncpos += sendsize;
			return true;
		}
	}
	//else client can't receive this info, stop trying to send it.

	InfoSync_Remove(&client->infosync, 0);
	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client, *sp;
	edict_t *ent;
	const char *name;

	float curgrav;
	float curspeed;
	int curfrags;

	static double pingtimer, lasttime;
	double t = Sys_DoubleTime();
	qboolean sendpings = false;
	pingtimer -= (t-lasttime);
	lasttime = t;
	if (pingtimer < 0)
	{	//update about once every 5 secs.
		sendpings = true;
		pingtimer = 5;
	}

// check for changes to be sent over the reliable streams to all clients
	for (i=0, host_client = svs.clients ; i<svs.allocated_client_slots ; i++, host_client++)
	{
		if ((svs.gametype == GT_Q1QVM || svs.gametype == GT_PROGS) && host_client->state == cs_spawned)
		{
#ifdef HAVE_LEGACY
			//DP_SV_CLIENTCOLORS
			if (host_client->edict->xv->clientcolors != host_client->playercolor)
			{
				InfoBuf_SetValueForKey(&host_client->userinfo, "topcolor", va("%i", (int)host_client->edict->xv->clientcolors/16));
				InfoBuf_SetValueForKey(&host_client->userinfo, "bottomcolor", va("%i", (int)host_client->edict->xv->clientcolors&15));
				{
					SV_ExtractFromUserinfo (host_client, true);	//this will take care of nq for us anyway.
					SV_BroadcastUserinfoChange(host_client, true, "*bothcolours", NULL);
				}
			}

			if (host_client->dp_ping)
				*host_client->dp_ping = SV_CalcPing (host_client, false);
			if (host_client->dp_pl)
				*host_client->dp_pl = host_client->lossage;
#endif

#ifdef PEXT_VIEW2
			j = PROG_TO_EDICTINDEX(svprogfuncs, host_client->edict->xv->clientcamera);
			if (j)
			{
				if ((unsigned int)j >= svprogfuncs->edicttable_length)
					j = i+1;
				if (j != host_client->clientcamera)
				{
					if (host_client->fteprotocolextensions & PEXT_VIEW2)
					{
						ClientReliableWrite_Begin(host_client, svc_setview, 4);
						ClientReliableWrite_Entity(host_client, j);
					}
					if (j == i+1)
						j = 0;	//self.
					host_client->viewent = j;
				}
			}
#endif

			name = PR_GetString(svprogfuncs, host_client->edict->v->netname);
#ifndef QCGC	//this optimisation doesn't really work with a QC instead of static string management
			if (name != host_client->name)
#endif
			{
				if (strcmp(host_client->name, name))
				{
					char oname[80];
					Q_strncpyz(oname, host_client->name, sizeof(oname));

					Con_DPrintf("Client %s programatically renamed to %s\n", host_client->name, name);
					InfoBuf_SetValueForKey(&host_client->userinfo, "name", name);
					SV_ExtractFromUserinfo (host_client, true);

					if (strcmp(oname, host_client->name))
					{
						SV_BroadcastUserinfoChange(host_client, true, "name", host_client->name);
					}

#ifdef QCGC
					//if it got rejected/mangled, make sure the qc properly sees the current value.
					svprogfuncs->SetStringField(svprogfuncs, host_client->edict, &host_client->edict->v->netname, host_client->name, true);
#endif
				}
#ifndef QCGC
				svprogfuncs->SetStringField(svprogfuncs, host_client->edict, &host_client->edict->v->netname, host_client->name, true);
#endif
			}
		}

		if (host_client->state != cs_spawned)
		{
			if (!host_client->state && host_client->name && host_client->name[0])	//if this is a writebyte bot
			{
				if (host_client->old_frags != (int)host_client->edict->v->frags)
				{
					for (j=0, client = svs.clients ; j<svs.allocated_client_slots ; j++, client++)
					{
						if (client->state < cs_connected)
							continue;
						ClientReliableWrite_Begin(client, svc_updatefrags, 4);
						ClientReliableWrite_Byte(client, i);
#ifdef NQPROT
						if (ISNQCLIENT(client) && host_client->spectator == 1)
							ClientReliableWrite_Short(client, -999);
						else
#endif
							ClientReliableWrite_Short(client, host_client->edict->v->frags);
					}

#ifdef MVD_RECORDING
					if (sv.mvdrecording)
					{
						sizebuf_t *msg = MVDWrite_Begin(dem_all, 0, 4);
						MSG_WriteByte(msg, svc_updatefrags);
						MSG_WriteByte(msg, i);
						MSG_WriteShort(msg, host_client->edict->v->frags);
					}
#endif

					host_client->old_frags = host_client->edict->v->frags;
				}
			}
			continue;
		}
		if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
		{
			ent = host_client->edict;

			curfrags = host_client->edict->v->frags;
			curgrav = ent->xv->gravity*sv_gravity.value;
			curspeed = ent->xv->maxspeed;
			if (progstype != PROG_QW)
			{
				if (!curgrav)
					curgrav = sv_gravity.value;
				if (!curspeed)
					curspeed = sv_maxspeed.value;
			}
#ifdef HEXEN2
			if (ent->xv->hasted)
				curspeed*=ent->xv->hasted;
#endif
		}
		else
		{
			curgrav = sv_gravity.value;
			curspeed = sv_maxspeed.value;
			curfrags = 0;
		}
#ifdef SVCHAT	//enforce a no moving time when chatting. Prevent client prediction going mad.
		if (host_client->chat.active)
			curspeed = 0;
#endif

		if (!ISQ2CLIENT(host_client))
		{
			if (host_client->sendinfo)
			{
				host_client->sendinfo = false;
				SV_FullClientUpdate (host_client, NULL);
			}

			if (host_client->qex && sendpings)
			{
				sizebuf_t *m;
				for (j=0, client = svs.clients ; j<svs.allocated_client_slots && j < host_client->max_net_clients; j++, client++)
				{
					if (client->state != cs_spawned)
						continue;

					m = ClientReliable_StartWrite(host_client, 64);
					MSG_WriteByte(m, svcqex_updateping);
					MSG_WriteByte(m, j);
					MSG_WriteSignedQEX(m, SV_CalcPing(client, false));
					ClientReliable_FinishWrite(host_client);

					if (coop.ival)
					{
						m = ClientReliable_StartWrite(host_client, 64);
						MSG_WriteByte(m, svcqex_updateplinfo);
						MSG_WriteByte(m, j);
						MSG_WriteSignedQEX(m, client->edict->v->health);
						MSG_WriteSignedQEX(m, client->edict->v->armorvalue);
						ClientReliable_FinishWrite(host_client);
					}
				}
			}

			if (host_client->old_frags != curfrags)
			{
				for (j=0, client = svs.clients ; j<sv.allocated_client_slots ; j++, client++)
				{
					if (client->state < cs_connected)
						continue;
					if (client->controller)
						continue;
					switch(client->protocol)
					{
					case SCP_BAD:	//bots
					case SCP_QUAKE2:
					case SCP_QUAKE2EX:
					case SCP_QUAKE3:
						break;
					default:
						ClientReliableWrite_Begin(client, svc_updatefrags, 4);
						ClientReliableWrite_Byte(client, i);
#ifdef NQPROT
						if (ISNQCLIENT(client) && host_client->spectator == 1)
							ClientReliableWrite_Short(client, -999);
						else
#endif
							ClientReliableWrite_Short(client, curfrags);
						break;
					}
				}

#ifdef MVD_RECORDING
				if (sv.mvdrecording)
				{
					sizebuf_t *msg = MVDWrite_Begin(dem_all, 0, 4);
					MSG_WriteByte(msg, svc_updatefrags);
					MSG_WriteByte(msg, i);
					MSG_WriteShort(msg, curfrags);
				}
#endif

				host_client->old_frags = curfrags;
			}

			{
				if (host_client->entgravity != curgrav)
				{
					if (ISQWCLIENT(host_client))
					{
						sp = SV_SplitClientDest(host_client, svc_entgravity, 5);
						ClientReliableWrite_Float(sp, curgrav/movevars.gravity);	//lie to the client in a cunning way
					}
					host_client->entgravity = curgrav;
				}

				if (host_client->maxspeed != curspeed)
				{	//MSVC can really suck at times (optimiser bug)
					if (ISQWCLIENT(host_client))
					{
						if (host_client->controller)
						{	//this is a slave client.
							//find the right number and send.
							int pnum = 0;
							client_t *sp;
							for (sp = host_client->controller; sp; sp = sp->controlled)
							{
								if (sp == host_client)
									break;
								pnum++;
							}
							sp = host_client->controller;

							ClientReliableWrite_Begin (sp, svcfte_choosesplitclient, 7);
							ClientReliableWrite_Byte (sp, pnum);
							ClientReliableWrite_Byte (sp, svc_maxspeed);
							ClientReliableWrite_Float(sp, curspeed);
						}
						else
						{
							ClientReliableWrite_Begin(host_client, svc_maxspeed, 5);
							ClientReliableWrite_Float(host_client, curspeed);
						}
					}
					host_client->maxspeed = curspeed;
				}
			}
		}

		while (host_client->infosync.numkeys)
		{
			if (!SV_SyncInfoBuf(host_client))
				break;
		}
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording && demo.recorder.infosync.numkeys)
	{
		while (demo.recorder.infosync.numkeys)
		{
			if (!SV_SyncInfoBuf(&demo.recorder))
				break;
		}
	}
#endif

	if (sv.reliable_datagram.overflowed)
	{
		Con_Printf("WARNING: Reliable datagram overflowed\n");
		SZ_Clear (&sv.reliable_datagram);
	}

	if (sv.datagram.overflowed)
		SZ_Clear (&sv.datagram);

#ifdef NQPROT
	if (sv.nqdatagram.overflowed)
		SZ_Clear (&sv.nqdatagram);
#endif

	SV_FlushBroadcasts();
}

//#ifdef _MSC_VER
//#pragma optimize( "", off )
//#endif


//a single userinfo value was changed.
//*bothcolours sends out both topcolor and bottomcolor, with a single svc_updatecolors in nq
static void SV_SendUserinfoChange(client_t *to, client_t *about, qboolean isbasic, const char *key, const char *newval)
{
	int playernum = about - svs.clients;

	if (playernum > to->max_net_clients)
		return;

	if (!newval)
		newval = InfoBuf_ValueForKey(&about->userinfo, key);

	if (ISQWCLIENT(to))
	{
		if (isbasic || (to->fteprotocolextensions & PEXT_BIGUSERINFOS))
		{
			if (!strcmp(key, "*bothcolours"))
			{	//hack to shorten sending vanilla nq colour updates
				newval = InfoBuf_ValueForKey(&about->userinfo, key="topcolor");
				ClientReliableWrite_Begin(to, svc_setinfo, 4+strlen(key)+strlen(newval));
				ClientReliableWrite_Byte(to, playernum);
				ClientReliableWrite_String(to, key);
				ClientReliableWrite_String(to, InfoBuf_ValueForKey(&about->userinfo, "topcolor"));
				
				newval = InfoBuf_ValueForKey(&about->userinfo, key = "bottomcolor");
			}

			ClientReliableWrite_Begin(to, svc_setinfo, 4+strlen(key)+strlen(newval));
			ClientReliableWrite_Byte(to, playernum);
			ClientReliableWrite_String(to, key);
			ClientReliableWrite_String(to, newval);
		}
	}
#ifdef NQPROT
	else if (ISNQCLIENT(to))
	{
		if (to->fteprotocolextensions2 & PEXT2_PREDINFO)
		{	//this client has an understanding of userinfo, using it instead of svc_updatename+svc_updatecolors.
			char quotedkey[1024];
			char quotedval[8192];
			char *s;
			if (!strcmp(key, "*bothcolours"))
			{	//hack to shorten sending vanilla nq colour updates
				newval = InfoBuf_ValueForKey(&about->userinfo, key="bottomcolor");
				s = va("//ui %i %s %s\n", playernum, COM_QuotedString(key, quotedkey, sizeof(quotedkey), false), COM_QuotedString(newval, quotedval, sizeof(quotedval), false));
				ClientReliableWrite_Begin(to, svc_stufftext, 2+strlen(s));
				ClientReliableWrite_String(to, s);

				newval = InfoBuf_ValueForKey(&about->userinfo, key="bottomcolor");
			}
			s = va("//ui %i %s %s\n", playernum, COM_QuotedString(key, quotedkey, sizeof(quotedkey), false), COM_QuotedString(newval, quotedval, sizeof(quotedval), false));
			ClientReliableWrite_Begin(to, svc_stufftext, 2+strlen(s));
			ClientReliableWrite_String(to, s);
		}
		else
		{	//legacy client.
			if (!strcmp(key, "*spectator"))
			{	//nq does not support spectators, mods tend to use frags=-999 or -99 instead.
				//yes, this breaks things.
				ClientReliableWrite_Begin(to, svc_updatefrags, 4);
				ClientReliableWrite_Byte(to, playernum);
				if (atoi(newval) == 1)
					ClientReliableWrite_Short(to, -999);
				else
					ClientReliableWrite_Short(to, about->old_frags);	//restore their true frag count
			}
			else if (!strcmp(key, "name"))
			{
				ClientReliableWrite_Begin(to, svc_updatename, 3+strlen(newval));
				ClientReliableWrite_Byte(to, playernum);
				ClientReliableWrite_String(to, newval);
			}
			else if (!strcmp(key, "topcolor") || !strcmp(key, "bottomcolor") || !strcmp(key, "*bothcolours"))
			{	//due to these being combined, nq players get double colour change notifications...
				int tc = atoi(InfoBuf_ValueForKey(&about->userinfo, "topcolor"));
				int bc = atoi(InfoBuf_ValueForKey(&about->userinfo, "bottomcolor"));
				if (tc < 0 || tc > 13)
					tc = 0;
				if (bc < 0 || bc > 13)
					bc = 0;
				ClientReliableWrite_Begin(to, svc_updatecolors, 3);
				ClientReliableWrite_Byte(to, playernum);
				ClientReliableWrite_Byte(to, 16*tc + bc);
			}
		}
	}
#endif
}
void SV_BroadcastUserinfoChange(client_t *about, qboolean isbasic, const char *key, const char *newval)
{
	client_t *client;
	int j;
	if (!newval)
		newval = InfoBuf_ValueForKey(&about->userinfo, key);
	for (j = 0; j < svs.allocated_client_slots; j++)
	{
		client = svs.clients+j;
		if (client->state < cs_connected)
			continue;	// reliables go to all connected or spawned
		if (client->controller)
			continue;	//splitscreen

		if (client->protocol == SCP_BAD)
			continue;	//botclient

		SV_SendUserinfoChange(client, about, isbasic, key, newval);
	}

#ifdef MVD_RECORDING
	if (sv.mvdrecording && (isbasic || (demo.recorder.fteprotocolextensions & PEXT_BIGUSERINFOS)))
	{
		sizebuf_t *msg = MVDWrite_Begin (dem_all, 0, strlen(key)+strlen(newval)+4);
		MSG_WriteByte (msg, svc_setinfo);
		MSG_WriteByte (msg, about - svs.clients);
		MSG_WriteString (msg, key);
		MSG_WriteString (msg, newval);
	}
#endif
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i, j;
	client_t	*c;
	int sentbytes, fnum;
#ifdef NQPROT
	float pt = sv.paused?realtime:sv.world.physicstime;
#endif

#ifdef NEWSPEEDCHEATPROT
	static unsigned int lasttime;
	unsigned int curtime = Sys_Milliseconds();
	unsigned int msecs = curtime - lasttime;
	lasttime = curtime;
#endif

#ifdef Q3SERVER
	if (svs.gametype == GT_QUAKE3)
	{
		for (i=0, c = svs.clients ; i<svs.allocated_client_slots ; i++, c++)
		{
			if (c->state < cs_connected)
				continue;

			if (c->drop)
			{
				SV_DropClient(c);
				c->drop = false;
				continue;
			}

			if (c->protocol == SCP_BAD)	//this is a bot.
			{
				SZ_Clear (&c->netchan.message);
				SZ_Clear (&c->datagram);
				continue;
			}

			if (c->lastoutgoingphysicstime == pt)
				continue;
			c->lastoutgoingphysicstime = pt;

			q3->sv.SendMessage(c);
		}
		return;
	}
#endif

// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, c = svs.clients ; i<svs.allocated_client_slots ; i++, c++)
	{
		if (c->state < cs_loadzombie)
			continue;

		if (c->drop)
		{
			SV_DropClient(c);
			c->drop = false;
			continue;
		}

		if (c->state == cs_loadzombie)
		{	//not yet present.
			c->netchan.message.cursize = 0;
			c->datagram.cursize = 0;
			continue;
		}

#ifdef SVCHAT
		SV_ChatThink(c);
#endif

#ifdef NEWSPEEDCHEATPROT
		//allow the client more time for client movement.
		//if they're running too slowly, FORCE them to run
		//this little check is to guard against people using msecs=0 to hover in mid-air. also keeps players animating/moving/etc when timing
		c->msecs += msecs;
		while (c->state >= cs_spawned && c->msecs > 1000)
		{
			if (c->msecs > 1200)
				c->msecs = 1200;
			if (c->isindependant && !sv.paused)
			{
				unsigned int stepmsec;
				usercmd_t cmd;
				memset(&cmd, 0, sizeof(cmd));
				host_client = c;
				sv_player = c->edict;
				SV_PreRunCmd();
				stepmsec = 13;
				cmd.msec = stepmsec;
				c->hoverms += cmd.msec;	//sv_showpredloss shows this later when we do get the next packet, instead of potentially spamming every frame while they lag.
				VectorCopy(c->lastcmd.angles, cmd.angles);
				cmd.buttons = c->lastcmd.buttons;
				SV_RunCmd (&cmd, true);
				SV_PostRunCmd();
				c->lastruncmd = (unsigned int)(sv.time*1000)-c->msecs;
				if (stepmsec > c->msecs)
					c->msecs = 0;
				else
					c->msecs -= stepmsec;
				if (c->msecs > 2000)
					c->msecs = 2000;	//assume debugger or system suspend/hibernate
				host_client = NULL;
				sv_player = NULL;
			}
			else
				c->msecs = 500;	//for switching between.
		}
#endif

#ifdef Q3SERVER
		if (ISQ3CLIENT(c))
		{	//q3 protocols bypass backbuffering and pretty much everything else
			q3->sv.SendMessage(c);
			continue;
		}
#endif

		// check to see if we have a backbuf to stick in the reliable
		if (c->num_backbuf)
		{
			// will it fit?
			if (c->netchan.message.cursize + c->backbuf_size[0] <=
				c->netchan.message.maxsize)
			{

				Con_DPrintf("%s: backbuf %d bytes\n",
					c->name, c->backbuf_size[0]);

				// it'll fit
				SZ_Write(&c->netchan.message, c->backbuf_data[0],
					c->backbuf_size[0]);

				//move along, move along
				for (j = 1; j < c->num_backbuf; j++)
				{
					memcpy(c->backbuf_data[j - 1], c->backbuf_data[j],
						c->backbuf_size[j]);
					c->backbuf_size[j - 1] = c->backbuf_size[j];
				}

				c->num_backbuf--;
				if (c->num_backbuf)
				{
					memset(&c->backbuf, 0, sizeof(c->backbuf));
					c->backbuf.prim = c->netchan.message.prim;
					c->backbuf.data = c->backbuf_data[c->num_backbuf - 1];
					c->backbuf.cursize = c->backbuf_size[c->num_backbuf - 1];
					c->backbuf.maxsize = min(c->netchan.message.maxsize, sizeof(c->backbuf_data[c->num_backbuf-1]));
				}
			}
		}

		if (c->protocol == SCP_BAD)
		{
			SZ_Clear (&c->netchan.message);
			SZ_Clear (&c->datagram);
			c->num_backbuf = 0;
			if (c->edict)
				c->edict->v->fixangle = FIXANGLE_NO;
			continue;
		}

		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			SZ_Clear (&c->netchan.message);
			SZ_Clear (&c->datagram);
			SV_BroadcastPrintf (PRINT_HIGH, "%s overflowed\n", c->name);
			Con_Printf ("WARNING: reliable overflow for %s\n",c->name);
			c->send_message = true;
			c->netchan.cleartime = 0;	// don't choke this message
			SV_DropClient (c);
			continue;
		}

#ifdef NQPROT
		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (ISNQCLIENT(c))
		{
			//tread carefully with NQ:
			//while loading models etc, NQ will error out if it receives anything that it wasn't expecting.
			//we should still send unreliable nops whenever we want as a keepalive (and we may need to in order to wake up the client).
			//other unreliables are disallowed when connecting, due to sync issues.
			//reliables may be sent only if some other code has said that its okay (to avoid stray name changes killing clients).
			if (c->state == cs_connected)
			{
				if (c->nextservertimeupdate > pt + 6)
					c->nextservertimeupdate = 0;

				c->netchan.cleartime = realtime - 100;
				if (c->netchan.nqunreliableonly == 1)
					c->netchan.nqunreliableonly = !c->send_message;
				c->datagram.cursize = 0;
				if (!c->send_message && c->nextservertimeupdate < pt)
				{
					if (c->nextservertimeupdate)
						MSG_WriteByte(&c->datagram, svc_nop);
					c->nextservertimeupdate = pt+5;
				}
				c->send_message = true;
				//we can still send an outgoing packet if something set send_message. This should really only be svnq_new_f and friends.
			}
			else
			{
				extern cvar_t sv_nqplayerphysics;
				if (c->nextservertimeupdate > pt + 0.1)
					c->nextservertimeupdate = 0;

				c->netchan.nqunreliableonly = false;
				c->send_message = false;
				//nq sends one packet only for each server physics frame
				if (sv_mintic.value || sv_nqplayerphysics.ival)	//(nqplayerphysics forces 72hz when mintic )
				{	//explicit packet/tick rate. don't spam faster/slower, clients don't like that too much.
					if (c->nextservertimeupdate != pt && c->state >= cs_connected)
					{
						c->send_message = true;
						c->nextservertimeupdate = pt;
					}
				}
				else
				{
					if (c->nextservertimeupdate < pt && c->state >= cs_connected)
					{
						c->send_message = true;
						c->nextservertimeupdate = pt + 1.0/77;
					}
				}
			}
		}
		//qw servers will set send_message on packet reception.
#endif

		SV_ProcessSendFlags(c);

		if (!c->send_message)
			continue;
		c->send_message = false;	// try putting this after choke?

		if (c->controller)
			continue;	/*shouldn't have been set*/

		if (!sv.paused && !Netchan_CanPacket (&c->netchan, SV_RateForClient(c)))
		{
			c->chokecount++;
			c->waschoked = true;
			continue;		// bandwidth choke
		}
		c->waschoked = false;

		if (sv.time > c->ratetime + 1)
		{
			c->inrate = c->netchan.bytesin / (sv.time - c->ratetime);
			c->outrate = c->netchan.bytesout / (sv.time - c->ratetime);
			c->netchan.bytesin = 0;
			c->netchan.bytesout = 0;
			c->ratetime = sv.time;
		}

		SV_ReplaceEntityFrame(c, c->netchan.outgoing_sequence);
		SV_SendClientPrespawnInfo(c);
		if (c->state == cs_spawned)
			SV_SendClientDatagram (c);
		else
		{
#ifdef NQPROT
			SV_DarkPlacesDownloadChunk(c, &c->datagram);
#endif
			fnum = c->netchan.outgoing_sequence;
			sentbytes = Netchan_Transmit (&c->netchan, c->datagram.cursize, c->datagram.data, SV_RateForClient(c));	// just update reliable
			if (ISQWCLIENT(c) || ISNQCLIENT(c))
				c->frameunion.frames[fnum & UPDATE_MASK].packetsizeout += sentbytes;
			c->datagram.cursize = 0;
		}
		c->lastoutgoingphysicstime = sv.world.physicstime;

		if (c->netchan.fatal_error)
			c->drop = true;
	}
#ifdef MVD_RECORDING
	if (sv.mvdrecording)
		SV_ProcessSendFlags(&demo.recorder);
#endif
	SV_CleanupEnts();
}

//#ifdef _MSC_VER
//#pragma optimize( "", on )
//#endif

#ifdef MVD_RECORDING
void SV_WriteMVDMessage (sizebuf_t *msg, int type, int to, float time);
void SV_MVD_CheckReverse(void);

void DemoWriteQTVTimePad(int msecs);
#define Max(a, b) ((a>b)?a:b)
void SV_SendMVDMessage(void)
{
	int			i, j, m, cls = 0;
	client_t	*c;
	qbyte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;
	int		statsi[MAX_CL_STATS];
	float	statsf[MAX_CL_STATS];
	const char	*statss[MAX_CL_STATS];
	float		min_fps;
	extern		cvar_t sv_demofps;
	extern		cvar_t sv_demoPings;
//	extern		cvar_t	sv_demoMaxSize;
	sizebuf_t *dmsg;

	SV_MVD_CheckReverse();

	if (!sv.mvdrecording)
		return;

	if (sv_demoPings.value)
	{
		if (sv.time - demo.pingtime > sv_demoPings.value)
		{
			SV_MVDPings();
			demo.pingtime = sv.time;
		}
	}


	if (sv_demofps.value <= 1)
		min_fps = 30.0;
	else
		min_fps = sv_demofps.value;

	min_fps = Max(4, min_fps);
	if (sv.time - demo.time < 1.0/min_fps)
		return;

	for (i=0, c = svs.clients ; i<svs.allocated_client_slots && i < 32; i++, c++)
	{
		if (c->state != cs_spawned)
			continue;	// datagrams only go to spawned

		cls |= 1 << i;
	}

	if (!cls)
	{
		SZ_Clear (&demo.datagram);
		DemoWriteQTVTimePad((int)((sv.time - demo.time)*1000));
		DestFlush(false);
		demo.time = sv.time;
		return;
	}

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;
	msg.allowoverflow = true;
	msg.overflowed = false;

	for (i=0, c = svs.clients ; i<svs.allocated_client_slots && i < 32; i++, c++)
	{
		if (c->state != cs_spawned)
			continue;	// datagrams only go to spawned

		if (c->spectator)
			continue;

		/*figure out what the stat values should be*/
		m = SV_CalcClientStats(c, statsi, statsf, statss);
		if (demo.recorder.fteprotocolextensions & (PEXT_HEXEN2|PEXT_CSQC))
			m = min(m,MAX_CL_STATS);
		else
			m = min(m,MAX_QW_STATS);

		//FIXME we should do something about the packet overhead here. each MVDWrite_Begin is a separate packet!

		for (j=0 ; j<m ; j++)
		{
			if (demo.recorder.fteprotocolextensions & PEXT_CSQC)
			{
				if (statss[j] || demo.statss[i][j])
				if (strcmp(statss[j]?statss[j]:"", demo.statss[i][j]?demo.statss[i][j]:""))
				{
					sizebuf_t *msg = MVDWrite_Begin(dem_stats, i, 3+strlen(statss[j]));

					if (demo.statss[i][j])
						Z_Free(demo.statss[i][j]);
					if (statss[j] && *statss[j])
						demo.statss[i][j] = Z_StrDup(statss[j]);
					else
						demo.statss[i][j] = NULL;

					MSG_WriteByte(msg, svcfte_updatestatstring);
					MSG_WriteByte(msg, j);
					MSG_WriteString(msg, statss[j]);
				}
			}

			if (statsf[j])
			{
				if (demo.recorder.fteprotocolextensions & PEXT_CSQC)
				{
					if (statsf[j] != demo.statsf[i][j])
					{
						if (statsf[j] - (float)(int)statsf[j] == 0 && statsf[j] >= 0 && statsf[j] <= 255)
						{
							dmsg = MVDWrite_Begin(dem_stats, i, 3);
							MSG_WriteByte(dmsg, svcqw_updatestatbyte);
							MSG_WriteByte(dmsg, j);
							MSG_WriteByte(dmsg, statsf[j]);
						}
						else
						{
							dmsg = MVDWrite_Begin(dem_stats, i, 6);
							MSG_WriteByte(dmsg, svcfte_updatestatfloat);
							MSG_WriteByte(dmsg, j);
							MSG_WriteFloat(dmsg, statsf[j]);
						}
						demo.statsf[i][j] = statsf[j];
						/*make sure statsf is correct*/
						demo.statsi[i][j] = statsf[j];
					}
					continue;
				}
				else
					statsi[j] = statsf[j];
			}

			if (statsi[j] != demo.statsi[i][j])
			{
				demo.statsi[i][j] = statsi[j];
				demo.statsf[i][j] = statsi[j];
				if (statsi[j] >=0 && statsi[j] <= 255)
				{
					dmsg = MVDWrite_Begin(dem_stats, i, 3);
					MSG_WriteByte(dmsg, svcqw_updatestatbyte);
					MSG_WriteByte(dmsg, j);
					MSG_WriteByte(dmsg, statsi[j]);
				}
				else
				{
					dmsg = MVDWrite_Begin(dem_stats, i, 6);
					MSG_WriteByte(dmsg, svcqw_updatestatlong);
					MSG_WriteByte(dmsg, j);
					MSG_WriteLong(dmsg, statsi[j]);
				}
			}
		}
	}

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	msg.cursize = 0;
	msg.prim = demo.recorder.netchan.netprim;

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (demo.datagram.cursize && sv.mvdrecording)
	{
		dmsg = MVDWrite_Begin(dem_all, 0, demo.datagram.cursize);
		SZ_Write (dmsg, demo.datagram.data, demo.datagram.cursize);
		SZ_Clear (&demo.datagram);
	}

	while (demo.lastwritten < demo.parsecount-1 && sv.mvdrecording)
	{
		if (!SV_MVDWritePackets(1))
			break;
	}

	if (demo.resetdeltas)
	{
		demo.resetdeltas = false;
		demo.recorder.delta_sequence = -1;
	}
	else
		demo.recorder.delta_sequence = demo.recorder.netchan.incoming_sequence&255;
	demo.recorder.netchan.incoming_sequence++;
	demo.frames[demo.parsecount&DEMO_FRAMES_MASK].time = demo.time = sv.time;

	if (sv.mvdrecording)
	{
		SV_WriteEntitiesToClient (&demo.recorder, &msg, true);
		SV_WriteMVDMessage(&msg, dem_all, 0, sv.time);
//		dmsg = MVDWrite_Begin(dem_all, 0, msg.cursize);
//		SZ_Write (dmsg, msg.data, msg.cursize);
	}

	demo.parsecount++;

//	MVDSetMsgBuf(demo.dbuf,&demo.frames[demo.parsecount&DEMO_FRAMES_MASK].buf);
}
#endif


/*
=======================
SV_SendMessagesToAll

FIXME: does this sequence right?
=======================
*/
void SV_SendMessagesToAll (void)
{
	int			i;
	client_t	*c;

	for (i=0, c = svs.clients ; i<svs.allocated_client_slots ; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
			c->send_message = true;

	SV_SendClientMessages ();
}

#endif
