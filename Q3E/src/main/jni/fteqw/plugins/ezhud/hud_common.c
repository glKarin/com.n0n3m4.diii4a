/*
Copyright (C) 2011 azazello and ezQuake team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//
// common HUD elements
// like clock etc..
//

#include "ezquakeisms.h"
/*
#include "common_draw.h"
#include "mp3_player.h"
#include <png.h>
#include "image.h"
#include "stats_grid.h"
#include "vx_stuff.h"
#include "gl_model.h"
#include "gl_local.h"
#include "tr_types.h"
#include "rulesets.h"
#include "utils.h"
#include "sbar.h"
#include "Ctrl.h"
#include "console.h"
#include "teamplay.h"
#include "mvd_utils.h"
*/
#include "hud.h"

//#define WITH_PNG	//more WITH_RADAR than anything else.

#define draw_disc draw_disc2

static mpic_t	*sb_ammo[4];
static mpic_t	*sb_faces[7][2];
static mpic_t	*sb_face_invis;
static mpic_t	*sb_face_quad;
static mpic_t	*sb_face_invuln;
static mpic_t	*sb_face_invis_invuln;
static mpic_t	*sb_weapons[7][8];
static mpic_t	*sb_items[6];
static mpic_t	*sb_sigil[4];
 mpic_t	*sb_nums[2][11];
static mpic_t	*sb_ibar;
static mpic_t	*sb_armor[3];
 mpic_t	*sb_colon;
static mpic_t	*sb_slash;
static mpic_t	*sb_disc;
static mpic_t	*sb_net;

void HUD_InitSbarImages(void)
{
	int i;
	sb_disc = Draw_CacheWadPic("disc");
	sb_net = Draw_CacheWadPic("net");

	for (i = 0; i < 10; i++) {
		sb_nums[0][i] = Draw_CacheWadPic (va("num_%i",i));
		sb_nums[1][i] = Draw_CacheWadPic (va("anum_%i",i));
	}

	sb_nums[0][10] = Draw_CacheWadPic ("num_minus");
	sb_nums[1][10] = Draw_CacheWadPic ("anum_minus");

	sb_colon = Draw_CacheWadPic ("num_colon");
	sb_slash = Draw_CacheWadPic ("num_slash");

	sb_weapons[0][0] = Draw_CacheWadPic ("inv_shotgun");
	sb_weapons[0][1] = Draw_CacheWadPic ("inv_sshotgun");
	sb_weapons[0][2] = Draw_CacheWadPic ("inv_nailgun");
	sb_weapons[0][3] = Draw_CacheWadPic ("inv_snailgun");
	sb_weapons[0][4] = Draw_CacheWadPic ("inv_rlaunch");
	sb_weapons[0][5] = Draw_CacheWadPic ("inv_srlaunch");
	sb_weapons[0][6] = Draw_CacheWadPic ("inv_lightng");

	sb_weapons[1][0] = Draw_CacheWadPic ("inv2_shotgun");
	sb_weapons[1][1] = Draw_CacheWadPic ("inv2_sshotgun");
	sb_weapons[1][2] = Draw_CacheWadPic ("inv2_nailgun");
	sb_weapons[1][3] = Draw_CacheWadPic ("inv2_snailgun");
	sb_weapons[1][4] = Draw_CacheWadPic ("inv2_rlaunch");
	sb_weapons[1][5] = Draw_CacheWadPic ("inv2_srlaunch");
	sb_weapons[1][6] = Draw_CacheWadPic ("inv2_lightng");

	for (i = 0; i < 5; i++) 
	{
		sb_weapons[2 + i][0] = Draw_CacheWadPic (va("inva%i_shotgun", i + 1));
		sb_weapons[2 + i][1] = Draw_CacheWadPic (va("inva%i_sshotgun", i + 1));
		sb_weapons[2 + i][2] = Draw_CacheWadPic (va("inva%i_nailgun", i + 1));
		sb_weapons[2 + i][3] = Draw_CacheWadPic (va("inva%i_snailgun", i + 1));
		sb_weapons[2 + i][4] = Draw_CacheWadPic (va("inva%i_rlaunch", i + 1));
		sb_weapons[2 + i][5] = Draw_CacheWadPic (va("inva%i_srlaunch", i + 1));
		sb_weapons[2 + i][6] = Draw_CacheWadPic (va("inva%i_lightng", i + 1));
	}

	sb_ammo[0] = Draw_CacheWadPic ("sb_shells");
	sb_ammo[1] = Draw_CacheWadPic ("sb_nails");
	sb_ammo[2] = Draw_CacheWadPic ("sb_rocket");
	sb_ammo[3] = Draw_CacheWadPic ("sb_cells");

	sb_armor[0] = Draw_CacheWadPic ("sb_armor1");
	sb_armor[1] = Draw_CacheWadPic ("sb_armor2");
	sb_armor[2] = Draw_CacheWadPic ("sb_armor3");

	sb_items[0] = Draw_CacheWadPic ("sb_key1");
	sb_items[1] = Draw_CacheWadPic ("sb_key2");
	sb_items[2] = Draw_CacheWadPic ("sb_invis");
	sb_items[3] = Draw_CacheWadPic ("sb_invuln");
	sb_items[4] = Draw_CacheWadPic ("sb_suit");
	sb_items[5] = Draw_CacheWadPic ("sb_quad");

	sb_sigil[0] = Draw_CacheWadPic ("sb_sigil1");
	sb_sigil[1] = Draw_CacheWadPic ("sb_sigil2");
	sb_sigil[2] = Draw_CacheWadPic ("sb_sigil3");
	sb_sigil[3] = Draw_CacheWadPic ("sb_sigil4");

	sb_faces[4][0] = Draw_CacheWadPic ("face1");
	sb_faces[4][1] = Draw_CacheWadPic ("face_p1");
	sb_faces[3][0] = Draw_CacheWadPic ("face2");
	sb_faces[3][1] = Draw_CacheWadPic ("face_p2");
	sb_faces[2][0] = Draw_CacheWadPic ("face3");
	sb_faces[2][1] = Draw_CacheWadPic ("face_p3");
	sb_faces[1][0] = Draw_CacheWadPic ("face4");
	sb_faces[1][1] = Draw_CacheWadPic ("face_p4");
	sb_faces[0][0] = Draw_CacheWadPic ("face5");
	sb_faces[0][1] = Draw_CacheWadPic ("face_p5");

	sb_face_invis = Draw_CacheWadPic ("face_invis");
	sb_face_invuln = Draw_CacheWadPic ("face_invul2");
	sb_face_invis_invuln = Draw_CacheWadPic ("face_inv2");
	sb_face_quad = Draw_CacheWadPic ("face_quad");

	sb_ibar = Draw_CacheWadPic("ibar");
}

plugnetinfo_t *GetNetworkInfo(void)
{
	static plugnetinfo_t ni;
	static int uc;
	if (uc != host_screenupdatecount)
	{
		uc = host_screenupdatecount;
		clientfuncs->GetNetworkInfo(&ni, sizeof(ni));
	}
	return &ni;
}


#ifndef STAT_MINUS
#define STAT_MINUS		10
#endif

hud_t *hud_netgraph = NULL;

// ----------------
// HUD planning
//

struct
{
	// this is temporary storage place for some of user's settings
	// hud_* values will be dumped into config file
	int old_multiview;
	int old_fov;
	int old_newhud;

	qbool active;
} autohud;

void OnAutoHudChange(cvar_t *var, char *value, qbool *cancel);
qbool autohud_loaded = false;
cvar_t *hud_planmode;
cvar_t *mvd_autohud;
cvar_t *hud_digits_trim;
cvar_t *cl_multiview;

int hud_stats[MAX_CL_STATS];

cvar_t *cl_weaponpreselect;
extern int IN_BestWeapon(void);
extern void DumpHUD(char *);
extern char *Macro_MatchType(void);

int HUD_Stats(int stat_num)
{
    if (hud_planmode->value)
        return hud_stats[stat_num];
    else
        return cl.stats[stat_num];
}

// ----------------
// HUD low levels
//

cvar_t *hud_tp_need;

/* tp need levels
int TP_IsHealthLow(void);
int TP_IsArmorLow(void);
int TP_IsAmmoLow(int weapon); */
cvar_t *tp_need_health, *tp_need_ra, *tp_need_ya, *tp_need_ga,
		*tp_weapon_order, *tp_need_weapon, *tp_need_shells,
		*tp_need_nails, *tp_need_rockets, *tp_need_cells;

int State_AmmoNumForWeapon(int weapon)
{	// returns ammo number (shells = 1, nails = 2, rox = 3, cells = 4) for given weapon
	switch (weapon) {
		case 2: case 3: return 1;
		case 4: case 5: return 2;
		case 6: case 7: return 3;
		case 8: return 4;
		default: return 0;
	}
}

int State_AmmoForWeapon(int weapon)
{	// returns ammo amount for given weapon
	int ammon = State_AmmoNumForWeapon(weapon);

	if (ammon)
		return cl.stats[STAT_SHELLS + ammon - 1];
	else
		return 0;
}

int TP_IsHealthLow(void)
{
    return cl.stats[STAT_HEALTH] <= tp_need_health->value;
}

int TP_IsArmorLow(void)
{
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR3))
        return cl.stats[STAT_ARMOR] <= tp_need_ra->value;
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR2))
        return cl.stats[STAT_ARMOR] <= tp_need_ya->value;
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR1))
        return cl.stats[STAT_ARMOR] <= tp_need_ga->value;
    return 1;
}

int TP_IsWeaponLow(void)
{
    char *s = tp_weapon_order->string;
    while (*s  &&  *s != tp_need_weapon->string[0])
    {
        if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << (*s-'0'-2)))
            return false;
        s++;
    }
    return true;
}

int TP_IsAmmoLow(int weapon)
{
    int ammo = State_AmmoForWeapon(weapon);
    switch (weapon)
    {
    case 2:
    case 3:  return ammo <= tp_need_shells->value;
    case 4:
    case 5:  return ammo <= tp_need_nails->value;
    case 6:
    case 7:  return ammo <= tp_need_rockets->value;
    case 8:  return ammo <= tp_need_cells->value;
    default: return 0;
    }
}

int TP_TeamFortressEngineerSpanner(void)
{
#ifdef HAXX
	char *player_skin=Info_ValueForKey(cl.players[cl.playernum].userinfo,"skin");
	char *model_name=cl.model_precache[cl.viewent.current.modelindex]->name;
	if (cl.teamfortress && player_skin
			&& (strcasecmp(player_skin, "tf_eng") == 0)
			&& model_name
			&& (strcasecmp(model_name, "progs/v_span.mdl") == 0))
	{
		return 1;
	}
	else
#endif
	{
		return 0;
	}
}

qbool HUD_HealthLow(void)
{
    if (hud_tp_need->value)
        return TP_IsHealthLow();
    else
        return HUD_Stats(STAT_HEALTH) <= 25;
}

qbool HUD_ArmorLow(void)
{
    if (hud_tp_need->value)
        return (TP_IsArmorLow());
    else
        return (HUD_Stats(STAT_ARMOR) <= 25);
}

qbool HUD_AmmoLow(void)
{
    if (hud_tp_need->value)
    {
        if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
            return TP_IsAmmoLow(2);
        else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
            return TP_IsAmmoLow(4);
        else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
            return TP_IsAmmoLow(6);
        else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
            return TP_IsAmmoLow(8);
        return false;
    }
    else
        return (HUD_Stats(STAT_AMMO) <= 10);
}

int HUD_AmmoLowByWeapon(int weapon)
{
    if (hud_tp_need->value)
        return TP_IsAmmoLow(weapon);
    else
    {
        int a;
        switch (weapon)
        {
        case 2:
        case 3:
            a = STAT_SHELLS; break;
        case 4:
        case 5:
            a = STAT_NAILS; break;
        case 6:
        case 7:
            a = STAT_ROCKETS; break;
        case 8:
            a = STAT_CELLS; break;
        default:
            return false;
        }
        return (HUD_Stats(a) <= 10);
    }
}

// ----------------
// DrawFPS
void SCR_HUD_DrawFPS(hud_t *hud)
{
    int x, y;
    char st[128];

    static cvar_t
	    *hud_fps_show_min = NULL,
	    *hud_fps_style,
	    *hud_fps_title,
	    *hud_fps_drop;

    if (hud_fps_show_min == NULL)   // first time called
    {
	    hud_fps_show_min = HUD_FindVar(hud, "show_min");
	    hud_fps_style    = HUD_FindVar(hud, "style");
	    hud_fps_title    = HUD_FindVar(hud, "title");
	    hud_fps_drop = HUD_FindVar(hud, "drop");
    }

    if (hud_fps_show_min->value)
        snprintf (st, sizeof (st), "%3d^Ue00f%3d", (int)(cls.min_fps + 0.25), (int) (cls.fps + 0.25));
    else
        snprintf (st, sizeof (st), "%3d", (int)(cls.fps + 0.25));

    if (hud_fps_title->value)
        strlcat (st, " fps", sizeof (st));

    if (HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
    {
		plugnetinfo_t *netinfo = GetNetworkInfo();
		if (netinfo->capturing == 2)	//don't show fps if its locked to something anyway.
			return;

		if ((hud_fps_style->value) == 1)
			Draw_Alt_String(x, y, st);
		else if ((hud_fps_style->value) == 2) {
			if ((hud_fps_drop->value) >= cls.fps) // if fps is less than a user-set value, then show it
				Draw_String(x, y, st);
		}
		else if ((hud_fps_style->value) == 3) {
			if ((hud_fps_drop->value) >= cls.fps) // if fps is less than a user-set value, then show it
				Draw_Alt_String(x, y, st);
		}
		else // hud_fps_style is anything other than 1,2,3
			Draw_String(x, y, st);
    }
}

void SCR_HUD_DrawVidLag(hud_t *hud)
{
	int x, y;
	char st[128];
	static cvar_t *hud_vidlag_style = NULL;

	plugnetinfo_t *netinfo = GetNetworkInfo();
	static double old_lag;

	if (netinfo->vlatency)
	{
		// take the average of last two values, otherwise it
		// changes very fast and is hard to read
		double current, avg;
		current = netinfo->vlatency;
		avg = (current + old_lag) * 0.5;
		old_lag = current;
		snprintf (st, sizeof (st), "%2.1f", avg * 1000);
	}
	else
		strcpy(st, "?");

	if (hud_vidlag_style == NULL)  // first time called
	{
		hud_vidlag_style = HUD_FindVar(hud, "style");
	}

	strlcat (st, " ms", sizeof (st));

	if (HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
	{
		if (hud_vidlag_style->value)
		{
			Draw_Alt_String(x, y, st);
		}
		else
		{
			Draw_String(x, y, st);
		}
	}
}

void SCR_HUD_DrawMouserate(hud_t *hud)
{
	int x, y;
	static int lastresult = 0;
	int newresult;
	char st[80];	// string buffer
	double t;		// current time
	static double lastframetime;	// last refresh
	plugnetinfo_t *netinfo = GetNetworkInfo();

    static cvar_t *hud_mouserate_title = NULL,
		*hud_mouserate_interval,
		*hud_mouserate_style;

    if (hud_mouserate_title == NULL) // first time called
    {
		hud_mouserate_style    = HUD_FindVar(hud, "style");
        hud_mouserate_title    = HUD_FindVar(hud, "title");
		hud_mouserate_interval = HUD_FindVar(hud, "interval");
    }

	t = cls.realtime;
	if ((t - lastframetime) >= hud_mouserate_interval->value) {
		newresult = netinfo->mrate;
		lastframetime = t;
	} else
		newresult = 0;

	if (newresult > 0) {
		snprintf(st, sizeof(st), "%4d", newresult);
		lastresult = newresult;
	} else if (!newresult)
		snprintf(st, sizeof(st), "%4d", lastresult);
	else
		snprintf(st, sizeof(st), "n/a");

    if (hud_mouserate_title->value)
        strlcat(st, " Hz", sizeof (st));

    if (HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
    {
		if (hud_mouserate_style->value)
		{
			Draw_Alt_String(x, y, st);
		}
		else
		{
			Draw_String(x, y, st);
		}
    }
}

#define MAX_TRACKING_STRING		512

void SCR_HUD_DrawTracking(hud_t *hud)
{
#ifdef HAXX
	static char tracked_strings[MV_VIEWS][MAX_TRACKING_STRING];
	static int tracked[MV_VIEWS] = {-1, -1, -1, -1};
	int view = 0;
#endif
	int views = 1;
    int x = 0, y = 0, width = 0, height = 0;
    char track_string[MAX_TRACKING_STRING];

	static cvar_t *hud_tracking_format = NULL,
		*hud_tracking_scale;

	if (!hud_tracking_format) {
		hud_tracking_format = HUD_FindVar(hud, "format");
		hud_tracking_scale = HUD_FindVar(hud, "scale");
	}

	strlcpy(track_string, hud_tracking_format->string, sizeof(track_string));

#ifdef HAXX
	if(cls.mvdplayback && cl_multiview->value && CURRVIEW > 0)
	{
		//
		// Multiview.
		//

		views = cl_multiview->value;

		// Save the currently tracked player for the slot being drawn
		// (this will be done for all views and we'll get a complete
		// list over who we're tracking).
		tracked[CURRVIEW - 1] = spec_track;

		for(view = 0; view < MV_VIEWS; view++)
		{
			int new_width = 0;

			// We haven't found who we're tracking in this view.
			if(tracked[view] < 0)
			{
				continue;
			}

			strlcpy(tracked_strings[view], hud_tracking_format->string, sizeof(tracked_strings[view]));

			Replace_In_String(tracked_strings[view], sizeof(tracked_strings[view]), '%', 3,
				"v", cl_multiview->value ? va("%d", view+1) : "",			// Replace %v with the current view (in multiview)
				"n", cl.players[tracked[view]].name,						// Replace %n with player name.
				"t", cl.teamplay ? cl.players[tracked[view]].team : "");	// Replace %t with player team if teamplay is on.

			// Set the width.
			new_width = 8 * strlen_color(tracked_strings[view]);
			width = (new_width > width) ? new_width : width;
		}
	}
	else
#endif
	{
		// Normal.
		Replace_In_String(track_string, sizeof(track_string), '%', 2,
			"n", cl.players[spec_track].name,						// Replace %n with player name.
			"t", cl.teamplay ? cl.players[spec_track].team : "");	// Replace %t with player team if teamplay is on.
		width = 8 * strlen_color(track_string);
	}

	height = 8 * views;
	height *= hud_tracking_scale->value;
	width *= hud_tracking_scale->value;

	if (!(cl.spectator && autocam == CAM_TRACK))
		height = 0;

	if(!HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		return;
	}

	if (height == 0)
		return;

#ifdef HAXX
	if (cls.mvdplayback && cl_multiview->value && autocam == CAM_TRACK)
	{
		// Multiview
		for(view = 0; view < MV_VIEWS; view++)
		{
			if(tracked[view] < 0 || CURRVIEW <= 0)
			{
				continue;
			}
			Draw_SString(x, y + view*8, tracked_strings[view], hud_tracking_scale->value);
		}
	}
	else
#endif
		if (cl.spectator && autocam == CAM_TRACK && !cl_multiview->value)
	{
		// Normal
		Draw_SString(x, y, track_string, hud_tracking_scale->value);
	}
}

#ifdef HAXX
void R_MQW_NetGraph(int outgoing_sequence, int incoming_sequence, int *packet_latency,
                int lost, int minping, int avgping, int maxping, int devping,
                int posx, int posy, int width, int height, int revx, int revy);
// ----------------
// Netgraph
static void SCR_HUD_Netgraph(hud_t *hud)
{
    static cvar_t
        *par_width = NULL, *par_height,
        *par_swap_x, *par_swap_y,
        *par_ploss;

    if (par_width == NULL)  // first time
    {
        par_width  = HUD_FindVar(hud, "width");
        par_height = HUD_FindVar(hud, "height");
        par_swap_x = HUD_FindVar(hud, "swap_x");
        par_swap_y = HUD_FindVar(hud, "swap_y");
        par_ploss  = HUD_FindVar(hud, "ploss");
    }

    R_MQW_NetGraph(cls.netchan.outgoing_sequence, cls.netchan.incoming_sequence,
        packet_latency, par_ploss->value ? CL_CalcNet() : -1, -1, -1, -1, -1, -1,
        -1, (int)par_width->value, (int)par_height->value,
        (int)par_swap_x->value, (int)par_swap_y->value);
}
#endif

//---------------------
//
// draw HUD ping
//
static void SCR_HUD_DrawPing(hud_t *hud)
{
    double t;
    static double last_calculated;
    static int ping_avg, pl, ping_min, ping_max;
    static float ping_dev;

    int width, height;
    int x, y;
    char buf[512];
	plugnetinfo_t *netinfo = GetNetworkInfo();

    static cvar_t
		*hud_ping_period = NULL,
        *hud_ping_show_pl,
        *hud_ping_show_dev,
        *hud_ping_show_min,
        *hud_ping_show_max,
		*hud_ping_style,
        *hud_ping_blink;

    if (hud_ping_period == NULL)    // first time
    {
        hud_ping_period   = HUD_FindVar(hud, "period");
        hud_ping_show_pl  = HUD_FindVar(hud, "show_pl");
        hud_ping_show_dev = HUD_FindVar(hud, "show_dev");
        hud_ping_show_min = HUD_FindVar(hud, "show_min");
        hud_ping_show_max = HUD_FindVar(hud, "show_max");
		hud_ping_style    = HUD_FindVar(hud, "style");
        hud_ping_blink    = HUD_FindVar(hud, "blink");
    }

    t = cls.realtime;
    if (t - last_calculated  >  hud_ping_period->value)
    {
//        float period;

        last_calculated = t;

//        period = max(hud_ping_period->value, 0);

        ping_avg = (int)(netinfo->ping.s_avg*1000 + 0.5);
        ping_min = (int)(netinfo->ping.s_mn*1000 + 0.5);
        ping_max = (int)(netinfo->ping.s_mx*1000 + 0.5);
        ping_dev = netinfo->ping.ms_stddev;
		pl = netinfo->loss.dropped*100;

        clamp(ping_avg, 0, 999);
        clamp(ping_min, 0, 999);
        clamp(ping_max, 0, 999);
        clamp(ping_dev, 0, 99.9);
        clamp(pl, 0, 100);
    }

    buf[0] = 0;

    // blink
    if (hud_ping_blink->value)   // add dot
        strlcat (buf, (last_calculated + hud_ping_period->value/2 > cls.realtime) ? "^Ue08f" : " ", sizeof (buf));

    // min ping
    if (hud_ping_show_min->value)
        strlcat (buf, va("%d^Ue00f", ping_min), sizeof (buf));

    // ping
    strlcat (buf, va("%d", ping_avg), sizeof (buf));

    // max ping
    if (hud_ping_show_max->value)
        strlcat (buf, va("^Ue00f%d", ping_max), sizeof (buf));

    // unit
    strlcat (buf, " ms", sizeof (buf));

    // standard deviation
    if (hud_ping_show_dev->value)
        strlcat (buf, va(" (%.1f)", ping_dev), sizeof (buf));

    // pl
    if (hud_ping_show_pl->value)
        strlcat (buf, va(" ^Ue08f %d%%", pl), sizeof (buf));

    // display that on screen
    width = strlen(buf) * 8;
    height = 8;

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
		if (hud_ping_style->value)
		{
			Draw_Alt_String(x, y, buf);
		}
		else
		{
			Draw_String(x, y, buf);
		}
    }
}

static const char *SCR_HUD_ClockFormat(int format)
{
	switch (format) {
		case 1: return "%I:%M %p";
		case 2: return "%I:%M:%S %p";
		case 3: return "%H:%M";
		default: case 0: return "%H:%M:%S";
	}
}

//---------------------
//
// draw HUD clock
//
void SCR_HUD_DrawClock(hud_t *hud)
{
    int width, height;
    int x, y;
	const char *t;

    static cvar_t
        *hud_clock_big = NULL,
        *hud_clock_style,
        *hud_clock_blink,
		*hud_clock_scale,
		*hud_clock_format;

    if (hud_clock_big == NULL)    // first time
    {
        hud_clock_big   = HUD_FindVar(hud, "big");
        hud_clock_style = HUD_FindVar(hud, "style");
        hud_clock_blink = HUD_FindVar(hud, "blink");
		hud_clock_scale = HUD_FindVar(hud, "scale");
		hud_clock_format= HUD_FindVar(hud, "format");
    }

	t = SCR_GetTimeString(TIMETYPE_CLOCK, SCR_HUD_ClockFormat(hud_clock_format->ival));
	width = SCR_GetClockStringWidth(t, hud_clock_big->ival, hud_clock_scale->value);
	height = SCR_GetClockStringHeight(hud_clock_big->ival, hud_clock_scale->value);

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_clock_big->value)
            SCR_DrawBigClock(x, y, hud_clock_style->value, hud_clock_blink->value, hud_clock_scale->value, t);
        else
            SCR_DrawSmallClock(x, y, hud_clock_style->value, hud_clock_blink->value, hud_clock_scale->value, t);
    }
}

//---------------------
//
// draw HUD notify
//

static void SCR_HUD_DrawNotify(hud_t* hud)
{
	static cvar_t* hud_notify_rows = NULL;
	static cvar_t* hud_notify_scale;
	static cvar_t* hud_notify_time;
	static cvar_t* hud_notify_cols;

	int x;
	int y;
	int width;
	int height;

	if (hud_notify_rows == NULL) // First time.
	{
       hud_notify_rows  = HUD_FindVar(hud, "rows");
       hud_notify_cols  = HUD_FindVar(hud, "cols");
       hud_notify_scale = HUD_FindVar(hud, "scale");
       hud_notify_time  = HUD_FindVar(hud, "time");
	}

	height = hud_notify_rows->ival * 8 * hud_notify_scale->value;
	width  = 8 * hud_notify_cols->ival * hud_notify_scale->value;

	if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		cvarfuncs->SetFloat("con_notify_x",		(float)x / vid.width);
		cvarfuncs->SetFloat("con_notify_y",		(float)y / vid.height);
		cvarfuncs->SetFloat("con_notify_w",		(float)width / vid.width);
		cvarfuncs->SetFloat("con_numnotifylines",(int)(height/(8*hud_notify_scale->value) + 0.01));
		cvarfuncs->SetFloat("con_notifytime",	(float)hud_notify_time->ival);
		cvarfuncs->SetFloat("con_textsize",		8.0 * hud_notify_scale->value);
//		SCR_DrawNotify(x, y, hud_notify_scale->value, hud_notify_time->ival, hud_notify_rows->ival, hud_notify_cols->ival);
	}
}

//---------------------
//
// draw HUD gameclock
//
void SCR_HUD_DrawGameClock(hud_t *hud)
{
    int width, height;
    int x, y;
	int timetype;
	const char *t;

    static cvar_t
        *hud_gameclock_big = NULL,
        *hud_gameclock_style,
        *hud_gameclock_blink,
		*hud_gameclock_countdown,
		*hud_gameclock_scale
//		*hud_gameclock_offset
		;

    if (hud_gameclock_big == NULL)    // first time
    {
        hud_gameclock_big   = HUD_FindVar(hud, "big");
        hud_gameclock_style = HUD_FindVar(hud, "style");
        hud_gameclock_blink = HUD_FindVar(hud, "blink");
		hud_gameclock_countdown = HUD_FindVar(hud, "countdown");
		hud_gameclock_scale = HUD_FindVar(hud, "scale");
//		hud_gameclock_offset = HUD_FindVar(hud, "offset");
//		gameclockoffset = &hud_gameclock_offset->ival;
    }

	timetype = (hud_gameclock_countdown->value) ? TIMETYPE_GAMECLOCKINV : TIMETYPE_GAMECLOCK;
	t = SCR_GetTimeString(timetype, NULL);
	width = SCR_GetClockStringWidth(t, hud_gameclock_big->ival, hud_gameclock_scale->value);
	height = SCR_GetClockStringHeight(hud_gameclock_big->ival, hud_gameclock_scale->value);

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_gameclock_big->value)
            SCR_DrawBigClock(x, y, hud_gameclock_style->value, hud_gameclock_blink->value, hud_gameclock_scale->value, t);
        else
            SCR_DrawSmallClock(x, y, hud_gameclock_style->value, hud_gameclock_blink->value, hud_gameclock_scale->value, t);
    }
}

//---------------------
//
// draw HUD democlock
//
void SCR_HUD_DrawDemoClock(hud_t *hud)
{
    int width = 0;
	int height = 0;
    int x = 0;
	int y = 0;
	const char *t;
    static cvar_t
        *hud_democlock_big = NULL,
        *hud_democlock_style,
        *hud_democlock_blink,
		*hud_democlock_scale;

	if (!cls.demoplayback || cls.mvdplayback == 2)
	{
		HUD_PrepareDraw(hud, width, height, &x, &y);
		return;
	}

    if (hud_democlock_big == NULL)    // first time
    {
        hud_democlock_big   = HUD_FindVar(hud, "big");
        hud_democlock_style = HUD_FindVar(hud, "style");
        hud_democlock_blink = HUD_FindVar(hud, "blink");
		hud_democlock_scale = HUD_FindVar(hud, "scale");
    }

	t = SCR_GetTimeString(TIMETYPE_DEMOCLOCK, NULL);
	width = SCR_GetClockStringWidth(t, hud_democlock_big->ival, hud_democlock_scale->value);
	height = SCR_GetClockStringHeight(hud_democlock_big->ival, hud_democlock_scale->value);

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_democlock_big->value)
            SCR_DrawBigClock(x, y, hud_democlock_style->value, hud_democlock_blink->value, hud_democlock_scale->value, t);
        else
            SCR_DrawSmallClock(x, y, hud_democlock_style->value, hud_democlock_blink->value, hud_democlock_scale->value, t);
	}
}

//---------------------
//
// network statistics
//
static void SCR_NetStats(int x, int y, float period, plugnetinfo_t *netinfo)
{
    char line[128];
    double t;

    // static data
    static double last_calculated;
    static int    ping_min, ping_max, ping_avg;
    static float  ping_dev;
    static float  f_min, f_max, f_avg;
    static int    lost_lost, lost_delta, lost_rate, lost_total;
    static int    size_all, size_in, size_out, pps_in, pps_out;
    static int    bandwidth_all, bandwidth_in, bandwidth_out;
    static int    with_delta;

    if (cls.state != ca_active)
        return;

    if (period < 0)
        period = 0;

    t = cls.realtime;
    if (t - last_calculated > period)
    {
        // recalculate

        last_calculated = t;

        ping_avg = (int)(netinfo->ping.s_avg*1000 + 0.5);
        ping_min = (int)(netinfo->ping.s_mn*1000 + 0.5);
        ping_max = (int)(netinfo->ping.s_mx*1000 + 0.5);
        ping_dev = netinfo->ping.ms_stddev;

        clamp(ping_avg, 0, 999);
        clamp(ping_min, 0, 999);
        clamp(ping_max, 0, 999);
        clamp(ping_dev, 0, 99.9);

        f_avg = (int)(netinfo->ping.fr_avg+0.5);
        f_min = netinfo->ping.fr_mn;
        f_max = netinfo->ping.fr_mx;

        clamp(f_avg, 0, 99);
        clamp(f_min, 0, 99);
        clamp(f_max, 0, 99);

        lost_lost     = (int)(netinfo->loss.dropped*100  + 0.5);
        lost_rate     = (int)(netinfo->loss.choked*100  + 0.5);
        lost_delta    = (int)(netinfo->loss.invalid*100  + 0.5);
        lost_total    = (int)((netinfo->loss.dropped + netinfo->loss.choked + netinfo->loss.invalid)*100 + 0.5);

        clamp(lost_lost,  0, 100);
        clamp(lost_rate,  0, 100);
        clamp(lost_delta, 0, 100);
        clamp(lost_total, 0, 100);

		pps_in = netinfo->clrate.in_pps;
		pps_out = netinfo->clrate.out_pps;

		//per packet sizes
        size_in  = (int)(netinfo->clrate.in_bps/netinfo->clrate.in_pps + 0.5);
        size_out = (int)(netinfo->clrate.out_bps/netinfo->clrate.out_pps + 0.5);
        size_all = (int)(netinfo->clrate.in_bps/netinfo->clrate.in_pps + netinfo->clrate.out_bps/netinfo->clrate.out_pps + 0.5);

		//overall rate
        bandwidth_in  = (int)(netinfo->clrate.in_bps  + 0.5);
        bandwidth_out = (int)(netinfo->clrate.out_bps + 0.5);
        bandwidth_all = (int)(netinfo->clrate.in_bps + netinfo->clrate.out_bps + 0.5);

        clamp(size_in,  0, 999);
        clamp(size_out, 0, 999);
        clamp(size_all, 0, 999);
        clamp(bandwidth_in,  0, 99999);
        clamp(bandwidth_out, 0, 99999);
        clamp(bandwidth_all, 0, 99999);

        with_delta = !cvarfuncs->GetFloat("cl_nodelta");
    }

    Draw_Alt_String(x+36, y, "latency");
    y+=12;

    snprintf (line, sizeof (line), "min  %4f %3d ms", f_min, ping_min);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "avg  %4f %3d ms", f_avg, ping_avg);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "max  %4f %3d ms", f_max, ping_max);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "dev     %f ms", ping_dev);
    Draw_String(x, y, line);
    y+=12;

    Draw_Alt_String(x+20, y, "packet loss");
    y+=12;

    snprintf(line, sizeof (line), "lost       %3d %%", lost_lost);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "rate cut   %3d %%", lost_rate);
    Draw_String(x, y, line);
    y+=8;

    if (with_delta)
        snprintf(line, sizeof (line), "bad delta  %3d %%", lost_delta);
    else
        strlcpy (line, "no delta compr", sizeof (line));
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "total      %3d %%", lost_total);
    Draw_String(x, y, line);
    y+=12;


    Draw_Alt_String(x+4, y, "packet size/BPS");
    y+=12;

    snprintf(line, sizeof (line), "out    %3d %5d %d", size_out, bandwidth_out, pps_out);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "in     %3d %5d %3d", size_in, bandwidth_in, pps_in);
    Draw_String(x, y, line);
    y+=8;

    snprintf(line, sizeof (line), "total  %3d %5d %3d", size_all, bandwidth_all, pps_in+pps_out);
    Draw_String(x, y, line);
    y+=8;
}

