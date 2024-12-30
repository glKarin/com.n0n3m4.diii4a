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
// sbar.c -- status bar code

#include "quakedef.h"
#include <time.h>
#include "cl_collision.h"
#include "csprogs.h"

cachepic_t *sb_disc;

#define STAT_MINUS 10 // num frame for '-' stats digit
cachepic_t *sb_nums[2][11];
cachepic_t *sb_colon, *sb_slash;
cachepic_t *sb_ibar;
cachepic_t *sb_sbar;
cachepic_t *sb_scorebar;
// AK only used by NEX
cachepic_t *sb_sbar_minimal;
cachepic_t *sb_sbar_overlay;

// AK changed the bound to 9
cachepic_t *sb_weapons[7][9]; // 0 is active, 1 is owned, 2-5 are flashes
cachepic_t *sb_ammo[4];
cachepic_t *sb_sigil[4];
cachepic_t *sb_armor[3];
cachepic_t *sb_items[32];

// 0-4 are based on health (in 20 increments)
// 0 is static, 1 is temporary animation
cachepic_t *sb_faces[5][2];
cachepic_t *sb_health; // GAME_NEXUIZ

cachepic_t *sb_face_invis;
cachepic_t *sb_face_quad;
cachepic_t *sb_face_invuln;
cachepic_t *sb_face_invis_invuln;

qbool sb_showscores;

int sb_lines;			// scan lines to draw

cachepic_t *rsb_invbar[2];
cachepic_t *rsb_weapons[5];
cachepic_t *rsb_items[2];
cachepic_t *rsb_ammo[3];
cachepic_t *rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
cachepic_t *hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
int hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
cachepic_t *hsb_items[2];

cachepic_t *zymsb_crosshair_center;
cachepic_t *zymsb_crosshair_line;
cachepic_t *zymsb_crosshair_health;
cachepic_t *zymsb_crosshair_ammo;
cachepic_t *zymsb_crosshair_clip;
cachepic_t *zymsb_crosshair_background;
cachepic_t *zymsb_crosshair_left1;
cachepic_t *zymsb_crosshair_left2;
cachepic_t *zymsb_crosshair_right;

cachepic_t *sb_ranking;
cachepic_t *sb_complete;
cachepic_t *sb_inter;
cachepic_t *sb_finale;

cvar_t cl_showfps = {CF_CLIENT | CF_ARCHIVE, "cl_showfps", "0", "shows your rendered fps (frames per second)"};
cvar_t cl_showsound = {CF_CLIENT | CF_ARCHIVE, "cl_showsound", "0", "shows number of active sound sources, sound latency, and other statistics"};
cvar_t cl_showblur = {CF_CLIENT | CF_ARCHIVE, "cl_showblur", "0", "shows the current alpha level of motionblur"};
cvar_t cl_showspeed = {CF_CLIENT | CF_ARCHIVE, "cl_showspeed", "0", "shows your current speed (qu per second); number selects unit: 1 = qu/s, 2 = m/s, 3 = km/h, 4 = mph, 5 = knots"};
cvar_t cl_showspeed_factor = {CF_CLIENT | CF_ARCHIVE, "cl_showspeed_factor", "2.54", "multiplier of the centimeter for cl_showspeed. 1 unit = 1 inch in Quake, so this should be 2.54 for Quake, etc"};
cvar_t cl_showtopspeed = {CF_CLIENT | CF_ARCHIVE, "cl_showtopspeed", "0", "shows your top speed (kept on screen for max 3 seconds); value -1 takes over the unit from cl_showspeed, otherwise it's an unit number just like in cl_showspeed"};
cvar_t cl_showtime = {CF_CLIENT | CF_ARCHIVE, "cl_showtime", "0", "shows current time of day (useful on screenshots)"};
cvar_t cl_showtime_format = {CF_CLIENT | CF_ARCHIVE, "cl_showtime_format", "%H:%M:%S", "format string for time of day"};
cvar_t cl_showdate = {CF_CLIENT | CF_ARCHIVE, "cl_showdate", "0", "shows current date (useful on screenshots)"};
cvar_t cl_showdate_format = {CF_CLIENT | CF_ARCHIVE, "cl_showdate_format", "%Y-%m-%d", "format string for date"};
cvar_t cl_showtex = {CF_CLIENT, "cl_showtex", "0", "shows the name of the texture on the crosshair (for map debugging)"};

cvar_t sbar_alpha_bg = {CF_CLIENT | CF_ARCHIVE, "sbar_alpha_bg", "0.4", "opacity value of the statusbar background image"};
cvar_t sbar_alpha_fg = {CF_CLIENT | CF_ARCHIVE, "sbar_alpha_fg", "1", "opacity value of the statusbar weapon/item icons and numbers"};
cvar_t sbar_hudselector = {CF_CLIENT | CF_ARCHIVE, "sbar_hudselector", "0", "selects which of the builtin hud layouts to use (meaning is somewhat dependent on gamemode, so nexuiz has a very different set of hud layouts than quake for example)"};
cvar_t sbar_scorerank = {CF_CLIENT | CF_ARCHIVE, "sbar_scorerank", "1", "shows an overlay for your score (or team score) and rank in the scoreboard"};
cvar_t sbar_gametime = {CF_CLIENT | CF_ARCHIVE, "sbar_gametime", "1", "shows an overlay for the time left in the current match/level (or current game time if there is no timelimit set)"};
cvar_t sbar_miniscoreboard_size = {CF_CLIENT | CF_ARCHIVE, "sbar_miniscoreboard_size", "-1", "sets the size of the mini deathmatch overlay in items, or disables it when set to 0, or sets it to a sane default when set to -1"};
cvar_t sbar_flagstatus_right = {CF_CLIENT | CF_ARCHIVE, "sbar_flagstatus_right", "0", "moves Nexuiz flag status icons to the right"};
cvar_t sbar_flagstatus_pos = {CF_CLIENT | CF_ARCHIVE, "sbar_flagstatus_pos", "115", "pixel position of the Nexuiz flag status icons, from the bottom"};
cvar_t sbar_info_pos = {CF_CLIENT | CF_ARCHIVE, "sbar_info_pos", "0", "pixel position of the info strings (such as showfps), from the bottom"};

cvar_t cl_deathscoreboard = {CF_CLIENT, "cl_deathscoreboard", "1", "shows scoreboard (+showscores) while dead"};

cvar_t crosshair_color_red = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_red", "1", "customizable crosshair color"};
cvar_t crosshair_color_green = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_green", "0", "customizable crosshair color"};
cvar_t crosshair_color_blue = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_blue", "0", "customizable crosshair color"};
cvar_t crosshair_color_alpha = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_alpha", "1", "how opaque the crosshair should be"};
cvar_t crosshair_size = {CF_CLIENT | CF_ARCHIVE, "crosshair_size", "1", "adjusts size of the crosshair on the screen"};

static void Sbar_MiniDeathmatchOverlay (int x, int y);
static void Sbar_IntermissionOverlay (void);
static void Sbar_FinaleOverlay (void);



/*
===============
Sbar_ShowScores

Tab key down
===============
*/
static void Sbar_ShowScores_f(cmd_state_t *cmd)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	CL_VM_UpdateShowingScoresState(sb_showscores);
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
static void Sbar_DontShowScores_f(cmd_state_t *cmd)
{
	sb_showscores = false;
	CL_VM_UpdateShowingScoresState(sb_showscores);
}

