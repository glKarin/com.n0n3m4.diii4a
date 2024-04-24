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
#include "utf8lib.h"
#include "server.h"
#include "sv_demo.h"

int current_skill;
cvar_t sv_cheats = {CF_SERVER | CF_NOTIFY, "sv_cheats", "0", "enables cheat commands in any game, and cheat impulses in dpmod"};
cvar_t sv_adminnick = {CF_SERVER | CF_ARCHIVE, "sv_adminnick", "", "nick name to use for admin messages instead of host name"};
cvar_t sv_status_privacy = {CF_SERVER | CF_ARCHIVE, "sv_status_privacy", "0", "do not show IP addresses in 'status' replies to clients"};
cvar_t sv_status_show_qcstatus = {CF_SERVER | CF_ARCHIVE, "sv_status_show_qcstatus", "0", "show the 'qcstatus' field in status replies, not the 'frags' field. Turn this on if your mod uses this field, and the 'frags' field on the other hand has no meaningful value."};
cvar_t sv_namechangetimer = {CF_SERVER | CF_ARCHIVE, "sv_namechangetimer", "5", "how often to allow name changes, in seconds (prevents people from using animated names and other tricks"};

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/

/*
======================
SV_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
static void SV_Map_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("map <levelname> : start a new game (kicks off all players)\n");
		return;
	}

	// GAME_DELUXEQUAKE - clear warpmark (used by QC)
	if (gamemode == GAME_DELUXEQUAKE)
		Cvar_Set(&cvars_all, "warpmark", "");

	if(host.hook.Disconnect)
		host.hook.Disconnect(false, NULL);

	SV_Shutdown();

	if(svs.maxclients != svs.maxclients_next)
	{
		svs.maxclients = svs.maxclients_next;
		if (svs.clients)
			Mem_Free(svs.clients);
		svs.clients = (client_t *)Mem_Alloc(sv_mempool, sizeof(client_t) * svs.maxclients);
	}

	if(host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	svs.serverflags = 0;			// haven't completed an episode yet
	SV_SpawnServer(Cmd_Argv(cmd, 1));

	if(sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

/*
==================
SV_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
static void SV_Changelevel_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("changelevel <levelname> : continue game on a new level\n");
		return;
	}

	if (!sv.active)
	{
		SV_Map_f(cmd);
		return;
	}

	if(host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	SV_SaveSpawnparms ();
	SV_SpawnServer(Cmd_Argv(cmd, 1));
	
	if(sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

/*
==================
SV_Restart_f

Restarts the current server for a dead player
==================
*/
static void SV_Restart_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 1)
	{
		Con_Print("restart : restart current level\n");
		return;
	}
	if (!sv.active)
	{
		Con_Print("Only the server may restart\n");
		return;
	}

	if(host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	SV_SpawnServer(sv.worldbasename);
	
	if(sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

//===========================================================================

// Disable cheats if sv_cheats is turned off
static void SV_DisableCheats_c(cvar_t *var)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;

	if (prog->loaded && var->value == 0)
	{
		for (i = 0; i < svs.maxclients; ++i)
		{
			if (((int)PRVM_serveredictfloat(svs.clients[i].edict, flags) & FL_GODMODE))
				PRVM_serveredictfloat(svs.clients[i].edict, flags) = (int)PRVM_serveredictfloat(svs.clients[i].edict, flags) ^ FL_GODMODE;
			if (((int)PRVM_serveredictfloat(svs.clients[i].edict, flags) & FL_NOTARGET))
				PRVM_serveredictfloat(svs.clients[i].edict, flags) = (int)PRVM_serveredictfloat(svs.clients[i].edict, flags) ^ FL_NOTARGET;
			if (PRVM_serveredictfloat(svs.clients[i].edict, movetype) == MOVETYPE_NOCLIP ||
				PRVM_serveredictfloat(svs.clients[i].edict, movetype) == MOVETYPE_FLY)
			{
				noclip_anglehack = false;
				PRVM_serveredictfloat(svs.clients[i].edict, movetype) = MOVETYPE_WALK;
			}
		}
	}
}

/*
==================
SV_God_f

Sets client to godmode
==================
*/
static void SV_God_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	PRVM_serveredictfloat(host_client->edict, flags) = (int)PRVM_serveredictfloat(host_client->edict, flags) ^ FL_GODMODE;
	if (!((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_GODMODE) )
		SV_ClientPrint("godmode OFF\n");
	else
		SV_ClientPrint("godmode ON\n");
}

qbool noclip_anglehack;

static void SV_Noclip_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	if (PRVM_serveredictfloat(host_client->edict, movetype) != MOVETYPE_NOCLIP)
	{
		noclip_anglehack = true;
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_NOCLIP;
		SV_ClientPrint("noclip ON\n");
	}
	else
	{
		noclip_anglehack = false;
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_WALK;
		SV_ClientPrint("noclip OFF\n");
	}
}

