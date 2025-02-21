/*

Copyright (C) 2001-2002       A Nourai

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


#include "quakedef.h"
#include "cl_ignore.h"

#include <ctype.h>

#define MAX_TEAMIGNORELIST	4
#define	FLOODLIST_SIZE		10

int Player_IdtoSlot (int id)
{
	int j;

	for (j = 0; j < cl.allocated_client_slots; j++)
	{
		if (cl.players[j].name[0] && cl.players[j].userid == id)
			return j;
	}
	return -1;
}

int Player_StringtoSlot(const char *arg)
{
	int i, slot;

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (cl.players[i].name[0] && !strncmp(arg, cl.players[i].name, MAX_SCOREBOARDNAME - 1))
			return i;
	}

	if (!arg[0])
		return PLAYER_NAME_NOMATCH;

	for (i = 0; arg[i]; i++)
	{
		if (!isdigit(arg[i]))
			return PLAYER_NAME_NOMATCH;
	}
	return ((slot = Player_IdtoSlot(Q_atoi(arg))) >= 0) ? slot : PLAYER_ID_NOMATCH;
}

int Player_NametoSlot(const char *name)
{
	int i;

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (cl.players[i].name[0] && !strncmp(cl.players[i].name, name, MAX_SCOREBOARDNAME - 1))
			return i;
	}
	return PLAYER_NAME_NOMATCH;
}

int Player_SlottoId (int slot)
{	
	return (slot >= 0 && slot < cl.allocated_client_slots && cl.players[slot].name[0]) ? cl.players[slot].userid : -1;
}

char *Player_MyName (void)
{
	return cl.players[cl.playerview[0].playernum].name;
}








cvar_t		ignore_spec				= CVARD("ignore_spec", "0", "0: Never ignore spectators.\n1: Ignore spectators only when playing.\n2: Always ignore spectators even when spectating.");
cvar_t		ignore_qizmo_spec		= CVAR("ignore_qizmo_spec", "0");
cvar_t		ignore_mode				= CVAR("ignore_mode", "0");
cvar_t		ignore_flood_duration	= CVARD("ignore_flood_duration", "4", "Time limit for inbound messages to be considered duplicates.");
cvar_t		ignore_flood			= CVARD("ignore_flood", "0", "Provides a way to reduce inbound spam from flooding out your chat (dupe messages are ignored).\n0: No inbound flood protection.\n1: Duplicate non-team messages will be filtered.\n2: ALL duplicate messages will be filtered");
cvar_t		ignore_opponents		= CVARD("ignore_opponents", "0", "0: Don't ignore chat from enemies.\n1: Always ignore chat from opponents (note: can also ignore f_ruleset checks).\n2: Ignore chat from opponents only during a match (requires servers that actually reports match state).\n");

char ignoreteamlist[MAX_TEAMIGNORELIST][16 + 1];

typedef struct flood_s
{
	int playernum;
	char data[2048];
	float time;
} flood_t;

static flood_t floodlist[FLOODLIST_SIZE];
static int		floodindex;

extern int PaddedPrint (char *s, int x);

static qboolean IsIgnored(int slot)
{
	return cl.players[slot].ignored;
}

static void Display_Ignorelist(void)
{
	int i;
	int x;
	qboolean foundone;
	playerview_t *pv = &cl.playerview[0];

	x = 0;
	foundone = false;
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (cl.players[i].name[0] && cl.players[i].ignored)
		{
			if (!foundone)
			{
				Con_Printf ("\x02" "User Ignore List:\n");
				foundone++;
			}
			x = PaddedPrint(cl.players[i].name, x);
		}
	}
	if (!foundone)
		Con_Printf("\x02" "User Ignore List: empty\n");
	else if (x)
		Con_Printf ("\n");

	x = 0;
	foundone = false;
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (cl.players[i].name[0] && cl.players[i].ignored)
		{
			if (!foundone)
			{
				Con_Printf ("\x02" "User Mute List:\n");
				foundone++;
			}
			x = PaddedPrint(cl.players[i].name, x);
		}
	}
	if (!foundone)
		Con_Printf("\x02" "User Mute List: empty\n");
	else if (x)
		Con_Printf ("\n");
	
	if (ignoreteamlist[0][0])
	{
		x = 0;
		Con_Printf ("\x02" "Team Ignore List:\n");
		for (i = 0; i < MAX_TEAMIGNORELIST && ignoreteamlist[i][0]; i++)
			x = PaddedPrint(ignoreteamlist[i], x);
		if (x)
			Con_Printf ("\n");
	}

	if (ignore_opponents.ival)
		Con_Printf("\x02" "Opponents are Ignored\n");

	if (ignore_spec.ival == 2 || (ignore_spec.ival == 1 && !pv->spectator))
		Con_Printf ("\x02" "Spectators are Ignored\n");

	if (ignore_qizmo_spec.ival)
		Con_Printf("\x02" "Qizmo spectators are Ignored\n");

	Con_Printf("\n");
}

static qboolean Ignorelist_Add(int slot)
{
	if (IsIgnored(slot))
		return false;

	cl.players[slot].ignored = true;
	cl.players[slot].vignored = true;
	S_Voip_Ignore(slot, true);
	return true;
}
static qboolean Ignorelist_VAdd(int slot)
{
	if (cl.players[slot].vignored)
		return false;

	cl.players[slot].vignored = true;
	S_Voip_Ignore(slot, true);
	return true;
}
static qboolean Ignorelist_VDel(int slot)
{
	if (!cl.players[slot].vignored)
		return false;

	cl.players[slot].vignored = false;
	S_Voip_Ignore(slot, false);
	return true;
}

static qboolean Ignorelist_Del(int slot)
{
	if (cl.players[slot].ignored == false)
		return false;

	cl.players[slot].ignored = false;
	cl.players[slot].vignored = true;
	S_Voip_Ignore(slot, false);
	return true;
}

static void VIgnore_f(void)
{
	int c, slot;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid | name]\n", Cmd_Argv(0));
		return;
	}

	if ((slot = Player_StringtoSlot(Cmd_Argv(1))) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), Q_atoi(Cmd_Argv(1)));
	}
	else if (slot == PLAYER_NAME_NOMATCH)
	{
		Con_Printf("%s : no player with name %s\n", Cmd_Argv(0), Cmd_Argv(1));
	}
	else
	{
		if (Ignorelist_VAdd(slot))
			Con_Printf("Added user %s to mute list\n", cl.players[slot].name);
		else
			Con_Printf ("User %s is already mute\n", cl.players[slot].name);
	}
}
static void VUnignore_f(void)
{
	int c, slot;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid | name]\n", Cmd_Argv(0));
		return;
	}

	if ((slot = Player_StringtoSlot(Cmd_Argv(1))) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), Q_atoi(Cmd_Argv(1)));
	}
	else if (slot == PLAYER_NAME_NOMATCH)
	{
		Con_Printf("%s : no player with name %s\n", Cmd_Argv(0), Cmd_Argv(1));
	}
	else
	{
		if (Ignorelist_VDel(slot))
			Con_Printf("Removed user %s from mute list\n", cl.players[slot].name);
		else
			Con_Printf ("User %s already wasn't muted\n", cl.players[slot].name);
	}
}

static void Ignore_f(void)
{
	int c, slot;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid | name]\n", Cmd_Argv(0));
		return;
	}

	if ((slot = Player_StringtoSlot(Cmd_Argv(1))) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), Q_atoi(Cmd_Argv(1)));
	}
	else if (slot == PLAYER_NAME_NOMATCH)
	{
		Con_Printf("%s : no player with name %s\n", Cmd_Argv(0), Cmd_Argv(1));
	}
	else
	{
		if (Ignorelist_Add(slot))
			Con_Printf("Added user %s to ignore list\n", cl.players[slot].name);
		else
			Con_Printf ("User %s is already ignored\n", cl.players[slot].name);
	}
}

static void IgnoreList_f(void)
{
	if (Cmd_Argc() != 1)
		Con_Printf("%s : no arguments expected\n", Cmd_Argv(0));
	else
		Display_Ignorelist();
}

static void Ignore_ID_f(void)
{
	int c, userid, i, slot;
	char *arg;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid]\n", Cmd_Argv(0));
		return;
	}
	arg = Cmd_Argv(1);
	for (i = 0; arg[i]; i++)
	{
		if (!isdigit(arg[i]))
		{
			Con_Printf("Usage: %s [userid]\n", Cmd_Argv(0));
			return;
		}
	}
	userid = Q_atoi(arg);
	if ((slot = Player_IdtoSlot(userid)) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), userid);
		return;
	}
	if (Ignorelist_Add(slot))
		Con_Printf("Added user %s to ignore list\n", cl.players[slot].name);
	else
		Con_Printf ("User %s is already ignored\n", cl.players[slot].name);
}

static void Unignore_f(void)
{
	int c, slot;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid | name]\n", Cmd_Argv(0));
		return;
	}

	if ((slot = Player_StringtoSlot(Cmd_Argv(1))) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), Q_atoi(Cmd_Argv(1)));
	}
	else if (slot == PLAYER_NAME_NOMATCH)
	{
		Con_Printf("%s : no player with name %s\n", Cmd_Argv(0), Cmd_Argv(1));
	}
	else
	{		
		if (Ignorelist_Del(slot))
			Con_Printf("Removed user %s from ignore list\n", cl.players[slot].name);
		else
			Con_Printf("User %s is not being ignored\n", cl.players[slot].name);
	}
}

static void Unignore_ID_f(void)
{
	int c, i, userid, slot;
	char *arg;

	if ((c = Cmd_Argc()) == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [userid]\n", Cmd_Argv(0));
		return;
	}
	arg = Cmd_Argv(1);
	for (i = 0; arg[i]; i++)
	{
		if (!isdigit(arg[i]))
		{
			Con_Printf("Usage: %s [userid]\n", Cmd_Argv(0));
			return;
		}
	}
	userid = Q_atoi(arg);
	if ((slot = Player_IdtoSlot(userid)) == PLAYER_ID_NOMATCH)
	{
		Con_Printf("%s : no player with userid %d\n", Cmd_Argv(0), userid);
		return;
	}
	if (Ignorelist_Del(slot))
		Con_Printf("Removed user %s from ignore list\n", cl.players[slot].name);
	else
		Con_Printf("User %s is not being ignored\n", cl.players[slot].name);
}

static void Ignoreteam_f(void)
{
	int c, i, j;
	char *arg;

	c = Cmd_Argc();
	if (c == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [team]\n", Cmd_Argv(0));
		return;
	}
	arg = Cmd_Argv(1);
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (cl.players[i].name[0] && !cl.players[i].spectator && !strcmp(arg, cl.players[i].team))
		{
			for (j = 0; j < MAX_TEAMIGNORELIST && ignoreteamlist[j][0]; j++)
			{
				if (!strncmp(arg, ignoreteamlist[j], sizeof(ignoreteamlist[j]) - 1))
				{
					Con_Printf ("Team %s is already ignored\n", arg);
					return;
				}
			}
			if (j == MAX_TEAMIGNORELIST)
				Con_Printf("You cannot ignore more than %d teams\n", MAX_TEAMIGNORELIST);
			else
			{
				Q_strncpyz(ignoreteamlist[j], arg, sizeof(ignoreteamlist[j]));
				if (j + 1 < MAX_TEAMIGNORELIST)
					ignoreteamlist[j + 1][0] = 0;
				Con_Printf("Added team %s to ignore list\n", arg);
			}
			return;
		}
	}
	Con_Printf("%s : no team with name %s\n", Cmd_Argv(0), arg);
}

static void Unignoreteam_f(void)
{
	int i, c, j;
	char *arg;

	c = Cmd_Argc();
	if (c == 1)
	{
		Display_Ignorelist();
		return;
	}
	else if (c != 2)
	{
		Con_Printf("Usage: %s [team]\n", Cmd_Argv(0));
		return;
	}	
	arg = Cmd_Argv(1);
	for (i = 0; i < MAX_TEAMIGNORELIST && ignoreteamlist[i][0]; i++)
	{
		if (!strncmp(arg, ignoreteamlist[i], sizeof(ignoreteamlist[i]) - 1))
		{
			for (j = i; j < MAX_TEAMIGNORELIST && ignoreteamlist[j][0]; j++) 
				;
			if ( --j >  i)
				Q_strncpyz(ignoreteamlist[i], ignoreteamlist[j], sizeof(ignoreteamlist[i]));
			ignoreteamlist[j][0] = 0;			
			Con_Printf("Removed team %s from ignore list\n", arg);
			return;
		}
	}
	Con_Printf("Team %s is not being ignored\n", arg);
}

static void UnignoreAll_f (void)
{
	int i;

	if (Cmd_Argc() != 1)
	{
		Con_Printf("%s : no arguments expected\n", Cmd_Argv(0));
		return;
	}
	for (i = 0; i < cl.allocated_client_slots; i++)
		Ignorelist_Del(i);
	Con_Printf("User ignore list cleared\n");
}

static void UnignoreteamAll_f (void)
{
	if (Cmd_Argc() != 1)
	{
		Con_Printf("%s : no arguments expected\n", Cmd_Argv(0));
		return;
	}
	ignoreteamlist[0][0] = 0;
	Con_Printf("Team ignore list cleared\n");
}

char Ignore_Check_Flood(player_info_t *sender, const char *s, int flags)
{
	int i;
	int slot;

	if ( !(  
			( (ignore_flood.value == 1 && ((flags & TPM_NORMAL) || (flags & TPM_SPECTATOR))) ||
			(ignore_flood.value == 2 && flags != 0) )
		)  )
	{
		return NO_IGNORE_NO_ADD;
	}

	if (!sender)	//don't ignore system messages.
		return NO_IGNORE_NO_ADD;

	slot = sender - cl.players;

	if (!cls.demoplayback && !strcmp(sender->name, Player_MyName()))
	{
		return NO_IGNORE_NO_ADD;
	}
	for (i = 0; i < FLOODLIST_SIZE; i++)
	{
		if (floodlist[i].playernum == slot && floodlist[i].data[0] && !strncmp(floodlist[i].data, s, sizeof(floodlist[i].data) - 1) &&
			realtime - floodlist[i].time < ignore_flood_duration.value) {
			return IGNORE_NO_ADD;
		}
	}
	return NO_IGNORE_ADD;
}

void Ignore_Flood_Add(player_info_t *sender, const char *s)
{
	floodlist[floodindex].playernum = sender - cl.players;
	floodlist[floodindex].data[0] = 0;
	Q_strncpyz(floodlist[floodindex].data, s, sizeof(floodlist[floodindex].data));
	floodlist[floodindex].time = realtime;
	floodindex++;
	if (floodindex == FLOODLIST_SIZE)
		floodindex = 0;
}


qboolean Ignore_Message(const char *sendername, const char *s, int flags)
{
	int slot, i;
	playerview_t *pv = &cl.playerview[0];

	if (!ignore_mode.ival && (flags & 2))
		return false;		


	if (ignore_spec.ival == 2 && (flags == 4 || (flags == 8 && ignore_mode.ival)))
		return true;
	else if (ignore_spec.ival == 1 && (flags == 4) && !pv->spectator)
		return true;

	if (!sendername)
		return false;

	if ((slot = Player_NametoSlot(sendername)) == PLAYER_NAME_NOMATCH)
		return false;	

	if (IsIgnored(slot))
		return true;

	if (ignore_opponents.ival && (
				(int) ignore_opponents.ival == 1 ||
				(cls.state >= ca_connected && cl.matchstate == MATCH_INPROGRESS && !cls.demoplayback && !pv->spectator) // match?
				) && 
			flags == 1 && !pv->spectator && slot != pv->playernum &&
			(!cl.teamplay || strcmp(cl.players[slot].team, cl.players[pv->playernum].team))
			)
	{
		return true;
	}


	if (!cl.teamplay)
		return false;

	if (cl.players[slot].spectator || !strcmp(Player_MyName(), sendername))
		return false;	

	for (i = 0; i < MAX_TEAMIGNORELIST && ignoreteamlist[i][0]; i++)
	{
		if (!strncmp(cl.players[slot].team, ignoreteamlist[i], sizeof(ignoreteamlist[i]) - 1))
			return true;
	}

	return false;
}

void Ignore_ResetFloodList(void)
{
	int i;

	for (i = 0; i < FLOODLIST_SIZE; i++)
		floodlist[i].data[0] = 0;
	floodindex = 0;
}

void Ignore_Init(void)
{
	int i;

#define IGNOREGROUP "Player Ignoring"

	for (i = 0; i < MAX_TEAMIGNORELIST; i++)
		ignoreteamlist[i][0] = 0;
	Ignore_ResetFloodList();

	Cvar_Register (&ignore_flood_duration, IGNOREGROUP);
	Cvar_Register (&ignore_flood, IGNOREGROUP);
	Cvar_Register (&ignore_spec, IGNOREGROUP);
	Cvar_Register (&ignore_qizmo_spec, IGNOREGROUP);
	Cvar_Register (&ignore_mode, IGNOREGROUP);
	Cvar_Register (&ignore_opponents, IGNOREGROUP);

	Cmd_AddCommand ("cl_voip_mute", VIgnore_f);
	Cmd_AddCommand ("cl_voip_unmute", VUnignore_f);
	Cmd_AddCommand ("ignore", Ignore_f);
	Cmd_AddCommand ("ignorelist", IgnoreList_f);			
	Cmd_AddCommand ("unignore", Unignore_f);				
	Cmd_AddCommand ("ignore_team", Ignoreteam_f);		
	Cmd_AddCommand ("unignore_team", Unignoreteam_f);	
	Cmd_AddCommand ("unignoreAll", UnignoreAll_f);			
	Cmd_AddCommand ("unignoreAll_team", UnignoreteamAll_f);	
	Cmd_AddCommand ("unignore_id", Unignore_ID_f);		
	Cmd_AddCommand ("ignore_id", Ignore_ID_f);			
}
