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
/* ZOID
 *
 * Player camera tracking in Spectator mode
 *
 * This takes over player controls for spectator automatic camera.
 * Player moves as a spectator, but the camera tracks and enemy player
 */

#include "quakedef.h"
#include "winquake.h"

#define	PM_SPECTATORMAXSPEED	500
#define	PM_STOPSPEED	100
#define	PM_MAXSPEED			320
#define BUTTON_JUMP 2
#define BUTTON_ATTACK 1
#define MAX_ANGLE_TURN 10

char cl_spectatorgroup[] = "Spectator Tracking";

enum
{
	TM_USER,			//user hit jump and changed pov explicitly
	TM_HIGHTRACK,		//tracking the player with the highest frags
	TM_KILLER,			//switch to the previous person's killer.
	TM_MODHINTS,		//using the mod's //at hints
	TM_STATS			//parsing mvd stats and making our own choices
} autotrackmode;
char *autotrack_statsrule;
static void QDECL CL_AutoTrackChanged(cvar_t *v, char *oldval)
{
	Cam_AutoTrack_Update(v->string);
}
// track high fragger
cvar_t cl_autotrack_team	= CVARD("cl_autotrack_team", "", "Specifies a team name that should be auto-tracked (players on other teams will not be candidates for autotracking). Accepts * and ? wildcards for awkward chars.");
cvar_t cl_autotrack			= CVARCD("cl_autotrack", "auto", CL_AutoTrackChanged, "Specifies the default tracking mode at the start of the map. Use the 'autotrack' command to reset/apply an auto-tracking mode without changing the default.\nValid values are: high, ^hkiller^h, mod, user. Other values are treated as weighting scripts for mvd playback, where available.");
cvar_t cl_hightrack			= CVARD("cl_hightrack", "0", "Obsolete. If you want hightrack, use '[cl_]autotrack high' instead.");

//cvar_t cl_camera_maxpitch = {"cl_camera_maxpitch", "10" };
//cvar_t cl_camera_maxyaw = {"cl_camera_maxyaw", "30" };
cvar_t cl_chasecam			= CVAR("cl_chasecam", "1");
cvar_t cl_selfcam			= CVAR("cl_selfcam", "1");

void Cam_AutoTrack_Update(const char *mode)
{
	if (!mode)
		mode = cl_autotrack.string;
	Z_Free(autotrack_statsrule);
	autotrack_statsrule = NULL;
	if (!*mode || !Q_strcasecmp(mode, "auto"))
	{
		if (cls.demoplayback == DPB_MVD)
		{
			autotrackmode = TM_STATS;
			autotrack_statsrule = Z_StrDup("");	//default
		}
		else if (cl_hightrack.ival)
			autotrackmode = TM_HIGHTRACK;
		else
			autotrackmode = TM_MODHINTS;
	}
	else if (!Q_strcasecmp(mode, "high"))
		autotrackmode = TM_HIGHTRACK;
	else if (!Q_strcasecmp(mode, "killer"))
		autotrackmode = TM_KILLER;
	else if (!Q_strcasecmp(mode, "mod"))
		autotrackmode = TM_MODHINTS;
	else if (!Q_strcasecmp(mode, "user") || !Q_strcasecmp(mode, "off"))
		autotrackmode = TM_USER;
	else if (!Q_strcasecmp(mode, "stats"))
	{
		autotrackmode = TM_STATS;
		autotrack_statsrule = NULL;
	}
	else
	{
		autotrackmode = TM_STATS;
		autotrack_statsrule = Z_StrDup(mode);
	}
}
static void Cam_AutoTrack_f(void)
{
	if (*Cmd_Argv(1))
		Cam_AutoTrack_Update(Cmd_Argv(1));
	else
		Cam_AutoTrack_Update(NULL);
}