static void sbar_start(void)
{
	char vabuf[1024];
	int i;

	if (gamemode == GAME_DELUXEQUAKE || gamemode == GAME_BLOODOMNICIDE)
	{
	}
	else if (IS_OLDNEXUIZ_DERIVED(gamemode))
	{
		for (i = 0;i < 10;i++)
			sb_nums[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/num_%i",i), CACHEPICFLAG_QUIET);
		sb_nums[0][10] = Draw_CachePic_Flags ("gfx/num_minus", CACHEPICFLAG_QUIET);
		sb_colon = Draw_CachePic_Flags ("gfx/num_colon", CACHEPICFLAG_QUIET | CACHEPICFLAG_FAILONMISSING);

		sb_ammo[0] = Draw_CachePic_Flags ("gfx/sb_shells", CACHEPICFLAG_QUIET);
		sb_ammo[1] = Draw_CachePic_Flags ("gfx/sb_bullets", CACHEPICFLAG_QUIET);
		sb_ammo[2] = Draw_CachePic_Flags ("gfx/sb_rocket", CACHEPICFLAG_QUIET);
		sb_ammo[3] = Draw_CachePic_Flags ("gfx/sb_cells", CACHEPICFLAG_QUIET);

		sb_armor[0] = Draw_CachePic_Flags ("gfx/sb_armor", CACHEPICFLAG_QUIET);
		sb_armor[1] = NULL;
		sb_armor[2] = NULL;

		sb_health = Draw_CachePic_Flags ("gfx/sb_health", CACHEPICFLAG_QUIET);

		sb_items[2] = Draw_CachePic_Flags ("gfx/sb_slowmo", CACHEPICFLAG_QUIET);
		sb_items[3] = Draw_CachePic_Flags ("gfx/sb_invinc", CACHEPICFLAG_QUIET);
		sb_items[4] = Draw_CachePic_Flags ("gfx/sb_energy", CACHEPICFLAG_QUIET);
		sb_items[5] = Draw_CachePic_Flags ("gfx/sb_str", CACHEPICFLAG_QUIET);

		sb_items[11] = Draw_CachePic_Flags ("gfx/sb_flag_red_taken", CACHEPICFLAG_QUIET);
		sb_items[12] = Draw_CachePic_Flags ("gfx/sb_flag_red_lost", CACHEPICFLAG_QUIET);
		sb_items[13] = Draw_CachePic_Flags ("gfx/sb_flag_red_carrying", CACHEPICFLAG_QUIET);
		sb_items[14] = Draw_CachePic_Flags ("gfx/sb_key_carrying", CACHEPICFLAG_QUIET);
		sb_items[15] = Draw_CachePic_Flags ("gfx/sb_flag_blue_taken", CACHEPICFLAG_QUIET);
		sb_items[16] = Draw_CachePic_Flags ("gfx/sb_flag_blue_lost", CACHEPICFLAG_QUIET);
		sb_items[17] = Draw_CachePic_Flags ("gfx/sb_flag_blue_carrying", CACHEPICFLAG_QUIET);

		sb_sbar = Draw_CachePic_Flags ("gfx/sbar", CACHEPICFLAG_QUIET);
		sb_sbar_minimal = Draw_CachePic_Flags ("gfx/sbar_minimal", CACHEPICFLAG_QUIET);
		sb_sbar_overlay = Draw_CachePic_Flags ("gfx/sbar_overlay", CACHEPICFLAG_QUIET);

		for(i = 0; i < 9;i++)
			sb_weapons[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inv_weapon%i",i), CACHEPICFLAG_QUIET);
	}
	else if (gamemode == GAME_ZYMOTIC)
	{
		zymsb_crosshair_center = Draw_CachePic_Flags ("gfx/hud/crosshair_center", CACHEPICFLAG_QUIET);
		zymsb_crosshair_line = Draw_CachePic_Flags ("gfx/hud/crosshair_line", CACHEPICFLAG_QUIET);
		zymsb_crosshair_health = Draw_CachePic_Flags ("gfx/hud/crosshair_health", CACHEPICFLAG_QUIET);
		zymsb_crosshair_clip = Draw_CachePic_Flags ("gfx/hud/crosshair_clip", CACHEPICFLAG_QUIET);
		zymsb_crosshair_ammo = Draw_CachePic_Flags ("gfx/hud/crosshair_ammo", CACHEPICFLAG_QUIET);
		zymsb_crosshair_background = Draw_CachePic_Flags ("gfx/hud/crosshair_background", CACHEPICFLAG_QUIET);
		zymsb_crosshair_left1 = Draw_CachePic_Flags ("gfx/hud/crosshair_left1", CACHEPICFLAG_QUIET);
		zymsb_crosshair_left2 = Draw_CachePic_Flags ("gfx/hud/crosshair_left2", CACHEPICFLAG_QUIET);
		zymsb_crosshair_right = Draw_CachePic_Flags ("gfx/hud/crosshair_right", CACHEPICFLAG_QUIET);
	}
	else
	{
		sb_disc = Draw_CachePic_Flags ("gfx/disc", CACHEPICFLAG_QUIET);

		for (i = 0;i < 10;i++)
		{
			sb_nums[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/num_%i",i), CACHEPICFLAG_QUIET);
			sb_nums[1][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/anum_%i",i), CACHEPICFLAG_QUIET);
		}

		sb_nums[0][10] = Draw_CachePic_Flags ("gfx/num_minus", CACHEPICFLAG_QUIET);
		sb_nums[1][10] = Draw_CachePic_Flags ("gfx/anum_minus", CACHEPICFLAG_QUIET);

		sb_colon = Draw_CachePic_Flags ("gfx/num_colon", CACHEPICFLAG_QUIET | CACHEPICFLAG_FAILONMISSING);
		sb_slash = Draw_CachePic_Flags ("gfx/num_slash", CACHEPICFLAG_QUIET);

		sb_weapons[0][0] = Draw_CachePic_Flags ("gfx/inv_shotgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][1] = Draw_CachePic_Flags ("gfx/inv_sshotgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][2] = Draw_CachePic_Flags ("gfx/inv_nailgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][3] = Draw_CachePic_Flags ("gfx/inv_snailgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][4] = Draw_CachePic_Flags ("gfx/inv_rlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[0][5] = Draw_CachePic_Flags ("gfx/inv_srlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[0][6] = Draw_CachePic_Flags ("gfx/inv_lightng", CACHEPICFLAG_QUIET);

		sb_weapons[1][0] = Draw_CachePic_Flags ("gfx/inv2_shotgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][1] = Draw_CachePic_Flags ("gfx/inv2_sshotgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][2] = Draw_CachePic_Flags ("gfx/inv2_nailgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][3] = Draw_CachePic_Flags ("gfx/inv2_snailgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][4] = Draw_CachePic_Flags ("gfx/inv2_rlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[1][5] = Draw_CachePic_Flags ("gfx/inv2_srlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[1][6] = Draw_CachePic_Flags ("gfx/inv2_lightng", CACHEPICFLAG_QUIET);

		for (i = 0;i < 5;i++)
		{
			sb_weapons[2+i][0] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_shotgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][1] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_sshotgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][2] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_nailgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][3] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_snailgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][4] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_rlaunch",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][5] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_srlaunch",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][6] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_lightng",i+1), CACHEPICFLAG_QUIET);
		}

		sb_ammo[0] = Draw_CachePic_Flags ("gfx/sb_shells", CACHEPICFLAG_QUIET);
		sb_ammo[1] = Draw_CachePic_Flags ("gfx/sb_nails", CACHEPICFLAG_QUIET);
		sb_ammo[2] = Draw_CachePic_Flags ("gfx/sb_rocket", CACHEPICFLAG_QUIET);
		sb_ammo[3] = Draw_CachePic_Flags ("gfx/sb_cells", CACHEPICFLAG_QUIET);

		sb_armor[0] = Draw_CachePic_Flags ("gfx/sb_armor1", CACHEPICFLAG_QUIET);
		sb_armor[1] = Draw_CachePic_Flags ("gfx/sb_armor2", CACHEPICFLAG_QUIET);
		sb_armor[2] = Draw_CachePic_Flags ("gfx/sb_armor3", CACHEPICFLAG_QUIET);

		sb_items[0] = Draw_CachePic_Flags ("gfx/sb_key1", CACHEPICFLAG_QUIET);
		sb_items[1] = Draw_CachePic_Flags ("gfx/sb_key2", CACHEPICFLAG_QUIET);
		sb_items[2] = Draw_CachePic_Flags ("gfx/sb_invis", CACHEPICFLAG_QUIET);
		sb_items[3] = Draw_CachePic_Flags ("gfx/sb_invuln", CACHEPICFLAG_QUIET);
		sb_items[4] = Draw_CachePic_Flags ("gfx/sb_suit", CACHEPICFLAG_QUIET);
		sb_items[5] = Draw_CachePic_Flags ("gfx/sb_quad", CACHEPICFLAG_QUIET);

		sb_sigil[0] = Draw_CachePic_Flags ("gfx/sb_sigil1", CACHEPICFLAG_QUIET);
		sb_sigil[1] = Draw_CachePic_Flags ("gfx/sb_sigil2", CACHEPICFLAG_QUIET);
		sb_sigil[2] = Draw_CachePic_Flags ("gfx/sb_sigil3", CACHEPICFLAG_QUIET);
		sb_sigil[3] = Draw_CachePic_Flags ("gfx/sb_sigil4", CACHEPICFLAG_QUIET);

		sb_faces[4][0] = Draw_CachePic_Flags ("gfx/face1", CACHEPICFLAG_QUIET);
		sb_faces[4][1] = Draw_CachePic_Flags ("gfx/face_p1", CACHEPICFLAG_QUIET);
		sb_faces[3][0] = Draw_CachePic_Flags ("gfx/face2", CACHEPICFLAG_QUIET);
		sb_faces[3][1] = Draw_CachePic_Flags ("gfx/face_p2", CACHEPICFLAG_QUIET);
		sb_faces[2][0] = Draw_CachePic_Flags ("gfx/face3", CACHEPICFLAG_QUIET);
		sb_faces[2][1] = Draw_CachePic_Flags ("gfx/face_p3", CACHEPICFLAG_QUIET);
		sb_faces[1][0] = Draw_CachePic_Flags ("gfx/face4", CACHEPICFLAG_QUIET);
		sb_faces[1][1] = Draw_CachePic_Flags ("gfx/face_p4", CACHEPICFLAG_QUIET);
		sb_faces[0][0] = Draw_CachePic_Flags ("gfx/face5", CACHEPICFLAG_QUIET);
		sb_faces[0][1] = Draw_CachePic_Flags ("gfx/face_p5", CACHEPICFLAG_QUIET);

		sb_face_invis = Draw_CachePic_Flags ("gfx/face_invis", CACHEPICFLAG_QUIET);
		sb_face_invuln = Draw_CachePic_Flags ("gfx/face_invul2", CACHEPICFLAG_QUIET);
		sb_face_invis_invuln = Draw_CachePic_Flags ("gfx/face_inv2", CACHEPICFLAG_QUIET);
		sb_face_quad = Draw_CachePic_Flags ("gfx/face_quad", CACHEPICFLAG_QUIET);

		sb_sbar = Draw_CachePic_Flags ("gfx/sbar", CACHEPICFLAG_QUIET);
		sb_ibar = Draw_CachePic_Flags ("gfx/ibar", CACHEPICFLAG_QUIET);
		sb_scorebar = Draw_CachePic_Flags ("gfx/scorebar", CACHEPICFLAG_QUIET);

	//MED 01/04/97 added new hipnotic weapons
		if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
		{
			hsb_weapons[0][0] = Draw_CachePic_Flags ("gfx/inv_laser", CACHEPICFLAG_QUIET);
			hsb_weapons[0][1] = Draw_CachePic_Flags ("gfx/inv_mjolnir", CACHEPICFLAG_QUIET);
			hsb_weapons[0][2] = Draw_CachePic_Flags ("gfx/inv_gren_prox", CACHEPICFLAG_QUIET);
			hsb_weapons[0][3] = Draw_CachePic_Flags ("gfx/inv_prox_gren", CACHEPICFLAG_QUIET);
			hsb_weapons[0][4] = Draw_CachePic_Flags ("gfx/inv_prox", CACHEPICFLAG_QUIET);

			hsb_weapons[1][0] = Draw_CachePic_Flags ("gfx/inv2_laser", CACHEPICFLAG_QUIET);
			hsb_weapons[1][1] = Draw_CachePic_Flags ("gfx/inv2_mjolnir", CACHEPICFLAG_QUIET);
			hsb_weapons[1][2] = Draw_CachePic_Flags ("gfx/inv2_gren_prox", CACHEPICFLAG_QUIET);
			hsb_weapons[1][3] = Draw_CachePic_Flags ("gfx/inv2_prox_gren", CACHEPICFLAG_QUIET);
			hsb_weapons[1][4] = Draw_CachePic_Flags ("gfx/inv2_prox", CACHEPICFLAG_QUIET);

			for (i = 0;i < 5;i++)
			{
				hsb_weapons[2+i][0] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_laser",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][1] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_mjolnir",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][2] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_gren_prox",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][3] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_prox_gren",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][4] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%i_prox",i+1), CACHEPICFLAG_QUIET);
			}

			hsb_items[0] = Draw_CachePic_Flags ("gfx/sb_wsuit", CACHEPICFLAG_QUIET);
			hsb_items[1] = Draw_CachePic_Flags ("gfx/sb_eshld", CACHEPICFLAG_QUIET);
		}
		else if (gamemode == GAME_ROGUE)
		{
			rsb_invbar[0] = Draw_CachePic_Flags ("gfx/r_invbar1", CACHEPICFLAG_QUIET);
			rsb_invbar[1] = Draw_CachePic_Flags ("gfx/r_invbar2", CACHEPICFLAG_QUIET);

			rsb_weapons[0] = Draw_CachePic_Flags ("gfx/r_lava", CACHEPICFLAG_QUIET);
			rsb_weapons[1] = Draw_CachePic_Flags ("gfx/r_superlava", CACHEPICFLAG_QUIET);
			rsb_weapons[2] = Draw_CachePic_Flags ("gfx/r_gren", CACHEPICFLAG_QUIET);
			rsb_weapons[3] = Draw_CachePic_Flags ("gfx/r_multirock", CACHEPICFLAG_QUIET);
			rsb_weapons[4] = Draw_CachePic_Flags ("gfx/r_plasma", CACHEPICFLAG_QUIET);

			rsb_items[0] = Draw_CachePic_Flags ("gfx/r_shield1", CACHEPICFLAG_QUIET);
			rsb_items[1] = Draw_CachePic_Flags ("gfx/r_agrav1", CACHEPICFLAG_QUIET);

	// PGM 01/19/97 - team color border
			rsb_teambord = Draw_CachePic_Flags ("gfx/r_teambord", CACHEPICFLAG_QUIET);
	// PGM 01/19/97 - team color border

			rsb_ammo[0] = Draw_CachePic_Flags ("gfx/r_ammolava", CACHEPICFLAG_QUIET);
			rsb_ammo[1] = Draw_CachePic_Flags ("gfx/r_ammomulti", CACHEPICFLAG_QUIET);
			rsb_ammo[2] = Draw_CachePic_Flags ("gfx/r_ammoplasma", CACHEPICFLAG_QUIET);
		}
	}

	sb_ranking = Draw_CachePic_Flags ("gfx/ranking", CACHEPICFLAG_QUIET);
	sb_complete = Draw_CachePic_Flags ("gfx/complete", CACHEPICFLAG_QUIET);
	sb_inter = Draw_CachePic_Flags ("gfx/inter", CACHEPICFLAG_QUIET);
	sb_finale = Draw_CachePic_Flags ("gfx/finale", CACHEPICFLAG_QUIET);
}

static void sbar_shutdown(void)
{
}

static void sbar_newmap(void)
{
}