/*
==================
SV_Give_f
==================
*/
static void SV_Give_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	const char *t;
	int v;
	int player_items;

	t = Cmd_Argv(cmd, 1);
	v = atoi (Cmd_Argv(cmd, 2));

	switch (t[0])
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// MED 01/04/97 added hipnotic give stuff
		if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
		{
			if (t[0] == '6')
			{
				if (t[1] == 'a')
					PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_PROXIMITY_GUN;
				else
					PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | IT_GRENADE_LAUNCHER;
			}
			else if (t[0] == '9')
				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_LASER_CANNON;
			else if (t[0] == '0')
				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_MJOLNIR;
			else if (t[0] >= '2')
				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | (IT_SHOTGUN << (t[0] - '2'));
		}
		else
		{
			if (t[0] >= '2')
				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | (IT_SHOTGUN << (t[0] - '2'));
		}
		break;

	case 's':
		if (gamemode == GAME_ROGUE)
			PRVM_serveredictfloat(host_client->edict, ammo_shells1) = v;

		PRVM_serveredictfloat(host_client->edict, ammo_shells) = v;
		break;
	case 'n':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_nails1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		}
		else
		{
			PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		}
		break;
	case 'l':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_lava_nails) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		}
		break;
	case 'r':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_rockets1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		}
		else
		{
			PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		}
		break;
	case 'm':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_multi_rockets) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		}
		break;
	case 'h':
		PRVM_serveredictfloat(host_client->edict, health) = v;
		break;
	case 'c':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_cells1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		}
		else
		{
			PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		}
		break;
	case 'p':
		if (gamemode == GAME_ROGUE)
		{
			PRVM_serveredictfloat(host_client->edict, ammo_plasma) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		}
		break;
	case 'a':
		//
		// Set the player armour value to the number specified and then adjust the armour type/colour based on the value
		//
 		player_items = PRVM_serveredictfloat(host_client->edict, items);
		PRVM_serveredictfloat(host_client->edict, armorvalue) = v;

		// Remove whichever armour item the player currently has
		if (gamemode == GAME_ROGUE)
			player_items &= ~(RIT_ARMOR1 | RIT_ARMOR2 | RIT_ARMOR3);
		else
			player_items &= ~(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3);

		if (v > 150)
		{
			// Give red armour
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.8;
			if (gamemode == GAME_ROGUE)
				player_items |= RIT_ARMOR3;
			else
				player_items |= IT_ARMOR3;
		}
		else if (v > 100)
		{
			// Give yellow armour
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.6;
			if (gamemode == GAME_ROGUE)
				player_items |= RIT_ARMOR2;
			else
				player_items |= IT_ARMOR2;
		}
		else if (v >= 0)
		{
			// Give green armour
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.3;
			if (gamemode == GAME_ROGUE)
				player_items |= RIT_ARMOR1;
			else
				player_items |= IT_ARMOR1;
		}

		PRVM_serveredictfloat(host_client->edict, items) = player_items;
		break;
	}
}

/*
==================
SV_Fly_f

Sets client to flymode
==================
*/
static void SV_Fly_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	if (PRVM_serveredictfloat(host_client->edict, movetype) != MOVETYPE_FLY)
	{
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_FLY;
		SV_ClientPrint("flymode ON\n");
	}
	else
	{
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_WALK;
		SV_ClientPrint("flymode OFF\n");
	}
}

static void SV_Notarget_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	PRVM_serveredictfloat(host_client->edict, flags) = (int)PRVM_serveredictfloat(host_client->edict, flags) ^ FL_NOTARGET;
	if (!((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_NOTARGET) )
		SV_ClientPrint("notarget OFF\n");
	else
		SV_ClientPrint("notarget ON\n");
}

/*
==================
SV_Kill_f
==================
*/
static void SV_Kill_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	if (PRVM_serveredictfloat(host_client->edict, health) <= 0)
	{
		SV_ClientPrint("Can't suicide -- already dead!\n");
		return;
	}

	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
	prog->ExecuteProgram(prog, PRVM_serverfunction(ClientKill), "QC function ClientKill is missing");
}

/*
==================
SV_Pause_f
==================
*/
static void SV_Pause_f(cmd_state_t *cmd)
{
	void (*print) (const char *fmt, ...);
	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!pausable.integer && cmd->source == src_client && LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
	{
		print("Pause not allowed.\n");
		return;
	}
	
	sv.paused ^= 1;
	if (cmd->source != src_local)
		SV_BroadcastPrintf("%s %spaused the game\n", host_client->name, sv.paused ? "" : "un");
	else if(*(sv_adminnick.string))
		SV_BroadcastPrintf("%s %spaused the game\n", sv_adminnick.string, sv.paused ? "" : "un");
	else
		SV_BroadcastPrintf("%s %spaused the game\n", hostname.string, sv.paused ? "" : "un");
	// send notification to all clients
	MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
	MSG_WriteByte(&sv.reliable_datagram, sv.paused);
}

static void SV_Say(cmd_state_t *cmd, qbool teamonly)
{
	prvm_prog_t *prog = SVVM_prog;
	client_t *save;
	int j, quoted;
	const char *p1;
	char *p2;
	// LadyHavoc: long say messages
	char text[1024];
	qbool fromServer = false;

	if (cmd->source == src_local)
	{
		fromServer = true;
		teamonly = false;
	}

	if (Cmd_Argc (cmd) < 2)
		return;

	if (!teamplay.integer)
		teamonly = false;

	p1 = Cmd_Args(cmd);
	quoted = false;
	if (*p1 == '\"')
	{
		quoted = true;
		p1++;
	}
	// note this uses the chat prefix \001
	if (!fromServer && !teamonly)
		dpsnprintf (text, sizeof(text), "\001%s: %s", host_client->name, p1);
	else if (!fromServer && teamonly)
		dpsnprintf (text, sizeof(text), "\001(%s): %s", host_client->name, p1);
	else if(*(sv_adminnick.string))
		dpsnprintf (text, sizeof(text), "\001<%s> %s", sv_adminnick.string, p1);
	else
		dpsnprintf (text, sizeof(text), "\001<%s> %s", hostname.string, p1);
	p2 = text + strlen(text);
	while ((const char *)p2 > (const char *)text && (p2[-1] == '\r' || p2[-1] == '\n' || (p2[-1] == '\"' && quoted)))
	{
		if (p2[-1] == '\"' && quoted)
			quoted = false;
		p2[-1] = 0;
		p2--;
	}
	dp_strlcat(text, "\n", sizeof(text));

	// note: save is not a valid edict if fromServer is true
	save = host_client;
	for (j = 0, host_client = svs.clients;j < svs.maxclients;j++, host_client++)
		if (host_client->active && (!teamonly || PRVM_serveredictfloat(host_client->edict, team) == PRVM_serveredictfloat(save->edict, team)))
			SV_ClientPrint(text);
	host_client = save;

	if(!host_isclient.integer)
		Con_Print(&text[1]);
}