static void SCR_HUD_DrawNetStats(hud_t *hud)
{
    int width, height;
    int x, y;

	plugnetinfo_t *netinfo = GetNetworkInfo();

    static cvar_t *hud_net_period = NULL;

    if (hud_net_period == NULL)    // first time
    {
        hud_net_period = HUD_FindVar(hud, "period");
    }

    width = 16*8 ;
    height = 12 + 8 + 8 + 8 + 8 + 16 + 8 + 8 + 8 + 8 + 16 + 8 + 8 + 8;

	if (!netinfo || netinfo->capturing==2)
		HUD_PrepareDraw(hud, 0, 0, &x, &y);
	else if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
        SCR_NetStats(x, y, hud_net_period->value, netinfo);
	}
}

#define SPEED_GREEN				"52"
#define SPEED_BROWN_RED			"100"
#define SPEED_DARK_RED			"72"
#define SPEED_BLUE				"216"
#define SPEED_RED				"229"

#define	SPEED_STOPPED			SPEED_GREEN
#define	SPEED_NORMAL			SPEED_BROWN_RED
#define	SPEED_FAST				SPEED_DARK_RED
#define	SPEED_FASTEST			SPEED_BLUE
#define	SPEED_INSANE			SPEED_RED

//---------------------
//
// speed-o-meter
//

#define SPEED_TAG_LENGTH		2
#define SPEED_OUTLINE_SPACING	SPEED_TAG_LENGTH
#define SPEED_FILL_SPACING		SPEED_OUTLINE_SPACING + 1
#define SPEED_WHITE				10
#define SPEED_TEXT_ONLY			1

#define	SPEED_TEXT_ALIGN_NONE	0
#define SPEED_TEXT_ALIGN_CLOSE	1
#define SPEED_TEXT_ALIGN_CENTER	2
#define SPEED_TEXT_ALIGN_FAR	3

// FIXME: hud-only now, can/should be moved
void SCR_DrawHUDSpeed (
	int x, int y, int width, int height,
	int type,
	float tick_spacing,
	float opacity,
	int vertical,
	int vertical_text,
	int text_align,
	byte color_stopped,
	byte color_normal,
	byte color_fast,
	byte color_fastest,
	byte color_insane,
	int style,
	float scale
)
{
	byte color_offset;
	byte color1, color2;
	int player_speed;
	vec_t *velocity;

	if (scr_con_current == vid.height) {
		return;     // console is full screen
	}

	// Get the velocity.
//	if (cl.players[cl.playernum].spectator && Cam_TrackNum() >= 0) {
//		velocity = cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK].playerstate[Cam_TrackNum()].velocity;
//	}
//	else {
		velocity = cl.simvel;
//	}

	// Calculate the speed
	if (!type)
	{
		// Based on XY.
		player_speed = sqrt(velocity[0]*velocity[0]
						  + velocity[1]*velocity[1]);
	}
	else
	{
		// Based on XYZ.
		player_speed = sqrt(velocity[0]*velocity[0]
						  + velocity[1]*velocity[1]
						  + velocity[2]*velocity[2]);
	}

	// Calculate the color offset for the "background color".
	if (vertical) {
		color_offset = height * (player_speed % 500) / 500;
	}
	else {
		color_offset = width * (player_speed % 500) / 500;
	}

	// Set the color based on the speed.
	switch (player_speed / 500)
	{
		case 0:
			color1 = color_stopped;
			color2 = color_normal;
			break;
		case 1:
			color1 = color_normal;
			color2 = color_fast;
			break;
		case 2:
			color1 = color_fast;
			color2 = color_fastest;
			break;
		default:
			color1 = color_fastest;
			color2 = color_insane;
			break;
	}

	// Draw tag marks.
	if (tick_spacing > 0.0 && style != SPEED_TEXT_ONLY)
	{
		float f;

		for(f = tick_spacing; f < 1.0; f += tick_spacing)
		{
			if(vertical)
			{
				// Left.
				Draw_AlphaFill(x,				// x
					y + (int)(f * height),	// y
					SPEED_TAG_LENGTH,		// Width
					1,						// Height
					SPEED_WHITE,			// Color
					opacity);				// Opacity

				// Right.
				Draw_AlphaFill(x + width - SPEED_TAG_LENGTH + 1,
					y + (int)(f * height),
					SPEED_TAG_LENGTH,
					1,
					SPEED_WHITE,
					opacity);
			}
			else
			{
				// Above.
				Draw_AlphaFill(x + (int)(f * width),
					y,
					1,
					SPEED_TAG_LENGTH,
					SPEED_WHITE,
					opacity);

				// Below.
				Draw_AlphaFill(x + (int)(f * width),
					y + height - SPEED_TAG_LENGTH + 1,
					1,
					SPEED_TAG_LENGTH,
					SPEED_WHITE,
					opacity);
			}
		}
	}

	//
	// Draw outline.
	//
	if (style != SPEED_TEXT_ONLY)
	{
		if(vertical)
		{
			// Left.
			Draw_AlphaFill(x + SPEED_OUTLINE_SPACING,
				y,
				1,
				height,
				SPEED_WHITE,
				opacity);

			// Right.
			Draw_AlphaFill(x + width - SPEED_OUTLINE_SPACING,
				y,
				1,
				height,
				SPEED_WHITE,
				opacity);
		}
		else
		{
			// Above.
			Draw_AlphaFill(x,
				y + SPEED_OUTLINE_SPACING,
				width,
				1,
				SPEED_WHITE,
				opacity);

			// Below.
			Draw_AlphaFill(x,
				y + height - SPEED_OUTLINE_SPACING,
				width,
				1,
				SPEED_WHITE,
				opacity);
		}
	}

	//
	// Draw fill.
	//
	if (style != SPEED_TEXT_ONLY)
	{
		if(vertical)
		{
			// Draw the right color (slower).
			Draw_AlphaFill (x + SPEED_FILL_SPACING,
				y,
				width - (2 * SPEED_FILL_SPACING),
				height - color_offset,
				color1,
				opacity);

			// Draw the left color (faster).
			Draw_AlphaFill (x + SPEED_FILL_SPACING,
				y + height - color_offset,
				width - (2 * SPEED_FILL_SPACING),
				color_offset,
				color2,
				opacity);
		}
		else
		{
			// Draw the right color (slower).
			Draw_AlphaFill (x + color_offset,
				y + SPEED_FILL_SPACING,
				width - color_offset,
				height - (2 * SPEED_FILL_SPACING),
				color1,
				opacity);

			// Draw the left color (faster).
			Draw_AlphaFill (x,
				y + SPEED_FILL_SPACING,
				color_offset,
				height - (2 * SPEED_FILL_SPACING),
				color2,
				opacity);
		}
	}

	// Draw the speed text.
	if(vertical && vertical_text)
	{
		int i = 1;
		int len = 0;

		// Align the text accordingly.
		switch(text_align)
		{
			case SPEED_TEXT_ALIGN_NONE:		return;
			case SPEED_TEXT_ALIGN_FAR:		y = y + height - 4*8; break;
			case SPEED_TEXT_ALIGN_CENTER:	y = Q_rint(y + height/2.0 - 16); break;
			case SPEED_TEXT_ALIGN_CLOSE:
			default: break;
		}

		len = strlen(va("%d", player_speed));

		// 10^len
		while(len > 0)
		{
			i *= 10;
			len--;
		}

		// Write one number per row.
		for(; i > 1; i /= 10)
		{
			int next;
			next = (i/10);

			// Really make sure we don't try division by zero :)
			if(next <= 0)
			{
				break;
			}

			Draw_SString(Q_rint(x + width/2.0 - 4 * scale), y, va("%1d", (player_speed % i) / next), scale);
			y += 8;
		}
	}
	else
	{
		// Align the text accordingly.
		switch(text_align)
		{
			case SPEED_TEXT_ALIGN_FAR:
				x = x + width - 4 * 8 * scale;
				break;
			case SPEED_TEXT_ALIGN_CENTER:
				x = Q_rint(x + width / 2.0 - 2 * 8 * scale);
				break;
			case SPEED_TEXT_ALIGN_CLOSE:
			case SPEED_TEXT_ALIGN_NONE:
			default:
				break;
		}

		Draw_SString(x, Q_rint(y + height/2.0 - 4 * scale), va("%4d", player_speed), scale);
	}
}

static void SCR_HUD_DrawSpeed(hud_t *hud)
{
    int width, height;
    int x, y;

    static cvar_t *hud_speed_xyz = NULL,
		*hud_speed_width,
        *hud_speed_height,
		*hud_speed_tick_spacing,
		*hud_speed_opacity,
		*hud_speed_color_stopped,
		*hud_speed_color_normal,
		*hud_speed_color_fast,
		*hud_speed_color_fastest,
		*hud_speed_color_insane,
		*hud_speed_vertical,
		*hud_speed_vertical_text,
		*hud_speed_text_align,
		*hud_speed_style,
		*hud_speed_scale;

    if (hud_speed_xyz == NULL)    // first time
    {
        hud_speed_xyz			= HUD_FindVar(hud, "xyz");
		hud_speed_width			= HUD_FindVar(hud, "width");
		hud_speed_height		= HUD_FindVar(hud, "height");
		hud_speed_tick_spacing	= HUD_FindVar(hud, "tick_spacing");
		hud_speed_opacity		= HUD_FindVar(hud, "opacity");
		hud_speed_color_stopped	= HUD_FindVar(hud, "color_stopped");
		hud_speed_color_normal	= HUD_FindVar(hud, "color_normal");
		hud_speed_color_fast	= HUD_FindVar(hud, "color_fast");
		hud_speed_color_fastest	= HUD_FindVar(hud, "color_fastest");
		hud_speed_color_insane	= HUD_FindVar(hud, "color_insane");
		hud_speed_vertical		= HUD_FindVar(hud, "vertical");
		hud_speed_vertical_text	= HUD_FindVar(hud, "vertical_text");
		hud_speed_text_align	= HUD_FindVar(hud, "text_align");
		hud_speed_style			= HUD_FindVar(hud, "style");
		hud_speed_scale			= HUD_FindVar(hud, "scale");
    }

	width = max(0, hud_speed_width->value);
	height = max(0, hud_speed_height->value);

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		SCR_DrawHUDSpeed(x, y, width, height,
			hud_speed_xyz->value,
			hud_speed_tick_spacing->value,
			hud_speed_opacity->value,
			hud_speed_vertical->value,
			hud_speed_vertical_text->value,
			hud_speed_text_align->value,
			hud_speed_color_stopped->value,
			hud_speed_color_normal->value,
			hud_speed_color_fast->value,
			hud_speed_color_fastest->value,
			hud_speed_color_insane->value,
			hud_speed_style->ival,
			hud_speed_scale->value);
	}
}

#define	HUD_SPEED2_ORIENTATION_UP		0
#define	HUD_SPEED2_ORIENTATION_DOWN		1
#define	HUD_SPEED2_ORIENTATION_RIGHT	2
#define	HUD_SPEED2_ORIENTATION_LEFT		3

void SCR_HUD_DrawSpeed2(hud_t *hud)
{
	int width, height;
    int x, y;

    static cvar_t *hud_speed2_xyz = NULL,
//		*hud_speed2_opacity,
		*hud_speed2_color_stopped,
		*hud_speed2_color_normal,
		*hud_speed2_color_fast,
		*hud_speed2_color_fastest,
		*hud_speed2_color_insane,
		*hud_speed2_radius,
		*hud_speed2_wrapspeed,
		*hud_speed2_orientation;

    if (hud_speed2_xyz == NULL)    // first time
    {
        hud_speed2_xyz				= HUD_FindVar(hud, "xyz");
//		hud_speed2_opacity			= HUD_FindVar(hud, "opacity");
		hud_speed2_color_stopped	= HUD_FindVar(hud, "color_stopped");
		hud_speed2_color_normal		= HUD_FindVar(hud, "color_normal");
		hud_speed2_color_fast		= HUD_FindVar(hud, "color_fast");
		hud_speed2_color_fastest	= HUD_FindVar(hud, "color_fastest");
		hud_speed2_color_insane		= HUD_FindVar(hud, "color_insane");
		hud_speed2_radius			= HUD_FindVar(hud, "radius");
		hud_speed2_wrapspeed		= HUD_FindVar(hud, "wrapspeed");
		hud_speed2_orientation		= HUD_FindVar(hud, "orientation");
    }

	// Calculate the height and width based on the radius.
	switch((int)hud_speed2_orientation->value)
	{
		case HUD_SPEED2_ORIENTATION_LEFT :
		case HUD_SPEED2_ORIENTATION_RIGHT :
			height = max(0, 2*hud_speed2_radius->value);
			width = max(0, (hud_speed2_radius->value));
			break;
		case HUD_SPEED2_ORIENTATION_DOWN :
		case HUD_SPEED2_ORIENTATION_UP :
		default :
			// Include the height of the speed text in the height.
			height = max(0, (hud_speed2_radius->value));
			width = max(0, 2*hud_speed2_radius->value);
			break;
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		int player_speed;
		int arc_length;
		int color1, color2;
		int text_x = x;
		int text_y = y;
		vec_t *velocity;

		// Start and end points for the needle
		int needle_start_x = 0;
		int needle_start_y = 0;
		int needle_end_x = 0;
		int needle_end_y = 0;

		// The length of the arc between the zero point
		// and where the needle is pointing at.
		int needle_offset = 0;

		// The angle between the zero point and the position
		// that the needle is drawn on.
		float needle_angle = 0.0;

		// The angle where to start drawing the half circle and where to end.
		// This depends on the orientation of the circle (left, right, up, down).
		float circle_startangle = 0.0;
		float circle_endangle = 0.0;

		// Avoid divison by zero.
		if(hud_speed2_radius->value <= 0)
		{
			return;
		}

		// Get the velocity.
#ifdef HAXX
		if (cl.players[cl.playernum].spectator && Cam_TrackNum() >= 0)
		{
			velocity = cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK].playerstate[Cam_TrackNum()].velocity;
		}
		else
#endif
		{
			velocity = cl.simvel;
		}

		// Calculate the speed
		if (!hud_speed2_xyz->value)
		{
			// Based on XY.
			player_speed = sqrt(velocity[0]*velocity[0]
							  + velocity[1]*velocity[1]);
		}
		else
		{
			// Based on XYZ.
			player_speed = sqrt(velocity[0]*velocity[0]
							  + velocity[1]*velocity[1]
							  + velocity[2]*velocity[2]);
		}

		// Set the color based on the wrap speed.
		switch ((int)(player_speed / hud_speed2_wrapspeed->value))
		{
			case 0:
				color1 = hud_speed2_color_stopped->ival;
				color2 = hud_speed2_color_normal->ival;
				break;
			case 1:
				color1 = hud_speed2_color_normal->ival;
				color2 = hud_speed2_color_fast->ival;
				break;
			case 2:
				color1 = hud_speed2_color_fast->ival;
				color2 = hud_speed2_color_fastest->ival;
				break;
			default:
				color1 = hud_speed2_color_fastest->ival;
				color2 = hud_speed2_color_insane->ival;
				break;
		}

		// Set some properties how to draw the half circle, needle and text
		// based on the orientation of the hud item.
		switch((int)hud_speed2_orientation->value)
		{
			case HUD_SPEED2_ORIENTATION_LEFT :
			{
				x += width;
				y += height / 2;
				circle_startangle = M_PI / 2.0;
				circle_endangle	= (3*M_PI) / 2.0;

				text_x = x - 32;
				text_y = y - 4;
				break;
			}
			case HUD_SPEED2_ORIENTATION_RIGHT :
			{
				y += height / 2;
				circle_startangle = (3*M_PI) / 2.0;
				circle_endangle = (5*M_PI) / 2.0;
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);

				text_x = x;
				text_y = y - 4;
				break;
			}
			case HUD_SPEED2_ORIENTATION_DOWN :
			{
				x += width / 2;
				circle_startangle = M_PI;
				circle_endangle = 2*M_PI;
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);

				text_x = x - 16;
				text_y = y;
				break;
			}
			case HUD_SPEED2_ORIENTATION_UP :
			default :
			{
				x += width / 2;
				y += height;
				circle_startangle = 0;
				circle_endangle = M_PI;
				needle_end_y = y - hud_speed2_radius->value * sin (needle_angle);

				text_x = x - 16;
				text_y = y - 8;
				break;
			}
		}

		//
		// Calculate the offsets and angles.
		//
		{
			// Calculate the arc length of the half circle background.
			arc_length = fabs((circle_endangle - circle_startangle) * hud_speed2_radius->value);

			// Calculate the angle where the speed needle should point.
			needle_offset = arc_length * (player_speed % Q_rint(hud_speed2_wrapspeed->value)) / Q_rint(hud_speed2_wrapspeed->value);
			needle_angle = needle_offset / hud_speed2_radius->value;

			// Draw from the center of the half circle.
			needle_start_x = x;
			needle_start_y = y;
		}

		// Set the needle end point depending on the orientation of the hud item.

		switch((int)hud_speed2_orientation->value)
		{
			case HUD_SPEED2_ORIENTATION_LEFT :
			{
				needle_end_x = x - hud_speed2_radius->value * sin (needle_angle);
				needle_end_y = y + hud_speed2_radius->value * cos (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_RIGHT :
			{
				needle_end_x = x + hud_speed2_radius->value * sin (needle_angle);
				needle_end_y = y - hud_speed2_radius->value * cos (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_DOWN :
			{
				needle_end_x = x + hud_speed2_radius->value * cos (needle_angle);
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_UP :
			default :
			{
				needle_end_x = x - hud_speed2_radius->value * cos (needle_angle);
				needle_end_y = y - hud_speed2_radius->value * sin (needle_angle);
				break;
			}
		}

#ifdef HAXX
		// Draw the speed-o-meter background.
		Draw_AlphaPieSlice (x, y,				// Position
			hud_speed2_radius->value,			// Radius
			circle_startangle,					// Start angle
			circle_endangle - needle_angle,		// End angle
			1,									// Thickness
			true,								// Fill
			color1,								// Color
			hud_speed2_opacity->value);			// Opacity

		// Draw a pie slice that shows the "color" of the speed.
		Draw_AlphaPieSlice (x, y,				// Position
			hud_speed2_radius->value,			// Radius
			circle_endangle - needle_angle,		// Start angle
			circle_endangle,					// End angle
			1,									// Thickness
			true,								// Fill
			color2,								// Color
			hud_speed2_opacity->value);			// Opacity

		// Draw the "needle attachment" circle.
		Draw_AlphaCircle (x, y, 2.0, 1, true, 15, hud_speed2_opacity->value);

		// Draw the speed needle.
		Draw_AlphaLineRGB (needle_start_x, needle_start_y, needle_end_x, needle_end_y, 1, RGBA_TO_COLOR(250, 250, 250, 255 * hud_speed2_opacity->value));
#else
		(void)color1;
		(void)color2;
		(void)needle_start_x;
		(void)needle_start_y;
		(void)needle_end_x;
		(void)needle_end_y;
#endif

		// Draw the speed.
		Draw_String (text_x, text_y, va("%d", player_speed));
	}
}

// =======================================================
//
//  s t a t u s   b a r   e l e m e n t s
//
//


// -----------
// gunz
//
void SCR_HUD_DrawGunByNum (hud_t *hud, int num, float scale, int style, int wide)
{
    int i = num - 2;
    int width, height;
    int x, y;
    char *tmp;

    scale = max(scale, 0.01);

    switch (style)
    {
	case 3: // opposite colors of case 1
    case 1:     // text, gold inactive, white active
        width = 16 * scale;
        height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            switch (num)
            {
            case 2: tmp = "sg"; break;
            case 3: tmp = "bs"; break;
            case 4: tmp = "ng"; break;
            case 5: tmp = "sn"; break;
            case 6: tmp = "gl"; break;
            case 7: tmp = "rl"; break;
            case 8: tmp = "lg"; break;
            default: tmp = "";
            }

            if ( ((HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i)) && (style==1)) ||
				 ((HUD_Stats(STAT_ACTIVEWEAPON) != (IT_SHOTGUN<<i)) && (style==3))
			   )
                Draw_SString(x, y, tmp, scale);
            else
                Draw_SAlt_String(x, y, tmp, scale);
        }
        break;
	case 4: // opposite colors of case 2
    case 2:     // numbers, gold inactive, white active
        width = 8 * scale;
        height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            if ( HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i) )
				num += '0' + (style == 4 ? 128 : 0);
            else
				num += '0' + (style == 4 ? 0 : 128);
            Draw_SCharacter(x, y, num, scale);
        }
        break;
	case 5: // COLOR active, gold inactive
	case 7: // COLOR active, white inactive
	case 6: // white active, COLOR inactive
	case 8: // gold active, COLOR inactive
        width = 16 * scale;
        height = 8 * scale;
       
		if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;

        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) ) {
			if ( HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i) ) {
				if ((style==5) || (style==7)) { // strip {}
					char *weap_str = TP_ItemName((IT_SHOTGUN<<i));
					char weap_white_stripped[32];
					Util_SkipChars(weap_str, "{}", weap_white_stripped, 32);
					Draw_SString(x, y, weap_white_stripped, scale);
				}
				else { //Strip both &cRGB and {}
					char inactive_weapon_buf[16];
					char inactive_weapon_buf_nowhite[16];
					Util_SkipEZColors(inactive_weapon_buf, TP_ItemName(IT_SHOTGUN<<i), sizeof(inactive_weapon_buf));
					Util_SkipChars(inactive_weapon_buf, "{}", inactive_weapon_buf_nowhite, sizeof(inactive_weapon_buf_nowhite));

					if (style==8) // gold active
						Draw_SAlt_String(x, y, inactive_weapon_buf_nowhite, scale);
					else if (style==6) // white active
						Draw_SString(x, y, inactive_weapon_buf_nowhite, scale);
				}
			}
			else {
				if ((style==5) || (style==7)) { //Strip both &cRGB and {}
					char inactive_weapon_buf[16];
					char inactive_weapon_buf_nowhite[16];
					Util_SkipEZColors(inactive_weapon_buf, TP_ItemName(IT_SHOTGUN<<i), sizeof(inactive_weapon_buf));
					Util_SkipChars(inactive_weapon_buf, "{}", inactive_weapon_buf_nowhite, sizeof(inactive_weapon_buf_nowhite));
				
					if (style==5) // gold inactive
						Draw_SAlt_String(x, y, inactive_weapon_buf_nowhite, scale);
					else if (style==7) // white inactive
						Draw_SString(x, y, inactive_weapon_buf_nowhite, scale);
				}
				else if ((style==6) || (style==8)) { // strip only {}
					char *weap_str = TP_ItemName((IT_SHOTGUN<<i));
					char weap_white_stripped[32];
					Util_SkipChars(weap_str, "{}", weap_white_stripped, 32);
					Draw_SString(x, y, weap_white_stripped, scale);
				}

			}
        }
        break;
    default:    // classic - pictures
        width  = scale * (wide ? 48 : 24);
        height = scale * 16;

        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;

        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            float   time;
            int     flashon;

            time = cl.item_gettime[i];
            flashon = (int)((cl.time - time)*10);
            if (flashon < 0)
                flashon = 0;
            if (flashon >= 10)
            {
                if ( HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i) )
                    flashon = 1;
                else
                    flashon = 0;
            }
            else
                flashon = (flashon%5) + 2;

            if (wide  ||  num != 8)
                Draw_SPic (x, y, sb_weapons[flashon][i], scale);
            else
                Draw_SSubPic (x, y, sb_weapons[flashon][i], 0, 0, 24, 16, scale);
        }
        break;
    }
}

void SCR_HUD_DrawGun2 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time callse
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 2, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun3 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 3, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun4 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 4, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun5 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 5, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun6 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 6, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun7 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 7, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun8 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *wide;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
        wide  = HUD_FindVar(hud, "wide");
    }
    SCR_HUD_DrawGunByNum (hud, 8, scale->value, style->value, wide->value);
}
void SCR_HUD_DrawGunCurrent (hud_t *hud)
{
    int gun;
    static cvar_t *scale = NULL, *style, *wide;

    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
        wide  = HUD_FindVar(hud, "wide");
    }

	if (ShowPreselectedWeap()) {
	// using weapon pre-selection so show info for current best pre-selected weapon
		gun = IN_BestWeapon();
		if (gun < 2) {
			return;
		}
	} else {
	// not using weapon pre-selection or player is dead so show current selected weapon
		switch (HUD_Stats(STAT_ACTIVEWEAPON))
		{
			case IT_SHOTGUN << 0:   gun = 2; break;
			case IT_SHOTGUN << 1:   gun = 3; break;
			case IT_SHOTGUN << 2:   gun = 4; break;
			case IT_SHOTGUN << 3:   gun = 5; break;
			case IT_SHOTGUN << 4:   gun = 6; break;
			case IT_SHOTGUN << 5:   gun = 7; break;
			case IT_SHOTGUN << 6:   gun = 8; break;
			default: return;
		}
	}

    SCR_HUD_DrawGunByNum (hud, gun, scale->value, style->value, wide->value);
}

// ----------------
// powerzz
//
void SCR_HUD_DrawPowerup(hud_t *hud, int num, float scale, int style)
{
    int    x, y, width, height;
    int    c;

    scale = max(scale, 0.01);

    switch (style)
    {
    case 1:     // letter
        width = height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(17+num)))
        {
            switch (num)
            {
            case 0: c = '1'; break;
            case 1: c = '2'; break;
            case 2: c = 'r'; break;
            case 3: c = 'p'; break;
            case 4: c = 's'; break;
            case 5: c = 'q'; break;
            default: c = '?';
            }
            Draw_SCharacter(x, y, c, scale);
        }
        break;
    default:    // classic - pics
        width = height = scale * 16;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(17+num)))
            Draw_SPic (x, y, sb_items[num], scale);
        break;
    }
}

void SCR_HUD_DrawKey1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 0, scale->value, style->value);
}
void SCR_HUD_DrawKey2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawRing(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawPent(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 3, scale->value, style->value);
}
void SCR_HUD_DrawSuit(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 4, scale->value, style->value);
}
void SCR_HUD_DrawQuad(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 5, scale->value, style->value);
}

// -----------
// sigils
//
void SCR_HUD_DrawSigil(hud_t *hud, int num, float scale, int style)
{
    int     x, y;

    scale = max(scale, 0.01);

    switch (style)
    {
    case 1:     // sigil number
        if (!HUD_PrepareDraw(hud, 8*scale, 8*scale, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(28+num)))
            Draw_SCharacter(x, y, num + '0', scale);
        break;
    default:    // classic - picture
        if (!HUD_PrepareDraw(hud, 8*scale, 16*scale, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(28+num)))
            Draw_SPic(x, y, sb_sigil[num], scale);
        break;
    }
}

void SCR_HUD_DrawSigil1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 0, scale->value, style->value);
}
void SCR_HUD_DrawSigil2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawSigil3(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawSigil4(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 3, scale->value, style->value);
}

// icons - active ammo, armor, face etc..
void SCR_HUD_DrawAmmoIcon(hud_t *hud, int num, float scale, int style)
{
    int   x, y, width, height;

    scale = max(scale, 0.01);

    width = height = (style ? 8 : 24) * scale;

    if (!HUD_PrepareDraw(hud, width, height, &x, &y))
        return;

    if (style)
    {
        switch (num)
        {
        case 1: Draw_SAlt_String(x, y, "s", scale); break;
        case 2: Draw_SAlt_String(x, y, "n", scale); break;
        case 3: Draw_SAlt_String(x, y, "r", scale); break;
        case 4: Draw_SAlt_String(x, y, "c", scale); break;
        }
    }
    else
    {
        Draw_SPic (x, y, sb_ammo[num-1], scale);
    }
}
void SCR_HUD_DrawAmmoIconCurrent (hud_t *hud)
{
    int num;
    static cvar_t *scale = NULL, *style;

    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }

	if (ShowPreselectedWeap()) {
	// using weapon pre-selection so show info for current best pre-selected weapon ammo
		if (!(num = State_AmmoNumForWeapon(IN_BestWeapon())))
			return;
	} else {
	// not using weapon pre-selection or player is dead so show current selected ammo
		if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
			num = 1;
		else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
			num = 2;
		else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
			num = 3;
		else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
			num = 4;
		else if (TP_TeamFortressEngineerSpanner())
			num = 4;
		else
			return;
	}

    SCR_HUD_DrawAmmoIcon(hud, num, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon1 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon2 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon3 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 3, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon4 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 4, scale->value, style->value);
}

void SCR_HUD_DrawArmorIcon(hud_t *hud)
{
    int   x, y, width, height;

    int style;
    float scale;

    static cvar_t *v_scale = NULL, *v_style;
    if (v_scale == NULL)  // first time called
    {
        v_scale = HUD_FindVar(hud, "scale");
        v_style = HUD_FindVar(hud, "style");
    }

    scale = max(v_scale->value, 0.01);
    style = (int)(v_style->value);

    width = height = (style ? 8 : 24) * scale;

    if (!HUD_PrepareDraw(hud, width, height, &x, &y))
        return;

    if (style)
    {
        int c;

        if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
            c = '@';
        else  if (HUD_Stats(STAT_ITEMS) & IT_ARMOR3)
            c = 'r';
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR2)
            c = 'y';
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR1)
            c = 'g';
        else return;

        c += 128;

        Draw_SCharacter(x, y, c, scale);
    }
    else
    {
        mpic_t  *pic;

        if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
            pic = sb_disc;
        else  if (HUD_Stats(STAT_ITEMS) & IT_ARMOR3)
            pic = sb_armor[2];
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR2)
            pic = sb_armor[1];
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR1)
            pic = sb_armor[0];
        else return;

        Draw_SPic (x, y, pic, scale);
    }
}