void Sbar_Init (void)
{
	Cmd_AddCommand(CF_CLIENT, "+showscores", Sbar_ShowScores_f, "show scoreboard");
	Cmd_AddCommand(CF_CLIENT, "-showscores", Sbar_DontShowScores_f, "hide scoreboard");
	Cvar_RegisterVariable(&cl_showfps);
	Cvar_RegisterVariable(&cl_showsound);
	Cvar_RegisterVariable(&cl_showblur);
	Cvar_RegisterVariable(&cl_showspeed);
	Cvar_RegisterVariable(&cl_showspeed_factor);
	Cvar_RegisterVariable(&cl_showtopspeed);
	Cvar_RegisterVariable(&cl_showtime);
	Cvar_RegisterVariable(&cl_showtime_format);
	Cvar_RegisterVariable(&cl_showdate);
	Cvar_RegisterVariable(&cl_showdate_format);
	Cvar_RegisterVariable(&cl_showtex);
	
	Cvar_RegisterVirtual(&cl_showfps, "showfps");
	Cvar_RegisterVirtual(&cl_showsound, "showsound");
	Cvar_RegisterVirtual(&cl_showblur, "showblur");
	Cvar_RegisterVirtual(&cl_showspeed, "showspeed");
	Cvar_RegisterVirtual(&cl_showtopspeed, "showtopspeed");
	Cvar_RegisterVirtual(&cl_showtime, "showtime");
	Cvar_RegisterVirtual(&cl_showtime_format, "showtime_format");
	Cvar_RegisterVirtual(&cl_showdate, "showdate");
	Cvar_RegisterVirtual(&cl_showdate_format, "showdate_format");
	Cvar_RegisterVirtual(&cl_showtex, "showtex");
	
	Cvar_RegisterVariable(&sbar_alpha_bg);
	Cvar_RegisterVariable(&sbar_alpha_fg);
	Cvar_RegisterVariable(&sbar_hudselector);
	Cvar_RegisterVariable(&sbar_scorerank);
	Cvar_RegisterVariable(&sbar_gametime);
	Cvar_RegisterVariable(&sbar_miniscoreboard_size);
	Cvar_RegisterVariable(&sbar_info_pos);
	Cvar_RegisterVariable(&cl_deathscoreboard);
	// This cvar is found in Fitzquake-derived engines and FTEQW and is read by the Alkaline 1.2 and Arcane Dimensions 1.80 CSQC
	Cvar_RegisterVirtual(&sbar_alpha_bg, "scr_sbaralpha");

	Cvar_RegisterVariable(&crosshair_color_red);
	Cvar_RegisterVariable(&crosshair_color_green);
	Cvar_RegisterVariable(&crosshair_color_blue);
	Cvar_RegisterVariable(&crosshair_color_alpha);
	Cvar_RegisterVariable(&crosshair_size);

	Cvar_RegisterVariable(&sbar_flagstatus_right); // (GAME_NEXUZI ONLY)
	Cvar_RegisterVariable(&sbar_flagstatus_pos); // (GAME_NEXUIZ ONLY)

	R_RegisterModule("sbar", sbar_start, sbar_shutdown, sbar_newmap, NULL, NULL);
}


//=============================================================================

// drawing routines are relative to the status bar location

int sbar_x, sbar_y;

/*
=============
Sbar_DrawPic
=============
*/
static void Sbar_DrawStretchPic (int x, int y, cachepic_t *pic, float alpha, float overridewidth, float overrideheight)
{
	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, overridewidth, overrideheight, 1, 1, 1, alpha, 0);
}

static void Sbar_DrawPic (int x, int y, cachepic_t *pic)
{
	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, 0, 0, 1, 1, 1, sbar_alpha_fg.value, 0);
}

static void Sbar_DrawAlphaPic (int x, int y, cachepic_t *pic, float alpha)
{
	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, 0, 0, 1, 1, 1, alpha, 0);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
static void Sbar_DrawCharacter (int x, int y, int num)
{
	char vabuf[1024];
	DrawQ_String (sbar_x + x + 4 , sbar_y + y, va(vabuf, sizeof(vabuf), "%c", num), 0, 8, 8, 1, 1, 1, sbar_alpha_fg.value, 0, NULL, true, FONT_SBAR);
}

/*
================
Sbar_DrawString
================
*/
static void Sbar_DrawString (int x, int y, char *str)
{
	DrawQ_String (sbar_x + x, sbar_y + y, str, 0, 8, 8, 1, 1, 1, sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR);
}

/*
=============
Sbar_DrawNum
=============
*/
static void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char str[32], *ptr;
	int l, frame;

	l = dpsnprintf(str, sizeof(str), "%i", num);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawPic (x, y, sb_nums[color][frame]);
		x += 24;

		ptr++;
	}
}

/*
=============
Sbar_DrawXNum
=============
*/

static void Sbar_DrawXNum (int x, int y, int num, int digits, int lettersize, float r, float g, float b, float a, int flags)
{
	char str[32], *ptr;
	int l, frame;

	if (digits < 0)
	{
		digits = -digits;
		l = dpsnprintf(str, sizeof(str), "%0*i", digits, num);
	}
	else
		l = dpsnprintf(str, sizeof(str), "%i", num);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l) * lettersize;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		DrawQ_Pic (sbar_x + x, sbar_y + y, sb_nums[0][frame],lettersize,lettersize,r,g,b,a * sbar_alpha_fg.value,flags);
		x += lettersize;

		ptr++;
	}
}

//=============================================================================


static int Sbar_IsTeammatch(void)
{
	// currently only nexuiz uses the team score board
	return (IS_OLDNEXUIZ_DERIVED(gamemode)
		&& (teamplay.integer > 0));
}

/*
===============
Sbar_SortFrags
===============
*/
static int fragsort[MAX_SCOREBOARD];
static int scoreboardlines;

int Sbar_GetSortedPlayerIndex (int index)
{
	return index >= 0 && index < scoreboardlines ? fragsort[index] : -1;
}

