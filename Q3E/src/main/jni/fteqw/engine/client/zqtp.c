/*
	teamplay.c

	Teamplay enhancements

	Copyright (C) 2000-2001       Anton Gavrilov

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

//Hacked by spike.
//things to fix:
//TP_SearchForMsgTriggers: should only allow safe commands. work out what the meaning of safe commands is.


#include "quakedef.h"

//#include "version.h"
#include "sound.h"
//#include "pmove.h"
#include <time.h>
#include <ctype.h>

typedef qboolean qbool;

#define SP 0

#define Com_Printf Con_Printf

#define strlcpy Q_strncpyz
#define strlcat Q_strncatz
#define Q_stricmp strcasecmp
#define Q_strnicmp strncasecmp


extern int		cl_spikeindex, cl_playerindex, cl_h_playerindex, cl_flagindex, cl_rocketindex, cl_grenadeindex, cl_gib1index, cl_gib2index, cl_gib3index;
extern cvar_t	v_viewheight, dpcompat_console;
trace_t PM_TraceLine (vec3_t start, vec3_t end);
#define ISDEAD(i) ( (i) >= 41 && (i) <= 102 )

qboolean suppress;

//note: csqc obsoletes and even breaks much of this stuff.
//cl.simorg is no longer valid, cl.viewangles isn't terribly valid either.

/*#define isalpha(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))
#define isdigit(x) ((x) >= '0' && (x) <= '9')
#define isxdigit(x) (isdigit(x) || ((x) >= 'a' && (x) <= 'f'))
*/
#define Q_rint(f) ((int)((f)+0.5))

// callbacks used for TP cvars

#ifdef QWSKINS
static void QDECL TP_SkinCvar_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL TP_TeamColor_CB (struct cvar_s *var, char *oldvalue);
static void QDECL TP_EnemyColor_CB (struct cvar_s *var, char *oldvalue);
//cvar_t enemyforceskins = CVARD("enemyforceskins", "0", "0: Read the skin as-is.\n1: Use the player's name instead of their skin setting.\n2: Use their userid (probably too unreliable).3: Use a per-team player index.\n");
//cvar_t teamforceskins = CVARD("enemyforceskins", "0", "0: Read the skin as-is.\n1: Use the player's name instead of their skin setting.\n2: Use their userid (probably too unreliable).3: Use a per-team player index.\n");
#define TP_SKIN_CVARS	\
	TP_CVARAC(cl_teamskin,		"", teamskin, TP_SkinCvar_Callback);	\
	TP_CVARAC(cl_enemyskin,		"", enemyskin, TP_SkinCvar_Callback);	\
	static TP_CVARC(enemycolor,		"off", TP_EnemyColor_CB);	\
	static TP_CVARC(teamcolor,			"off", TP_TeamColor_CB);
#else
#define TP_SKIN_CVARS
#endif

#ifdef QUAKESTATS
#define TP_NAME_CVARS	\
	static TP_CVAR(tp_name_none,			"");	\
	static TP_CVAR(tp_name_axe,				"axe");	\
	TP_CVAR(tp_name_sg,						"sg");	\
	TP_CVAR(tp_name_ssg,					"ssg");	\
	TP_CVAR(tp_name_ng,						"ng");	\
	TP_CVAR(tp_name_sng,					"sng");	\
	TP_CVAR(tp_name_gl,						"gl");	\
	TP_CVAR(tp_name_rl,						"rl");	\
	TP_CVAR(tp_name_lg,						"lg");	\
	static TP_CVAR(tp_name_ra,				"ra");	\
	static TP_CVAR(tp_name_ya,				"ya");	\
	static TP_CVAR(tp_name_ga,				"ga");	\
	static TP_CVAR(tp_name_quad,			"quad");	\
	static TP_CVAR(tp_name_pent,			"pent");	\
	static TP_CVAR(tp_name_ring,			"ring");	\
	static TP_CVAR(tp_name_suit,			"suit");	\
	static TP_CVAR(tp_name_shells,			"shells");	\
	static TP_CVAR(tp_name_nails,			"nails");	\
	static TP_CVAR(tp_name_rockets,			"rockets");	\
	static TP_CVAR(tp_name_cells,			"cells");	\
	static TP_CVAR(tp_name_mh,				"mega");	\
	static TP_CVAR(tp_name_health,			"health");	\
	static TP_CVAR(tp_name_backpack,		"pack");	\
	static TP_CVAR(tp_name_flag,			"flag");	\
	static TP_CVAR(tp_name_nothing,			"nothing");	\
	static TP_CVAR(tp_name_at,				"at");	\
	static TP_CVAR(tp_need_ra,				"50");	\
	static TP_CVAR(tp_need_ya,				"50");	\
	static TP_CVAR(tp_need_ga,				"50");	\
	static TP_CVAR(tp_need_health,			"50");	\
	static TP_CVAR(tp_need_weapon,			"35687");	\
	static TP_CVAR(tp_need_rl,				"1");	\
	static TP_CVAR(tp_need_rockets,			"5");	\
	static TP_CVAR(tp_need_cells,			"20");	\
	static TP_CVAR(tp_need_nails,			"40");	\
	static TP_CVAR(tp_need_shells,			"10");	\
	static TP_CVAR(tp_name_disp,			"dispenser");	\
	static TP_CVAR(tp_name_sentry,			"sentry gun");	\
	static TP_CVAR(tp_name_rune_1,			"resistance rune");	\
	static TP_CVAR(tp_name_rune_2,			"strength rune");	\
	static TP_CVAR(tp_name_rune_3,			"haste rune");	\
	static TP_CVAR(tp_name_rune_4,			"regeneration rune");	\
	\
	static TP_CVAR(tp_name_status_red,		"$R");	\
	static TP_CVAR(tp_name_status_green,	"$G");	\
	static TP_CVAR(tp_name_status_yellow,	"$Y");	\
	static TP_CVAR(tp_name_status_blue,		"$B");	\
	\
	static TP_CVAR(tp_name_armortype_ga,	"g");	\
	static TP_CVAR(tp_name_armortype_ya,	"y");	\
	static TP_CVAR(tp_name_armortype_ra,	"r");	\
	static TP_CVAR(tp_name_armor,			"armor");	\
	static TP_CVAR(tp_name_weapon,			"weapon");	\
	static TP_CVAR(tp_weapon_order,			"78654321");	\
	\
  	static TP_CVAR(tp_name_quaded,			"quaded");	\
	static TP_CVAR(tp_name_pented,			"pented");	\
	static TP_CVAR(tp_name_separator,		"/");	\
	\
	static TP_CVAR(tp_name_enemy,			"enemy");	\
	static TP_CVAR(tp_name_teammate,		"");	\
	static TP_CVAR(tp_name_eyes,			"eyes");	\
	\
	static TP_CVAR(loc_name_separator,		"-");	\
	static TP_CVAR(loc_name_ssg,			"ssg");	\
	static TP_CVAR(loc_name_ng,				"ng");	\
	static TP_CVAR(loc_name_sng,			"sng");	\
	static TP_CVAR(loc_name_gl,				"gl");	\
	static TP_CVAR(loc_name_rl,				"rl");	\
	static TP_CVAR(loc_name_lg,				"lg");	\
	static TP_CVAR(loc_name_ga,				"ga");	\
	static TP_CVAR(loc_name_ya,				"ya");	\
	static TP_CVAR(loc_name_ra,				"ra");	\
	static TP_CVAR(loc_name_mh,				"mh");	\
	static TP_CVAR(loc_name_quad,			"quad");	\
	static TP_CVAR(loc_name_pent,			"pent");	\
	static TP_CVAR(loc_name_ring,			"ring");	\
	static TP_CVAR(loc_name_suit,			"suit");
#else
	#define TP_NAME_CVARS
#endif

//a list of all the cvars
//this is down to the fact that I keep defining them but forgetting to register. :/
#define TP_CVARS \
	TP_SKIN_CVARS \
	TP_NAME_CVARS \
	static TP_CVAR(cl_fakename,		"");	\
	TP_CVAR(cl_parseSay,		"1");	\
	TP_CVAR(cl_parseFunChars,		"1");	\
	TP_CVAR(cl_triggers,		"1");	\
	static TP_CVAR(tp_autostatus,		"");	/* things which will not always change, but are useful */ \
	TP_CVAR(tp_forceTriggers,		"0");	\
	TP_CVAR(tp_loadlocs,		"1");	\
	static TP_CVAR(tp_soundtrigger,		"~");	\
	\
	static TP_CVAR(tp_name_someplace,	"someplace")

//create the globals for all the TP cvars.
#define TP_CVAR(name,def) cvar_t	name = CVAR(#name, def)
#define TP_CVARC(name,def,call) cvar_t name = CVARC(#name, def, call)
#define TP_CVARAC(name,def,name2,call) cvar_t name = CVARAFC(#name, def, #name2, 0, call)
TP_CVARS;
#undef TP_CVAR
#undef TP_CVARC
#undef TP_CVARAC

extern cvar_t	host_mapname;

#define MAX_LOC_NAME 48

#ifdef QUAKESTATS
static void TP_FindModelNumbers (void);
static void TP_FindPoint (void);

// this structure is cleared after entering a new map
typedef struct tvars_s {
	char	autoteamstatus[256];
	float	autoteamstatus_time;

	int		health;
	int		items;
	int		olditems;
	int		stat_framecounts[MAX_CL_STATS];
	int		activeweapon;
	float	respawntrigger_time;
	float	deathtrigger_time;
	float	f_version_reply_time;
	char	lastdeathloc[MAX_LOC_NAME];
	char	tookname[32];
	char	tookloc[MAX_LOC_NAME];
	float	tooktime;

	int		pointframe;		// cls.framecount for which point* vars are valid
	char	pointname[32];
	vec3_t	pointorg;
	char	pointloc[MAX_LOC_NAME];
	int		droppedweapon;
	char	lastdroploc[MAX_LOC_NAME];

	int last_numenemies;
	int numenemies;

	int last_numfriendlies;
	int numfriendlies;

	int enemy_powerups;
	float enemy_powerups_time;

	float lastdrop_time;

	char lasttrigger_match[256];

	enum {
		POINT_TYPE_ENEMY,
		POINT_TYPE_TEAMMATE,
		POINT_TYPE_POWERUP,
		POINT_TYPE_ITEM
	} pointtype;
	float	pointtime;
} tvars_t;

static tvars_t vars;
#endif


typedef struct item_vis_s {
	vec3_t	vieworg;
	vec3_t	forward;
	vec3_t	right;
	vec3_t	up;
	vec3_t	entorg;
	float	radius;
	vec3_t	dir;
	float	dist;
} item_vis_t;


#define	TP_TOOK_EXPIRE_TIME		15
#define	TP_POINT_EXPIRE_TIME	TP_TOOK_EXPIRE_TIME





//===========================================================================
//								TRIGGERS
//===========================================================================

void TP_ExecTrigger (char *s, qboolean indemos)
{
	char *astr;

	if (!cl_triggers.value)
		return;

	if (!indemos && cls.demoplayback)
		return;

	astr = Cmd_AliasExist(s, RESTRICT_LOCAL);
	if (astr)
	{
		char *p;
		qbool quote = false;

		for (p=astr ; *p ; p++)
		{
			if (*p == '"')
				quote = !quote;
			if (!quote && *p == ';')
			{
				// more than one command, add it to the command buffer
				Cbuf_AddText (astr, RESTRICT_LOCAL);
				Cbuf_AddText ("\n", RESTRICT_LOCAL);
				return;
			}
		}
		// a single line, so execute it right away
		Cmd_ExecuteString (astr, RESTRICT_LOCAL);
		return;
	}
}

/*
==========================================================================
						        HELPER FUNCTIONS
==========================================================================
*/
static int	TP_CountPlayers (void)
{
	int	i, count;

	count = 0;
	for (i = 0; i < cl.allocated_client_slots ; i++) {
		if (cl.players[i].name[0] && !cl.players[i].spectator)
			count++;
	}

	return count;
}

static char *TP_PlayerTeam (void)
{
	return cl.players[cl.playerview[SP].playernum].team;
}

static char *TP_EnemyTeam (void)
{
	int			i;
	static char	enemyteam[MAX_INFO_KEY];
	char *myteam = TP_PlayerTeam();

	for (i = 0; i < cl.allocated_client_slots ; i++) {
		if (cl.players[i].name[0] && !cl.players[i].spectator)
		{
			strcpy (enemyteam, cl.players[i].team);
			if (strcmp(myteam, cl.players[i].team) != 0)
				return enemyteam;
		}
	}
	return "";
}

static char *TP_PlayerName (void)
{
	return cl.players[cl.playerview[SP].playernum].name;
}


static char *TP_EnemyName (void)
{
	int			i;
	char		*myname;
	static char	enemyname[MAX_SCOREBOARDNAME];

	myname = TP_PlayerName ();

	for (i = 0; i < cl.allocated_client_slots ; i++) {
		if (cl.players[i].name[0] && !cl.players[i].spectator)
		{
			strcpy (enemyname, cl.players[i].name);
			if (!strcmp(enemyname, myname))
				return enemyname;
		}
	}
	return "";
}

static char *TP_MapName (void)
{
	return host_mapname.string;
}

char *TP_GenerateDemoName(void)
{
	if (cl.playerview[SP].spectator)
	{	// FIXME: if tracking a player, use his name
		return va ("spec_%s_%s",
						TP_PlayerName(),
						TP_MapName());
	}
	else
	{	// guess game type and write demo name
		int i = TP_CountPlayers();
		if (cl.teamplay && i >= 3)
		{	// Teamplay
			return va ("%s_%s_vs_%s_%s",
							TP_PlayerName(),
							TP_PlayerTeam(),
							TP_EnemyTeam(),
							TP_MapName());
		}
		else
		{
			if (i == 2)
			{	// Duel
				return va ("%s_vs_%s_%s",
								TP_PlayerName(),
								TP_EnemyName(),
								TP_MapName());
			}
			else if (i > 2)
			{	// FFA
				return va ("%s_ffa_%s",
					TP_PlayerName(),
					TP_MapName());
			}
			else
			{	// one player
				return va ("%s_%s",
					TP_PlayerName(),
					TP_MapName());
			}
		}
	}
}


/*
==========================================================================
						        MACRO FUNCTIONS
==========================================================================
*/

#define MAX_MACRO_VALUE	256
static char	macro_buf[MAX_MACRO_VALUE] = "";

#ifdef QUAKESTATS
// buffer-size-safe helper functions
//static void MacroBuf_strcat (char *str) {
//	strlcat (macro_buf, str, sizeof(macro_buf));
//}
static void MacroBuf_strcat_with_separator (char *str) {
	if (macro_buf[0])
		strlcat (macro_buf, tp_name_separator.string, sizeof(macro_buf));
	strlcat (macro_buf, str, sizeof(macro_buf));
}
#endif


static char *Macro_Latency (void)
{
	sprintf(macro_buf, "%i", Q_rint(cls.latency*1000));
	return macro_buf;
}

static char *Macro_Gamedir (void)
{
	extern char gamedirfile[];
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%s", gamedirfile);
	return macro_buf;
}

#ifdef QUAKESTATS
static char *Macro_Health (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_HEALTH]);
	return macro_buf;
}

static char *Macro_Armor (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_ARMOR]);
	return macro_buf;
}

static char *Macro_Shells (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_SHELLS]);
	return macro_buf;
}

static char *Macro_Nails (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_NAILS]);
	return macro_buf;
}

static char *Macro_Rockets (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_ROCKETS]);
	return macro_buf;
}