// face
void SCR_HUD_DrawFace(hud_t *hud)
{
    int     f, anim;
    int     x, y;
    float   scale;

    static cvar_t *v_scale = NULL;
    if (v_scale == NULL)  // first time called
    {
        v_scale = HUD_FindVar(hud, "scale");
    }

    scale = max(v_scale->value, 0.01);

    if (!HUD_PrepareDraw(hud, 24*scale, 24*scale, &x, &y))
        return;

    if ( (HUD_Stats(STAT_ITEMS) & (IT_INVISIBILITY | IT_INVULNERABILITY) )
		== (IT_INVISIBILITY | IT_INVULNERABILITY) )
    {
        Draw_SPic (x, y, sb_face_invis_invuln, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_QUAD)
    {
        Draw_SPic (x, y, sb_face_quad, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_INVISIBILITY)
    {
        Draw_SPic (x, y, sb_face_invis, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
    {
        Draw_SPic (x, y, sb_face_invuln, scale);
        return;
    }

    if (HUD_Stats(STAT_HEALTH) >= 100)
        f = 4;
    else
        f = max(0, HUD_Stats(STAT_HEALTH)) / 20;

    if (cl.time <= cl.faceanimtime)
        anim = 1;
    else
        anim = 0;
    Draw_SPic (x, y, sb_faces[f][anim], scale);
}


// status numbers
void SCR_HUD_DrawNum(hud_t *hud, int num, qbool low,
                     float scale, int style, int digits, char *s_align)
{
    int  i;
    char buf[sizeof(int) * 3]; // each byte need <= 3 chars
    int  len;

    int width, height, x, y;
    int size;
    int align;

    clamp(num, -99999, 999999);

    scale = max(scale, 0.01);

    if (digits > 0)
		clamp(digits, 1, 6);
	else
		digits = 0; // auto-resize

    align = 2;
    switch (tolower(s_align[0]))
    {
    default:
    case 'l':   // 'l'eft
        align = 0; break;
    case 'c':   // 'c'enter
        align = 1; break;
    case 'r':   // 'r'ight
        align = 2; break;
    }

	snprintf(buf, sizeof (buf), "%d", (style == 2 || style == 3) ? num : abs(num));

	if(digits)
	{
		switch (hud_digits_trim->ival)
		{
			case 0: // 10030 -> 999
				len = strlen(buf);
				if (len > digits)
				{
					char *p = buf;
					if(num < 0)
						*p++ = '-';
					for (i = (num < 0) ? 1 : 0 ; i < digits; i++)
						*p++ = '9';
					*p = 0;
					len = digits;
				}
				break;
			default:
			case 1: // 10030 -> 030
				len = strlen(buf);
				if(len > digits)
				{
					char *p = buf;
					memmove(p, p + (len - digits), digits);
					buf[digits] = '\0';
					len = strlen(buf);
				}
				break;
			case 2: // 10030 -> 100
				buf[digits] = '\0';
    			len = strlen(buf);
				break;
		}
	}
	else
	{
		len = strlen(buf);
	}

    switch (style)
    {
		case 1:
		case 3:
			size = 8;
			break;
		case 0:
		case 2:
		default:
			size = 24;
			break;
    }

    if(digits)
		width = digits * size;
	else
		width = size * len;

    height = size;

    switch (style)
    {
		case 1:
		case 3:
			if (!HUD_PrepareDraw(hud, scale*width, scale*height, &x, &y))
				return;
			switch (align)
			{
				case 0: break;
				case 1: x += scale * (width - size * len) / 2; break;
				case 2: x += scale * (width - size * len); break;
			}
			if (low)
				Draw_SAlt_String(x, y, buf, scale);
			else
				Draw_SString(x, y, buf, scale);
			break;

		case 0:
		case 2:
		default:
			if (!HUD_PrepareDraw(hud, scale*width, scale*height, &x, &y))
				return;
			switch (align)
			{
				case 0: break;
				case 1: x += scale * (width - size * len) / 2; break;
				case 2: x += scale * (width - size * len); break;
			}
			for (i = 0; i < len; i++)
			{
				if(buf[i] == '-' && style == 2)
				{
					Draw_STransPic (x, y, sb_nums[low ? 1 : 0][STAT_MINUS], scale);
					x += 24 * scale;
				}
				else
				{
					Draw_STransPic (x, y, sb_nums[low ? 1 : 0][buf[i] - '0'], scale);
					x += 24 * scale;
				}
			}
			break;
    }
}

void SCR_HUD_DrawHealth(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
	static int value;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
	value = HUD_Stats(STAT_HEALTH);
    SCR_HUD_DrawNum(hud, (value < 0 ? 0 : value), HUD_HealthLow(),
        scale->value, style->value, digits->value, align->string);
}

void SCR_HUD_DrawArmor(hud_t *hud)
{
    int level;
    qbool low;
    static cvar_t *scale = NULL, *style, *digits, *align, *pent_666;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
		pent_666 = HUD_FindVar(hud, "pent_666"); // Show 666 or armor value when carrying pentagram
    }

    if (HUD_Stats(STAT_HEALTH) > 0)
    {
		if ((HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY) && pent_666->ival)
        {
            level = 666;
            low = true;
        }
        else
        {
            level = HUD_Stats(STAT_ARMOR);
            low = HUD_ArmorLow();
        }
    }
    else
    {
        level = 0;
        low = true;
    }

    SCR_HUD_DrawNum(hud, level, low,
        scale->value, style->value, digits->value, align->string);
}

//void Draw_AMFStatLoss (int stat, hud_t* hud);
static int vxdamagecount, vxdamagecount_time, vxdamagecount_oldhealth;
static int vxdamagecountarmour, vxdamagecountarmour_time, vxdamagecountarmour_oldhealth;
void Amf_Reset_DamageStats(void)
{
	vxdamagecount = vxdamagecount_time = vxdamagecount_oldhealth = 0;
	vxdamagecountarmour = vxdamagecountarmour_time = vxdamagecountarmour_oldhealth = 0;
}
void Draw_AMFStatLoss (int stat, hud_t* hud) {
	//fixme: should reset these on pov change
    int * vxdmgcnt, * vxdmgcnt_t, * vxdmgcnt_o;
    float alpha;
	int elem;

	if (stat == STAT_HEALTH) {
        vxdmgcnt = &vxdamagecount;
        vxdmgcnt_t = &vxdamagecount_time;
        vxdmgcnt_o = &vxdamagecount_oldhealth;
		elem = 0;
    } else {
        vxdmgcnt = &vxdamagecountarmour;
        vxdmgcnt_t = &vxdamagecountarmour_time;
        vxdmgcnt_o = &vxdamagecountarmour_oldhealth;
		elem = 1;
    }

    //VULT STAT LOSS
	//Pretty self explanitory, I just thought it would be a nice feature to go with my "what the hell is going on?" theme
	//and obscure even more of the screen
	if (cl.stats[stat] < (*vxdmgcnt_o - 1))
    {
      	if (*vxdmgcnt_t > cl.time) //add to damage
        		*vxdmgcnt = *vxdmgcnt + (*vxdmgcnt_o - cl.stats[stat]);
      	else
        		*vxdmgcnt = *vxdmgcnt_o - cl.stats[stat];
      	*vxdmgcnt_t = cl.time + 2 * (HUD_FindVar(hud, "duration")->value);
    }
    *vxdmgcnt_o = cl.stats[stat];

    if (*vxdmgcnt_t > cl.time)
      	alpha = min(1, (*vxdmgcnt_t - cl.time));
	else
		alpha = 0;

	drawfuncs->Colour4f(1,1,1,alpha);
	{
		static cvar_t *scale[2] = {NULL}, *style[2], *digits[2], *align[2];
		if (scale[elem] == NULL)  // first time called
		{
			scale[elem]  = HUD_FindVar(hud, "scale");
			style[elem]  = HUD_FindVar(hud, "style");
			digits[elem] = HUD_FindVar(hud, "digits");
			align[elem]  = HUD_FindVar(hud, "align");
		}
		SCR_HUD_DrawNum (hud, abs(*vxdmgcnt), 1,
            scale[elem]->value, style[elem]->value, digits[elem]->ival, align[elem]->string);
	}
	drawfuncs->Colour4f(1,1,1,1);
}

static void SCR_HUD_DrawHealthDamage(hud_t *hud)
{
	Draw_AMFStatLoss (STAT_HEALTH, hud);
	if (HUD_Stats(STAT_HEALTH) <= 0)
	{
		Amf_Reset_DamageStats();
	}
}

static void SCR_HUD_DrawArmorDamage(hud_t *hud)
{
	Draw_AMFStatLoss (STAT_ARMOR, hud);
}

void SCR_HUD_DrawAmmo(hud_t *hud, int num,
                      float scale, int style, int digits, char *s_align)
{
    int value, num_old;
    qbool low;

	num_old = num;
    if (num < 1 || num > 4)
    {	// draw 'current' ammo, which one is it?

		if (ShowPreselectedWeap()) {
		// using weapon pre-selection so show info for current best pre-selected weapon ammo
			if (!(num = State_AmmoNumForWeapon(IN_BestWeapon())))
				return;
		} else {
		// not using weapon pre-selection or player is dead so show current selected ammo
			if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
				num = 1;
			else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
				num = 2;
			else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
				num = 3;
			else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
				num = 4;
			else if (TP_TeamFortressEngineerSpanner())
				num = 4;
			else
				return;
		}
    }

    low = HUD_AmmoLowByWeapon(num * 2);
	if (num_old == 0 && (!ShowPreselectedWeap() || cl.standby)) {
		// this check is here to display a feature from KTPRO/KTX where you can see received damage in prewar
		// also we make sure this applies only to 'ammo' element
		// weapon preselection must always use HUD_Stats()
		value = cl.stats[STAT_AMMO];
	} else {
		value = HUD_Stats(STAT_SHELLS + num - 1);
	}

    if (style < 2)
    {
        // simply draw number
        SCR_HUD_DrawNum(hud, value, low, scale, style, digits, s_align);
    }
    else
    {
        // else - draw classic ammo-count box with background
        char buf[8];
        int  x, y;

        scale = max(scale, 0.01);

        if (!HUD_PrepareDraw(hud, 42*scale, 11*scale, &x, &y))
            return;

        snprintf (buf, sizeof (buf), "%3i", value);
        Draw_SSubPic(x, y, sb_ibar, 3+((num-1)*48), 0, 42, 11, scale);
        if (buf[0] != ' ')  Draw_SCharacter (x +  7*scale, y, 18+buf[0]-'0', scale);
        if (buf[1] != ' ')  Draw_SCharacter (x + 15*scale, y, 18+buf[1]-'0', scale);
        if (buf[2] != ' ')  Draw_SCharacter (x + 23*scale, y, 18+buf[2]-'0', scale);
    }
}

void SCR_HUD_DrawAmmoCurrent(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 0, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 1, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 2, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo3(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 3, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo4(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 4, scale->value, style->value, digits->value, align->string);
}

// Problem icon, Net

static void SCR_HUD_NetProblem (hud_t *hud) {
	static cvar_t *scale = NULL;
	int x, y;
	extern qbool hud_editor;
	plugnetinfo_t *netinfo = GetNetworkInfo();

	float picwidth = 64;
	float picheight = 64;
	drawfuncs->ImageSize((intptr_t)sb_net, &picwidth, &picheight);

	if(scale == NULL)
		scale = HUD_FindVar(hud, "scale");

	if (netinfo->loss.dropped < 1)
	{
		if (hud_editor)
			HUD_PrepareDraw(hud, picwidth, picheight, &x, &y);
		return;
	}

	if (!HUD_PrepareDraw(hud, picwidth, picheight, &x, &y))
		return;

	Draw_SPic (x, y, sb_net, scale->value);
}

// ============================================================================0
// Groups
// ============================================================================0

mpic_t *hud_pic_group1;
mpic_t *hud_pic_group2;
mpic_t *hud_pic_group3;
mpic_t *hud_pic_group4;
mpic_t *hud_pic_group5;
mpic_t *hud_pic_group6;
mpic_t *hud_pic_group7;
mpic_t *hud_pic_group8;
mpic_t *hud_pic_group9;

void SCR_HUD_DrawGroup(hud_t *hud, int width, int height, mpic_t *pic, int pic_scalemode, float pic_alpha)
{
	#define HUD_GROUP_SCALEMODE_TILE		1
	#define HUD_GROUP_SCALEMODE_STRETCH		2
	#define HUD_GROUP_SCALEMODE_GROW		3
	#define HUD_GROUP_SCALEMODE_CENTER		4

	int x, y;

	float picwidth = 64;
	float picheight = 64;

	if (pic && drawfuncs->ImageSize((intptr_t)pic, &picwidth, &picheight) <= 0)
	{
		pic = NULL;
		picwidth = 64;
		picheight = 64;
	}

	clamp(width, 1, 99999);
    clamp(height, 1, 99999);

	// Set it to this, because 1.0 will make the colors
	// completly saturated, and no semi-transparency will show.
	pic_alpha = (pic_alpha) >= 1.0 ? 0.99 : pic_alpha;

	// Grow the group if necessary.
	if (pic_scalemode == HUD_GROUP_SCALEMODE_GROW
		&& pic != NULL && picheight > 0 && picwidth > 0)
	{
		width = max(picwidth, width);
		height = max(picheight, height);
	}

	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
	{
        return;
	}

    // Draw the picture if it's set.
	if (pic != NULL && picheight > 0)
    {
        int pw, ph;

		if (pic_scalemode == HUD_GROUP_SCALEMODE_TILE)
        {
            // Tile.
            int cx = 0, cy = 0;
            while (cy < height)
            {
                while (cx < width)
                {
                    pw = min(picwidth, width - cx);
                    ph = min(picheight, height - cy);

                    if (pw >= picwidth  &&  ph >= picheight)
					{
                        Draw_AlphaPic (x + cx , y + cy, pic, pic_alpha);
					}
                    else
					{
                        Draw_AlphaSubPic (x + cx, y + cy, pic, 0, 0, pw, ph, pic_alpha);
					}

                    cx += picwidth;
                }

                cx = 0;
                cy += picheight;
            }
        }
		else if (pic_scalemode == HUD_GROUP_SCALEMODE_STRETCH)
		{
			// Stretch or shrink the picture to fit.
			float scale_x = (float)width / picwidth;
			float scale_y = (float)height / picheight;

			Draw_SAlphaSubPic2 (x, y, pic, 0, 0, picwidth, picheight, scale_x, scale_y, pic_alpha);
		}
		else if (pic_scalemode == HUD_GROUP_SCALEMODE_CENTER)
		{
			// Center the picture in the group.
			int pic_x = x + (width - picwidth) / 2;
			int pic_y = y + (height - picheight) / 2;

			int src_x = 0;
			int src_y = 0;

			if(x > pic_x)
			{
				src_x = x - pic_x;
				pic_x = x;
			}

			if(y > pic_y)
			{
				src_y = y - pic_y;
				pic_y = y;
			}

			Draw_AlphaSubPic (pic_x, pic_y,	pic, src_x, src_y, min(width, picwidth), min(height, picheight), pic_alpha);
		}
		else
        {
			// Normal. Draw in the top left corner.
			Draw_AlphaSubPic (x, y, pic, 0, 0, min(width, picwidth), min(height, picheight), pic_alpha);
        }
    }
}

void SCR_HUD_LoadGroupPic(cvar_t *var, mpic_t **hud_pic, char *oldval)
{
	char *newpic = var->string;
	#define HUD_GROUP_PIC_BASEPATH	"gfx/%s"

	mpic_t *temp_pic = NULL;
	char pic_path[MAX_QPATH];

	if (!hud_pic)
	{
		Com_Printf ("Couldn't load picture %s for hud group. HUD PIC is null\n", newpic);
		return;
	}

	// If we have no pic name.
	if(!newpic || !strcmp (newpic, ""))
	{
		*hud_pic = NULL;
		return;
	}

	// Get the path for the pic.
	snprintf (pic_path, sizeof(pic_path), HUD_GROUP_PIC_BASEPATH, newpic);

	// Try loading the pic.
	if (!(temp_pic = Draw_CachePicSafe(pic_path, false, true)))
	{
		Com_Printf("Couldn't load picture %s for hud group.\n", newpic);
		cvarfuncs->SetString(var->name, "");
		return;
	}

	// Save the pic.
	if (hud_pic)
		*hud_pic = temp_pic;

	return;
}

void SCR_HUD_OnChangePic_Group1(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group1, oldval);
}

void SCR_HUD_OnChangePic_Group2(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group2, oldval);
}

void SCR_HUD_OnChangePic_Group3(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group3, oldval);
}

void SCR_HUD_OnChangePic_Group4(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group4, oldval);
}

void SCR_HUD_OnChangePic_Group5(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group5, oldval);
}

void SCR_HUD_OnChangePic_Group6(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group6, oldval);
}

void SCR_HUD_OnChangePic_Group7(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group7, oldval);
}

void SCR_HUD_OnChangePic_Group8(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group8, oldval);
}

void SCR_HUD_OnChangePic_Group9(cvar_t *var, char *oldval)
{
	SCR_HUD_LoadGroupPic(var, &hud_pic_group9, oldval);
}