static void SV_Say_f(cmd_state_t *cmd)
{
	SV_Say(cmd, false);
}

static void SV_Say_Team_f(cmd_state_t *cmd)
{
	SV_Say(cmd, true);
}

static void SV_Tell_f(cmd_state_t *cmd)
{
	const char *playername_start = NULL;
	size_t playername_length = 0;
	int playernumber = 0;
	client_t *save;
	int j;
	const char *p1, *p2;
	char text[MAX_INPUTLINE]; // LadyHavoc: FIXME: temporary buffer overflow fix (was 64)
	qbool fromServer = false;

	if (cmd->source == src_local)
		fromServer = true;

	if (Cmd_Argc (cmd) < 2)
		return;

	// note this uses the chat prefix \001
	if (!fromServer)
		dpsnprintf (text, sizeof(text), "\001%s tells you: ", host_client->name);
	else if(*(sv_adminnick.string))
		dpsnprintf (text, sizeof(text), "\001<%s tells you> ", sv_adminnick.string);
	else
		dpsnprintf (text, sizeof(text), "\001<%s tells you> ", hostname.string);

	p1 = Cmd_Args(cmd);
	p2 = p1 + strlen(p1);
	// remove the target name
	while (p1 < p2 && *p1 == ' ')
		p1++;
	if(*p1 == '#')
	{
		++p1;
		while (p1 < p2 && *p1 == ' ')
			p1++;
		while (p1 < p2 && isdigit(*p1))
		{
			playernumber = playernumber * 10 + (*p1 - '0');
			p1++;
		}
		--playernumber;
	}
	else if(*p1 == '"')
	{
		++p1;
		playername_start = p1;
		while (p1 < p2 && *p1 != '"')
			p1++;
		playername_length = p1 - playername_start;
		if(p1 < p2)
			p1++;
	}
	else
	{
		playername_start = p1;
		while (p1 < p2 && *p1 != ' ')
			p1++;
		playername_length = p1 - playername_start;
	}
	while (p1 < p2 && *p1 == ' ')
		p1++;
	if(playername_start)
	{
		// set playernumber to the right client
		char namebuf[128];
		if(playername_length >= sizeof(namebuf))
		{
			if (fromServer)
				Con_Print("Host_Tell: too long player name/ID\n");
			else
				SV_ClientPrint("Host_Tell: too long player name/ID\n");
			return;
		}
		memcpy(namebuf, playername_start, playername_length);
		namebuf[playername_length] = 0;
		for (playernumber = 0; playernumber < svs.maxclients; playernumber++)
		{
			if (!svs.clients[playernumber].active)
				continue;
			if (strcasecmp(svs.clients[playernumber].name, namebuf) == 0)
				break;
		}
	}
	if(playernumber < 0 || playernumber >= svs.maxclients || !(svs.clients[playernumber].active))
	{
		if (fromServer)
			Con_Print("Host_Tell: invalid player name/ID\n");
		else
			SV_ClientPrint("Host_Tell: invalid player name/ID\n");
		return;
	}
	// remove trailing newlines
	while (p2 > p1 && (p2[-1] == '\n' || p2[-1] == '\r'))
		p2--;
	// remove quotes if present
	if (*p1 == '"')
	{
		p1++;
		if (p2[-1] == '"')
			p2--;
		else if (fromServer)
			Con_Print("Host_Tell: missing end quote\n");
		else
			SV_ClientPrint("Host_Tell: missing end quote\n");
	}
	while (p2 > p1 && (p2[-1] == '\n' || p2[-1] == '\r'))
		p2--;
	if(p1 == p2)
		return; // empty say
	for (j = (int)strlen(text);j < (int)(sizeof(text) - 2) && p1 < p2;)
		text[j++] = *p1++;
	text[j++] = '\n';
	text[j++] = 0;

	save = host_client;
	host_client = svs.clients + playernumber;
	SV_ClientPrint(text);
	host_client = save;
}

/*
==================
SV_Ping_f

==================
*/
static void SV_Ping_f(cmd_state_t *cmd)
{
	int i;
	client_t *client;
	void (*print) (const char *fmt, ...);

	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!sv.active)
		return;

	print("Client ping times:\n");
	for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
	{
		if (!client->active)
			continue;
		print("%4i %s\n", bound(0, (int)floor(client->ping*1000+0.5), 9999), client->name);
	}
}

/*
====================
SV_Pings_f

Send back ping and packet loss update for all current players to this player
====================
*/
static void SV_Pings_f(cmd_state_t *cmd)
{
	int		i, j, ping, packetloss, movementloss;
	char temp[128];

	if (!host_client->netconnection)
		return;

	if (sv.protocol != PROTOCOL_QUAKEWORLD)
	{
		MSG_WriteByte(&host_client->netconnection->message, svc_stufftext);
		MSG_WriteUnterminatedString(&host_client->netconnection->message, "pingplreport");
	}
	for (i = 0;i < svs.maxclients;i++)
	{
		packetloss = 0;
		movementloss = 0;
		if (svs.clients[i].netconnection)
		{
			for (j = 0;j < NETGRAPH_PACKETS;j++)
				if (svs.clients[i].netconnection->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET)
					packetloss++;
			for (j = 0;j < NETGRAPH_PACKETS;j++)
				if (svs.clients[i].movement_count[j] < 0)
					movementloss++;
		}
		packetloss = (packetloss * 100 + NETGRAPH_PACKETS - 1) / NETGRAPH_PACKETS;
		movementloss = (movementloss * 100 + NETGRAPH_PACKETS - 1) / NETGRAPH_PACKETS;
		ping = (int)floor(svs.clients[i].ping*1000+0.5);
		ping = bound(0, ping, 9999);
		if (sv.protocol == PROTOCOL_QUAKEWORLD)
		{
			// send qw_svc_updateping and qw_svc_updatepl messages
			MSG_WriteByte(&host_client->netconnection->message, qw_svc_updateping);
			MSG_WriteShort(&host_client->netconnection->message, ping);
			MSG_WriteByte(&host_client->netconnection->message, qw_svc_updatepl);
			MSG_WriteByte(&host_client->netconnection->message, packetloss);
		}
		else
		{
			// write the string into the packet as multiple unterminated strings to avoid needing a local buffer
			if(movementloss)
				dpsnprintf(temp, sizeof(temp), " %d %d,%d", ping, packetloss, movementloss);
			else
				dpsnprintf(temp, sizeof(temp), " %d %d", ping, packetloss);
			MSG_WriteUnterminatedString(&host_client->netconnection->message, temp);
		}
	}
	if (sv.protocol != PROTOCOL_QUAKEWORLD)
		MSG_WriteString(&host_client->netconnection->message, "\n");
}