static char *Macro_Cells (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_CELLS]);
	return macro_buf;
}

static char *Macro_Ammo (void)
{
	sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_AMMO]);
	return macro_buf;
}

static char *Weapon_NumToString (int wnum)
{
	switch (wnum)
	{
	case IT_AXE:				return tp_name_axe.string;
	case IT_SHOTGUN:			return tp_name_sg.string;
	case IT_SUPER_SHOTGUN:		return tp_name_ssg.string;
	case IT_NAILGUN:			return tp_name_ng.string;
	case IT_SUPER_NAILGUN:		return tp_name_sng.string;
	case IT_GRENADE_LAUNCHER:	return tp_name_gl.string;
	case IT_ROCKET_LAUNCHER:	return tp_name_rl.string;
	case IT_LIGHTNING:			return tp_name_lg.string;
	default:					return tp_name_none.string;
	}
}

static char *Macro_Weapon (void)
{
	return Weapon_NumToString(cl.playerview[SP].stats[STAT_ACTIVEWEAPON]);
}

static char *Macro_DroppedWeapon (void)
{
	return Weapon_NumToString(vars.droppedweapon);
}

static char *Macro_Weapons (void) {
	macro_buf[0] = 0;

	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_LIGHTNING)
		strcpy(macro_buf, tp_name_lg.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)
		MacroBuf_strcat_with_separator (tp_name_rl.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_GRENADE_LAUNCHER)
		MacroBuf_strcat_with_separator (tp_name_gl.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_NAILGUN)
		MacroBuf_strcat_with_separator (tp_name_sng.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_NAILGUN)
		MacroBuf_strcat_with_separator (tp_name_ng.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_SHOTGUN)
		MacroBuf_strcat_with_separator (tp_name_ssg.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SHOTGUN)
		MacroBuf_strcat_with_separator (tp_name_sg.string);
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_AXE)
		MacroBuf_strcat_with_separator (tp_name_axe.string);
//	if (!macro_buf[0])
//		strlcpy(macro_buf, tp_name_none.string, sizeof(macro_buf));

	return macro_buf;
}

static char *Macro_WeaponAndAmmo (void)
{
	char buf[sizeof(macro_buf)];
	Q_snprintfz (buf, sizeof(buf), "%s:%s", Macro_Weapon(), Macro_Ammo());
	strcpy (macro_buf, buf);
	return macro_buf;
}

static char *Macro_WeaponNum (void)
{
	switch (cl.playerview[SP].stats[STAT_ACTIVEWEAPON])
	{
	case IT_AXE: return "1";
	case IT_SHOTGUN: return "2";
	case IT_SUPER_SHOTGUN: return "3";
	case IT_NAILGUN: return "4";
	case IT_SUPER_NAILGUN: return "5";
	case IT_GRENADE_LAUNCHER: return "6";
	case IT_ROCKET_LAUNCHER: return "7";
	case IT_LIGHTNING: return "8";
	default:
		return "0";
	}
}

static int	_Macro_BestWeapon (void)
{
	int i;
	char *t[] = {tp_weapon_order.string, "78654321", NULL}, **s;

	for (s = t; *s; s++)
	{
		for (i = 0 ; i < strlen(*s) ; i++)
		{
			switch ((*s)[i]) {
				case '1': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_AXE) return IT_AXE; break;
				case '2': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SHOTGUN) return IT_SHOTGUN; break;
				case '3': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_SHOTGUN) return IT_SUPER_SHOTGUN; break;
				case '4': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_NAILGUN) return IT_NAILGUN; break;
				case '5': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_NAILGUN) return IT_SUPER_NAILGUN; break;
				case '6': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_GRENADE_LAUNCHER) return IT_GRENADE_LAUNCHER; break;
				case '7': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER) return IT_ROCKET_LAUNCHER; break;
				case '8': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_LIGHTNING) return IT_LIGHTNING; break;
			}
		}
	}
	return 0;
}

static char *Macro_BestWeapon (void)
{
	return Weapon_NumToString(_Macro_BestWeapon());
}

static char *Macro_BestAmmo (void)
{
	switch (_Macro_BestWeapon())
	{
	case IT_SHOTGUN: case IT_SUPER_SHOTGUN:
		sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_SHELLS]);
		return macro_buf;

	case IT_NAILGUN: case IT_SUPER_NAILGUN:
		sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_NAILS]);
		return macro_buf;

	case IT_GRENADE_LAUNCHER: case IT_ROCKET_LAUNCHER:
		sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_ROCKETS]);
		return macro_buf;

	case IT_LIGHTNING:
		sprintf(macro_buf, "%i", cl.playerview[SP].stats[STAT_CELLS]);
		return macro_buf;

	default:
		return "0";
	}
}

// needed for %b parsing
static char *Macro_BestWeaponAndAmmo (void)
{
	char buf[MAX_MACRO_VALUE];
	Q_snprintfz (buf, sizeof(buf), "%s:%s", Macro_BestWeapon(), Macro_BestAmmo());
	strcpy (macro_buf, buf);
	return macro_buf;
}

static char *Macro_ArmorType (void)
{
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR1)
		return tp_name_armortype_ga.string;
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR2)
		return tp_name_armortype_ya.string;
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR3)
		return tp_name_armortype_ra.string;
	else
		return tp_name_none.string;	// no armor at all
}

static char *Macro_Powerups (void)
{
	int effects;

	macro_buf[0] = 0;

	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_QUAD)
		MacroBuf_strcat_with_separator (tp_name_quad.string);

	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVULNERABILITY)
		MacroBuf_strcat_with_separator (tp_name_pent.string);

	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVISIBILITY)
		MacroBuf_strcat_with_separator (tp_name_ring.string);

	effects = cl.inframes[cl.parsecount&UPDATE_MASK].playerstate[cl.playerview[SP].playernum].effects;
	if ( (effects & (QWEF_FLAG1|QWEF_FLAG2)) ||		// CTF
		(cl.teamfortress && cl.playerview[SP].stats[STAT_ITEMS] & (IT_KEY1|IT_KEY2)) ) // TF
		MacroBuf_strcat_with_separator (tp_name_flag.string);

	return macro_buf;
}
#endif

static char *Macro_Location (void)
{
	return TP_LocationName (cl.playerview[SP].simorg);
}

#ifdef QUAKESTATS
static char *Macro_LastDeath (void)
{
	if (vars.deathtrigger_time)
		return vars.lastdeathloc;
	else
		return tp_name_someplace.string;
}

static char *Macro_Last_Location (void)
{
	if (vars.deathtrigger_time && realtime - vars.deathtrigger_time <= 5)
		return vars.lastdeathloc;
	return Macro_Location();
}

// returns the last item picked up
static char *Macro_Took (void)
{
	if (!vars.tooktime || realtime > vars.tooktime + 20)
		strlcpy (macro_buf, tp_name_nothing.string, sizeof(macro_buf));
	else
		strcpy (macro_buf, vars.tookname);
	return macro_buf;
}

// returns location of the last item picked up
static char *Macro_TookLoc (void)
{
	if (!vars.tooktime || realtime > vars.tooktime + 20)
		strlcpy (macro_buf, tp_name_someplace.string, sizeof(macro_buf));
	else
		strcpy (macro_buf, vars.tookloc);
	return macro_buf;
}


// %i macro - last item picked up in "name at location" style
static char *Macro_TookAtLoc (void)
{
	if (!vars.tooktime || realtime > vars.tooktime + 20)
		strncpy (macro_buf, tp_name_nothing.string, sizeof(macro_buf)-1);
	else
	{
		strlcpy (macro_buf, va("%s %s %s", vars.tookname,
			tp_name_at.string, vars.tookloc), sizeof(macro_buf));
	}
	return macro_buf;
}

// pointing calculations are CPU expensive, so the results are cached
// in vars.pointname & vars.pointloc
static char *Macro_PointName (void)
{
	if (cls.framecount != vars.pointframe)
		TP_FindPoint ();
	return vars.pointname;
}

static char *Macro_PointLocation (void)
{
	if (cls.framecount != vars.pointframe)
		TP_FindPoint ();
	if (vars.pointloc[0])
		return vars.pointloc;
	else {
		strlcpy (macro_buf, tp_name_someplace.string, sizeof(macro_buf));
		return macro_buf;
	}
}

static char *Macro_PointNameAtLocation (void)
{
	if (cls.framecount != vars.pointframe)
		TP_FindPoint ();
	if (vars.pointloc[0])
		return va ("%s %s %s", vars.pointname, tp_name_at.string, vars.pointloc);
	else
		return vars.pointname;
}


static char *Macro_Need (void)
{
	int i, weapon;
	char	*needammo = NULL;

	macro_buf[0] = 0;

	// check armor
	if (   ((cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR1) && cl.playerview[SP].stats[STAT_ARMOR] < tp_need_ga.value)
		|| ((cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR2) && cl.playerview[SP].stats[STAT_ARMOR] < tp_need_ya.value)
		|| ((cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR3) && cl.playerview[SP].stats[STAT_ARMOR] < tp_need_ra.value)
		|| (!(cl.playerview[SP].stats[STAT_ITEMS] & (IT_ARMOR1|IT_ARMOR2|IT_ARMOR3))
			&& (tp_need_ga.value || tp_need_ya.value || tp_need_ra.value)))
		strcpy (macro_buf, tp_name_armor.string);

	// check health
	if (tp_need_health.value && cl.playerview[SP].stats[STAT_HEALTH] < tp_need_health.value) {
		MacroBuf_strcat_with_separator (tp_name_health.string);
	}

	if (cl.teamfortress)
	{
		// in TF, we have all weapons from the start,
		// and ammo is checked differently
		if (cl.playerview[SP].stats[STAT_ROCKETS] < tp_need_rockets.value)
			MacroBuf_strcat_with_separator (tp_name_rockets.string);
		if (cl.playerview[SP].stats[STAT_SHELLS] < tp_need_shells.value)
			MacroBuf_strcat_with_separator (tp_name_shells.string);
		if (cl.playerview[SP].stats[STAT_NAILS] < tp_need_nails.value)
			MacroBuf_strcat_with_separator (tp_name_nails.string);
		if (cl.playerview[SP].stats[STAT_CELLS] < tp_need_cells.value)
			MacroBuf_strcat_with_separator (tp_name_cells.string);
		goto done;
	}

	// check weapon
	weapon = 0;
	for (i=strlen(tp_need_weapon.string)-1 ; i>=0 ; i--) {
		switch (tp_need_weapon.string[i]) {
			case '2': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SHOTGUN) weapon = 2; break;
			case '3': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_SHOTGUN) weapon = 3; break;
			case '4': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_NAILGUN) weapon = 4; break;
			case '5': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_SUPER_NAILGUN) weapon = 5; break;
			case '6': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_GRENADE_LAUNCHER) weapon = 6; break;
			case '7': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER) weapon = 7; break;
			case '8': if (cl.playerview[SP].stats[STAT_ITEMS] & IT_LIGHTNING) weapon = 8; break;
		}
		if (weapon)
			break;
	}

	if (!weapon) {
		MacroBuf_strcat_with_separator (tp_name_weapon.string);
	} else {
		if (tp_need_rl.value && !(cl.playerview[SP].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)) {
			MacroBuf_strcat_with_separator (tp_name_rl.string);
		}

		switch (weapon) {
			case 2: case 3: if (cl.playerview[SP].stats[STAT_SHELLS] < tp_need_shells.value) needammo = tp_name_shells.string; break;
			case 4: case 5: if (cl.playerview[SP].stats[STAT_NAILS] < tp_need_nails.value) needammo = tp_name_nails.string; break;
			case 6: case 7: if (cl.playerview[SP].stats[STAT_ROCKETS] < tp_need_rockets.value) needammo = tp_name_rockets.string; break;
			case 8: if (cl.playerview[SP].stats[STAT_CELLS] < tp_need_cells.value) needammo = tp_name_cells.string; break;
		}
		if (needammo) {
			MacroBuf_strcat_with_separator (needammo);
		}
	}

done:
	if (!macro_buf[0])
		strcpy (macro_buf, "nothing");

	return macro_buf;
}

static char *Skin_To_TFSkin (char *myskin)
{
	playerview_t *pv = &cl.playerview[SP];
	if (!cl.teamfortress || pv->spectator || Q_strncasecmp(myskin, "tf_", 3))
	{
		Q_strncpyz(macro_buf, myskin, sizeof(macro_buf));
	}
	else
	{
		if (!Q_strcasecmp(myskin, "tf_demo"))
			Q_strncpyz(macro_buf, "demoman", sizeof(macro_buf));
		else if (!Q_strcasecmp(myskin, "tf_eng"))
			Q_strncpyz(macro_buf, "engineer", sizeof(macro_buf));
		else if (!Q_strcasecmp(myskin, "tf_snipe"))
			Q_strncpyz(macro_buf, "sniper", sizeof(macro_buf));
		else if (!Q_strcasecmp(myskin, "tf_sold"))
			Q_strncpyz(macro_buf, "soldier", sizeof(macro_buf));
		else
			Q_strncpyz(macro_buf, myskin + 3, sizeof(macro_buf));
	}
	return macro_buf;
}

static char *Macro_TF_Skin (void)
{
	return Skin_To_TFSkin(InfoBuf_ValueForKey(&cl.players[cl.playerview[SP].playernum].userinfo, "skin"));
}

//Spike: added these:
static char *Macro_Team (void)
{
	infobuf_t *info;
	int seat = SP;
	//read the userinfo's team from the server instead of our local/private cvar/userinfo, if we can.
	if (cl.players[cl.playerview[seat].playernum].userinfovalid)
		info = &cl.players[cl.playerview[seat].playernum].userinfo;
	else //just use the userinfo (which should mirror the cvar - fixme: splitscreen...)
		info = &cls.userinfo[seat];
	return InfoBuf_ValueForKey(info, "team");
}
static char *Macro_ConnectionType (void)
{
	playerview_t *pv = &cl.playerview[SP];
	if (!cls.state)
		return "disconnected";
	if (pv->spectator)
		return "spectator";
	return "connected";
}

static char *Macro_demoplayback (void)
{
	switch (cls.demoplayback)
	{
	case DPB_NONE:
		return "0";
	case DPB_QUAKEWORLD:
		return "qwdplayback";
	case DPB_MVD:
		return "mvdplayback";
#ifdef NQPROT
	case DPB_NETQUAKE:
		return "demplayback";
#endif
#ifdef Q2CLIENT
	case DPB_QUAKE2:
		return "dm2playback";
#endif
	}
	return "1";	//unknown.
}

static char *Macro_Match_Name (void)
{
	int i;
	i = TP_CountPlayers();
	if (cl.teamplay && i >= 3)
	{	// Teamplay
		return va ("%s %s vs %s - %s",
			TP_PlayerName(),
			TP_PlayerTeam(),
			TP_EnemyTeam(),
			TP_MapName());
	}
	else
	{
		if (i == 2)
		{	// Duel
			return va ("%s vs %s - %s",
				TP_PlayerName(),
				TP_EnemyName(),
				TP_MapName());
		}
		else if (i > 2)
		{	// FFA
			return va ("%s ffa - %s",
				TP_PlayerName(),
				TP_MapName());
		}
		else
		{	// one player
			return va ("%s - %s",
				TP_PlayerName(),
				TP_MapName());
		}
	}
}

//$matchtype
//duel,2on2,4on4,ffa,etc...
static char *Macro_Match_Type (void)
{
	int i;
	i = TP_CountPlayers();
	if (cl.teamplay && i >= 3)
	{
		if (i >= 6)
			return "4on4";
		return "2on2";
	}
	if (i == 2)
		return "duel";
	if (i == 1)
		return "single";
	if (i == 0)
		return "empty";
	return "ffa";
}