static float CL_TrackScoreProp(player_info_t *pl, char rule, float *weights)
{
#ifdef QUAKESTATS
	float r;
#endif
	switch(rule)
	{
	case '.':	//currently being tracked. combine with currently alive or something
		{
			int i;
			for (i = 0; i < cl.splitclients; i++)
			{
				//not valid if an earlier view is tracking it.
				if (Cam_TrackNum(&cl.playerview[i]) == pl-cl.players)
					return 1;
			}
			return 0;
		}
#ifdef QUAKESTATS
	case 'a':	//armour value
		return pl->statsf[STAT_ARMOR];
	case 'h':
		return pl->statsf[STAT_HEALTH];
	case 'A':	//armour type
		if		(pl->stats[STAT_ITEMS] & IT_ARMOR3)	r = weights[10];
		else if (pl->stats[STAT_ITEMS] & IT_ARMOR2)	r = weights[9];
		else if (pl->stats[STAT_ITEMS] & IT_ARMOR1)	r = weights[8];
		else										r = 0;
		return r;
	case 'W':	//best weapon
		r = 0;
		if (r < weights[0] && (pl->stats[STAT_ITEMS] & IT_AXE)) r = weights[0];
		if (r < weights[1] && (pl->stats[STAT_ITEMS] & IT_SHOTGUN)) r = weights[1];
		if (r < weights[2] && (pl->stats[STAT_ITEMS] & IT_SUPER_SHOTGUN)) r = weights[2];
		if (r < weights[3] && (pl->stats[STAT_ITEMS] & IT_NAILGUN)) r = weights[3];
		if (r < weights[4] && (pl->stats[STAT_ITEMS] & IT_SUPER_NAILGUN)) r = weights[4];
		if (r < weights[5] && (pl->stats[STAT_ITEMS] & IT_GRENADE_LAUNCHER)) r = weights[5];
		if (r < weights[6] && (pl->stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)) r = weights[6];
		if (r < weights[7] && (pl->stats[STAT_ITEMS] & IT_LIGHTNING)) r = weights[7];
		return r;
	case 'p':	//powerups held
		r = 0;
		r += (pl->stats[STAT_ITEMS] & IT_INVISIBILITY)?weights[11]:0;
		r += (pl->stats[STAT_ITEMS] & IT_QUAD)?weights[12]:0;
		r += (pl->stats[STAT_ITEMS] & IT_INVULNERABILITY)?weights[13]:0;
		return r;
#endif
	case 'f':	//frags
		return pl->frags;
//	case 'F':	//team frags
//		return 0;
#ifdef QUAKEHUD
	case 'g':	//deaths
		return Stats_GetDeaths(pl - cl.players);
#endif
	case 'u':	//userid
		return pl - cl.players;
	case 'c':	//'current run time'
	case 'C':	//'current run frags'
	case 'd':	//'current run teamfrags'
	case 'I':	//ssg grabs
	case 'j':	//ng grabs
	case 'J':	//sng grabs
	case 'k':	//gl grabs
	case 'K':	//gl lost
	case 'l':	//rl grabs
	case 'L':	//rl lost
	case 'm':	//lg grabs
	case 'M':	//lg lost
	case 'n':	//mh grabs
	case 'N':	//ga grabs
	case 'o':	//ya grabs
	case 'O':	//ra grabs
	case 'v':	//average run time
	case 'V':	//average run frags
	case 'w':	//average run teamfrags
	case 'x':	//ring grabs
	case 'X':	//ring losts
	case 'y':	//quad grabs
	case 'Y':	//quad losts
	case 'z':	//pent grabs
	default:
		return 0;
	}
}
#define PRI_TOP 3
#define PRI_LOGIC 3
#define PRI_ADD 2
#define PRI_MUL 1
#define PRI_VAL 0
static float CL_TrackScore(player_info_t *pl, char **rule, float *weights, int pri)
{
	float l, r;
	char *s = *rule;
	while (*s == ' ' || *s == '\t')
		s++;
	if (!pri)
	{
		if (*s == '!')
		{
			s++;
			l = !CL_TrackScore(pl, &s, weights, pri);
		}
		if (*s == '(')
		{
			l = CL_TrackScore(pl, &s, weights, PRI_TOP);
			while (*s == ' ' || *s == '\t')
				s++;
			if (*s == ')')
				s++;
		}
		else if (*s == '%')
		{
			l = CL_TrackScoreProp(pl, *++s, weights);
			s++;
		}
		else if (*s == '#')
		{
			int i = strtoul(s+1, &s, 0);
			if (i >= 0 && i < MAX_CL_STATS)
				l = pl->statsf[i];
			else
				l = 0;
		}
		else
			l = strtod(s, &s);
		while (*s == ' ' || *s == '\t')
			s++;
	}
	else
		l = CL_TrackScore(pl, &s, weights, pri-1);

	for (;;)
	{
		if (pri == PRI_MUL)
		{
			if (*s == '*')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l *= r;
				continue;
			}
			else if (*s == '/')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l /= r;
				continue;
			}
		}
		else if (pri == PRI_ADD)
		{
			if (*s == '+')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l += r;
				continue;
			}
			else if (*s == '-')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l -= r;
				continue;
			}
		}
		else if (pri == PRI_LOGIC)
		{
			if (*s == '|' && s[1] == '|')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l||r;
				continue;
			}
			else if (*s == '&' && s[1] == '&')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l&&r;
				continue;
			}
			else if (*s == '&')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = (int)l&(int)r;
				continue;
			}
			else if (*s == '|')
			{
				s++;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = (int)l|(int)r;
				continue;
			}
			else if (*s == '>' && s[1] == '=')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l>=r;
				continue;
			}
			else if (*s == '<' && s[1] == '=')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l<=r;
				continue;
			}
			else if (*s == '>')
			{
				s+=1;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l>r;
				continue;
			}
			else if (*s == '<')
			{
				s+=1;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l>r;
				continue;
			}
			else if (*s == '!' && s[1] == '=')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l!=r;
				continue;
			}
			else if (*s == '=' && s[1] == '=')
			{
				s+=2;
				r = CL_TrackScore(pl, &s, weights, pri-1);
				l = l!=r;
				continue;
			}
		}
		break;
	}
	*rule = s;
	return l;
}
static qboolean CL_MayAutoTrack(int seat, int player)
{
	if (player < 0)
		return false;
	if (!cl.players[player].userid || !cl.players[player].name[0] || cl.players[player].spectator)
		return false;
	if (*cl_autotrack_team.string && !wildcmp(cl_autotrack_team.string, cl.players[player].team))
		return false;
	for (seat--; seat >= 0; seat--)
	{
		//not valid if an earlier view is tracking it.
		if (Cam_TrackNum(&cl.playerview[seat]) == player)
			return false;
	}
	return true;
}
//returns the player with the highest frags
static int CL_FindHighTrack(int seat, char *rule)
{
	int j = -1;
	int i;
	float max, score;
	player_info_t *s;
	float weights[14];
	char *p;
	qboolean instant;

	instant = (rule && *rule == '!');
	if (instant)
		rule++;

	if (rule && *rule == '[')
	{
		rule++;
		weights[0] = strtod(rule, &rule);	//axe
		weights[1] = strtod(rule, &rule);	//shot
		weights[2] = strtod(rule, &rule);	//sshot
		weights[3] = strtod(rule, &rule);	//nail
		weights[4] = strtod(rule, &rule);	//snail
		weights[5] = strtod(rule, &rule);	//gren
		weights[6] = strtod(rule, &rule);	//rock
		weights[7] = strtod(rule, &rule);	//lg
		weights[8] = strtod(rule, &rule);	//ga
		weights[9] = strtod(rule, &rule);	//ya
		weights[10] = strtod(rule, &rule);	//ra
		weights[11] = strtod(rule, &rule);	//ring
		weights[12] = strtod(rule, &rule);	//quad
		weights[13] = strtod(rule, &rule);	//pent
		if (*rule == ']')
			rule++;
	}
	else
	{
		weights[0] = 1;	//axe
		weights[1] = 2;	//shot
		weights[2] = 3;	//sshot
		weights[3] = 2;	//nail
		weights[4] = 3;	//snail
		weights[5] = 3;	//gren
		weights[6] = 8;	//rock
		weights[7] = 8;	//lg
		weights[8] = 1;	//ga
		weights[9] = 2;	//ya
		weights[10] = 3;	//ra
		weights[11] = 500;	//ring
		weights[12] = 900;	//quad
		weights[13] = 1000;	//pent
	}

	if (!rule || !*rule)
		rule = "%a * %A + 50 * %W + %p + %f";

	//set a default to the currently tracked player, to reuse the current player we're tracking if someone lower equalises.
	j = cl.playerview[seat].cam_spec_track;
	if (CL_MayAutoTrack(seat, j))
	{
		p = rule;
		max = CL_TrackScore(&cl.players[j], &p, weights, PRI_TOP);
	}
	else
	{
		max = -9999;
		j = -1;
	}

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		s = &cl.players[i];
		if (j == i)	//this was our default.
			continue;
		if (!CL_MayAutoTrack(seat, i))
			continue;
		if (cl.teamplay && seat && cl.playerview[0].cam_spec_track >= 0 && strcmp(cl.players[cl.playerview[0].cam_spec_track].team, s->team))
			continue;	//when using multiview, keep tracking the team
		p = rule;
		score = CL_TrackScore(s, &p, weights, PRI_TOP);
		if (score > max)
		{
			max = score;
			j = i;
		}
	}
	if (j == -1 && cl.teamplay && seat)
	{
		//do it again, but with the teamplay check inverted
		//fixme: should probably reduce seat count instead, at least in demos.
		for (i = 0; i < cl.allocated_client_slots; i++)
		{
			s = &cl.players[i];
			if (j == i)	//this was our default.
				continue;
			if (!CL_MayAutoTrack(seat, i))
				continue;
			if (!(cl.teamplay && seat && cl.playerview[0].cam_spec_track >= 0 && strcmp(cl.players[cl.playerview[0].cam_spec_track].team, s->team)))
				continue;	//when using multiview, keep tracking the team
			p = rule;
			score = CL_TrackScore(s, &p, weights, PRI_TOP);
			if (score > max)
			{
				max = score;
				j = i;
			}
		}
	}

	if (j != -1 && CL_MayAutoTrack(seat, cl.playerview[seat].cam_spec_track) && !instant)
	{
		i = cl.playerview[seat].cam_spec_track;
		//extra hacks to prevent 'random' switching mid-game
#ifdef QUAKESTATS
		if ((cl.players[j].stats[STAT_ITEMS] ^ cl.players[i].stats[STAT_ITEMS]) & (IT_INVULNERABILITY|IT_QUAD))
			;	//don't block if the players have different powerups
		else if ((cl.players[j].stats[STAT_ITEMS] & (IT_ROCKET_LAUNCHER|IT_LIGHTNING)) && !(cl.players[i].stats[STAT_ITEMS] & (IT_ROCKET_LAUNCHER|IT_LIGHTNING)))
			;	//don't block the switch if the new player has a decent weapon, and the guy we're tracking does not.
		else if ((cl.players[j].stats[STAT_ITEMS] & (IT_ROCKET_LAUNCHER|IT_INVULNERABILITY))==(IT_ROCKET_LAUNCHER|IT_INVULNERABILITY) && (cl.players[i].stats[STAT_ITEMS] & (IT_ROCKET_LAUNCHER|IT_INVULNERABILITY)) != (IT_ROCKET_LAUNCHER|IT_INVULNERABILITY))
			;	//don't block if we're switching to someone with pent+rl from someone that does not.
		else
#endif
			return cl.playerview[seat].cam_spec_track;
	}
	return j;
}