void SCR_HUD_Group1(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width				= HUD_FindVar(hud, "width");
        height				= HUD_FindVar(hud, "height");
        picture				= HUD_FindVar(hud, "picture");
		pic_alpha			= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode		= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group1;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group1, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group1,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group2(hud_t *hud)
{
	extern void DrawNewText(int x, int y, char *text);
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group2;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group2, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group2,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group3(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group3;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group3, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group3,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group4(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group4;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group4, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group4,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group5(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group5;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group5, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group5,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group6(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group6;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group6, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group6,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group7(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group7;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group7, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group7,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group8(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group8;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group8, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group8,
		pic_scalemode->value,
		pic_alpha->value);
}

void SCR_HUD_Group9(hud_t *hud)
{
    static cvar_t *width = NULL,
		*height,
		*picture,
		*pic_alpha,
		*pic_scalemode;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->callback	= SCR_HUD_OnChangePic_Group9;
		SCR_HUD_LoadGroupPic(picture, &hud_pic_group9, picture->string);
    }

	SCR_HUD_DrawGroup(hud,
		width->value,
		height->value,
		hud_pic_group9,
		pic_scalemode->value,
		pic_alpha->value);
}

// player sorting
// for frags and players
typedef struct sort_teams_info_s
{
	char *name;
	int  frags;
	int  min_ping;
	int  avg_ping;
	int  max_ping;
	int  nplayers;
	int  top, bottom;   // leader colours
	int  rlcount;		// Number of RL's present in the team. (Cokeman 2006-05-27)
}
sort_teams_info_t;

typedef struct sort_players_info_s
{
    int playernum;
    sort_teams_info_t *team;
}
sort_players_info_t;

static sort_players_info_t		sorted_players[MAX_CLIENTS];
static sort_teams_info_t		sorted_teams[MAX_CLIENTS];
static int						n_teams;
static int						n_players;
static int						n_spectators;
static int						sort_teamsort = 0;

static int HUD_ComparePlayers(const void *vp1, const void *vp2)
{
	const sort_players_info_t *p1 = vp1;
	const sort_players_info_t *p2 = vp2;

    int r = 0;
    player_info_t *i1 = &cl.players[p1->playernum];
    player_info_t *i2 = &cl.players[p2->playernum];

    if (i1->spectator && !i2->spectator)
	{
        r = -1;
	}
    else if (!i1->spectator && i2->spectator)
	{
        r = 1;
	}
    else if (i1->spectator && i2->spectator)
    {
        r = strcmp(i1->name, i2->name);
    }
    else
    {
		//
		// Both are players.
		//
		if(sort_teamsort && cl.teamplay && p1->team && p2->team)
		{
			// Leading team on top, sort players inside of the teams.

			// Teamsort 1, first sort on team frags.
			if (sort_teamsort == 1)
			{
				r = p1->team->frags - p2->team->frags;
			}

			// Teamsort == 2, sort on team name only.
			r = (r == 0) ? -strcmp(p1->team->name, p2->team->name) : r;
		}

		r = (r == 0) ? i1->frags - i2->frags : r;
		r = (r == 0) ? strcmp(i1->name, i2->name) : r;
    }

	r = (r == 0) ? (p1->playernum - p2->playernum) : r;

	// qsort() sorts ascending by default, we want descending.
	// So negate the result.
	return -r;
}

static int HUD_CompareTeams(const void *vt1, const void *vt2)
{
	int r = 0;
	const sort_teams_info_t *t1 = vt1;
	const sort_teams_info_t *t2 = vt2;

	r = (t1->frags - t2->frags);
	r = !r ? strcmp(t1->name, t2->name) : r;

	// qsort() sorts ascending by default, we want descending.
	// So negate the result.
	return -r;
}

#define HUD_SCOREBOARD_ALL			0xffffffff
#define HUD_SCOREBOARD_SORT_TEAMS	(1 << 0)
#define HUD_SCOREBOARD_SORT_PLAYERS	(1 << 1)
#define HUD_SCOREBOARD_UPDATE		(1 << 2)
#define HUD_SCOREBOARD_AVG_PING		(1 << 3)

static void HUD_Sort_Scoreboard(int flags)
{
    int i;
    int team;

    n_teams = 0;
    n_players = 0;
    n_spectators = 0;

    // Set team properties.
	if(flags & HUD_SCOREBOARD_UPDATE)
	{
		memset(sorted_teams, 0, sizeof(sorted_teams));

		for (i=0; i < MAX_CLIENTS; i++)
		{
			if (cl.players[i].name[0] && !cl.players[i].spectator)
			{
				// Find players team
				for (team = 0; team < n_teams; team++)
				{
					if (!strcmp(cl.players[i].team, sorted_teams[team].name)
						&& sorted_teams[team].name[0])
					{
						break;
					}
				}

				// The team wasn't found in the list of existing teams
				// so add a new team.
				if (team == n_teams)
				{
					team = n_teams++;
					sorted_teams[team].avg_ping = 0;
					sorted_teams[team].max_ping = 0;
					sorted_teams[team].min_ping = 999;
					sorted_teams[team].nplayers = 0;
					sorted_teams[team].frags = 0;
					sorted_teams[team].top = Sbar_TopColor(&cl.players[i]);
					sorted_teams[team].bottom = Sbar_BottomColor(&cl.players[i]);
					sorted_teams[team].name = cl.players[i].team;
					sorted_teams[team].rlcount = 0;
				}

				sorted_teams[team].nplayers++;
				sorted_teams[team].frags += cl.players[i].frags;
				sorted_teams[team].avg_ping += cl.players[i].ping;
				sorted_teams[team].min_ping = min(sorted_teams[team].min_ping, cl.players[i].ping);
				sorted_teams[team].max_ping = max(sorted_teams[team].max_ping, cl.players[i].ping);

#ifdef HAXX
				// The total RL count for the players team.
				if(cl.players[i].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)
				{
					sorted_teams[team].rlcount++;
				}
#endif

				// Set player data.
				sorted_players[n_players + n_spectators].playernum = i;
				//sorted_players[n_players + n_spectators].team = &sorted_teams[team];

				// Increase the count.
				if (cl.players[i].spectator)
				{
					n_spectators++;
				}
				else
				{
					n_players++;
				}
			}
		}
	}

    // Calc avg ping.
	if(flags & HUD_SCOREBOARD_AVG_PING)
	{
		for (team = 0; team < n_teams; team++)
		{
			sorted_teams[team].avg_ping /= sorted_teams[team].nplayers;
		}
	}

	// Sort teams.
	if(flags & HUD_SCOREBOARD_SORT_TEAMS)
	{
		qsort(sorted_teams, n_teams, sizeof(sort_teams_info_t), HUD_CompareTeams);

		// BUGFIX, this needs to happen AFTER the team array has been sorted, otherwise the
		// players might be pointing to the incorrect team adress.
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			player_info_t *player = &cl.players[sorted_players[i].playernum];
			sorted_players[i].team = NULL;

			// Find players team.
			for (team = 0; team < n_teams; team++)
			{
				if (!strcmp(player->team, sorted_teams[team].name)
					&& sorted_teams[team].name[0])
				{
					sorted_players[i].team = &sorted_teams[team];
					break;
				}
			}
		}
	}

	// Sort players.
	if(flags & HUD_SCOREBOARD_SORT_PLAYERS)
	{
		qsort(sorted_players, n_players + n_spectators, sizeof(sort_players_info_t), HUD_ComparePlayers);
	}
}

void Frags_DrawColors(int x, int y, int width, int height,
					  int top_color, int bottom_color, float color_alpha,
					  int frags, int drawBrackets, int style,
					  float bignum)
{
	char buf[32];
	int posy = 0;
	int char_size = (bignum > 0) ? Q_rint(24 * bignum) : 8;

	Draw_AlphaFill(x, y, width, height / 2, top_color, color_alpha);
	Draw_AlphaFill(x, y + height / 2, width, height - height / 2, bottom_color, color_alpha);

	posy = y + (height - char_size) / 2;

	if (bignum > 0)
	{
		//
		// Scaled big numbers for frags.
		//
		char *t = buf;
		int char_x;
		int char_y;
		snprintf(buf, sizeof (buf), "%d", frags);

		char_x = max(x, x + (width  - (int)strlen(buf) * char_size) / 2);
		char_y = max(y, posy);

		while (*t)
		{
			if (*t >= '0' && *t <= '9')
			{
				Draw_STransPic(char_x, char_y, sb_nums[0][*t - '0'], bignum);
				char_x += char_size;
			}
			else if (*t == '-')
			{
				Draw_STransPic(char_x, char_y, sb_nums[0][STAT_MINUS], bignum);
				char_x += char_size;
			}

			t++;
		}
	}
	else
	{
		// Normal text size.
		snprintf(buf, sizeof (buf), "%3d", frags);
		Draw_String(x - 2 + (width - char_size * strlen(buf) - 2) / 2, posy, buf);
	}

	if(drawBrackets)
    {
		// Brackets [] are not available scaled, so use normal size even
		// if we're drawing big frag nums.
		int brack_posy = y + (height - 8) / 2;
        int d = (width >= 32) ? 0 : 1;

		switch(style)
		{
			case 1 :
				Draw_Character(x - 8, posy, 13);
				break;
			case 2 :
				// Red outline.
				Draw_Fill(x, y - 1, width, 1, 0x4f);
				Draw_Fill(x, y - 1, 1, height + 2, 0x4f);
				Draw_Fill(x + width - 1, y - 1, 1, height + 2, 0x4f);
				Draw_Fill(x, y + height, width, 1, 0x4f);
				break;
			case 0 :
			default :
				Draw_Character(x - 2 - 2 * d, brack_posy, 16); // [
				Draw_Character(x + width - 8 + 1 + d, brack_posy, 17); // ]
				break;
		}
    }
}

#define	FRAGS_HEALTHBAR_WIDTH			5

#define FRAGS_HEALTHBAR_NORMAL_COLOR	75
#define FRAGS_HEALTHBAR_MEGA_COLOR		251
#define	FRAGS_HEALTHBAR_TWO_MEGA_COLOR	238
#define	FRAGS_HEALTHBAR_UNNATURAL_COLOR	144

void Frags_DrawHealthBar(int original_health, int x, int y, int height, int width)
{
	float health_height = 0.0;
	int health;

	// Get the health.
	health = original_health;
	health = min(100, health);

	// Draw a health bar.
	health_height = Q_rint((height / 100.0) * health);
	health_height = (health_height > 0.0 && health_height < 1.0) ? 1 : health_height;
	health_height = (health_height < 0.0) ? 0.0 : health_height;
	Draw_Fill(x, y + height - (int)health_height, 3, (int)health_height, FRAGS_HEALTHBAR_NORMAL_COLOR);

	// Get the health again to check if health is more than 100.
	health = original_health;
	if(health > 100 && health <= 200)
	{
		health_height = (int)Q_rint((height / 100.0) * (health - 100));
		Draw_Fill(x, y + height - health_height, width, health_height, FRAGS_HEALTHBAR_MEGA_COLOR);
	}
	else if(health > 200 && health <= 250)
	{
		health_height = (int)Q_rint((height / 100.0) * (health - 200));
		Draw_Fill(x, y, width, height, FRAGS_HEALTHBAR_MEGA_COLOR);
		Draw_Fill(x, y + height - health_height, width, health_height, FRAGS_HEALTHBAR_TWO_MEGA_COLOR);
	}
	else if(health > 250)
	{
		// This will never happen during a normal game.
		Draw_Fill(x, y, width, health_height, FRAGS_HEALTHBAR_UNNATURAL_COLOR);
	}
}

#define	TEAMFRAGS_EXTRA_SPEC_NONE	0
#define TEAMFRAGS_EXTRA_SPEC_BEFORE	1
#define	TEAMFRAGS_EXTRA_SPEC_ONTOP	2
#define TEAMFRAGS_EXTRA_SPEC_NOICON 3
#define TEAMFRAGS_EXTRA_SPEC_RLTEXT 4

int TeamFrags_DrawExtraSpecInfo(int num, int px, int py, int width, int height, int style)
{
	float rl_width, rl_height;
	mpic_t *pic = sb_weapons[0][5];
	drawfuncs->ImageSize((intptr_t)pic, &rl_width, &rl_height);

	// Only allow this for spectators.
	if (!(cls.demoplayback || cl.spectator)
		|| style > TEAMFRAGS_EXTRA_SPEC_RLTEXT
		|| style <= TEAMFRAGS_EXTRA_SPEC_NONE
		|| !style)
	{
		return px;
	}

	// Check if the team has any RL's.
	if(sorted_teams[num].rlcount > 0)
	{
		int y_pos = py;

		//
		// Draw the RL + count depending on style.
		//

		if((style == TEAMFRAGS_EXTRA_SPEC_BEFORE || style == TEAMFRAGS_EXTRA_SPEC_NOICON)
			&& style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = Q_rint(py + (height / 2.0) - 4);
			Draw_ColoredString(px, y_pos, va("%d", sorted_teams[num].rlcount), 0);
			px += 8 + 1;
		}

		if(style != TEAMFRAGS_EXTRA_SPEC_NOICON && style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = Q_rint(py + (height / 2.0) - (rl_height / 2.0));
			Draw_SSubPic (px, y_pos, pic, 0, 0, rl_width, rl_height, 1);
			px += rl_width + 1;
		}

		if(style == TEAMFRAGS_EXTRA_SPEC_ONTOP && style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = Q_rint(py + (height / 2.0) - 4);
			Draw_ColoredString(px - 14, y_pos, va("%d", sorted_teams[num].rlcount), 0);
		}

		if(style == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = Q_rint(py + (height / 2.0) - 4);
			Draw_ColoredString(px, y_pos, va("&ce00RL&cfff%d", sorted_teams[num].rlcount), 0);
			px += 8*3 + 1;
		}
	}
	else
	{
		// If the team has no RL's just pad with nothing.
		if(style == TEAMFRAGS_EXTRA_SPEC_BEFORE)
		{
			// Draw the rl count before the rl icon.
			px += rl_width + 8 + 1 + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_ONTOP)
		{
			// Draw the rl count on top of the RL instead of infront.
			px += rl_width + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_NOICON)
		{
			// Only draw the rl count.
			px += 8 + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			px += 8*3 + 1;
		}
	}

	return px;
}

static qbool hud_frags_extra_spec_info	= true;
static qbool hud_frags_show_rl			= true;
static qbool hud_frags_show_armor		= true;
static qbool hud_frags_show_health		= true;
static qbool hud_frags_show_powerup		= true;
static qbool hud_frags_textonly			= false;

static void QDECL Frags_OnChangeExtraSpecInfo(cvar_t *var, char *oldvalue)
{
	// Parse the extra spec info.
	hud_frags_show_rl		= Utils_RegExpMatch("RL|ALL",		var->string);
	hud_frags_show_armor	= Utils_RegExpMatch("ARMOR|ALL",	var->string);
	hud_frags_show_health	= Utils_RegExpMatch("HEALTH|ALL",	var->string);
	hud_frags_show_powerup	= Utils_RegExpMatch("POWERUP|ALL",	var->string);
	hud_frags_textonly		= Utils_RegExpMatch("TEXT",			var->string);

	hud_frags_extra_spec_info = (hud_frags_show_rl || hud_frags_show_armor || hud_frags_show_health || hud_frags_show_powerup);
}

static int Frags_DrawExtraSpecInfo(player_info_t *info,
							 int px, int py,
							 int cell_width, int cell_height,
							 int space_x, int space_y, int flip)
{
#ifdef HAXX
	mpic_t *rl_picture = sb_weapons[0][5];	// Picture of RL.
	float rl_width, rl_height;

	float	armor_height = 0.0;
	int		armor = 0;
	int		armor_bg_color = 0;
	float	armor_bg_power = 0;
	int		health_spacing = 1;
	int		weapon_width = 24;

	drawfuncs->ImageSize((intptr_t)rl_picture, &rl_width, &rl_height);

	// Only allow this for spectators.
	if (!(cls.demoplayback || cl.spectator))
	{
		return px;
	}

	// Set width based on text or picture.
	weapon_width = hud_frags_textonly ? rl_width : 24;

	// Draw health bar. (flipped)
	if(flip && hud_frags_show_health)
	{
		Frags_DrawHealthBar(info->stats[STAT_HEALTH], px, py, cell_height, 3);
		px += 3 + health_spacing;
	}

	armor = info->stats[STAT_ARMOR];

	// If the player has any armor, draw it in the appropriate color.
	if(info->stats[STAT_ITEMS] & IT_ARMOR1)
	{
		armor_bg_power = 100;
		armor_bg_color = 178; // Green armor.
	}
	else if(info->stats[STAT_ITEMS] & IT_ARMOR2)
	{
		armor_bg_power = 150;
		armor_bg_color = 111; // Yellow armor.
	}
	else if(info->stats[STAT_ITEMS] & IT_ARMOR3)
	{
		armor_bg_power = 200;
		armor_bg_color = 79; // Red armor.
	}

	// Only draw the armor if the current player has one and if the style allows it.
	if(armor_bg_power && hud_frags_show_armor)
	{
		armor_height = Q_rint((cell_height / armor_bg_power) * armor);

		Draw_AlphaFill(px,												// x
						py + cell_height - (int)armor_height,			// y (draw from bottom up)
						weapon_width,									// width
						(int)armor_height,								// height
						armor_bg_color,									// color
						0.3);											// alpha
	}

	// Draw the rl if the current player has it and the style allows it.
	if(info->stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER && hud_frags_show_rl)
	{
		if(!hud_frags_textonly)
		{
			// Draw the rl-pic.
			Draw_SSubPic (px,
				py + Q_rint((cell_height/2.0)) - (rl_height/2.0),
				rl_picture, 0, 0,
				rl_width,
				rl_height, 1);
		}
		else
		{
			// Just print "RL" instead.
			Draw_String(px + 12 - 8, py + Q_rint((cell_height/2.0)) - 4, "RL");
		}
	}

	// Only draw powerups is the current player has it and the style allows it.
	if(hud_frags_show_powerup)
	{

		//float powerups_x = px + (spec_extra_weapon_w / 2.0);
		float powerups_x = px + (weapon_width / 2.0);

		if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY
			&& info->stats[STAT_ITEMS] & IT_QUAD)
		{
			Draw_ColoredString(Q_rint(powerups_x - 10), py, "&c0ffQ&cf00P&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD
			&& info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
		{
			Draw_ColoredString(Q_rint(powerups_x - 8), py, "&c0ffQ&cf00P", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(Q_rint(powerups_x - 8), py, "&c0ffQ&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(Q_rint(powerups_x - 8), py, "&cf00P&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD)
		{
			Draw_ColoredString(Q_rint(powerups_x - 4), py, "&c0ffQ", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
		{
			Draw_ColoredString(Q_rint(powerups_x - 4), py, "&cf00P", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(Q_rint(powerups_x - 4), py, "&cff0R", 0);
		}
	}

	px += weapon_width + health_spacing;

	// Draw health bar. (not flipped)
	if(!flip && hud_frags_show_health)
	{
		Frags_DrawHealthBar(info->stats[STAT_HEALTH], px, py, cell_height, 3);
		px += 3 + health_spacing;
	}
#endif
	return px;
}

void Frags_DrawBackground(int px, int py, int cell_width, int cell_height,
						  int space_x, int space_y, int max_name_length, int max_team_length,
						  int bg_color, int shownames, int showteams, int drawBrackets, int style)
{
	int bg_width = cell_width + space_x;
	//int bg_color = Sbar_BottomColor(info);
	float bg_alpha = 0.3;

	if(style == 4
		|| style == 6
		|| style == 8)
		bg_alpha = 0;

	if(shownames)
		bg_width += max_name_length*8 + space_x;

	if(showteams)
		bg_width += max_team_length * 8 + space_x;

	if(drawBrackets)
		bg_alpha = 0.7;

	if(style == 7 || style == 8)
		bg_color = 0x4f;

	Draw_AlphaFill(px - 1, py - space_y / 2, bg_width, cell_height + space_y, bg_color, bg_alpha);

	if(drawBrackets && (style == 5 || style == 6))
	{
		Draw_Fill(px - 1, py - 1 - space_y / 2, bg_width, 1, 0x4f);

		Draw_Fill(px - 1, py - space_y / 2, 1, cell_height + space_y, 0x4f);
		Draw_Fill(px + bg_width - 1, py - 1 - space_y / 2, 1, cell_height + 1 + space_y, 0x4f);

		Draw_Fill(px - 1, py + cell_height + space_y / 2, bg_width + 1, 1, 0x4f);
	}
}

int Frags_DrawText(int px, int py,
					int cell_width, int cell_height,
					int space_x, int space_y,
					int max_name_length, int max_team_length,
					int flip, int pad,
					int shownames, int showteams,
					char* name, char* team)
{
	char _name[MAX_SCOREBOARDNAME + 1];
	char _team[MAX_SCOREBOARDNAME + 1];
	int team_length = 0;
	int name_length = 0;
	int char_size = 8;
	int y_pos;

	y_pos = Q_rint(py + (cell_height / 2.0) - 4);

	// Draw team
	if(showteams && cl.teamplay)
	{
		strlcpy(_team, team, clamp(max_team_length, 0, sizeof(_team)));
		team_length = strlen(_team);

		if(!flip)
			px += space_x;

		if(pad && flip)
		{
			px += (max_team_length - team_length) * char_size;
			Draw_String(px, y_pos, _team);
			px += team_length * char_size;
		}
		else if(pad)
		{
			Draw_String(px, y_pos, _team);
			px += max_team_length * char_size;
		}
		else
		{
			Draw_String(px, y_pos, _team);
			px += team_length * char_size;
		}

		if(flip)
			px += space_x;
	}

	if(shownames)
	{
		// Draw name
		strlcpy(_name, name, clamp(max_name_length, 0, sizeof(_name)));
		name_length = strlen(_name);

		if(flip && pad)
		{
			px += (max_name_length - name_length) * char_size;
			Draw_String(px, y_pos, _name);
			px += name_length * char_size;
		}
		else if(pad)
		{
			Draw_String(px, y_pos, _name);
			px += max_name_length * char_size;
		}
		else
		{
			Draw_String(px, y_pos, _name);
			px += name_length * char_size;
		}

		px += space_x;
	}

	return px;
}

void SCR_HUD_DrawFrags(hud_t *hud)
{
    int width = 0, height = 0;
    int x, y;
	int max_team_length = 0;
	int max_name_length = 0;

    int rows, cols, cell_width, cell_height, space_x, space_y;
    int a_rows, a_cols; // actual

    static cvar_t
        *hud_frags_cell_width = NULL,
        *hud_frags_cell_height,
        *hud_frags_rows,
        *hud_frags_cols,
        *hud_frags_space_x,
        *hud_frags_space_y,
        *hud_frags_vertical,
        *hud_frags_strip,
        *hud_frags_teamsort,
		*hud_frags_shownames,
		*hud_frags_teams,
		*hud_frags_padtext,
		*hud_frags_showself,
		*hud_frags_extra_spec,
		*hud_frags_fliptext,
		*hud_frags_style,
		*hud_frags_bignum,
		*hud_frags_colors_alpha,
		*hud_frags_maxname,
		*hud_frags_notintp;

	mpic_t *rl_picture = sb_weapons[0][5];
	float rl_width, rl_height;
	drawfuncs->ImageSize((intptr_t)rl_picture, &rl_width, &rl_height);

    if (hud_frags_cell_width == NULL)    // first time
    {
		char specval[256];

        hud_frags_cell_width    = HUD_FindVar(hud, "cell_width");
        hud_frags_cell_height   = HUD_FindVar(hud, "cell_height");
        hud_frags_rows          = HUD_FindVar(hud, "rows");
        hud_frags_cols          = HUD_FindVar(hud, "cols");
        hud_frags_space_x       = HUD_FindVar(hud, "space_x");
        hud_frags_space_y       = HUD_FindVar(hud, "space_y");
        hud_frags_teamsort      = HUD_FindVar(hud, "teamsort");
        hud_frags_strip         = HUD_FindVar(hud, "strip");
        hud_frags_vertical      = HUD_FindVar(hud, "vertical");
		hud_frags_shownames		= HUD_FindVar(hud, "shownames");
		hud_frags_teams			= HUD_FindVar(hud, "showteams");
		hud_frags_padtext		= HUD_FindVar(hud, "padtext");
		hud_frags_showself		= HUD_FindVar(hud, "showself_always");
		hud_frags_extra_spec	= HUD_FindVar(hud, "extra_spec_info");
		hud_frags_fliptext		= HUD_FindVar(hud, "fliptext");
		hud_frags_style			= HUD_FindVar(hud, "style");
		hud_frags_bignum		= HUD_FindVar(hud, "bignum");
		hud_frags_colors_alpha	= HUD_FindVar(hud, "colors_alpha");
		hud_frags_maxname		= HUD_FindVar(hud, "maxname");
		hud_frags_notintp		= HUD_FindVar(hud, "notintp");

		// Set the OnChange function for extra spec info.
		hud_frags_extra_spec->callback = Frags_OnChangeExtraSpecInfo;
		strlcpy(specval, hud_frags_extra_spec->string, sizeof(specval));
		Cvar_Set(hud_frags_extra_spec, specval);
    }

	// Don't draw the frags if we're in teamplay.
	if(hud_frags_notintp->value && cl.teamplay)
	{
		HUD_PrepareDraw(hud, width, height, &x, &y);
		return;
	}

	//
	// Clamp values to be "sane".
	//
	{
		rows = hud_frags_rows->value;
		clamp(rows, 1, MAX_CLIENTS);

		cols = hud_frags_cols->value;
		clamp(cols, 1, MAX_CLIENTS);

		// Some users doesn't want to show the actual frags, just
		// extra_spec_info stuff + names.
		cell_width = hud_frags_cell_width->value;
		clamp(cell_width, 0, 128);

		cell_height = hud_frags_cell_height->value;
		clamp(cell_height, 7, 32);

		space_x = hud_frags_space_x->value;
		clamp(space_x, 0, 128);

		space_y = hud_frags_space_y->value;
		clamp(space_y, 0, 128);
	}

	sort_teamsort = hud_frags_teamsort->ival;

    if (hud_frags_strip->ival)
    {
		// Auto set the number of rows / cols based on the number of players.
		// (This is kinda fucked up, but I won't mess with it for the sake of backwards compability).

        if (hud_frags_vertical->value)
        {
            a_cols = min((n_players + rows - 1) / rows, cols);
            a_rows = min(rows, n_players);
        }
        else
        {
            a_rows = min((n_players + cols - 1) / cols, rows);
            a_cols = min(cols, n_players);
        }
    }
    else
    {
        a_rows = rows;
        a_cols = cols;
    }

    width  = (a_cols * cell_width)  + ((a_cols + 1) * space_x);
    height = (a_rows * cell_height) + ((a_rows + 1) * space_y);

	// Get the longest name/team name for padding.
	if(hud_frags_shownames->value || hud_frags_teams->value)
	{
		int cur_length = 0;
		int n;

		for(n = 0; n < n_players; n++)
		{
			player_info_t *info = &cl.players[sorted_players[n].playernum];
			cur_length = strlen(info->name);

			// Name.
			if(cur_length >= max_name_length)
			{
				max_name_length = cur_length + 1;
			}

			cur_length = strlen(info->team);

			// Team name.
			if(cur_length >= max_team_length)
			{
				max_team_length = cur_length + 1;
			}
		}

		// If the user has set a limit on how many chars that
		// are allowed to be shown for a name/teamname.
		max_name_length = min(max(0, (int)hud_frags_maxname->value), max_name_length) + 1;
		max_team_length = min(max(0, (int)hud_frags_maxname->value), max_team_length) + 1;

		// We need a wider box to draw in if we show the names.
		if(hud_frags_shownames->value)
		{
			width += (a_cols * (max_name_length + 3) * 8) + ((a_cols + 1) * space_x);
		}

		if(cl.teamplay && hud_frags_teams->value)
		{
			width += (a_cols * max_team_length * 8) + ((a_cols + 1) * space_x);
		}
	}

	// Make room for the extra spectator stuff.
	if(hud_frags_extra_spec_info && (cls.demoplayback || cl.spectator) )
	{
		width += a_cols * (rl_width + FRAGS_HEALTHBAR_WIDTH);
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        int i = 0;
        int player_x = 0;
        int player_y = 0;
        int num = 0;
		int drawBrackets = 0;

		// The number of players that are to be visible.
		int limit = min(n_players, a_rows * a_cols);

		// Always show my current frags (don't just show the leaders).
		// TODO: When all players aren't being shown in the frags, draw
		// a small arrow that indicates that there are more frags to be seen.
		if(hud_frags_showself->value && !cl_multiview->value)
		{
			int player_pos = 0;

			// Find my position in the scoreboard.
			for(player_pos = 0; i < n_players; player_pos++)
			{
				if (cls.demoplayback || cl.spectator)
				{
					if (spec_track == sorted_players[player_pos].playernum)
					{
						break;
					}
				}
				else if(sorted_players[player_pos].playernum == cl.playernum)
				{
					break;
				}
			}

			if(player_pos + 1 <= (a_rows * a_cols))
			{
				// If I'm not "outside" the shown frags, start drawing from the top.
				num = 0;
			}
			else
			{
				// Always include me in the shown frags.
				num = abs((a_rows * a_cols) - (player_pos + 1));
			}

			// Make sure we're not trying to go outside the player array.
			num = (num < 0 || num > n_players) ? 0 : num;
		}

		//num = 0;  // FIXME! johnnycz; (see fixme below)

		//
		// Loop through all the positions that should be drawn (columns * rows or number of players).
		//
		// Start drawing player "num", usually the first player in the array, but if
		// showself_always is set this might be someone else (since we need to make sure the current
		// player is always shown).
		//
        for (i = 0; i < limit; i++)
        {
            player_info_t *info = &cl.players[sorted_players[num].playernum]; // FIXME! johnnycz; causes crashed on some demos

			//
			// Set the coordinates where to draw the next element.
			//
            if (hud_frags_vertical->value)
            {
                if (i % a_rows == 0)
                {
					// We're drawing a new column.

					int element_width = cell_width + space_x;

					// Get the width of all the stuff that is shown, the name, frag cell and so on.

					if(hud_frags_shownames->value)
					{
						element_width += (max_name_length) * 8;
					}

					if(hud_frags_teams->value)
					{
						element_width += (max_team_length) * 8;
					}

					if(hud_frags_extra_spec_info && (cls.demoplayback || cl.spectator) )
					{
						element_width += rl_width;
					}

					player_x = x + space_x + ((i / a_rows) * element_width);

					// New column.
                    player_y = y + space_y;
                }
            }
            else
            {
                if (i % a_cols == 0)
                {
					// Drawing new row.
                    player_x = x + space_x;
                    player_y = y + space_y + (i / a_cols) * (cell_height + space_y);
                }
            }

			drawBrackets = 0;

			// Bug fix. Before the wrong player would be higlighted
			// during qwd-playback, since you ARE the player that you're
			// being spectated (you're not a spectator).
			if(cls.demoplayback && !cl.spectator && !cls.mvdplayback)
			{
				drawBrackets = (sorted_players[num].playernum == cl.playernum);
			}
			else if (cls.demoplayback || cl.spectator)
			{
				drawBrackets = (spec_track == sorted_players[num].playernum && Cam_TrackNum() >= 0);
			}
			else
			{
				drawBrackets = (sorted_players[num].playernum == cl.playernum);
			}

			// Don't draw any brackets in multiview since we're
			// tracking several players.
			if (cl_multiview->value > 1 && cls.mvdplayback)
			{
				// TODO: Highlight all players being tracked (See tracking hud-element)
				drawBrackets = 0;
			}

			if(hud_frags_shownames->value || hud_frags_teams->value || hud_frags_extra_spec_info)
			{
				// Relative x coordinate where we draw the subitems.
				int rel_player_x = player_x;

				if(hud_frags_style->value >= 4 && hud_frags_style->value <= 8)
				{
					// Draw background based on the style.

					Frags_DrawBackground(player_x, player_y, cell_width, cell_height, space_x, space_y,
						max_name_length, max_team_length, Sbar_BottomColor(info),
						hud_frags_shownames->value, hud_frags_teams->value, drawBrackets,
						hud_frags_style->value);
				}

				if(hud_frags_fliptext->value)
				{
					//
					// Flip the text
					// NAME | TEAM | FRAGS | EXTRA_SPEC_INFO
					//

					// Draw name.
					rel_player_x = Frags_DrawText(rel_player_x, player_y, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						hud_frags_shownames->value, 0,
						info->name, info->team);

					// Draw team.
					rel_player_x = Frags_DrawText(rel_player_x, player_y, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						0, hud_frags_teams->value,
						info->name, info->team);

					Frags_DrawColors(rel_player_x, player_y, cell_width, cell_height,
						Sbar_TopColor(info), Sbar_BottomColor(info), hud_frags_colors_alpha->value,
						info->frags,
						drawBrackets,
						hud_frags_style->value,
						hud_frags_bignum->value);

					rel_player_x += cell_width + space_x;

					// Show extra information about all the players if spectating:
					// - What armor they have.
					// - How much health.
					// - If they have RL or not.
					rel_player_x = Frags_DrawExtraSpecInfo(info, rel_player_x, player_y, cell_width, cell_height,
							 space_x, space_y,
							 hud_frags_fliptext->value);

				}
				else
				{
					//
					// Don't flip the text
					// EXTRA_SPEC_INFO | FRAGS | TEAM | NAME
					//

					rel_player_x = Frags_DrawExtraSpecInfo(info, rel_player_x, player_y, cell_width, cell_height,
							 space_x, space_y,
							 hud_frags_fliptext->value);

					Frags_DrawColors(rel_player_x, player_y, cell_width, cell_height,
						Sbar_TopColor(info), Sbar_BottomColor(info), hud_frags_colors_alpha->value,
						info->frags,
						drawBrackets,
						hud_frags_style->value,
						hud_frags_bignum->value);

					rel_player_x += cell_width + space_x;

					// Draw team.
					rel_player_x = Frags_DrawText(rel_player_x, player_y, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						0, hud_frags_teams->value,
						info->name, info->team);

					// Draw name.
					rel_player_x = Frags_DrawText(rel_player_x, player_y, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						hud_frags_shownames->value, 0,
						info->name, info->team);
				}

				if(hud_frags_vertical->value)
				{
					// Next row.
					player_y += cell_height + space_y;
				}
				else
				{
					// Next column.
					player_x = rel_player_x + space_x;
				}
			}
			else
			{
				// Only showing the frags, no names or extra spec info.

				Frags_DrawColors(player_x, player_y, cell_width, cell_height,
					Sbar_TopColor(info), Sbar_BottomColor(info), hud_frags_colors_alpha->value,
					info->frags,
					drawBrackets,
					hud_frags_style->value,
					hud_frags_bignum->value);

				if (hud_frags_vertical->value)
				{
					// Next row.
					player_y += cell_height + space_y;
				}
				else
				{
					// Next column.
					player_x += cell_width + space_x;
				}
			}

			// Next player.
            num++;
        }
    }
}

void SCR_HUD_DrawTeamFrags(hud_t *hud)
{
    int width = 0, height = 0;
    int x, y;
	int max_team_length = 0, num = 0;
    int rows, cols, cell_width, cell_height, space_x, space_y;
    int a_rows, a_cols; // actual

    static cvar_t
        *hud_teamfrags_cell_width,
        *hud_teamfrags_cell_height,
        *hud_teamfrags_rows,
        *hud_teamfrags_cols,
        *hud_teamfrags_space_x,
        *hud_teamfrags_space_y,
        *hud_teamfrags_vertical,
        *hud_teamfrags_strip,
		*hud_teamfrags_shownames,
		*hud_teamfrags_fliptext,
		*hud_teamfrags_padtext,
		*hud_teamfrags_style,
		*hud_teamfrags_extra_spec,
		*hud_teamfrags_onlytp,
		*hud_teamfrags_bignum,
		*hud_teamfrags_colors_alpha;

	mpic_t *rl_picture = sb_weapons[0][5];
	float rl_width, rl_height;
	drawfuncs->ImageSize((intptr_t)rl_picture, &rl_width, &rl_height);

    if (hud_teamfrags_cell_width == 0)    // first time
    {
        hud_teamfrags_cell_width    = HUD_FindVar(hud, "cell_width");
        hud_teamfrags_cell_height   = HUD_FindVar(hud, "cell_height");
        hud_teamfrags_rows          = HUD_FindVar(hud, "rows");
        hud_teamfrags_cols          = HUD_FindVar(hud, "cols");
        hud_teamfrags_space_x       = HUD_FindVar(hud, "space_x");
        hud_teamfrags_space_y       = HUD_FindVar(hud, "space_y");
        hud_teamfrags_strip         = HUD_FindVar(hud, "strip");
        hud_teamfrags_vertical      = HUD_FindVar(hud, "vertical");
		hud_teamfrags_shownames		= HUD_FindVar(hud, "shownames");
		hud_teamfrags_fliptext		= HUD_FindVar(hud, "fliptext");
		hud_teamfrags_padtext		= HUD_FindVar(hud, "padtext");
		hud_teamfrags_style			= HUD_FindVar(hud, "style");
		hud_teamfrags_extra_spec	= HUD_FindVar(hud, "extra_spec_info");
		hud_teamfrags_onlytp		= HUD_FindVar(hud, "onlytp");
		hud_teamfrags_bignum		= HUD_FindVar(hud, "bignum");
		hud_teamfrags_colors_alpha	= HUD_FindVar(hud, "colors_alpha");
    }

	// Don't draw the frags if we're not in teamplay.
	if(hud_teamfrags_onlytp->value && !cl.teamplay)
	{
		HUD_PrepareDraw(hud, width, height, &x, &y);
		return;
	}

    rows = hud_teamfrags_rows->value;
    clamp(rows, 1, MAX_CLIENTS);
    cols = hud_teamfrags_cols->value;
    clamp(cols, 1, MAX_CLIENTS);
    cell_width = hud_teamfrags_cell_width->value;
    clamp(cell_width, 28, 128);
    cell_height = hud_teamfrags_cell_height->value;
    clamp(cell_height, 7, 32);
    space_x = hud_teamfrags_space_x->value;
    clamp(space_x, 0, 128);
    space_y = hud_teamfrags_space_y->value;
    clamp(space_y, 0, 128);

	if (hud_teamfrags_strip->value)
    {
        if (hud_teamfrags_vertical->value)
        {
            a_cols = min((n_teams+rows-1) / rows, cols);
            a_rows = min(rows, n_teams);
        }
        else
        {
            a_rows = min((n_teams+cols-1) / cols, rows);
            a_cols = min(cols, n_teams);
        }
    }
    else
    {
        a_rows = rows;
        a_cols = cols;
    }

    width  = a_cols*cell_width  + (a_cols+1)*space_x;
    height = a_rows*cell_height + (a_rows+1)*space_y;

	// Get the longest team name for padding.
	if(hud_teamfrags_shownames->value || hud_teamfrags_extra_spec->value)
	{
		int rlcount_width = 0;

		int cur_length = 0;
		int n;

		for(n=0; n < n_teams; n++)
		{
			if(hud_teamfrags_shownames->value)
			{
				cur_length = strlen(sorted_teams[n].name);

				// Team name
				if(cur_length >= max_team_length)
				{
					max_team_length = cur_length + 1;
				}
			}
		}

		// Calculate the length of the extra spec info.
		if(hud_teamfrags_extra_spec->value && (cls.demoplayback || cl.spectator))
		{
			if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_BEFORE)
			{
				// Draw the rl count before the rl icon.
				rlcount_width = rl_width + 8 + 1 + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_ONTOP)
			{
				// Draw the rl count on top of the RL instead of infront.
				rlcount_width = rl_width + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_NOICON)
			{
				// Only draw the rl count.
				rlcount_width = 8 + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
			{
				rlcount_width = 8*3 + 1;
			}
		}

		width += a_cols*max_team_length*8 + (a_cols+1)*space_x + a_cols*rlcount_width;
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        int i;
        int px = 0;
        int py = 0;
		int drawBrackets;
		int limit = min(n_teams, a_rows*a_cols);

        for (i=0; i < limit; i++)
        {
            if (hud_teamfrags_vertical->value)
            {
                if (i % a_rows == 0)
                {
                    px = x + space_x + (i/a_rows) * (cell_width+space_x);
                    py = y + space_y;
                }
            }
            else
            {
                if (i % a_cols == 0)
                {
                    px = x + space_x;
                    py = y + space_y + (i/a_cols) * (cell_height+space_y);
                }
            }

			drawBrackets = 0;

			// Bug fix. Before the wrong player would be higlighted
			// during qwd-playback, since you ARE the player that you're
			// being spectated.
			if(cls.demoplayback && !cl.spectator && !cls.mvdplayback)
			{
				// QWD Playback.
				if (!strcmp(sorted_teams[num].name, cl.players[cl.playernum].team))
				{
					drawBrackets = 1;
				}
			}
			else if (cls.demoplayback || cl.spectator)
			{
				// MVD playback / spectating.
				if (!strcmp(cl.players[spec_track].team, sorted_teams[num].name) && Cam_TrackNum() >= 0)
				{
					drawBrackets = 1;
				}
			}
			else
			{
				// Normal player.
				if (!strcmp(sorted_teams[num].name, cl.players[cl.playernum].team))
				{
					drawBrackets = 1;
				}
			}

			if (cl_multiview->value && cl.splitscreenview != 0 )  // Only draw bracket for first view, might make todo below unnecessary
			{
				// TODO: Check if "track team" is set, if it is then draw brackets around that team.
				//cl.players[nPlayernum]

				drawBrackets = 0;
			}

			if(hud_teamfrags_shownames->value || hud_teamfrags_extra_spec->value)
			{
				int _px = px;

				// Draw a background if the style tells us to.
				if(hud_teamfrags_style->value >= 4 && hud_teamfrags_style->value <= 8)
				{
					Frags_DrawBackground(px, py, cell_width, cell_height, space_x, space_y,
						0, max_team_length, sorted_teams[num].bottom,
						0, hud_teamfrags_shownames->value, drawBrackets,
						hud_teamfrags_style->value);
				}

				// Draw the text on the left or right side of the score?
				if(hud_teamfrags_fliptext->value)
				{
					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, 0, max_team_length,
						hud_teamfrags_fliptext->value, hud_teamfrags_padtext->value,
						0, hud_teamfrags_shownames->value,
						"", sorted_teams[num].name);

					Frags_DrawColors(_px, py, cell_width, cell_height,
						sorted_teams[num].top,
						sorted_teams[num].bottom,
						hud_teamfrags_colors_alpha->value,
						sorted_teams[num].frags,
						drawBrackets,
						hud_teamfrags_style->value,
						hud_teamfrags_bignum->value);

					_px += cell_width + space_x;

					// Draw the rl if the current player has it and the style allows it.
					_px = TeamFrags_DrawExtraSpecInfo(num, _px, py, cell_width, cell_height, hud_teamfrags_extra_spec->value);

				}
				else
				{
					// Draw the rl if the current player has it and the style allows it.
					_px = TeamFrags_DrawExtraSpecInfo(num, _px, py, cell_width, cell_height, hud_teamfrags_extra_spec->value);

					Frags_DrawColors(_px, py, cell_width, cell_height,
						sorted_teams[num].top,
						sorted_teams[num].bottom,
						hud_teamfrags_colors_alpha->value,
						sorted_teams[num].frags,
						drawBrackets,
						hud_teamfrags_style->value,
						hud_teamfrags_bignum->value);

					_px += cell_width + space_x;

					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, 0, max_team_length,
						hud_teamfrags_fliptext->value, hud_teamfrags_padtext->value,
						0, hud_teamfrags_shownames->value,
						"", sorted_teams[num].name);
				}

				if(hud_teamfrags_vertical->value)
				{
					py += cell_height + space_y;
				}
				else
				{
					px = _px + space_x;
				}
			}
			else
			{
				Frags_DrawColors(px, py, cell_width, cell_height,
					sorted_teams[num].top,
					sorted_teams[num].bottom,
					hud_teamfrags_colors_alpha->value,
					sorted_teams[num].frags,
					drawBrackets,
					hud_teamfrags_style->value,
					hud_teamfrags_bignum->value);

				if (hud_teamfrags_vertical->value)
				{
					py += cell_height + space_y;
				}
				else
				{
					px += cell_width + space_x;
				}
			}
            num ++;
        }
    }
}

char *Get_MP3_HUD_style(float style, char *st)
{
	static char HUD_style[32];
	if(style == 1.0)
	{
		strlcpy(HUD_style, va("%s:", st), sizeof(HUD_style));
	}
	else if(style == 2.0)
	{
		strlcpy(HUD_style, va("^Ue010%s^Ue011", st), sizeof(HUD_style));
	}
	else
	{
		strlcpy(HUD_style, "", sizeof(HUD_style));
	}
	return HUD_style;
}

// Draws MP3 Title.
void SCR_HUD_DrawMP3_Title(hud_t *hud)
{
	int x=0, y=0/*, n=1*/;
    int width = 64;
	int height = 8;

#ifdef WITH_MP3_PLAYER
	//int width_as_text = 0;
	static int title_length = 0;
	//int row_break = 0;
	//int i=0;
	int status = 0;
	static char title[MP3_MAXSONGTITLE];
	double t;		// current time
	static double lastframetime;	// last refresh

	static cvar_t *style = NULL, *width_var, *height_var, *scroll, *scroll_delay, *on_scoreboard, *wordwrap;

	if (style == NULL)  // first time called
    {
        style =				HUD_FindVar(hud, "style");
		width_var =			HUD_FindVar(hud, "width");
		height_var =		HUD_FindVar(hud, "height");
		scroll =			HUD_FindVar(hud, "scroll");
		scroll_delay =		HUD_FindVar(hud, "scroll_delay");
		on_scoreboard =		HUD_FindVar(hud, "on_scoreboard");
		wordwrap =			HUD_FindVar(hud, "wordwrap");
    }

	if(on_scoreboard->value)
	{
		hud->flags |= HUD_ON_SCORES;
	}
	else if((int)on_scoreboard->value & HUD_ON_SCORES)
	{
		hud->flags -= HUD_ON_SCORES;
	}

	width = (int)width_var->value;
	height = (int)height_var->value;

	if(width < 0) width = 0;
	if(width > vid.width) width = vid.width;
	if(height < 0) height = 0;
	if(height > vid.width) height = vid.height;

	t = Sys_DoubleTime();

	if ((t - lastframetime) >= 2) { // 2 sec refresh rate
		lastframetime = t;
		status = MP3_GetStatus();

		switch(status)
		{
			case MP3_PLAYING :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Playing"), MP3_Macro_MP3Info());
				break;
			case MP3_PAUSED :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Paused"), MP3_Macro_MP3Info());
				break;
			case MP3_STOPPED :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Stopped"), MP3_Macro_MP3Info());
				break;
			case MP3_NOTRUNNING	:
			default :
				status = MP3_NOTRUNNING;
				title_length = snprintf (title, sizeof (title), "%s is not running.", mp3_player->PlayerName_AllCaps);
				break;
		}

		if(title_length < 0)
		{
			snprintf(title, sizeof (title), "Error retrieving current song.");
		}
	}

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		SCR_DrawWordWrapString(x, y, 8, width, height, (int)wordwrap->value, (int)scroll->value, (float)scroll_delay->value, title);
	}
#else
	HUD_PrepareDraw(hud, width , height, &x, &y);
#endif
}

// Draws MP3 Time as a HUD-element.
void SCR_HUD_DrawMP3_Time(hud_t *hud)
{
	int x = 0, y = 0, width = 0, height = 0;
#ifdef WITH_MP3_PLAYER
	int elapsed = 0;
	int remain = 0;
	int total = 0;
	static char time_string[MP3_MAXSONGTITLE];
	static char elapsed_string[MP3_MAXSONGTITLE];
	double t; // current time
	static double lastframetime; // last refresh

	static cvar_t *style = NULL, *on_scoreboard;

	if(style == NULL)
	{
		style			= HUD_FindVar(hud, "style");
		on_scoreboard	= HUD_FindVar(hud, "on_scoreboard");
	}

	if(on_scoreboard->value)
	{
		hud->flags |= HUD_ON_SCORES;
	}
	else if((int)on_scoreboard->value & HUD_ON_SCORES)
	{
		hud->flags -= HUD_ON_SCORES;
	}

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 2) { // 2 sec refresh rate
		lastframetime = t;

		if(!MP3_GetOutputtime(&elapsed, &total) || elapsed < 0 || total < 0)
		{
			snprintf (time_string, sizeof (time_string), "^Ue010-:-^Ue011");
		}
		else
		{
			switch((int)style->value)
			{
				case 1 :
					remain = total - elapsed;
					strlcpy (elapsed_string, SecondsToMinutesString (remain), sizeof (elapsed_string));
					snprintf (time_string, sizeof (time_string), "^Ue010-%s/%s^Ue011", elapsed_string, SecondsToMinutesString (total));
					break;
				case 2 :
					remain = total - elapsed;
					snprintf (time_string, sizeof (time_string), "^Ue010-%s^Ue011", SecondsToMinutesString (remain));
					break;
				case 3 :
					snprintf (time_string, sizeof (time_string), "^Ue010%s^Ue011", SecondsToMinutesString (elapsed));
					break;
				case 4 :
					remain = total - elapsed;
					strlcpy (elapsed_string, SecondsToMinutesString (remain), sizeof (elapsed_string));
					snprintf (time_string, sizeof (time_string), "%s/%s", elapsed_string, SecondsToMinutesString (total));
					break;
				case 5 :
					strlcpy (elapsed_string, SecondsToMinutesString (elapsed), sizeof (elapsed_string));
					snprintf (time_string, sizeof (time_string), "-%s/%s", elapsed_string, SecondsToMinutesString (total));
					break;
				case 6 :
					remain = total - elapsed;
					snprintf (time_string, sizeof (time_string), "-%s", SecondsToMinutesString (remain));
					break;
				case 7 :
					snprintf (time_string, sizeof (time_string), "%s", SecondsToMinutesString (elapsed));
					break;
				case 0 :
				default :
					strlcpy (elapsed_string, SecondsToMinutesString (elapsed), sizeof (elapsed_string));
					snprintf (time_string, sizeof (time_string), "^Ue010%s/%s^Ue011", elapsed_string, SecondsToMinutesString (total));
					break;
			}
		}

	}

	// Don't allow showing the timer if ruleset disallows it
	// It could be used for timing powerups
	// Use same check that is used for any external communication
	if(Rulesets_RestrictPacket())
		snprintf (time_string, sizeof (time_string), "^Ue010%s^Ue011", "Not allowed");

	width = strlen (time_string) * 8;
	height = 8;

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
		Draw_String(x, y, time_string);
#else
	HUD_PrepareDraw(hud, width , height, &x, &y);
#endif
}

#ifdef WITH_PNG

// Map picture to draw for the mapoverview hud control.
mpic_t *radar_pic;
static qbool radar_pic_found = false;

// The conversion formula used for converting from quake coordinates to pixel coordinates
// when drawing on the map overview.
static float map_x_slope;
static float map_x_intercept;
static float map_y_slope;
static float map_y_intercept;
static qbool conversion_formula_found = false;

// Used for drawing the height of the player.
static float map_height_diff = 0.0;

#define RADAR_BASE_PATH_FORMAT	"radars/%s.png"

//
// Is run when a new map is loaded.
//
void HUD_NewRadarMap()
{
	int i = 0;
	int len = 0;
	int n_textcount = 0;
	mpic_t *radar_pic_p = NULL;
	png_textp txt = NULL;
	char *radar_filename = NULL;

	if (!cl.worldmodel)
		return; // seems we are not ready to do that

	// Reset the radar pic status.
	radar_pic = NULL;
	radar_pic_found = false;
	conversion_formula_found = false;

	// Allocate a string for the path to the radar image.
	len = strlen (RADAR_BASE_PATH_FORMAT) +  strlen (host_mapname.string);
	radar_filename = Q_calloc (len, sizeof(char));
	snprintf (radar_filename, len, RADAR_BASE_PATH_FORMAT, host_mapname.string);

	// Load the map picture.
	if ((radar_pic_p = GL_LoadPicImage (radar_filename, host_mapname.string, 0, 0, TEX_ALPHA)) != NULL)
	{
		radar_pic = *radar_pic_p;
		radar_pic_found = true;

		// Calculate the height of the map.
		map_height_diff = abs(cl.worldmodel->maxs[2] - cl.worldmodel->mins[2]);

		// Get the comments from the PNG.
		txt = Image_LoadPNG_Comments(radar_filename, &n_textcount);

		// Check if we found any comments.
		if(txt != NULL)
		{
			int found_count = 0;

			// Find the conversion formula in the comments found in the PNG.
			for(i = 0; i < n_textcount; i++)
			{
				if(!strcmp(txt[i].key, "QWLMConversionSlopeX"))
				{
					map_x_slope = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionInterceptX"))
				{
					map_x_intercept = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionSlopeY"))
				{
					map_y_slope = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionInterceptY"))
				{
					map_y_intercept = atof(txt[i].text);
					found_count++;
				}

				conversion_formula_found = (found_count == 4);
			}

			// Free the text chunks.
			Q_free(txt);
		}
		else
		{
			conversion_formula_found = false;
		}
	}
	else
	{
		// No radar pic found.
		memset (&radar_pic, 0, sizeof(radar_pic));
		radar_pic_found = false;
		conversion_formula_found = false;
	}

	// Free the path string to the radar png.
	Q_free (radar_filename);
}
#endif // WITH_PNG

#define TEMPHUD_NAME "_temphud"
#define TEMPHUD_FULLPATH "configs/"TEMPHUD_NAME".cfg"

// will check if user wants to un/load external MVD HUD automatically
void HUD_AutoLoad_MVD(int autoload) {
#ifdef HAXX
	char *cfg_suffix = "custom";
	extern cvar_t *scr_fov;
	extern cvar_t *scr_newHud;
	extern void Cmd_Exec_f (void);
	extern void DumpConfig(char *name);

	if (autoload && cls.mvdplayback) {
		// Turn autohud ON here

		Com_DPrintf("Loading MVD Hud\n");
		// Store current settings.
		if (!autohud.active)
		{
			// Save old cfg_save values so that we don't screw the users
			// settings when saving the temp config.
			int old_cmdline = cvarfuncs->GetFloat("cfg_save_cmdline");
			int old_cvars	= cvarfuncs->GetFloat("cfg_save_cvars");
			int old_cmds	= cvarfuncs->GetFloat("cfg_save_cmds");
			int old_aliases = cvarfuncs->GetFloat("cfg_save_aliases");
			int old_binds	= cvarfuncs->GetFloat("cfg_save_binds");

			autohud.old_fov = (int) scr_fov->value;
			autohud.old_multiview = (int) cl_multiview->value;
			autohud.old_newhud = (int) scr_newHud->value;

			// Make sure everything current settings are saved.
			cvarfuncs->SetFloat("cfg_save_cmdline",	1);
			cvarfuncs->SetFloat("cfg_save_cvars",	1);
			cvarfuncs->SetFloat("cfg_save_cmds",		1);
			cvarfuncs->SetFloat("cfg_save_aliases",	1);
			cvarfuncs->SetFloat("cfg_save_binds",	1);

			// Save a temporary config.
			DumpConfig(TEMPHUD_NAME".cfg");

			cvarfuncs->SetFloat("cfg_save_cmdline",	old_cmdline);
			cvarfuncs->SetFloat("cfg_save_cvars",	old_cvars);
			cvarfuncs->SetFloat("cfg_save_cmds",		old_cmds);
			cvarfuncs->SetFloat("cfg_save_aliases",	old_aliases);
			cvarfuncs->SetFloat("cfg_save_binds",	old_binds);
		}

		// load MVD HUD config
		switch ((int) autoload) {
			case 1: // load 1on1 or 4on4 or custom according to $matchtype
				if (!strncmp(Macro_MatchType(), "duel", 4)) {
					cfg_suffix = "1on1";
				} else if (!strncmp(Macro_MatchType(), "4on4", 4)) {
					cfg_suffix = "4on4";
				} else {
					cfg_suffix = "custom";
				}
				break;
			default:
			case 2:
				cfg_suffix = "custom";
				break;
		}

		Cbuf_AddText(va("exec cfg/mvdhud_%s.cfg\n", cfg_suffix));

		autohud.active = true;
		return;
	}

	if ((!cls.mvdplayback || !autoload) && autohud.active) {
		// either user decided to turn mvd autohud off or mvd playback is over
		// -> Turn autohud OFF here
		FILE *tempfile;
		char *fullname = va("%s/ezquake/"TEMPHUD_FULLPATH, com_basedir);

		Com_DPrintf("Unloading MVD Hud\n");
		// load stored settings
		cvarfuncs->SetFloat(scr_fov->name, autohud.old_fov);
		cvarfuncs->SetFloat(cl_multiview->name, autohud.old_multiview);
		cvarfuncs->SetFloat(scr_newHud->name, autohud.old_newhud);
		//Cmd_TokenizeString("exec "TEMPHUD_FULLPATH);
		Cmd_TokenizeString("cfg_load "TEMPHUD_FULLPATH);
		Cmd_Exec_f();

		// delete temp config with hud_* settings
		if ((tempfile = fopen(fullname, "rb")) && (fclose(tempfile) != EOF))
			unlink(fullname);

		autohud.active = false;
		return;
	}
#endif
}

void OnAutoHudChange(cvar_t *var, char *value, qbool *cancel) {
	HUD_AutoLoad_MVD(Q_atoi(value));
}

// Is run when a new map is loaded.
void HUD_NewMap(void) {
#if defined(WITH_PNG)
	HUD_NewRadarMap();
#endif // WITH_PNG

	autohud_loaded = false;
}

#define HUD_SHOW_ONLY_IN_TEAMPLAY		1
#define HUD_SHOW_ONLY_IN_DEMOPLAYBACK	2

qbool HUD_ShowInDemoplayback(int val)
{
	if(!cl.teamplay && val == HUD_SHOW_ONLY_IN_TEAMPLAY)
	{
		return false;
	}
	else if(!cls.demoplayback && val == HUD_SHOW_ONLY_IN_DEMOPLAYBACK)
	{
		return false;
	}
	else if(!cl.teamplay && !cls.demoplayback
		&& val == HUD_SHOW_ONLY_IN_TEAMPLAY + HUD_SHOW_ONLY_IN_DEMOPLAYBACK)
	{
		return false;
	}

	return true;
}

// Team hold filters.
static qbool teamhold_show_pent		= false;
static qbool teamhold_show_quad		= false;
static qbool teamhold_show_ring		= false;
static qbool teamhold_show_suit		= false;
static qbool teamhold_show_rl		= false;
static qbool teamhold_show_lg		= false;
static qbool teamhold_show_gl		= false;
static qbool teamhold_show_sng		= false;
static qbool teamhold_show_mh		= false;
static qbool teamhold_show_ra		= false;
static qbool teamhold_show_ya		= false;
static qbool teamhold_show_ga		= false;

void TeamHold_DrawBars(int x, int y, int width, int height,
						float team1_percent, float team2_percent,
						int team1_color, int team2_color,
						float opacity)
{
	int team1_width = 0;
	int team2_width = 0;
	int bar_height = 0;

	bar_height = Q_rint (height/2.0);
	team1_width = (int) (width * team1_percent);
	team2_width = (int) (width * team2_percent);

	clamp(team1_width, 0, width);
	clamp(team2_width, 0, width);

	Draw_AlphaFill(x, y, team1_width, bar_height, team1_color, opacity);

	y += bar_height;

	Draw_AlphaFill(x, y, team2_width, bar_height, team2_color, opacity);
}

void TeamHold_DrawPercentageBar(int x, int y, int width, int height,
								float team1_percent, float team2_percent,
								int team1_color, int team2_color,
								int show_text, int vertical,
								int vertical_text, float opacity)
{
	int _x, _y;
	int _width, _height;

	if(vertical)
	{
		//
		// Draw vertical.
		//

		// Team 1.
		_x = x;
		_y = y;
		_width = max(0, width);
		_height = Q_rint(height * team1_percent);
		_height = max(0, height);

		Draw_AlphaFill(_x, _y, _width, _height, team1_color, opacity);

		// Team 2.
		_x = x;
		_y = Q_rint(y + (height * team1_percent));
		_width = max(0, width);
		_height = Q_rint(height * team2_percent);
		_height = max(0, _height);

		Draw_AlphaFill(_x, _y, _width, _height, team2_color, opacity);

		// Show the percentages in numbers also.
		if(show_text)
		{
			// TODO: Move this to a separate function (since it's prett much copy and paste for both teams).
			// Team 1.
			if(team1_percent > 0.05)
			{
				if(vertical_text)
				{
					int percent = 0;
					int percent10 = 0;
					int percent100 = 0;

					_x = x + (width / 2) - 4;
					_y = Q_rint(y + (height * team1_percent)/2 - 12);

					percent = Q_rint(100 * team1_percent);

					if((percent100 = percent / 100))
					{
						Draw_String(_x, _y, va("%d", percent100));
						_y += 8;
					}

					if((percent10 = percent / 10))
					{
						Draw_String(_x, _y, va("%d", percent10));
						_y += 8;
					}

					Draw_String(_x, _y, va("%d", percent % 10));
					_y += 8;

					Draw_String(_x, _y, "%");
				}
				else
				{
					_x = x + (width / 2) - 12;
					_y = Q_rint(y + (height * team1_percent)/2 - 4);
					Draw_String(_x, _y, va("%2.0f%%", 100 * team1_percent));
				}
			}

			// Team 2.
			if(team2_percent > 0.05)
			{
				if(vertical_text)
				{
					int percent = 0;
					int percent10 = 0;
					int percent100 = 0;

					_x = x + (width / 2) - 4;
					_y = Q_rint(y + (height * team1_percent) + (height * team2_percent)/2 - 12);

					percent = Q_rint(100 * team2_percent);

					if((percent100 = percent / 100))
					{
						Draw_String(_x, _y, va("%d", percent100));
						_y += 8;
					}

					if((percent10 = percent / 10))
					{
						Draw_String(_x, _y, va("%d", percent10));
						_y += 8;
					}

					Draw_String(_x, _y, va("%d", percent % 10));
					_y += 8;

					Draw_String(_x, _y, "%");
				}
				else
				{
					_x = x + (width / 2) - 12;
					_y = Q_rint(y + (height * team1_percent) + (height * team2_percent)/2 - 4);
					Draw_String(_x, _y, va("%2.0f%%", 100 * team2_percent));
				}
			}
		}
	}
	else
	{
		//
		// Draw horizontal.
		//

		// Team 1.
		_x = x;
		_y = y;
		_width = Q_rint(width * team1_percent);
		_width = max(0, _width);
		_height = max(0, height);

		Draw_AlphaFill(_x, _y, _width, _height, team1_color, opacity);

		// Team 2.
		_x = Q_rint(x + (width * team1_percent));
		_y = y;
		_width = Q_rint(width * team2_percent);
		_width = max(0, _width);
		_height = max(0, height);

		Draw_AlphaFill(_x, _y, _width, _height, team2_color, opacity);

		// Show the percentages in numbers also.
		if(show_text)
		{
			// Team 1.
			if(team1_percent > 0.05)
			{
				_x = Q_rint(x + (width * team1_percent)/2 - 8);
				_y = y + (height / 2) - 4;
				Draw_String(_x, _y, va("%2.0f%%", 100 * team1_percent));
			}

			// Team 2.
			if(team2_percent > 0.05)
			{
				_x = Q_rint(x + (width * team1_percent) + (width * team2_percent)/2 - 8);
				_y = y + (height / 2) - 4;
				Draw_String(_x, _y, va("%2.0f%%", 100 * team2_percent));
			}
		}
	}
}

#ifdef HAXX
static void SCR_HUD_DrawTeamHoldBar(hud_t *hud)
{
	int x, y;
	int height = 8;
	int width = 0;
	float team1_percent = 0;
	float team2_percent = 0;

	static cvar_t
        *hud_teamholdbar_style = NULL,
		*hud_teamholdbar_opacity,
		*hud_teamholdbar_width,
		*hud_teamholdbar_height,
		*hud_teamholdbar_vertical,
		*hud_teamholdbar_show_text,
		*hud_teamholdbar_onlytp,
		*hud_teamholdbar_vertical_text;

    if (hud_teamholdbar_style == NULL)    // first time
    {
		hud_teamholdbar_style				= HUD_FindVar(hud, "style");
		hud_teamholdbar_opacity				= HUD_FindVar(hud, "opacity");
		hud_teamholdbar_width				= HUD_FindVar(hud, "width");
		hud_teamholdbar_height				= HUD_FindVar(hud, "height");
		hud_teamholdbar_vertical			= HUD_FindVar(hud, "vertical");
		hud_teamholdbar_show_text			= HUD_FindVar(hud, "show_text");
		hud_teamholdbar_onlytp				= HUD_FindVar(hud, "onlytp");
		hud_teamholdbar_vertical_text		= HUD_FindVar(hud, "vertical_text");
    }

	height = max(1, hud_teamholdbar_height->value);
	width = max(0, hud_teamholdbar_width->value);

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_teamholdbar_onlytp->value))
	{
		HUD_PrepareDraw(hud, width , height, &x, &y);
		return;
	}

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		// We need something to work with.
		if(stats_grid != NULL)
		{
			// Check if we have any hold values to calculate from.
			if(stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count > 0)
			{
				// Calculate the percentage for the two teams for the "team strength bar".
				team1_percent = ((float)stats_grid->teams[STATS_TEAM1].hold_count) / (stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count);
				team2_percent = ((float)stats_grid->teams[STATS_TEAM2].hold_count) / (stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count);

				team1_percent = fabs(max(0, team1_percent));
				team2_percent = fabs(max(0, team2_percent));
			}
			else
			{
				Draw_AlphaFill(x, y, hud_teamholdbar_width->value, height, 0, hud_teamholdbar_opacity->value*0.5);
				return;
			}

			// Draw the percentage bar.
			TeamHold_DrawPercentageBar(x, y, width, height,
				team1_percent, team2_percent,
				stats_grid->teams[STATS_TEAM1].color,
				stats_grid->teams[STATS_TEAM2].color,
				hud_teamholdbar_show_text->value,
				hud_teamholdbar_vertical->value,
				hud_teamholdbar_vertical_text->value,
				hud_teamholdbar_opacity->value);
		}
		else
		{
			// If there's no stats grid available we don't know what to show, so just show a black frame.
			Draw_AlphaFill(x, y, hud_teamholdbar_width->value, height, 0, hud_teamholdbar_opacity->value * 0.5);
		}
	}
}
#endif

void TeamHold_OnChangeItemFilterInfo(cvar_t *var, char *oldvalue)
{
//	char *start = var->string;
//	char *end = start;
//	int order = 0;

	// Parse the item filter.
	teamhold_show_rl		= Utils_RegExpMatch("RL",	var->string);
	teamhold_show_quad		= Utils_RegExpMatch("QUAD",	var->string);
	teamhold_show_ring		= Utils_RegExpMatch("RING",	var->string);
	teamhold_show_pent		= Utils_RegExpMatch("PENT",	var->string);
	teamhold_show_suit		= Utils_RegExpMatch("SUIT",	var->string);
	teamhold_show_lg		= Utils_RegExpMatch("LG",	var->string);
	teamhold_show_gl		= Utils_RegExpMatch("GL",	var->string);
	teamhold_show_sng		= Utils_RegExpMatch("SNG",	var->string);
	teamhold_show_mh		= Utils_RegExpMatch("MH",	var->string);
	teamhold_show_ra		= Utils_RegExpMatch("RA",	var->string);
	teamhold_show_ya		= Utils_RegExpMatch("YA",	var->string);
	teamhold_show_ga		= Utils_RegExpMatch("GA",	var->string);
#ifdef HAXX
	// Reset the ordering of the items.
	StatsGrid_ResetHoldItemsOrder();

	// Trim spaces from the start of the word.
	while (*start && *start == ' ')
	{
		start++;
	}

	end = start;

	// Go through the string word for word and set a
	// rising order for each hold item based on their
	// order in the string.
	while (*end)
	{
		if (*end != ' ')
		{
			// Not at the end of the word yet.
			end++;
			continue;
		}
		else
		{
			// We've found a word end.
			char temp[256];

			// Try matching the current word with a hold item
			// and set it's ordering according to it's placement
			// in the string.
			strlcpy (temp, start, min(end - start, sizeof(temp)));
			StatsGrid_SetHoldItemOrder(temp, order);
			order++;

			// Get rid of any additional spaces.
			while (*end && *end == ' ')
			{
				end++;
			}

			// Start trying to find a new word.
			start = end;
		}
	}

	// Order the hold items.
	StatsGrid_SortHoldItems();
#endif
}

#define HUD_TEAMHOLDINFO_STYLE_TEAM_NAMES		0
#define HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS		1
#define HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS2	2

#ifdef HAXX
static void SCR_HUD_DrawTeamHoldInfo(hud_t *hud)
{
	int i;
	int x, y;
	int width, height;

	static cvar_t
        *hud_teamholdinfo_style = NULL,
		*hud_teamholdinfo_opacity,
		*hud_teamholdinfo_width,
		*hud_teamholdinfo_height,
		*hud_teamholdinfo_onlytp,
		*hud_teamholdinfo_itemfilter;

    if (hud_teamholdinfo_style == NULL)    // first time
    {
		char val[256];

		hud_teamholdinfo_style				= HUD_FindVar(hud, "style");
		hud_teamholdinfo_opacity			= HUD_FindVar(hud, "opacity");
		hud_teamholdinfo_width				= HUD_FindVar(hud, "width");
		hud_teamholdinfo_height				= HUD_FindVar(hud, "height");
		hud_teamholdinfo_onlytp				= HUD_FindVar(hud, "onlytp");
		hud_teamholdinfo_itemfilter			= HUD_FindVar(hud, "itemfilter");

		// Unecessary to parse the item filter string on each frame.
		hud_teamholdinfo_itemfilter->OnChange = TeamHold_OnChangeItemFilterInfo;

		// Parse the item filter the first time (trigger the OnChange function above).
		strlcpy (val, hud_teamholdinfo_itemfilter->string, sizeof(val));
		Cvar_Set (hud_teamholdinfo_itemfilter, val);
    }

	// Get the height based on how many items we have, or what the user has set it to.
	height = max(0, hud_teamholdinfo_height->value);
	width = max(0, hud_teamholdinfo_width->value);

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_teamholdinfo_onlytp->value))
	{
		HUD_PrepareDraw(hud, width , height, &x, &y);
		return;
	}

	// We don't have anything to show.
	if(stats_important_ents == NULL || stats_grid == NULL)
	{
		HUD_PrepareDraw(hud, width , height, &x, &y);
		return;
	}

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		int _y = 0;

		_y = y;

		// Go through all the items and print the stats for them.
		for(i = 0; i < stats_important_ents->count; i++)
		{
			float team1_percent;
			float team2_percent;
			int team1_hold_count = 0;
			int team2_hold_count = 0;
			int names_width = 0;

			// Don't draw outside the specified height.
			if((_y - y) + 8 > height)
			{
				break;
			}

			// If the item isn't of the specified type, then skip it.
			if(!(	(teamhold_show_rl	&& !strncmp(stats_important_ents->list[i].name, "RL",	2))
				||	(teamhold_show_quad	&& !strncmp(stats_important_ents->list[i].name, "QUAD", 4))
				||	(teamhold_show_ring	&& !strncmp(stats_important_ents->list[i].name, "RING", 4))
				||	(teamhold_show_pent	&& !strncmp(stats_important_ents->list[i].name, "PENT", 4))
				||	(teamhold_show_suit	&& !strncmp(stats_important_ents->list[i].name, "SUIT", 4))
				||	(teamhold_show_lg	&& !strncmp(stats_important_ents->list[i].name, "LG",	2))
				||	(teamhold_show_gl	&& !strncmp(stats_important_ents->list[i].name, "GL",	2))
				||	(teamhold_show_sng	&& !strncmp(stats_important_ents->list[i].name, "SNG",	3))
				||	(teamhold_show_mh	&& !strncmp(stats_important_ents->list[i].name, "MH",	2))
				||	(teamhold_show_ra	&& !strncmp(stats_important_ents->list[i].name, "RA",	2))
				||	(teamhold_show_ya	&& !strncmp(stats_important_ents->list[i].name, "YA",	2))
				||	(teamhold_show_ga	&& !strncmp(stats_important_ents->list[i].name, "GA",	2))
				))
			{
				continue;
			}

			// Calculate the width of the longest item name so we can use it for padding.
			names_width = 8 * (stats_important_ents->longest_name + 1);

			// Calculate the percentages of this item that the two teams holds.
			team1_hold_count = stats_important_ents->list[i].teams_hold_count[STATS_TEAM1];
			team2_hold_count = stats_important_ents->list[i].teams_hold_count[STATS_TEAM2];

			team1_percent = ((float)team1_hold_count) / (team1_hold_count + team2_hold_count);
			team2_percent = ((float)team2_hold_count) / (team1_hold_count + team2_hold_count);

			team1_percent = fabs(max(0, team1_percent));
			team2_percent = fabs(max(0, team2_percent));

			// Write the name of the item.
			Draw_ColoredString(x, _y, va("&cff0%s:", stats_important_ents->list[i].name), 0);

			if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_TEAM_NAMES)
			{
				//
				// Prints the team name that holds the item.
				//
				if(team1_percent > team2_percent)
				{
					Draw_ColoredString(x + names_width, _y, stats_important_ents->teams[STATS_TEAM1].name, 0);
				}
				else if(team1_percent < team2_percent)
				{
					Draw_ColoredString(x + names_width, _y, stats_important_ents->teams[STATS_TEAM2].name, 0);
				}
			}
			else if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS)
			{
				//
				// Show a percenteage bar for the item.
				//
				TeamHold_DrawPercentageBar(x + names_width, _y,
					Q_rint(hud_teamholdinfo_width->value - names_width), 8,
					team1_percent, team2_percent,
					stats_important_ents->teams[STATS_TEAM1].color,
					stats_important_ents->teams[STATS_TEAM2].color,
					0, // Don't show percentage values, get's too cluttered.
					false,
					false,
					hud_teamholdinfo_opacity->value);
			}
			else if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS2)
			{
				TeamHold_DrawBars(x + names_width, _y,
					Q_rint(hud_teamholdinfo_width->value - names_width), 8,
					team1_percent, team2_percent,
					stats_important_ents->teams[STATS_TEAM1].color,
					stats_important_ents->teams[STATS_TEAM2].color,
					hud_teamholdinfo_opacity->value);
			}

			// Next line.
			_y += 8;
		}
	}
}
#endif