static char *Macro_Point_LED(void)
{
	TP_FindPoint();

	if (vars.pointtype == POINT_TYPE_ENEMY)
		return tp_name_status_red.string;
	else if (vars.pointtype == POINT_TYPE_TEAMMATE)
		return tp_name_status_green.string;
	else if (vars.pointtype == POINT_TYPE_POWERUP)
		return tp_name_status_yellow.string;
	else	// POINT_TYPE_ITEM
		return tp_name_status_blue.string;

	return macro_buf;
}

static char *Macro_MyStatus_LED(void)
{
	int count;
	float save_need_rl;
	char *s, *save_separator;
	static char separator[] = {'/', '\0'};

	save_need_rl = tp_need_rl.value;
	save_separator = tp_name_separator.string;
	tp_need_rl.value = 0;
	tp_name_separator.string = separator;
	s = Macro_Need();
	tp_need_rl.value = save_need_rl;
	tp_name_separator.string = save_separator;

	if (!strcmp(s, tp_name_nothing.string)) {
		count = 0;
	} else  {
		for (count = 1; *s; s++)
			if (*s == separator[0])
				count++;
	}

	if (count == 0)
		Q_snprintfz(macro_buf, sizeof(macro_buf), "%s", tp_name_status_green.string);
	else if (count <= 1)
		Q_snprintfz(macro_buf, sizeof(macro_buf), "%s", tp_name_status_yellow.string);
	else
		Q_snprintfz(macro_buf, sizeof(macro_buf), "%s", tp_name_status_red.string);

	return macro_buf;
}

static void CountNearbyPlayers(qboolean dead)
{
	int i;
	player_state_t *state;
	player_info_t *info;
	static int lastframecount = -1;
	playerview_t *pv = &cl.playerview[SP];

	if (cls.framecount == lastframecount)
		return;
	lastframecount = cls.framecount;

	vars.numenemies = vars.numfriendlies = 0;

	if (!pv->spectator && !dead)
		vars.numfriendlies++;

	if (!cl.oldparsecount || !cl.parsecount || cls.state < ca_active)
		return;

	state = cl.inframes[cl.oldparsecount & UPDATE_MASK].playerstate;
	info = cl.players;
	for (i = 0; i < cl.allocated_client_slots; i++, info++, state++) {
		if (i != pv->playernum && state->messagenum == cl.oldparsecount && !info->spectator && !ISDEAD(state->frame)) {
			if (cl.teamplay && !strcmp(info->team, TP_PlayerTeam()))
				vars.numfriendlies++;
			else
				vars.numenemies++;
		}
	}
}

static char *Macro_CountNearbyEnemyPlayers (void)
{
	if (!ruleset_allow_playercount.ival)
		return " ";
	CountNearbyPlayers(false);
	sprintf(macro_buf, "\xffz%d\xff", vars.numenemies);
	suppress = true;
	return macro_buf;
}


static char *Macro_Count_Last_NearbyEnemyPlayers (void)
{
	if (!ruleset_allow_playercount.ival)
		return " ";
	if (vars.deathtrigger_time && realtime - vars.deathtrigger_time <= 5)
	{
		sprintf(macro_buf, "\xffz%d\xff", vars.last_numenemies);
	}
	else
	{
		CountNearbyPlayers(false);
		sprintf(macro_buf, "\xffz%d\xff", vars.numenemies);
	}
	suppress = true;
	return macro_buf;
}


static char *Macro_CountNearbyFriendlyPlayers (void)
{
	if (!ruleset_allow_playercount.ival)
		return " ";
	CountNearbyPlayers(false);
	sprintf(macro_buf, "\xffz%d\xff", vars.numfriendlies);
	suppress = true;
	return macro_buf;
}


static char *Macro_Count_Last_NearbyFriendlyPlayers (void)
{
	if (!ruleset_allow_playercount.ival)
		return " ";
	if (vars.deathtrigger_time && realtime - vars.deathtrigger_time <= 5)
	{
		sprintf(macro_buf, "\xffz%d\xff", vars.last_numfriendlies);
	}
	else
	{
		CountNearbyPlayers(false);
		sprintf(macro_buf, "\xffz%d\xff", vars.numfriendlies);
	}
	suppress = true;
	return macro_buf;
}

static char *Macro_EnemyStatus_LED(void)
{
	CountNearbyPlayers(false);
	if (vars.numenemies == 0)
		Q_snprintfz(macro_buf, sizeof(macro_buf), "\xffl%s\xff", tp_name_status_green.string);
	else if (vars.numenemies <= vars.numfriendlies)
		Q_snprintfz(macro_buf, sizeof(macro_buf), "\xffl%s\xff", tp_name_status_yellow.string);
	else
		Q_snprintfz(macro_buf, sizeof(macro_buf), "\xffl%s\xff", tp_name_status_red.string);

	suppress = true;
	return macro_buf;
}

static char *Macro_LastPointAtLoc (void)
{
	if (!vars.pointtime || realtime - vars.pointtime > TP_POINT_EXPIRE_TIME)
		Q_strncpyz (macro_buf, tp_name_nothing.string, sizeof(macro_buf));
	else
		Q_snprintfz (macro_buf, sizeof(macro_buf), "%s %s %s", vars.pointname, tp_name_at.string, vars.pointloc[0] ? vars.pointloc : Macro_Location());
	return macro_buf;
}

static char *Macro_LastTookOrPointed (void)
{
	if (vars.tooktime && vars.tooktime > vars.pointtime && realtime - vars.tooktime < 5)
		return Macro_TookAtLoc();
	else if (vars.pointtime && vars.tooktime <= vars.pointtime && realtime - vars.pointtime < 5)
		return Macro_LastPointAtLoc();

	Q_snprintfz(macro_buf, sizeof(macro_buf), "%s %s %s", tp_name_nothing.string, tp_name_at.string, tp_name_someplace.string);
    return macro_buf;
}

#define TP_PENT 1
#define TP_QUAD 2
#define TP_RING 4

static char *Macro_LastSeenPowerup(void)
{
/*	if (!vars.enemy_powerups_time || realtime - vars.enemy_powerups_time > 5)
	{
		Q_strncpyz(macro_buf, tp_name_quad.string, sizeof(macro_buf));
	}
	else*/
	{
		macro_buf[0] = 0;
		if (vars.enemy_powerups & TP_QUAD)
			Q_strncatz2(macro_buf, tp_name_quad.string);
		if (vars.enemy_powerups & TP_PENT)
		{
			if (macro_buf[0])
				Q_strncatz2(macro_buf, tp_name_separator.string);
			Q_strncatz2(macro_buf, tp_name_pent.string);
		}
		if (vars.enemy_powerups & TP_RING)
		{
			if (macro_buf[0])
				Q_strncatz2(macro_buf, tp_name_separator.string);
			Q_strncatz2(macro_buf, tp_name_ring.string);
		}
	}
	return macro_buf;
}

static char *Macro_LastDrop (void)
{
	if (vars.lastdrop_time)
		return vars.lastdroploc;
	else
		return tp_name_someplace.string;
}

static char *Macro_LastDropTime (void)
{
	if (vars.lastdrop_time)
		Q_snprintfz (macro_buf, 32, "%d", (int) (realtime - vars.lastdrop_time));
	else
		Q_snprintfz (macro_buf, 32, "%d", -1);
	return macro_buf;
}

static char *Macro_CombinedHealth(void)
{
	float h;
	float t, a, m;
	//total health = health+armour*armourfrac
	//however,you're dead if health drops below 0 rather than the entire equation.

	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR1)
		t = 0.3;
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR2)
		t = 0.6;
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR3)
		t = 0.8;
	else
		t = 0;
	a = cl.playerview[SP].stats[STAT_ARMOR];
	h = cl.playerview[SP].stats[STAT_HEALTH];

	//work out the max useful armour
	//this will under-exagurate, due to usage of ceil based on damage
	m = h/(1-t);
	if (m < 0)
		a = 0;
	else if (m < a)
		a = m;

	h = h + a;
	Q_snprintfz (macro_buf, 32, "%d", (int)h);
	return macro_buf;
}

static char *Macro_Coloured_Armour(void)
{
	if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR3)
		return "{^s^xe00%%a^r}";
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR2)
		return "{^s^xff0%%a^r}";
	else if (cl.playerview[SP].stats[STAT_ITEMS] & IT_ARMOR1)
		return "{^s^x0b0%%a^r}";
	else
		return "{0}";
}

static char *Macro_Coloured_Powerups(void)
{
	char *quad, *pent, *ring;
	quad = (cl.playerview[SP].stats[STAT_ITEMS] & IT_QUAD)				?va("^x03f%s", tp_name_quad.string):"";
	pent = (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVULNERABILITY)	?va("^xe00%s", tp_name_pent.string):"";
	ring = (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVISIBILITY)		?va("^xff0%s", tp_name_ring.string):"";

	if (*quad || *pent || *ring)
	{
		Q_snprintfz (macro_buf, 32, "{^s%s%s%s^r}", quad, pent, ring);
		return macro_buf;
	}
	else
		return "";
}
static char *Macro_Coloured_Short_Powerups(void)
{
	char *quad, *pent, *ring;
	quad = (cl.playerview[SP].stats[STAT_ITEMS] & IT_QUAD)				?"^x03fq":"";
	pent = (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVULNERABILITY)	?"^xe00p":"";
	ring = (cl.playerview[SP].stats[STAT_ITEMS] & IT_INVISIBILITY)		?"^xff0r":"";

	if (*quad || *pent || *ring)
	{
		Q_snprintfz (macro_buf, 32, "{^s%s%s%s^r}", quad, pent, ring);
		return macro_buf;
	}
	else
		return "";
}
static char *Macro_Match_Status(void)
{
	if (cls.state == ca_disconnected)
		return "disconnected";
	if (cls.state < ca_active)
		return "connecting";
	switch(cl.matchstate)
	{
	case MATCH_DONTKNOW:
	case MATCH_INPROGRESS:
	default:
		return "normal";
	case MATCH_COUNTDOWN:
		return "countdown";
	case MATCH_STANDBY:
		return "standby";
	}
}
/*static char *Macro_LastIP(void)
{	//report the last ip that someone said in chat. requires making guesses about what's an ip or not. can't handle hostnames properly
	return "---";
}
static char *Macro_MP3Info(void)
{	//for people trying to be cool but really just annoying everyone
	return "---";
}
*/
static char *Macro_LastTrigger_Match(void)
{	//returns the last line that triggered a msg_trigger
	return vars.lasttrigger_match;
}
#endif

/*
$matchname
you can use to get the name of the match
manually (echo $matchname).
Example: a matchname might be
"[clan]quaker - [4on4_myclan_vs_someclan] - [dm3]" or whatever.

$matchstatus
("disconnected", "standby" or "normal"). This can be
used for detecting prewar/prematch on ktpro/oztf servers.

$mp3info
Evaluates to "author - title".
Examples:
if you bind space "say listening to $mp3info"
then hitting space will say something like
"listening to disturbed - rise".
bind x "if disturbed isin $mp3info then say dde music is cool"

$triggermatch
$triggermatch is the last chat message that exec'd a msg_trigger.

*/
//Spike: added end.

static void TP_InitMacros(void)
{
	Cmd_AddMacro("latency", Macro_Latency, false);
	Cmd_AddMacro("location", Macro_Location, false);
#ifdef QUAKESTATS
	Cmd_AddMacro("health", Macro_Health, true);
	Cmd_AddMacro("armortype", Macro_ArmorType, true);
	Cmd_AddMacro("armor", Macro_Armor, true);
	Cmd_AddMacro("shells", Macro_Shells, true);
	Cmd_AddMacro("nails", Macro_Nails, true);
	Cmd_AddMacro("rockets", Macro_Rockets, true);
	Cmd_AddMacro("cells", Macro_Cells, true);
	Cmd_AddMacro("weaponnum", Macro_WeaponNum, true);
	Cmd_AddMacro("weapons", Macro_Weapons, true);
	Cmd_AddMacro("weapon", Macro_Weapon, true);
	Cmd_AddMacro("ammo", Macro_Ammo, true);
	Cmd_AddMacro("bestweapon", Macro_BestWeapon, true);
	Cmd_AddMacro("bestammo", Macro_BestAmmo, true);
	Cmd_AddMacro("powerups", Macro_Powerups, true);
	Cmd_AddMacro("droppedweapon", Macro_DroppedWeapon, true);
	Cmd_AddMacro("tf_skin", Macro_TF_Skin, true);
	Cmd_AddMacro("team", Macro_Team, true);	//confusing

	Cmd_AddMacro("deathloc", Macro_LastDeath, true);
	Cmd_AddMacro("tookatloc", Macro_TookAtLoc, true);
	Cmd_AddMacro("tookloc", Macro_TookLoc, true);
	Cmd_AddMacro("took", Macro_Took, true);

	//ones added by Spike, for fuhquake compatability
	Cmd_AddMacro("connectiontype", Macro_ConnectionType, false);
	Cmd_AddMacro("demoplayback", Macro_demoplayback, false);
	Cmd_AddMacro("point", Macro_PointName, true);
	Cmd_AddMacro("pointatloc", Macro_PointNameAtLocation, true);
	Cmd_AddMacro("pointloc", Macro_PointLocation, true);
	Cmd_AddMacro("matchname", Macro_Match_Name, false);
	Cmd_AddMacro("matchtype", Macro_Match_Type, false);
	Cmd_AddMacro("need", Macro_Need, true);
	Cmd_AddMacro("ledstatus", Macro_MyStatus_LED, true);
	Cmd_AddMacro("ledpoint", Macro_Point_LED, true);
	Cmd_AddMacro("droploc", Macro_LastDrop, true);
	Cmd_AddMacro("droptime", Macro_LastDropTime, true);

	Cmd_AddMacro("matchstatus", Macro_Match_Status, false);
	Cmd_AddMacro("triggermatch", Macro_LastTrigger_Match, false);
#endif
//	Cmd_AddMacro("mp3info", Macro_MP3Info, false);

	//new, fte only (at least when first implemented)
#ifdef QUAKESTATS
	Cmd_AddMacro("chealth", Macro_CombinedHealth, true);
#endif

	//added for ezquake compatability
//	Cmd_AddMacro("lastip", Macro_LastIP, false);
	Cmd_AddMacro("ping", Macro_Latency, false);
#ifdef QUAKESTATS
	Cmd_AddMacro("colored_armor", Macro_Coloured_Armour, true);	//*shudder*
	Cmd_AddMacro("colored_powerups", Macro_Coloured_Powerups, true);
	Cmd_AddMacro("colored_short_powerups", Macro_Coloured_Short_Powerups, true);
	Cmd_AddMacro("lastloc", Macro_Last_Location, true);
	Cmd_AddMacro("lastpowerup", Macro_LastSeenPowerup, true);
#endif
	Cmd_AddMacro("gamedir", Macro_Gamedir, false);
}

#define MAX_MACRO_STRING 1024

