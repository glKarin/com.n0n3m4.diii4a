
#include "quakedef.h"

#ifdef QUAKEHUD
#define MAX_WEAPONS 64 //fixme: make dynamic.

typedef enum {
	//one componant
	ff_death,
	ff_tkdeath,
	ff_suicide,
	ff_bonusfrag,
	ff_tkbonus,
	ff_flagtouch,
	ff_flagcaps,
	ff_flagdrops,

	//two componant
	ff_frags,		//must be the first of the two componant
	ff_fragedby,
	ff_tkills,
	ff_tkilledby,
} fragfilemsgtypes_t;

typedef struct statmessage_s {
	fragfilemsgtypes_t type;
	int wid;
	size_t l1, l2;
	char *msgpart1;
	char *msgpart2;
	struct statmessage_s *next;
} statmessage_t;

typedef unsigned short stat;
typedef struct {
	stat totaldeaths;
	stat totalsuicides;
	stat totalteamkills;
	stat totalkills;
	stat totaltouches;
	stat totalcaps;
	stat totaldrops;

	//I was going to keep track of kills with a certain gun - too much memory
	//track only your own and total weapon kills rather than per client
	struct wt_s {
		//these include you.
		stat kills;
		stat teamkills;
		stat suicides;		

		stat ownkills;
		stat owndeaths;
		stat ownteamkills;
		stat ownteamdeaths;
		stat ownsuicides;
		char *fullname;
		char *abrev;
		char *image;
		char *codename;
	} weapontotals[MAX_WEAPONS];

	struct ct_s {
		stat caps;		//times they captured the flag
		stat drops;		//times lost the flag
		stat grabs;		//times grabbed flag

		stat owndeaths;	//times you killed them
		stat ownkills;	//times they killed you
		stat deaths;	//times they died (including by you)
		stat kills;		//times they killed (including by you)
		stat teamkills;	//times they killed a team member.
		stat teamdeaths;	//times they died to a team member.
		stat suicides;	//times they were stupid.
	} clienttotals[MAX_CLIENTS];

	qboolean readcaps;
	qboolean readkills;
	statmessage_t *message;
} fragstats_t;

static void TrackerCallback(struct cvar_s *var, char *oldvalue);
static cvar_t r_tracker_frags = CVARD("r_tracker_frags", "0", "0: like vanilla quake\n1: shows only your kills/deaths\n2: shows all kills\n");
static cvar_t r_tracker_time = CVARCD("r_tracker_time", "4", TrackerCallback, "how long it takes for r_tracker messages to start fading\n");
static cvar_t r_tracker_fadetime = CVARCD("r_tracker_fadetime", "1", TrackerCallback, "how long it takes for r_tracker messages to fully fade once they start fading\n");
static cvar_t r_tracker_x = CVARCD("r_tracker_x", "0.5", TrackerCallback, "left position of the r_tracker messages, as a fraction of the screen's width, eg 0.5\n");
static cvar_t r_tracker_y = CVARCD("r_tracker_y", "0.333", TrackerCallback, "top position of the r_tracker messages, as a fraction of the screen's height, eg 0.333\n");
static cvar_t r_tracker_w = CVARCD("r_tracker_w", "0.5", TrackerCallback, "width of the r_tracker messages, as a fraction of the screen's width, eg 0.5\n");
static cvar_t r_tracker_lines = CVARAFCD("r_tracker_lines", "8", "r_tracker_messages", 0, TrackerCallback, "number of r_tracker messages to display\n");
static void Tracker_Update(console_t *tracker)
{
	tracker->notif_l = tracker->maxlines = max(1,r_tracker_lines.ival);
	tracker->notif_x = r_tracker_x.value;
	tracker->notif_y = r_tracker_y.value;
	tracker->notif_w = max(0,r_tracker_w.value);
	tracker->notif_t = max(0,r_tracker_time.value);
	tracker->notif_fade = max(0,r_tracker_fadetime.value);

	//if its mostly on one side of the screen, align it accordingly.
	if (tracker->notif_x + tracker->notif_w*0.5 >= 0.5)
		tracker->flags |= CONF_NOTIFY_RIGHT;
	else
		tracker->flags &= ~(CONF_NOTIFY_RIGHT);
}
static void TrackerCallback(struct cvar_s *var, char *oldvalue)
{
	console_t *tracker = Con_FindConsole("tracker");
	if (tracker)
		Tracker_Update(tracker);
}