/*
==================
SV_Status_f
==================
*/
static void SV_Status_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char qcstatus[256];
	client_t *client;
	int seconds = 0, minutes = 0, hours = 0, i, j, k, in, players, ping = 0, packetloss = 0;
	void (*print) (const char *fmt, ...);
	char ip[48]; // can contain a full length v6 address with [] and a port
	int frags;
	char vabuf[1024];

	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!sv.active)
		return;

	in = 0;
	if (Cmd_Argc(cmd) == 2)
	{
		if (strcmp(Cmd_Argv(cmd, 1), "1") == 0)
			in = 1;
		else if (strcmp(Cmd_Argv(cmd, 1), "2") == 0)
			in = 2;
	}

	for (players = 0, i = 0;i < svs.maxclients;i++)
		if (svs.clients[i].active)
			players++;

	print ("host:     %s\n", Cvar_VariableString (&cvars_all, "hostname", CF_SERVER));
	print ("version:  %s\n", engineversion);
	print ("protocol: %i (%s)\n", Protocol_NumberForEnum(sv.protocol), Protocol_NameForEnum(sv.protocol));
	print ("map:      %s\n", sv.worldbasename);
	print ("timing:   %s\n", SV_TimingReport(vabuf, sizeof(vabuf)));
	print ("players:  %i active (%i max)\n\n", players, svs.maxclients);

	if (in == 1)
		print ("^2IP                                             %%pl ping  time   frags  no   name\n");
	else if (in == 2)
		print ("^5IP                                              no   name\n");

	for (i = 0, k = 0, client = svs.clients;i < svs.maxclients;i++, client++)
	{
		if (!client->active)
			continue;

		++k;

		if (in == 0 || in == 1)
		{
			seconds = (int)(host.realtime - client->connecttime);
			minutes = seconds / 60;
			if (minutes)
			{
				seconds -= (minutes * 60);
				hours = minutes / 60;
				if (hours)
					minutes -= (hours * 60);
			}
			else
				hours = 0;
			
			packetloss = 0;
			if (client->netconnection)
				for (j = 0;j < NETGRAPH_PACKETS;j++)
					if (client->netconnection->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET)
						packetloss++;
			packetloss = (packetloss * 100 + NETGRAPH_PACKETS - 1) / NETGRAPH_PACKETS;
			ping = bound(0, (int)floor(client->ping*1000+0.5), 9999);
		}

		if(sv_status_privacy.integer && cmd->source != src_local && LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
			dp_strlcpy(ip, client->netconnection ? "hidden" : "botclient", 48);
		else
			dp_strlcpy(ip, (client->netconnection && *client->netconnection->address) ? client->netconnection->address : "botclient", 48);

		frags = client->frags;

		if(sv_status_show_qcstatus.integer)
		{
			prvm_edict_t *ed = PRVM_EDICT_NUM(i + 1);
			const char *str = PRVM_GetString(prog, PRVM_serveredictstring(ed, clientstatus));
			if(str && *str)
			{
				char *p;
				const char *q;
				p = qcstatus;
				for(q = str; *q && p != qcstatus + sizeof(qcstatus) - 1; ++q)
					if(*q != '\\' && *q != '"' && !ISWHITESPACE(*q))
						*p++ = *q;
				*p = 0;
				if(*qcstatus)
					frags = atoi(qcstatus);
			}
		}
		
		if (in == 0) // default layout
		{
			if (sv.protocol == PROTOCOL_QUAKE && svs.maxclients <= 99)
			{
				// LadyHavoc: this is very touchy because we must maintain ProQuake compatible status output
				print ("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", i+1, client->name, frags, hours, minutes, seconds);
				print ("   %s\n", ip);
			}
			else
			{
				// LadyHavoc: no real restrictions here, not a ProQuake-compatible protocol anyway...
				print ("#%-3u %-16.16s %4i  %2i:%02i:%02i\n", i+1, client->name, frags, hours, minutes, seconds);
				print ("   %s\n", ip);
			}
		}
		else if (in == 1) // extended layout
		{
			print ("%s%-47s %2i %4i %2i:%02i:%02i %4i  #%-3u ^7%s\n", k%2 ? "^3" : "^7", ip, packetloss, ping, hours, minutes, seconds, frags, i+1, client->name);
		}
		else if (in == 2) // reduced layout
		{
			print ("%s%-47s #%-3u ^7%s\n", k%2 ? "^3" : "^7", ip, i+1, client->name);
		}
	}
}

void SV_Name(int clientnum)
{
	prvm_prog_t *prog = SVVM_prog;
	PRVM_serveredictstring(host_client->edict, netname) = PRVM_SetEngineString(prog, host_client->name);
	if (strcmp(host_client->old_name, host_client->name))
	{
		if (host_client->begun)
			SV_BroadcastPrintf("\003%s ^7changed name to ^3%s\n", host_client->old_name, host_client->name);
		dp_strlcpy(host_client->old_name, host_client->name, sizeof(host_client->old_name));
		// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
		MSG_WriteByte (&sv.reliable_datagram, clientnum);
		MSG_WriteString (&sv.reliable_datagram, host_client->name);
		SV_WriteNetnameIntoDemo(host_client);
	}	
}