/*
=============
TP_ParseMacroString

Parses %a-like expressions
=============
*/
static char *TP_ParseMacroString (char *s)
{
	static char	buf[MAX_MACRO_STRING];
	int		i = 0;
	char	*macro_string;

	if (!cl_parseSay.ival)
		return s;

	while (*s && i < MAX_MACRO_STRING-1)
	{
		// check %[P], etc
		if (*s == '%' && s[1]=='[' && s[2] && s[3]==']')
		{
#ifdef QUAKESTATS
			static char mbuf[MAX_MACRO_VALUE];
#endif
			switch (s[2]) {
#ifdef QUAKESTATS
			case 'a':
				macro_string = Macro_ArmorType();
				if (!macro_string[0])
					macro_string = "a";
				if (cl.playerview[SP].stats[STAT_ARMOR] < 30)
					Q_snprintfz (mbuf, sizeof(mbuf), "\x10%s:%i\x11", macro_string, cl.playerview[SP].stats[STAT_ARMOR]);
				else
					Q_snprintfz (mbuf, sizeof(mbuf), "%s:%i", macro_string, cl.playerview[SP].stats[STAT_ARMOR]);
				macro_string = mbuf;
				break;

			case 'h':
				if (cl.playerview[SP].stats[STAT_HEALTH] >= 50)
					Q_snprintfz (macro_buf, sizeof(macro_buf), "%i", cl.playerview[SP].stats[STAT_HEALTH]);
				else
					Q_snprintfz (macro_buf, sizeof(macro_buf), "\x10%i\x11", cl.playerview[SP].stats[STAT_HEALTH]);
				macro_string = macro_buf;
				break;

			case 'p':
			case 'P':
				macro_string = Macro_Powerups();
				if (macro_string[0])
					Q_snprintfz (mbuf, sizeof(mbuf), "\x10%s\x11", macro_string);
				else
					mbuf[0] = 0;
				macro_string = mbuf;
				break;
#endif
				// todo: %[w], %[b]

			default:
				buf[i++] = *s++;
				continue;
			}
			if (i + strlen(macro_string) >= MAX_MACRO_STRING-1)
				Sys_Error("TP_ParseMacroString: macro string length > MAX_MACRO_STRING)");
			strcpy (&buf[i], macro_string);
			i += strlen(macro_string);
			s += 4;	// skip %[<char>]
			continue;
		}

		// check %a, etc
		if (*s == '%')
		{
			switch (s[1])
			{
#ifdef QUAKESTATS
				case 'a':	macro_string = Macro_Armor(); break;
				case 'A':	macro_string = Macro_ArmorType(); break;
				case 'b':	macro_string = Macro_BestWeaponAndAmmo(); break;
				case 'c':	macro_string = Macro_Cells(); break;
				case 'd':	macro_string = Macro_LastDeath(); break;
//				case 'D':
				case 'h':	macro_string = Macro_Health(); break;
				case 'i':	macro_string = Macro_TookAtLoc(); break;
				case 'j':	macro_string = Macro_LastPointAtLoc(); break;
				case 'k':	macro_string = Macro_LastTookOrPointed(); break;
				case 'L':	macro_string = Macro_Last_Location(); break;
#endif
				case 'l':	macro_string = Macro_Location(); break;
#ifdef QUAKESTATS
				case 'm':	macro_string = Macro_LastTookOrPointed(); break;

				case 'o':	macro_string = Macro_CountNearbyFriendlyPlayers(); break;
				case 'e':	macro_string = Macro_CountNearbyEnemyPlayers(); break;
				case 'O':	macro_string = Macro_Count_Last_NearbyFriendlyPlayers(); break;
				case 'E':	macro_string = Macro_Count_Last_NearbyEnemyPlayers(); break;

				case 'P':
				case 'p':	macro_string = Macro_Powerups(); break;
				case 'q':	macro_string = Macro_LastSeenPowerup(); break;
//				case 'r':	macro_string = Macro_LastReportedLoc(); break;
				case 's':	macro_string = Macro_EnemyStatus_LED(); break;
				case 'S':	macro_string = Macro_TF_Skin(); break;
				case 't':	macro_string = Macro_PointNameAtLocation(); break;
				case 'u':	macro_string = Macro_Need(); break;
				case 'w':	macro_string = Macro_WeaponAndAmmo(); break;
				case 'x':	macro_string = Macro_PointName(); break;
				case 'X':	macro_string = Macro_Took(); break;
				case 'y':	macro_string = Macro_PointLocation(); break;
				case 'Y':	macro_string = Macro_TookLoc(); break;
#endif

				case 'n':	//vicinity
				case 'N':	//hides from you
				case 'g':	//bonus timers.
				case 'C':	//colour
				case 'z':	//nearest waypoint (looking)
				case 'Z':	//nearest waypoint (moving)
				default:
					buf[i++] = *s++;
					continue;
			}
			if (i + strlen(macro_string) >= MAX_MACRO_STRING-1)
				Sys_Error("TP_ParseMacroString: macro string length > MAX_MACRO_STRING)");
			strcpy (&buf[i], macro_string);
			i += strlen(macro_string);
			s += 2;	// skip % and letter
			continue;
		}

		buf[i++] = *s++;
	}
	buf[i] = 0;

	return buf;
}


/*
==============
TP_ParseFunChars

Doesn't check for overflows, so strlen(s) should be < MAX_MACRO_STRING
==============
*/
const char *TP_ParseFunChars (const char *s)
{
	static char	 buf[MAX_MACRO_STRING];
	char		*out = buf;
	int			 c;

	if (!cl_parseFunChars.ival || com_parseutf8.ival != 0)
		return s;

	while (*s && out < buf+countof(buf)-1) {
		if (*s == '$' && s[1] == 'x') {
			int i;
			// check for $x10, $x8a, etc
			c = tolower((int)(unsigned char)s[2]);
			if ( isdigit(c) )
				i = (c - (int)'0') << 4;
			else if ( isxdigit(c) )
				i = (c - (int)'a' + 10) << 4;
			else goto skip;
			c = tolower((int)(unsigned char)s[3]);
			if ( isdigit(c) )
				i += (c - (int)'0');
			else if ( isxdigit(c) )
				i += (c - (int)'a' + 10);
			else goto skip;
			if (!i)
				i = (int)' ';
			*out++ = (char)i;
			s += 4;
			continue;
		}
		if (*s == '$' && s[1]) {
			c = 0;
			switch (s[1]) {
				case '\\': c = 0x0D; break;
				case ':': c = 0x0A; break;
				case '[': c = 0x10; break;
				case ']': c = 0x11; break;
				case 'G': c = 0x86; break;
				case 'R': c = 0x87; break;
				case 'Y': c = 0x88; break;
				case 'B': c = 0x89; break;
				case '(': c = 0x80; break;
				case '=': c = 0x81; break;
				case ')': c = 0x82; break;
				case 'a': c = 0x83; break;
				case '<': c = 0x1d; break;
				case '-': c = 0x1e; break;
				case '>': c = 0x1f; break;
				case ',': c = 0x1c; break;
				case '.': c = 0x9c; break;
				case 'b': c = 0x8b; break;
				case 'c':
				case 'd': c = 0x8d; break;
				case '$': c = '$'; break;
				case '^': c = '^'; break;
			}
			if ( isdigit((int)(unsigned char)s[1]) )
				c = s[1] - (int)'0' + 0x12;
			if (c) {
				*out++ = (char)c;
				s += 2;
				continue;
			}
		}
skip:
		*out++ = *s++;
	}
	*out = 0;

	return buf;
}

/*
=============================================================================

							PROXY .LOC FILES

=============================================================================
*/

typedef struct locdata_s {
	vec3_t min;
	vec3_t max;
	char name[MAX_LOC_NAME];
} locdata_t;

static locdata_t	*locdata;	// FIXME: allocate dynamically?
static size_t		loc_numentries;
static size_t		loc_maxentries;


static void TP_LoadLocFile (char *filename, qbool quiet)
{
	char	fullpath[MAX_QPATH];
	char	*buf, *p;
	char	line[1024];
	int		i, argc;
	int		errorcount = 0;
	locdata_t	*loc;
	char *comma, *space;

	if (!*filename)
		return;

	Q_snprintfz (fullpath, sizeof(fullpath) - 4, "locs/%s", filename);
	COM_DefaultExtension (fullpath, ".loc", sizeof(fullpath));

	buf = (char *) COM_LoadTempFile (fullpath, 0, NULL);
	if (!buf)
	{
		if (!quiet)
			Com_Printf ("Could not load %s\n", fullpath);
		return;
	}

	loc_numentries = 0;

	// parse the file
	// we rely on the fact that FS_Load*File always appends a 0 at the end
	p = buf;
	while (1)
	{
		if (!*p)
			break;		// end of file

		// get a line out
		for (i = 0; i < sizeof(line)-1; )
		{
			char c = *p++;
			if (!c || c == 10)
				break;
			if (c != 13)
				line[i++] = c;
		}
		line[i] = 0;

		comma = strchr(line, ',');
		space = strchr(line, ' ');

		if (comma && (comma < space || !space))
		{
			vec3_t min, max;
			
			min[0] = strtod(line, &comma);
			if (*comma++ == ',')
			{
				while(*comma == ' ' || *comma == '\t')
					comma++;
				min[1] = strtod(comma, &comma);
				if (*comma++ == ',')
				{
					while(*comma == ' ' || *comma == '\t')
						comma++;
					min[2] = strtod(comma, &comma);
					if (*comma++ == ',')
					{
						while(*comma == ' ' || *comma == '\t')
							comma++;
						max[0] = strtod(comma, &comma);
						if (*comma++ == ',')
						{
							while(*comma == ' ' || *comma == '\t')
								comma++;
							max[1] = strtod(comma, &comma);
							if (*comma++ == ',')
							{
								while(*comma == ' ' || *comma == '\t')
									comma++;
								max[2] = strtod(comma, &comma);
								if (*comma++ == ',')
								{
									if (loc_numentries == loc_maxentries)
										Z_ReallocElements((void**)&locdata, &loc_maxentries, loc_numentries+64, sizeof(*locdata));
									loc = &locdata[loc_numentries];
									loc_numentries++;

									while(*comma == ' ' || *comma == '\t')
										comma++;
									if (*comma == '\"')
										COM_ParseOut(comma, loc->name, sizeof(loc->name));
									else
										Q_strncpyz(loc->name, comma, sizeof(loc->name));

									for (i = 0; i < 3; i++)
									{
										if (min[i] > max[i])
										{
											loc->min[i] = max[i];
											loc->max[i] = min[i];
										}
										else
										{
											loc->min[i] = min[i];
											loc->max[i] = max[i];
										}
									}
								}
							}
						}
					}
				}
			}

			errorcount++;
		}
		else
		{
			Cmd_TokenizeString (line, true, false);

			argc = Cmd_Argc();
			if (!argc)
				continue;

			if (argc < 4)
			{
				errorcount++;
				continue;
			}

			if (atoi(Cmd_Argv(0)) == 0 && Cmd_Argv(0)[0] != '0')
			{
				// first token is not a number
				errorcount++;
				continue;
			}

			if (loc_numentries == loc_maxentries)
				Z_ReallocElements((void**)&locdata, &loc_maxentries, loc_numentries+64, sizeof(*locdata));
			loc = &locdata[loc_numentries];
			loc_numentries++;

			for (i = 0; i < 3; i++)
				loc->min[i] = loc->max[i] = atoi(Cmd_Argv(i)) / 8.0;

			loc->name[0] = 0;
			for (i = 3; i < argc; i++)
			{
				if (i != 3)
					Q_strncatz (loc->name, " ", sizeof(loc->name));
				Q_strncatz (loc->name, Cmd_Argv(i), sizeof(loc->name));
			}
		}
	}

	if (!quiet)
		Com_Printf ("Loaded %s (%lu points)\n", fullpath, (unsigned long)loc_numentries);
}

static void TP_LoadLocFile_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("loadloc <filename> : load a loc file\n");
		return;
	}

	TP_LoadLocFile (Cmd_Argv(1), false);
}
qboolean TP_HaveLocations(void)
{
	return loc_numentries>0;
}
char *TP_LocationName (const vec3_t location)
{
	int		i, j, minnum;
	float	dist, mindist;
	vec3_t	vec;
	static qbool	recursive;
	static char buf[MAX_LOC_NAME];
	int level;

	if (!loc_numentries || (cls.state != ca_active))
		return tp_name_someplace.string;

	if (recursive)
		return "";

	minnum = 0;
	mindist = 9999999;

	for (i = 0; i < loc_numentries; i++)
	{
		//clip the point to the box, to find the nearest point within it.
		for (j = 0; j < 3; j++)
			vec[j] = bound(locdata[i].min[j], location[j], locdata[i].max[j]);
		VectorSubtract (location, vec, vec);
		dist = VectorLength (vec);
		if (dist < mindist)
		{
			minnum = i;
			mindist = dist;

			//break out if we're actually inside it, to always favour the first.
			if (dist < 0.01)
				break;
		}
	}

	recursive = true;
	level = Cmd_ExecLevel;
	Cmd_ExpandString (locdata[minnum].name, buf, sizeof(buf), &level, false, true, false);
	recursive = false;

	return buf;
}

/*
=============================================================================

							MESSAGE TRIGGERS

=============================================================================
*/

// FIXME, we don't provide a way to remove triggers
// allocated heap memory is not freed when the engine shuts down

typedef struct msg_trigger_s {
	char	name[32];
	char	string[64];
	int		level;
	struct msg_trigger_s *next;
} msg_trigger_t;

static msg_trigger_t *msg_triggers;

static msg_trigger_t *TP_FindTrigger (char *name)
{
	msg_trigger_t *t;

	for (t=msg_triggers; t; t=t->next)
		if (!strcmp(t->name, name))
			return t;

	return NULL;
}


static void TP_MsgTrigger_f (void)
{
	int		c;
	char	*name;
	msg_trigger_t	*trig;

	if (Cmd_IsInsecure())
		return;

	c = Cmd_Argc();

	if (c > 5) {
		Com_Printf ("msg_trigger <trigger name> \"string\" [-l <level>]\n");
		return;
	}

	if (c == 1) {
		if (!msg_triggers)
			Com_Printf ("no triggers defined\n");
		else
		for (trig=msg_triggers; trig; trig=trig->next)
			Com_Printf ("%s : \"%s\"\n", trig->name, trig->string);
		return;
	}

	name = Cmd_Argv(1);
	if (strlen(name) > 31) {
		Com_Printf ("trigger name too long\n");
		return;
	}

	if (c == 2) {
		trig = TP_FindTrigger (name);
		if (trig)
			Com_Printf ("%s: \"%s\"\n", trig->name, trig->string);
		else
			Com_Printf ("trigger \"%s\" not found\n", name);
		return;
	}

	if (c >= 3) {
		if (strlen(Cmd_Argv(2)) >= countof(trig->string)) {
			Com_Printf ("trigger string too long\n");
			return;
		}

		trig = TP_FindTrigger (name);

		if (!trig) {
			// allocate new trigger
			trig = Z_Malloc (sizeof(msg_trigger_t));
			trig->next = msg_triggers;
			msg_triggers = trig;
			strcpy (trig->name, name);	// safe (length checked earlier)
			trig->level = PRINT_HIGH;
		}

		strcpy (trig->string, Cmd_Argv(2));	// safe (length checked earlier)
		if (c == 5 && !Q_stricmp (Cmd_Argv(3), "-l")) {
			if (!strcmp(Cmd_Argv(4), "t"))
				trig->level = 4;
			else {
				trig->level = Q_atoi (Cmd_Argv(4));
				if ((unsigned)trig->level > PRINT_CHAT)
					trig->level = PRINT_HIGH;
			}
		}
	}
}