static int CL_AutoTrack_Choose(int seat)
{
	int best = -1;
	if (autotrackmode == TM_KILLER)
		best = cl.autotrack_killer;
	if (autotrackmode == TM_MODHINTS && seat == 0 && cl.autotrack_hint >= 0)
		best = cl.autotrack_hint;
	if (autotrackmode == TM_STATS && cls.demoplayback == DPB_MVD)
		best = CL_FindHighTrack(seat, autotrack_statsrule);
	if (autotrackmode == TM_HIGHTRACK || best == -1)
		best = CL_FindHighTrack(seat, "%f");
	//TM_USER should generally avoid autotracking
	cl.autotrack_killer = best;	//killer should continue to track whatever is currently tracked until its changed by frag message parsing
	return best;
}

static int Cam_FindSortedPlayer(int number);

int selfcam=1;

static float vlen(vec3_t v)
{
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// returns true if weapon model should be drawn in camera mode
qboolean Cam_DrawViewModel(playerview_t *pv)
{
	if (pv->spectator)
	{
		if (pv->cam_state == CAM_EYECAM && cl_chasecam.ival)
			return true;
		return false;
	}
	else
	{
		if (selfcam == 1 && !r_refdef.externalview)
			return true;
		return false;
	}
}

int Cam_TrackNum(playerview_t *pv)
{
	if (pv->spectator && CAM_ISLOCKED(pv))
		return pv->cam_spec_track;
	return -1;
}

void Cam_Unlock(playerview_t *pv)
{
	if (pv->cam_state)
	{
		CL_SendSeatClientCommand(true, pv-cl.playerview, "ptrack");
		pv->cam_state = CAM_FREECAM;
		pv->viewentity = (cls.demoplayback)?0:(pv->playernum+1);	//free floating
		SCR_CenterPrint(pv-cl.playerview, NULL, true);
		Sbar_Changed();

		//flashgrens suck
		pv->cshifts[CSHIFT_SERVER].percent = 0;

		Skin_FlushPlayers();
	}
}

void Cam_Lock(playerview_t *pv, int playernum)
{
	pv->cam_lastviewtime = -1000;	//allow the wallcam to re-snap as soon as it can

	CL_SendSeatClientCommand(true, pv-cl.playerview, "ptrack %i", playernum);

	if (pv->cam_spec_track != playernum)
	{	//flashgrens suck
		pv->cshifts[CSHIFT_SERVER].percent = 0;
	}
	pv->cam_spec_track = playernum;
	pv->cam_state = CAM_PENDING;
	pv->viewentity = (cls.demoplayback)?0:(pv->playernum+1);	//free floating until actually locked
	SCR_CenterPrint(pv-cl.playerview, NULL, true);

	
	Skin_FlushPlayers();

	if (cls.demoplayback == DPB_MVD)
	{
		memcpy(&pv->stats, cl.players[playernum].stats, sizeof(pv->stats));
//		pv->cam_state = CAM_;
	
//		pv->viewentity = playernum+1;
		/*
		pv->cam_state = cl_chasecam.ival?CAM_EYECAM:CAM_PENDING;	//instantly lock if the player is valid.
		pv->viewentity = playernum+1;
		*/

#ifdef QUAKESTATS
		if (cls.z_ext & Z_EXT_VIEWHEIGHT)
			pv->viewheight = cl.players[playernum].statsf[STAT_VIEWHEIGHT];
#endif
	}

	Sbar_Changed();

#ifdef QWSKINS
	{
		int i;
		for (i = 0; i < cl.allocated_client_slots; i++)
			CL_NewTranslation(i);
	}
#endif
}

trace_t Cam_DoTrace(vec3_t vec1, vec3_t vec2)
{
#if 0
	memset(&pmove, 0, sizeof(pmove));

	pmove.numphysent = 1;
	memset(&pmove.physents[0], 0, sizeof(physent_t));
	VectorClear (pmove.physents[0].origin);
	pmove.physents[0].model = cl.worldmodel;
#endif

	VectorCopy (vec1, pmove.origin);
	return PM_PlayerTrace(pmove.origin, vec2, MASK_PLAYERSOLID);
}
	
// Returns distance or 9999 if invalid for some reason
static float Cam_TryFlyby(vec3_t selforigin, vec3_t playerorigin, vec3_t vec, qboolean checkvis)
{
	vec3_t v;
	trace_t trace;
	float len;

	pmove.player_mins[0] = pmove.player_mins[1] = -16;
	pmove.player_mins[2] = -24;
	pmove.player_maxs[0] = pmove.player_maxs[1] = 16;
	pmove.player_maxs[2] = 32;

	VectorAngles(vec, NULL, v, true);
	VectorCopy (v, pmove.angles);
	VectorNormalize(vec);
	VectorMA(playerorigin, 800, vec, v);
	// v is endpos
	// fake a player move
	trace = Cam_DoTrace(playerorigin, v);
	if (/*trace.inopen ||*/ trace.inwater)
		return 9999;
	VectorCopy(trace.endpos, vec);
	VectorSubtract(trace.endpos, playerorigin, v);
	len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len < 32 || len > 800)
		return 9999;
	if (checkvis)
	{
		VectorSubtract(trace.endpos, selforigin, v);
		len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

		trace = Cam_DoTrace(selforigin, vec);
		if (trace.fraction != 1 || trace.inwater)
			return 9999;
	}
	return len;
}

