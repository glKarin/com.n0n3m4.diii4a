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
// view.c -- player eye positioning

#include "quakedef.h"
#include "cl_collision.h"
#include "image.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t cl_rollspeed = {CF_CLIENT | CF_ARCHIVE, "cl_rollspeed", "200", "how much strafing is necessary to tilt the view"};
cvar_t cl_rollangle = {CF_CLIENT | CF_ARCHIVE, "cl_rollangle", "2.0", "how much to tilt the view when strafing"};

cvar_t cl_bob = {CF_CLIENT | CF_ARCHIVE, "cl_bob","0.02", "view bobbing amount"};
cvar_t cl_bobcycle = {CF_CLIENT | CF_ARCHIVE, "cl_bobcycle","0.6", "view bobbing speed"};
cvar_t cl_bobup = {CF_CLIENT | CF_ARCHIVE, "cl_bobup","0.5", "view bobbing adjustment that makes the up or down swing of the bob last longer"};
cvar_t cl_bob2 = {CF_CLIENT | CF_ARCHIVE, "cl_bob2","0", "sideways view bobbing amount"};
cvar_t cl_bob2cycle = {CF_CLIENT | CF_ARCHIVE, "cl_bob2cycle","0.6", "sideways view bobbing speed"};
cvar_t cl_bob2smooth = {CF_CLIENT | CF_ARCHIVE, "cl_bob2smooth","0.05", "how fast the view goes back when you stop touching the ground"};
cvar_t cl_bobfall = {CF_CLIENT | CF_ARCHIVE, "cl_bobfall","0", "how much the view swings down when falling (influenced by the speed you hit the ground with)"};
cvar_t cl_bobfallcycle = {CF_CLIENT | CF_ARCHIVE, "cl_bobfallcycle","3", "speed of the bobfall swing"};
cvar_t cl_bobfallminspeed = {CF_CLIENT | CF_ARCHIVE, "cl_bobfallminspeed","200", "necessary amount of speed for bob-falling to occur"};
cvar_t cl_bobmodel = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel", "1", "enables gun bobbing"};
cvar_t cl_bobmodel_side = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel_side", "0.15", "gun bobbing sideways sway amount"};
cvar_t cl_bobmodel_up = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel_up", "0.06", "gun bobbing upward movement amount"};
cvar_t cl_bobmodel_forward = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel_forward", "0", "gun bobbing forward movement amount"};
cvar_t cl_bobmodel_classic = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel_classic", "1", "classic Quake-style forward gun bobbing"};
cvar_t cl_bobmodel_speed = {CF_CLIENT | CF_ARCHIVE, "cl_bobmodel_speed", "7", "gun bobbing speed"};
cvar_t cl_bob_limit = {CF_CLIENT | CF_ARCHIVE, "cl_bob_limit", "7", "limits bobbing to this much distance from view_ofs"};
cvar_t cl_bob_limit_heightcheck = {CF_CLIENT | CF_ARCHIVE, "cl_bob_limit_heightcheck", "0", "check ceiling and floor height against cl_bob_limit and scale down all view bobbing if could result in camera being in solid"};
cvar_t cl_bob_limit_heightcheck_dontcrosswatersurface = {CF_CLIENT | CF_ARCHIVE, "cl_bob_limit_heightcheck_dontcrosswatersurface", "1", "limit cl_bob_limit to not crossing liquid surfaces also"};
cvar_t cl_bob_velocity_limit = {CF_CLIENT | CF_ARCHIVE, "cl_bob_velocity_limit", "400", "limits the xyspeed value in the bobbing code"};

cvar_t cl_leanmodel = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel", "0", "enables gun leaning"};
cvar_t cl_leanmodel_side_speed = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_side_speed", "0.7", "gun leaning sideways speed"};
cvar_t cl_leanmodel_side_limit = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_side_limit", "35", "gun leaning sideways limit"};
cvar_t cl_leanmodel_side_highpass1 = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_side_highpass1", "30", "gun leaning sideways pre-highpass in 1/s"};
cvar_t cl_leanmodel_side_highpass = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_side_highpass", "3", "gun leaning sideways highpass in 1/s"};
cvar_t cl_leanmodel_side_lowpass = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_side_lowpass", "20", "gun leaning sideways lowpass in 1/s"};
cvar_t cl_leanmodel_up_speed = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_up_speed", "0.65", "gun leaning upward speed"};
cvar_t cl_leanmodel_up_limit = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_up_limit", "50", "gun leaning upward limit"};
cvar_t cl_leanmodel_up_highpass1 = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_up_highpass1", "5", "gun leaning upward pre-highpass in 1/s"};
cvar_t cl_leanmodel_up_highpass = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_up_highpass", "15", "gun leaning upward highpass in 1/s"};
cvar_t cl_leanmodel_up_lowpass = {CF_CLIENT | CF_ARCHIVE, "cl_leanmodel_up_lowpass", "20", "gun leaning upward lowpass in 1/s"};

cvar_t cl_followmodel = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel", "0", "enables gun following"};
cvar_t cl_followmodel_side_speed = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_side_speed", "0.25", "gun following sideways speed"};
cvar_t cl_followmodel_side_limit = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_side_limit", "6", "gun following sideways limit"};
cvar_t cl_followmodel_side_highpass1 = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_side_highpass1", "30", "gun following sideways pre-highpass in 1/s"};
cvar_t cl_followmodel_side_highpass = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_side_highpass", "5", "gun following sideways highpass in 1/s"};
cvar_t cl_followmodel_side_lowpass = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_side_lowpass", "10", "gun following sideways lowpass in 1/s"};
cvar_t cl_followmodel_up_speed = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_up_speed", "0.5", "gun following upward speed"};
cvar_t cl_followmodel_up_limit = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_up_limit", "5", "gun following upward limit"};
cvar_t cl_followmodel_up_highpass1 = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_up_highpass1", "60", "gun following upward pre-highpass in 1/s"};
cvar_t cl_followmodel_up_highpass = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_up_highpass", "2", "gun following upward highpass in 1/s"};
cvar_t cl_followmodel_up_lowpass = {CF_CLIENT | CF_ARCHIVE, "cl_followmodel_up_lowpass", "10", "gun following upward lowpass in 1/s"};

cvar_t cl_viewmodel_scale = {CF_CLIENT, "cl_viewmodel_scale", "1", "changes size of gun model, lower values prevent poking into walls but cause strange artifacts on lighting and especially r_stereo/vid_stereobuffer options where the size of the gun becomes visible"};

cvar_t v_kicktime = {CF_CLIENT, "v_kicktime", "0.5", "how long a view kick from damage lasts"};
cvar_t v_kickroll = {CF_CLIENT, "v_kickroll", "0.6", "how much a view kick from damage rolls your view"};
cvar_t v_kickpitch = {CF_CLIENT, "v_kickpitch", "0.6", "how much a view kick from damage pitches your view"};

cvar_t v_iyaw_cycle = {CF_CLIENT, "v_iyaw_cycle", "2", "v_idlescale yaw speed"};
cvar_t v_iroll_cycle = {CF_CLIENT, "v_iroll_cycle", "0.5", "v_idlescale roll speed"};
cvar_t v_ipitch_cycle = {CF_CLIENT, "v_ipitch_cycle", "1", "v_idlescale pitch speed"};
cvar_t v_iyaw_level = {CF_CLIENT, "v_iyaw_level", "0.3", "v_idlescale yaw amount"};
cvar_t v_iroll_level = {CF_CLIENT, "v_iroll_level", "0.1", "v_idlescale roll amount"};
cvar_t v_ipitch_level = {CF_CLIENT, "v_ipitch_level", "0.3", "v_idlescale pitch amount"};