static int SCR_HudDrawTeamInfoPlayer(teamplayerinfo_t *ti_cl, int x, int y, int maxname, int maxloc, qbool width_only, hud_t *hud);

#define FONTWIDTH 8
static void SCR_HUD_DrawTeamInfo(hud_t *hud)
{
	int x, y, _y, width, height;
	int i, j, k, slots_num, maxname, maxloc;
	char tmp[1024], *nick;
	teamplayerinfo_t ti_clients[MAX_CLIENTS];

	extern qbool hud_editor;

	static cvar_t
		*hud_teaminfo_weapon_style = NULL,
                *hud_teaminfo_align_right,
		*hud_teaminfo_loc_width,
		*hud_teaminfo_name_width,
		*hud_teaminfo_show_enemies,
		*hud_teaminfo_show_self,
		*hud_teaminfo_scale;

	if (hud_teaminfo_weapon_style == NULL)    // first time
	{
		hud_teaminfo_weapon_style			= HUD_FindVar(hud, "weapon_style");
		hud_teaminfo_align_right			= HUD_FindVar(hud, "align_right");
		hud_teaminfo_loc_width				= HUD_FindVar(hud, "loc_width");
		hud_teaminfo_name_width				= HUD_FindVar(hud, "name_width");
		hud_teaminfo_show_enemies			= HUD_FindVar(hud, "show_enemies");
		hud_teaminfo_show_self				= HUD_FindVar(hud, "show_self");
		hud_teaminfo_scale					= HUD_FindVar(hud, "scale");
	}

	// Don't update hud item unless first view is beeing displayed
//	if ( CURRVIEW != 1 && CURRVIEW != 0)
//		return;

	slots_num = clientfuncs->GetTeamInfo?clientfuncs->GetTeamInfo(ti_clients, countof(ti_clients), hud_teaminfo_show_enemies->ival, hud_teaminfo_show_self->ival?-1:0):0;

	// fill data we require to draw teaminfo
	for ( maxloc = maxname = i = 0; i < slots_num; i++ ) {
		// dynamically guess max length of name/location
		nick = (ti_clients[i].nick[0] ? ti_clients[i].nick : cl.players[i].name); // we use nick or name
		maxname = max(maxname, strlen(TP_ParseFunChars(nick)));

		strlcpy(tmp, TP_LocationName(ti_clients[i].org), sizeof(tmp));
		maxloc  = max(maxloc,  strlen(TP_ParseFunChars(tmp)));
	}

	// well, better use fixed loc length
	maxloc  = bound(0, hud_teaminfo_loc_width->ival, 100);
	// limit name length
	maxname = bound(0, maxname, hud_teaminfo_name_width->ival);

	// this does't draw anything, just calculate width
	width = FONTWIDTH * hud_teaminfo_scale->value * SCR_HudDrawTeamInfoPlayer(&ti_clients[0], 0, 0, maxname, maxloc, true, hud);
	height = FONTWIDTH * hud_teaminfo_scale->value * (hud_teaminfo_show_enemies->ival?slots_num+n_teams:slots_num);

	if (hud_editor)
		HUD_PrepareDraw(hud, width , FONTWIDTH, &x, &y);

	if ( !slots_num )
		return;

	if (!cl.teamplay)  // non teamplay mode
		return;

	if (!HUD_PrepareDraw(hud, width , height, &x, &y))
		return;

	_y = y ;
	x = (hud_teaminfo_align_right->value ? x - (width * (FONTWIDTH * hud_teaminfo_scale->value)) : x);

	// If multiple teams are displayed then sort the display and print team header on overlay
	k=0;
	if (hud_teaminfo_show_enemies->ival)
	{
		while (sorted_teams[k].name)
		{
			Draw_SString (x, _y, sorted_teams[k].name, hud_teaminfo_scale->value);
			sprintf(tmp,"%s %i",TP_ParseFunChars("$."), sorted_teams[k].frags);
			Draw_SString (x+(strlen(sorted_teams[k].name)+1)*FONTWIDTH, _y, tmp, hud_teaminfo_scale->value);
			_y += FONTWIDTH * hud_teaminfo_scale->value;
			for ( j = 0; j < slots_num; j++ ) 
			{
				i = ti_clients[j].client;
				if (!strcmp(cl.players[i].team,sorted_teams[k].name))
				{
					SCR_HudDrawTeamInfoPlayer(&ti_clients[j], x, _y, maxname, maxloc, false, hud);
					_y += FONTWIDTH * hud_teaminfo_scale->value;
				}
			}
		k++;
		}
	}
	else 
	{
		for ( j = 0; j < slots_num; j++ ) {
			SCR_HudDrawTeamInfoPlayer(&ti_clients[j], x, _y, maxname, maxloc, false, hud);
			_y += FONTWIDTH * hud_teaminfo_scale->value;
		}
	}
}