/*
======================
SV_Name_f
======================
*/
static void SV_Name_f(cmd_state_t *cmd)
{
	int i, j;
	qbool valid_colors;
	const char *newNameSource;
	char newName[sizeof(host_client->name)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		newNameSource = Cmd_Argv(cmd, 1);
	else
		newNameSource = Cmd_Args(cmd);

	dp_strlcpy(newName, newNameSource, sizeof(newName));

	if (cmd->source == src_local)
		return;

	if (host.realtime < host_client->nametime && strcmp(newName, host_client->name))
	{
		SV_ClientPrintf("You can't change name more than once every %.1f seconds!\n", max(0.0f, sv_namechangetimer.value));
		return;
	}

	host_client->nametime = host.realtime + max(0.0f, sv_namechangetimer.value);

	// point the string back at updateclient->name to keep it safe
	dp_strlcpy (host_client->name, newName, sizeof (host_client->name));

	for (i = 0, j = 0;host_client->name[i];i++)
		if (host_client->name[i] != '\r' && host_client->name[i] != '\n')
			host_client->name[j++] = host_client->name[i];
	host_client->name[j] = 0;

	if(host_client->name[0] == 1 || host_client->name[0] == 2)
	// may interfere with chat area, and will needlessly beep; so let's add a ^7
	{
		memmove(host_client->name + 2, host_client->name, sizeof(host_client->name) - 2);
		host_client->name[sizeof(host_client->name) - 1] = 0;
		host_client->name[0] = STRING_COLOR_TAG;
		host_client->name[1] = '0' + STRING_COLOR_DEFAULT;
	}

	u8_COM_StringLengthNoColors(host_client->name, 0, &valid_colors);
	if(!valid_colors) // NOTE: this also proves the string is not empty, as "" is a valid colored string
	{
		size_t l;
		l = strlen(host_client->name);
		if(l < sizeof(host_client->name) - 1)
		{
			// duplicate the color tag to escape it
			host_client->name[i] = STRING_COLOR_TAG;
			host_client->name[i+1] = 0;
			//Con_DPrintf("abuse detected, adding another trailing color tag\n");
		}
		else
		{
			// remove the last character to fix the color code
			host_client->name[l-1] = 0;
			//Con_DPrintf("abuse detected, removing a trailing color tag\n");
		}
	}

	// find the last color tag offset and decide if we need to add a reset tag
	for (i = 0, j = -1;host_client->name[i];i++)
	{
		if (host_client->name[i] == STRING_COLOR_TAG)
		{
			if (host_client->name[i+1] >= '0' && host_client->name[i+1] <= '9')
			{
				j = i;
				// if this happens to be a reset  tag then we don't need one
				if (host_client->name[i+1] == '0' + STRING_COLOR_DEFAULT)
					j = -1;
				i++;
				continue;
			}
			if (host_client->name[i+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(host_client->name[i+2]) && isxdigit(host_client->name[i+3]) && isxdigit(host_client->name[i+4]))
			{
				j = i;
				i += 4;
				continue;
			}
			if (host_client->name[i+1] == STRING_COLOR_TAG)
			{
				i++;
				continue;
			}
		}
	}
	// does not end in the default color string, so add it
	if (j >= 0 && strlen(host_client->name) < sizeof(host_client->name) - 2)
		memcpy(host_client->name + strlen(host_client->name), STRING_COLOR_DEFAULT_STR, strlen(STRING_COLOR_DEFAULT_STR) + 1);

	SV_Name(host_client - svs.clients);
}

static void SV_Rate_f(cmd_state_t *cmd)
{
	int rate;

	rate = atoi(Cmd_Argv(cmd, 1));

	if (cmd->source == src_local)
		return;

	host_client->rate = rate;
}

static void SV_Rate_BurstSize_f(cmd_state_t *cmd)
{
	int rate_burstsize;

	if (Cmd_Argc(cmd) != 2)
		return;

	rate_burstsize = atoi(Cmd_Argv(cmd, 1));

	host_client->rate_burstsize = rate_burstsize;
}

static void SV_Color_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int top, bottom, playercolor;

	top = atoi(Cmd_Argv(cmd, 1));
	bottom = atoi(Cmd_Argv(cmd, 2));

	top &= 15;
	bottom &= 15;

	playercolor = top*16 + bottom;

	if (host_client->edict && PRVM_serverfunction(SV_ChangeTeam))
	{
		Con_DPrint("Calling SV_ChangeTeam\n");
		prog->globals.fp[OFS_PARM0] = playercolor;
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SV_ChangeTeam), "QC function SV_ChangeTeam is missing");
	}
	else
	{
		if (host_client->edict)
		{
			PRVM_serveredictfloat(host_client->edict, clientcolors) = playercolor;
			PRVM_serveredictfloat(host_client->edict, team) = bottom + 1;
		}
		host_client->colors = playercolor;
		if (host_client->old_colors != host_client->colors)
		{
			host_client->old_colors = host_client->colors;
			// send notification to all clients
			MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
			MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
			MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
		}
	}
}