void TP_SearchForMsgTriggers (char *s, int level)
{
	msg_trigger_t	*t;
	char *string;

	if (cls.demoplayback)
		return;

	if (!ruleset_allow_triggers.ival)
		return;

	for (t=msg_triggers; t; t=t->next)
		if ((t->level == level || (t->level == 3 && level == 4))
			&& t->string[0] && strstr(s, t->string))
		{
			if (level == PRINT_CHAT && (
				strstr (s, "_version") || strstr (s, "_system") ||
				strstr (s, "_speed") || strstr (s, "_modified") || strstr (s, "_ruleset")))
				continue; 	// don't let llamas fake proxy replies

			string = Cmd_AliasExist (t->name, RESTRICT_LOCAL);
			if (string)
			{
#ifdef QUAKESTATS
				Q_strncpyz(vars.lasttrigger_match, s, sizeof (vars.lasttrigger_match));
#endif
				Cbuf_AddText (string, RESTRICT_LOCAL);
				Cbuf_AddText ("\n", RESTRICT_LOCAL);
//				Cbuf_ExecuteLevel (RESTRICT_LOCAL);
			}
			else
				Com_Printf ("trigger \"%s\" has no matching alias\n", t->name);
		}
}

#ifdef QWSKINS
/*
=============================================================================
						TEAMCOLOR & ENEMYCOLOR
=============================================================================
*/

unsigned int		cl_teamtopcolor = ~0;
unsigned int		cl_teambottomcolor = ~0;
unsigned int		cl_enemytopcolor = ~0;
unsigned int		cl_enemybottomcolor = ~0;

static unsigned int TP_ForceColour(char *col)
{
	float rgb[3];
	unsigned int bitval;
	if (!strcmp(col, "off"))
		return ~0;
	if (!strncmp(col, "0x", 2))
	{
		if (strlen(col+2) == 3)
		{
			bitval = strtoul(col+2, NULL, 16);
			bitval = ((bitval & 0xf00)<<12) | ((bitval & 0x0f0)<<8) | ((bitval & 0x00f)<<4);
			bitval |= 0x01000000;
		}
		else
			bitval = 0x01000000 | strtoul(col+2, NULL, 16);
		if (bitval == ~0)
			bitval = 0x01ffffff;
		return bitval;
	}
	if (!strncmp(col, "x", 1))
	{
		if (strlen(col+1) == 3)
		{
			bitval = strtoul(col+1, NULL, 16);
			bitval = ((bitval & 0xf00)<<12) | ((bitval & 0x0f0)<<8) | ((bitval & 0x00f)<<4)
				   | ((bitval & 0xf00)<< 8) | ((bitval & 0x0f0)<<4) | ((bitval & 0x00f)<<0);
			bitval |= 0x01000000;
		}
		else
			bitval = 0x01000000 | strtoul(col+1, NULL, 16);
		if (bitval == ~0)
			bitval = 0x01ffffff;
		return bitval;
	}
	if (strchr(col, ' '))
	{
		SCR_StringToRGB(col, rgb, 1);
		bitval = ((unsigned char)rgb[0]<<0) | ((unsigned char)rgb[1]<<8) | ((unsigned char)rgb[2]<<16) | (0xff<<24);
		if (bitval == ~0)
			bitval = 0x01ffffff;
		return bitval;
	}
	return atoi(col);
}

colourised_t *TP_FindColours(char *name)
{
	colourised_t  *col;
	for (col = cls.colourised; col; col = col->next)
	{
		if (!strncmp(col->name, name, sizeof(col->name)-1))
		{
			return col;
		}
	}
	return NULL;
}

static void TP_Colourise_f (void)
{
	int i;
	unsigned char *topstr, *botstr;
	colourised_t *col, *last;
	if (Cmd_Argc() == 1)
	{
		return;
	}

	col = TP_FindColours(Cmd_Argv(1));

	if (Cmd_Argc() == 2)
	{
		if (col)
		{
			if (col == cls.colourised)
				cls.colourised = col->next;
			else
			{
				for (last = cls.colourised; last; last = last->next)
				{
					if (last->next == col)
					{
						last->next = col->next;
						break;
					}
				}
			}
			Z_Free(col);
		}
	}
	else
	{
		if (!col)
		{
			col = Z_Malloc(sizeof(*col));
			col->next = cls.colourised;
			cls.colourised = col;
			Q_strncpyz(col->name, Cmd_Argv(1), sizeof(col->skin));
		}

		topstr = Cmd_Argv(2);
		botstr = strchr(topstr, '.');
		if (botstr)
			*botstr++ = '\0';
		else
			botstr = topstr;

		col->topcolour = TP_ForceColour(topstr);
		col->bottomcolour = TP_ForceColour(botstr);
		Q_strncpyz(col->skin, Cmd_Argv(3), sizeof(col->skin));
	}

	Skin_FlushPlayers();
	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		cl.players[i].colourised = TP_FindColours(cl.players[i].name);
		CL_NewTranslation(i);
	}
}

static void TP_TeamColor_CB (struct cvar_s *var, char *oldvalue)
{
	unsigned int	top, bottom;
	int	i;
	char *n = COM_Parse(var->string);
	top = TP_ForceColour(com_token);
	COM_Parse(n);
	if (!*com_token)
		bottom = top;
	else {
		bottom = TP_ForceColour(com_token);
	}

	cl_teamtopcolor = top;
	cl_teambottomcolor = bottom;

	if (qrenderer != QR_NONE)	//make sure we have the renderer initialised...
		for (i = 0; i < cl.allocated_client_slots; i++)
			CL_NewTranslation(i);
}

static void TP_EnemyColor_CB (struct cvar_s *var, char *oldvalue)
{
	unsigned int	top, bottom;
	int	i;
	char *n = COM_Parse(var->string);
	top = TP_ForceColour(com_token);
	COM_Parse(n);
	if (!*com_token)
		bottom = top;
	else {
		bottom = TP_ForceColour(com_token);
	}

	cl_enemytopcolor = top;
	cl_enemybottomcolor = bottom;

	if (qrenderer != QR_NONE)	//make sure we have the renderer initialised...
		for (i = 0; i < cl.allocated_client_slots; i++)
			CL_NewTranslation(i);
}


static void QDECL TP_SkinCvar_Callback(struct cvar_s *var, char *oldvalue)
{
	Skin_FlushPlayers();
}

#endif

//===================================================================

void TP_NewMap (void)
{
	static char last_map[MAX_QPATH];
	char locname[MAX_QPATH];

#ifdef QUAKESTATS
	memset (&vars, 0, sizeof(vars));
	TP_FindModelNumbers ();
#endif

	// FIXME, just try to load the loc file no matter what?
	if (strcmp(host_mapname.string, last_map))
	{	// map name has changed
		loc_numentries = 0;	// clear loc file
		if (tp_loadlocs.value && cl.allocated_client_slots > 1)// && !cls.demoplayback)
		{
			Q_snprintfz (locname, sizeof(locname), "%s.loc", host_mapname.string);
			TP_LoadLocFile (locname, true);

			strlcpy (last_map, host_mapname.string, sizeof(last_map));
		}
		else
			strlcpy (last_map, "", sizeof(last_map));
	}

#ifdef QUAKESTATS
	TP_UpdateAutoStatus();
#endif
	TP_ExecTrigger ("f_newmap", false);
}

/*
======================
TP_CategorizeMessage

returns a combination of these values:
0 -- unknown
1 -- normal
2 -- team message
4 -- spectator
16 -- faked or serverside
Note that sometimes we can't be sure who really sent the message,
e.g. when there's a player "unnamed" in your team and "(unnamed)"
in the enemy team. The result will be 3 (1+2)

Never returns 2 if we are a spectator.

Now additionally returns player info (NULL if no player detected)
======================
*/
int TP_CategorizeMessage (char *s, int *offset, player_info_t **plr)
{
	int		i, msglen, len;
	int		flags;
	player_info_t	*player;
	char	*name;
	playerview_t *pv = &cl.playerview[SP];

	*offset = 0;
	*plr = NULL;

	flags = TPM_UNKNOWN;
	msglen = strlen(s);
	if (!msglen)
		return TPM_UNKNOWN;

	if ((s[0] == '^' && s[1] == '[') || (s[0] == '(' && s[1] == '^' && s[2] == '['))
	{
		char *end, *info;
		i = 0;
		for(info = s; *info; )
		{
			if (info[0] == '^' && info[1] == ']')
				break;
			if (*info == '\\')
				break;
			if (info[0] == '^' && info[1] == '^')
				info+=2;
			else
				info++;
		}
		for(end = info; *end; )
		{
			if (end[0] == '^' && end[1] == ']')
			{
				*end = 0;
				info = Info_ValueForKey(info, "player");
				if (*info)
					i = atoi(info)+1;
				*end = '^';
				break;
			}
			if (end[0] == '^' && end[1] == '^')
				end+=2;
			else
				end++;
		}
		if (!*end || i < 1 || i > cl.allocated_client_slots)
			return TPM_UNKNOWN;
		if (*s == '(')
		{
			if (end[2] != ')')
				return TPM_UNKNOWN;
			end+=3;
		}
		else
			end+=2;
		if (*end++ != ':')
			return TPM_UNKNOWN;
		if (*end++ != ' ')
			return TPM_UNKNOWN;
		*plr = player = &cl.players[i-1];
		*offset = end - s;

		if (*s == '(')
			flags = TPM_TEAM;
		else
		{
			if (player->spectator)
				flags |= TPM_SPECTATOR;
			else
				flags |= TPM_NORMAL;
		}
	}
	else
	{
		for (i=0, player=cl.players ; i < cl.allocated_client_slots ; i++, player++)
		{
			name = player->name;
			if (!(*name))
				continue;
			len = strlen(name);
			// check messagemode1
			if (len+2 <= msglen && s[len] == ':' && s[len+1] == ' '	&&
				!strncmp(name, s, len))
			{
				if (player->spectator)
					flags |= TPM_SPECTATOR;
				else
					flags |= TPM_NORMAL;
				*offset = len + 2;
				*plr = player;
			}
			// check messagemode2
			else if (s[0] == '(' && len+4 <= msglen &&
				!strncmp(s+len+1, "): ", 3) &&
				!strncmp(name, s+1, len))
			{
				// no team messages in teamplay 0, except for our own
				if (pv->spectator)
				{
					int track = Cam_TrackNum(pv);
					if (track>=0 && (i == track || ( cl.teamplay &&
						!strcmp(cl.players[track].team, player->team)) ))
					{
						flags |= TPM_OBSERVEDTEAM;
					}
				}
				else
				{
					if (i == pv->playernum || ( cl.teamplay &&
						!strcmp(cl.players[pv->playernum].team, player->team)) )
					{
						flags |= TPM_TEAM;
					}
				}

				*offset = len + 4;
				*plr = player;
			}
		}

//		if (i == cl.allocated_client_slots)
//			return flags;

	}
/*
	if (!flags) // search for fake/non player
	{
		if ((name = strstr(s, ": "))) // use name as temp
		{
			*offset = (name - s) + 2;
			flags = TPM_FAKED;

			if (msglen > 4 && *s == '(' && s[-1] == ')')
				flags |= TPM_TEAM;
		}
	}
*/

	if (!flags)
	{
		char *qtv = NULL;
		if (!strncmp(s, "#0:qtv_say:#", 12))
		{
			qtv = s+11;
			flags = TPM_QTV|TPM_SPECTATOR;
		}
		else if (!strncmp(s, "#0:qtv_say_game:#", 17))
		{
			qtv = s+16;
			flags = TPM_QTV|TPM_SPECTATOR;
		}
		else if (!strncmp(s, "#0:qtv_say_team_game:#", 22))
		{
			qtv = s+21;
			flags = TPM_QTV|TPM_TEAM|TPM_SPECTATOR;
		}
		if (flags)
		{
			*offset = (qtv - s);
			for (;;)
			{
				char *sub = qtv;
				if (*sub == '#')
				{
					strtoul(sub+1, &sub, 10);
					if (*sub++ == ':')
					{
						qtv = strstr(sub, ": ");
						if (qtv)
						{
							*offset = (sub - s);
							qtv += 2;
							continue;
						}
					}
				}
				break;
			}
		}
	}

	return flags;
}

#ifdef QUAKESTATS
//===================================================================
// Pickup triggers
//

// symbolic names used in tp_took, tp_pickup, tp_point commands
char *pknames[] = {"quad", "pent", "ring", "suit", "ra", "ya",	"ga",
"mh", "health", "lg", "rl", "gl", "sng", "ng", "ssg", "pack",
"cells", "rockets", "nails", "shells", "flag",
"teammate", "enemy", "eyes", "sentry", "disp", "runes"};

#define it_quad		(1 << 0)
#define it_pent		(1 << 1)
#define it_ring		(1 << 2)
#define it_suit		(1 << 3)
#define it_ra		(1 << 4)
#define it_ya		(1 << 5)
#define it_ga		(1 << 6)
#define it_mh		(1 << 7)
#define it_health	(1 << 8)
#define it_lg		(1 << 9)
#define it_rl		(1 << 10)
#define it_gl		(1 << 11)
#define it_sng		(1 << 12)
#define it_ng		(1 << 13)
#define it_ssg		(1 << 14)
#define it_pack		(1 << 15)
#define it_cells	(1 << 16)
#define it_rockets	(1 << 17)
#define it_nails	(1 << 18)
#define it_shells	(1 << 19)
#define it_flag		(1 << 20)
#define it_teammate	(1 << 21)
#define it_enemy	(1 << 22)
#define it_eyes		(1 << 23)
#define it_sentry   (1 << 24)
#define it_disp		(1 << 25)
#define it_runes	(1 << 26)
#define NUM_ITEMFLAGS 27

#define it_powerups	(it_quad|it_pent|it_ring)
#define it_weapons	(it_lg|it_rl|it_gl|it_sng|it_ng|it_ssg)
#define it_armor	(it_ra|it_ya|it_ga)
#define it_ammo		(it_cells|it_rockets|it_nails|it_shells)
#define it_players	(it_teammate|it_enemy|it_eyes)

#define default_pkflags (it_powerups|it_suit|it_armor|it_weapons|it_mh| \
				it_rockets|it_pack|it_flag)

#define default_tookflags (it_powerups|it_ra|it_ya|it_lg|it_rl|it_mh|it_flag)

#define default_pointflags (it_powerups|it_suit|it_armor|it_mh| \
				it_lg|it_rl|it_gl|it_sng|it_rockets|it_pack|it_flag|it_players)

int pkflags = default_pkflags;
int tookflags = default_tookflags;
int pointflags = default_pointflags;

static void FlagCommand (int *flags, int defaultflags) {
	int i, j, c, flag;
	char *p, str[255] = {0};
	qboolean removeflag = false;

	c = Cmd_Argc ();
	if (c == 1)	{
		if (!*flags)
			strcpy (str, "none");
		for (i = 0 ; i < NUM_ITEMFLAGS ; i++)
			if (*flags & (1 << i)) {
				if (*str)
					Q_strncatz (str, " ", sizeof(str));
				Q_strncatz (str, pknames[i], sizeof(str));
			}
		Com_Printf ("%s\n", str);
		return;
	}

	if (c == 2 && !Q_strcasecmp(Cmd_Argv(1), "none")) {
		*flags = 0;
		return;
	}

	if (*Cmd_Argv(1) != '+' && *Cmd_Argv(1) != '-')
		*flags = 0;

	for (i = 1; i < c; i++) {
		p = Cmd_Argv (i);
		if (*p == '+') {
			removeflag = false;
			p++;
		} else if (*p == '-') {
			removeflag = true;
			p++;
		}

		flag = 0;
		for (j=0 ; j<NUM_ITEMFLAGS ; j++) {
			if (!Q_strncasecmp (p, pknames[j], 3)) {
				flag = 1<<j;
				break;
			}
		}

		if (!flag) {
			if (!Q_strcasecmp (p, "armor"))
				flag = it_armor;
			else if (!Q_strcasecmp (p, "weapons"))
				flag = it_weapons;
			else if (!Q_strcasecmp (p, "powerups"))
				flag = it_powerups;
			else if (!Q_strcasecmp (p, "ammo"))
				flag = it_ammo;
			else if (!Q_strcasecmp (p, "players"))
				flag = it_players;
			else if (!Q_strcasecmp (p, "default"))
				flag = defaultflags;
			else if (!Q_strcasecmp (p, "all"))
				flag = (1<<NUM_ITEMFLAGS)-1;
		}


		if (flags != &pointflags)
			flag &= ~(it_sentry|it_disp|it_players);

		if (removeflag)
			*flags &= ~flag;
		else
			*flags |= flag;
	}
}