qbool Has_Both_RL_and_LG (int flags) { return (flags & IT_ROCKET_LAUNCHER) && (flags & IT_LIGHTNING); }
#define FONTWIDTH 8
void str_align_right (char *target, size_t size, const char *source, size_t length)
{
	if (length > size - 1)
		length = size - 1;

	if (strlen(source) >= length) {
		strlcpy(target, source, size);
		target[length] = 0;
	} else {
		int i;

		for (i = 0; i < length - strlen(source); i++) {
			target[i] = ' ';
		}

		strlcpy(target + i, source, size - i);
	}
}
int Player_GetTrackId(int uid)
{
	return uid;
}
unsigned int BestWeaponFromStatItems(unsigned int items)
{
	int i;
	for (i = 1<<7; i; i>>=1)
	{
		if (items & i)
			return i;
	}
	return 0;
}
mpic_t * SCR_GetWeaponIconByFlag (int flag)
{
	int i, j;
	for (i = 0, j = 1; i < 7; i++, j*=2)
	{
		if (flag == j)
			return sb_weapons[0][i];
	}
	return NULL;
}
static int SCR_HudDrawTeamInfoPlayer(teamplayerinfo_t *ti_cl, int x, int y, int maxname, int maxloc, qbool width_only, hud_t *hud)
{
	extern mpic_t * SCR_GetWeaponIconByFlag (int flag);

	char *s, *loc, tmp[1024], tmp2[1024], *aclr;
	int x_in = x; // save x
	int i;
	mpic_t *pic;
	float scale = HUD_FindVar(hud, "scale")->value;

	if (!ti_cl)
		return 0;

	i = ti_cl->client;

	if (i < 0 || i >= MAX_CLIENTS)
	{
		Com_DPrintf("SCR_Draw_TeamInfoPlayer: wrong client %d\n", i);
		return 0;
	}

	// this limit len of string because TP_ParseFunChars() do not check overflow
	strlcpy(tmp2, HUD_FindVar(hud, "layout")->string , sizeof(tmp2));
	strlcpy(tmp2, TP_ParseFunChars(tmp2), sizeof(tmp2));
	s = tmp2;

	//
	// parse/draw string like this "%n %h:%a %l %p %w"
	//

	for ( ; *s; s++) {
		switch( (int) s[0] ) {
		case '%':

			s++; // advance

			switch( (int) s[0] ) {
			case 'n': // draw name

				if(!width_only) {
					char *nick = TP_ParseFunChars(ti_cl->nick[0] ? ti_cl->nick : cl.players[i].name);
					str_align_right(tmp, sizeof(tmp), nick, maxname);
					Draw_SString (x, y, tmp, scale);
				}
				x += maxname * FONTWIDTH * scale;

				break;
			case 'w': // draw "best" weapon icon/name

				switch (HUD_FindVar(hud, "weapon_style")->ival) {
				case 1:
					if(!width_only) {
						if (Has_Both_RL_and_LG(ti_cl->items)) {
							char *weap_str = cvarfuncs->GetNVFDG("tp_name_rlg", "rlg", 0, NULL, NULL)->string;
							char weap_white_stripped[32];
							Util_SkipChars(weap_str, "{}", weap_white_stripped, 32);
							Draw_ColoredString (x, y, weap_white_stripped, false);
						}
						else {
							char *weap_str = TP_ItemName(BestWeaponFromStatItems( ti_cl->items ));
							char weap_white_stripped[32];
							Util_SkipChars(weap_str, "{}", weap_white_stripped, 32);
							Draw_ColoredString (x, y, weap_white_stripped, false);
						}
					}
					x += 3 * FONTWIDTH * scale;

					break;
				default: // draw image by default
					if(!width_only)
						if ( (pic = SCR_GetWeaponIconByFlag(BestWeaponFromStatItems( ti_cl->items ))) )
							Draw_SPic (x, y, pic, 0.5 * scale);
					x += 2 * FONTWIDTH * scale;

					break;
				}

				break;
			case 'h': // draw health, padding with space on left side
			case 'H': // draw health, padding with space on right side

				if(!width_only) {
					snprintf(tmp, sizeof(tmp), (s[0] == 'h' ? "%s%3d" : "%s%-3d"), (ti_cl->health < HUD_FindVar(hud, "low_health")->ival ? "&cf00" : ""), (int)ti_cl->health);
					Draw_SString (x, y, tmp, scale);
				}
				x += 3 * FONTWIDTH * scale;

				break;
			case 'a': // draw armor, padded with space on left side
			case 'A': // draw armor, padded with space on right side

				aclr = "";

				//
				// different styles of armor
				//
				switch (HUD_FindVar(hud,"armor_style")->ival) {
				case 1: // image prefixed armor value
					if(!width_only) {
						if (ti_cl->items & IT_ARMOR3)
							Draw_SPic (x, y, sb_armor[2], 1.0/3 * scale);
						else if (ti_cl->items & IT_ARMOR2)
							Draw_SPic (x, y, sb_armor[1], 1.0/3 * scale);
						else if (ti_cl->items & IT_ARMOR1)
							Draw_SPic (x, y, sb_armor[0], 1.0/3 * scale);
					}
					x += FONTWIDTH * scale;

					break;
				case 2: // colored background of armor value
                                        /*
					if(!width_only) {
						byte col[4] = {255, 255, 255, 0};

						if (ti_cl->items & IT_ARMOR3) {
							col[0] = 255; col[1] =   0; col[2] =   0; col[3] = 255;
						}
						else if (ti_cl->items & IT_ARMOR2) {
							col[0] = 255; col[1] = 255; col[2] =   0; col[3] = 255;
						}
						else if (ti_cl->items & IT_ARMOR1) {
							col[0] =   0; col[1] = 255; col[2] =   0; col[3] = 255;
						}
					}
                                        */

					break;
				case 3: // colored armor value
					if(!width_only) {
						if (ti_cl->items & IT_ARMOR3)
							aclr = "&cf00";
						else if (ti_cl->items & IT_ARMOR2)
							aclr = "&cff0";
						else if (ti_cl->items & IT_ARMOR1)
							aclr = "&c0f0";
					}

					break;
				case 4: // armor value prefixed with letter
					if(!width_only) {
						if (ti_cl->items & IT_ARMOR3)
							Draw_SString (x, y, "r", scale);
						else if (ti_cl->items & IT_ARMOR2)
							Draw_SString (x, y, "y", scale);
						else if (ti_cl->items & IT_ARMOR1)
							Draw_SString (x, y, "g", scale);
					}
					x += FONTWIDTH * scale;

					break;
				}

				if(!width_only) { // value drawed no matter which style
					snprintf(tmp, sizeof(tmp), (s[0] == 'a' ? "%s%3d" : "%s%-3d"), aclr, (int)ti_cl->armor);
					Draw_SString (x, y, tmp, scale);
				}
				x += 3 * FONTWIDTH * scale;

				break;
			case 'l': // draw location

				if(!width_only) {
					loc = TP_LocationName(ti_cl->org);
					if (!loc[0])
						loc = "unknown";

					str_align_right(tmp, sizeof(tmp), TP_ParseFunChars(loc), maxloc);
					Draw_SString (x, y, tmp, scale);
				}
				x += maxloc * FONTWIDTH * scale;

				break;
			case 'p': // draw powerups
			switch (HUD_FindVar(hud, "powerup_style")->ival) {
				case 1: // quad/pent/ring image
					if(!width_only) {
						if (ti_cl->items & IT_QUAD)
							Draw_SPic (x, y, sb_items[5], 1.0/2);
						x += FONTWIDTH;
						if (ti_cl->items & IT_INVULNERABILITY)
							Draw_SPic (x, y, sb_items[3], 1.0/2);
						x += FONTWIDTH;
						if (ti_cl->items & IT_INVISIBILITY)
							Draw_SPic (x, y, sb_items[2], 1.0/2);
						x += FONTWIDTH;
					}
					else { x += 3* FONTWIDTH; }
					break;

				case 2: // player powerup face
					if(!width_only) {
						if ( sb_face_quad && (ti_cl->items & IT_QUAD))
							Draw_SPic (x, y, sb_face_quad, 1.0/3);
						x += FONTWIDTH;
						if ( sb_face_invuln && (ti_cl->items & IT_INVULNERABILITY))
							Draw_SPic (x, y, sb_face_invuln, 1.0/3);
						x += FONTWIDTH;
						if ( sb_face_invis && (ti_cl->items & IT_INVISIBILITY))
							Draw_SPic (x, y, sb_face_invis, 1.0/3);
						x += FONTWIDTH;
					}
					else { x += 3* FONTWIDTH; }
					break;

				case 3: // colored font (QPR)
					if(!width_only) {
						if (ti_cl->items & IT_QUAD)
							Draw_ColoredString (x, y, "&c03fQ", false);
						x += FONTWIDTH;
						if (ti_cl->items & IT_INVULNERABILITY)
							Draw_ColoredString (x, y, "&cf00P", false);
						x += FONTWIDTH;
						if (ti_cl->items & IT_INVISIBILITY)
							Draw_ColoredString (x, y, "&cff0R", false);
						x += FONTWIDTH;
					}
					else { x += 3* FONTWIDTH; }
					break;
			}
			break;

			case 't':
				if(!width_only)
				{
					sprintf(tmp, "%i", Player_GetTrackId(cl.players[ti_cl->client].userid));
					Draw_SString (x, y, tmp, scale);
				}
				x += FONTWIDTH * scale; // will break if tracknumber is double digits
				break;

			case '%': // wow, %% result in one %, how smart

				if(!width_only)
					Draw_SString (x, y, "%", scale);
				x += FONTWIDTH * scale;

				break;

			default: // print %x - that mean sequence unknown

				if(!width_only) {
					snprintf(tmp, sizeof(tmp), "%%%c", s[0]);
					Draw_SString (x, y, tmp, scale);
				}
				x += (s[0] ? 2 : 1) * FONTWIDTH * scale;

				break;
			}

			break;

		default: // print x
			if(!width_only) {
				snprintf(tmp, sizeof(tmp), "%c", s[0]);
				if (s[0] != ' ') // inhuman smart optimization, do not print space!
					Draw_SString (x, y, tmp, scale);
			}
			x += FONTWIDTH * scale;

			break;
		}
	}

	return (x - x_in) / (FONTWIDTH * scale); // return width
}

#ifdef HAXX
void SCR_HUD_DrawItemsClock(hud_t *hud)
{
	extern qbool hud_editor;
	int width, height;
	int x, y;
	static cvar_t *hud_itemsclock_timelimit = NULL, *hud_itemsclock_style;

	if (hud_itemsclock_timelimit == NULL) {
		hud_itemsclock_timelimit = HUD_FindVar(hud, "timelimit");
		hud_itemsclock_style = HUD_FindVar(hud, "style");
	}

	MVD_ClockList_TopItems_DimensionsGet(hud_itemsclock_timelimit->value, hud_itemsclock_style->ival, &width, &height);
	
	if (hud_editor)
		HUD_PrepareDraw(hud, width, LETTERHEIGHT, &x, &y);

	if (!height)
		return;

    if (!HUD_PrepareDraw(hud, width, height, &x, &y))
        return;
	
	MVD_ClockList_TopItems_Draw(hud_itemsclock_timelimit->value, hud_itemsclock_style->ival, x, y);
}
#endif

//
// TODO: decide what to do in freefly mode (and how to catch it?!), now all score_* hud elements just draws "0"
//
void SCR_HUD_DrawScoresTeam(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align, *colorize;
	int value = 0;
	int i;

    if (scale == NULL)  // first time called
    {
        scale		= HUD_FindVar(hud, "scale");
        style		= HUD_FindVar(hud, "style");
        digits		= HUD_FindVar(hud, "digits");
        align		= HUD_FindVar(hud, "align");
		colorize	= HUD_FindVar(hud, "colorize");
    }
	
	//
	// AAS: someone please tell me how to do it in a proper way!
	//
	if(cl.teamplay)
	{
		for(i = 0; i < n_teams; i++)
		{
			// playing qwd demo || mvd spec/demo || playing 
			if( (cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].team, sorted_teams[i].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) )
			{
				value = sorted_teams[i].frags;
				break;
			}
		}
	}
	else if(cl.deathmatch)
	{
		for(i = 0; i < n_players; i++)
		{
			if( (cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].name, cl.players[sorted_players[i].playernum].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) )
			{
				value = cl.players[sorted_players[i].playernum].frags;
				break;
			}
		}
	}

	SCR_HUD_DrawNum(hud, value, (colorize->ival) ? (value < 0 || colorize->ival > 1) : false, scale->value, style->value, digits->value, align->string);
}

void SCR_HUD_DrawScoresEnemy(hud_t *hud)
{
	static cvar_t *scale = NULL, *style, *digits, *align, *colorize;
	int value = 0;
	int i;

	if (scale == NULL)  // first time called
	{
		scale		= HUD_FindVar(hud, "scale");
		style		= HUD_FindVar(hud, "style");
		digits		= HUD_FindVar(hud, "digits");
		align		= HUD_FindVar(hud, "align");
		colorize	= HUD_FindVar(hud, "colorize");
	}
	
	//
	// AAS: voodoo, again
	//
	if(cl.teamplay)
	{
		for(i = 0; i < n_teams; i++)
		{
			
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].team, sorted_teams[i].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) )
			{
				if(n_teams > 1)
					value = sorted_teams[i == 0 ? 1 : 0].frags;
				break;
			}
		}
	}
	else if(cl.deathmatch)
	{
		for(i = 0; i < n_players; i++)
		{
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].name, cl.players[sorted_players[i].playernum].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) )
			{
				if(n_players > 1)
					value = cl.players[sorted_players[i == 0 ? 1 : 0].playernum].frags;
				break;
			}
		}
	}

	SCR_HUD_DrawNum(hud, value, (colorize->ival) ? (value < 0 || colorize->ival > 1) : false, scale->value, style->value, digits->value, align->string);
}

void SCR_HUD_DrawScoresDifference(hud_t *hud)
{
	static cvar_t *scale = NULL, *style, *digits, *align, *colorize;
	int value = 0;
	int i;

	if (scale == NULL)  // first time called
	{
		scale		= HUD_FindVar(hud, "scale");
		style		= HUD_FindVar(hud, "style");
		digits		= HUD_FindVar(hud, "digits");
		align		= HUD_FindVar(hud, "align");
		colorize	= HUD_FindVar(hud, "colorize");
	}

	//
	// AAS: more voodoo
	//
	if(cl.teamplay)
	{
		for(i = 0; i < n_teams; i++)
		{
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].team, sorted_teams[i].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) )
			{
				if(i == 0)
				{
					if(n_teams > 1)
						value = sorted_teams[0].frags - sorted_teams[1].frags;
					else
						value = sorted_teams[0].frags;
				}
				else
				{
					if(n_teams > 1)
						value = sorted_teams[i].frags - sorted_teams[0].frags;
				}
				break;
			}
		}
	}
	else if(cl.deathmatch)
	{
		for(i = 0; i < n_players; i++)
		{
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].name, cl.players[sorted_players[i].playernum].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) )
			{
				if(i == 0)
				{
					if(n_players > 1)
						value = cl.players[sorted_players[0].playernum].frags - cl.players[sorted_players[1].playernum].frags;
					else
						value = cl.players[sorted_players[0].playernum].frags;
				}
				else
				{
					if(n_players > 1)
						value = cl.players[sorted_players[i].playernum].frags - cl.players[sorted_players[0].playernum].frags;
				}
				break;
			}
		}
	}

	SCR_HUD_DrawNum(hud, value, (colorize->ival) ? (value < 0 || colorize->ival > 1) : false, scale->value, style->value, digits->value, align->string);
}

void SCR_HUD_DrawScoresPosition(hud_t *hud)
{
	static cvar_t *scale = NULL, *style, *digits, *align, *colorize;
	int value = 0;
	int i;

	if (scale == NULL)  // first time called
	{
		scale		= HUD_FindVar(hud, "scale");
		style		= HUD_FindVar(hud, "style");
		digits		= HUD_FindVar(hud, "digits");
		align		= HUD_FindVar(hud, "align");
		colorize	= HUD_FindVar(hud, "colorize");
	}

	//
	// AAS: someone, please stop me
	//
	if(cl.teamplay)
	{
		for(i = 0; i < n_teams; i++)
		{
			if( (cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].team, sorted_teams[i].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) )
			{
				value = i;
				break;
			}
		}
	}
	else if(cl.deathmatch)
	{
		for(i = 0; i < n_players; i++)
		{
			if( (cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].name, cl.players[sorted_players[i].playernum].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) )
			{
				value = i;
				break;
			}
		}
	}

	SCR_HUD_DrawNum(hud, value, (colorize->ival) ? (value < 0 || colorize->ival > 1) : false, scale->value, style->value, digits->value, align->string);
}

/*
	ezQuake's analogue of +scores of KTX
	( t:x e:x [x] )
*/
void SCR_HUD_DrawScoresBar(hud_t *hud)
{
	static	cvar_t *scale = NULL, *style, *format_big, *format_small;
	int		width = 0, height = 0, x, y;
	int		i = 0;

	int		s_team = 0, s_enemy = 0, s_difference = 0;
	char	*n_team = "T", *n_enemy = "E";

	char	buf[256];
	char	c, *out, *temp,	*in;

	if (scale == NULL)  // first time called
	{
		scale		= HUD_FindVar(hud, "scale");
		style		= HUD_FindVar(hud, "style");
		format_big	= HUD_FindVar(hud, "format_big");
		format_small= HUD_FindVar(hud, "format_small");
	}

	//
	// AAS: nightmare comes back
	//
	if(cl.teamplay)
	{
		for(i = 0; i < n_teams; i++)
		{
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].team, sorted_teams[i].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(sorted_teams[i].name, cl.players[cl.playernum].team) == 0) )
			{
				s_team = sorted_teams[i].frags;
				n_team = sorted_teams[i].name;
				if(n_teams > 1)
				{
					s_enemy = sorted_teams[i == 0 ? 1 : 0].frags;
					n_enemy = sorted_teams[i == 0 ? 1 : 0].name;
				}
				s_difference = s_team - s_enemy;
				break;
			}
		}
	}
	else if(cl.deathmatch)
	{
		for(i = 0; i < n_players; i++)
		{
			if(	(cls.demoplayback && !cl.spectator && !cls.mvdplayback && strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) ||
				((cls.demoplayback || cl.spectator) && ((strcmp(cl.players[spec_track].name, cl.players[sorted_players[i].playernum].name) == 0) && (Cam_TrackNum() >= 0))) ||
				(strcmp(cl.players[sorted_players[i].playernum].name, cl.players[cl.playernum].name) == 0) )
			{
				s_team = cl.players[sorted_players[i].playernum].frags;
				if(n_players > 1)
				{
					s_enemy = cl.players[sorted_players[i == 0 ? 1 : 0].playernum].frags;
				}
				s_difference = s_team - s_enemy;
				break;
			}
		}
	}


	// two pots of delicious customized copypasta from math_tools.c
	switch(style->ival)
	{
		// Big
		case 1:
			in = TP_ParseFunChars(format_big->string);
			buf[0] = 0;
			out = buf;

			while((c = *in++) && (out - buf < sizeof(buf) - 1))
			{
				if((c == '%') && *in)
				{
					switch((c = *in++))
					{
						// c = colorize, r = reset
						case 'd':
							temp = va("%d", s_difference);
							width += (s_difference >= 0) ? strlen(temp) * 24 : ((strlen(temp) - 1) * 24) + 16;
							break;
						case 'D':
							temp = va("c%dr", s_difference);
							width += (s_difference >= 0) ? (strlen(temp) - 2) * 24 : ((strlen(temp) - 3) * 24) + 16;
							break;
						case 'e':
							temp = va("%d", s_enemy);
							width += (s_enemy >= 0) ? strlen(temp) * 24 : ((strlen(temp) - 1) * 24) + 16;
							break;
						case 'E':
							temp = va("c%dr", s_enemy);
							width += (s_enemy >= 0) ? (strlen(temp) - 2) * 24 : ((strlen(temp) - 3) * 24) + 16;
							break;
						case 'p':
							temp = va("%d", i + 1);
							width += 24;
							break;
						case 't':
							temp = va("%d", s_team);
							width += (s_team >= 0) ? strlen(temp) * 24 : ((strlen(temp) - 1) * 24) + 16;
							break;
						case 'T':
							temp = va("c%dr", s_team);
							width += (s_team >= 0) ? (strlen(temp) - 2) * 24 : ((strlen(temp) - 3) * 24) + 16;
							break;
						case 'z':
							if(s_difference >= 0)
							{
								temp = va("%d", s_difference);
								width += strlen(temp) * 24;
							}
							else
							{
								temp = va("c%dr", -(s_difference));
								width += (strlen(temp) - 2) * 24;
							}
							break;
						case 'Z':
							if(s_difference >= 0)
							{
								temp = va("c%dr", s_difference);
								width += (strlen(temp) - 2) * 24;
							}
							else
							{
								temp = va("%d", -(s_difference));
								width += strlen(temp) * 24;
							}
							break;
						default:
							temp = NULL;
							break;
					}
					
					if(temp != NULL)
					{
						strlcpy(out, temp, sizeof(buf) - (out - buf));
						out += strlen(temp);
					}
				}
				else if (c == ':' || c == '/' || c == '-' || c == ' ')
				{
					width += 16;
					*out++ = c;
				}
			}
			*out = 0;
			break;

		// Small
		case 0:	
		default:
			in = TP_ParseFunChars(format_small->string);
			buf[0] = 0;
			out = buf;

			while((c = *in++) && (out - buf < sizeof(buf) - 1))
			{
				if((c == '%') && *in)
				{
					switch((c = *in++))
					{
						case '%':
							temp = "%";
							break;
						case 't':
							temp = va("%d", s_team);
							break;
						case 'e':
							temp = va("%d", s_enemy);
							break;
						case 'd':
							temp = va("%d", s_difference);
							break;
						case 'p':
							temp = va("%d", i + 1);
							break;
						case 'T':
							temp = n_team;
							break;
						case 'E':
							temp = n_enemy;
							break;
						case 'D':
							temp = va("%+d", s_difference);
							break;
						default:
							temp = va("%%%c", c);
							break;
					}
					strlcpy(out, temp, sizeof(buf) - (out - buf));
					out += strlen(temp);
				}
				else
				{
					*out++ = c;
				}
			}
			*out = 0;
			break;
	}

	switch(style->ival)
	{
		// Big
		case 1:
			width *= scale->value;
			height = 24 * scale->value;

			if(HUD_PrepareDraw(hud, width, height, &x, &y))
			{
				SCR_DrawWadString(x, y, scale->value, buf);
			}
			break;

		// Small
		case 0:
		default:
			width = 8 * strlen_color(buf) * scale->value;
			height = 8 * scale->value;

			if(HUD_PrepareDraw(hud, width, height, &x, &y))
			{
				Draw_SString(x, y, buf, scale->value);
			}
			break;
	}
}

void SCR_HUD_DrawBarArmor(hud_t *hud)
{
	static	cvar_t *width = NULL, *height, *direction, *color_noarmor, *color_ga, *color_ya, *color_ra, *color_unnatural;
	int		x, y;
	int		armor = HUD_Stats(STAT_ARMOR);
	qbool	alive = cl.stats[STAT_HEALTH] > 0;
	
	if (width == NULL)  // first time called
	{
		width			= HUD_FindVar(hud, "width");
		height			= HUD_FindVar(hud, "height");
		direction		= HUD_FindVar(hud, "direction");
		color_noarmor	= HUD_FindVar(hud, "color_noarmor");
		color_ga		= HUD_FindVar(hud, "color_ga");
		color_ya		= HUD_FindVar(hud, "color_ya");
		color_ra		= HUD_FindVar(hud, "color_ra");
		color_unnatural	= HUD_FindVar(hud, "color_unnatural");
	}
	
	if(HUD_PrepareDraw(hud, width->ival, height->ival, &x, &y))
	{
		if(!width->ival || !height->ival)
			return;

		if(HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY && alive)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_unnatural->vec4, x, y, width->ival, height->ival);
		}
		else  if (HUD_Stats(STAT_ITEMS) & IT_ARMOR3 && alive)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_noarmor->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, armor, 200.0, color_ra->vec4, x, y, width->ival, height->ival);
		}
		else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR2 && alive)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_noarmor->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, armor, 150.0, color_ya->vec4, x, y, width->ival, height->ival);
		}
		else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR1 && alive)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_noarmor->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, armor, 100.0, color_ga->vec4, x, y, width->ival, height->ival);
		}
		else
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_noarmor->vec4, x, y, width->ival, height->ival);
		}
	}
}

void SCR_HUD_DrawBarHealth(hud_t *hud)
{
	static	cvar_t *width = NULL, *height, *direction, *color_nohealth, *color_normal, *color_mega, *color_twomega, *color_unnatural;
	int		x, y;
	int		health = cl.stats[STAT_HEALTH];

	if (width == NULL)  // first time called
	{
		width			= HUD_FindVar(hud, "width");
		height			= HUD_FindVar(hud, "height");
		direction		= HUD_FindVar(hud, "direction");
		color_nohealth	= HUD_FindVar(hud, "color_nohealth");
		color_normal	= HUD_FindVar(hud, "color_normal");
		color_mega		= HUD_FindVar(hud, "color_mega");
		color_twomega	= HUD_FindVar(hud, "color_twomega");
		color_unnatural	= HUD_FindVar(hud, "color_unnatural");
	}

	if(HUD_PrepareDraw(hud, width->ival, height->ival, &x, &y))
	{
		if(!width->ival || !height->ival)
			return;

		if(health > 250)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_unnatural->vec4, x, y, width->ival, height->ival);
		}
		else if(health > 200)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_normal->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_mega->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, health - 200, 100.0, color_twomega->vec4, x, y, width->ival, height->ival);
		}
		else if(health > 100)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_normal->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, health - 100, 100.0, color_mega->vec4, x, y, width->ival, height->ival);
		}
		else if(health > 0)
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_nohealth->vec4, x, y, width->ival, height->ival);
			SCR_HUD_DrawBar(direction->ival, health, 100.0, color_normal->vec4, x, y, width->ival, height->ival);
		}
		else
		{
			SCR_HUD_DrawBar(direction->ival, 100, 100.0, color_nohealth->vec4, x, y, width->ival, height->ival);
		}
	}
}

void SCR_HUD_DrawOwnFrags(hud_t *hud)
{
	// not implemented yet: scale, color
	// fixme: add appropriate opengl functions that will add alpha, scale and color
	char ownfragtext[256];
	float age;
	int width;
	int height = 8;
	int x, y;
	double alpha;
	static cvar_t
		*hud_ownfrags_timeout = NULL,
		*hud_ownfrags_scale = NULL;
//		*hud_ownfrags_color = NULL;
	extern qbool hud_editor;

	if (hud_ownfrags_timeout == NULL)    // first time
	{
		hud_ownfrags_scale				= HUD_FindVar(hud, "scale");
		// hud_ownfrags_color			= HUD_FindVar(hud, "color");
		hud_ownfrags_timeout			= HUD_FindVar(hud, "timeout");
	}

	if (hud_editor)
	{
		strcpy(ownfragtext, "Own Frags");
		age = 0;
	}
	else if (clientfuncs->GetTrackerOwnFrags)
		age = clientfuncs->GetTrackerOwnFrags(0, ownfragtext, sizeof(ownfragtext));
	else
		age = 999999;
	width = strlen(ownfragtext)*8;

	width *= hud_ownfrags_scale->value;
	height *= hud_ownfrags_scale->value;

	if (age >= hud_ownfrags_timeout->value)
		width = 0;

	alpha = 2 - age / hud_ownfrags_timeout->value * 2;
	alpha = bound(0, alpha, 1);

	if (!width)
	{
		HUD_PrepareDraw(hud, width, height, NULL, NULL);
		return;
	}
	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
		return;

	drawfuncs->Colour4f(1, 1, 1, alpha);
	Draw_SString(x, y, ownfragtext, hud_ownfrags_scale->value);
	drawfuncs->Colour4f(1, 1, 1, 1);
}

#ifdef QUAKEHUD
static struct wstats_s *findweapon(struct wstats_s *w, size_t wc, char *wn)
{
	for (; wc>0; wc--, w++)
	{
		if (!strcmp(wn, w->wname))
			return w;
	}
	return NULL;
}
static void SCR_HUD_DrawWeaponStats(hud_t *hud)
{
	char line[1024], *o, *i;
	int width;
	int height = 8;
	int x, y;
	static cvar_t *hud_weaponstats_scale = NULL;
	static cvar_t *hud_weaponstats_fmt = NULL;
	extern qbool hud_editor;

	int ws;
	struct wstats_s wstats[16];
	ws = clientfuncs->GetWeaponStats?clientfuncs->GetWeaponStats(-1, wstats, countof(wstats)):0;

	if (hud_editor)
	{
		ws = 0;
		strcpy(wstats[ws].wname, "axe");
		wstats[ws].hit = 20;
		wstats[ws].total = 100;
		ws++;
		strcpy(wstats[ws].wname, "rl");
		wstats[ws].hit = 60;
		wstats[ws].total = 120;
		ws++;
		strcpy(wstats[ws].wname, "lg");
		wstats[ws].hit = 20;
		wstats[ws].total = 100;
		ws++;
	}

	if (hud_weaponstats_scale == NULL)    // first time
	{
		hud_weaponstats_scale				= HUD_FindVar(hud, "scale");
		hud_weaponstats_fmt					= HUD_FindVar(hud, "fmt");
//		"&c990sg&r:[%sg] &c099ssg&r:[%ssg] &c900rl&r:[#rl] &c009lg&r:[%lg]"
	}

	height = 8;
	for (o = line, i = hud_weaponstats_fmt->string; ws && *i && o < line+countof(line)-1; )
	{
		if (i[0] == '[' && (i[1] == '%' || i[1] == '#'))
		{
			struct wstats_s *w;
			char wname[16];
			int pct = i[1]=='%', j;
			i+=2;
			for (j = 0; *i && j < countof(wname)-1; j++)
			{
				if (*i == ']')
				{
					i++;
					break;
				}
				wname[j] = *i++;
			}
			wname[j] = 0;
			w = findweapon(wstats, ws, wname);
			if (pct && w && w->total)
				snprintf(wname, sizeof(wname), "%.1f", (100.0 * w->hit) / w->total);
			else if (pct)
				snprintf(wname, sizeof(wname), "%.1f", 0.0);
			else if (w)
				snprintf(wname, sizeof(wname), "%u", w->hit);
			else
				snprintf(wname, sizeof(wname), "%u", 0);

			for (j = 0; wname[j] && o < line+countof(line)-1; j++)
				*o++ = wname[j];
		}
		else if (*i == '\n')
		{
			height += 8;
			*o++ = *i++;
		}
		else
			*o++ = *i++;
	}
	*o++ = 0;

	width = 8*strlen_color(line);

	width *= hud_weaponstats_scale->value;
	height *= hud_weaponstats_scale->value;

	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
		return;

	Draw_SString(x, y, line, hud_weaponstats_scale->value);
}
#endif

void SCR_HUD_DrawKeys(hud_t *hud)
{
	char line1[32], line2[32];
	int width, height, x, y;
	usercmd_t b;
	static cvar_t* vscale = NULL;
	float scale;

	memset(&b, 0, sizeof(b));
	clientfuncs->GetLastInputFrame(0, &b);

	if (!vscale) {
		vscale = HUD_FindVar(hud, "scale");
	}

	scale = vscale->value;
	scale = max(0, scale);

	snprintf(line1, sizeof(line1), "^{%x}^{%x}^{%x}", 
		0xe000 + 'x' + ((b.buttons & 1)?0x80:0),
		0xe000 + '^' + ((b.forwardmove > 0)?0x80:0),
		0xe000 + 'J' + ((b.buttons & 2)?0x80:0));
	snprintf(line2, sizeof(line2), "^{%x}^{%x}^{%x}", 
		0xe000 + '<' + ((b.sidemove < 0)?0x80:0),
		0xe000 + '_' + ((b.forwardmove < 0)?0x80:0),
		0xe000 + '>' + ((b.sidemove > 0)?0x80:0));

	width = 8 * 3 * scale;
	height = 8 * 2 * scale;

	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
		return;

	Draw_SString(x, y, line1, scale);
	Draw_SString(x, y + 8*scale, line2, scale);
}

#ifdef WITH_PNG
// What stats to draw.
#define HUD_RADAR_STATS_NONE				0
#define HUD_RADAR_STATS_BOTH_TEAMS_HOLD		1
#define HUD_RADAR_STATS_TEAM1_HOLD			2
#define HUD_RADAR_STATS_TEAM2_HOLD			3
#define HUD_RADAR_STATS_BOTH_TEAMS_DEATHS	4
#define HUD_RADAR_STATS_TEAM1_DEATHS		5
#define HUD_RADAR_STATS_TEAM2_DEATHS		6