static fragstats_t fragstats;

int Stats_GetKills(int playernum)
{
	return fragstats.clienttotals[playernum].kills;
}
int Stats_GetTKills(int playernum)
{
	return fragstats.clienttotals[playernum].teamkills;
}
int Stats_GetDeaths(int playernum)
{
	return fragstats.clienttotals[playernum].deaths;
}
int Stats_GetTouches(int playernum)
{
	return fragstats.clienttotals[playernum].grabs;
}
int Stats_GetCaptures(int playernum)
{
	return fragstats.clienttotals[playernum].caps;
}

qboolean Stats_HaveFlags(int showtype)
{
	int i;
	if (showtype)
		return fragstats.readcaps;
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (fragstats.clienttotals[i].caps ||
			fragstats.clienttotals[i].drops ||
			fragstats.clienttotals[i].grabs)
			return fragstats.readcaps;
	}
	return false;
}
qboolean Stats_HaveKills(void)
{
	return fragstats.readkills;
}

static char lastownfragplayer[64];
static float lastownfragtime;
float Stats_GetLastOwnFrag(int seat, char *res, int reslen)
{
	if (seat)
	{
		if (reslen)
			*res = 0;
		return 0;
	}

	//erk, realtime was reset?
	if (lastownfragtime > (float)realtime)
		lastownfragtime = 0;

	Q_strncpyz(res, lastownfragplayer, reslen);
	return realtime - lastownfragtime;
};
static void Stats_OwnFrag(char *name)
{
	Q_strncpyz(lastownfragplayer, name, sizeof(lastownfragplayer));
	lastownfragtime = realtime;
}

void VARGS Stats_Message(char *msg, ...);

qboolean Stats_TrackerImageLoaded(const char *in)
{
	int error;
	if (in)
		return Font_TrackerValid(unicode_decode(&error, in, &in, true));
	return false;
}
static char *Stats_GenTrackerImageString(char *in)
{	//images are of the form "foo \sg\ bar \q\"
	//which should eg be remapped to: "foo ^Ue200 bar foo ^Ue201"
	char res[256];
	char image[MAX_QPATH];
	char *outi;
	char *out;
	int i;
	if (!in || !*in)
		return NULL;

	for (out = res; *in && out < res+sizeof(res)-10; )
	{
		if (*in == '\\')
		{
			in++;
			for (outi = image; *in && outi < image+sizeof(image)-10; )
			{
				if (*in == '\\')
					break;
				*outi++ = *in++;
			}
			*outi = 0;
			in++;
			
			i = Font_RegisterTrackerImage(va("tracker/%s", image));
			if (i)
			{
				char hexchars[16] = "0123456789abcdef";
				*out++ = '^';
				*out++ = 'U';
				*out++ = hexchars[(i>>12)&15];
				*out++ = hexchars[(i>>8)&15];
				*out++ = hexchars[(i>>4)&15];
				*out++ = hexchars[(i>>0)&15];
			}
			else
			{
				//just copy the short name over, not much else we can do.
				for(outi = image; out < res+sizeof(res)-10 && *outi; )
					*out++ = *outi++;
			}
		}
		else
			*out++ = *in++;
	}
	*out = 0;
	return Z_StrDup(res);
}