// Is player visible?
static qboolean Cam_IsVisible(vec3_t playerorigin, vec3_t vec)
{
	trace_t trace;
	vec3_t v;
	float d;

	VectorClear(pmove.player_mins);
	VectorClear(pmove.player_maxs);

	trace = Cam_DoTrace(playerorigin, vec);
	if (trace.fraction != 1 || /*trace.inopen ||*/ trace.inwater)
		return false;
	// check distance, don't let the player get too far away or too close
	VectorSubtract(playerorigin, vec, v);
	d = vlen(v);
	if (d < 16)
		return false;
	return true;
}

static qboolean InitFlyby(playerview_t *pv, vec3_t selforigin, vec3_t playerorigin, vec3_t playerviewangles, int checkvis) 
{
	vec3_t dirs[] = {
		{1,1,1},
		{1,-1,1},
		{1,1,0},
		{1,-1,1},
		{1,0,1},
		{1,0,-1},
		{-1,1,1},
		{-1,-1,1},
		{-1,0,0},
		{1,0,0},
		{0,0,-1},
		{0,0,1}
		};
	int dir;
	float f, max;
	vec3_t vec, vec2;
	vec3_t forward, right, up;

	VectorCopy(playerviewangles, vec);
	vec[0] = 0;
	AngleVectors (vec, forward, right, up);
//	for (i = 0; i < 3; i++)
//		forward[i] *= 3;

	max = 1000;
	for (dir = 0; dir < sizeof(dirs)/sizeof(dirs[0]); dir++)
	{
		VectorScale(forward, dirs[dir][0], vec2);
		VectorMA(vec2, dirs[dir][1], right, vec2);
		VectorMA(vec2, dirs[dir][2], up, vec2);
		if ((f = Cam_TryFlyby(selforigin, playerorigin, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
	}
	// ack, can't find him
	if (max >= 1000)
	{
//		Cam_Unlock();
		return false;
	}

	pv->cam_state = CAM_WALLCAM;
	pv->viewentity = pv->playernum+1;//pv->cam_spec_track+1;
	VectorCopy(vec, pv->cam_desired_position); 
	return true;
}

static void Cam_CheckHighTarget(playerview_t *pv)
{
	int j;
	playerview_t *spv;

	j = CL_AutoTrack_Choose(pv - cl.playerview);
	if (j >= 0)
	{
		if (pv->cam_spec_track != j || pv->cam_state == CAM_FREECAM)
		{
#ifdef QUAKEHUD
			if (cl.teamplay)
				Stats_Message("Now tracking:\n%s\n%s", cl.players[j].name, cl.players[j].team);
			else
				Stats_Message("Now tracking:\n%s", cl.players[j].name);
#endif
			Cam_Lock(pv, j);
			//un-lock any higher seats watching our new target. this keeps things ordered.
			for (spv = pv+1; spv >= cl.playerview && spv < &cl.playerview[cl.splitclients]; spv++)
			{
				if (Cam_TrackNum(spv) == j)
					spv->cam_state = CAM_FREECAM;
			}
		}
	}
	else
		Cam_Unlock(pv);
}

void Cam_SelfTrack(playerview_t *pv)
{
	vec3_t vec;
	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		return;

	if (selfcam == 1)
	{	//view-from-eyes
	}
	else
	{
		if (selfcam == 2)
		{	//fixme:
			vec3_t forward, right, up;
			trace_t tr;
			AngleVectors(r_refdef.viewangles, forward, right, up);
			VectorMA(pv->simorg, -128, forward, pv->cam_desired_position);
			tr = Cam_DoTrace(pv->simorg, pv->cam_desired_position);
			VectorCopy(tr.endpos, pv->cam_desired_position);
		}
		else
		{	//view from a random wall
			if (pv->cam_state != CAM_WALLCAM || !Cam_IsVisible(pv->simorg, pv->cam_desired_position))
			{
				if (pv->cam_state != CAM_WALLCAM || realtime - pv->cam_lastviewtime > 0.1)
				{
					if (!InitFlyby(pv, pv->cam_desired_position, pv->simorg, pv->simangles, true))
						InitFlyby(pv, pv->cam_desired_position, pv->simorg, pv->simangles, false);
					pv->cam_lastviewtime = realtime;
				}
			}
			else
			{
				pv->cam_lastviewtime = realtime;
			}

			//tracking failed.
			if (pv->cam_state != CAM_WALLCAM)
				return;
		}


		// move there locally immediately
		VectorCopy(pv->cam_desired_position, r_refdef.vieworg);

		VectorSubtract(pv->simorg, pv->cam_desired_position, vec);
		VectorAngles(vec, NULL, r_refdef.viewangles, false);
	}
}

//player entity became visible, lock on to them (now that we know where they are etc)
void Cam_NowLocked(playerview_t *pv)
{
	pv->cam_lastviewtime = realtime;
	if (!cl_chasecam.ival)
	{
		if (pv->cam_state != CAM_WALLCAM || !Cam_IsVisible(pv->simorg, pv->cam_desired_position))
		{
			if (pv->cam_state != CAM_WALLCAM || realtime - pv->cam_lastviewtime > 0.1)
			{
				if (!InitFlyby(pv, pv->cam_desired_position, pv->simorg, pv->simangles, true))
					InitFlyby(pv, pv->cam_desired_position, pv->simorg, pv->simangles, false);
			}
		}
	}
}

// ZOID
//
// Take over the user controls and track a player.
// We find a nice position to watch the player and move there
void Cam_Track(playerview_t *pv, usercmd_t *cmd)
{
	player_state_t *player, *self;
	inframe_t *frame;
	vec3_t vec;
	float len;

	if (!pv->spectator || !cl.worldmodel)	//can happen when the server changes level
		return;
	
	if (autotrackmode != TM_USER && pv->cam_state == CAM_FREECAM)
		Cam_CheckHighTarget(pv);

	if (pv->cam_state == CAM_FREECAM || cls.state != ca_active || cl.worldmodel->loadstate != MLS_LOADED)
		return;

	if (CAM_ISLOCKED(pv) && (!cl.players[pv->cam_spec_track].name[0] || cl.players[pv->cam_spec_track].spectator))
	{
		pv->cam_state = CAM_FREECAM;
		if (autotrackmode != TM_USER)
			Cam_CheckHighTarget(pv);
		else
			Cam_Unlock(pv);
		return;
	}

	frame = &cl.inframes[cl.validsequence & UPDATE_MASK];
	player = frame->playerstate + pv->cam_spec_track;
	if (!cmd)
	{	//this is our fancy force-wallcam mode.
		if (pv->cam_state != CAM_WALLCAM || !Cam_IsVisible(player->origin, pv->cam_desired_position))
		{
			if (!InitFlyby(pv, pv->cam_desired_position, player->origin, player->viewangles, true))
				InitFlyby(pv, pv->cam_desired_position, player->origin, player->viewangles, false);
		}
		return;
	}
	self = frame->playerstate + pv->playernum;

	if (!cl_chasecam.value && (pv->cam_state != CAM_WALLCAM || !Cam_IsVisible(player->origin, pv->cam_desired_position)))
	{
		if (pv->cam_state != CAM_WALLCAM || realtime - pv->cam_lastviewtime > 0.1)
		{
			if (!InitFlyby(pv, self->origin, player->origin, player->viewangles, true))
				InitFlyby(pv, self->origin, player->origin, player->viewangles, false);
			pv->cam_lastviewtime = realtime;
		}
	}
	else if (cl_chasecam.value && pv->cam_state == CAM_WALLCAM)
		pv->cam_state = CAM_PENDING;
	else
	{
		pv->cam_lastviewtime = realtime;
	}

	//tracking failed.
	if (pv->cam_state == CAM_FREECAM || pv->cam_state == CAM_PENDING)
		return;


	if (cl_chasecam.value)
	{
		float *neworg;
//		float *newang;
		if (pv->nolocalplayer)
			neworg = cl.lerpents[pv->viewentity].origin;
		else
			neworg = player->origin;

		pv->cam_lastviewtime = realtime;

//		VectorCopy(newang, pv->viewangles);
		if (memcmp(neworg, &self->origin, sizeof(vec3_t)) != 0)
		{
			if (!cls.demoplayback)
			{
				MSG_WriteByte (&cls.netchan.message, clc_tmove);
				MSG_WriteCoord (&cls.netchan.message, neworg[0]);
				MSG_WriteCoord (&cls.netchan.message, neworg[1]);
				MSG_WriteCoord (&cls.netchan.message, neworg[2]);
			}
			// move there locally immediately
			VectorCopy(neworg, self->origin);
		}
		self->weaponframe = player->weaponframe;

		VectorCopy(player->viewangles, pv->viewangles);
		return;
	}
	
	// Ok, move to our desired position and set our angles to view
	// the player
	VectorSubtract(pv->cam_desired_position, self->origin, vec);
	len = vlen(vec);
	cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
	if (len > 16)
	{ // close enough?
		MSG_WriteByte (&cls.netchan.message, clc_tmove);
		MSG_WriteCoord (&cls.netchan.message, pv->cam_desired_position[0]);
		MSG_WriteCoord (&cls.netchan.message, pv->cam_desired_position[1]);
		MSG_WriteCoord (&cls.netchan.message, pv->cam_desired_position[2]);
	}

	// move there locally immediately
	VectorCopy(pv->cam_desired_position, self->origin);

//	VectorSubtract(player->origin, pv->cam_desired_position, vec);
//	VectorAngles(vec, NULL, pv->viewangles);
//	pv->viewangles[0] = -pv->viewangles[0];
}

void Cam_SetModAutoTrack(int userid)
{	//this is a hint from the server about who to track
	int slot;
	cl.autotrack_hint = -1;
	for (slot = 0; slot < cl.allocated_client_slots; slot++)
	{
		if (cl.players[slot].userid == userid)
		{
			cl.autotrack_hint = slot;
			return;
		}
	}
	Con_Printf("//at: invalid userid %i\n", userid);
}

/*static void Cam_TrackCrosshairedPlayer(playerview_t *pv)
{
	inframe_t *frame;
	player_state_t *player;
	int i;
	float dot = 0.1, bestdot=0;
	int best = -1;
	vec3_t selforg;
	vec3_t dir;

	frame = &cl.inframes[cl.validsequence & UPDATE_MASK];
	player = frame->playerstate + pv->playernum;
	VectorCopy(player->origin, selforg);

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		player = frame->playerstate + i;
		VectorSubtract(player->origin, selforg, dir);
		VectorNormalize(dir);
		dot = DotProduct(vpn, dir);
		if (dot > bestdot)
		{
			bestdot = dot;
			best = i;
		}
	}
//	Con_Printf("Track %i? %f\n", best, bestdot);
	if (best > .707)	//did we actually get someone?
	{
		Cam_Lock(pv, best);
	}
}*/

void Cam_FinishMove(playerview_t *pv, usercmd_t *cmd)
{
	int i;
	player_info_t	*s;
	int end;
	extern cvar_t cl_demospeed;

	if (cls.state != ca_active)
		return;

	if (!pv->spectator && cls.demoplayback != DPB_MVD) // only in spectator mode
		return;

	if (cls.demoplayback == DPB_MVD)
	{
		int nb = 0;
		nb |= (cmd->sidemove<0)?4:0;
		nb |= (cmd->sidemove>0)?8:0;
		nb |= (cmd->forwardmove<0)?16:0;
		nb |= (cmd->forwardmove>0)?32:0;
		nb |= (cmd->upmove<0)?64:0;
		nb |= (cmd->upmove>0)?128:0;
		if (Cam_TrackNum(pv) >= 0)
		{
			if (*cls.lastdemoname)
			{	//changing rates or jumping doesn't make sense with mvds.
				if (nb & (nb ^ pv->cam_oldbuttons) & 4)
					Cvar_SetValue(&cl_demospeed, max(cl_demospeed.value - 0.1, 0));
				if (nb & (nb ^ pv->cam_oldbuttons) & 8)
					Cvar_SetValue(&cl_demospeed, min(cl_demospeed.value + 0.1, 10));
				if (nb & (nb ^ pv->cam_oldbuttons) & (4|8))
					Con_Printf("playback speed: %g%%\n", cl_demospeed.value*100);
				if (nb & (nb ^ pv->cam_oldbuttons) & 16)
					Cbuf_AddText("demo_jump +10", RESTRICT_LOCAL);
				if (nb & (nb ^ pv->cam_oldbuttons) & 32)
					Cbuf_AddText("demo_jump -10", RESTRICT_LOCAL);
				if (nb & (nb ^ pv->cam_oldbuttons) & (4|8))
					Con_Printf("playback speed: %g%%\n", cl_demospeed.value*100);
			}
			if (nb & (nb ^ pv->cam_oldbuttons) & 64)
				Cvar_SetValue(&cl_splitscreen, max(cl_splitscreen.ival - 1, 0));
			if (nb & (nb ^ pv->cam_oldbuttons) & 128)
				Cvar_SetValue(&cl_splitscreen, min(cl_splitscreen.ival + 1, MAX_SPLITS-1));
		}
		pv->cam_oldbuttons = (pv->cam_oldbuttons & 3) | (nb & ~3);
		if (cmd->impulse)
		{
			int pl = cmd->impulse;

#if 1
			do
			{
				Cam_Lock(pv, Cam_FindSortedPlayer(pl));
				pv++;
				pl++;
			} while (pv >= &cl.playerview[0] && pv < &cl.playerview[cl.splitclients]);
#else
			for (i = 0; ; i++)
			{
				if (i == MAX_CLIENTS)
				{
					if (pl == cmd->impulse)
						break;
					i = 0;
				}

				s = &cl.players[i];
				if (s->name[0] && !s->spectator)
				{
					pl--;
					if (!pl)
					{
						Cam_Lock(pv, i);
						if (pv >= &cl.playerview[0] && pv < &cl.playerview[cl.splitclients])
						{
							pl = 1;
							pv++;
						}
						else
							break;
					}
				}
			}
#endif
			return;
		}
	}

	if (cmd->buttons & BUTTON_ATTACK) 
	{
		if (!(pv->cam_oldbuttons & BUTTON_ATTACK)) 
		{
			pv->cam_oldbuttons |= BUTTON_ATTACK;

			if (pv->cam_state != CAM_FREECAM)
			{
				Cam_Unlock(pv);
				VectorCopy(pv->viewangles, cmd->angles);
				autotrackmode = TM_USER;
				return;
			}
		}
		else
			return;
	}
	else
	{
		pv->cam_oldbuttons &= ~BUTTON_ATTACK;
		if (pv->cam_state == CAM_FREECAM && autotrackmode == TM_USER)
		{
//			if ((cmd->buttons & BUTTON_JUMP) && !(pv->cam_oldbuttons & BUTTON_JUMP))
//				Cam_TrackCrosshairedPlayer(pv);
			pv->cam_oldbuttons = (pv->cam_oldbuttons&~BUTTON_JUMP) | (cmd->buttons & BUTTON_JUMP);
			return;
		}
	}

	if (autotrackmode != TM_USER) 
	{
		if ((cmd->buttons & BUTTON_JUMP) && !(pv->cam_oldbuttons & BUTTON_JUMP))
			autotrackmode = TM_USER;
		else
		{
			Cam_CheckHighTarget(pv);
			return;
		}
	}

	if (pv->cam_state != CAM_FREECAM) 
	{
		if ((cmd->buttons & BUTTON_JUMP) && (pv->cam_oldbuttons & BUTTON_JUMP))
			return;		// don't pogo stick

		if (!(cmd->buttons & BUTTON_JUMP)) 
		{
			pv->cam_oldbuttons &= ~BUTTON_JUMP;
			return;
		}
		pv->cam_oldbuttons |= BUTTON_JUMP;	// don't jump again until released
	}

//	Con_Printf("Selecting track target...\n");

	if (pv->cam_state != CAM_FREECAM)
		end =		 (pv->cam_spec_track + 1) % MAX_CLIENTS;
	else
		end = pv->cam_spec_track;
	end = max(0, end) % cl.allocated_client_slots;
	i = end;
	do 
	{
		s = &cl.players[i];
		//players with userid 0 are typically bots.
		//trying to spectate such bots just does not work (we have no idea which entity slot the bot is actually using, so don't try to track them).
		if (s->name[0] && !s->spectator && s->userid) 
		{
			Cam_Lock(pv, i);
			return;
		}
		i = (i + 1) % cl.allocated_client_slots;
	} while (i != end);
	// stay on same guy?
	i = pv->cam_spec_track;
	s = &cl.players[i];
	if (s->name[0] && !s->spectator && s->userid) 
	{
		Cam_Lock(pv, i);
		return;
	}
	Con_Printf("No target found ...\n");
	pv->cam_state = CAM_FREECAM;
}

void Cam_Reset(void)
{
	unsigned int pnum;
	for (pnum = 0; pnum < MAX_SPLITS; pnum++)
	{
		playerview_t *pv = &cl.playerview[pnum];
		pv->cam_state = CAM_FREECAM;
		pv->cam_spec_track = 0;
	}
}

static int QDECL Cam_SortPlayers(const void *p1,const void *p2)
{
	const player_info_t *a = *(player_info_t**)p1;
	const player_info_t *b = *(player_info_t**)p2;

	if (!*a->team)
		return *b->team;
	if (!*b->team)
		return -*a->team;
//	if (a->spectator || b->spectator)
//		return a->spectator - b->spectator;
	return Q_strcasecmp(a->team, b->team);
}
static int Cam_FindSortedPlayer(int number)
{
	player_info_t *playerlist[MAX_CLIENTS], *s;
	int i;
	int slots;
	number--;
	for (slots = 0, i = 0; i < cl.allocated_client_slots; i++)
	{
		s = &cl.players[i];
		if (s->name[0] && !s->spectator)
			playerlist[slots++] = s;
	}
	if (!slots)
		return 0;
//	if (number > slot)
//		return cl.allocated_client_slots;	//error

	qsort(playerlist, slots, sizeof(playerlist[0]), Cam_SortPlayers);
	return playerlist[number % slots] - cl.players;
}

void Cam_TrackPlayer(int seat, char *cmdname, char *plrarg)
{
	playerview_t *pv = &cl.playerview[seat];
	int slot;
	player_info_t	*s;
	char *e;

	if (seat >= MAX_SPLITS)
		return;

	if (cls.state <= ca_connected)
	{
		Con_Printf("Not connected.\n");
		return;
	}

	if (!pv->spectator)
	{
		Con_Printf("Not spectating.\n");
		return;
	}

	if (!Q_strcasecmp(plrarg, "off"))
	{
		Cam_Unlock(pv);
		return;
	}

	if (*plrarg == '#' && (slot=strtoul(plrarg+1, &e, 10)) && !*e)
		slot = Cam_FindSortedPlayer(slot);
	else
	{
		// search nicks first
		for (slot = 0; slot < cl.allocated_client_slots; slot++)
		{
			s = &cl.players[slot];
			if (s->name[0] && !s->spectator && !Q_strcasecmp(s->name, plrarg))
				break;
		}
	}

	if (slot == cl.allocated_client_slots)
	{
		// didn't find nick, so search userids
		int userid;
		char *c;

		// check if given arg is in fact a number
		c = plrarg;
		while (*c)
		{
			if (*c < '0' || *c > '9')
			{
				Con_Printf("Couldn't find nick %s\n", plrarg);
				return;
			}
			c++;
		}

		userid = atoi(plrarg);

		for (slot = 0; slot < cl.allocated_client_slots; slot++)
		{
			s = &cl.players[slot];
			if (s->name[0] && !s->spectator && s->userid == userid)
				break;
		}

		if (slot == cl.allocated_client_slots)
		{
			Con_Printf("Couldn't find userid %i\n", userid);
			return;
		}
	}

	Cam_Lock(pv, slot);
}

void Cam_Track_f(void)
{
	int i, j;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: %s userid|nick|off\n", Cmd_Argv(0));
		return;
	}

	i = 1;
	j = Cmd_Argc() - 1;
	if (j > MAX_SPLITS)
		j = MAX_SPLITS;

	while (j > 0)
	{
		Cam_TrackPlayer(i - 1, Cmd_Argv(0), Cmd_Argv(i));
		i++;
		j--;
	}
}

void Cam_Track1_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: %s userid|nick|off\n", Cmd_Argv(0));
		return;
	}

	Cam_TrackPlayer(0, Cmd_Argv(0), Cmd_Argv(1));
}