static scoreboard_t teams[MAX_SCOREBOARD];
static int teamsort[MAX_SCOREBOARD];
static int teamlines;
void Sbar_SortFrags (void)
{
	int i, j, k, color;

	// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.maxclients ; i++)
	{
		if (cl.scores[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.scores[fragsort[j]].frags < cl.scores[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}

	teamlines = 0;
	if (Sbar_IsTeammatch ())
	{
		// now sort players by teams.
		for (i=0 ; i<scoreboardlines ; i++)
		{
			for (j=0 ; j<scoreboardlines-1-i ; j++)
			{
				if (cl.scores[fragsort[j]].colors < cl.scores[fragsort[j+1]].colors)
				{
					k = fragsort[j];
					fragsort[j] = fragsort[j+1];
					fragsort[j+1] = k;
				}
			}
		}

		// calculate team scores
		color = -1;
		for (i=0 ; i<scoreboardlines ; i++)
		{
			if (color != (cl.scores[fragsort[i]].colors & 15))
			{
				const char* teamname;

				color = cl.scores[fragsort[i]].colors & 15;
				teamlines++;

				switch (color)
				{
					case 4:
						teamname = "^1Red Team";
						break;
					case 13:
						teamname = "^4Blue Team";
						break;
					case 9:
						teamname = "^6Pink Team";
						break;
					case 12:
						teamname = "^3Yellow Team";
						break;
					default:
						teamname = "Total Team Score";
						break;
				}
				dp_strlcpy(teams[teamlines-1].name, teamname, sizeof(teams[teamlines-1].name));

				teams[teamlines-1].frags = 0;
				teams[teamlines-1].colors = color + 16 * color;
			}

			if (cl.scores[fragsort[i]].frags != -666)
			{
				// do not add spedcators
				// (ugly hack for nexuiz)
				teams[teamlines-1].frags += cl.scores[fragsort[i]].frags;
			}
		}

		// now sort teams by scores.
		for (i=0 ; i<teamlines ; i++)
			teamsort[i] = i;
		for (i=0 ; i<teamlines ; i++)
		{
			for (j=0 ; j<teamlines-1-i ; j++)
			{
				if (teams[teamsort[j]].frags < teams[teamsort[j+1]].frags)
				{
					k = teamsort[j];
					teamsort[j] = teamsort[j+1];
					teamsort[j+1] = k;
				}
			}
		}
	}
}

/*
===============
Sbar_SoloScoreboard
===============
*/
static void Sbar_SoloScoreboard (void)
{
#if 1
	char	str[80], timestr[40];
	int		max, timelen;
	int		minutes, seconds;
	double	t;
	char vabuf[1024];

	t = (cl.intermission ? cl.completed_time : cl.time);
	minutes = (int)(t / 60);
	seconds = (int)(t - 60*floor(t/60));

	// monsters and secrets are now both on the top row
	if (cl.stats[STAT_TOTALMONSTERS])
		Sbar_DrawString(8, 4, va(vabuf, sizeof(vabuf), "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]));
	else if (cl.stats[STAT_MONSTERS]) // LA: Display something if monsters_killed is non-zero, but total_monsters is zero
		Sbar_DrawString(8, 4, va(vabuf, sizeof(vabuf), "Monsters:%3i", cl.stats[STAT_MONSTERS]));

	if (cl.stats[STAT_TOTALSECRETS])
		Sbar_DrawString(8+22*8, 4, va(vabuf, sizeof(vabuf), "Secrets:%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]));
	else if (cl.stats[STAT_SECRETS]) // LA: And similarly for secrets
		Sbar_DrawString(8+22*8, 4, va(vabuf, sizeof(vabuf), "Secrets:%3i", cl.stats[STAT_SECRETS]));

	// format is like this: e1m1:The Sligpate Complex
	dpsnprintf(str, sizeof(str), "%s:%.39s", cl.worldbasename, cl.worldmessage);

	// if there's a newline character, terminate the string there
	if (strchr(str, '\n'))
		*(strchr(str, '\n')) = 0;

	// make the time string
	timelen = dpsnprintf(timestr, sizeof(timestr), " %i:%02i", minutes, seconds);

	// truncate the level name if necessary to make room for time
	max = 38 - timelen;
	if ((int)strlen(str) > max)
		str[max] = 0;

	// print the filename and message
	Sbar_DrawString(8, 12, str);

	// print the time
	Sbar_DrawString(8 + max*8, 12, timestr);

#else
	char	str[80];
	int		minutes, seconds, tens, units;
	int		l;

	if (IS_OLDNEXUIZ_DERIVED(gamemode)) {
		dpsnprintf (str, sizeof(str), "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
		Sbar_DrawString (8, 4, str);

		dpsnprintf (str, sizeof(str), "Secrets :%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
		Sbar_DrawString (8, 12, str);
	}

// time
	minutes = (int)(cl.time / 60);
	seconds = (int)(cl.time - 60*minutes);
	tens = seconds / 10;
	units = seconds - 10*tens;
	dpsnprintf (str, sizeof(str), "Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

// draw level name
	if (IS_OLDNEXUIZ_DERIVED(gamemode)) {
		l = (int) strlen (cl.worldname);
		Sbar_DrawString (232 - l*4, 12, cl.worldname);
	} else {
		l = (int) strlen (cl.worldmessage);
		Sbar_DrawString (232 - l*4, 12, cl.worldmessage);
	}
#endif
}

/*
===============
Sbar_DrawScoreboard
===============
*/
static void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	// LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode
	//if (cl.gametype == GAME_DEATHMATCH)
	if (!cl.islocalgame)
		Sbar_DeathmatchOverlay ();
}

//=============================================================================

// AK to make DrawInventory smaller
static void Sbar_DrawWeapon(int nr, float fade, int active)
{
	char vabuf[1024];
	if (sbar_hudselector.integer == 1)
	{
		// width = 300, height = 100
		const int w_width = 32, w_height = 12, w_space = 2, font_size = 8;

		DrawQ_Pic((vid_conwidth.integer - w_width * 9) * 0.5 + w_width * nr, vid_conheight.integer - w_height, sb_weapons[0][nr], w_width, w_height, (active) ? 1 : 0.6, active ? 1 : 0.6, active ? 1 : 0.6, (active ? 1 : 0.6) * fade * sbar_alpha_fg.value, DRAWFLAG_NORMAL);
		// FIXME ??
		DrawQ_String((vid_conwidth.integer - w_width * 9) * 0.5 + w_width * nr + w_space, vid_conheight.integer - w_height + w_space, va(vabuf, sizeof(vabuf), "%i",nr+1), 0, font_size, font_size, 1, 1, 0, sbar_alpha_fg.value, 0, NULL, true, FONT_DEFAULT);
	}
	else
	{
		// width = 300, height = 100
		const int w_width = 300, w_height = 100, w_space = 10;
		const float w_scale = 0.4;

		DrawQ_Pic(vid_conwidth.integer - (w_width + w_space) * w_scale, (w_height + w_space) * w_scale * nr + w_space, sb_weapons[0][nr], w_width * w_scale, w_height * w_scale, (active) ? 1 : 0.6, active ? 1 : 0.6, active ? 1 : 1, fade * sbar_alpha_fg.value, DRAWFLAG_NORMAL);
		//DrawQ_String(vid_conwidth.integer - (w_space + font_size ), (w_height + w_space) * w_scale * nr + w_space, va(vabuf, sizeof(vabuf), "%i",nr+1), 0, font_size, font_size, 1, 0, 0, fade, 0, NULL, true, FONT_DEFAULT);
	}
}

/*
===============
Sbar_DrawInventory
===============
*/
static void Sbar_DrawInventory (void)
{
	int i;
	char num[6];
	float time;
	int flashon;

	if (gamemode == GAME_ROGUE)
	{
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			Sbar_DrawAlphaPic (0, -24, rsb_invbar[0], sbar_alpha_bg.value);
		else
			Sbar_DrawAlphaPic (0, -24, rsb_invbar[1], sbar_alpha_bg.value);
	}
	else
		Sbar_DrawAlphaPic (0, -24, sb_ibar, sbar_alpha_bg.value);

	// weapons
	for (i=0 ; i<7 ; i++)
	{
		if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN<<i) )
		{
			time = cl.item_gettime[i];
			flashon = (int)(max(0, cl.time - time)*10);
			if (flashon >= 10)
			{
				if ( cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon%5) + 2;

			Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);
		}
	}

	// MED 01/04/97
	// hipnotic weapons
	if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
	{
		int grenadeflashing=0;
		for (i=0 ; i<4 ; i++)
		{
			if (cl.stats[STAT_ITEMS] & (1<<hipweapons[i]) )
			{
				time = max(0, cl.item_gettime[hipweapons[i]]);
				flashon = (int)((cl.time - time)*10);
				if (flashon >= 10)
				{
					if ( cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i])  )
						flashon = 1;
					else
						flashon = 0;
				}
				else
					flashon = (flashon%5) + 2;

				// check grenade launcher
				if (i==2)
				{
					if (cl.stats[STAT_ITEMS] & HIT_PROXIMITY_GUN)
					{
						if (flashon)
						{
							grenadeflashing = 1;
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
						}
					}
				}
				else if (i==3)
				{
					if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN<<4))
					{
						if (!grenadeflashing)
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
					}
					else
						Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
				}
				else
					Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);
			}
		}
	}

	if (gamemode == GAME_ROGUE)
	{
		// check for powered up weapon.
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			for (i=0;i<5;i++)
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
	}

	// ammo counts
	for (i=0 ; i<4 ; i++)
	{
		dpsnprintf (num, sizeof(num), "%4i",cl.stats[STAT_SHELLS+i] );
		if (num[0] != ' ')
			Sbar_DrawCharacter ( (6*i+0)*8 - 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ( (6*i+1)*8 - 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ( (6*i+2)*8 - 2, -24, 18 + num[2] - '0');
		if (num[3] != ' ')
			Sbar_DrawCharacter ( (6*i+3)*8 - 2, -24, 18 + num[3] - '0');
	}

	// items
	for (i=0 ; i<6 ; i++)
		if (cl.stats[STAT_ITEMS] & (1u<<(17+i)))
		{
			//MED 01/04/97 changed keys
			if (!(gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) || (i>1))
				Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
		}

	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
	{
		for (i=0 ; i<2 ; i++)
			if (cl.stats[STAT_ITEMS] & (1u<<(24+i)))
				Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
	}

	if (gamemode == GAME_ROGUE)
	{
		// new rogue items
		for (i=0 ; i<2 ; i++)
			if (cl.stats[STAT_ITEMS] & (1u<<(29+i)))
				Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
	}
	else
	{
		// sigils
		for (i=0 ; i<4 ; i++)
			if (cl.stats[STAT_ITEMS] & (1u<<(28+i)))
				Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
static void Sbar_DrawFrags (void)
{
	int i, k, l, x, f;
	char num[12];
	scoreboard_t *s;
	unsigned char *c;

	Sbar_SortFrags ();

	// draw the text
	l = min(scoreboardlines, 4);

	x = 23 * 8;

	for (i = 0;i < l;i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];

		// draw background
		c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
		DrawQ_Fill (sbar_x + x + 10, sbar_y     - 23, 28, 4, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
		c = palette_rgb_shirtscoreboard[s->colors & 0xf];
		DrawQ_Fill (sbar_x + x + 10, sbar_y + 4 - 23, 28, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);

		// draw number
		f = s->frags;
		dpsnprintf (num, sizeof(num), "%3i",f);

		if (k == cl.viewentity - 1)
		{
			Sbar_DrawCharacter ( x      + 2, -24, 16);
			Sbar_DrawCharacter ( x + 32 - 4, -24, 17);
		}
		Sbar_DrawCharacter (x +  8, -24, num[0]);
		Sbar_DrawCharacter (x + 16, -24, num[1]);
		Sbar_DrawCharacter (x + 24, -24, num[2]);
		x += 32;
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
static void Sbar_DrawFace (void)
{
	int f;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if (gamemode == GAME_ROGUE && !cl.islocalgame && (teamplay.integer > 3) && (teamplay.integer < 7))
	{
		char num[12];
		scoreboard_t *s;
		unsigned char *c;

		s = &cl.scores[cl.viewentity - 1];
		// draw background
		Sbar_DrawPic (112, 0, rsb_teambord);
		c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
		DrawQ_Fill (sbar_x + 113, vid_conheight.integer-SBAR_HEIGHT+3, 22, 9, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
		c = palette_rgb_shirtscoreboard[s->colors & 0xf];
		DrawQ_Fill (sbar_x + 113, vid_conheight.integer-SBAR_HEIGHT+12, 22, 9, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);

		// draw number
		f = s->frags;
		dpsnprintf (num, sizeof(num), "%3i",f);

		if ((s->colors & 0xf0)==0)
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter ( 109, 3, num[0]);
			Sbar_DrawCharacter ( 116, 3, num[1]);
			Sbar_DrawCharacter ( 123, 3, num[2]);
		}

		return;
	}
// PGM 01/19/97 - team color drawing

	if ( (cl.stats[STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY) ) == (IT_INVISIBILITY | IT_INVULNERABILITY) )
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
	else if (cl.stats[STAT_ITEMS] & IT_QUAD)
		Sbar_DrawPic (112, 0, sb_face_quad );
	else if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
		Sbar_DrawPic (112, 0, sb_face_invis );
	else if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
		Sbar_DrawPic (112, 0, sb_face_invuln);
	else
	{
		f = cl.stats[STAT_HEALTH] / 20;
		f = bound(0, f, 4);
		Sbar_DrawPic (112, 0, sb_faces[f][cl.time <= cl.faceanimtime]);
	}
}
double topspeed = 0;
double topspeedxy = 0;
time_t current_time = 3;
time_t top_time = 0;
time_t topxy_time = 0;

static void get_showspeed_unit(int unitnumber, double *conversion_factor, const char **unit)
{
	if(unitnumber < 0)
		unitnumber = cl_showspeed.integer;
	switch(unitnumber)
	{
		default:
		case 1:
			*unit = "qu/s";
			*conversion_factor = 1.0;
			break;
		case 2:
			*unit = "m/s";
			*conversion_factor = 1.0 * cl_showspeed_factor.value * 0.01;
			break;
		case 3:
			*unit = "km/h";
			*conversion_factor = 1.0 * (cl_showspeed_factor.value * 0.01) * 3.6;
			break;
		case 4:
			*unit = "mph";
			*conversion_factor = 1.0 * (cl_showspeed_factor.value * 0.01 * 3.6) * 0.6213711922;
			break;
		case 5:
			*unit = "knots";
			*conversion_factor = 1.0 * (cl_showspeed_factor.value * 0.01 * 3.6) * 0.539957;
			break;
	}
}

static double showfps_nexttime = 0, showfps_lasttime = -1;
static double showfps_framerate = 0;
static int showfps_framecount = 0;

void Sbar_ShowFPS_Update(void)
{
	double interval = 1;
	double newtime;
	newtime = host.realtime;
	if (newtime >= showfps_nexttime)
	{
		showfps_framerate = showfps_framecount / (newtime - showfps_lasttime);
		if (showfps_nexttime < newtime - interval * 1.5)
			showfps_nexttime = newtime;
		showfps_lasttime = newtime;
		showfps_nexttime += interval;
		showfps_framecount = 0;
	}
	showfps_framecount++;
}

void Sbar_ShowFPS(void)
{
	float fps_x, fps_y, fps_scalex, fps_scaley, fps_strings = 0;
	char soundstring[32];
	char fpsstring[32];
	char timestring[32];
	char datestring[32];
	char timedemostring1[32];
	char timedemostring2[32];
	char speedstring[32];
	char blurstring[32];
	char topspeedstring[48];
	char texstring[MAX_QPATH];
	char entstring[32];
	qbool red = false;
	soundstring[0] = 0;
	fpsstring[0] = 0;
	timedemostring1[0] = 0;
	timedemostring2[0] = 0;
	timestring[0] = 0;
	datestring[0] = 0;
	speedstring[0] = 0;
	blurstring[0] = 0;
	topspeedstring[0] = 0;
	texstring[0] = 0;
	entstring[0] = 0;
	if (cl_showfps.integer)
	{
		red = (showfps_framerate < 1.0f);
		if(cl_showfps.integer == 2)
			dpsnprintf(fpsstring, sizeof(fpsstring), "%7.3f mspf", (1000.0 / showfps_framerate));
		else if (red)
			dpsnprintf(fpsstring, sizeof(fpsstring), "%4i spf", (int)(1.0 / showfps_framerate + 0.5));
		else
			dpsnprintf(fpsstring, sizeof(fpsstring), "%4i fps", (int)(showfps_framerate + 0.5));
		fps_strings++;
		if (cls.timedemo)
		{
			dpsnprintf(timedemostring1, sizeof(timedemostring1), "frame%4i %f", cls.td_frames, host.realtime - cls.td_starttime);
			dpsnprintf(timedemostring2, sizeof(timedemostring2), "%i seconds %3.0f/%3.0f/%3.0f fps", cls.td_onesecondavgcount, cls.td_onesecondminfps, cls.td_onesecondavgfps / max(1, cls.td_onesecondavgcount), cls.td_onesecondmaxfps);
			fps_strings++;
			fps_strings++;
		}
	}
	if (cl_showtime.integer)
	{
		Sys_TimeString(timestring, sizeof(timestring), cl_showtime_format.string);
		fps_strings++;
	}
	if (cl_showdate.integer)
	{
		Sys_TimeString(datestring, sizeof(datestring), cl_showdate_format.string);
		fps_strings++;
	}
	if (cl_showblur.integer)
	{
		dpsnprintf(blurstring, sizeof(blurstring), "%3i%% blur", (int)(cl.motionbluralpha * 100));
		fps_strings++;
	}
	if (cl_showsound.integer)
	{
		dpsnprintf(soundstring, sizeof(soundstring), "%4i/4%i at %3ims", cls.soundstats.mixedsounds, cls.soundstats.totalsounds, cls.soundstats.latency_milliseconds);
		fps_strings++;
	}
	if (cl_showspeed.integer || cl_showtopspeed.integer)
	{
		double speed, speedxy, f;
		const char *unit;
		speed = VectorLength(cl.movement_velocity);
		speedxy = sqrt(cl.movement_velocity[0] * cl.movement_velocity[0] + cl.movement_velocity[1] * cl.movement_velocity[1]);
		if (cl_showspeed.integer)
		{
			get_showspeed_unit(cl_showspeed.integer, &f, &unit);
			dpsnprintf(speedstring, sizeof(speedstring), "%.0f (%.0f) %s", f*speed, f*speedxy, unit);
			fps_strings++;
		}
		if (cl_showtopspeed.integer)
		{
			qbool topspeed_latched = false, topspeedxy_latched = false;
			get_showspeed_unit(cl_showtopspeed.integer, &f, &unit);
			if (speed >= topspeed || current_time - top_time > 3)
			{
				topspeed = speed;
				time(&top_time);
			}
			else
				topspeed_latched = true;
			if (speedxy >= topspeedxy || current_time - topxy_time > 3)
			{
				topspeedxy = speedxy;
				time(&topxy_time);
			}
			else
				topspeedxy_latched = true;
			dpsnprintf(topspeedstring, sizeof(topspeedstring), "%s%.0f%s (%s%.0f%s) %s",
				topspeed_latched ? "^1" : "^xf88", f*topspeed, "^xf88",
				topspeedxy_latched ? "^1" : "^xf88", f*topspeedxy, "^xf88",
				unit);
			time(&current_time);
			fps_strings++;
		}
	}
	if (cl_showtex.integer)
	{
		vec3_t org;
		vec3_t dest;
		vec3_t temp;
		trace_t svtrace, cltrace;
		int hitnetentity = -1;

		Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&r_refdef.view.matrix, temp, dest);
		// clear the traces as we may or may not fill them out, and mark them with an invalid fraction so we know if we did
		memset(&svtrace, 0, sizeof(svtrace));
		memset(&cltrace, 0, sizeof(cltrace));
		svtrace.fraction = 2.0;
		cltrace.fraction = 2.0;
		// ray hits models (even animated ones) and ignores translucent materials
		if (sv.active)
			svtrace = SV_TraceLine(org, dest, MOVE_HITMODEL, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value);
		cltrace = CL_TraceLine(org, dest, MOVE_HITMODEL, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, &hitnetentity, true, true);
		if (cltrace.hittexture)
			dp_strlcpy(texstring, cltrace.hittexture->name, sizeof(texstring));
		else
			dp_strlcpy(texstring, "(no texture hit)", sizeof(texstring));
		fps_strings++;
		if (svtrace.fraction < cltrace.fraction)
		{
			if (svtrace.ent != NULL)
			{
				prvm_prog_t *prog = SVVM_prog;
				dpsnprintf(entstring, sizeof(entstring), "server entity %i", (int)PRVM_EDICT_TO_PROG(svtrace.ent));
			}
			else
				dp_strlcpy(entstring, "(no entity hit)", sizeof(entstring));
		}
		else
		{
			if (cltrace.ent != NULL)
			{
				prvm_prog_t *prog = CLVM_prog;
				dpsnprintf(entstring, sizeof(entstring), "client entity %i", (int)PRVM_EDICT_TO_PROG(cltrace.ent));
			}
			else if (hitnetentity > 0)
				dpsnprintf(entstring, sizeof(entstring), "network entity %i", hitnetentity);
			else if (hitnetentity == 0)
				dp_strlcpy(entstring, "world entity", sizeof(entstring));
			else
				dp_strlcpy(entstring, "(no entity hit)", sizeof(entstring));
		}
		fps_strings++;
	}
	if (fps_strings)
	{
		fps_scalex = 12;
		fps_scaley = 12;
		//fps_y = vid_conheight.integer - sb_lines; // yes this may draw over the sbar
		//fps_y = bound(0, fps_y, vid_conheight.integer - fps_strings*fps_scaley);
		fps_y = vid_conheight.integer - sbar_info_pos.integer - fps_strings*fps_scaley;
		if (soundstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(soundstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, soundstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (fpsstring[0])
		{
			r_draw2d_force = true;
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(fpsstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			if (red)
				DrawQ_String(fps_x, fps_y, fpsstring, 0, fps_scalex, fps_scaley, 1, 0, 0, 1, 0, NULL, true, FONT_INFOBAR);
			else
				DrawQ_String(fps_x, fps_y, fpsstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
			r_draw2d_force = false;
		}
		if (timedemostring1[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timedemostring1, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timedemostring1, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (timedemostring2[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timedemostring2, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timedemostring2, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (timestring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timestring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timestring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (datestring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(datestring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, datestring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (speedstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(speedstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, speedstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (topspeedstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(topspeedstring, 0, fps_scalex, fps_scaley, false, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, topspeedstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, false, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (blurstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(blurstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, blurstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (texstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(texstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, texstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (entstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(entstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, entstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
	}
}

static void Sbar_DrawGauge(float x, float y, cachepic_t *pic, float width, float height, float rangey, float rangeheight, float c1, float c2, float c1r, float c1g, float c1b, float c1a, float c2r, float c2g, float c2b, float c2a, float c3r, float c3g, float c3b, float c3a, int drawflags)
{
	float r[5];
	c2 = bound(0, c2, 1);
	c1 = bound(0, c1, 1 - c2);
	r[0] = 0;
	r[1] = rangey + rangeheight * (c2 + c1);
	r[2] = rangey + rangeheight * (c2);
	r[3] = rangey;
	r[4] = height;
	if (r[1] > r[0])
		DrawQ_SuperPic(x, y + r[0], pic, width, (r[1] - r[0]), 0,(r[0] / height), c3r,c3g,c3b,c3a, 1,(r[0] / height), c3r,c3g,c3b,c3a, 0,(r[1] / height), c3r,c3g,c3b,c3a, 1,(r[1] / height), c3r,c3g,c3b,c3a, drawflags);
	if (r[2] > r[1])
		DrawQ_SuperPic(x, y + r[1], pic, width, (r[2] - r[1]), 0,(r[1] / height), c1r,c1g,c1b,c1a, 1,(r[1] / height), c1r,c1g,c1b,c1a, 0,(r[2] / height), c1r,c1g,c1b,c1a, 1,(r[2] / height), c1r,c1g,c1b,c1a, drawflags);
	if (r[3] > r[2])
		DrawQ_SuperPic(x, y + r[2], pic, width, (r[3] - r[2]), 0,(r[2] / height), c2r,c2g,c2b,c2a, 1,(r[2] / height), c2r,c2g,c2b,c2a, 0,(r[3] / height), c2r,c2g,c2b,c2a, 1,(r[3] / height), c2r,c2g,c2b,c2a, drawflags);
	if (r[4] > r[3])
		DrawQ_SuperPic(x, y + r[3], pic, width, (r[4] - r[3]), 0,(r[3] / height), c3r,c3g,c3b,c3a, 1,(r[3] / height), c3r,c3g,c3b,c3a, 0,(r[4] / height), c3r,c3g,c3b,c3a, 1,(r[4] / height), c3r,c3g,c3b,c3a, drawflags);
}

/*
===============
Sbar_Draw
===============
*/
extern float v_dmg_time, v_dmg_roll, v_dmg_pitch;
extern cvar_t v_kicktime;
void Sbar_Score (int margin);
void Sbar_Draw (void)
{
	cachepic_t *pic;
	char vabuf[1024];

	if(cl.csqc_vidvars.drawenginesbar)	//[515]: csqc drawsbar
	{
		if (sb_showscores)
			Sbar_DrawScoreboard ();
		else if (cl.intermission == 1)
		{
			if(IS_OLDNEXUIZ_DERIVED(gamemode)) // display full scoreboard (that is, show scores + map name)
			{
				Sbar_DrawScoreboard();
				return;
			}
			Sbar_IntermissionOverlay();
		}
		else if (cl.intermission == 2)
			Sbar_FinaleOverlay();
		else if (gamemode == GAME_DELUXEQUAKE)
		{
		}
		else if (IS_OLDNEXUIZ_DERIVED(gamemode))
		{
			if (sb_showscores || (cl.stats[STAT_HEALTH] <= 0 && cl_deathscoreboard.integer))
			{
				sbar_x = (vid_conwidth.integer - 640)/2;
				sbar_y = vid_conheight.integer - 47;
				Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
				Sbar_DrawScoreboard ();
			}
			else if (sb_lines && sbar_hudselector.integer == 1)
			{
				int i;
				float fade;
				int redflag, blueflag;
				float x;

				sbar_x = (vid_conwidth.integer - 320)/2;
				sbar_y = vid_conheight.integer - 24 - 16;

				// calculate intensity to draw weapons bar at
				fade = 3.2 - 2 * (cl.time - cl.weapontime);
				fade = bound(0.7, fade, 1);
				for (i = 0; i < 8;i++)
					if (cl.stats[STAT_ITEMS] & (1 << i))
						Sbar_DrawWeapon(i + 1, fade, (i + 2 == cl.stats[STAT_ACTIVEWEAPON]));
				if((cl.stats[STAT_ITEMS] & (1<<12)))
					Sbar_DrawWeapon(0, fade, (cl.stats[STAT_ACTIVEWEAPON] == 1));

				// flag icons
				redflag = ((cl.stats[STAT_ITEMS]>>15) & 3);
				blueflag = ((cl.stats[STAT_ITEMS]>>17) & 3);
				x = sbar_flagstatus_right.integer ? vid_conwidth.integer - 10 - sbar_x - 64 : 10 - sbar_x;
				if (redflag == 3 && blueflag == 3)
				{
					// The Impossible Combination[tm]
					// Can only happen in Key Hunt mode...
					Sbar_DrawPic ((int) x, (int) ((vid_conheight.integer - sbar_y) - (sbar_flagstatus_pos.value + 128)), sb_items[14]);
				}
				else
				{
					if (redflag)
						Sbar_DrawPic ((int) x, (int) ((vid_conheight.integer - sbar_y) - (sbar_flagstatus_pos.value + 64)), sb_items[redflag+10]);
					if (blueflag)
						Sbar_DrawPic ((int) x, (int) ((vid_conheight.integer - sbar_y) - (sbar_flagstatus_pos.value + 128)), sb_items[blueflag+14]);
				}

				// armor
				if (cl.stats[STAT_ARMOR] > 0)
				{
					Sbar_DrawStretchPic (72, 0, sb_armor[0], sbar_alpha_fg.value, 24, 24);
					if(cl.stats[STAT_ARMOR] > 200)
						Sbar_DrawXNum(0,0,cl.stats[STAT_ARMOR],3,24,0,1,0,1,0);
					else if(cl.stats[STAT_ARMOR] > 100)
						Sbar_DrawXNum(0,0,cl.stats[STAT_ARMOR],3,24,0.2,1,0.2,1,0);
					else if(cl.stats[STAT_ARMOR] > 50)
						Sbar_DrawXNum(0,0,cl.stats[STAT_ARMOR],3,24,0.6,0.7,0.8,1,0);
					else if(cl.stats[STAT_ARMOR] > 25)
						Sbar_DrawXNum(0,0,cl.stats[STAT_ARMOR],3,24,1,1,0.2,1,0);
					else
						Sbar_DrawXNum(0,0,cl.stats[STAT_ARMOR],3,24,0.7,0,0,1,0);
				}

				// health
				if (cl.stats[STAT_HEALTH] != 0)
				{
					Sbar_DrawStretchPic (184, 0, sb_health, sbar_alpha_fg.value, 24, 24);
					if(cl.stats[STAT_HEALTH] > 200)
						Sbar_DrawXNum(112,0,cl.stats[STAT_HEALTH],3,24,0,1,0,1,0);
					else if(cl.stats[STAT_HEALTH] > 100)
						Sbar_DrawXNum(112,0,cl.stats[STAT_HEALTH],3,24,0.2,1,0.2,1,0);
					else if(cl.stats[STAT_HEALTH] > 50)
						Sbar_DrawXNum(112,0,cl.stats[STAT_HEALTH],3,24,0.6,0.7,0.8,1,0);
					else if(cl.stats[STAT_HEALTH] > 25)
						Sbar_DrawXNum(112,0,cl.stats[STAT_HEALTH],3,24,1,1,0.2,1,0);
					else
						Sbar_DrawXNum(112,0,cl.stats[STAT_HEALTH],3,24,0.7,0,0,1,0);
				}

				// ammo
				if ((cl.stats[STAT_ITEMS] & (NEX_IT_SHELLS | NEX_IT_BULLETS | NEX_IT_ROCKETS | NEX_IT_CELLS)) || cl.stats[STAT_AMMO] != 0)
				{
					if (cl.stats[STAT_ITEMS] & NEX_IT_SHELLS)
						Sbar_DrawStretchPic (296, 0, sb_ammo[0], sbar_alpha_fg.value, 24, 24);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_BULLETS)
						Sbar_DrawStretchPic (296, 0, sb_ammo[1], sbar_alpha_fg.value, 24, 24);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_ROCKETS)
						Sbar_DrawStretchPic (296, 0, sb_ammo[2], sbar_alpha_fg.value, 24, 24);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_CELLS)
						Sbar_DrawStretchPic (296, 0, sb_ammo[3], sbar_alpha_fg.value, 24, 24);
					if(cl.stats[STAT_AMMO] > 10)
						Sbar_DrawXNum(224, 0, cl.stats[STAT_AMMO], 3, 24, 0.6,0.7,0.8,1,0);
					else
						Sbar_DrawXNum(224, 0, cl.stats[STAT_AMMO], 3, 24, 0.7,0,0,1,0);
				}

				if (sbar_x + 320 + 160 <= vid_conwidth.integer)
					Sbar_MiniDeathmatchOverlay (sbar_x + 320, sbar_y);
				if (sbar_x > 0)
					Sbar_Score(16);
					// The margin can be at most 8 to support 640x480 console size:
					//   320 + 2 * (144 + 16) = 640
			}
			else if (sb_lines)
			{
				int i;
				float fade;
				int redflag, blueflag;
				float x;

				sbar_x = (vid_conwidth.integer - 640)/2;
				sbar_y = vid_conheight.integer - 47;

				// calculate intensity to draw weapons bar at
				fade = 3 - 2 * (cl.time - cl.weapontime);
				if (fade > 0)
				{
					fade = min(fade, 1);
					for (i = 0; i < 8;i++)
						if (cl.stats[STAT_ITEMS] & (1 << i))
							Sbar_DrawWeapon(i + 1, fade, (i + 2 == cl.stats[STAT_ACTIVEWEAPON]));

					if((cl.stats[STAT_ITEMS] & (1<<12)))
						Sbar_DrawWeapon(0, fade, (cl.stats[STAT_ACTIVEWEAPON] == 1));
				}

				//if (!cl.islocalgame)
				//	Sbar_DrawFrags ();

				if (sb_lines > 24)
					Sbar_DrawAlphaPic (0, 0, sb_sbar, sbar_alpha_fg.value);
				else
					Sbar_DrawAlphaPic (0, 0, sb_sbar_minimal, sbar_alpha_fg.value);

				// flag icons
				redflag = ((cl.stats[STAT_ITEMS]>>15) & 3);
				blueflag = ((cl.stats[STAT_ITEMS]>>17) & 3);
				x = sbar_flagstatus_right.integer ? vid_conwidth.integer - 10 - sbar_x - 64 : 10 - sbar_x;
				if (redflag == 3 && blueflag == 3)
				{
					// The Impossible Combination[tm]
					// Can only happen in Key Hunt mode...
					Sbar_DrawPic ((int) x, -179, sb_items[14]);
				}
				else
				{
					if (redflag)
						Sbar_DrawPic ((int) x, -117, sb_items[redflag+10]);
					if (blueflag)
						Sbar_DrawPic ((int) x, -177, sb_items[blueflag+14]);
				}

				// armor
				Sbar_DrawXNum ((340-3*24), 12, cl.stats[STAT_ARMOR], 3, 24, 0.6,0.7,0.8,1,0);

				// health
				if(cl.stats[STAT_HEALTH] > 100)
					Sbar_DrawXNum((154-3*24),12,cl.stats[STAT_HEALTH],3,24,1,1,1,1,0);
				else if(cl.stats[STAT_HEALTH] <= 25 && cl.time - (int)cl.time > 0.5)
					Sbar_DrawXNum((154-3*24),12,cl.stats[STAT_HEALTH],3,24,0.7,0,0,1,0);
				else
					Sbar_DrawXNum((154-3*24),12,cl.stats[STAT_HEALTH],3,24,0.6,0.7,0.8,1,0);

				// AK dont draw ammo for the laser
				if(cl.stats[STAT_ACTIVEWEAPON] != 12)
				{
					if (cl.stats[STAT_ITEMS] & NEX_IT_SHELLS)
						Sbar_DrawPic (519, 0, sb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_BULLETS)
						Sbar_DrawPic (519, 0, sb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_ROCKETS)
						Sbar_DrawPic (519, 0, sb_ammo[2]);
					else if (cl.stats[STAT_ITEMS] & NEX_IT_CELLS)
						Sbar_DrawPic (519, 0, sb_ammo[3]);

					if(cl.stats[STAT_AMMO] <= 10)
						Sbar_DrawXNum ((519-3*24), 12, cl.stats[STAT_AMMO], 3, 24, 0.7, 0,0,1,0);
					else
						Sbar_DrawXNum ((519-3*24), 12, cl.stats[STAT_AMMO], 3, 24, 0.6, 0.7,0.8,1,0);

				}

				if (sb_lines > 24)
					DrawQ_Pic(sbar_x,sbar_y,sb_sbar_overlay,0,0,1,1,1,1,DRAWFLAG_MODULATE);

				if (sbar_x + 600 + 160 <= vid_conwidth.integer)
					Sbar_MiniDeathmatchOverlay (sbar_x + 600, sbar_y);

				if (sbar_x > 0)
					Sbar_Score(-16);
					// Because:
					//   Mini scoreboard uses 12*4 per other team, that is, 144
					//   pixels when there are four teams...
					//   Nexuiz by default sets vid_conwidth to 800... makes
					//   sbar_x == 80...
					//   so we need to shift it by 64 pixels to the right to fit
					//   BUT: then it overlaps with the image that gets drawn
					//   for viewsize 100! Therefore, just account for 3 teams,
					//   that is, 96 pixels mini scoreboard size, needing 16 pixels
					//   to the right!
			}
		}
		else if (gamemode == GAME_ZYMOTIC)
		{
#if 1
			float scale = 64.0f / 256.0f;
			float kickoffset[3];
			VectorClear(kickoffset);
			if (v_dmg_time > 0)
			{
				kickoffset[0] = (v_dmg_time/v_kicktime.value*v_dmg_roll) * 10 * scale;
				kickoffset[1] = (v_dmg_time/v_kicktime.value*v_dmg_pitch) * 10 * scale;
			}
			sbar_x = (int)((vid_conwidth.integer - 256 * scale)/2 + kickoffset[0]);
			sbar_y = (int)((vid_conheight.integer - 256 * scale)/2 + kickoffset[1]);
			// left1 16, 48 : 126 -66
			// left2 16, 128 : 196 -66
			// right 176, 48 : 196 -136
			Sbar_DrawGauge(sbar_x +  16 * scale, sbar_y +  48 * scale, zymsb_crosshair_left1, 64*scale,  80*scale, 78*scale,  -66*scale, cl.stats[STAT_AMMO]  * (1.0 / 200.0), cl.stats[STAT_SHELLS]  * (1.0 / 200.0), 0.8f,0.8f,0.0f,1.0f, 0.8f,0.5f,0.0f,1.0f, 0.3f,0.3f,0.3f,1.0f, DRAWFLAG_NORMAL);
			Sbar_DrawGauge(sbar_x +  16 * scale, sbar_y + 128 * scale, zymsb_crosshair_left2, 64*scale,  80*scale, 68*scale,  -66*scale, cl.stats[STAT_NAILS] * (1.0 / 200.0), cl.stats[STAT_ROCKETS] * (1.0 / 200.0), 0.8f,0.8f,0.0f,1.0f, 0.8f,0.5f,0.0f,1.0f, 0.3f,0.3f,0.3f,1.0f, DRAWFLAG_NORMAL);
			Sbar_DrawGauge(sbar_x + 176 * scale, sbar_y +  48 * scale, zymsb_crosshair_right, 64*scale, 160*scale, 148*scale, -136*scale, cl.stats[STAT_ARMOR]  * (1.0 / 300.0), cl.stats[STAT_HEALTH]  * (1.0 / 300.0), 0.0f,0.5f,1.0f,1.0f, 1.0f,0.0f,0.0f,1.0f, 0.3f,0.3f,0.3f,1.0f, DRAWFLAG_NORMAL);
			DrawQ_Pic(sbar_x + 120 * scale, sbar_y + 120 * scale, zymsb_crosshair_center, 16 * scale, 16 * scale, 1, 1, 1, 1, DRAWFLAG_NORMAL);
#else
			float scale = 128.0f / 256.0f;
			float healthstart, healthheight, healthstarttc, healthendtc;
			float shieldstart, shieldheight, shieldstarttc, shieldendtc;
			float ammostart, ammoheight, ammostarttc, ammoendtc;
			float clipstart, clipheight, clipstarttc, clipendtc;
			float kickoffset[3], offset;
			VectorClear(kickoffset);
			if (v_dmg_time > 0)
			{
				kickoffset[0] = (v_dmg_time/v_kicktime.value*v_dmg_roll) * 10 * scale;
				kickoffset[1] = (v_dmg_time/v_kicktime.value*v_dmg_pitch) * 10 * scale;
			}
			sbar_x = (vid_conwidth.integer - 256 * scale)/2 + kickoffset[0];
			sbar_y = (vid_conheight.integer - 256 * scale)/2 + kickoffset[1];
			offset = 0; // TODO: offset should be controlled by recoil (question: how to detect firing?)
			DrawQ_SuperPic(sbar_x +  120           * scale, sbar_y + ( 88 - offset) * scale, zymsb_crosshair_line, 16 * scale, 36 * scale, 0,0, 1,1,1,1, 1,0, 1,1,1,1, 0,1, 1,1,1,1, 1,1, 1,1,1,1, 0);
			DrawQ_SuperPic(sbar_x + (132 + offset) * scale, sbar_y + 120            * scale, zymsb_crosshair_line, 36 * scale, 16 * scale, 0,1, 1,1,1,1, 0,0, 1,1,1,1, 1,1, 1,1,1,1, 1,0, 1,1,1,1, 0);
			DrawQ_SuperPic(sbar_x +  120           * scale, sbar_y + (132 + offset) * scale, zymsb_crosshair_line, 16 * scale, 36 * scale, 1,1, 1,1,1,1, 0,1, 1,1,1,1, 1,0, 1,1,1,1, 0,0, 1,1,1,1, 0);
			DrawQ_SuperPic(sbar_x + ( 88 - offset) * scale, sbar_y + 120            * scale, zymsb_crosshair_line, 36 * scale, 16 * scale, 1,0, 1,1,1,1, 1,1, 1,1,1,1, 0,0, 1,1,1,1, 0,1, 1,1,1,1, 0);
			healthheight = cl.stats[STAT_HEALTH] * (152.0f / 300.0f);
			shieldheight = cl.stats[STAT_ARMOR] * (152.0f / 300.0f);
			healthstart = 204 - healthheight;
			shieldstart = healthstart - shieldheight;
			healthstarttc = healthstart * (1.0f / 256.0f);
			healthendtc = (healthstart + healthheight) * (1.0f / 256.0f);
			shieldstarttc = shieldstart * (1.0f / 256.0f);
			shieldendtc = (shieldstart + shieldheight) * (1.0f / 256.0f);
			ammoheight = cl.stats[STAT_SHELLS] * (62.0f / 200.0f);
			ammostart = 114 - ammoheight;
			ammostarttc = ammostart * (1.0f / 256.0f);
			ammoendtc = (ammostart + ammoheight) * (1.0f / 256.0f);
			clipheight = cl.stats[STAT_AMMO] * (122.0f / 200.0f);
			clipstart = 190 - clipheight;
			clipstarttc = clipstart * (1.0f / 256.0f);
			clipendtc = (clipstart + clipheight) * (1.0f / 256.0f);
			if (healthheight > 0) DrawQ_SuperPic(sbar_x + 0 * scale, sbar_y + healthstart * scale, zymsb_crosshair_health, 256 * scale, healthheight * scale, 0,healthstarttc, 1.0f,0.0f,0.0f,1.0f, 1,healthstarttc, 1.0f,0.0f,0.0f,1.0f, 0,healthendtc, 1.0f,0.0f,0.0f,1.0f, 1,healthendtc, 1.0f,0.0f,0.0f,1.0f, DRAWFLAG_NORMAL);
			if (shieldheight > 0) DrawQ_SuperPic(sbar_x + 0 * scale, sbar_y + shieldstart * scale, zymsb_crosshair_health, 256 * scale, shieldheight * scale, 0,shieldstarttc, 0.0f,0.5f,1.0f,1.0f, 1,shieldstarttc, 0.0f,0.5f,1.0f,1.0f, 0,shieldendtc, 0.0f,0.5f,1.0f,1.0f, 1,shieldendtc, 0.0f,0.5f,1.0f,1.0f, DRAWFLAG_NORMAL);
			if (ammoheight > 0)   DrawQ_SuperPic(sbar_x + 0 * scale, sbar_y + ammostart   * scale, zymsb_crosshair_ammo,   256 * scale, ammoheight   * scale, 0,ammostarttc,   0.8f,0.8f,0.0f,1.0f, 1,ammostarttc,   0.8f,0.8f,0.0f,1.0f, 0,ammoendtc,   0.8f,0.8f,0.0f,1.0f, 1,ammoendtc,   0.8f,0.8f,0.0f,1.0f, DRAWFLAG_NORMAL);
			if (clipheight > 0)   DrawQ_SuperPic(sbar_x + 0 * scale, sbar_y + clipstart   * scale, zymsb_crosshair_clip,   256 * scale, clipheight   * scale, 0,clipstarttc,   1.0f,1.0f,0.0f,1.0f, 1,clipstarttc,   1.0f,1.0f,0.0f,1.0f, 0,clipendtc,   1.0f,1.0f,0.0f,1.0f, 1,clipendtc,   1.0f,1.0f,0.0f,1.0f, DRAWFLAG_NORMAL);
			DrawQ_Pic(sbar_x + 0 * scale, sbar_y + 0 * scale, zymsb_crosshair_background, 256 * scale, 256 * scale, 1, 1, 1, 1, DRAWFLAG_NORMAL);
			DrawQ_Pic(sbar_x + 120 * scale, sbar_y + 120 * scale, zymsb_crosshair_center, 16 * scale, 16 * scale, 1, 1, 1, 1, DRAWFLAG_NORMAL);
#endif
		}
		else // Quake and others
		{
			sbar_x = (vid_conwidth.integer - 320)/2;
			sbar_y = vid_conheight.integer - SBAR_HEIGHT;
			// LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode
			//if (cl.gametype == GAME_DEATHMATCH && gamemode != GAME_TRANSFUSION)

			if (sb_lines > 24)
			{
				if (gamemode != GAME_GOODVSBAD2)
					Sbar_DrawInventory ();
				if (!cl.islocalgame && gamemode != GAME_TRANSFUSION)
					Sbar_DrawFrags ();
			}

			if (sb_showscores || (cl.stats[STAT_HEALTH] <= 0 && cl_deathscoreboard.integer))
			{
				if (gamemode != GAME_GOODVSBAD2)
					Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
				Sbar_DrawScoreboard ();
			}
			else if (sb_lines)
			{
				Sbar_DrawAlphaPic (0, 0, sb_sbar, sbar_alpha_bg.value);

				// keys (hipnotic only)
				//MED 01/04/97 moved keys here so they would not be overwritten
				if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
				{
					if (cl.stats[STAT_ITEMS] & IT_KEY1)
						Sbar_DrawPic (209, 3, sb_items[0]);
					if (cl.stats[STAT_ITEMS] & IT_KEY2)
						Sbar_DrawPic (209, 12, sb_items[1]);
				}
				// armor
				if (gamemode != GAME_GOODVSBAD2)
				{
					if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
					{
						Sbar_DrawNum (24, 0, 666, 3, 1);
						Sbar_DrawPic (0, 0, sb_disc);
					}
					else
					{
						if (gamemode == GAME_ROGUE)
						{
							Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
							if (cl.stats[STAT_ITEMS] & RIT_ARMOR3)
								Sbar_DrawPic (0, 0, sb_armor[2]);
							else if (cl.stats[STAT_ITEMS] & RIT_ARMOR2)
								Sbar_DrawPic (0, 0, sb_armor[1]);
							else if (cl.stats[STAT_ITEMS] & RIT_ARMOR1)
								Sbar_DrawPic (0, 0, sb_armor[0]);
						}
						else
						{
							Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
							if (cl.stats[STAT_ITEMS] & IT_ARMOR3)
								Sbar_DrawPic (0, 0, sb_armor[2]);
							else if (cl.stats[STAT_ITEMS] & IT_ARMOR2)
								Sbar_DrawPic (0, 0, sb_armor[1]);
							else if (cl.stats[STAT_ITEMS] & IT_ARMOR1)
								Sbar_DrawPic (0, 0, sb_armor[0]);
						}
					}
				}

				// face
				Sbar_DrawFace ();

				// health
				Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

				// ammo icon
				if (gamemode == GAME_ROGUE)
				{
					if (cl.stats[STAT_ITEMS] & RIT_SHELLS)
						Sbar_DrawPic (224, 0, sb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & RIT_NAILS)
						Sbar_DrawPic (224, 0, sb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & RIT_ROCKETS)
						Sbar_DrawPic (224, 0, sb_ammo[2]);
					else if (cl.stats[STAT_ITEMS] & RIT_CELLS)
						Sbar_DrawPic (224, 0, sb_ammo[3]);
					else if (cl.stats[STAT_ITEMS] & RIT_LAVA_NAILS)
						Sbar_DrawPic (224, 0, rsb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & RIT_PLASMA_AMMO)
						Sbar_DrawPic (224, 0, rsb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & RIT_MULTI_ROCKETS)
						Sbar_DrawPic (224, 0, rsb_ammo[2]);
				}
				else
				{
					if (cl.stats[STAT_ITEMS] & IT_SHELLS)
						Sbar_DrawPic (224, 0, sb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & IT_NAILS)
						Sbar_DrawPic (224, 0, sb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & IT_ROCKETS)
						Sbar_DrawPic (224, 0, sb_ammo[2]);
					else if (cl.stats[STAT_ITEMS] & IT_CELLS)
						Sbar_DrawPic (224, 0, sb_ammo[3]);
				}

				Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);

				// LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode
				if ((!cl.islocalgame || cl.gametype != GAME_COOP))
				{
					if (gamemode == GAME_TRANSFUSION)
						Sbar_MiniDeathmatchOverlay (0, 0);
					else
						Sbar_MiniDeathmatchOverlay (sbar_x + 324, vid_conheight.integer - 8*8);
					Sbar_Score(24);
				}
			}
		}
	}

	if (cl.csqc_vidvars.drawcrosshair && crosshair.integer >= 1 && !cl.intermission && !r_letterbox.value)
	{
		pic = Draw_CachePic (va(vabuf, sizeof(vabuf), "gfx/crosshair%i", crosshair.integer));
		DrawQ_Pic((vid_conwidth.integer - Draw_GetPicWidth(pic) * crosshair_size.value) * 0.5f, (vid_conheight.integer - Draw_GetPicHeight(pic) * crosshair_size.value) * 0.5f, pic, Draw_GetPicWidth(pic) * crosshair_size.value, Draw_GetPicHeight(pic) * crosshair_size.value, crosshair_color_red.value, crosshair_color_green.value, crosshair_color_blue.value, crosshair_color_alpha.value, 0);
	}

	if (cl_prydoncursor.integer > 0)
		DrawQ_Pic((cl.cmd.cursor_screen[0] + 1) * 0.5 * vid_conwidth.integer, (cl.cmd.cursor_screen[1] + 1) * 0.5 * vid_conheight.integer, Draw_CachePic (va(vabuf, sizeof(vabuf), "gfx/prydoncursor%03i", cl_prydoncursor.integer)), 0, 0, 1, 1, 1, 1, 0);
}

//=============================================================================

/*
==================
Sbar_DeathmatchOverlay

==================
*/
static float Sbar_PrintScoreboardItem(scoreboard_t *s, float x, float y)
{
	int minutes;
	qbool myself = false;
	unsigned char *c;
	char vabuf[1024];
	minutes = (int)((cl.intermission ? cl.completed_time - s->qw_entertime : cl.time - s->qw_entertime) / 60.0);

	if((s - cl.scores) == cl.playerentity - 1)
		myself = true;
	if((s - teams) >= 0 && (s - teams) < MAX_SCOREBOARD)
		if((s->colors & 15) == (cl.scores[cl.playerentity - 1].colors & 15))
			myself = true;

	if (cls.protocol == PROTOCOL_QUAKEWORLD)
	{
		if (s->qw_spectator)
		{
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %4i spectator  %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), minutes, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %4i spectator  %c%s", minutes, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
		else
		{
			// draw colors behind score
			//
			//
			//
			//
			//
			c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
			DrawQ_Fill(x + 14*8*FONT_SBAR->maxwidth, y+1, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			c = palette_rgb_shirtscoreboard[s->colors & 0xf];
			DrawQ_Fill(x + 14*8*FONT_SBAR->maxwidth, y+4, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			// print the text
			//DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%c%4i %s", myself ? 13 : ' ', (int) s->frags, s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, true, FONT_DEFAULT);
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %4i %5i %-4s %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), minutes,(int) s->frags, cl.qw_teamplay ? s->qw_team : "", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %4i %5i %-4s %c%s", minutes,(int) s->frags, cl.qw_teamplay ? s->qw_team : "", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
	}
	else
	{
		if (s->qw_spectator)
		{
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i spect %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         spect %c%s", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
		else
		{
			// draw colors behind score
			c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
			DrawQ_Fill(x + 9*8*FONT_SBAR->maxwidth, y+1, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			c = palette_rgb_shirtscoreboard[s->colors & 0xf];
			DrawQ_Fill(x + 9*8*FONT_SBAR->maxwidth, y+4, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			// print the text
			//DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%c%4i %s", myself ? 13 : ' ', (int) s->frags, s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, true, FONT_DEFAULT);
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %5i %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), (int) s->frags, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %5i %c%s", (int) s->frags, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
	}
	return 8;
}

void Sbar_DeathmatchOverlay (void)
{
	int i, y, xmin, xmax, ymin, ymax;
	char vabuf[1024];

	// request new ping times every two second
	if (cl.last_ping_request < host.realtime - 2 && cls.netcon)
	{
		cl.last_ping_request = host.realtime;
		if (cls.protocol == PROTOCOL_QUAKEWORLD)
		{
			MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, "pings");
		}
		else if (cls.protocol == PROTOCOL_QUAKE || cls.protocol == PROTOCOL_QUAKEDP || cls.protocol == PROTOCOL_NEHAHRAMOVIE || cls.protocol == PROTOCOL_NEHAHRABJP || cls.protocol == PROTOCOL_NEHAHRABJP2 || cls.protocol == PROTOCOL_NEHAHRABJP3 || cls.protocol == PROTOCOL_DARKPLACES1 || cls.protocol == PROTOCOL_DARKPLACES2 || cls.protocol == PROTOCOL_DARKPLACES3 || cls.protocol == PROTOCOL_DARKPLACES4 || cls.protocol == PROTOCOL_DARKPLACES5 || cls.protocol == PROTOCOL_DARKPLACES6/* || cls.protocol == PROTOCOL_DARKPLACES7*/)
		{
			// these servers usually lack the pings command and so a less efficient "ping" command must be sent, which on modern DP servers will also reply with a pingplreport command after the ping listing
			static int ping_anyway_counter = 0;
			if(cl.parsingtextexpectingpingforscores == 1)
			{
				Con_DPrintf("want to send ping, but still waiting for other reply\n");
				if(++ping_anyway_counter >= 5)
					cl.parsingtextexpectingpingforscores = 0;
			}
			if(cl.parsingtextexpectingpingforscores != 1)
			{
				ping_anyway_counter = 0;
				cl.parsingtextexpectingpingforscores = 1; // hide the output of the next ping report
				MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
				MSG_WriteString(&cls.netcon->message, "ping");
			}
		}
		else
		{
			// newer server definitely has pings command, so use it for more efficiency, avoids ping reports spamming the console if they are misparsed, and saves a little bandwidth
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, "pings");
		}
	}

	// scores
	Sbar_SortFrags ();

	ymin = 8;
	ymax = 40 + 8 + (Sbar_IsTeammatch() ? (teamlines * 8 + 5): 0) + scoreboardlines * 8 - 1;

	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		xmin = (int) (vid_conwidth.integer - (26 + 15) * 8 * FONT_SBAR->maxwidth) / 2; // 26 characters until name, then we assume 15 character names (they can be longer but usually aren't)
	else
		xmin = (int) (vid_conwidth.integer - (16 + 25) * 8 * FONT_SBAR->maxwidth) / 2; // 16 characters until name, then we assume 25 character names (they can be longer but usually aren't)
	xmax = vid_conwidth.integer - xmin;

	if(IS_OLDNEXUIZ_DERIVED(gamemode))
		DrawQ_Pic (xmin - 8, ymin - 8, 0, xmax-xmin+1 + 2*8, ymax-ymin+1 + 2*8, 0, 0, 0, sbar_alpha_bg.value, 0);

	DrawQ_Pic ((vid_conwidth.integer - Draw_GetPicWidth(sb_ranking))/2, 8, sb_ranking, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);

	// draw the text
	y = 40;
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
	{
		DrawQ_String(xmin, y, va(vabuf, sizeof(vabuf), "ping pl%% time frags team  name"), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
	}
	else
	{
		DrawQ_String(xmin, y, va(vabuf, sizeof(vabuf), "ping pl%% frags  name"), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
	}
	y += 8;

	if (Sbar_IsTeammatch ())
	{
		// show team scores first
		for (i = 0;i < teamlines && y < vid_conheight.integer;i++)
			y += (int)Sbar_PrintScoreboardItem((teams + teamsort[i]), xmin, y);
		y += 5;
	}

	for (i = 0;i < scoreboardlines && y < vid_conheight.integer;i++)
		y += (int)Sbar_PrintScoreboardItem(cl.scores + fragsort[i], xmin, y);
}

/*
==================
Sbar_MiniDeathmatchOverlay

==================
*/
void Sbar_MiniDeathmatchOverlay (int x, int y)
{
	int i, j, numlines, range_begin, range_end, myteam, teamsep;

	// do not draw this if sbar_miniscoreboard_size is zero
	if(sbar_miniscoreboard_size.value == 0)
		return;
	// adjust the given y if sbar_miniscoreboard_size doesn't indicate default (< 0)
	if(sbar_miniscoreboard_size.value > 0)
		y = (int) (vid_conheight.integer - sbar_miniscoreboard_size.value * 8);

	// scores
	Sbar_SortFrags ();

	// decide where to print
	if (gamemode == GAME_TRANSFUSION)
		numlines = (vid_conwidth.integer - x + 127) / 128;
	else
		numlines = (vid_conheight.integer - y + 7) / 8;

	// give up if there isn't room
	if (x >= vid_conwidth.integer || y >= vid_conheight.integer || numlines < 1)
		return;

	//find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.playerentity - 1)
			break;

	range_begin = 0;
	range_end = scoreboardlines;
	teamsep = 0;

	if (gamemode != GAME_TRANSFUSION)
		if (Sbar_IsTeammatch ())
		{
			// reserve space for the team scores
			numlines -= teamlines;

			// find first and last player of my team (only draw the team totals and my own team)
			range_begin = range_end = i;
			myteam = cl.scores[fragsort[i]].colors & 15;
			while(range_begin > 0 && (cl.scores[fragsort[range_begin-1]].colors & 15) == myteam)
				--range_begin;
			while(range_end < scoreboardlines && (cl.scores[fragsort[range_end]].colors & 15) == myteam)
				++range_end;

			// looks better than two players
			if(numlines == 2)
			{
				teamsep = 8;
				numlines = 1;
			}
		}

	// figure out start
	i -= numlines/2;
	i = min(i, range_end - numlines);
	i = max(i, range_begin);

	if (gamemode == GAME_TRANSFUSION)
	{
		for (;i < range_end && x < vid_conwidth.integer;i++)
			x += 128 + (int)Sbar_PrintScoreboardItem(cl.scores + fragsort[i], x, y);
	}
	else
	{
		if(range_end - i < numlines) // won't draw to bottom?
			y += 8 * (numlines - (range_end - i)); // bottom align
		// show team scores first
		for (j = 0;j < teamlines && y < vid_conheight.integer;j++)
			y += (int)Sbar_PrintScoreboardItem((teams + teamsort[j]), x, y);
		y += teamsep;
		for (;i < range_end && y < vid_conheight.integer;i++)
			y += (int)Sbar_PrintScoreboardItem(cl.scores + fragsort[i], x, y);
	}
}

static int Sbar_TeamColorCompare(const void *t1_, const void *t2_)
{
	static int const sortorder[16] =
	{
		1001,
		1002,
		1003,
		1004,
		1, // red
		1005,
		1006,
		1007,
		1008,
		4, // pink
		1009,
		1010,
		3, // yellow
		2, // blue
		1011,
		1012
	};
	const scoreboard_t *t1 = *(scoreboard_t **) t1_;
	const scoreboard_t *t2 = *(scoreboard_t **) t2_;
	int tc1 = sortorder[t1->colors & 15];
	int tc2 = sortorder[t2->colors & 15];
	return tc1 - tc2;
}

void Sbar_Score (int margin)
{
	int i, me, score, otherleader, place, distribution, minutes, seconds;
	double timeleft;
	int sbar_x_save = sbar_x;
	int sbar_y_save = sbar_y;


	sbar_y = (int) (vid_conheight.value - (32+12));
	sbar_x -= margin;

	me = cl.playerentity - 1;
	if (sbar_scorerank.integer && me >= 0 && me < cl.maxclients)
	{
		if(Sbar_IsTeammatch())
		{
			// Layout:
			//
			//   team1 team3 team4
			//
			//         TEAM2

			scoreboard_t *teamcolorsort[16];

			Sbar_SortFrags();
			for(i = 0; i < teamlines; ++i)
				teamcolorsort[i] = &(teams[i]);

			// Now sort them by color
			qsort(teamcolorsort, teamlines, sizeof(*teamcolorsort), Sbar_TeamColorCompare);

			// : margin
			// -12*4: four digits space
			place = (teamlines - 1) * (-12 * 4);

			for(i = 0; i < teamlines; ++i)
			{
				int cindex = teamcolorsort[i]->colors & 15;
				unsigned char *c = palette_rgb_shirtscoreboard[cindex];
				float cm = max(max(c[0], c[1]), c[2]);
				float cr = c[0] / cm;
				float cg = c[1] / cm;
				float cb = c[2] / cm;
				if(cindex == (cl.scores[cl.playerentity - 1].colors & 15)) // my team
				{
					Sbar_DrawXNum(-32*4, 0, teamcolorsort[i]->frags, 4, 32, cr, cg, cb, 1, 0);
				}
				else // other team
				{
					Sbar_DrawXNum(place, -12, teamcolorsort[i]->frags, 4, 12, cr, cg, cb, 1, 0);
					place += 4 * 12;
				}
			}
		}
		else
		{
			// Layout:
			//
			//   leading  place
			//
			//        FRAGS
			//
			// find leading score other than ourselves, to calculate distribution
			// find our place in the scoreboard
			score = cl.scores[me].frags;
			for (i = 0, otherleader = -1, place = 1;i < cl.maxclients;i++)
			{
				if (cl.scores[i].name[0] && i != me)
				{
					if (otherleader == -1 || cl.scores[i].frags > cl.scores[otherleader].frags)
						otherleader = i;
					if (score < cl.scores[i].frags || (score == cl.scores[i].frags && i < me))
						place++;
				}
			}
			distribution = otherleader >= 0 ? score - cl.scores[otherleader].frags : 0;
			if (place == 1)
				Sbar_DrawXNum(-3*12, -12, place, 3, 12, 1, 1, 1, 1, 0);
			else if (place == 2)
				Sbar_DrawXNum(-3*12, -12, place, 3, 12, 1, 1, 0, 1, 0);
			else
				Sbar_DrawXNum(-3*12, -12, place, 3, 12, 1, 0, 0, 1, 0);
			if (otherleader < 0)
				Sbar_DrawXNum(-32*4,   0, score, 4, 32, 1, 1, 1, 1, 0);
			if (distribution >= 0)
			{
				Sbar_DrawXNum(-7*12, -12, distribution, 4, 12, 1, 1, 1, 1, 0);
				Sbar_DrawXNum(-32*4,   0, score, 4, 32, 1, 1, 1, 1, 0);
			}
			else if (distribution >= -5)
			{
				Sbar_DrawXNum(-7*12, -12, distribution, 4, 12, 1, 1, 0, 1, 0);
				Sbar_DrawXNum(-32*4,   0, score, 4, 32, 1, 1, 0, 1, 0);
			}
			else
			{
				Sbar_DrawXNum(-7*12, -12, distribution, 4, 12, 1, 0, 0, 1, 0);
				Sbar_DrawXNum(-32*4,   0, score, 4, 32, 1, 0, 0, 1, 0);
			}
		}
	}

	if (sbar_gametime.integer && cl.statsf[STAT_TIMELIMIT])
	{
		timeleft = max(0, cl.statsf[STAT_TIMELIMIT] * 60 - cl.time);
		minutes = (int)floor(timeleft / 60);
		seconds = (int)(floor(timeleft) - minutes * 60);
		if (minutes >= 5)
		{
			Sbar_DrawXNum(-12*6, 32, minutes,  3, 12, 1, 1, 1, 1, 0);
			if (Draw_IsPicLoaded(sb_colon))
				DrawQ_Pic(sbar_x + -12*3, sbar_y + 32, sb_colon, 12, 12, 1, 1, 1, sbar_alpha_fg.value, 0);
			Sbar_DrawXNum(-12*2, 32, seconds, -2, 12, 1, 1, 1, 1, 0);
		}
		else if (minutes >= 1)
		{
			Sbar_DrawXNum(-12*6, 32, minutes,  3, 12, 1, 1, 0, 1, 0);
			if (Draw_IsPicLoaded(sb_colon))
				DrawQ_Pic(sbar_x + -12*3, sbar_y + 32, sb_colon, 12, 12, 1, 1, 0, sbar_alpha_fg.value, 0);
			Sbar_DrawXNum(-12*2, 32, seconds, -2, 12, 1, 1, 0, 1, 0);
		}
		else if ((int)(timeleft * 4) & 1)
			Sbar_DrawXNum(-12*2, 32, seconds, -2, 12, 1, 1, 1, 1, 0);
		else
			Sbar_DrawXNum(-12*2, 32, seconds, -2, 12, 1, 0, 0, 1, 0);
	}
	else if (sbar_gametime.integer)
	{
		minutes = (int)floor(cl.time / 60);
		seconds = (int)(floor(cl.time) - minutes * 60);
		Sbar_DrawXNum(-12*6, 32, minutes,  3, 12, 1, 1, 1, 1, 0);
		if (Draw_IsPicLoaded(sb_colon))
			DrawQ_Pic(sbar_x + -12*3, sbar_y + 32, sb_colon, 12, 12, 1, 1, 1, sbar_alpha_fg.value, 0);
		Sbar_DrawXNum(-12*2, 32, seconds, -2, 12, 1, 1, 1, 1, 0);
	}

	sbar_x = sbar_x_save;
	sbar_y = sbar_y_save;
}

/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay (void)
{
	int		dig;
	int		num;

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	sbar_x = (vid_conwidth.integer - 320) >> 1;
	sbar_y = (vid_conheight.integer - 200) >> 1;

	DrawQ_Pic (sbar_x + 64, sbar_y + 24, sb_complete, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);
	DrawQ_Pic (sbar_x + 0, sbar_y + 56, sb_inter, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);

// time
	dig = (int)cl.completed_time / 60;
	Sbar_DrawNum (160, 64, dig, 3, 0);
	num = (int)cl.completed_time - dig*60;
	Sbar_DrawPic (234,64,sb_colon);
	Sbar_DrawPic (246,64,sb_nums[0][num/10]);
	Sbar_DrawPic (266,64,sb_nums[0][num%10]);

// LA: Display as "a" instead of "a/b" if b is 0
	if(cl.stats[STAT_TOTALSECRETS])
	{
		Sbar_DrawNum (160, 104, cl.stats[STAT_SECRETS], 3, 0);
		if (!IS_OLDNEXUIZ_DERIVED(gamemode))
			Sbar_DrawPic (232, 104, sb_slash);
		Sbar_DrawNum (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);
	}
	else
	{
		Sbar_DrawNum (240, 104, cl.stats[STAT_SECRETS], 3, 0);
	}

	if(cl.stats[STAT_TOTALMONSTERS])
	{
		Sbar_DrawNum (160, 144, cl.stats[STAT_MONSTERS], 3, 0);
		if (!IS_OLDNEXUIZ_DERIVED(gamemode))
			Sbar_DrawPic (232, 144, sb_slash);
		Sbar_DrawNum (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);
	}
	else
	{
		Sbar_DrawNum (240, 144, cl.stats[STAT_MONSTERS], 3, 0);
	}
}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay (void)
{
	DrawQ_Pic((vid_conwidth.integer - Draw_GetPicWidth(sb_finale))/2, 16, sb_finale, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);
}