void Stats_FragMessage(int p1, int wid, int p2, qboolean teamkill)
{
	static const char *nonplayers[] = {
		"BUG",
		"(teamkill)",
		"(suicide)",
		"(death)",
		"(unknown)",
		"(fixme)",
		"(fixme)"
	};
	char message[512];
	console_t *tracker;
	struct wt_s *w = &fragstats.weapontotals[wid];
	const char *p1n = (p1 < 0)?nonplayers[-p1]:cl.players[p1].name;
	const char *p2n = (p2 < 0)?nonplayers[-p2]:cl.players[p2].name;
	int localplayer = (cl.playerview[0].cam_state == CAM_EYECAM)?cl.playerview[0].cam_spec_track:cl.playerview[0].playernum;

#define YOU_GOOD		S_COLOR_GREEN
#define YOU_BAD			S_COLOR_BLUE
#define TEAM_GOOD		S_COLOR_GREEN
#define TEAM_BAD		S_COLOR_RED
#define TEAM_VBAD		S_COLOR_BLUE
#define TEAM_NEUTRAL	S_COLOR_WHITE	//enemy team thing that does not directly affect us
#define ENEMY_GOOD		S_COLOR_RED
#define ENEMY_BAD		S_COLOR_GREEN
#define ENEMY_NEUTRAL	S_COLOR_WHITE


	char *p1c = S_COLOR_WHITE;
	char *p2c = S_COLOR_WHITE;

	if (!r_tracker_frags.ival)
		return;
	if (r_tracker_frags.ival < 2)
		if (p1 != localplayer && p2 != localplayer)
			return;

	if (teamkill)
	{//team kills/suicides are always considered bad.
		if (p1 == localplayer)
			p1c = YOU_BAD;
		else if (cl.teamplay && !strcmp(cl.players[p1].team, cl.players[localplayer].team))
			p1c = TEAM_VBAD;
		else
			p1c = TEAM_NEUTRAL;
		p2c = p1c;
	}
	else if (p1 == p2)
		p1c = p2c = YOU_BAD;
	else if (cl.teamplay && p1 >= 0 && p2 >= 0 && !strcmp(cl.players[p1].team, cl.players[p2].team))
		p1c = p2c = TEAM_VBAD;
	else
	{
		if (p2 >= 0)
		{
			//us/teammate killing is good - unless it was a teammate.
			if (p2 == localplayer)
				p2c = YOU_GOOD;
			else if (cl.teamplay && !strcmp(cl.players[p2].team, cl.players[localplayer].team))
				p2c = TEAM_GOOD;
			else
				p2c = ENEMY_GOOD;
		}
		if (p1 >= 0)
		{
			//us/teammate dying is bad.
			if (p1 == localplayer)
				p1c = YOU_BAD;
			else if (cl.teamplay && !strcmp(cl.players[p1].team, cl.players[localplayer].team))
				p1c = TEAM_BAD;
			else
				p1c = p2c;
		}
	}

	Q_snprintfz(message, sizeof(message), "%s%s ^7%s %s%s\n", p2c, p2n, Stats_TrackerImageLoaded(w->image)?w->image:w->abrev, p1c, p1n);

	tracker = Con_FindConsole("tracker");
	if (!tracker)
	{
		tracker = Con_Create("tracker", CONF_HIDDEN|CONF_NOTIFY|CONF_NOTIFY_BOTTOM);
		Tracker_Update(tracker);
	}
	Con_PrintCon(tracker, message, tracker->parseflags);
}