static void TP_Took_f (void)
{
	FlagCommand (&tookflags, default_tookflags);
}

void TP_Pickup_f (void)
{
	FlagCommand (&pkflags, default_pkflags);
}

static void TP_Point_f (void)
{
	FlagCommand (&pointflags, default_pointflags);
}


/*
// FIXME: maybe use sound indexes so we don't have to make strcmp's
// every time?

#define S_LOCK4		1	// weapons/lock4.wav
#define S_PKUP		2	// weapons/pkup.wav
#define S_HEALTH25	3	// items/health1.wav
#define S_HEALTH15	4	// items/r_item1.wav
#define S_MHEALTH	5	// items/r_item2.wav
#define S_DAMAGE	6	// items/damage.wav
#define S_EYES		7	// items/inv1.wav
#define S_PENT		8	// items/protect.wav
#define S_ARMOR		9	// items/armor1.wav

static char *tp_soundnames[] =
{
	"weapons/lock4.wav",
	"weapons/pkup.wav",
	"items/health1.wav",
	"items/r_item1.wav",
	"items/r_item2.wav",
	"items/damage.wav",
	"items/inv1.wav",
	"items/protect.wav"
	"items/armor1.wav"
};

#define TP_NUMSOUNDS (sizeof(tp_soundnames)/sizeof(tp_soundnames[0]))

int	sound_numbers[MAX_SOUNDS];

void TP_FindSoundNumbers (void)
{
	int		i, j;
	char	*s;
	for (i=0 ; i<MAX_SOUNDS ; i++)
	{
		s = &cl.sound_name[i];
		for (j=0 ; j<TP_NUMSOUNDS ; j++)
			...
	}
}
*/

typedef struct {
	int		itemflag;
	cvar_t	*cvar;
	char	*modelname;
	vec3_t	offset;		// offset of model graphics center
	float	radius;		// model graphics radius
	float	respawntime;// automatic respawn timer for mvds.
	int		flags;		// TODO: "NOPICKUP" (disp), "TEAMENEMY" (flag, disp)
} item_t;

static item_t	tp_items[] = {
	{	it_quad,	&tp_name_quad,	"progs/quaddama.mdl",
		{0, 0, 24},	25, 60
	},
	{	it_pent,	&tp_name_pent,	"progs/invulner.mdl",
		{0, 0, 22},	25, 5*60
	},
	{	it_ring,	&tp_name_ring,	"progs/invisibl.mdl",
		{0, 0, 16},	12, 5*60
	},
	{	it_suit,	&tp_name_suit,	"progs/suit.mdl",
		{0, 0, 24}, 20, 60
	},
	{	it_lg,		&tp_name_lg,	"progs/g_light.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_rl,		&tp_name_rl,	"progs/g_rock2.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_gl,		&tp_name_gl,	"progs/g_rock.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_sng,		&tp_name_sng,	"progs/g_nail2.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_ng,		&tp_name_ng,	"progs/g_nail.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_ssg,		&tp_name_ssg,	"progs/g_shot.mdl",
		{0, 0, 30},	20, 30
	},
	{	it_cells,	&tp_name_cells,	"maps/b_batt0.bsp",
		{16, 16, 24},	18, 30
	},
	{	it_cells,	&tp_name_cells,	"maps/b_batt1.bsp",
		{16, 16, 24},	18, 30
	},
	{	it_rockets,	&tp_name_rockets,"maps/b_rock0.bsp",
		{8, 8, 20},	18, 30
	},
	{	it_rockets,	&tp_name_rockets,"maps/b_rock1.bsp",
		{16, 8, 20},	18, 30
	},
	{	it_nails,	&tp_name_nails,	"maps/b_nail0.bsp",
		{16, 16, 10},	18, 30
	},
	{	it_nails,	&tp_name_nails,	"maps/b_nail1.bsp",
		{16, 16, 10},	18, 30
	},
	{	it_shells,	&tp_name_shells,"maps/b_shell0.bsp",
		{16, 16, 10},	18, 30
	},
	{	it_shells,	&tp_name_shells,"maps/b_shell1.bsp",
		{16, 16, 10},	18, 30
	},
	{	it_health,	&tp_name_health,"maps/b_bh10.bsp",
		{16, 16, 8},	18, 20
	},
	{	it_health,	&tp_name_health,"maps/b_bh25.bsp",
		{16, 16, 8},	18, 20
	},
	{	it_mh,		&tp_name_mh,	"maps/b_bh100.bsp",
		{16, 16, 14},	20, 0
	},
	{	it_pack,	&tp_name_backpack, "progs/backpack.mdl",
		{0, 0, 18},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/tf_flag.mdl",
		{0, 0, 14},	25, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/tf_stan.mdl",
		{0, 0, 45},	40, 0
	},
	{	it_ra|it_ya|it_ga, &tp_name_armor,	"progs/armor.mdl",	//generic armour, used only when the skin number if invalid for the other types.
		{0, 0, 24},	22, 0
	},
	{	it_ga,		&tp_name_ga,		"progs/armor.mdl",
		{0, 0, 24},	22, 20
	},
	{	it_ya,		&tp_name_ya,		"progs/armor.mdl",
		{0, 0, 24},	22, 20
	},
	{	it_ra,		&tp_name_ra,		"progs/armor.mdl",
		{0, 0, 24},	22, 20
	},
	{	it_flag,	&tp_name_flag,	"progs/w_g_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/w_s_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/m_g_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/m_s_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/b_s_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/b_g_key.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_flag,	&tp_name_flag,	"progs/flag.mdl",
		{0, 0, 14},	25, 0
	},
	{	it_runes,	&tp_name_rune_1,	"progs/end1.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_runes,	&tp_name_rune_2,	"progs/end2.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_runes,	&tp_name_rune_3,	"progs/end3.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_runes,	&tp_name_rune_4,	"progs/end4.mdl",
		{0, 0, 20},	18, 0
	},
	{	it_sentry, &tp_name_sentry, "progs/turrgun.mdl",
		{0, 0, 23},	25, 0
	},
	{	it_disp, &tp_name_disp,	"progs/disp.mdl",
		{0, 0, 24},	25, 0
	}
};

#define NUMITEMS (sizeof(tp_items) / sizeof(tp_items[0]))

static item_t	*model2item[MAX_PRECACHE_MODELS];

static void TP_FindModelNumbers (void)
{
	int		i, j;
	char	*s;
	item_t	*item;

	for (i=0 ; i<MAX_PRECACHE_MODELS ; i++) {
		model2item[i] = NULL;
		s = cl.model_name[i];
		if (!s)
			continue;
		for (j=0, item=tp_items ; j<NUMITEMS ; j++, item++)
			if (!strcmp(s, item->modelname))
			{
				model2item[i] = item;
				break;
			}
	}
}

// on success, result is non-zero
// on failure, result is zero
// for armors, returns skinnum+1 on success
static int FindNearestItem (vec3_t org, int flags, item_t **pitem)
{
	inframe_t		*frame;
	packet_entities_t	*pak;
	entity_state_t		*ent;
	int	i = 0, bestidx = 0, bestskin = 0;
	float bestdist = 0.0, dist = 0.0;
	vec3_t	v;
	item_t	*item;
	entity_state_t *baseline;

	// look in previous frame
	frame = &cl.inframes[cl.oldvalidsequence&UPDATE_MASK];
	pak = &frame->packet_entities;
	bestdist = 100.0f;
	bestidx = 0;
	*pitem = NULL;
	for (i=0,ent=pak->entities ; i<pak->num_entities ; i++,ent++)
	{
		item = model2item[ent->modelindex];
		if (!item)
			continue;
		if ( ! (item->itemflag & flags) )
			continue;

		VectorCopy(ent->origin, v);
		VectorSubtract (v, org, v);
		VectorAdd (v, item->offset, v);
		dist = VectorLength (v);
//		Com_Printf ("%s %f\n", item->modelname, dist);

		if (dist <= bestdist) {
			bestdist = dist;
			bestidx = ent->number;
			bestskin = ent->skinnum;
			*pitem = item;
		}
	}

	if (!bestidx)
	for (i=1; i<cl_baselines_count ; i++)
	{
		baseline = &cl_baselines[i];
		item = model2item[baseline->modelindex];
		if (!item)
			continue;
		if ( ! (item->itemflag & flags) )
			continue;

		VectorCopy(baseline->origin, v);
		VectorSubtract (v, org, v);
		VectorAdd (v, item->offset, v);
		dist = VectorLength (v);
//		Com_Printf ("%s %f\n", item->modelname, dist);

		if (dist <= bestdist) {
			bestdist = dist;
			bestidx = i;
			bestskin = baseline->skinnum;
			*pitem = item;
		}
	}

	if (bestidx && (*pitem)->itemflag == it_armor)
		if (bestskin >= 0 && bestskin <= 3)
			*pitem += bestskin + 1;

	return bestidx;
}

static int CountTeammates (void)
{
	int	i, count;
	player_info_t	*player;
	char	*myteam;

	if (tp_forceTriggers.ival)
		return 1;

	if (!cl.teamplay)
		return 0;

	count = 0;
	myteam = cl.players[cl.playerview[SP].playernum].team;
	for (i=0, player=cl.players; i < cl.allocated_client_slots ; i++, player++) {
		if (player->name[0] && !player->spectator && (i != cl.playerview[SP].playernum)
									&& !strcmp(player->team, myteam))
			count++;
	}

	return count;
}

static qboolean CheckTrigger (void)
{
	int	i, count;
	player_info_t *player;
	char *myteam;
	playerview_t *pv = &cl.playerview[SP];

	if (pv->spectator)
		return false;

	if (tp_forceTriggers.ival)
		return true;

	if (!cl.teamplay)
		return false;

	count = 0;
	myteam = cl.players[pv->playernum].team;
	for (i = 0, player= cl.players; i < cl.allocated_client_slots; i++, player++) {
		if (player->name[0] && !player->spectator && i != pv->playernum && !strcmp(player->team, myteam))
			count++;
	}

	return count;
}

static void ExecTookTrigger_ (char *s, int flag, vec3_t org)
{
	int pkflags_dmm, tookflags_dmm;

	pkflags_dmm = pkflags;
	tookflags_dmm = tookflags;

	if (!cl.teamfortress && cl.deathmatch >= 1 && cl.deathmatch <= 4) {
		if (cl.deathmatch == 4) {
			pkflags_dmm &= ~(it_ammo|it_weapons);
			tookflags_dmm &= ~(it_ammo|it_weapons);
		}
	}
	if (!((pkflags_dmm|tookflags_dmm) & flag))
		return;

	vars.tooktime = realtime;
	strncpy (vars.tookname, s, sizeof(vars.tookname)-1);
	strncpy (vars.tookloc, TP_LocationName (org), sizeof(vars.tookloc)-1);

	if ((tookflags_dmm & flag) && CheckTrigger())
		TP_ExecTrigger ("f_took", false);
}

/*
void TP_GetSimpleItemTexture ()
{
}
*/

static void TP_ItemTaken (char *s, int flag, vec3_t org, int entnum, item_t *item, int seat)
{
	if (seat == 0)
		ExecTookTrigger_(s, flag, org);

/*	if (entnum < cl_baselines_count && cl_baselines[entnum].modelindex && item && item->respawntime && (cl.spectator || cls.demoplayback))
	{
		struct itemtimer_s *timer = Z_Malloc(sizeof(*timer));
		timer->next = cl.itemtimers;
		cl.itemtimers = timer;
		timer->origin[0] = cl_baselines[entnum].origin[0] + item->offset[0];
		timer->origin[1] = cl_baselines[entnum].origin[1] + item->offset[1];
		timer->origin[2] = cl_baselines[entnum].origin[2];
		timer->start = cl.time;
		timer->duration = item->respawntime;
		timer->end = cl.time + item->respawntime;
		timer->radius = item->radius;
		timer->entnum = entnum;
	}
*/
}

void TP_ParsePlayerInfo(player_state_t *oldstate, player_state_t *state, player_info_t *info)
{
	playerview_t *pv = &cl.playerview[SP];
//	if (TP_NeedRefreshSkins())
//	{
//		if ((state->effects & (EF_BLUE|EF_RED) ) != (oldstate->effects & (EF_BLUE|EF_RED)))
//			TP_RefreshSkin(info - cl.players);
//	}

	if (!pv->spectator && cl.teamplay && strcmp(info->team, TP_PlayerTeam()))
	{
		qboolean eyes;

		eyes = state->modelindex && cl.model_precache[state->modelindex] && !strcmp(cl.model_precache[state->modelindex]->name, "progs/eyes.mdl");

		if (state->effects & (EF_BLUE | EF_RED) || eyes)
		{
			vars.enemy_powerups = 0;
			vars.enemy_powerups_time = realtime;

			if (state->effects & EF_BLUE)
			vars.enemy_powerups |= TP_QUAD;
			if (state->effects & EF_RED)
				vars.enemy_powerups |= TP_PENT;
			if (eyes)
				vars.enemy_powerups |= TP_RING;
		}
	}
	if (!pv->spectator && !cl.teamfortress && info - cl.players == pv->playernum)
	{
		if ((state->effects & (QWEF_FLAG1|QWEF_FLAG2)) && !(oldstate->effects & (QWEF_FLAG1|QWEF_FLAG2)))
		{
			ExecTookTrigger_ (tp_name_flag.string, it_flag, cl.inframes[cl.validsequence & UPDATE_MASK].playerstate[pv->playernum].origin);
		}
		else if (!(state->effects & (QWEF_FLAG1|QWEF_FLAG2)) && (oldstate->effects & (QWEF_FLAG1|QWEF_FLAG2)))
		{
			vars.lastdrop_time = realtime;
			strcpy (vars.lastdroploc, Macro_Location());
		}
	}
}