/*
==================
SV_Kick_f

Kicks a user off of the server
==================
*/
static void SV_Kick_f(cmd_state_t *cmd)
{
	const char *who;
	const char *message = NULL;
	char reason[512];
	client_t *save;
	int i;
	qbool byNumber = false;

	if (!sv.active)
		return;

	save = host_client;

	if (Cmd_Argc(cmd) > 2 && strcmp(Cmd_Argv(cmd, 1), "#") == 0)
	{
		i = (int)(atof(Cmd_Argv(cmd, 2)) - 1);
		if (i < 0 || i >= svs.maxclients || !(host_client = svs.clients + i)->active)
			return;
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
		{
			if (!host_client->active)
				continue;
			if (strcasecmp(host_client->name, Cmd_Argv(cmd, 1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd->source == src_local)
		{
			if(!host_isclient.integer)
				who = "Console";
			else
				who = cl_name.string;
		}
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc(cmd) > 2)
		{
			message = Cmd_Args(cmd);
			COM_ParseToken_Simple(&message, false, false, true);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += strlen(Cmd_Argv(cmd, 2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_DropClient (false, va(reason, sizeof(reason), "Kicked by %s: %s", who, message)); // kicked
			//SV_ClientPrintf("Kicked by %s: %s\n", who, message);
		else
			//SV_ClientPrintf("Kicked by %s\n", who);
			SV_DropClient (false, va(reason, sizeof(reason), "Kicked by %s", who)); // kicked
	}

	host_client = save;
}

static void SV_MaxPlayers_f(cmd_state_t *cmd)
{
	int n;

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Printf("\"maxplayers\" is \"%u\"\n", svs.maxclients_next);
		return;
	}

	if (sv.active)
	{
		Con_Print("maxplayers can not be changed while a server is running.\n");
		Con_Print("It will be changed on next server startup (\"map\" command).\n");
	}

	n = atoi(Cmd_Argv(cmd, 1));
	n = bound(1, n, MAX_SCOREBOARD);
	Con_Printf("\"maxplayers\" set to \"%u\"\n", n);

	svs.maxclients_next = n;
	if (n == 1)
		Cvar_Set (&cvars_all, "deathmatch", "0");
	else
		Cvar_Set (&cvars_all, "deathmatch", "1");
}

/*
======================
SV_Playermodel_f
======================
*/
// the old playermodel in cl_main has been renamed to __cl_playermodel
static void SV_Playermodel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, j;
	char newPath[sizeof(host_client->playermodel)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		dp_strlcpy (newPath, Cmd_Argv(cmd, 1), sizeof (newPath));
	else
		dp_strlcpy (newPath, Cmd_Args(cmd), sizeof (newPath));

	for (i = 0, j = 0;newPath[i];i++)
		if (newPath[i] != '\r' && newPath[i] != '\n')
			newPath[j++] = newPath[i];
	newPath[j] = 0;

	/*
	if (host.realtime < host_client->nametime)
	{
		SV_ClientPrintf("You can't change playermodel more than once every 5 seconds!\n");
		return;
	}

	host_client->nametime = host.realtime + 5;
	*/

	// point the string back at updateclient->name to keep it safe
	dp_strlcpy (host_client->playermodel, newPath, sizeof (host_client->playermodel));
	PRVM_serveredictstring(host_client->edict, playermodel) = PRVM_SetEngineString(prog, host_client->playermodel);
	if (strcmp(host_client->old_model, host_client->playermodel))
	{
		dp_strlcpy(host_client->old_model, host_client->playermodel, sizeof(host_client->old_model));
		/*// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatepmodel);
		MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
		MSG_WriteString (&sv.reliable_datagram, host_client->playermodel);*/
	}
}

/*
======================
SV_Playerskin_f
======================
*/
static void SV_Playerskin_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, j;
	char newPath[sizeof(host_client->playerskin)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		dp_strlcpy (newPath, Cmd_Argv(cmd, 1), sizeof (newPath));
	else
		dp_strlcpy (newPath, Cmd_Args(cmd), sizeof (newPath));

	for (i = 0, j = 0;newPath[i];i++)
		if (newPath[i] != '\r' && newPath[i] != '\n')
			newPath[j++] = newPath[i];
	newPath[j] = 0;

	/*
	if (host.realtime < host_client->nametime)
	{
		SV_ClientPrintf("You can't change playermodel more than once every 5 seconds!\n");
		return;
	}

	host_client->nametime = host.realtime + 5;
	*/

	// point the string back at updateclient->name to keep it safe
	dp_strlcpy (host_client->playerskin, newPath, sizeof (host_client->playerskin));
	PRVM_serveredictstring(host_client->edict, playerskin) = PRVM_SetEngineString(prog, host_client->playerskin);
	if (strcmp(host_client->old_skin, host_client->playerskin))
	{
		//if (host_client->begun)
		//	SV_BroadcastPrintf("%s changed skin to %s\n", host_client->name, host_client->playerskin);
		dp_strlcpy(host_client->old_skin, host_client->playerskin, sizeof(host_client->old_skin));
		/*// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatepskin);
		MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
		MSG_WriteString (&sv.reliable_datagram, host_client->playerskin);*/
	}
}

/*
======================
SV_PModel_f
LadyHavoc: only supported for Nehahra, I personally think this is dumb, but Mindcrime won't listen.
LadyHavoc: correction, Mindcrime will be removing pmodel in the future, but it's still stuck here for compatibility.
======================
*/
static void SV_PModel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	if (Cmd_Argc (cmd) == 1)
		return;

	PRVM_serveredictfloat(host_client->edict, pmodel) = atoi(Cmd_Argv(cmd, 1));
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

static prvm_edict_t	*FindViewthing(prvm_prog_t *prog)
{
	int		i;
	prvm_edict_t	*e;

	for (i=0 ; i<prog->num_edicts ; i++)
	{
		e = PRVM_EDICT_NUM(i);
		if (!strcmp (PRVM_GetString(prog, PRVM_serveredictstring(e, classname)), "viewthing"))
			return e;
	}
	Con_Print("No viewthing on map\n");
	return NULL;
}

/*
==================
SV_Viewmodel_f
==================
*/
static void SV_Viewmodel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = Mod_ForName (Cmd_Argv(cmd, 1), false, true, NULL);
		if (m && m->loaded && m->Draw)
		{
			PRVM_serveredictfloat(e, frame) = 0;
			cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)] = m;
		}
		else
			Con_Printf("viewmodel: can't load %s\n", Cmd_Argv(cmd, 1));
	}
}

/*
==================
SV_Viewframe_f
==================
*/
static void SV_Viewframe_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	int		f;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		f = atoi(Cmd_Argv(cmd, 1));
		if (f >= m->numframes)
			f = m->numframes-1;

		PRVM_serveredictfloat(e, frame) = f;
	}
}