cvar_t v_idlescale = {CF_CLIENT, "v_idlescale", "0", "how much of the quake 'drunken view' effect to use"};

cvar_t v_isometric = {CF_CLIENT, "v_isometric", "0", "changes view to isometric (non-perspective)"};
cvar_t v_isometric_verticalfov = {CF_CLIENT, "v_isometric_verticalfov", "512", "vertical field of view in game units (horizontal is computed using aspect ratio based on this)"};
cvar_t v_isometric_xx = {CF_CLIENT, "v_isometric_xx", "1", "camera matrix"};
cvar_t v_isometric_xy = {CF_CLIENT, "v_isometric_xy", "0", "camera matrix"};
cvar_t v_isometric_xz = {CF_CLIENT, "v_isometric_xz", "0", "camera matrix"};
cvar_t v_isometric_yx = {CF_CLIENT, "v_isometric_yx", "0", "camera matrix"};
cvar_t v_isometric_yy = {CF_CLIENT, "v_isometric_yy", "1", "camera matrix"};
cvar_t v_isometric_yz = {CF_CLIENT, "v_isometric_yz", "0", "camera matrix"};
cvar_t v_isometric_zx = {CF_CLIENT, "v_isometric_zx", "0", "camera matrix"};
cvar_t v_isometric_zy = {CF_CLIENT, "v_isometric_zy", "0", "camera matrix"};
cvar_t v_isometric_zz = {CF_CLIENT, "v_isometric_zz", "1", "camera matrix"};
cvar_t v_isometric_tx = {CF_CLIENT, "v_isometric_tx", "0", "camera position (player-relative)"};
cvar_t v_isometric_ty = {CF_CLIENT, "v_isometric_ty", "0", "camera position (player-relative)"};
cvar_t v_isometric_tz = {CF_CLIENT, "v_isometric_tz", "0", "camera position (player-relative)"};
cvar_t v_isometric_rot_pitch = {CF_CLIENT, "v_isometric_rot_pitch", "60", "camera rotation"};
cvar_t v_isometric_rot_yaw = {CF_CLIENT, "v_isometric_rot_yaw", "135", "camera rotation"};
cvar_t v_isometric_rot_roll = {CF_CLIENT, "v_isometric_rot_roll", "0", "camera rotation"};
cvar_t v_isometric_relx = {CF_CLIENT, "v_isometric_relx", "0", "camera position*forward"};
cvar_t v_isometric_rely = {CF_CLIENT, "v_isometric_rely", "0", "camera position*left"};
cvar_t v_isometric_relz = {CF_CLIENT, "v_isometric_relz", "0", "camera position*up"};
cvar_t v_isometric_flipcullface = {CF_CLIENT, "v_isometric_flipcullface", "0", "flips the backface culling"};
cvar_t v_isometric_locked_orientation = {CF_CLIENT, "v_isometric_locked_orientation", "1", "camera rotation is fixed"};
cvar_t v_isometric_usevieworiginculling = {CF_CLIENT, "v_isometric_usevieworiginculling", "0", "check visibility to the player location (can look pretty weird)"};

cvar_t crosshair = {CF_CLIENT | CF_ARCHIVE, "crosshair", "0", "selects crosshair to use (0 is none)"};

cvar_t v_centermove = {CF_CLIENT, "v_centermove", "0.15", "how long before the view begins to center itself (if freelook/+mlook/+jlook/+klook are off)"};
cvar_t v_centerspeed = {CF_CLIENT, "v_centerspeed","500", "how fast the view centers itself"};

cvar_t cl_stairsmoothspeed = {CF_CLIENT | CF_ARCHIVE, "cl_stairsmoothspeed", "160", "how fast your view moves upward/downward when running up/down stairs"};

cvar_t cl_smoothviewheight = {CF_CLIENT | CF_ARCHIVE, "cl_smoothviewheight", "0", "time of the averaging to the viewheight value so that it creates a smooth transition. higher values = longer transition, 0 for instant transition."};

cvar_t chase_back = {CF_CLIENT | CF_ARCHIVE, "chase_back", "48", "chase cam distance from the player"};
cvar_t chase_up = {CF_CLIENT | CF_ARCHIVE, "chase_up", "24", "chase cam distance from the player"};
cvar_t chase_active = {CF_CLIENT | CF_ARCHIVE, "chase_active", "0", "enables chase cam"};
cvar_t chase_overhead = {CF_CLIENT | CF_ARCHIVE, "chase_overhead", "0", "chase cam looks straight down if this is not zero"};
// GAME_GOODVSBAD2
cvar_t chase_stevie = {CF_CLIENT, "chase_stevie", "0", "(GOODVSBAD2 only) chase cam view from above"};

cvar_t v_deathtilt = {CF_CLIENT, "v_deathtilt", "1", "whether to use sideways view when dead"};
cvar_t v_deathtiltangle = {CF_CLIENT, "v_deathtiltangle", "80", "what roll angle to use when tilting the view while dead"};

// Prophecy camera pitchangle by Alexander "motorsep" Zubov
cvar_t chase_pitchangle = {CF_CLIENT | CF_ARCHIVE, "chase_pitchangle", "55", "chase cam pitch angle"};

cvar_t v_yshearing = {CF_CLIENT, "v_yshearing", "0", "be all out of gum (set this to the maximum angle to allow Y shearing for - try values like 75)"};

cvar_t r_viewmodel_quake = {CF_CLIENT | CF_ARCHIVE, "r_viewmodel_quake", "0", "Quake-style weapon viewmodel angle adjustment"};

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

int cl_punchangle_applied;