void TP_CheckPickupSound (char *s, vec3_t org, int seat)
{
	int entnum;
	item_t	*item;
	playerview_t *pv = &cl.playerview[seat];
	//if we're spectating, we don't want to do any actual triggers, so pretend it was someone else.
	if (pv->spectator)
		seat = -1;
	if (!s)
		return;

	//FIXME: on items/itembk2.wav kill relevant item timer.

	if (!strcmp(s, "items/damage.wav"))
	{
		entnum = FindNearestItem (org, it_quad, &item);
		TP_ItemTaken (tp_name_quad.string, it_quad, org, entnum, item, seat);
	}
	else if (!strcmp(s, "items/protect.wav"))
	{
		entnum = FindNearestItem (org, it_pent, &item);
		TP_ItemTaken (tp_name_pent.string, it_pent, org, entnum, item, seat);
	}
	else if (!strcmp(s, "items/inv1.wav"))
	{
		entnum = FindNearestItem (org, it_ring, &item);
		TP_ItemTaken (tp_name_ring.string, it_ring, org, entnum, item, seat);
	}
	else if (!strcmp(s, "items/suit.wav"))
	{
		entnum = FindNearestItem (org, it_suit, &item);
		TP_ItemTaken (tp_name_suit.string, it_suit, org, entnum, item, seat);
	}
	else if (!strcmp(s, "items/health1.wav") ||
			 !strcmp(s, "items/r_item1.wav"))
	{
		entnum = FindNearestItem (org, it_health, &item);
		TP_ItemTaken (tp_name_health.string, it_health, org, entnum, item, seat);
	}
	else if (!strcmp(s, "items/r_item2.wav"))
	{
		entnum = FindNearestItem (org, it_mh, &item);
		TP_ItemTaken (tp_name_mh.string, it_mh, org, entnum, item, seat);
	}
	else
		goto more;
	return;

more:
	if (!cl.validsequence || !cl.oldvalidsequence)
		return;

	// weapons
	if (!strcmp(s, "weapons/pkup.wav"))
	{
		entnum = FindNearestItem (org, it_weapons, &item);
		if (item)
			TP_ItemTaken (item->cvar->string, item->itemflag, org, entnum, item, seat);
		else if (seat >= 0)
		{
			// we don't know what entity caused the sound, try to guess...
			if (vars.stat_framecounts[STAT_ITEMS] == cls.framecount)
			{
				if (vars.items & ~vars.olditems & IT_LIGHTNING)
					TP_ItemTaken (tp_name_lg.string, it_lg, pv->simorg, entnum, item, seat);
				else if (vars.items & ~vars.olditems & IT_ROCKET_LAUNCHER)
					TP_ItemTaken (tp_name_rl.string, it_rl, pv->simorg, entnum, item, seat);
				else if (vars.items & ~vars.olditems & IT_GRENADE_LAUNCHER)
					TP_ItemTaken (tp_name_gl.string, it_gl, pv->simorg, entnum, item, seat);
				else if (vars.items & ~vars.olditems & IT_SUPER_NAILGUN)
					TP_ItemTaken (tp_name_sng.string, it_sng, pv->simorg, entnum, item, seat);
				else if (vars.items & ~vars.olditems & IT_NAILGUN)
					TP_ItemTaken (tp_name_ng.string, it_ng, pv->simorg, entnum, item, seat);
				else if (vars.items & ~vars.olditems & IT_SUPER_SHOTGUN)
					TP_ItemTaken (tp_name_ssg.string, it_ssg, pv->simorg, entnum, item, seat);
			}
		}
		return;
	}

	// armor
	if (!strcmp(s, "items/armor1.wav"))
	{
		item_t	*item;
		qbool armor_updated;

		armor_updated = (vars.stat_framecounts[STAT_ARMOR] == cls.framecount);
		entnum = FindNearestItem (org, it_armor, &item);
		if (item)
			TP_ItemTaken (item->cvar->string, item->itemflag, org, entnum, item, seat);
		else if (seat >= 0)
		{
			if (armor_updated && pv->stats[STAT_ARMOR] == 100)
				TP_ItemTaken (tp_name_ga.string, it_ga, org, entnum, NULL, seat);
			else if (armor_updated && pv->stats[STAT_ARMOR] == 150)
				TP_ItemTaken (tp_name_ya.string, it_ya, org, entnum, NULL, seat);
			else if (armor_updated && pv->stats[STAT_ARMOR] == 200)
				TP_ItemTaken (tp_name_ra.string, it_ra, org, entnum, NULL, seat);
		}
		return;
	}

	// backpack or ammo
	if (!strcmp (s, "weapons/lock4.wav"))
	{
		item_t	*item;
		entnum = FindNearestItem (org, it_ammo|it_pack|it_runes, &item);
		if (!item)
			return;
		TP_ItemTaken (item->cvar->string, item->itemflag, org, entnum, item, seat);
	}
}