void Stats_Evaluate(fragfilemsgtypes_t mt, int wid, int p1, int p2)
{
	qboolean u1;
	qboolean u2;

	if (mt == ff_frags || mt == ff_tkills)
	{
		int tmp = p1;
		p1 = p2;
		p2 = tmp;
	}

	u1 = (p1 == (cl.playerview[0].playernum));
	u2 = (p2 == (cl.playerview[0].playernum));

	if (p1 == -1)
		p1 = p2;
	if (p2 == -1)
		p2 = p1;

	//messages are killed weapon killer
	switch(mt)
	{
	case ff_death:
		if (u1)
		{
			fragstats.weapontotals[wid].owndeaths++;
			fragstats.weapontotals[wid].ownkills++;
		}

		fragstats.weapontotals[wid].kills++;
		if (p1 >= 0)
			fragstats.clienttotals[p1].deaths++;
		fragstats.totaldeaths++;

		Stats_FragMessage(p1, wid, -3, true);

		if (u1)
			Stats_Message("You died\n%s deaths: %i\n", fragstats.weapontotals[wid].fullname, fragstats.weapontotals[wid].owndeaths);
		break;
	case ff_suicide:
		if (u1)
		{
			fragstats.weapontotals[wid].ownsuicides++;
			fragstats.weapontotals[wid].owndeaths++;
			fragstats.weapontotals[wid].ownkills++;
		}

		fragstats.weapontotals[wid].suicides++;
		fragstats.weapontotals[wid].kills++;
		if (p1 >= 0)
		{
			fragstats.clienttotals[p1].suicides++;
			fragstats.clienttotals[p1].deaths++;
		}
		fragstats.totalsuicides++;
		fragstats.totaldeaths++;

		Stats_FragMessage(p1, wid, -2, true);
		if (u1)
			Stats_Message("You killed your own dumb self\n%s suicides: %i (%i)\n", fragstats.weapontotals[wid].fullname, fragstats.weapontotals[wid].ownsuicides, fragstats.weapontotals[wid].suicides);
		break;
	case ff_bonusfrag:
		if (u1)
			fragstats.weapontotals[wid].ownkills++;
		fragstats.weapontotals[wid].kills++;
		if (p1 >= 0)
			fragstats.clienttotals[p1].kills++;
		fragstats.totalkills++;

		Stats_FragMessage(-4, wid, p1, false);
		if (u1)
		{
			Stats_OwnFrag("someone");
			Stats_Message("You killed someone\n%s kills: %i\n", fragstats.weapontotals[wid].fullname, fragstats.weapontotals[wid].ownkills);
		}
		break;
	case ff_tkbonus:
		if (u1)
			fragstats.weapontotals[wid].ownkills++;
		fragstats.weapontotals[wid].kills++;
		fragstats.clienttotals[p1].kills++;
		fragstats.totalkills++;

		if (u1)
			fragstats.weapontotals[wid].ownteamkills++;
		fragstats.weapontotals[wid].teamkills++;
		if (p1 >= 0)
			fragstats.clienttotals[p1].teamkills++;
		fragstats.totalteamkills++;

		Stats_FragMessage(-1, wid, p1, true);

		if (u1)
		{
			Stats_Message("You killed your teammate\n%s teamkills: %i\n", fragstats.weapontotals[wid].fullname, fragstats.weapontotals[wid].ownteamkills);
		}
		break;
	case ff_flagtouch:
		fragstats.clienttotals[p1].grabs++;
		fragstats.totaltouches++;

		if (u1 && p1 >= 0)
		{
			Stats_Message("You grabbed the flag\nflag grabs: %i (%i)\n", fragstats.clienttotals[p1].grabs, fragstats.totaltouches);
		}
		break;
	case ff_flagcaps:
		if (p1 >= 0)
			fragstats.clienttotals[p1].caps++;
		fragstats.totalcaps++;

		if (u1 && p1 >= 0)
		{
			Stats_Message("You captured the flag\nflag captures: %i (%i)\n", fragstats.clienttotals[p1].caps, fragstats.totalcaps);
		}
		break;
	case ff_flagdrops:
		if (p1 >= 0)
			fragstats.clienttotals[p1].drops++;
		fragstats.totaldrops++;

		if (u1 && p1 >= 0)
		{
			Stats_Message("You dropped the flag\nflag drops: %i (%i)\n", fragstats.clienttotals[p1].drops, fragstats.totaldrops);
		}
		break;

	//p1 died, p2 killed
	case ff_frags:
	case ff_fragedby:
		fragstats.weapontotals[wid].kills++;

		if (p1 >= 0)
			fragstats.clienttotals[p1].deaths++;
		fragstats.totaldeaths++;
		if (u1)
		{
			fragstats.weapontotals[wid].owndeaths++;
			if (p1 >= 0 && p2 >= 0)
				Stats_Message("%s killed you\n%s deaths: %i (%i/%i)\n", cl.players[p2].name, fragstats.weapontotals[wid].fullname, fragstats.clienttotals[p2].owndeaths, fragstats.weapontotals[wid].owndeaths, fragstats.totaldeaths);
		}

		if (p2 >= 0)
			fragstats.clienttotals[p2].kills++;
		fragstats.totalkills++;
		if (u2)
		{
			if (p1 >= 0)
				Stats_OwnFrag(cl.players[p1].name);
			fragstats.weapontotals[wid].ownkills++;
			if (p1 >= 0 && p2 >= 0)
				Stats_Message("You killed %s\n%s kills: %i (%i/%i)\n", cl.players[p1].name, fragstats.weapontotals[wid].fullname, fragstats.clienttotals[p2].kills, fragstats.weapontotals[wid].kills, fragstats.totalkills);
		}

		Stats_FragMessage(p1, wid, p2, false);
		break;
	case ff_tkdeath:
		//killed by a teammate, but we don't know who
		//kinda useless, but this is all some mods give us
		fragstats.weapontotals[wid].teamkills++;
		fragstats.weapontotals[wid].kills++;
		fragstats.totalkills++;		//its a kill, but we don't know who from
		fragstats.totalteamkills++;

		if (u1)
			fragstats.weapontotals[wid].owndeaths++;
		fragstats.clienttotals[p1].teamdeaths++;
		fragstats.clienttotals[p1].deaths++;
		fragstats.totaldeaths++;

		Stats_FragMessage(p1, wid, -1, true);

		if (u1)
			Stats_Message("Your teammate killed you\n%s deaths: %i\n", fragstats.weapontotals[wid].fullname, fragstats.weapontotals[wid].owndeaths);
		break;

	case ff_tkills:
	case ff_tkilledby:
		//p1 killed by p2 (kills is already inverted)
		fragstats.weapontotals[wid].teamkills++;
		fragstats.weapontotals[wid].kills++;

		if (u1)
		{
			fragstats.weapontotals[wid].ownteamdeaths++;
			fragstats.weapontotals[wid].owndeaths++;
		}
		if (p1 >= 0)
		{
			fragstats.clienttotals[p1].teamdeaths++;
			fragstats.clienttotals[p1].deaths++;
		}
		fragstats.totaldeaths++;

		if (u2)
		{
			fragstats.weapontotals[wid].ownkills++;
			fragstats.weapontotals[wid].ownkills++;
		}
		if (p2 >= 0)
		{
			fragstats.clienttotals[p2].teamkills++;
			fragstats.clienttotals[p2].kills++;
		}
		fragstats.totalkills++;

		fragstats.totalteamkills++;

		Stats_FragMessage(p1, wid, p2, false);
		if (u1 && p2 >= 0)
			Stats_Message("%s killed you\n%s deaths: %i (%i/%i)\n", cl.players[p2].name, fragstats.weapontotals[wid].fullname, fragstats.clienttotals[p2].owndeaths, fragstats.weapontotals[wid].owndeaths, fragstats.totaldeaths);
		if (u2 && p1 >= 0 && p2 >= 0)
		{
			Stats_OwnFrag(cl.players[p1].name);
			Stats_Message("You killed %s\n%s kills: %i (%i/%i)\n", cl.players[p1].name, fragstats.weapontotals[wid].fullname, fragstats.clienttotals[p2].kills, fragstats.weapontotals[wid].kills, fragstats.totalkills);
		}
		break;
	}
}