static void PrintFrameName (model_t *m, int frame)
{
	if (m->animscenes)
		Con_Printf("frame %i: %s\n", frame, m->animscenes[frame].name);
	else
		Con_Printf("frame %i\n", frame);
}

/*
==================
SV_Viewnext_f
==================
*/
static void SV_Viewnext_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		PRVM_serveredictfloat(e, frame) = PRVM_serveredictfloat(e, frame) + 1;
		if (PRVM_serveredictfloat(e, frame) >= m->numframes)
			PRVM_serveredictfloat(e, frame) = m->numframes - 1;

		PrintFrameName (m, (int)PRVM_serveredictfloat(e, frame));
	}
}

/*
==================
SV_Viewprev_f
==================
*/
static void SV_Viewprev_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		PRVM_serveredictfloat(e, frame) = PRVM_serveredictfloat(e, frame) - 1;
		if (PRVM_serveredictfloat(e, frame) < 0)
			PRVM_serveredictfloat(e, frame) = 0;

		PrintFrameName (m, (int)PRVM_serveredictfloat(e, frame));
	}
}

static void SV_SendCvar_f(cmd_state_t *cmd)
{
	int i;	
	const char *cvarname;
	client_t *old;
	
	if(Cmd_Argc(cmd) != 2)
		return;

	if(!sv.active)// || !PRVM_serverfunction(SV_ParseClientCommand))
		return;

	cvarname = Cmd_Argv(cmd, 1);

	old = host_client;
	if(host_isclient.integer)
		i = 1;
	else
		i = 0;
	for(;i<svs.maxclients;i++)
		if(svs.clients[i].active && svs.clients[i].netconnection)
		{
			host_client = &svs.clients[i];
			SV_ClientCommands("sendcvar %s\n", cvarname);
		}
	host_client = old;
}

static void SV_Ent_Create_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ed;
	mdef_t *key;
	int i;
	qbool haveorigin;

	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	if(!Cmd_Argc(cmd))
		return;

	ed = PRVM_ED_Alloc(SVVM_prog);

	PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "classname"), Cmd_Argv(cmd, 1), false);

	// Spawn where the player is aiming. We need a view matrix first.
	if(cmd->source == src_client)
	{
		vec3_t org, temp, dest;
		matrix4x4_t view;
		trace_t trace;
		char buf[128];

		SV_GetEntityMatrix(prog, host_client->edict, &view, true);

		Matrix4x4_OriginFromMatrix(&view, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&view, temp, dest);		

		trace = SV_TraceLine(org, dest, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value);

		dpsnprintf(buf, sizeof(buf), "%g %g %g", trace.endpos[0], trace.endpos[1], trace.endpos[2]);
		PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "origin"), buf, false);

		haveorigin = true;
	}
	// Or spawn at a specified origin.
	else
	{
		print = Con_Printf;
		haveorigin = false;
	}

	// Allow more than one key/value pair by cycling between expecting either one.
	for(i = 2; i < Cmd_Argc(cmd); i += 2)
	{
		if(!(key = PRVM_ED_FindField(prog, Cmd_Argv(cmd, i))))
		{
			print("Key %s not found!\n", Cmd_Argv(cmd, i));
			PRVM_ED_Free(prog, ed);
			return;
		}

		/*
		 * This is mostly for dedicated server console, but if the
		 * player gave a custom origin, we can ignore the traceline.
		 */
		if(!strcmp(Cmd_Argv(cmd, i), "origin"))
			haveorigin = true;

		if (i + 1 < Cmd_Argc(cmd))
			PRVM_ED_ParseEpair(prog, ed, key, Cmd_Argv(cmd, i+1), false);
	}

	if(!haveorigin)
	{
		print("Missing origin\n");
		PRVM_ED_Free(prog, ed);
		return;
	}

	// Spawn it
	PRVM_ED_CallPrespawnFunction(prog, ed);
	
	if(!PRVM_ED_CallSpawnFunction(prog, ed, NULL, NULL))
	{
		print("Could not spawn a \"%s\". No such entity or it has no spawn function\n", Cmd_Argv(cmd, 1));
		if(cmd->source == src_client)
			Con_Printf("%s tried to spawn a \"%s\"\n", host_client->name, Cmd_Argv(cmd, 1));
		// CallSpawnFunction already freed the edict for us.
		return;
	}

	PRVM_ED_CallPostspawnFunction(prog, ed);	

	// Make it appear in the world
	SV_LinkEdict(ed);

	if(cmd->source == src_client)
		Con_Printf("%s spawned a \"%s\"\n", host_client->name, Cmd_Argv(cmd, 1));
}

static void SV_Ent_Remove_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ed;
	int i, ednum = 0;
	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	if(!Cmd_Argc(cmd))
		return;

	// Allow specifying edict by number
	if(Cmd_Argc(cmd) > 1 && Cmd_Argv(cmd, 1))
	{
		ednum = atoi(Cmd_Argv(cmd, 1));
		if(!ednum)
		{
			print("Cannot remove the world\n");
			return;
		}
	}
	// Or trace a line if it's a client who didn't specify one.
	else if(cmd->source == src_client)
	{
		vec3_t org, temp, dest;
		matrix4x4_t view;
		trace_t trace;

		SV_GetEntityMatrix(prog, host_client->edict, &view, true);

		Matrix4x4_OriginFromMatrix(&view, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&view, temp, dest);		

		trace = SV_TraceLine(org, dest, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY, 0, 0, collision_extendmovelength.value);
		
		if(trace.ent)
			ednum = (int)PRVM_EDICT_TO_PROG(trace.ent);
		if(!trace.ent || !ednum)
			// Don't remove the world, but don't annoy players with a print if they miss
			return;
	}
	else
	{
		// Only a dedicated server console should be able to reach this.
		print("No edict given\n");
		return;
	}

	ed = PRVM_EDICT_NUM(ednum);

	if(ed)
	{
		// Skip players
		for (i = 0; i < svs.maxclients; i++)
		{
			if(ed == svs.clients[i].edict)
				return;
		}

		if(!ed->free)
		{
			print("Removed a \"%s\"\n", PRVM_GetString(prog, PRVM_serveredictstring(ed, classname)));
			PRVM_ED_ClearEdict(prog, ed);
			PRVM_ED_Free(prog, ed);
		}
	}
	else
	{
		// This should only be reachable if an invalid edict number was given
		print("No such entity\n");
		return;
	}
}