qboolean R_CullSphere (vec3_t org, float radius);
static qboolean TP_IsItemVisible(item_vis_t *visitem)	//BE sure that pmove.skipent is set correctly first
{
	vec3_t end, v;
	trace_t trace;

	if (visitem->dist <= visitem->radius)
		return true;

	if (R_CullSphere(visitem->entorg, visitem->radius))
		return false;

	VectorNegate (visitem->dir, v);
	VectorNormalize (v);
	VectorMA (visitem->entorg, visitem->radius, v, end);
	trace = PM_TraceLine (visitem->vieworg, end);
	if (trace.fraction == 1)
		return true;

	VectorMA (visitem->entorg, visitem->radius, visitem->right, end);
	VectorSubtract (visitem->vieworg, end, v);
	VectorNormalize (v);
	VectorMA (end, visitem->radius, v, end);
	trace = PM_TraceLine (visitem->vieworg, end);
	if (trace.fraction == 1)
		return true;

	VectorMA(visitem->entorg, -visitem->radius, visitem->right, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA(end, visitem->radius, v, end);
	trace = PM_TraceLine(visitem->vieworg, end);
	if (trace.fraction == 1)
		return true;

	VectorMA(visitem->entorg, visitem->radius, visitem->up, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA (end, visitem->radius, v, end);
	trace = PM_TraceLine(visitem->vieworg, end);
	if (trace.fraction == 1)
		return true;

	// use half the radius, otherwise it's possible to see through floor in some places
	VectorMA(visitem->entorg, -visitem->radius / 2, visitem->up, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA(end, visitem->radius, v, end);
	trace = PM_TraceLine(visitem->vieworg, end);
	if (trace.fraction == 1)
		return true;

	return false;
}

//checks to see if a point at org the size of a player is visible or not
qboolean TP_IsPlayerVisible(vec3_t origin)
{
	item_vis_t visitem;

	VectorCopy(vpn, visitem.forward);
	VectorCopy(vright, visitem.right);
	VectorCopy(vup, visitem.up);
	VectorCopy(r_origin, visitem.vieworg);

	VectorCopy (origin, visitem.entorg);
	visitem.entorg[2] += 27;
	VectorSubtract (visitem.entorg, visitem.vieworg, visitem.dir);
	visitem.dist = DotProduct (visitem.dir, visitem.forward);
	visitem.radius = 25;

	return TP_IsItemVisible(&visitem);
}

static float TP_RankPoint(item_vis_t *visitem)
{
	vec3_t v2, v3;
	float miss;

	if (visitem->dist < 10)
		return -1;

	VectorScale (visitem->forward, visitem->dist, v2);
	VectorSubtract (v2, visitem->dir, v3);
	miss = VectorLength (v3);
	if (miss > 300)
		return -1;
	if (miss > visitem->dist * 1.7)
		return -1;		// over 60 degrees off

	return (visitem->dist < 3000.0 / 8.0) ? miss * (visitem->dist * 8.0 * 0.0002f + 0.3f) : miss;
}

static char *Utils_TF_ColorToTeam_Failsafe(int color)
{
	int i, j, teamcounts[8], numteamsseen = 0, best = -1;
	char *teams[MAX_CLIENTS];

	memset(teams, 0, sizeof(teams));
	memset(teamcounts, 0, sizeof(teamcounts));

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (!cl.players[i].name[0] || cl.players[i].spectator)
			continue;
		if (cl.players[i].rbottomcolor != color)
			continue;
		for (j = 0; j < numteamsseen; j++)
		{
			if (!strcmp(cl.players[i].team, teams[j]))
				break;
		}
		if (j == numteamsseen)
		{
			teams[numteamsseen] = cl.players[i].team;
			teamcounts[numteamsseen] = 1;
			numteamsseen++;
		}
		else
		{
			teamcounts[j]++;
		}
	}
	for (i = 0; i < numteamsseen; i++)
	{
		if (best == -1 || teamcounts[i] > teamcounts[best])
			best = i;
	}
	return (best == -1) ? "" : teams[best];
}

static char *Utils_TF_ColorToTeam(int color)
{
	char *s;

	switch (color)
	{
		case 13:
			if (*(s = InfoBuf_ValueForKey(&cl.serverinfo, "team1")) || *(s = InfoBuf_ValueForKey(&cl.serverinfo, "t1")))
				return s;
			break;
		case 4:
			if (*(s = InfoBuf_ValueForKey(&cl.serverinfo, "team2")) || *(s = InfoBuf_ValueForKey(&cl.serverinfo, "t2")))
				return s;
			break;
		case 12:
			if (*(s = InfoBuf_ValueForKey(&cl.serverinfo, "team3")) || *(s = InfoBuf_ValueForKey(&cl.serverinfo, "t3")))
				return s;
			break;
		case 11:
			if (*(s = InfoBuf_ValueForKey(&cl.serverinfo, "team4")) || *(s = InfoBuf_ValueForKey(&cl.serverinfo, "t4")))
				return s;
			break;
		default:
			return "";
	}
	return Utils_TF_ColorToTeam_Failsafe(color);
}

static void TP_FindPoint (void)
{
	packet_entities_t *pak;
	entity_state_t *ent;
	int	i, j, pointflags_dmm;
	float best = -1, rank;
	entity_state_t *bestent=NULL;
	vec3_t ang;
	item_t *item, *bestitem = NULL;
	player_state_t *state, *beststate = NULL;
	player_info_t *info, *bestinfo = NULL;
	item_vis_t visitem;
	extern cvar_t v_viewheight;
	int oldskip = pmove.skipent;
	playerview_t *pv = &cl.playerview[SP];

	if (vars.pointtime == realtime)
		return;

	if (!cl.validsequence)
		goto nothing;

	pmove.skipent = pv->viewentity;

	ang[0] = pv->viewangles[0]; ang[1] = pv->viewangles[1]; ang[2] = 0;
	AngleVectors (ang, visitem.forward, visitem.right, visitem.up);
	VectorCopy (pv->simorg, visitem.vieworg);
	visitem.vieworg[2] += 22 + (v_viewheight.value ? bound (-7, v_viewheight.value, 4) : 0);

	pointflags_dmm = pointflags;
	if (!cl.teamfortress && cl.deathmatch >= 1 && cl.deathmatch <= 4)
	{
		if (cl.deathmatch == 4)
			pointflags_dmm &= ~it_ammo;
		if (cl.deathmatch != 1)
			pointflags_dmm &= ~it_weapons;
	}

	pak = &cl.inframes[cl.validsequence & UPDATE_MASK].packet_entities;
	for (i = 0,ent = pak->entities; i < pak->num_entities; i++, ent++)
	{
		item = model2item[ent->modelindex];
		if (!item || !(item->itemflag & pointflags_dmm))
			continue;
		// special check for armors
		if (item->itemflag == (it_ra|it_ya|it_ga))
		{
			item += 1 + bound(0, ent->skinnum, 2);
			if (!(item->itemflag & pointflags_dmm))
				continue;
		}

		VectorAdd (ent->origin, item->offset, visitem.entorg);
		VectorSubtract (visitem.entorg, visitem.vieworg, visitem.dir);
		visitem.dist = DotProduct (visitem.dir, visitem.forward);
		visitem.radius = ent->effects & (EF_BLUE|EF_RED|EF_DIMLIGHT|EF_BRIGHTLIGHT) ? 200 : item->radius;

		if ((rank = TP_RankPoint(&visitem)) < 0)
			continue;

		// check if we can actually see the object
		if ((rank < best || best < 0) && TP_IsItemVisible(&visitem))
		{
			best = rank;
			bestent = ent;
			bestitem = item;
		}
	}

	state = cl.inframes[cl.parsecount & UPDATE_MASK].playerstate;
	info = cl.players;
	for (j = 0; j < cl.allocated_client_slots; j++, info++, state++)
	{
		if (state->messagenum != cl.parsecount || j == pv->playernum || info->spectator)
			continue;

		if (
			(state->modelindex == cl_playerindex && ISDEAD(state->frame)) ||
			state->modelindex == cl_h_playerindex
		)
			continue;

		VectorCopy (state->origin, visitem.entorg);
		visitem.entorg[2] += 30;
		VectorSubtract (visitem.entorg, visitem.vieworg, visitem.dir);
		visitem.dist = DotProduct (visitem.dir, visitem.forward);
		visitem.radius = (state->effects & (EF_BLUE|EF_RED|EF_DIMLIGHT|EF_BRIGHTLIGHT) ) ? 200 : 27;

		if ((rank = TP_RankPoint(&visitem)) < 0)
			continue;

		// check if we can actually see the object
		if ((rank < best || best < 0) && TP_IsItemVisible(&visitem))
		{
			qboolean teammate, eyes = false;

			eyes = state->modelindex && cl.model_precache[state->modelindex] && !strcmp(cl.model_precache[state->modelindex]->name, "progs/eyes.mdl");
			teammate = !!(cl.teamplay && !strcmp(info->team, TP_PlayerTeam()));

			if (eyes && !(pointflags_dmm & it_eyes))
				continue;
			else if (teammate && !(pointflags_dmm & it_teammate))
				continue;
			else if (!(pointflags_dmm & it_enemy))
				continue;

			best = rank;
			bestinfo = info;
			beststate = state;
		}
	}

	if (best >= 0 && bestinfo)
	{
		qboolean teammate, eyes;
		char *name, buf[256] = {0};

		eyes = beststate->modelindex && cl.model_precache[beststate->modelindex] && !strcmp(cl.model_precache[beststate->modelindex]->name, "progs/eyes.mdl");
		if (cl.teamfortress)
		{
			teammate = !strcmp(Utils_TF_ColorToTeam(bestinfo->rbottomcolor), TP_PlayerTeam());

			if (eyes)
				name = tp_name_eyes.string;		//duck on 2night2
			else if (pv->spectator)
				name = bestinfo->name;
			else if (teammate)
				name = tp_name_teammate.string[0] ? tp_name_teammate.string : "teammate";
			else
				name = tp_name_enemy.string;

			if (!eyes)
				name = va("%s%s%s", name, name[0] ? " " : "", Skin_To_TFSkin(InfoBuf_ValueForKey(&bestinfo->userinfo, "skin")));
		}
		else
		{
			teammate = !!(cl.teamplay && !strcmp(bestinfo->team, TP_PlayerTeam()));

			if (eyes)
				name = tp_name_eyes.string;
			else if (pv->spectator || (teammate && !tp_name_teammate.string[0]))
				name = bestinfo->name;
			else
				name = teammate ? tp_name_teammate.string : tp_name_enemy.string;
		}
		if (beststate->effects & EF_BLUE)
			Q_strncatz2(buf, tp_name_quaded.string);
		if (beststate->effects & EF_RED)
			Q_strncatz2(buf, va("%s%s", buf[0] ? " " : "", tp_name_pented.string));
		Q_strncatz2(buf, va("%s%s", buf[0] ? " " : "", name));
		Q_strncpyz (vars.pointname, buf, sizeof(vars.pointname));
		Q_strncpyz (vars.pointloc, TP_LocationName (beststate->origin), sizeof(vars.pointloc));

		vars.pointtype = (teammate && !eyes) ? POINT_TYPE_TEAMMATE : POINT_TYPE_ENEMY;
	}
	else if (best >= 0)
	{
		char *p;

		if (!bestitem->cvar)
		{
			p = tp_name_nothing.string;
		}
		else
		{
			p = bestitem->cvar->string;
		}

		vars.pointtype = (bestitem->itemflag & (it_powerups|it_flag)) ? POINT_TYPE_POWERUP : POINT_TYPE_ITEM;
		Q_strncpyz (vars.pointname, p, sizeof(vars.pointname));
		Q_strncpyz (vars.pointloc, TP_LocationName (bestent->origin), sizeof(vars.pointloc));
	}
	else
	{
nothing:
		Q_strncpyz (vars.pointname, tp_name_nothing.string, sizeof(vars.pointname));
		vars.pointloc[0] = 0;
		vars.pointtype = POINT_TYPE_ITEM;
	}
	vars.pointtime = realtime;
	pmove.skipent = oldskip;
}

void TP_UpdateAutoStatus(void)
{
	char newstatusbuf[sizeof(vars.autoteamstatus)];
	char *newstatus;
	int level;
	playerview_t *pv = &cl.playerview[SP];

	if (vars.autoteamstatus_time > realtime || !*tp_autostatus.string)
		return;
	vars.autoteamstatus_time = realtime + 3;

	level = tp_autostatus.restriction;
	newstatus = Cmd_ExpandString(tp_autostatus.string, newstatusbuf, sizeof(newstatusbuf), &level, false, true, true);
	newstatus = TP_ParseMacroString(newstatus);

	if (!strcmp(newstatus, vars.autoteamstatus))
		return;
	if (!*vars.autoteamstatus && !vars.health)
	{
		if (cls.state != ca_active)
			strcpy(vars.autoteamstatus, newstatus);
		return;	//don't start it with a death (stops spamming of locations when we originally connect, before spawning)
	}
	strcpy(vars.autoteamstatus, newstatus);

	if (strchr(tp_autostatus.string, ';'))
		return;	//don't take risks

	if (tp_autostatus.latched_string)
		return;

	if (pv->spectator)	//don't spam as spectators, that's just silly
		return;
	if (!cl.teamplay)	//don't spam in deathmatch, that's just pointless
		return;

	//the tp code will reexpand it as part of the say team
	Cbuf_AddText(va("say_team $\\%s\n", tp_autostatus.string), RESTRICT_LOCAL);
}

void TP_StatChanged (int stat, int value)
{
	playerview_t *pv = &cl.playerview[SP];
	int		i;
	if (stat == STAT_HEALTH)
	{
		if (value > 0)
		{
			if (vars.health <= 0)
			{
				// we just respawned
				vars.respawntrigger_time = realtime;

				if (!pv->spectator && CountTeammates())
					TP_ExecTrigger ("f_respawn", false);
			}
		}
		else if (vars.health > 0)
		{		// We have just died

			vars.droppedweapon = pv->stats[STAT_ACTIVEWEAPON];

			vars.deathtrigger_time = realtime;
			strcpy (vars.lastdeathloc, Macro_Location());

			CountNearbyPlayers(true);
			vars.last_numenemies = vars.numenemies;
			vars.last_numfriendlies = vars.numfriendlies;

			if (!pv->spectator && CountTeammates())
			{
				if (cl.teamfortress && (pv->stats[STAT_ITEMS] & (IT_KEY1|IT_KEY2))
					&& Cmd_AliasExist("f_flagdeath", RESTRICT_LOCAL))
					TP_ExecTrigger ("f_flagdeath", false);
				else
					TP_ExecTrigger ("f_death", false);
			}
		}
		vars.health = value;
	}
	else if (stat == STAT_ITEMS)
	{
		i = value &~ vars.items;

		if (i & (IT_KEY1|IT_KEY2)) {
			if (cl.teamfortress && !pv->spectator)
			{
				ExecTookTrigger_ (tp_name_flag.string, it_flag,
						cl.inframes[cl.validsequence&UPDATE_MASK].playerstate[pv->playernum].origin);
			}
		}

		if (!pv->spectator && cl.teamfortress && ~value & vars.items & (IT_KEY1|IT_KEY2))
		{
			vars.lastdrop_time = realtime;
			strcpy (vars.lastdroploc, Macro_Location());
		}

		vars.olditems = vars.items;
		vars.items = value;
	}
	else if (stat == STAT_ACTIVEWEAPON)
	{
		if (pv->stats[STAT_ACTIVEWEAPON] != vars.activeweapon)
			TP_ExecTrigger ("f_weaponchange", false);
		vars.activeweapon = pv->stats[STAT_ACTIVEWEAPON];
	}
	vars.stat_framecounts[stat] = cls.framecount;

	TP_UpdateAutoStatus();
}
#endif


/*
======================
TP_CheckSoundTrigger

Find and execute sound triggers.
A sound trigger must be terminated by either a CR or LF.
Returns true if a sound was found and played
======================
*/
qbool TP_CheckSoundTrigger (char *str)
{
	int		i, j;
	int		start, length;
	char	soundname[MAX_QPATH];

	if (!*str)
		return false;

	if (!tp_soundtrigger.string[0])
		return false;

	for (i=strlen(str)-1 ; i ; i--)
	{
		if (str[i] != 0x0A && str[i] != 0x0D)
			continue;

		for (j = i-1 ; j >= 0 ; j--)
		{
			// quick check for chars that cannot be used
			// as sound triggers but might be part of a file name
			if ( isalpha((int)(unsigned char)str[j]) ||
					 isdigit((int)(unsigned char)str[j]) )
				continue;	// file name or chat

			if (strchr(tp_soundtrigger.string, str[j]))
			{
				// this might be a sound trigger

				start = j + 1;
				length = i - start;

				if (!length)
					break;
				if (length >= MAX_QPATH)
					break;

				strlcpy (soundname, str + start, length + 1);
				if (strstr(soundname, ".."))
					break;	// no thank you

				// clean up the message
				strcpy (str + j, str + i);

				if (!S_HaveOutput())
					return false;

				COM_DefaultExtension (soundname, ".wav", sizeof(soundname));

				// make sure we have it on disk (FIXME)
				if (!FS_FLocateFile (va("sound/%s", soundname), FSLF_IFFOUND, NULL))
					return false;

				// now play the sound
				S_LocalSound (soundname);
				return true;
			}

			if (str[j] <= ' ' || strchr("\"&'*,:;<>?\\|\x7f", str[j]))
				break;	// we don't allow these in a file name
		}
	}

	return false;
}


#define MAX_FILTERS 8
#define MAX_FILTER_LENGTH 4
static char filter_strings[MAX_FILTERS][MAX_FILTER_LENGTH+1];
static int	num_filters = 0;

/*
======================
TP_FilterMessage

returns false if the message shouldn't be printed
matching filters are stripped from the message
======================
*/
qbool TP_FilterMessage (char *s)
{
	int i, j, len, maxlen;

	if (!num_filters)
		return true;

	len = strlen (s);
	if (len < 2 || s[len-1] != '\n' || s[len-2] == '#')
		return true;

	maxlen = MAX_FILTER_LENGTH + 1;
	for (i=len-2 ; i >= 0 && maxlen > 0 ; i--, maxlen--) {
		if (s[i] == ' ')
			return true;
		if (s[i] == '#')
			break;
	}
	if (i < 0 || !maxlen)
		return true;	// no filter at all

	s[len-1] = 0;	// so that strcmp works properly

	for (j=0 ; j<num_filters ; j++)
		if (!strcmp(s + i + 1, filter_strings[j]))
		{
			// strip the filter from message
			if (i && s[i-1] == ' ')
			{	// there's a space just before the filter, remove it
				// so that soundtriggers like ^blah #att work
				s[i-1] = '\n';
				s[i] = 0;
			} else {
				s[i] = '\n';
				s[i+1] = 0;
			}
			return true;
		}

	s[len-1] = '\n';
	return false;	// this message is not for us, don't print it
}

static void TP_MsgFilter_f (void)
{
	int c, i;
	char *s;

	c = Cmd_Argc ();
	if (c == 1) {
		if (!num_filters) {
			Com_Printf ("No filters defined\n");
			return;
		}
		for (i=0 ; i<num_filters ; i++)
			Com_Printf ("%s#%s", i ? " " : "", filter_strings[i]);
		Com_Printf ("\n");
		return;
	}

	if (c == 2 && (Cmd_Argv(1)[0] == 0 || !strcmp(Cmd_Argv(1), "clear"))) {
		num_filters = 0;
		return;
	}

	num_filters = 0;
	for (i=1 ; i < c ; i++) {
		s = Cmd_Argv(i);
		if (*s != '#') {
			Com_Printf ("A filter must start with \"#\"\n");
			return;
		}
		if (strchr(s+1, ' ')) {
			Com_Printf ("A filter may not contain spaces\n");
			return;
		}
		strlcpy (filter_strings[num_filters], s+1, sizeof(filter_strings[0]));
		num_filters++;
		if (num_filters >= countof(filter_strings))
			break;
	}
}

void TP_Init (void)
{
#define TEAMPLAYVARS	"Teamplay Variables"

	//register all the TeamPlay cvars.
#define TP_CVAR(name,def) Cvar_Register (&name,	TEAMPLAYVARS);
#define TP_CVARC(name,def,callback) Cvar_Register (&name, TEAMPLAYVARS);
#define TP_CVARAC(name,def,name2,callback) Cvar_Register (&name, TEAMPLAYVARS);
#define static
	TP_CVARS;
#undef static
#undef TP_CVAR
#undef TP_CVARC
#undef TP_CVARAC

	Cmd_AddCommand ("loadloc", TP_LoadLocFile_f);
	Cmd_AddCommand ("filter", TP_MsgFilter_f);
	Cmd_AddCommand ("msg_trigger", TP_MsgTrigger_f);
#ifdef QWSKINS
	Cmd_AddCommand ("colourise", TP_Colourise_f);	//uk
	Cmd_AddCommand ("colorize", TP_Colourise_f);	//us
	//Cmd_AddCommand ("colorise", TP_Colourise_f);	//piss off both.
#endif
#ifdef QUAKESTATS
	Cmd_AddCommand ("tp_took", TP_Took_f);
	Cmd_AddCommand ("tp_pickup", TP_Pickup_f);
	Cmd_AddCommand ("tp_point", TP_Point_f);
#endif

	TP_InitMacros();
}





qboolean TP_SuppressMessage(char *buf) {
	char *s;
	unsigned int seat;

	for (s = buf; *s && *s != 0x7f; s++)
		;

	if (*s == 0x7f && *(s + 1) == '!')	{
		*s++ = '\n';
		*s++ = 0;

		if (!cls.demoplayback)
			for (seat = 0; seat < cl.splitclients; seat++)
				if (!cl.playerview[seat].spectator && *s - 'A' == cl.playerview[seat].playernum)
					return true;
	}
	return false;
}

void CL_PrintChat(player_info_t *plr, char *msg, int plrflags);

void CL_Say (qboolean team, char *extra)
{
	extern cvar_t cl_fakename;
	char	text[2048], sendtext[2048], *s;
	playerview_t *pv = &cl.playerview[CL_TargettedSplit(false)];

	if (Cmd_Argc() < 2)
	{
		if (team)
			Con_Printf ("%s <text>: send a team message\n", Cmd_Argv(0));
		return;
	}

	if (cls.state == ca_disconnected)
	{
		Con_TPrintf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	suppress = false;

	s = TP_ParseMacroString (Cmd_Args());
	Q_strncpyz (text, TP_ParseFunChars (s), sizeof(text));

	sendtext[0] = 0;
	if (team && !pv->spectator && cl_fakename.string[0] &&
		!strchr(s, '\x0d') /* explicit $\ in message overrides cl_fakename */)
	{
		char buf[1024];
		int level = Cmd_ExecLevel;
		Cmd_ExpandString (cl_fakename.string, buf, sizeof(buf), &level, false, true, true);
		strcpy (buf, TP_ParseMacroString (buf));
		Q_snprintfz (sendtext, sizeof(sendtext), "\x0d%s: ", TP_ParseFunChars(buf));
	}

	strlcat (sendtext, text, sizeof(sendtext));
	if (suppress)
	{
		//print it locally:
		char *d;
		for (s = sendtext, d = text; *s; s++, d++)
		{
			if (*s == '\xff')	//text that is hidden to us
			{	//
				s++;
				*d++ = '^';
				*d++ = 's';
				*d++ = '^';
				*d++ = '&';
				*d++ = '4';
				*d++ = '0';
				if (*s == 'z')
					*d++ = 'x';
				else
					*d++ = (char)139;

				*d++ = '^';
				*d++ = 'r';
				d--;

				while(*s != '\xff')
				{
					if (!*s)
						break;
					s++;
				}
				if (!*s)
					break;
			}
			else
				*d = *s;
		}
		*d++ = '\n';
		*d = '\0';

		{
			int plrflags = 0;
			if (team)
				plrflags |= 2;

			CL_PrintChat(&cl.players[pv->playernum], text, plrflags);
		}

		//strip out the extra markup
		for (s = sendtext, d = sendtext; *s; s++, d++)
		{
			if (*s == '\xff')	//text that is hidden to us
			{	//
				s++;
				if (*s == 'z')
					s++;
				while(*s != '\xff')
				{
					if (!*s)
						break;
					*d++ = *s++;
				}
				if (!*s)
					break;
				d--;
			}

			else
				*d = *s;
		}
		*d = '\0';

		//mark the message so that we ignore it when we get the echo.
		strlcat (sendtext, va("\x7f!%c", 'A'+pv->playernum), sizeof(sendtext));
	}

#ifdef Q2CLIENT
	if (cls.netchan.remote_address.prot == NP_KEXLAN && NET_GetConnectionCertificate(cls.sockets, &cls.netchan.remote_address, QCERT_LOBBYSENDCHAT, sendtext, strlen(sendtext))>0)
		return;
#endif

#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
		q3->cl.SendClientCommand("%s %s%s", team ? "say_team" : "say", extra?extra:"", sendtext);
	else
#endif
	{
		int split = CL_TargettedSplit(true);
		if (split >= cl.splitclients)
			return;
		//messagemode always adds quotes. the console command never did.
		//the server is expected to use Cmd_Args and to strip first+last chars if the first is a quote. this is annoying and clumsy for mods to parse.
#ifdef HAVE_LEGACY
		if (!dpcompat_console.ival
#ifdef NQPROT
				&& !cls.qex
#endif
				)
			CL_SendSeatClientCommand(true, split, "%s \"%s%s\"", team ? "say_team" : "say", extra?extra:"", sendtext);
		else
#endif
			CL_SendSeatClientCommand(true, split, "%s %s%s", team ? "say_team" : "say", extra?extra:"", sendtext);
	}
}


void CL_Say_f (void)
{
#ifndef CLIENTONLY
	if (isDedicated)
		SV_ConSay_f();
	else
#endif
		CL_Say (false, NULL);
}

void CL_SayMe_f (void)
{
	CL_Say (false, "/me ");
}

void CL_SayTeam_f (void)
{
#ifdef QUAKESTATS
	vars.autoteamstatus_time = realtime + 3;
#endif
	CL_Say (true, NULL);
}

qboolean TP_SoundTrigger(char *message)	//if there is a trigger there, play it. Return true if we found one, stripping off the file (it's neater that way).
{
	char *strip;
	char *lineend = NULL;
	char soundname[128];
	int filter = 0;

	for (strip = message+strlen(message)-1; *strip && strip >= message; strip--)
	{
		if (*strip == '#')
			filter++;
		if (*strip == ':')
			break;	//if someone says just one word, we can take any tidles in their name to be a voice command
		if (*strip == '\n')
			lineend = strip;
		else if (*strip <= ' ')
		{
			if (filter == 0 || filter == 1)	//allow one space in front of a filter.
			{
				filter++;
				continue;
			}
			break;
		}
		else if (*strip == '~')
		{
			//looks like a trigger, whoopie!
			if (lineend-strip > sizeof(soundname)-1)
			{
				Con_Printf("Sound trigger's file-name was too long\n");
				return false;
			}
			Q_strncpyz(soundname, strip+1, lineend-strip);
			memmove(strip, lineend, strlen(lineend)+1);

			Cbuf_AddText(va("play %s\n", soundname), RESTRICT_LOCAL);
			return true;
		}
	}
	return false;
}