static int Stats_FindWeapon(char *codename, qboolean create)
{
	int i;

	if (!strcmp(codename, "NONE"))
		return 0;
	if (!strcmp(codename, "NULL"))
		return 0;
	if (!strcmp(codename, "NOWEAPON"))
		return 0;

	for (i = 1; i < MAX_WEAPONS; i++)
	{
		if (!fragstats.weapontotals[i].codename)
		{
			fragstats.weapontotals[i].codename = Z_Malloc(strlen(codename)+1);
			strcpy(fragstats.weapontotals[i].codename, codename);
			return i;
		}

		if (!stricmp(fragstats.weapontotals[i].codename, codename))
		{
			if (create)
				return -2;
			return i;
		}
	}
	return -1;
}

static void Stats_StatMessage(fragfilemsgtypes_t type, int wid, char *token1, char *token2)
{
	statmessage_t *ms;
	char *t;
	ms = Z_Malloc(sizeof(statmessage_t) + strlen(token1)+1 + (token2 && *token2?strlen(token2)+1:0));
	t = (char *)(ms+1);
	ms->msgpart1 = t;
	strcpy(t, token1);
	ms->l1 = strlen(ms->msgpart1);
	if (token2 && *token2)
	{
		t += strlen(t)+1;
		ms->msgpart2 = t;
		strcpy(t, token2);
		ms->l2 = strlen(ms->msgpart2);
	}
	ms->type = type;
	ms->wid = wid;

	ms->next = fragstats.message;
	fragstats.message = ms;

	//we have a message type, save the fact that we have it.
	if (type == ff_flagtouch || type == ff_flagcaps || type == ff_flagdrops)
		fragstats.readcaps = true;
	if (type == ff_frags || type == ff_fragedby)
		fragstats.readkills = true;
}