void Radar_DrawGrid(stats_weight_grid_t *grid, int x, int y, float scale, int pic_width, int pic_height, int style)
{
	int row, col;

	// Don't try to draw anything if we got no data.
	if(grid == NULL || grid->cells == NULL || style == HUD_RADAR_STATS_NONE)
	{
		return;
	}

	// Go through all the cells and draw them based on their weight.
	for(row = 0; row < grid->row_count; row++)
	{
		// Just to be safe if something went wrong with the allocation.
		if(grid->cells[row] == NULL)
		{
			continue;
		}

		for(col = 0; col < grid->col_count; col++)
		{
			float weight = 0.0;
			int color = 0;

			float tl_x, tl_y;			// The pixel coordinate of the top left corner of a grid cell.
			float p_cell_length_x;		// The pixel length of a cell.
			float p_cell_length_y;		// The pixel "length" on the Y-axis. We calculate this
										// seperatly because we'll get errors when converting from
										// quake coordinates -> pixel coordinates.

			// Calculate the pixel coordinates of the top left corner of the current cell.
			// (This is times 8 because the conversion formula was calculated from a .loc-file)
			tl_x = (map_x_slope * (8.0 * grid->cells[row][col].tl_x) + map_x_intercept) * scale;
			tl_y = (map_y_slope * (8.0 * grid->cells[row][col].tl_y) + map_y_intercept) * scale;

			// Calculate the cell length in pixel length.
			p_cell_length_x = map_x_slope*(8.0 * grid->cell_length) * scale;
			p_cell_length_y = map_y_slope*(8.0 * grid->cell_length) * scale;

			// Add rounding errors (so that we don't get weird gaps in the grid).
			p_cell_length_x += tl_x - Q_rint(tl_x);
			p_cell_length_y += tl_y - Q_rint(tl_y);

			// Don't draw the stats stuff outside the picture.
			if(tl_x + p_cell_length_x > pic_width || tl_y + p_cell_length_y > pic_height || x + tl_x < x || y + tl_y < y)
			{
				continue;
			}

			//
			// Death stats.
			//
			if(grid->cells[row][col].teams[STATS_TEAM1].death_weight + grid->cells[row][col].teams[STATS_TEAM2].death_weight > 0)
			{
				weight = 0;

				if(style == HUD_RADAR_STATS_BOTH_TEAMS_DEATHS || style == HUD_RADAR_STATS_TEAM1_DEATHS)
				{
					weight = grid->cells[row][col].teams[STATS_TEAM1].death_weight;
				}

				if(style == HUD_RADAR_STATS_BOTH_TEAMS_DEATHS || style == HUD_RADAR_STATS_TEAM2_DEATHS)
				{
					weight += grid->cells[row][col].teams[STATS_TEAM2].death_weight;
				}

				color = 79;
			}

			//
			// Team stats.
			//
			{
				// No point in drawing if we have no weight.
				if(grid->cells[row][col].teams[STATS_TEAM1].weight + grid->cells[row][col].teams[STATS_TEAM2].weight <= 0
					&& (style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD
					||	style == HUD_RADAR_STATS_TEAM1_HOLD
					||	style == HUD_RADAR_STATS_TEAM2_HOLD))
				{
					continue;
				}

				// Get the team with the highest weight for this cell.
				if(grid->cells[row][col].teams[STATS_TEAM1].weight > grid->cells[row][col].teams[STATS_TEAM2].weight
					&& (style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD
					||	style == HUD_RADAR_STATS_TEAM1_HOLD))
				{
					weight = grid->cells[row][col].teams[STATS_TEAM1].weight;
					color = stats_grid->teams[STATS_TEAM1].color;
				}
				else if(style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD ||	style == HUD_RADAR_STATS_TEAM2_HOLD)
				{
					weight = grid->cells[row][col].teams[STATS_TEAM2].weight;
					color = stats_grid->teams[STATS_TEAM2].color;
				}
			}

			// Draw the cell in the color of the team with the
			// biggest weight for this cell. Or draw deaths.
			Draw_AlphaFill(
				x + Q_rint(tl_x),			// X.
				y + Q_rint(tl_y),			// Y.
				Q_rint(p_cell_length_x),		// Width.
				Q_rint(p_cell_length_y),		// Height.
				color,						// Color.
				weight);					// Alpha.
		}
	}
}

// The skinnum property in the entity_s structure is used
// for determening what type of armor to draw on the radar.
#define HUD_RADAR_GA					0
#define HUD_RADAR_YA					1
#define HUD_RADAR_RA					2

// Radar filters.
#define RADAR_SHOW_WEAPONS (radar_show_ssg || radar_show_ng || radar_show_sng || radar_show_gl || radar_show_rl || radar_show_lg)
static qbool radar_show_ssg			= false;
static qbool radar_show_ng			= false;
static qbool radar_show_sng			= false;
static qbool radar_show_gl			= false;
static qbool radar_show_rl			= false;
static qbool radar_show_lg			= false;

#define RADAR_SHOW_ITEMS (radar_show_backpacks || radar_show_health || radar_show_ra || radar_show_ya || radar_show_ga || radar_show_rockets || radar_show_nails || radar_show_cells || radar_show_shells || radar_show_quad || radar_show_pent || radar_show_ring || radar_show_suit)
static qbool radar_show_backpacks	= false;
static qbool radar_show_health		= false;
static qbool radar_show_ra			= false;
static qbool radar_show_ya			= false;
static qbool radar_show_ga			= false;
static qbool radar_show_rockets		= false;
static qbool radar_show_nails		= false;
static qbool radar_show_cells		= false;
static qbool radar_show_shells		= false;
static qbool radar_show_quad		= false;
static qbool radar_show_pent		= false;
static qbool radar_show_ring		= false;
static qbool radar_show_suit		= false;
static qbool radar_show_mega		= false;

#define RADAR_SHOW_OTHER (radar_show_gibs || radar_show_explosions || radar_show_nails_p || radar_show_rockets_p || radar_show_shaft_p || radar_show_teleport || radar_show_shotgun)
static qbool radar_show_nails_p		= false;
static qbool radar_show_rockets_p	= false;
static qbool radar_show_shaft_p		= false;
static qbool radar_show_gibs		= false;
static qbool radar_show_explosions	= false;
static qbool radar_show_teleport	= false;
static qbool radar_show_shotgun		= false;

void Radar_OnChangeWeaponFilter(cvar_t *var, char *oldval)
{
	// Parse the weapon filter.
	radar_show_ssg		= Utils_RegExpMatch("SSG|SUPERSHOTGUN|ALL",			var->string);
	radar_show_ng		= Utils_RegExpMatch("([^S]|^)NG|NAILGUN|ALL",		var->string); // Yes very ugly, but we don't want to match SNG.
	radar_show_sng		= Utils_RegExpMatch("SNG|SUPERNAILGUN|ALL",			var->string);
	radar_show_rl		= Utils_RegExpMatch("RL|ROCKETLAUNCHER|ALL",		var->string);
	radar_show_gl		= Utils_RegExpMatch("GL|GRENADELAUNCHER|ALL",		var->string);
	radar_show_lg		= Utils_RegExpMatch("LG|SHAFT|LIGHTNING|ALL",		var->string);
}

void Radar_OnChangeItemFilter(cvar_t *var, char *oldval)
{
	// Parse the item filter.
	radar_show_backpacks		= Utils_RegExpMatch("BP|BACKPACK|ALL",					var->string);
	radar_show_health			= Utils_RegExpMatch("HP|HEALTH|ALL",					var->string);
	radar_show_ra				= Utils_RegExpMatch("RA|REDARMOR|ARMOR|ALL",			var->string);
	radar_show_ya				= Utils_RegExpMatch("YA|YELLOWARMOR|ARMOR|ALL",			var->string);
	radar_show_ga				= Utils_RegExpMatch("GA|GREENARMOR|ARMOR|ALL",			var->string);
	radar_show_rockets			= Utils_RegExpMatch("ROCKETS|ROCKS|AMMO|ALL",			var->string);
	radar_show_nails			= Utils_RegExpMatch("NAILS|SPIKES|AMMO|ALL",			var->string);
	radar_show_cells			= Utils_RegExpMatch("CELLS|BATTERY|AMMO|ALL",			var->string);
	radar_show_shells			= Utils_RegExpMatch("SHELLS|AMMO|ALL",					var->string);
	radar_show_quad				= Utils_RegExpMatch("QUAD|POWERUPS|ALL",				var->string);
	radar_show_pent				= Utils_RegExpMatch("PENT|PENTAGRAM|666|POWERUPS|ALL",	var->string);
	radar_show_ring				= Utils_RegExpMatch("RING|INVISIBLE|EYES|POWERUPS|ALL",	var->string);
	radar_show_suit				= Utils_RegExpMatch("SUIT|POWERUPS|ALL",				var->string);
	radar_show_mega				= Utils_RegExpMatch("MH|MEGA|MEGAHEALTH|100\\+|ALL",	var->string);
}

void Radar_OnChangeOtherFilter(cvar_t *var, char *oldval)
{
	// Parse the "other" filter.
	radar_show_nails_p			= Utils_RegExpMatch("NAILS|PROJECTILES|ALL",	var->string);
	radar_show_rockets_p		= Utils_RegExpMatch("ROCKETS|PROJECTILES|ALL",	var->string);
	radar_show_shaft_p			= Utils_RegExpMatch("SHAFT|PROJECTILES|ALL",	var->string);
	radar_show_gibs				= Utils_RegExpMatch("GIBS|ALL",					var->string);
	radar_show_explosions		= Utils_RegExpMatch("EXPLOSIONS|ALL",			var->string);
	radar_show_teleport			= Utils_RegExpMatch("TELE|ALL",					var->string);
	radar_show_shotgun			= Utils_RegExpMatch("SHOTGUN|SG|BUCK|ALL",		var->string);
}


#define HUD_COLOR_DEFAULT_TRANSPARENCY	75

byte hud_radar_highlight_color[4] = {255, 255, 0, HUD_COLOR_DEFAULT_TRANSPARENCY};

void Radar_OnChangeHighlightColor(cvar_t *var, char *newval, qbool *cancel)
{
	char *new_color;
	char buf[MAX_COM_TOKEN];

	// Translate a colors name to RGB values.
	new_color = ColorNameToRGBString(newval);

	// Parse the colors.
	//color = StringToRGB(new_color);
	strlcpy(buf,new_color,sizeof(buf));
	memcpy(hud_radar_highlight_color, StringToRGB(buf), sizeof(byte) * 4);

	// Set the cvar to contain the new color string
	// (if the user entered "red" it will be "255 0 0").
	Cvar_Set(var, new_color);
}

void Radar_DrawEntities(int x, int y, float scale, float player_size, int show_hold_areas)
{
	int i;

	// Entities (weapons and such). cl_main.c
	extern visentlist_t cl_visents;

	// Go through all the entities and draw the ones we're supposed to.
	for (i = 0; i < cl_visents.count; i++)
	{
		int entity_q_x = 0;
		int entity_q_y = 0;
		int entity_p_x = 0;
		int entity_p_y = 0;

		// Get quake coordinates (times 8 to get them in the same format as .locs).
		entity_q_x = cl_visents.list[i].origin[0]*8;
		entity_q_y = cl_visents.list[i].origin[1]*8;

		// Convert from quake coordiantes -> pixel coordinates.
		entity_p_x = x + Q_rint((map_x_slope*entity_q_x + map_x_intercept) * scale);
		entity_p_y = y + Q_rint((map_y_slope*entity_q_y + map_y_intercept) * scale);

		// TODO: Replace all model name comparison below with MOD_HINT's instead for less comparisons (create new ones in Mod_LoadAliasModel() in r_model.c and gl_model.c/.h for the ones that don't have one already).

		//
		// Powerups.
		//

		if(radar_show_pent && !strcmp(cl_visents.list[i].model->name, "progs/invulner.mdl"))
		{
			// Pentagram.
			Draw_ColoredString(entity_p_x, entity_p_y, "&cf00P", 0);
		}
		else if(radar_show_quad && !strcmp(cl_visents.list[i].model->name, "progs/quaddama.mdl"))
		{
			// Quad.
			Draw_ColoredString(entity_p_x, entity_p_y, "&c0ffQ", 0);
		}
		else if(radar_show_ring && !strcmp(cl_visents.list[i].model->name, "progs/invisibl.mdl"))
		{
			// Ring.
			Draw_ColoredString(entity_p_x, entity_p_y, "&cff0R", 0);
		}
		else if(radar_show_suit && !strcmp(cl_visents.list[i].model->name, "progs/suit.mdl"))
		{
			// Suit.
			Draw_ColoredString(entity_p_x, entity_p_y, "&c0f0S", 0);
		}

		//
		// Show RL, LG and backpacks.
		//
		if(radar_show_rl && !strcmp(cl_visents.list[i].model->name, "progs/g_rock2.mdl"))
		{
			// RL.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "RL");
		}
		else if(radar_show_lg && !strcmp(cl_visents.list[i].model->name, "progs/g_light.mdl"))
		{
			// LG.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "LG");
		}
		else if(radar_show_backpacks && cl_visents.list[i].model->modhint == MOD_BACKPACK)
		{
			// Back packs.
			float back_pack_size = 0;

			back_pack_size = max(player_size * 0.5, 0.05);

			Draw_AlphaCircleFill (entity_p_x, entity_p_y, back_pack_size, 114, 1);
			Draw_AlphaCircleOutline (entity_p_x, entity_p_y, back_pack_size, 1.0, 0, 1);
		}

		if(!strcmp(cl_visents.list[i].model->name, "progs/armor.mdl"))
		{
			//
			// Show armors.
			//

			if(radar_show_ga && cl_visents.list[i].skinnum == HUD_RADAR_GA)
			{
				// GA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 178, 1.0);
			}
			else if(radar_show_ya && cl_visents.list[i].skinnum == HUD_RADAR_YA)
			{
				// YA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 192, 1.0);
			}
			else if(radar_show_ra && cl_visents.list[i].skinnum == HUD_RADAR_RA)
			{
				// RA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 251, 1.0);
			}

			Draw_AlphaCircleOutline (entity_p_x, entity_p_y, 3.0, 1.0, 0, 1.0);
		}

		if(radar_show_mega && !strcmp(cl_visents.list[i].model->name, "maps/b_bh100.bsp"))
		{
			//
			// Show megahealth.
			//

			// Draw a red border around the cross.
			Draw_AlphaRectangleRGB(entity_p_x - 3, entity_p_y - 3, 8, 8, 1, false, RGBA_TO_COLOR(200, 0, 0, 200));

			// Draw a black outline cross.
			Draw_AlphaFill(entity_p_x - 3, entity_p_y - 1, 8, 4, 0, 1);
			Draw_AlphaFill(entity_p_x - 1, entity_p_y - 3, 4, 8, 0, 1);

			// Draw a 2 pixel cross.
			Draw_AlphaFill(entity_p_x - 2, entity_p_y, 6, 2, 79, 1);
			Draw_AlphaFill(entity_p_x, entity_p_y - 2, 2, 6, 79, 1);
		}

		if(radar_show_ssg && !strcmp(cl_visents.list[i].model->name, "progs/g_shot.mdl"))
		{
			// SSG.
			Draw_String(entity_p_x - (3*8)/2, entity_p_y - 4, "SSG");
		}
		else if(radar_show_ng && !strcmp(cl_visents.list[i].model->name, "progs/g_nail.mdl"))
		{
			// NG.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "NG");
		}
		else if(radar_show_sng && !strcmp(cl_visents.list[i].model->name, "progs/g_nail2.mdl"))
		{
			// SNG.
			Draw_String(entity_p_x - (3*8)/2, entity_p_y - 4, "SNG");
		}
		else if(radar_show_gl && !strcmp(cl_visents.list[i].model->name, "progs/g_rock.mdl"))
		{
			// GL.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "GL");
		}

		if(radar_show_gibs
			&&(!strcmp(cl_visents.list[i].model->name, "progs/gib1.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/gib2.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/gib3.mdl")))
		{
			//
			// Gibs.
			//

			Draw_AlphaCircleFill(entity_p_x, entity_p_y, 2.0, 251, 1);
		}

		if(radar_show_health
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_bh25.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_bh10.bsp")))
		{
			//
			// Health.
			//

			// Draw a black outline cross.
			Draw_AlphaFill (entity_p_x - 3, entity_p_y - 1, 7, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 3, 3, 7, 0, 1);

			// Draw a cross.
			Draw_AlphaFill (entity_p_x - 2, entity_p_y, 5, 1, 79, 1);
			Draw_AlphaFill (entity_p_x, entity_p_y - 2, 1, 5, 79, 1);
		}

		//
		// Ammo.
		//
		if(radar_show_rockets
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_rock0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_rock1.bsp")))
		{
			//
			// Rockets.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 6, 3, 5, 0, 1);
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 1, 5, 5, 0, 1);

			// The brown rocket.
			Draw_AlphaFill (entity_p_x, entity_p_y - 5, 1, 5, 120, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y, 1, 3, 120, 1);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y, 1, 3, 120, 1);
		}

		if(radar_show_cells
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_batt0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_batt1.bsp")))
		{
			//
			// Cells.
			//

			// Draw a black outline.
			Draw_AlphaLine(entity_p_x - 3, entity_p_y, entity_p_x + 4, entity_p_y - 5, 3, 0, 1);
			Draw_AlphaLine(entity_p_x - 3, entity_p_y, entity_p_x + 3 , entity_p_y, 3, 0, 1);
			Draw_AlphaLine(entity_p_x + 3, entity_p_y, entity_p_x - 3, entity_p_y + 4, 3, 0, 1);

			// Draw a yellow lightning!
			Draw_AlphaLine(entity_p_x - 2, entity_p_y, entity_p_x + 3, entity_p_y - 4, 1, 111, 1);
			Draw_AlphaLine(entity_p_x - 2, entity_p_y, entity_p_x + 2 , entity_p_y, 1, 111, 1);
			Draw_AlphaLine(entity_p_x + 2, entity_p_y, entity_p_x - 2, entity_p_y + 3, 1, 111, 1);
		}

		if(radar_show_nails
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_nail0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_nail1.bsp")))
		{
			//
			// Nails.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 3, entity_p_y - 3, 7, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 2, 5, 3, 0, 0.5);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y, 3, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y + 3, 1, 1, 0, 0.5);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y + 3, 1, 1, 0, 0.5);
			Draw_AlphaFill (entity_p_x, entity_p_y + 4, 1, 1, 0, 1);

			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 2, 5, 1, 6, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 1, 3, 1, 6, 0.5);
			Draw_AlphaFill (entity_p_x, entity_p_y, 1, 4, 6, 1);
		}

		if(radar_show_shells
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_shell0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_shell1.bsp")))
		{
			//
			// Shells.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 3, 5, 9, 0, 1);

			// Draw 2 shotgun shells.
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 2, 1, 4, 73, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 2 + 5, 1, 2, 104, 1);

			Draw_AlphaFill (entity_p_x + 1, entity_p_y - 2, 1, 4, 73, 1);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y - 2 + 5, 1, 2, 104, 1);
		}

		//
		// Show projectiles (rockets, grenades, nails, shaft).
		//

		if(radar_show_nails_p
			&& (!strcmp(cl_visents.list[i].model->name, "progs/s_spike.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/spike.mdl")))
		{
			//
			// Spikes from SNG and NG.
			//

			Draw_AlphaFill(entity_p_x, entity_p_y, 1, 1, 254, 1);
		}
		else if(radar_show_rockets_p
			&& (!strcmp(cl_visents.list[i].model->name, "progs/missile.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/grenade.mdl")))
		{
			//
			// Rockets and grenades.
			//

			float entity_angle = 0;
			int x_line_end = 0;
			int y_line_end = 0;

			// Get the entity angle in radians.
			entity_angle = DEG2RAD(cl_visents.list[i].angles[1]);

			x_line_end = entity_p_x + 5 * cos(entity_angle) * scale;
			y_line_end = entity_p_y - 5 * sin(entity_angle) * scale;

			// Draw the rocket/grenade showing it's angle also.
			Draw_AlphaLine (entity_p_x, entity_p_y, x_line_end, y_line_end, 1.0, 254, 1);
		}
		else if(radar_show_shaft_p
			&& (!strcmp(cl_visents.list[i].model->name, "progs/bolt.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/bolt2.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/bolt3.mdl")))
		{
			//
			// Shaft beam.
			//

			float entity_angle = 0;
			float shaft_length = 0;
			float x_line_end = 0;
			float y_line_end = 0;

			// Get the length and angle of the shaft.
			shaft_length = cl_visents.list[i].model->maxs[1];
			entity_angle = (cl_visents.list[i].angles[1]*M_PI)/180;

			// Calculate where the shaft beam's ending point.
			x_line_end = entity_p_x + shaft_length * cos(entity_angle);
			y_line_end = entity_p_y - shaft_length * sin(entity_angle);

			// Draw the shaft beam.
			Draw_AlphaLine (entity_p_x, entity_p_y, x_line_end, y_line_end, 1.0, 254, 1);
		}
	}

	// Draw a circle around "hold areas", the grid cells within this circle
	// are the ones that are counted for that particular hold area. The team
	// that has the most percentage of these cells is considered to hold that area.
	if(show_hold_areas && stats_important_ents != NULL && stats_important_ents->list != NULL)
	{
		int entity_p_x = 0;
		int entity_p_y = 0;

		for(i = 0; i < stats_important_ents->count; i++)
		{
			entity_p_x = x + Q_rint((map_x_slope*8*stats_important_ents->list[i].origin[0] + map_x_intercept) * scale);
			entity_p_y = y + Q_rint((map_y_slope*8*stats_important_ents->list[i].origin[1] + map_y_intercept) * scale);

			Draw_ColoredString(entity_p_x  - (8 * strlen(stats_important_ents->list[i].name)) / 2.0, entity_p_y - 4,
				va("&c55f%s", stats_important_ents->list[i].name), 0);

			Draw_AlphaCircleOutline(entity_p_x , entity_p_y, map_x_slope * 8 * stats_important_ents->hold_radius * scale, 1.0, 15, 0.2);
		}
	}

	//
	// Draw temp entities (explosions, blood, teleport effects).
	//
	for(i = 0; i < MAX_TEMP_ENTITIES; i++)
	{
		float time_diff = 0.0;

		int entity_q_x = 0;
		int entity_q_y = 0;
		int entity_p_x = 0;
		int entity_p_y = 0;

		// Get the time since the entity spawned.
		if(cls.demoplayback)
		{
			time_diff = cls.demotime - temp_entities.list[i].time;
		}
		else
		{
			time_diff = cls.realtime - temp_entities.list[i].time;
		}

		// Don't show temp entities for long.
		if(time_diff < 0.25)
		{
			float radius = 0.0;
			radius = (time_diff < 0.125) ? (time_diff * 32.0) : (time_diff * 32.0) - time_diff;
			radius *= scale;
			radius = min(max(radius, 0), 200);

			// Get quake coordinates (times 8 to get them in the same format as .locs).
			entity_q_x = temp_entities.list[i].pos[0]*8;
			entity_q_y = temp_entities.list[i].pos[1]*8;

			entity_p_x = x + Q_rint((map_x_slope*entity_q_x + map_x_intercept) * scale);
			entity_p_y = y + Q_rint((map_y_slope*entity_q_y + map_y_intercept) * scale);

			if(radar_show_explosions
				&& (temp_entities.list[i].type == TE_EXPLOSION
				|| temp_entities.list[i].type == TE_TAREXPLOSION))
			{
				//
				// Explosions.
				//

				Draw_AlphaCircleFill (entity_p_x, entity_p_y, radius, 235, 0.8);
			}
			else if(radar_show_teleport && temp_entities.list[i].type == TE_TELEPORT)
			{
				//
				// Teleport effect.
				//

				radius *= 1.5;
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, radius, 244, 0.8);
			}
			else if(radar_show_shotgun && temp_entities.list[i].type == TE_GUNSHOT)
			{
				//
				// Shotgun fire.
				//

				#define SHOTGUN_SPREAD 10
				int spread_x = 0;
				int spread_y = 0;
				int n = 0;

				for(n = 0; n < 10; n++)
				{
					spread_x = (int)(rand() / (((double)RAND_MAX + 1) / SHOTGUN_SPREAD));
					spread_y = (int)(rand() / (((double)RAND_MAX + 1) / SHOTGUN_SPREAD));

					Draw_AlphaFill (entity_p_x + spread_x - (SHOTGUN_SPREAD/2), entity_p_y + spread_y - (SHOTGUN_SPREAD/2), 1, 1, 8, 0.9);
				}
			}
		}
	}
}