void V_StartPitchDrift (void)
{
	if (cl.laststop == cl.time)
		return;		// something else is keeping it from drifting

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void V_StartPitchDrift_f(cmd_state_t *cmd)
{
	V_StartPitchDrift();
}

void V_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
void V_DriftPitch (void)
{
	float		delta, move;

	if (noclip_anglehack || !cl.onground || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( fabs(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else
			cl.driftmove += cl.realframetime;

		if ( cl.driftmove > v_centermove.value)
		{
			V_StartPitchDrift ();
		}
		return;
	}

	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = cl.realframetime * cl.pitchvel;
	cl.pitchvel += cl.realframetime * v_centerspeed.value;

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}


/*
==============================================================================

						SCREEN FLASHES

==============================================================================
*/


/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (void)
{
	int armor, blood;
	vec3_t from;
	//vec3_t forward, right;
	vec3_t localfrom;
	entity_t *ent;
	//float side;
	float count;

	armor = MSG_ReadByte(&cl_message);
	blood = MSG_ReadByte(&cl_message);
	MSG_ReadVector(&cl_message, from, cls.protocol);

	// Send the Dmg Globals to CSQC
	CL_VM_UpdateDmgGlobals(blood, armor, from);

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// put sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	cl.cshifts[CSHIFT_DAMAGE].alphafade = 150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

	// calculate view angle kicks
	if (cl.entities[cl.viewentity].state_current.active)
	{
		ent = &cl.entities[cl.viewentity];
		Matrix4x4_Transform(&ent->render.inversematrix, from, localfrom);
		VectorNormalize(localfrom);
		v_dmg_pitch = count * localfrom[0] * v_kickpitch.value;
		v_dmg_roll = count * localfrom[1] * v_kickroll.value;
		v_dmg_time = v_kicktime.value;
	}
}

static cshift_t v_cshift;

/*
==================
V_cshift_f
==================
*/
static void V_cshift_f(cmd_state_t *cmd)
{
	v_cshift.destcolor[0] = atof(Cmd_Argv(cmd, 1));
	v_cshift.destcolor[1] = atof(Cmd_Argv(cmd, 2));
	v_cshift.destcolor[2] = atof(Cmd_Argv(cmd, 3));
	v_cshift.percent = atof(Cmd_Argv(cmd, 4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
static void V_BonusFlash_f(cmd_state_t *cmd)
{
	if(Cmd_Argc(cmd) == 1)
	{
		cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
		cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
		cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
		cl.cshifts[CSHIFT_BONUS].percent = 50;
		cl.cshifts[CSHIFT_BONUS].alphafade = 100;
	}
	else if(Cmd_Argc(cmd) >= 4 && Cmd_Argc(cmd) <= 6)
	{
		cl.cshifts[CSHIFT_BONUS].destcolor[0] = atof(Cmd_Argv(cmd, 1)) * 255;
		cl.cshifts[CSHIFT_BONUS].destcolor[1] = atof(Cmd_Argv(cmd, 2)) * 255;
		cl.cshifts[CSHIFT_BONUS].destcolor[2] = atof(Cmd_Argv(cmd, 3)) * 255;
		if(Cmd_Argc(cmd) >= 5)
			cl.cshifts[CSHIFT_BONUS].percent = atof(Cmd_Argv(cmd, 4)) * 255; // yes, these are HEXADECIMAL percent ;)
		else
			cl.cshifts[CSHIFT_BONUS].percent = 50;
		if(Cmd_Argc(cmd) >= 6)
			cl.cshifts[CSHIFT_BONUS].alphafade = atof(Cmd_Argv(cmd, 5)) * 255;
		else
			cl.cshifts[CSHIFT_BONUS].alphafade = 100;
	}
	else
		Con_Printf("usage:\nbf, or bf R G B [A [alphafade]]\n");
}

/*
==============================================================================

						VIEW RENDERING

==============================================================================
*/

extern matrix4x4_t viewmodelmatrix_nobob;
extern matrix4x4_t viewmodelmatrix_withbob;

#include "cl_collision.h"
#include "csprogs.h"

/*
==================
V_CalcRefdef

==================
*/
#if 0
static vec3_t eyeboxmins = {-16, -16, -24};
static vec3_t eyeboxmaxs = { 16,  16,  32};
#endif

static vec_t lowpass(vec_t value, vec_t frac, vec_t *store)
{
	frac = bound(0, frac, 1);
	return (*store = *store * (1 - frac) + value * frac);
}

static vec_t lowpass_limited(vec_t value, vec_t frac, vec_t limit, vec_t *store)
{
	lowpass(value, frac, store);
	return (*store = bound(value - limit, *store, value + limit));
}

static vec_t highpass(vec_t value, vec_t frac, vec_t *store)
{
	return value - lowpass(value, frac, store);
}

static vec_t highpass_limited(vec_t value, vec_t frac, vec_t limit, vec_t *store)
{
	return value - lowpass_limited(value, frac, limit, store);
}

static void lowpass3(vec3_t value, vec_t fracx, vec_t fracy, vec_t fracz, vec3_t store, vec3_t out)
{
	out[0] = lowpass(value[0], fracx, &store[0]);
	out[1] = lowpass(value[1], fracy, &store[1]);
	out[2] = lowpass(value[2], fracz, &store[2]);
}

static void highpass3(vec3_t value, vec_t fracx, vec_t fracy, vec_t fracz, vec3_t store, vec3_t out)
{
	out[0] = highpass(value[0], fracx, &store[0]);
	out[1] = highpass(value[1], fracy, &store[1]);
	out[2] = highpass(value[2], fracz, &store[2]);
}

static void highpass3_limited(vec3_t value, vec_t fracx, vec_t limitx, vec_t fracy, vec_t limity, vec_t fracz, vec_t limitz, vec3_t store, vec3_t out)
{
	out[0] = highpass_limited(value[0], fracx, limitx, &store[0]);
	out[1] = highpass_limited(value[1], fracy, limity, &store[1]);
	out[2] = highpass_limited(value[2], fracz, limitz, &store[2]);
}

/*
 * State:
 *   cl.bob2_smooth
 *   cl.bobfall_speed
 *   cl.bobfall_swing
 *   cl.gunangles_adjustment_highpass
 *   cl.gunangles_adjustment_lowpass
 *   cl.gunangles_highpass
 *   cl.gunangles_prev
 *   cl.gunorg_adjustment_highpass
 *   cl.gunorg_adjustment_lowpass
 *   cl.gunorg_highpass
 *   cl.gunorg_prev
 *   cl.hitgroundtime
 *   cl.lastongroundtime
 *   cl.oldongrounbd
 *   cl.stairsmoothtime
 *   cl.stairsmoothz
 *   cl.calcrefdef_prevtime
 * Extra input:
 *   cl.movecmd[0].time
 *   cl.movevars_stepheight
 *   cl.movevars_timescale
 *   cl.oldtime
 *   cl.punchangle
 *   cl.punchvector
 *   cl.qw_intermission_angles
 *   cl.qw_intermission_origin
 *   cl.qw_weaponkick
 *   cls.protocol
 *   cl.time
 * Output:
 *   cl.csqc_viewanglesfromengine
 *   cl.csqc_viewmodelmatrixfromengine
 *   cl.csqc_vieworiginfromengine
 *   r_refdef.view.matrix
 *   viewmodelmatrix_nobob
 *   viewmodelmatrix_withbob
 */

/*
==================
V_CalcIntermissionRefdef

==================
*/
static void V_CalcIntermissionRefdef (vec3_t vieworg, vec3_t viewangles, const matrix4x4_t *entrendermatrix, float clstatsviewheight)
{
	matrix4x4_t tmpmatrix;

	// entity is a fixed camera, just copy the matrix
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, cl.qw_intermission_origin[0], cl.qw_intermission_origin[1], cl.qw_intermission_origin[2], cl.qw_intermission_angles[0], cl.qw_intermission_angles[1], cl.qw_intermission_angles[2], 1);
	else
	{
		r_refdef.view.matrix = *entrendermatrix;
		Matrix4x4_AdjustOrigin(&r_refdef.view.matrix, 0, 0, clstatsviewheight);
	}
	if (v_yshearing.value > 0)
		Matrix4x4_QuakeToDuke3D(&r_refdef.view.matrix, &r_refdef.view.matrix, v_yshearing.value);
	Matrix4x4_Copy(&viewmodelmatrix_nobob, &r_refdef.view.matrix);
	Matrix4x4_ConcatScale(&viewmodelmatrix_nobob, cl_viewmodel_scale.value);
	Matrix4x4_Copy(&viewmodelmatrix_withbob, &viewmodelmatrix_nobob);

	VectorCopy(vieworg, cl.csqc_vieworiginfromengine);
	VectorCopy(viewangles, cl.csqc_viewanglesfromengine);

	Matrix4x4_Invert_Simple(&tmpmatrix, &r_refdef.view.matrix);
	Matrix4x4_CreateScale(&cl.csqc_viewmodelmatrixfromengine, cl_viewmodel_scale.value);
}

void V_CalcRefdefUsing (const matrix4x4_t *entrendermatrix, const vec3_t clviewangles, qbool teleported, qbool clonground, qbool clcmdjump, float clstatsviewheight, qbool cldead, const vec3_t clvelocity)
{
	float vieworg[3], viewangles[3], smoothtime;
	float gunorg[3], gunangles[3];
	matrix4x4_t tmpmatrix;
	static float viewheightavg;
	float viewheight;
	trace_t trace;

	// react to clonground state changes (for gun bob)
	if (clonground)
	{
		if (!cl.oldonground)
			cl.hitgroundtime = cl.movecmd[0].time;
		cl.lastongroundtime = cl.movecmd[0].time;
	}
	cl.oldonground = clonground;
	cl.calcrefdef_prevtime = max(cl.calcrefdef_prevtime, cl.oldtime);

	VectorClear(gunangles);
	VectorClear(gunorg);
	viewmodelmatrix_nobob = identitymatrix;
	viewmodelmatrix_withbob = identitymatrix;
	r_refdef.view.matrix = identitymatrix;

	// player can look around, so take the origin from the entity,
	// and the angles from the input system
	Matrix4x4_OriginFromMatrix(entrendermatrix, vieworg);
	VectorCopy(clviewangles, viewangles);

	// calculate how much time has passed since the last V_CalcRefdef
	smoothtime = bound(0, cl.time - cl.stairsmoothtime, 0.1);
	cl.stairsmoothtime = cl.time;

	// fade damage flash
	if (v_dmg_time > 0)
		v_dmg_time -= bound(0, smoothtime, 0.1);

	if (cl.intermission)
		V_CalcIntermissionRefdef(vieworg, viewangles, entrendermatrix, clstatsviewheight);
	else
	{
		// smooth stair stepping, but only if clonground and enabled
		if (!clonground || cl_stairsmoothspeed.value <= 0 || teleported)
			cl.stairsmoothz = vieworg[2];
		else
		{
			if (cl.stairsmoothz < vieworg[2])
				vieworg[2] = cl.stairsmoothz = bound(vieworg[2] - cl.movevars_stepheight, cl.stairsmoothz + smoothtime * cl_stairsmoothspeed.value, vieworg[2]);
			else if (cl.stairsmoothz > vieworg[2])
				vieworg[2] = cl.stairsmoothz = bound(vieworg[2], cl.stairsmoothz - smoothtime * cl_stairsmoothspeed.value, vieworg[2] + cl.movevars_stepheight);
		}

		// apply qw weapon recoil effect (this did not work in QW)
		// TODO: add a cvar to disable this
		viewangles[PITCH] += cl.qw_weaponkick;

		// apply the viewofs (even if chasecam is used)
		// Samual: Lets add smoothing for this too so that things like crouching are done with a transition.
		viewheight = bound(0, (cl.time - cl.calcrefdef_prevtime) / max(0.0001, cl_smoothviewheight.value), 1);
		viewheightavg = viewheightavg * (1 - viewheight) + clstatsviewheight * viewheight;
		vieworg[2] += viewheightavg;

		if (chase_active.value)
		{
			// observing entity from third person. Added "campitch" by Alexander "motorsep" Zubov
			vec_t camback, camup, dist, campitch, forward[3], chase_dest[3];

			camback = chase_back.value;
			camup = chase_up.value;
			campitch = chase_pitchangle.value;

			AngleVectors(viewangles, forward, NULL, NULL);

			if (chase_overhead.integer)
			{
				vec3_t offset;
				vec3_t bestvieworg;
				vec3_t up;
				viewangles[PITCH] = 0;
				AngleVectors(viewangles, forward, NULL, up);
				// trace a little further so it hits a surface more consistently (to avoid 'snapping' on the edge of the range)
				chase_dest[0] = vieworg[0] - forward[0] * camback + up[0] * camup;
				chase_dest[1] = vieworg[1] - forward[1] * camback + up[1] * camup;
				chase_dest[2] = vieworg[2] - forward[2] * camback + up[2] * camup;
				// trace from first person view location to our chosen third person view location
				trace = CL_TraceLine(vieworg, chase_dest, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
				VectorCopy(trace.endpos, bestvieworg);
				offset[2] = 0;
				for (offset[0] = -16;offset[0] <= 16;offset[0] += 8)
				{
					for (offset[1] = -16;offset[1] <= 16;offset[1] += 8)
					{
						AngleVectors(viewangles, NULL, NULL, up);
						chase_dest[0] = vieworg[0] - forward[0] * camback + up[0] * camup + offset[0];
						chase_dest[1] = vieworg[1] - forward[1] * camback + up[1] * camup + offset[1];
						chase_dest[2] = vieworg[2] - forward[2] * camback + up[2] * camup + offset[2];
						trace = CL_TraceLine(vieworg, chase_dest, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
						if (bestvieworg[2] > trace.endpos[2])
							bestvieworg[2] = trace.endpos[2];
					}
				}
				bestvieworg[2] -= 8;
				VectorCopy(bestvieworg, vieworg);
				viewangles[PITCH] = campitch;
			}
			else
			{
				if (gamemode == GAME_GOODVSBAD2 && chase_stevie.integer)
				{
					// look straight down from high above
					viewangles[PITCH] = 90;
					camback = 2048;
					VectorSet(forward, 0, 0, -1);
				}

				// trace a little further so it hits a surface more consistently (to avoid 'snapping' on the edge of the range)
				dist = -camback - 8;
				chase_dest[0] = vieworg[0] + forward[0] * dist;
				chase_dest[1] = vieworg[1] + forward[1] * dist;
				chase_dest[2] = vieworg[2] + forward[2] * dist + camup;
				trace = CL_TraceLine(vieworg, chase_dest, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
				VectorMAMAM(1, trace.endpos, 8, forward, 4, trace.plane.normal, vieworg);
			}
		}
		else
		{
			// first person view from entity
			// angles
			if (cldead && v_deathtilt.integer)
				viewangles[ROLL] = v_deathtiltangle.value;

			// origin
			VectorAdd(vieworg, cl.punchvector, vieworg);
			if (!cldead)
			{
				double xyspeed = 0, bob = 0, bobfall = 0;
				double cycle = 0; // double-precision because cl.time can be a very large number, where float would get stuttery at high time values
				vec_t frametime;

				frametime = (cl.time - cl.calcrefdef_prevtime) * cl.movevars_timescale;

				if(cl_followmodel.integer || cl_leanmodel.integer)
				{
					// 1. if we teleported, clear the frametime... the lowpass will recover the previous value then
					if(teleported)
					{
						// try to fix the first highpass; result is NOT
						// perfect! TODO find a better fix
						VectorCopy(viewangles, cl.gunangles_prev);
						VectorCopy(vieworg, cl.gunorg_prev);
					}

					// 2. for the gun origin, only keep the high frequency (non-DC) parts, which is "somewhat like velocity"
					VectorAdd(cl.gunorg_highpass, cl.gunorg_prev, cl.gunorg_highpass);
					highpass3_limited(vieworg, frametime*cl_followmodel_side_highpass1.value, cl_followmodel_side_limit.value, frametime*cl_followmodel_side_highpass1.value, cl_followmodel_side_limit.value, frametime*cl_followmodel_up_highpass1.value, cl_followmodel_up_limit.value, cl.gunorg_highpass, gunorg);
					VectorCopy(vieworg, cl.gunorg_prev);
					VectorSubtract(cl.gunorg_highpass, cl.gunorg_prev, cl.gunorg_highpass);

					// in the highpass, we _store_ the DIFFERENCE to the actual view angles...
					VectorAdd(cl.gunangles_highpass, cl.gunangles_prev, cl.gunangles_highpass);
					cl.gunangles_highpass[PITCH] += 360 * floor((viewangles[PITCH] - cl.gunangles_highpass[PITCH]) / 360 + 0.5);
					cl.gunangles_highpass[YAW] += 360 * floor((viewangles[YAW] - cl.gunangles_highpass[YAW]) / 360 + 0.5);
					cl.gunangles_highpass[ROLL] += 360 * floor((viewangles[ROLL] - cl.gunangles_highpass[ROLL]) / 360 + 0.5);
					highpass3_limited(viewangles, frametime*cl_leanmodel_up_highpass1.value, cl_leanmodel_up_limit.value, frametime*cl_leanmodel_side_highpass1.value, cl_leanmodel_side_limit.value, 0, 0, cl.gunangles_highpass, gunangles);
					VectorCopy(viewangles, cl.gunangles_prev);
					VectorSubtract(cl.gunangles_highpass, cl.gunangles_prev, cl.gunangles_highpass);

					// 3. calculate the RAW adjustment vectors
					gunorg[0] *= -cl_followmodel_side_speed.value;
					gunorg[1] *= -cl_followmodel_side_speed.value;
					gunorg[2] *= -cl_followmodel_up_speed.value;

					gunangles[PITCH] *= -cl_leanmodel_up_speed.value;
					gunangles[YAW] *= -cl_leanmodel_side_speed.value;
					gunangles[ROLL] = 0;

					// 4. perform highpass/lowpass on the adjustment vectors (turning velocity into acceleration!)
					//    trick: we must do the lowpass LAST, so the lowpass vector IS the final vector!
					highpass3(gunorg, frametime*cl_followmodel_side_highpass.value, frametime*cl_followmodel_side_highpass.value, frametime*cl_followmodel_up_highpass.value, cl.gunorg_adjustment_highpass, gunorg);
					lowpass3(gunorg, frametime*cl_followmodel_side_lowpass.value, frametime*cl_followmodel_side_lowpass.value, frametime*cl_followmodel_up_lowpass.value, cl.gunorg_adjustment_lowpass, gunorg);
					// we assume here: PITCH = 0, YAW = 1, ROLL = 2
					highpass3(gunangles, frametime*cl_leanmodel_up_highpass.value, frametime*cl_leanmodel_side_highpass.value, 0, cl.gunangles_adjustment_highpass, gunangles);
					lowpass3(gunangles, frametime*cl_leanmodel_up_lowpass.value, frametime*cl_leanmodel_side_lowpass.value, 0, cl.gunangles_adjustment_lowpass, gunangles);

					// 5. use the adjusted vectors
					VectorAdd(vieworg, gunorg, gunorg);
					VectorAdd(viewangles, gunangles, gunangles);
				}
				else
				{
					// Just initialize gunorg/gunangles.
					VectorCopy(vieworg, gunorg);
					VectorCopy(viewangles, gunangles);
				}

				// bounded XY speed, used by several effects below
				xyspeed = bound (0, sqrt(clvelocity[0]*clvelocity[0] + clvelocity[1]*clvelocity[1]), cl_bob_velocity_limit.value);

				// vertical view bobbing code
				if (cl_bob.value && cl_bobcycle.value)
				{
					float bob_limit = cl_bobmodel_classic.integer ? 4 : cl_bob_limit.value;

					if (cl_bob_limit_heightcheck.integer)
					{
						// use traces to determine what range the view can bob in, and scale down the bob as needed
						float trace1fraction;
						float trace2fraction;
						vec3_t bob_height_check_dest;

						// these multipliers are expanded a bit (the actual bob sin range is from -0.4 to 1.0) to reduce nearclip issues, especially on water surfaces
						bob_height_check_dest[0] = vieworg[0];
						bob_height_check_dest[1] = vieworg[1];
						bob_height_check_dest[2] = vieworg[2] + cl_bob_limit.value * 1.1f;
						trace = CL_TraceLine(vieworg, bob_height_check_dest, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY | (cl_bob_limit_heightcheck_dontcrosswatersurface.integer ? SUPERCONTENTS_LIQUIDSMASK : 0), 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
						trace1fraction = trace.fraction;

						bob_height_check_dest[0] = vieworg[0];
						bob_height_check_dest[1] = vieworg[1];
						bob_height_check_dest[2] = vieworg[2] + cl_bob_limit.value * -0.5f;
						trace = CL_TraceLine(vieworg, bob_height_check_dest, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY | (cl_bob_limit_heightcheck_dontcrosswatersurface.integer ? SUPERCONTENTS_LIQUIDSMASK : 0), 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
						trace2fraction = trace.fraction;

						bob_limit *= min(trace1fraction, trace2fraction);
					}

					// LadyHavoc: this code is *weird*, but not replacable (I think it
					// should be done in QC on the server, but oh well, quake is quake)
					// LadyHavoc: figured out bobup: the time at which the sin is at 180
					// degrees (which allows lengthening or squishing the peak or valley)
					cycle = cl.time / cl_bobcycle.value;
					cycle -= (int) cycle;
					if (cycle < cl_bobup.value)
						cycle = M_PI * cycle / cl_bobup.value;
					else
						cycle = M_PI + M_PI * (cycle-cl_bobup.value)/(1.0 - cl_bobup.value);
					// bob is proportional to velocity in the xy plane
					// (don't count Z, or jumping messes it up)
					bob = xyspeed * cl_bob.value;
					bob = bob*0.3 + bob*0.7*sin(cycle);
					bob = bound(-7, bob, bob_limit);

					vieworg[2] += bob;

					// we also need to adjust gunorg, or this appears like pushing the gun!
					// In the old code, this was applied to vieworg BEFORE copying to gunorg,
					// but this is not viable with the new followmodel code as that would mean
					// that followmodel would work on the munged-by-bob vieworg and do feedback
					if(!cl_bobmodel_classic.integer)
						gunorg[2] += bob;
				}

				// horizontal view bobbing code
				if (cl_bob2.value && cl_bob2cycle.value)
				{
					vec3_t bob2vel;
					vec3_t forward, right, up;
					float side, front;

					cycle = cl.time / cl_bob2cycle.value;
					cycle -= (int) cycle;
					if (cycle < 0.5)
						cycle = cos(M_PI * cycle / 0.5); // cos looks better here with the other view bobbing using sin
					else
						cycle = cos(M_PI + M_PI * (cycle-0.5)/0.5);
					bob = cl_bob2.value * cycle;

					// this value slowly decreases from 1 to 0 when we stop touching the ground.
					// The cycle is later multiplied with it so the view smooths back to normal
					if (clonground && !clcmdjump) // also block the effect while the jump button is pressed, to avoid twitches when bunny-hopping
						cl.bob2_smooth = 1;
					else
					{
						if(cl.bob2_smooth > 0)
							cl.bob2_smooth -= bound(0, cl_bob2smooth.value, 1);
						else
							cl.bob2_smooth = 0;
					}

					// calculate the front and side of the player between the X and Y axes
					AngleVectors(viewangles, forward, right, up);
					// now get the speed based on those angles. The bounds should match the same value as xyspeed's
					side = bound(-cl_bob_velocity_limit.value, DotProduct (clvelocity, right) * cl.bob2_smooth, cl_bob_velocity_limit.value);
					front = bound(-cl_bob_velocity_limit.value, DotProduct (clvelocity, forward) * cl.bob2_smooth, cl_bob_velocity_limit.value);
					VectorScale(forward, bob, forward);
					VectorScale(right, bob, right);
					// we use side with forward and front with right, so the bobbing goes
					// to the side when we walk forward and to the front when we strafe
					VectorMAMAM(side, forward, front, right, 0, up, bob2vel);
					vieworg[0] += bob2vel[0];
					vieworg[1] += bob2vel[1];
					// we also need to adjust gunorg, or this appears like pushing the gun!
					// In the old code, this was applied to vieworg BEFORE copying to gunorg,
					// but this is not viable with the new followmodel code as that would mean
					// that followmodel would work on the munged-by-bob vieworg and do feedback
					gunorg[0] += bob2vel[0];
					gunorg[1] += bob2vel[1];
				}

				// fall bobbing code
				// causes the view to swing down and back up when touching the ground
				if (cl_bobfall.value && cl_bobfallcycle.value)
				{
					if (!clonground)
					{
						cl.bobfall_speed = bound(-400, clvelocity[2], 0) * bound(0, cl_bobfall.value, 0.1);
						if (clvelocity[2] < -cl_bobfallminspeed.value)
							cl.bobfall_swing = 1;
						else
							cl.bobfall_swing = 0; // TODO really?
					}
					else
					{
						cl.bobfall_swing = max(0, cl.bobfall_swing - cl_bobfallcycle.value * frametime);

						bobfall = sin(M_PI * cl.bobfall_swing) * cl.bobfall_speed;
						vieworg[2] += bobfall;
						gunorg[2] += bobfall;
					}
				}
				VectorCopy(clviewangles, viewangles);

				// Hanicef: don't apply punchangle twice if the scene is rendered more than once.
				if (!cl_punchangle_applied)
				{
					VectorAdd(viewangles, cl.punchangle, viewangles);
					cl_punchangle_applied = 1;
				}
				viewangles[ROLL] += Com_CalcRoll(clviewangles, clvelocity, cl_rollangle.value, cl_rollspeed.value);

				if (v_dmg_time > 0)
				{
					viewangles[ROLL] += v_dmg_time/v_kicktime.value*v_dmg_roll;
					viewangles[PITCH] += v_dmg_time/v_kicktime.value*v_dmg_pitch;
				}

				// gun model bobbing code
				if (cl_bobmodel.value)
				{
					vec3_t forward, right, up;
					AngleVectors (gunangles, forward, right, up);

					if(!cl_bobmodel_classic.integer)
					{
						// calculate for swinging gun model
						// the gun bobs when running on the ground, but doesn't bob when you're in the air.
						// Sajt: I tried to smooth out the transitions between bob and no bob, which works
						// for the most part, but for some reason when you go through a message trigger or
						// pick up an item or anything like that it will momentarily jolt the gun.
						float bspeed;
						float s;
						float t;

						s = cl.time * cl_bobmodel_speed.value;
						if (clonground)
						{
							if (cl.time - cl.hitgroundtime < 0.2)
							{
								// just hit the ground, speed the bob back up over the next 0.2 seconds
								t = cl.time - cl.hitgroundtime;
								t = bound(0, t, 0.2);
								t *= 5;
							}
							else
								t = 1;
						}
						else
						{
							// recently left the ground, slow the bob down over the next 0.2 seconds
							t = cl.time - cl.lastongroundtime;
							t = 0.2 - bound(0, t, 0.2);
							t *= 5;
						}

						bspeed = xyspeed * 0.01f;
						bob = bspeed * cl_bobmodel_side.value * cl_viewmodel_scale.value * sin (s) * t;
						VectorMA (gunorg, bob, right, gunorg);
						bob = bspeed * cl_bobmodel_up.value * cl_viewmodel_scale.value * cos (s * 2) * t;
						VectorMA (gunorg, bob, up, gunorg);
					}
					else
					{
						// Classic Quake bobbing
						for (int i = 0; i < 3; i++)
							gunorg[i] += forward[i]*bob*0.4;
						gunorg[2] += bob;

						if (r_viewmodel_quake.value)
						{
							if (scr_viewsize.value == 110)
								gunorg[2] += 1;
							else if (scr_viewsize.value == 100)
								gunorg[2] += 2;
							else if (scr_viewsize.value == 90)
								gunorg[2] += 1;
							else if (scr_viewsize.value == 80)
								gunorg[2] += 0.5;
						}
					}
				}
			}
		}
		// calculate a view matrix for rendering the scene
		if (v_idlescale.value)
		{
			viewangles[0] += v_idlescale.value * sin(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
			viewangles[1] += v_idlescale.value * sin(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
			viewangles[2] += v_idlescale.value * sin(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
		}
		Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, vieworg[0], vieworg[1], vieworg[2], viewangles[0], viewangles[1], viewangles[2], 1);
		if (v_yshearing.value > 0)
			Matrix4x4_QuakeToDuke3D(&r_refdef.view.matrix, &r_refdef.view.matrix, v_yshearing.value);

		// calculate a viewmodel matrix for use in view-attached entities
		Matrix4x4_Copy(&viewmodelmatrix_nobob, &r_refdef.view.matrix);
		Matrix4x4_ConcatScale(&viewmodelmatrix_nobob, cl_viewmodel_scale.value);

		Matrix4x4_CreateFromQuakeEntity(&viewmodelmatrix_withbob, gunorg[0], gunorg[1], gunorg[2], gunangles[0], gunangles[1], gunangles[2], cl_viewmodel_scale.value);
		if (v_yshearing.value > 0)
			Matrix4x4_QuakeToDuke3D(&viewmodelmatrix_withbob, &viewmodelmatrix_withbob, v_yshearing.value);

		VectorCopy(vieworg, cl.csqc_vieworiginfromengine);
		VectorCopy(viewangles, cl.csqc_viewanglesfromengine);

		Matrix4x4_Invert_Simple(&tmpmatrix, &r_refdef.view.matrix);
		Matrix4x4_Concat(&cl.csqc_viewmodelmatrixfromengine, &tmpmatrix, &viewmodelmatrix_withbob);
	}

	cl.calcrefdef_prevtime = cl.time;
}

void V_CalcRefdef (void)
{
	entity_t *ent;
	qbool cldead;

	if (cls.state == ca_connected && cls.signon == SIGNONS && !cl.csqc_server2csqcentitynumber[cl.viewentity])
	{
		// ent is the view entity (visible when out of body)
		ent = &cl.entities[cl.viewentity];

		cldead = (cl.stats[STAT_HEALTH] <= 0 && cl.stats[STAT_HEALTH] != -666 && cl.stats[STAT_HEALTH] != -2342);
		V_CalcRefdefUsing(&ent->render.matrix, cl.viewangles, !ent->persistent.trail_allowed, cl.onground, cl.cmd.jump, cl.stats[STAT_VIEWHEIGHT], cldead, cl.velocity); // FIXME use a better way to detect teleport/warp than trail_allowed
	}
	else
	{
		viewmodelmatrix_nobob = identitymatrix;
		viewmodelmatrix_withbob = identitymatrix;
		cl.csqc_viewmodelmatrixfromengine = identitymatrix;
		r_refdef.view.matrix = identitymatrix;
		VectorClear(cl.csqc_vieworiginfromengine);
		VectorCopy(cl.viewangles, cl.csqc_viewanglesfromengine);
	}
}

void V_MakeViewIsometric(void)
{
	// when using isometric view to play normal games we have to rotate the camera to make the Ortho matrix do the right thing (forward as up the screen, etc)
	matrix4x4_t relative;
	matrix4x4_t modifiedview;
	matrix4x4_t modify;
	vec3_t forward, left, up, org;
	float t[16];

	r_refdef.view.useperspective = false;
	r_refdef.view.usevieworiginculling = !r_trippy.value && v_isometric_usevieworiginculling.integer;
	r_refdef.view.frustum_y = v_isometric_verticalfov.value * cl.viewzoom;
	r_refdef.view.frustum_x = r_refdef.view.frustum_y * (float)r_refdef.view.width / (float)r_refdef.view.height / vid_pixelheight.value;
	r_refdef.view.frustum_x *= r_refdef.frustumscale_x;
	r_refdef.view.frustum_y *= r_refdef.frustumscale_y;
	r_refdef.view.ortho_x = r_refdef.view.frustum_x; // used by VM_CL_R_SetView
	r_refdef.view.ortho_y = r_refdef.view.frustum_y; // used by VM_CL_R_SetView

	t[0]  = v_isometric_xx.value;
	t[1]  = v_isometric_xy.value;
	t[2]  = v_isometric_xz.value;
	t[3]  = 0.0f;
	t[4]  = v_isometric_yx.value;
	t[5]  = v_isometric_yy.value;
	t[6]  = v_isometric_yz.value;
	t[7]  = 0.0f;
	t[8]  = v_isometric_zx.value;
	t[9]  = v_isometric_zy.value;
	t[10] = v_isometric_zz.value;
	t[11] = 0.0f;
	t[12] = 0.0f;
	t[13] = 0.0f;
	t[14] = 0.0f;
	t[15] = 1.0f;
	Matrix4x4_FromArrayFloatGL(&modify, t);

	// if the orientation is locked, extract the origin and create just a translate matrix to start with
	if (v_isometric_locked_orientation.integer)
	{
		vec3_t vx, vy, vz, origin;
		Matrix4x4_ToVectors(&r_refdef.view.matrix, vx, vy, vz, origin);
		Matrix4x4_CreateTranslate(&r_refdef.view.matrix, origin[0], origin[1], origin[2]);
	}

	Matrix4x4_Concat(&modifiedview, &r_refdef.view.matrix, &modify);
	Matrix4x4_CreateFromQuakeEntity(&relative, v_isometric_tx.value, v_isometric_ty.value, v_isometric_tz.value, v_isometric_rot_pitch.value, v_isometric_rot_yaw.value, v_isometric_rot_roll.value, 1.0f);
	Matrix4x4_Concat(&r_refdef.view.matrix, &modifiedview, &relative);
	Matrix4x4_ToVectors(&r_refdef.view.matrix, forward, left, up, org);
	VectorMAMAMAM(1.0f, org, v_isometric_relx.value, forward, v_isometric_rely.value, left, v_isometric_relz.value, up, org);
	Matrix4x4_FromVectors(&r_refdef.view.matrix, forward, left, up, org);

	if (v_isometric_flipcullface.integer)
	{
		int a = r_refdef.view.cullface_front;
		r_refdef.view.cullface_front = r_refdef.view.cullface_back;
		r_refdef.view.cullface_back = a;
	}
}


void V_FadeViewFlashs(void)
{
	// don't flash if time steps backwards
	if (cl.time <= cl.oldtime)
		return;
	// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= (cl.time - cl.oldtime)*cl.cshifts[CSHIFT_DAMAGE].alphafade;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= (cl.time - cl.oldtime)*cl.cshifts[CSHIFT_BONUS].alphafade;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;
}

void V_CalcViewBlend(void)
{
	float a2;
	int j;
	r_refdef.viewblend[0] = 0;
	r_refdef.viewblend[1] = 0;
	r_refdef.viewblend[2] = 0;
	r_refdef.viewblend[3] = 0;
	r_refdef.frustumscale_x = 1;
	r_refdef.frustumscale_y = 1;
	if (cls.state == ca_connected && cls.signon == SIGNONS)
	{
		// set contents color
		int supercontents;
		vec3_t vieworigin;
		Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, vieworigin);
		supercontents = CL_PointSuperContents(vieworigin);
		if (supercontents & SUPERCONTENTS_LIQUIDSMASK)
		{
			cl.view_underwater = true;
			r_refdef.frustumscale_x *= 1 - (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
			r_refdef.frustumscale_y *= 1 - (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
			if (supercontents & SUPERCONTENTS_LAVA)
			{
				cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = 255;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = 80;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = 0;
			}
			else if (supercontents & SUPERCONTENTS_SLIME)
			{
				cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = 0;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = 25;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = 5;
			}
			else
			{
				cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = 130;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = 80;
				cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = 50;
			}
			cl.cshifts[CSHIFT_CONTENTS].percent = 150 * 0.5;
		}
		else
		{
			cl.view_underwater = false;
			cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = 0;
			cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = 0;
			cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = 0;
			cl.cshifts[CSHIFT_CONTENTS].percent = 0;
		}

		if (gamemode != GAME_TRANSFUSION)
		{
			if (cl.stats[STAT_ITEMS] & IT_QUAD)
			{
				cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
				cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
				cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
				cl.cshifts[CSHIFT_POWERUP].percent = 30;
			}
			else if (cl.stats[STAT_ITEMS] & IT_SUIT)
			{
				cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
				cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
				cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
				cl.cshifts[CSHIFT_POWERUP].percent = 20;
			}
			else if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
			{
				cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
				cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
				cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
				cl.cshifts[CSHIFT_POWERUP].percent = 100;
			}
			else if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
			{
				cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
				cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
				cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
				cl.cshifts[CSHIFT_POWERUP].percent = 30;
			}
			else
				cl.cshifts[CSHIFT_POWERUP].percent = 0;
		}

		cl.cshifts[CSHIFT_VCSHIFT].destcolor[0] = v_cshift.destcolor[0];
		cl.cshifts[CSHIFT_VCSHIFT].destcolor[1] = v_cshift.destcolor[1];
		cl.cshifts[CSHIFT_VCSHIFT].destcolor[2] = v_cshift.destcolor[2];
		cl.cshifts[CSHIFT_VCSHIFT].percent = v_cshift.percent;

		// LadyHavoc: fixed V_CalcBlend
		for (j = 0;j < NUM_CSHIFTS;j++)
		{
			a2 = bound(0.0f, cl.cshifts[j].percent * (1.0f / 255.0f), 1.0f);
			if (a2 > 0)
			{
				VectorLerp(r_refdef.viewblend, a2, cl.cshifts[j].destcolor, r_refdef.viewblend);
				r_refdef.viewblend[3] = (1 - (1 - r_refdef.viewblend[3]) * (1 - a2)); // correct alpha multiply...  took a while to find it on the web
			}
		}
		// saturate color (to avoid blending in black)
		if (r_refdef.viewblend[3])
		{
			a2 = 1 / r_refdef.viewblend[3];
			VectorScale(r_refdef.viewblend, a2, r_refdef.viewblend);
		}
		r_refdef.viewblend[0] = bound(0.0f, r_refdef.viewblend[0], 255.0f);
		r_refdef.viewblend[1] = bound(0.0f, r_refdef.viewblend[1], 255.0f);
		r_refdef.viewblend[2] = bound(0.0f, r_refdef.viewblend[2], 255.0f);
		r_refdef.viewblend[3] = bound(0.0f, r_refdef.viewblend[3] * gl_polyblend.value, 1.0f);
		if (vid.sRGB3D)
		{
			r_refdef.viewblend[0] = Image_LinearFloatFromsRGB(r_refdef.viewblend[0]);
			r_refdef.viewblend[1] = Image_LinearFloatFromsRGB(r_refdef.viewblend[1]);
			r_refdef.viewblend[2] = Image_LinearFloatFromsRGB(r_refdef.viewblend[2]);
		}
		else
		{
			r_refdef.viewblend[0] *= (1.0f/256.0f);
			r_refdef.viewblend[1] *= (1.0f/256.0f);
			r_refdef.viewblend[2] *= (1.0f/256.0f);
		}
		
		// Samual: Ugly hack, I know. But it's the best we can do since
		// there is no way to detect client states from the engine.
		if (cl.stats[STAT_HEALTH] <= 0 && cl.stats[STAT_HEALTH] != -666 && 
			cl.stats[STAT_HEALTH] != -2342 && cl_deathfade.value > 0)
		{
			cl.deathfade += cl_deathfade.value * max(0.00001, cl.time - cl.oldtime);
			cl.deathfade = bound(0.0f, cl.deathfade, 0.9f);
		}
		else
			cl.deathfade = 0.0f;

		if(cl.deathfade > 0)
		{
			float a;
			float deathfadevec[3] = {0.3f, 0.0f, 0.0f};
			a = r_refdef.viewblend[3] + cl.deathfade - r_refdef.viewblend[3]*cl.deathfade;
			if(a > 0)
				VectorMAM(r_refdef.viewblend[3] * (1 - cl.deathfade) / a, r_refdef.viewblend, cl.deathfade / a, deathfadevec, r_refdef.viewblend);
			r_refdef.viewblend[3] = a;
		}
	}
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "v_cshift", V_cshift_f, "sets tint color of view");
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "bf", V_BonusFlash_f, "briefly flashes a bright color tint on view (used when items are picked up); optionally takes R G B [A [alphafade]] arguments to specify how the flash looks");
	Cmd_AddCommand(CF_CLIENT, "centerview", V_StartPitchDrift_f, "gradually recenter view (stop looking up/down)");

	Cvar_RegisterVariable (&v_centermove);
	Cvar_RegisterVariable (&v_centerspeed);

	Cvar_RegisterVariable (&v_iyaw_cycle);
	Cvar_RegisterVariable (&v_iroll_cycle);
	Cvar_RegisterVariable (&v_ipitch_cycle);
	Cvar_RegisterVariable (&v_iyaw_level);
	Cvar_RegisterVariable (&v_iroll_level);
	Cvar_RegisterVariable (&v_ipitch_level);

	Cvar_RegisterVariable(&v_isometric);
	Cvar_RegisterVariable(&v_isometric_verticalfov);
	Cvar_RegisterVariable(&v_isometric_xx);
	Cvar_RegisterVariable(&v_isometric_xy);
	Cvar_RegisterVariable(&v_isometric_xz);
	Cvar_RegisterVariable(&v_isometric_yx);
	Cvar_RegisterVariable(&v_isometric_yy);
	Cvar_RegisterVariable(&v_isometric_yz);
	Cvar_RegisterVariable(&v_isometric_zx);
	Cvar_RegisterVariable(&v_isometric_zy);
	Cvar_RegisterVariable(&v_isometric_zz);
	Cvar_RegisterVariable(&v_isometric_tx);
	Cvar_RegisterVariable(&v_isometric_ty);
	Cvar_RegisterVariable(&v_isometric_tz);
	Cvar_RegisterVariable(&v_isometric_rot_pitch);
	Cvar_RegisterVariable(&v_isometric_rot_yaw);
	Cvar_RegisterVariable(&v_isometric_rot_roll);
	Cvar_RegisterVariable(&v_isometric_relx);
	Cvar_RegisterVariable(&v_isometric_rely);
	Cvar_RegisterVariable(&v_isometric_relz);
	Cvar_RegisterVariable(&v_isometric_flipcullface);
	Cvar_RegisterVariable(&v_isometric_locked_orientation);
	Cvar_RegisterVariable(&v_isometric_usevieworiginculling);

	Cvar_RegisterVariable (&v_idlescale);
	Cvar_RegisterVariable (&crosshair);

	Cvar_RegisterVariable (&cl_rollspeed);
	Cvar_RegisterVariable (&cl_rollangle);
	Cvar_RegisterVariable (&cl_bob);
	Cvar_RegisterVariable (&cl_bobcycle);
	Cvar_RegisterVariable (&cl_bobup);
	Cvar_RegisterVariable (&cl_bob2);
	Cvar_RegisterVariable (&cl_bob2cycle);
	Cvar_RegisterVariable (&cl_bob2smooth);
	Cvar_RegisterVariable (&cl_bobfall);
	Cvar_RegisterVariable (&cl_bobfallcycle);
	Cvar_RegisterVariable (&cl_bobfallminspeed);
	Cvar_RegisterVariable (&cl_bobmodel);
	Cvar_RegisterVariable (&cl_bobmodel_side);
	Cvar_RegisterVariable (&cl_bobmodel_up);
	Cvar_RegisterVariable (&cl_bobmodel_forward);
	Cvar_RegisterVariable (&cl_bobmodel_classic);
	Cvar_RegisterVariable (&cl_bobmodel_speed);
	Cvar_RegisterVariable (&cl_bob_limit);
	Cvar_RegisterVariable (&cl_bob_limit_heightcheck);
	Cvar_RegisterVariable (&cl_bob_limit_heightcheck_dontcrosswatersurface);
	Cvar_RegisterVariable (&cl_bob_velocity_limit);

	Cvar_RegisterVariable (&cl_leanmodel);
	Cvar_RegisterVariable (&cl_leanmodel_side_speed);
	Cvar_RegisterVariable (&cl_leanmodel_side_limit);
	Cvar_RegisterVariable (&cl_leanmodel_side_highpass1);
	Cvar_RegisterVariable (&cl_leanmodel_side_lowpass);
	Cvar_RegisterVariable (&cl_leanmodel_side_highpass);
	Cvar_RegisterVariable (&cl_leanmodel_up_speed);
	Cvar_RegisterVariable (&cl_leanmodel_up_limit);
	Cvar_RegisterVariable (&cl_leanmodel_up_highpass1);
	Cvar_RegisterVariable (&cl_leanmodel_up_lowpass);
	Cvar_RegisterVariable (&cl_leanmodel_up_highpass);

	Cvar_RegisterVariable (&cl_followmodel);
	Cvar_RegisterVariable (&cl_followmodel_side_speed);
	Cvar_RegisterVariable (&cl_followmodel_side_limit);
	Cvar_RegisterVariable (&cl_followmodel_side_highpass1);
	Cvar_RegisterVariable (&cl_followmodel_side_lowpass);
	Cvar_RegisterVariable (&cl_followmodel_side_highpass);
	Cvar_RegisterVariable (&cl_followmodel_up_speed);
	Cvar_RegisterVariable (&cl_followmodel_up_limit);
	Cvar_RegisterVariable (&cl_followmodel_up_highpass1);
	Cvar_RegisterVariable (&cl_followmodel_up_lowpass);
	Cvar_RegisterVariable (&cl_followmodel_up_highpass);

	Cvar_RegisterVariable (&cl_viewmodel_scale);

	Cvar_RegisterVariable (&v_kicktime);
	Cvar_RegisterVariable (&v_kickroll);
	Cvar_RegisterVariable (&v_kickpitch);

	Cvar_RegisterVariable (&cl_stairsmoothspeed);
	
	Cvar_RegisterVariable (&cl_smoothviewheight);

	Cvar_RegisterVariable (&chase_back);
	Cvar_RegisterVariable (&chase_up);
	Cvar_RegisterVariable (&chase_active);
	Cvar_RegisterVariable (&chase_overhead);
	Cvar_RegisterVariable (&chase_pitchangle);
	Cvar_RegisterVariable (&chase_stevie);

	Cvar_RegisterVariable (&v_deathtilt);
	Cvar_RegisterVariable (&v_deathtiltangle);

	Cvar_RegisterVariable (&v_yshearing);
	Cvar_RegisterVariable (&r_viewmodel_quake);
}