void Stats_Clear(void)
{
	int i;
	statmessage_t *ms;

	while (fragstats.message)
	{
		ms = fragstats.message;
		fragstats.message = ms->next;
		Z_Free(ms);
	}

	for (i = 1; i < MAX_WEAPONS; i++)
	{
		if (fragstats.weapontotals[i].codename)	Z_Free(fragstats.weapontotals[i].codename);
		if (fragstats.weapontotals[i].fullname)	Z_Free(fragstats.weapontotals[i].fullname);
		if (fragstats.weapontotals[i].abrev)	Z_Free(fragstats.weapontotals[i].abrev);
		if (fragstats.weapontotals[i].image)	Z_Free(fragstats.weapontotals[i].image);
	}

	memset(&fragstats, 0, sizeof(fragstats));
}

#define Z_Copy(tk) tz = Z_Malloc(strlen(tk)+1);strcpy(tz, tk)	//remember the braces

void Stats_Init(void)
{
	Cvar_Register(&r_tracker_frags, NULL);
	Cvar_Register(&r_tracker_time, NULL);
	Cvar_Register(&r_tracker_fadetime, NULL);
	Cvar_Register(&r_tracker_x, NULL);
	Cvar_Register(&r_tracker_y, NULL);
	Cvar_Register(&r_tracker_w, NULL);
	Cvar_Register(&r_tracker_lines, NULL);
}
static void Stats_LoadFragFile(char *name)
{
	char filename[MAX_QPATH];
	char *file;
	char *end;
	char *tk, *tz;
	char oend;

	Stats_Clear();

	strcpy(filename, name);
	COM_DefaultExtension(filename, ".dat", sizeof(filename));

	file = COM_LoadTempFile(filename, 0, NULL);
	if (!file || !*file)
	{
		Con_DPrintf("Couldn't load %s\n", filename);
		return;
	}
	else
		Con_DPrintf("Loaded %s\n", filename);

	oend = 1;
	for (;oend;)
	{
		for (end = file; *end && *end != '\n'; end++)
			;
		oend = *end;
		*end = '\0';
		Cmd_TokenizeString(file, true, false);
		file = end+1;
		if (!Cmd_Argc())
			continue;

		tk = Cmd_Argv(0);
		if (!stricmp(tk, "#fragfile"))
		{
			tk = Cmd_Argv(1);
				 if (!stricmp(tk, "version"))		{}
			else if (!stricmp(tk, "gamedir"))		{}
			else Con_Printf("Unrecognised #meta \"%s\"\n", tk);
		}
		else if (!stricmp(tk, "#meta"))
		{
			tk = Cmd_Argv(1);
				 if (!stricmp(tk, "title"))			{}
			else if (!stricmp(tk, "description"))	{}
			else if (!stricmp(tk, "author"))		{}
			else if (!stricmp(tk, "email"))			{}
			else if (!stricmp(tk, "webpage"))		{}
			else {Con_Printf("Unrecognised #meta \"%s\"\n", tk);continue;}
		}
		else if (!stricmp(tk, "#define"))
		{
			tk = Cmd_Argv(1);
			if (!stricmp(tk, "weapon_class") ||
				!stricmp(tk, "wc"))	
			{
				int wid;

				tk = Cmd_Argv(2);

				wid = Stats_FindWeapon(tk, true);
				if (wid == -1)
				{Con_Printf("Too many weapon definitions. The max is %i\n", MAX_WEAPONS);continue;}
				else if (wid < -1)
				{Con_Printf("Weapon \"%s\" is already defined\n", tk);continue;}
				else
				{
					fragstats.weapontotals[wid].fullname = Z_Copy(Cmd_Argv(3));
					fragstats.weapontotals[wid].abrev = Z_Copy(Cmd_Argv(4));
					fragstats.weapontotals[wid].image = Stats_GenTrackerImageString(Cmd_Argv(5));
				}
			}
			else if (!stricmp(tk, "obituary") ||
					 !stricmp(tk, "obit"))
			{
				int fftype;
				tk = Cmd_Argv(2);

					 if (!stricmp(tk, "PLAYER_DEATH"))			{fftype = ff_death;}
				else if (!stricmp(tk, "PLAYER_SUICIDE"))		{fftype = ff_suicide;}
				else if (!stricmp(tk, "X_FRAGS_UNKNOWN"))		{fftype = ff_bonusfrag;}
				else if (!stricmp(tk, "X_TEAMKILLS_UNKNOWN"))	{fftype = ff_tkbonus;}
				else if (!stricmp(tk, "X_TEAMKILLED_UNKNOWN"))	{fftype = ff_tkdeath;}
				else if (!stricmp(tk, "X_FRAGS_Y"))				{fftype = ff_frags;}
				else if (!stricmp(tk, "X_FRAGGED_BY_Y"))		{fftype = ff_fragedby;}
				else if (!stricmp(tk, "X_TEAMKILLS_Y"))			{fftype = ff_tkills;}
				else if (!stricmp(tk, "X_TEAMKILLED_BY_Y"))		{fftype = ff_tkilledby;}
				else {Con_Printf("Unrecognised obituary \"%s\"\n", tk);continue;}

				Stats_StatMessage(fftype, Stats_FindWeapon(Cmd_Argv(3), false), Cmd_Argv(4), Cmd_Argv(5));
			}
			else if (!stricmp(tk, "flag_alert") ||
					 !stricmp(tk, "flag_msg"))
			{
				int fftype;
				tk = Cmd_Argv(2);

					 if (!stricmp(tk, "X_TOUCHES_FLAG"))		{fftype = ff_flagtouch;}
				else if (!stricmp(tk, "X_GETS_FLAG"))			{fftype = ff_flagtouch;}
				else if (!stricmp(tk, "X_TAKES_FLAG"))			{fftype = ff_flagtouch;}
				else if (!stricmp(tk, "X_CAPTURES_FLAG"))		{fftype = ff_flagcaps;}
				else if (!stricmp(tk, "X_CAPS_FLAG"))			{fftype = ff_flagcaps;}
				else if (!stricmp(tk, "X_SCORES"))				{fftype = ff_flagcaps;}
				else if (!stricmp(tk, "X_DROPS_FLAG"))			{fftype = ff_flagdrops;}
				else if (!stricmp(tk, "X_FUMBLES_FLAG"))		{fftype = ff_flagdrops;}
				else if (!stricmp(tk, "X_LOSES_FLAG"))			{fftype = ff_flagdrops;}
				else {Con_Printf("Unrecognised flag alert \"%s\"\n", tk);continue;}

				Stats_StatMessage(fftype, 0, Cmd_Argv(3), NULL);
			}
			else
			{Con_Printf("Unrecognised directive \"%s\"\n", tk);continue;}
		}
		else
		{Con_Printf("Unrecognised directive \"%s\"\n", tk);continue;}
	}
}