void Cam_Track2_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: %s userid|nick|off\n", Cmd_Argv(0));
		return;
	}

	Cam_TrackPlayer(1, Cmd_Argv(0), Cmd_Argv(1));
}

void Cam_Track3_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: %s userid|nick|off\n", Cmd_Argv(0));
		return;
	}

	Cam_TrackPlayer(2, Cmd_Argv(0), Cmd_Argv(1));
}

void Cam_Track4_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: %s userid|nick|off\n", Cmd_Argv(0));
		return;
	}

	Cam_TrackPlayer(3, Cmd_Argv(0), Cmd_Argv(1));
}

void CL_InitCam(void)
{
	Cvar_Register (&cl_autotrack, cl_spectatorgroup);
	Cvar_Register (&cl_autotrack_team, cl_spectatorgroup);
	Cvar_Register (&cl_hightrack, cl_spectatorgroup);
	Cvar_Register (&cl_chasecam, cl_spectatorgroup);
//	Cvar_Register (&cl_camera_maxpitch, cl_spectatorgroup);
//	Cvar_Register (&cl_camera_maxyaw, cl_spectatorgroup);
//	Cvar_Register (&cl_selfcam, cl_spectatorgroup);

	Cmd_AddCommand("autotrack", Cam_AutoTrack_f);	//reset it, if they jumped or something.
	Cmd_AddCommand("track", Cam_Track_f);
	Cmd_AddCommand("track1", Cam_Track1_f);
	Cmd_AddCommand("track2", Cam_Track2_f);
	Cmd_AddCommand("track3", Cam_Track3_f);
	Cmd_AddCommand("track4", Cam_Track4_f);
}