void Radar_DrawPlayers(int x, int y, int width, int height, float scale,
					   float show_height, float show_powerups,
					   float player_size, float show_names,
					   float fade_players, float highlight,
					   char *highlight_color)
{
	int i;
	player_state_t *state;
	player_info_t *info;

	// Get player state so we can know where he is (or on rare occassions, she).
	state = cl.frames[cl.oldparsecount & UPDATE_MASK].playerstate;

	// Get the info for the player.
	info = cl.players;

	//
	// Draw the players.
	//
	for (i = 0; i < MAX_CLIENTS; i++, info++, state++)
	{
		// Players quake coordinates
		// (these are multiplied by 8, since the conversion formula was
		// calculated using the coordinates in a .loc-file, which are in
		// the format quake-coordainte*8).
		int player_q_x = 0;
		int player_q_y = 0;

		// The height of the player.
		float player_z = 1.0;
		float player_z_relative = 1.0;

		// Players pixel coordinates.
		int player_p_x = 0;
		int player_p_y = 0;

		// Used for drawing the the direction the
		// player is looking at.
		float player_angle = 0;
		int x_line_start = 0;
		int y_line_start = 0;
		int x_line_end = 0;
		int y_line_end = 0;

		// Color and opacity of the player.
		int player_color = 0;
		float player_alpha = 1.0;

		// Make sure we're not drawing any ghosts.
		if(!info->name[0])
		{
			continue;
		}

		if (state->messagenum == cl.oldparsecount)
		{
			// TODO: Implement lerping to get smoother drawing.

			// Get the quake coordinates. Multiply by 8 since
			// the conversion formula has been calculated using
			// a .loc-file which is in that format.
			player_q_x = state->origin[0]*8;
			player_q_y = state->origin[1]*8;

			// Get the players view angle.
			player_angle = cls.demoplayback ? state->viewangles[1] : cl.simangles[1];

			// Convert from quake coordiantes -> pixel coordinates.
			player_p_x = Q_rint((map_x_slope*player_q_x + map_x_intercept) * scale);
			player_p_y = Q_rint((map_y_slope*player_q_y + map_y_intercept) * scale);

			player_color = Sbar_BottomColor(info);

			// Calculate the height of the player.
			if(show_height)
			{
				player_z = state->origin[2];
				player_z += (player_z >= 0) ? fabs(cl.worldmodel->mins[2]) : fabs(cl.worldmodel->maxs[2]);
				player_z_relative = min(fabs(player_z / map_height_diff), 1.0);
				player_z_relative = max(player_z_relative, 0.2);
			}

			// Make the players fade out as they get less armor/health.
			if(fade_players)
			{
				int armor_strength = 0;
				armor_strength = (info->stats[STAT_ITEMS] & IT_ARMOR1) ? 100 :
					((info->stats[STAT_ITEMS] & IT_ARMOR2) ? 150 :
					((info->stats[STAT_ITEMS] & IT_ARMOR3) ? 200 : 0));

				// Don't let the players get completly transparent so add 0.2 to the final value.
				player_alpha = ((info->stats[STAT_HEALTH] + (info->stats[STAT_ARMOR] * armor_strength)) / 100.0) + 0.2;
			}

			// Turn dead people red.
			if(info->stats[STAT_HEALTH] <= 0)
			{
				player_alpha = 1.0;
				player_color = 79;
			}

			// Draw a ring around players with powerups if it's enabled.
			if(show_powerups)
			{
				if(info->stats[STAT_ITEMS] & IT_INVISIBILITY)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 161, 0.2);
				}

				if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 79, 0.5);
				}

				if(info->stats[STAT_ITEMS] & IT_QUAD)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 244, 0.2);
				}
			}

			#define HUD_RADAR_HIGHLIGHT_NONE			0
			#define HUD_RADAR_HIGHLIGHT_TEXT_ONLY		1
			#define HUD_RADAR_HIGHLIGHT_OUTLINE			2
			#define HUD_RADAR_HIGHLIGHT_FIXED_OUTLINE	3
			#define HUD_RADAR_HIGHLIGHT_CIRCLE			4
			#define HUD_RADAR_HIGHLIGHT_FIXED_CIRCLE	5
			#define HUD_RADAR_HIGHLIGHT_ARROW_BOTTOM	6
			#define HUD_RADAR_HIGHLIGHT_ARROW_CENTER	7
			#define HUD_RADAR_HIGHLIGHT_ARROW_TOP		8
			#define HUD_RADAR_HIGHLIGHT_CROSS_CORNERS	9

			// Draw a circle around the tracked player.
			if (highlight != HUD_RADAR_HIGHLIGHT_NONE && Cam_TrackNum() >= 0 && info->userid == cl.players[Cam_TrackNum()].userid)
			{
				color_t higlight_color = RGBAVECT_TO_COLOR(hud_radar_highlight_color);

				// Draw the highlight.
				switch ((int)highlight)
				{
					case HUD_RADAR_HIGHLIGHT_CROSS_CORNERS :
					{
						// Top left
						Draw_AlphaLineRGB (x, y, x + player_p_x, y + player_p_y, 2, higlight_color);

						// Top right.
						Draw_AlphaLineRGB (x + width, y, x + player_p_x, y + player_p_y, 2, higlight_color);

						// Bottom left.
						Draw_AlphaLineRGB (x, y + height, x + player_p_x, y + player_p_y, 2, higlight_color);

						// Bottom right.
						Draw_AlphaLineRGB (x + width, y + height, x + player_p_x, y + player_p_y, 2, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_TOP :
					{
						// Top center.
						Draw_AlphaLineRGB (x + width / 2, y, x + player_p_x, y + player_p_y, 2, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_CENTER :
					{
						// Center.
						Draw_AlphaLineRGB (x + width / 2, y + height / 2, x + player_p_x, y + player_p_y, 2, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_BOTTOM :
					{
						// Bottom center.
						Draw_AlphaLineRGB (x + width / 2, y + height, x + player_p_x, y + player_p_y, 2, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_FIXED_CIRCLE :
					{
						Draw_AlphaCircleRGB (x + player_p_x, y + player_p_y, player_size * 1.5, 1.0, true, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_CIRCLE :
					{
						Draw_AlphaCircleRGB (x + player_p_x, y + player_p_y, player_size * player_z_relative * 2.0, 1.0, true, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_FIXED_OUTLINE :
					{
						Draw_AlphaCircleOutlineRGB (x + player_p_x, y + player_p_y, player_size * 1.5, 1.0, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_OUTLINE :
					{
						Draw_AlphaCircleOutlineRGB (x + player_p_x, y + player_p_y, player_size * player_z_relative * 2.0, 1.0, higlight_color);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_TEXT_ONLY :
					default :
					{
						break;
					}
				}
			}

			// Draw the actual player and a line showing what direction the player is looking in.
			{
				float relative_x = 0;
				float relative_y = 0;

				x_line_start = x + player_p_x;
				y_line_start = y + player_p_y;

				// Translate the angle into radians.
				player_angle = DEG2RAD(player_angle);

				relative_x = cos(player_angle);
				relative_y = sin(player_angle);

				// Draw a slightly larger line behind the colored one
				// so that it get's an outline.
				x_line_end = x_line_start + (player_size * 2 * player_z_relative + 1) * relative_x;
				y_line_end = y_line_start - (player_size * 2 * player_z_relative + 1) * relative_y;
				Draw_AlphaLine (x_line_start, y_line_start, x_line_end, y_line_end, 4.0, 0, 1.0);

				// Draw the colored line.
				x_line_end = x_line_start + (player_size * 2 * player_z_relative) * relative_x;
				y_line_end = y_line_start - (player_size * 2 * player_z_relative) * relative_y;
				Draw_AlphaLine (x_line_start, y_line_start, x_line_end, y_line_end, 2.0, player_color, player_alpha);

				// Draw the player on the map.
				Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size * player_z_relative, player_color, player_alpha);
				Draw_AlphaCircleOutline (x + player_p_x, y + player_p_y, player_size * player_z_relative, 1.0, 0, 1.0);
			}

			// Draw the players name.
			if(show_names)
			{
				int name_x = 0;
				int name_y = 0;

				name_x = x + player_p_x;
				name_y = y + player_p_y;

				// Make sure we're not too far right.
				while(name_x + 8 * strlen(info->name) > x + width)
				{
					name_x--;
				}

				// Make sure we're not outside the radar to the left.
				name_x = max(name_x, x);

				// Draw the name.
				if (highlight >= HUD_RADAR_HIGHLIGHT_TEXT_ONLY
					&& info->userid == cl.players[Cam_TrackNum()].userid)
				{
					// Draw the tracked players name in the user specified color.
					Draw_ColoredString (name_x, name_y,
						va("&c%x%x%x%s",
						(unsigned int)(hud_radar_highlight_color[0] * 15),
						(unsigned int)(hud_radar_highlight_color[1] * 15),
						(unsigned int)(hud_radar_highlight_color[2] * 15), info->name), 0);
				}
				else
				{
					// Draw other players in normal character color.
					Draw_String (name_x, name_y, info->name);
				}
			}

			// Show if a person lost an RL-pack.
			if(info->stats[STAT_HEALTH] <= 0 && info->stats[STAT_ACTIVEWEAPON] == IT_ROCKET_LAUNCHER)
			{
				Draw_AlphaCircleOutline (x + player_p_x, y + player_p_y, player_size*player_z_relative*2, 1.0, 254, player_alpha);
				Draw_ColoredString (x + player_p_x, y + player_p_y, va("&cf00PACK!"), 1);
			}
		}
	}
}

//
// Draws a map of the current level and plots player movements on it.
//
void SCR_HUD_DrawRadar(hud_t *hud)
{
	int width, height, x, y;
	float width_limit, height_limit;
	float scale;
	float x_scale;
	float y_scale;

    static cvar_t
		*hud_radar_opacity = NULL,
		*hud_radar_width,
		*hud_radar_height,
		*hud_radar_autosize,
		*hud_radar_fade_players,
		*hud_radar_show_powerups,
		*hud_radar_show_names,
		*hud_radar_highlight,
		*hud_radar_highlight_color,
		*hud_radar_player_size,
		*hud_radar_show_height,
		*hud_radar_show_stats,
		*hud_radar_show_hold,
		*hud_radar_weaponfilter,
		*hud_radar_itemfilter,
		*hud_radar_onlytp,
		*hud_radar_otherfilter;

    if (hud_radar_opacity == NULL)    // first time
    {
		char checkval[256];

		hud_radar_opacity			= HUD_FindVar(hud, "opacity");
		hud_radar_width				= HUD_FindVar(hud, "width");
		hud_radar_height			= HUD_FindVar(hud, "height");
		hud_radar_autosize			= HUD_FindVar(hud, "autosize");
		hud_radar_fade_players		= HUD_FindVar(hud, "fade_players");
		hud_radar_show_powerups		= HUD_FindVar(hud, "show_powerups");
		hud_radar_show_names		= HUD_FindVar(hud, "show_names");
		hud_radar_player_size		= HUD_FindVar(hud, "player_size");
		hud_radar_show_height		= HUD_FindVar(hud, "show_height");
		hud_radar_show_stats		= HUD_FindVar(hud, "show_stats");
		hud_radar_show_hold			= HUD_FindVar(hud, "show_hold");
		hud_radar_weaponfilter		= HUD_FindVar(hud, "weaponfilter");
		hud_radar_itemfilter		= HUD_FindVar(hud, "itemfilter");
		hud_radar_otherfilter		= HUD_FindVar(hud, "otherfilter");
		hud_radar_onlytp			= HUD_FindVar(hud, "onlytp");
		hud_radar_highlight			= HUD_FindVar(hud, "highlight");
		hud_radar_highlight_color	= HUD_FindVar(hud, "highlight_color");

		//
		// Only parse the the filters when they change, not on each frame.
		//

		// Weapon filter.
		hud_radar_weaponfilter->OnChange = Radar_OnChangeWeaponFilter;
		strlcpy(checkval, hud_radar_weaponfilter->string, sizeof(checkval));
		Cvar_Set(hud_radar_weaponfilter, checkval);

		// Item filter.
		hud_radar_itemfilter->OnChange = Radar_OnChangeItemFilter;
		strlcpy(checkval, hud_radar_itemfilter->string, sizeof(checkval));
		Cvar_Set(hud_radar_itemfilter, checkval);

		// Other filter.
		hud_radar_otherfilter->OnChange = Radar_OnChangeOtherFilter;
		strlcpy(checkval, hud_radar_otherfilter->string, sizeof(checkval));
		Cvar_Set(hud_radar_otherfilter, checkval);

		// Highlight color.
		hud_radar_highlight_color->OnChange = Radar_OnChangeHighlightColor;
		strlcpy(checkval, hud_radar_highlight_color->string, sizeof(checkval));
		Cvar_Set(hud_radar_highlight_color, checkval);
    }

	// Don't show anything if it's a normal player.
	if(!(cls.demoplayback || cl.spectator))
	{
		HUD_PrepareDraw(hud, hud_radar_width->value, hud_radar_height->value, &x, &y);
		return;
	}

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_radar_onlytp->value))
	{
		HUD_PrepareDraw(hud, hud_radar_width->value, hud_radar_height->value, &x, &y);
		return;
	}

	// Save the width and height of the HUD. We're using these because
	// if autosize is on these will be altered and we don't want to change
	// the settings that the user set, if we try, and the user turns off
	// autosize again the size of the HUD will remain "autosized" until the user
	// resets it by hand again.
    width_limit = hud_radar_width->value;
	height_limit = hud_radar_height->value;

    // we support also sizes specified as a percentage of total screen width/height
    if (strchr(hud_radar_width->string, '%'))
        width_limit = width_limit * vid.conwidth / 100.0;
    if (strchr(hud_radar_height->string, '%'))
	    height_limit = hud_radar_height->value * vid.conheight / 100.0;

	// This map doesn't have a map pic.
	if(!radar_pic_found)
	{
		if(HUD_PrepareDraw(hud, Q_rint(width_limit), Q_rint(height_limit), &x, &y))
		{
			Draw_String(x, y, "No radar picture found!");
		}
		return;
	}

	// Make sure we can translate the coordinates.
	if(!conversion_formula_found)
	{
		if(HUD_PrepareDraw(hud, Q_rint(width_limit), Q_rint(height_limit), &x, &y))
		{
			Draw_String(x, y, "No conversion formula found!");
		}
		return;
	}

	x = 0;
	y = 0;

	scale = 1;

	if(hud_radar_autosize->value)
	{
		//
		// Autosize the hud element based on the size of the radar picture.
		//

		width = width_limit = radar_pic.width;
		height = height_limit = radar_pic.height;
	}
	else
	{
		//
		// Size the picture so that it fits inside the hud element.
		//

		// Set the scaling based on the picture dimensions.
		x_scale = (width_limit / radar_pic.width);
		y_scale = (height_limit / radar_pic.height);

		scale = (x_scale < y_scale) ? x_scale : y_scale;

		width = radar_pic.width * scale;
		height = radar_pic.height * scale;
	}

	if (HUD_PrepareDraw(hud, Q_rint(width_limit), Q_rint(height_limit), &x, &y))
	{
		float player_size = 1.0;
		static int lastframecount = -1;

		// Place the map picture in the center of the HUD element.
		x += Q_rint((width_limit / 2.0) - (width / 2.0));
		x = max(0, x);
		x = min(x + width, x);

		y += Q_rint((height_limit / 2.0) - (height / 2.0));
		y = max(0, y);
		y = min(y + height, y);

		// Draw the radar background.
		Draw_SAlphaPic (x, y, &radar_pic, hud_radar_opacity->value, scale);

		// Only draw once per frame.
		if (cls.framecount == lastframecount)
		{
			return;
		}
		lastframecount = cls.framecount;

		if (!cl.oldparsecount || !cl.parsecount || cls.state < ca_active)
		{
			return;
		}

		// Scale the player size after the size of the radar.
		player_size = hud_radar_player_size->value * scale;

		// Draw team stats.
		if(hud_radar_show_stats->value)
		{
			Radar_DrawGrid(stats_grid, x, y, scale, width, height, hud_radar_show_stats->value);
		}

		// Draw entities such as powerups, weapons and backpacks.
		if(RADAR_SHOW_WEAPONS || RADAR_SHOW_ITEMS || RADAR_SHOW_OTHER)
		{
			Radar_DrawEntities(x, y, scale,
				player_size,
				hud_radar_show_hold->value);
		}

		// Draw the players.
		Radar_DrawPlayers(x, y, width, height, scale,
			hud_radar_show_height->value,
			hud_radar_show_powerups->value,
			player_size,
			hud_radar_show_names->value,
			hud_radar_fade_players->value,
			hud_radar_highlight->value,
			hud_radar_highlight_color->string);
	}
}

#endif // WITH_PNG

//
// Run before HUD elements are drawn.
// Place stuff that is common for HUD elements here.
//
void HUD_BeforeDraw()
{
	// Only sort once per draw.
	HUD_Sort_Scoreboard (HUD_SCOREBOARD_ALL);
}

//
// Run after HUD elements are drawn.
// Place stuff that is common for HUD elements here.
//
void HUD_AfterDraw()
{
}


#ifndef HAXX
static void SCR_HUD_DrawNotImplemented(hud_t *hud)
{
	char line1[64];
	int width, height, x, y;

	snprintf(line1, sizeof(line1), "%s not implemented", hud->name);

	width = 8 * strlen(line1);
	height = 8;

	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
		return;

	Draw_SString(x, y, line1, 1);
}
#define SCR_HUD_DrawTeamHoldBar SCR_HUD_DrawNotImplemented
#define SCR_HUD_DrawTeamHoldInfo SCR_HUD_DrawNotImplemented
#define SCR_HUD_DrawItemsClock SCR_HUD_DrawNotImplemented
#endif


// ----------------
// Init
// and add some common elements to hud (clock etc)
//

void CommonDraw_Init(void)
{
    int i;

	HUD_InitSbarImages();

	// variables
	hud_planmode		= cvarfuncs->GetNVFDG("hud_planmode", "0", 0, NULL, "ezhud");
	hud_tp_need			= cvarfuncs->GetNVFDG("hud_tp_need", "0", 0, NULL, "ezhud");
	hud_digits_trim		= cvarfuncs->GetNVFDG("hud_digits_trim", "1", 0, NULL, "ezhud");
	mvd_autohud			= cvarfuncs->GetNVFDG("mvd_autohud", "0", 0, NULL, "ezhud");
	cl_weaponpreselect	= cvarfuncs->GetNVFDG("cl_weaponpreselect", "0", 0, NULL, "ezhud");
	cl_multiview		= cvarfuncs->GetNVFDG("cl_multiview", "0", 0, NULL, "ezhud");


	tp_need_health		= cvarfuncs->GetNVFDG("tp_need_health",	"50",		0, NULL, "ezhud");
	tp_need_ra			= cvarfuncs->GetNVFDG("tp_need_ra",		"50",		0, NULL, "ezhud");
	tp_need_ya			= cvarfuncs->GetNVFDG("tp_need_ya",		"50",		0, NULL, "ezhud");
	tp_need_ga			= cvarfuncs->GetNVFDG("tp_need_ga",		"50",		0, NULL, "ezhud");
	tp_weapon_order		= cvarfuncs->GetNVFDG("tp_weapon_order",	"78654321",	0, NULL, "ezhud");
	tp_need_weapon		= cvarfuncs->GetNVFDG("tp_need_weapon",	"35687",	0, NULL, "ezhud");
	tp_need_shells		= cvarfuncs->GetNVFDG("tp_need_shells",	"10",		0, NULL, "ezhud");
	tp_need_nails		= cvarfuncs->GetNVFDG("tp_need_nails",	"40",		0, NULL, "ezhud");
	tp_need_rockets		= cvarfuncs->GetNVFDG("tp_need_rockets",	"5",		0, NULL, "ezhud");
	tp_need_cells		= cvarfuncs->GetNVFDG("tp_need_cells",	"20",		0, NULL, "ezhud");

    // init HUD STAT table
    for (i=0; i < MAX_CL_STATS; i++)
        hud_stats[i] = 0;
    hud_stats[STAT_HEALTH]  = 200;
    hud_stats[STAT_AMMO]    = 100;
    hud_stats[STAT_ARMOR]   = 200;
    hud_stats[STAT_SHELLS]  = 100;
    hud_stats[STAT_NAILS]   = 200;
    hud_stats[STAT_ROCKETS] = 100;
    hud_stats[STAT_CELLS]   = 100;
    hud_stats[STAT_ACTIVEWEAPON] = 32;
    hud_stats[STAT_ITEMS] = 0xffffffff - IT_ARMOR2 - IT_ARMOR1;

	autohud.active = 0;

    // init gameclock
	HUD_Register("gameclock", NULL, "Shows current game time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 8, SCR_HUD_DrawGameClock,
        "1", "top", "right", "console", "0", "0", "0", "0 0 0", NULL,
        "big",      "1",
        "style",    "0",
		"scale",    "1",
        "blink",    "1",
		"countdown","0",
		"offset","0",
        NULL);

	HUD_Register("notify", NULL, "Shows last console lines",
		HUD_PLUSMINUS, ca_disconnected, 8, SCR_HUD_DrawNotify,
		"0", "top", "left", "top", "0", "0", "0", "0 0 0", NULL,
		"rows", "4",
		"cols", "30",
		"scale", "1",
		"time", "4",
		NULL);

	// fps
	HUD_Register("fps", NULL,
        "Shows your current framerate in frames per second (fps)."
        "This can also show the minimum framerate that occured in the last measured period.",
        HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawFPS,
        "1", "gameclock", "center", "after", "0", "0", "0", "0 0 0", NULL,
        "show_min", "0",
		"style",	"0",
        "title",    "1",
		"drop", "70",
        NULL);

	HUD_Register("vidlag", NULL,
        "Shows the delay between the time a frame is rendered and the time it's displayed.",
        HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawVidLag,
        "0", "top", "right", "top", "0", "0", "0", "0 0 0", NULL,
		"style",	"0",
        NULL);

	HUD_Register("mouserate", NULL, "Show your current mouse input rate", HUD_PLUSMINUS, ca_active, 9,
		SCR_HUD_DrawMouserate,
		"0", "screen", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
		"title", "1",
		"interval", "1",
		"style",	"0",
		NULL);

    // init clock
	HUD_Register("clock", NULL, "Shows current local time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 8, SCR_HUD_DrawClock,
        "0", "top", "right", "console", "0", "0", "0", "0 0 0", NULL,
        "big",      "1",
        "style",    "0",
		"scale",    "1",
        "blink",    "1",
		"format",   "0",
        NULL);

    // init democlock
	HUD_Register("democlock", NULL, "Shows current demo time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 7, SCR_HUD_DrawDemoClock,
        "1", "top", "right", "console", "0", "8", "0", "0 0 0", NULL,
        "big",      "0",
        "style",    "0",
		"scale",    "1",
        "blink",    "0",
        NULL);

    // init ping
    HUD_Register("ping", NULL, "Shows most important net conditions, like ping and pl. Shown only when you are connected to a server.",
        HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawPing,
        "0", "screen", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
        "period",       "1",
        "show_pl",      "1",
        "show_min",     "0",
        "show_max",     "0",
        "show_dev",     "0",
		"style",		"0",
        "blink",        "1",
        NULL);

	// init net
    HUD_Register("net", NULL, "Shows network statistics, like latency, packet loss, average packet sizes and bandwidth. Shown only when you are connected to a server.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawNetStats,
        "0", "top", "left", "center", "0", "0", "0.2", "0 0 0", NULL,
        "period",  "1",
        NULL);

    // init speed
    HUD_Register("speed", NULL, "Shows your current running speed. It is measured over XY or XYZ axis depending on \'xyz\' property.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawSpeed,
        "0", "top", "center", "bottom", "0", "-5", "0", "0 0 0", NULL,
        "xyz",  "0",
		"width", "160",
		"height", "15",
		"opacity", "1.0",
		"tick_spacing", "0.2",
		"color_stopped", SPEED_STOPPED,
		"color_normal", SPEED_NORMAL,
		"color_fast", SPEED_FAST,
		"color_fastest", SPEED_FASTEST,
		"color_insane", SPEED_INSANE,
		"vertical", "0",
		"vertical_text", "1",
		"text_align", "1",
		"style", "0",
		"scale", "1",
		NULL);

    // Init speed2 (half circle thingie).
    HUD_Register("speed2", NULL, "Shows your current running speed. It is measured over XY or XYZ axis depending on \'xyz\' property.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawSpeed2,
        "0", "top", "center", "bottom", "0", "0", "0", "0 0 0", NULL,
        "xyz",  "0",
		"opacity", "1.0",
		"color_stopped", SPEED_STOPPED,
		"color_normal", SPEED_NORMAL,
		"color_fast", SPEED_FAST,
		"color_fastest", SPEED_FASTEST,
		"color_insane", SPEED_INSANE,
		"radius", "50.0",
		"wrapspeed", "500",
		"orientation", "0",
		NULL);

    // init guns
    HUD_Register("gun", NULL, "Part of your inventory - current weapon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGunCurrent,
        "0", "ibar", "center", "bottom", "0", "0", "0", "0 0 0", NULL,
        "wide",  "0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun2", NULL, "Part of your inventory - shotgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun2,
        "1", "ibar", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun3", NULL, "Part of your inventory - super shotgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun3,
        "1", "gun2", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun4", NULL, "Part of your inventory - nailgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun4,
        "1", "gun3", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun5", NULL, "Part of your inventory - super nailgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun5,
        "1", "gun4", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun6", NULL, "Part of your inventory - grenade launcher.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun6,
        "1", "gun5", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun7", NULL, "Part of your inventory - rocket launcher.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun7,
        "1", "gun6", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun8", NULL, "Part of your inventory - thunderbolt.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun8,
        "1", "gun7", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "wide",  "0",
        "style", "0",
        "scale", "1",
        NULL);

    // init powerzz
    HUD_Register("key1", NULL, "Part of your inventory - silver key.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawKey1,
        "1", "ibar", "top", "left", "0", "64", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("key2", NULL, "Part of your inventory - gold key.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawKey2,
        "1", "key1", "left", "after", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("ring", NULL, "Part of your inventory - invisibility.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawRing,
        "1", "key2", "left", "after", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("pent", NULL, "Part of your inventory - invulnerability.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawPent,
        "1", "ring", "left", "after", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("suit", NULL, "Part of your inventory - biosuit.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSuit,
        "1", "pent", "left", "after", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("quad", NULL, "Part of your inventory - quad damage.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawQuad,
        "1", "suit", "left", "after", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);

	// netproblem icon
    HUD_Register("netproblem", NULL, "Shows an icon if you are experiencing network problems",
        HUD_NO_FRAME, ca_active, 0, SCR_HUD_NetProblem,
        "1", "top", "left", "top", "0", "0", "0", "0 0 0", NULL,
        "scale", "1",
        NULL);

    // sigilzz
    HUD_Register("sigil1", NULL, "Part of your inventory - sigil 1.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil2", NULL, "Part of your inventory - sigil 2.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil2,
        "0", "sigil1", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil3", NULL, "Part of your inventory - sigil 3.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil3,
        "0", "sigil2", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil4", NULL, "Part of your inventory - sigil 4.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil4,
        "0", "sigil3", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);

	// player face (health indicator)
    HUD_Register("face", NULL, "Your bloody face.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawFace,
        "1", "screen", "center", "bottom", "0", "0", "0", "0 0 0", NULL,
        "scale", "1",
        NULL);

	// health
    HUD_Register("health", NULL, "Part of your status - health level.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawHealth,
        "1", "face", "after", "center", "0", "0", "0", "0 0 0", NULL,
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
        NULL);

    // ammo/s
    HUD_Register("ammo", NULL, "Part of your inventory - ammo for active weapon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoCurrent,
        "1", "health", "after", "center", "32", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo1", NULL, "Part of your inventory - ammo - shells.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo2", NULL, "Part of your inventory - ammo - nails.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo2,
        "0", "ammo1", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo3", NULL, "Part of your inventory - ammo - rockets.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo3,
        "0", "ammo2", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo4", NULL, "Part of your inventory - ammo - cells.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo4,
        "0", "ammo3", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);

    // ammo icon/s
    HUD_Register("iammo", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIconCurrent,
        "1", "ammo", "before", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("iammo1", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo2", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon2,
        "0", "iammo1", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo3", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon3,
        "0", "iammo2", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo4", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon4,
        "0", "iammo3", "after", "top", "0", "0", "0", "0 0 0", NULL,
        "style", "2",
        "scale", "1",
        NULL);

    // armor count
    HUD_Register("armor", NULL, "Part of your inventory - armor level.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmor,
        "1", "face", "before", "center", "-32", "0", "0", "0 0 0", NULL,
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
		"pent_666", "1",  // Show 666 instead of armor value
        NULL);

	// armor icon
    HUD_Register("iarmor", NULL, "Part of your inventory - armor icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmorIcon,
        "1", "armor", "before", "center", "0", "0", "0", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        NULL);

	// Tracking JohnNy_cz (Contains name of the player who's player we're watching at the moment)
	HUD_Register("tracking", NULL, "Shows the name of tracked player.",
		HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawTracking,
		"1", "face", "center", "before", "0", "0", "0", "0 0 0", NULL,
		"format", "^mTracking:^m %t %n"/*, ^mJUMP^m for next"*/, //"Tracking: team name, JUMP for next", "Tracking:" and "JUMP" are brown. default: "Tracking %t %n, [JUMP] for next"
		"scale", "1",
		NULL);

    // groups
    HUD_Register("group1", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group1,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0", NULL,
        "name", "group1",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group2", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group2,
        "0", "screen", "center", "top", "0", "0", ".5", "0 0 0", NULL,
        "name", "group2",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group3", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group3,
        "0", "screen", "right", "top", "0", "0", ".5", "0 0 0", NULL,
        "name", "group3",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group4", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group4,
        "0", "screen", "left", "center", "0", "0", ".5", "0 0 0", NULL,
        "name", "group4",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group5", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group5,
        "0", "screen", "center", "center", "0", "0", ".5", "0 0 0", NULL,
        "name", "group5",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group6", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group6,
        "0", "screen", "right", "center", "0", "0", ".5", "0 0 0", NULL,
        "name", "group6",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group7", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group7,
        "0", "screen", "left", "bottom", "0", "0", ".5", "0 0 0", NULL,
        "name", "group7",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group8", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group8,
        "0", "screen", "center", "bottom", "0", "0", ".5", "0 0 0", NULL,
        "name", "group8",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group9", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group9,
        "0", "screen", "right", "bottom", "0", "0", ".5", "0 0 0", NULL,
        "name", "group9",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);

    // healthdamage
    HUD_Register("healthdamage", NULL, "Shows amount of damage done to your health.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawHealthDamage,
        "0", "health", "left", "before", "0", "0", "0", "0 0 0", NULL,
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
		"duration", "0.8",
        NULL);

    // armordamage
    HUD_Register("armordamage", NULL, "Shows amount of damage done to your armour.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmorDamage,
        "0", "armor", "left", "before", "0", "0", "0", "0 0 0", NULL,
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
		"duration", "0.8",
        NULL);

    HUD_Register("frags", NULL, "Show list of player frags in short form.",
        0, ca_active, 0, SCR_HUD_DrawFrags,
        "0", "top", "right", "bottom", "0", "0", "0", "0 0 0", NULL,
        "cell_width", "32",
        "cell_height", "8",
        "rows", "1",
        "cols", "4",
        "space_x", "1",
        "space_y", "1",
        "teamsort", "0",
        "strip", "1",
        "vertical", "0",
		"shownames", "0",
		"showteams", "0",
		"padtext", "1",
		"showself_always", "1",
		"extra_spec_info", "ALL",
		"fliptext", "0",
		"style", "0",
		"bignum", "0",
		"colors_alpha", "1.0",
		"maxname", "16",
		"notintp", "0",
        NULL);

    HUD_Register("teamfrags", NULL, "Show list of team frags in short form.",
        0, ca_active, 0, SCR_HUD_DrawTeamFrags,
        "1", "ibar", "center", "before", "0", "0", "0", "0 0 0", NULL,
        "cell_width", "32",
        "cell_height", "8",
        "rows", "1",
        "cols", "2",
        "space_x", "1",
        "space_y", "1",
        "strip", "1",
        "vertical", "0",
		"shownames", "0",
		"padtext", "1",
		"fliptext", "1",
		"style", "0",
		"extra_spec_info", "1",
		"onlytp", "0",
		"bignum", "0",
		"colors_alpha", "1.0",
		"maxname", "16",
		NULL);

    HUD_Register("teaminfo", NULL, "Show information about your team in short form.",
        0, ca_active, 0, SCR_HUD_DrawTeamInfo,
        "0", "", "right", "center", "0", "0", "0.2", "20 20 20", NULL,
		"layout", "%p%n $x10%l$x11 %a/%H %w",
		"align_right","0",
		"loc_width","5",
		"name_width","6",
		"low_health","25",
		"armor_style","3",
		"weapon_style","0",
		"show_enemies","0",
		"show_self","1",
		"scale","1",
		"powerup_style","1",
		NULL);

	HUD_Register("mp3_title", NULL, "Shows current mp3 playing.",
        HUD_PLUSMINUS, ca_disconnected, 0, SCR_HUD_DrawMP3_Title,
        "0", "top", "right", "bottom", "0", "0", "0", "0 0 0", NULL,
		"style",	"2",
		"width",	"512",
		"height",	"8",
		"scroll",	"1",
		"scroll_delay", "0.5",
		"on_scoreboard", "0",
		"wordwrap", "0",
        NULL);

	HUD_Register("mp3_time", NULL, "Shows the time of the current mp3 playing.",
        HUD_PLUSMINUS, ca_disconnected, 0, SCR_HUD_DrawMP3_Time,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
		"style",	"0",
		"on_scoreboard", "0",
        NULL);

#ifdef WITH_PNG
	HUD_Register("radar", NULL, "Plots the players on a picture of the map. (Only when watching MVD's or QTV).",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawRadar,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
		"opacity", "0.5",
		"width", "30%",
		"height", "25%",
		"autosize", "0",
		"show_powerups", "1",
		"show_names", "0",
		"highlight_color", "yellow",
		"highlight", "0",
		"player_size", "10",
		"show_height", "1",
		"show_stats", "1",
		"fade_players", "1",
		"show_hold", "0",
		"weaponfilter", "gl rl lg",
		"itemfilter", "backpack quad pent armor mega",
		"otherfilter", "projectiles gibs explosions shotgun",
		"onlytp", "0",
        NULL);
#endif // WITH_PNG

	HUD_Register("teamholdbar", NULL, "Shows how much of the level (in percent) that is currently being held by either team.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawTeamHoldBar,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
		"opacity", "0.8",
		"width", "200",
		"height", "8",
		"vertical", "0",
		"vertical_text", "0",
		"show_text", "1",
		"onlytp", "0",
        NULL);

	HUD_Register("teamholdinfo", NULL, "Shows which important items in the level that are being held by the teams.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawTeamHoldInfo,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0", NULL,
		"opacity", "0.8",
		"width", "200",
		"height", "8",
		"onlytp", "0",
		"style", "1",
		"itemfilter", "quad ra ya ga mega pent rl quad",
        NULL);

    HUD_Register("ownfrags" /* jeez someone give me a better name please */, NULL, "Highlights your own frags",
        0, ca_active, 1, SCR_HUD_DrawOwnFrags,
        "1", "screen", "center", "top", "0", "50", "0.2", "0 0 100", NULL,
        /*
        "color", "255 255 255",
        */
        "timeout", "3",
		"scale", "1.5",
        NULL
        );

	HUD_Register("keys", NULL, "Shows which keys user does press at the moment",
		0, ca_active, 1, SCR_HUD_DrawKeys,
		"0", "screen", "right", "center", "0", "0", "0.5", "20 20 20", NULL,
		"scale", "2",
		NULL
		);

	HUD_Register("itemsclock", NULL, "Displays upcoming item respawns",
		0, ca_active, 1, SCR_HUD_DrawItemsClock,
		"0", "screen", "right", "center", "0", "0", "0", "0 0 0", NULL,
		"timelimit", "5",
		"style", "0",
		NULL
		);

    HUD_Register("score_team", NULL, "Own scores or team scores.",
        0, ca_active, 0, SCR_HUD_DrawScoresTeam,
        "0", "screen", "left", "bottom", "0", "0", "0.5", "4 8 32", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "0",
		"colorize", "0",
        NULL
		);

	HUD_Register("score_enemy", NULL, "Scores of enemy or enemy team.",
        0, ca_active, 0, SCR_HUD_DrawScoresEnemy,
        "0", "score_team", "after", "bottom", "0", "0", "0.5", "32 4 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "0",
		"colorize", "0",
        NULL
		);

	HUD_Register("score_difference", NULL, "Difference between teamscores and enemyscores.",
        0, ca_active, 0, SCR_HUD_DrawScoresDifference,
        "0", "score_enemy", "after", "bottom", "0", "0", "0.5", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "0",
		"colorize", "1",
        NULL
		);

	HUD_Register("score_position", NULL, "Position on scoreboard.",
        0, ca_active, 0, SCR_HUD_DrawScoresPosition,
        "0", "score_difference", "after", "bottom", "0", "0", "0.5", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "0",
		"colorize", "1",
        NULL
		);

	HUD_Register("score_bar", NULL, "Team, enemy, and difference scores together.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawScoresBar,
        "0", "screen", "center", "console", "0", "0", "0.5", "0 0 0", NULL,
        "style", "0",
        "scale", "1",
		"format_small", "&c69f%T&r:%t &cf10%E&r:%e $[%D$]",
		"format_big", "%t:%e:%Z",

        NULL
		);

	HUD_Register("bar_armor", NULL, "Armor bar.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawBarArmor,
        "0", "armor", "left", "center", "0", "0", "0", "0 0 0", NULL,
        "height", "16",
        "width", "64",
		"direction", "1",
		"color_noarmor", "128 128 128 64",
		"color_ga", "32 128 0 128",
		"color_ya", "192 128 0 128",
		"color_ra", "128 0 0 128",
		"color_unnatural", "255 255 255 128",
        NULL
		);

	HUD_Register("bar_health", NULL, "Health bar.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawBarHealth,
        "0", "health", "right", "center", "0", "0", "0", "0 0 0", NULL,
        "height", "16",
        "width", "64",
		"direction", "0",
		"color_nohealth", "128 128 128 64",
		"color_normal", "32 64 128 128",
		"color_mega", "64 96 128 128",
		"color_twomega", "128 128 255 128",
		"color_unnatural", "255 255 255 128",
        NULL
		);

#ifdef QUAKEHUD
	HUD_Register("weaponstats", NULL, "Weapon Stats",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawWeaponStats,
        "0", "screen", "right", "center", "0", "0", "0", "0 0 0", NULL,
		"scale", "1",
		"fmt", "&c990sg&r:[%sg] &c099ssg&r:[%ssg] &c900rl&r:[#rl] &c009lg&r:[%lg]",
        NULL
		);
#endif

/* hexum -> FIXME? this is used only for debug purposes, I wont bother to port it (it shouldnt be too difficult if anyone cares)
#ifdef _DEBUG
    HUD_Register("framegraph", NULL, "Shows different frame times for debug/profiling purposes.",
        HUD_PLUSMINUS | HUD_ON_SCORES, ca_disconnected, 0, SCR_HUD_DrawFrameGraph,
        "0", "top", "left", "bottom", "0", "0", "2",
        "swap_x",       "0",
        "swap_y",       "0",
        "scale",        "14",
        "width",        "256",
        "height",       "64",
        "alpha",        "1",
        NULL);
#endif
*/

}