static void SV_Ent_Remove_All_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, rmcount;
	prvm_edict_t *ed;
	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	for (i = 0, rmcount = 0, ed = PRVM_EDICT_NUM(i); i < prog->num_edicts; i++, ed = PRVM_NEXT_EDICT(ed))
	{
		if(!ed->free && !strcmp(PRVM_GetString(prog, PRVM_serveredictstring(ed, classname)), Cmd_Argv(cmd, 1)))
		{
			if(!i)
			{
				print("Cannot remove the world\n");
				return;
			}
			PRVM_ED_ClearEdict(prog, ed);
			PRVM_ED_Free(prog, ed);
			rmcount++;
		}
	}

	if(!rmcount)
		print("No \"%s\" found\n", Cmd_Argv(cmd, 1));
	else
		print("Removed %i of \"%s\"\n", rmcount, Cmd_Argv(cmd, 1));
}

void SV_InitOperatorCommands(void)
{
	Cvar_RegisterVariable(&sv_cheats);
	Cvar_RegisterCallback(&sv_cheats, SV_DisableCheats_c);
	Cvar_RegisterVariable(&sv_adminnick);
	Cvar_RegisterVariable(&sv_status_privacy);
	Cvar_RegisterVariable(&sv_status_show_qcstatus);
	Cvar_RegisterVariable(&sv_namechangetimer);
	
	Cmd_AddCommand(CF_SERVER | CF_SERVER_FROM_CLIENT, "status", SV_Status_f, "print server status information");
	Cmd_AddCommand(CF_SHARED, "map", SV_Map_f, "kick everyone off the server and start a new level");
	Cmd_AddCommand(CF_SHARED, "restart", SV_Restart_f, "restart current level");
	Cmd_AddCommand(CF_SHARED, "changelevel", SV_Changelevel_f, "change to another level, bringing along all connected clients (or start a new level if none is loaded)");
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "say", SV_Say_f, "send a chat message to everyone on the server");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "say_team", SV_Say_Team_f, "send a chat message to your team on the server");
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "tell", SV_Tell_f, "send a chat message to only one person on the server");
	Cmd_AddCommand(CF_SERVER | CF_SERVER_FROM_CLIENT, "pause", SV_Pause_f, "pause the game (if the server allows pausing)");
	Cmd_AddCommand(CF_SHARED, "kick", SV_Kick_f, "kick a player off the server by number or name");
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "ping", SV_Ping_f, "print ping times of all players on the server");
	Cmd_AddCommand(CF_SHARED, "load", SV_Loadgame_f, "load a saved game file");
	Cmd_AddCommand(CF_SHARED, "save", SV_Savegame_f, "save the game to a file");
	Cmd_AddCommand(CF_SHARED, "viewmodel", SV_Viewmodel_f, "change model of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewframe", SV_Viewframe_f, "change animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewnext", SV_Viewnext_f, "change to next animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewprev", SV_Viewprev_f, "change to previous animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "maxplayers", SV_MaxPlayers_f, "sets limit on how many players (or bots) may be connected to the server at once");
	host.hook.SV_SendCvar = SV_SendCvar_f;

	// commands that do not have automatic forwarding from cmd_local, these are internal details of the network protocol and not of interest to users (if they know what they are doing they can still use a generic "cmd prespawn" or similar)
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "prespawn", SV_PreSpawn_f, "internal use - signon 1 (client acknowledges that server information has been received)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "spawn", SV_Spawn_f, "internal use - signon 2 (client has sent player information, and is asking server to send scoreboard rankings)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "begin", SV_Begin_f, "internal use - signon 3 (client asks server to start sending entities, and will go to signon 4 (playing) when the first entity update is received)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "pings", SV_Pings_f, "internal use - command sent by clients to request updated ping and packetloss of players on scoreboard (originally from QW, but also used on NQ servers)");

	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "god", SV_God_f, "god mode (invulnerability)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "notarget", SV_Notarget_f, "notarget mode (monsters do not see you)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "fly", SV_Fly_f, "fly mode (flight)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "noclip", SV_Noclip_f, "noclip mode (flight without collisions, move through walls)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "give", SV_Give_f, "alter inventory");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "kill", SV_Kill_f, "die instantly");
	
	Cmd_AddCommand(CF_USERINFO, "color", SV_Color_f, "change your player shirt and pants colors");
	Cmd_AddCommand(CF_USERINFO, "name", SV_Name_f, "change your player name");
	Cmd_AddCommand(CF_USERINFO, "rate", SV_Rate_f, "change your network connection speed");
	Cmd_AddCommand(CF_USERINFO, "rate_burstsize", SV_Rate_BurstSize_f, "change your network connection speed");
	Cmd_AddCommand(CF_USERINFO, "pmodel", SV_PModel_f, "(Nehahra-only) change your player model choice");
	Cmd_AddCommand(CF_USERINFO, "playermodel", SV_Playermodel_f, "change your player model");
	Cmd_AddCommand(CF_USERINFO, "playerskin", SV_Playerskin_f, "change your player skin number");

	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_create", SV_Ent_Create_f, "Creates an entity at the specified coordinate, of the specified classname. If executed from a server, origin has to be specified manually.");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_remove_all", SV_Ent_Remove_All_f, "Removes all entities of the specified classname");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_remove", SV_Ent_Remove_f, "Removes an entity by number, or the entity you're aiming at");
}