static int qm_strcmp(const char *s1, const char *s2)//not like strcmp at all...
{
	while(*s1)
	{
		if ((*s1++&0x7f)!=(*s2++&0x7f))
			return 1;
	}
	return 0;
}
/*
static int qm_stricmp(char *s1, char *s2)//not like strcmp at all...
{
	int c1,c2;
	while(*s1)
	{
		c1 = *s1++&0x7f;
		c2 = *s2++&0x7f;

		if (c1 >= 'A' && c1 <= 'Z')
			c1 = c1 - 'A' + 'a';

		if (c2 >= 'A' && c2 <= 'Z')
			c2 = c2 - 'A' + 'a';

		if (c1!=c2)
			return 1;
	}
	return 0;
}
*/

static int Stats_ExtractName(const char **line)
{
	int i;
	int bm;
	int ml = 0;
	int l;
	bm = -1;
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (!qm_strcmp(cl.players[i].name, *line))
		{
			l = strlen(cl.players[i].name);
			if (l > ml)
			{
				bm = i;
				ml = l;
			}
		}
	}
	*line += ml;
	return bm;
}

qboolean Stats_ParsePickups(const char *line)
{
#ifdef HAVE_LEGACY
	//fixme: rework this to support custom strings, with custom pickup icons
	if (!Q_strncmp(line, "You got the ", 12))	//weapons, ammo, keys, powerups
		return true;
	if (!Q_strncmp(line, "You got armor", 13))	//caaake...
		return true;
	if (!Q_strncmp(line, "You get ", 8))	//backpacks
		return true;
	if (!Q_strncmp(line, "You receive ", 12)) //%i health\n
		return true;
#endif
	return false;
}

qboolean Stats_ParsePrintLine(const char *line)
{
	statmessage_t *ms;
	int p1;
	int p2;
	const char *m2;

	p1 = Stats_ExtractName(&line);
	if (p1<0)	//reject it.
	{
		return false;
	}
	
	for (ms = fragstats.message; ms; ms = ms->next)
	{
		if (!Q_strncmp(ms->msgpart1, line, ms->l1))
		{
			if (ms->type >= ff_frags)
			{	//two players
				m2 = line + ms->l1;
				p2 = Stats_ExtractName(&m2);
				if ((!ms->msgpart2 && *m2=='\n') || (ms->msgpart2 && !Q_strncmp(ms->msgpart2, m2, ms->l2)))
				{
					Stats_Evaluate(ms->type, ms->wid, p1, p2);
					return true;	//done.
				}
			}
			else
			{	//one player
				Stats_Evaluate(ms->type, ms->wid, p1, p1);
				return true;	//done.
			}
		}
	}
	return false;
}

void Stats_NewMap(void)
{
	Stats_LoadFragFile("fragfile");
}
#endif
