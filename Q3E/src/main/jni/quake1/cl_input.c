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
// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"
#include "csprogs.h"
#include "thread.h"

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_jump, in_attack, in_use;
kbutton_t	in_up, in_down;
// LadyHavoc: added 6 new buttons
kbutton_t	in_button3, in_button4, in_button5, in_button6, in_button7, in_button8;
//even more
kbutton_t	in_button9, in_button10, in_button11, in_button12, in_button13, in_button14, in_button15, in_button16;

int			in_impulse;



static void KeyDown (cmd_state_t *cmd, kbutton_t *b)
{
	int k;
	const char *c;

	c = Cmd_Argv(cmd, 1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_Print("Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

static void KeyUp (cmd_state_t *cmd, kbutton_t *b)
{
	int k;
	const char *c;

	c = Cmd_Argv(cmd, 1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)
	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

static void IN_KLookDown(cmd_state_t *cmd) {KeyDown(cmd, &in_klook);}
static void IN_KLookUp(cmd_state_t *cmd) {KeyUp(cmd, &in_klook);}
static void IN_MLookDown(cmd_state_t *cmd) {KeyDown(cmd, &in_mlook);}
static void IN_MLookUp(cmd_state_t *cmd)
{
	KeyUp(cmd, &in_mlook);
	if ( !(in_mlook.state&1) && lookspring.value)
		V_StartPitchDrift_f(cmd);
}
static void IN_UpDown(cmd_state_t *cmd) {KeyDown(cmd, &in_up);}
static void IN_UpUp(cmd_state_t *cmd) {KeyUp(cmd, &in_up);}
static void IN_DownDown(cmd_state_t *cmd) {KeyDown(cmd, &in_down);}
static void IN_DownUp(cmd_state_t *cmd) {KeyUp(cmd, &in_down);}
static void IN_LeftDown(cmd_state_t *cmd) {KeyDown(cmd, &in_left);}
static void IN_LeftUp(cmd_state_t *cmd) {KeyUp(cmd, &in_left);}
static void IN_RightDown(cmd_state_t *cmd) {KeyDown(cmd, &in_right);}
static void IN_RightUp(cmd_state_t *cmd) {KeyUp(cmd, &in_right);}
static void IN_ForwardDown(cmd_state_t *cmd) {KeyDown(cmd, &in_forward);}
static void IN_ForwardUp(cmd_state_t *cmd) {KeyUp(cmd, &in_forward);}
static void IN_BackDown(cmd_state_t *cmd) {KeyDown(cmd, &in_back);}
static void IN_BackUp(cmd_state_t *cmd) {KeyUp(cmd, &in_back);}
static void IN_LookupDown(cmd_state_t *cmd) {KeyDown(cmd, &in_lookup);}
static void IN_LookupUp(cmd_state_t *cmd) {KeyUp(cmd, &in_lookup);}
static void IN_LookdownDown(cmd_state_t *cmd) {KeyDown(cmd, &in_lookdown);}
static void IN_LookdownUp(cmd_state_t *cmd) {KeyUp(cmd, &in_lookdown);}
static void IN_MoveleftDown(cmd_state_t *cmd) {KeyDown(cmd, &in_moveleft);}
static void IN_MoveleftUp(cmd_state_t *cmd) {KeyUp(cmd, &in_moveleft);}
static void IN_MoverightDown(cmd_state_t *cmd) {KeyDown(cmd, &in_moveright);}
static void IN_MoverightUp(cmd_state_t *cmd) {KeyUp(cmd, &in_moveright);}

static void IN_SpeedDown(cmd_state_t *cmd) {KeyDown(cmd, &in_speed);}
static void IN_SpeedUp(cmd_state_t *cmd) {KeyUp(cmd, &in_speed);}
static void IN_StrafeDown(cmd_state_t *cmd) {KeyDown(cmd, &in_strafe);}
static void IN_StrafeUp(cmd_state_t *cmd) {KeyUp(cmd, &in_strafe);}

static void IN_AttackDown(cmd_state_t *cmd) {KeyDown(cmd, &in_attack);}
static void IN_AttackUp(cmd_state_t *cmd) {KeyUp(cmd, &in_attack);}

static void IN_UseDown(cmd_state_t *cmd) {KeyDown(cmd, &in_use);}
static void IN_UseUp(cmd_state_t *cmd) {KeyUp(cmd, &in_use);}

// LadyHavoc: added 6 new buttons
static void IN_Button3Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button3);}
static void IN_Button3Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button3);}
static void IN_Button4Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button4);}
static void IN_Button4Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button4);}
static void IN_Button5Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button5);}
static void IN_Button5Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button5);}
static void IN_Button6Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button6);}
static void IN_Button6Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button6);}
static void IN_Button7Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button7);}
static void IN_Button7Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button7);}
static void IN_Button8Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button8);}
static void IN_Button8Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button8);}

static void IN_Button9Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button9);}
static void IN_Button9Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button9);}
static void IN_Button10Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button10);}
static void IN_Button10Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button10);}
static void IN_Button11Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button11);}
static void IN_Button11Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button11);}
static void IN_Button12Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button12);}
static void IN_Button12Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button12);}
static void IN_Button13Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button13);}
static void IN_Button13Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button13);}
static void IN_Button14Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button14);}
static void IN_Button14Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button14);}
static void IN_Button15Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button15);}
static void IN_Button15Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button15);}
static void IN_Button16Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button16);}
static void IN_Button16Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button16);}

static void IN_JumpDown(cmd_state_t *cmd) {KeyDown(cmd, &in_jump);}
static void IN_JumpUp(cmd_state_t *cmd) {KeyUp(cmd, &in_jump);}

static void IN_Impulse(cmd_state_t *cmd) {in_impulse=atoi(Cmd_Argv(cmd, 1));}

in_bestweapon_info_t in_bestweapon_info[IN_BESTWEAPON_MAX];

static void IN_BestWeapon_Register(const char *name, int impulse, int weaponbit, int activeweaponcode, int ammostat, int ammomin)
{
	int i;
	for(i = 0; i < IN_BESTWEAPON_MAX && in_bestweapon_info[i].impulse; ++i)
		if(in_bestweapon_info[i].impulse == impulse)
			break;
	if(i >= IN_BESTWEAPON_MAX)
	{
		Con_Printf("no slot left for weapon definition; increase IN_BESTWEAPON_MAX\n");
		return; // sorry
	}
	dp_strlcpy(in_bestweapon_info[i].name, name, sizeof(in_bestweapon_info[i].name));
	in_bestweapon_info[i].impulse = impulse;
	if(weaponbit != -1)
		in_bestweapon_info[i].weaponbit = weaponbit;
	if(activeweaponcode != -1)
		in_bestweapon_info[i].activeweaponcode = activeweaponcode;
	if(ammostat != -1)
		in_bestweapon_info[i].ammostat = ammostat;
	if(ammomin != -1)
		in_bestweapon_info[i].ammomin = ammomin;
}

void IN_BestWeapon_ResetData (void)
{
	memset(in_bestweapon_info, 0, sizeof(in_bestweapon_info));
	IN_BestWeapon_Register("1", 1, IT_AXE, IT_AXE, STAT_SHELLS, 0);
	IN_BestWeapon_Register("2", 2, IT_SHOTGUN, IT_SHOTGUN, STAT_SHELLS, 1);
	IN_BestWeapon_Register("3", 3, IT_SUPER_SHOTGUN, IT_SUPER_SHOTGUN, STAT_SHELLS, 1);
	IN_BestWeapon_Register("4", 4, IT_NAILGUN, IT_NAILGUN, STAT_NAILS, 1);
	IN_BestWeapon_Register("5", 5, IT_SUPER_NAILGUN, IT_SUPER_NAILGUN, STAT_NAILS, 1);
	IN_BestWeapon_Register("6", 6, IT_GRENADE_LAUNCHER, IT_GRENADE_LAUNCHER, STAT_ROCKETS, 1);
	IN_BestWeapon_Register("7", 7, IT_ROCKET_LAUNCHER, IT_ROCKET_LAUNCHER, STAT_ROCKETS, 1);
	IN_BestWeapon_Register("8", 8, IT_LIGHTNING, IT_LIGHTNING, STAT_CELLS, 1);
	IN_BestWeapon_Register("9", 9, 128, 128, STAT_CELLS, 1); // generic energy weapon for mods
	IN_BestWeapon_Register("p", 209, 128, 128, STAT_CELLS, 1); // dpmod plasma gun
	IN_BestWeapon_Register("w", 210, 8388608, 8388608, STAT_CELLS, 1); // dpmod plasma wave cannon
	IN_BestWeapon_Register("l", 225, HIT_LASER_CANNON, HIT_LASER_CANNON, STAT_CELLS, 1); // hipnotic laser cannon
	IN_BestWeapon_Register("h", 226, HIT_MJOLNIR, HIT_MJOLNIR, STAT_CELLS, 0); // hipnotic mjolnir hammer
}

static void IN_BestWeapon_Register_f(cmd_state_t *cmd)
{
	if(Cmd_Argc(cmd) == 7)
	{
		IN_BestWeapon_Register(
			Cmd_Argv(cmd, 1),
			atoi(Cmd_Argv(cmd, 2)),
			atoi(Cmd_Argv(cmd, 3)),
			atoi(Cmd_Argv(cmd, 4)),
			atoi(Cmd_Argv(cmd, 5)),
			atoi(Cmd_Argv(cmd, 6))
		);
	}
	else if(Cmd_Argc(cmd) == 2 && !strcmp(Cmd_Argv(cmd, 1), "clear"))
	{
		memset(in_bestweapon_info, 0, sizeof(in_bestweapon_info));
	}
	else if(Cmd_Argc(cmd) == 2 && !strcmp(Cmd_Argv(cmd, 1), "quake"))
	{
		IN_BestWeapon_ResetData();
	}
	else
	{
		Con_Printf("Usage: %s weaponshortname impulse itemcode activeweaponcode ammostat ammomin; %s clear; %s quake\n", Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0));
	}
}

static void IN_BestWeapon_f(cmd_state_t *cmd)
{
	int i, n;
	const char *t;
	if (Cmd_Argc(cmd) < 2)
	{
		Con_Printf("bestweapon requires 1 or more parameters\n");
		return;
	}
	for (i = 1;i < Cmd_Argc(cmd);i++)
	{
		t = Cmd_Argv(cmd, i);
		// figure out which weapon this character refers to
		for (n = 0;n < IN_BESTWEAPON_MAX && in_bestweapon_info[n].impulse;n++)
		{
			if (!strcmp(in_bestweapon_info[n].name, t))
			{
				// we found out what weapon this character refers to
				// check if the inventory contains the weapon and enough ammo
				if ((cl.stats[STAT_ITEMS] & in_bestweapon_info[n].weaponbit) && (cl.stats[in_bestweapon_info[n].ammostat] >= in_bestweapon_info[n].ammomin))
				{
					// we found one of the weapons the player wanted
					// send an impulse to switch to it
					in_impulse = in_bestweapon_info[n].impulse;
					return;
				}
				break;
			}
		}
		// if we couldn't identify the weapon we just ignore it and continue checking for other weapons
	}
	// if we couldn't find any of the weapons, there's nothing more we can do...
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
#ifdef _DIII4A
extern void CL_Analog(const kbutton_t *key, float *val);
#endif
float CL_KeyState (kbutton_t *key)
{
	float		val;
	qbool	impulsedown, impulseup, down;

	impulsedown = (key->state & 2) != 0;
	impulseup = (key->state & 4) != 0;
	down = (key->state & 1) != 0;
	val = 0;

	if (impulsedown && !impulseup)
	{
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}
	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}
	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}
	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state &= 1;		// clear impulses

#ifdef _DIII4A
	//ANALOG
	CL_Analog(key, &val);
	//END
#endif

	return val;
}




//==========================================================================

cvar_t cl_upspeed = {CF_CLIENT | CF_ARCHIVE, "cl_upspeed","400","vertical movement speed (while swimming or flying)"};
cvar_t cl_forwardspeed = {CF_CLIENT | CF_ARCHIVE, "cl_forwardspeed","400","forward movement speed"};
cvar_t cl_backspeed = {CF_CLIENT | CF_ARCHIVE, "cl_backspeed","400","backward movement speed"};
cvar_t cl_sidespeed = {CF_CLIENT | CF_ARCHIVE, "cl_sidespeed","350","strafe movement speed"};

cvar_t cl_movespeedkey = {CF_CLIENT | CF_ARCHIVE, "cl_movespeedkey","2.0","how much +speed multiplies keyboard movement speed"};
cvar_t cl_movecliptokeyboard = {CF_CLIENT, "cl_movecliptokeyboard", "0", "if set to 1, any move is clipped to the nine keyboard states; if set to 2, only the direction is clipped, not the amount"};

cvar_t cl_yawspeed = {CF_CLIENT | CF_ARCHIVE, "cl_yawspeed","140","keyboard yaw turning speed"};
cvar_t cl_pitchspeed = {CF_CLIENT | CF_ARCHIVE, "cl_pitchspeed","150","keyboard pitch turning speed"};

cvar_t cl_anglespeedkey = {CF_CLIENT | CF_ARCHIVE, "cl_anglespeedkey","1.5","how much +speed multiplies keyboard turning speed"};

cvar_t cl_movement = {CF_CLIENT | CF_ARCHIVE, "cl_movement", "0", "enables clientside prediction of your player movement on DP servers (use cl_nopred for QWSV servers)"};
cvar_t cl_movement_replay = {CF_CLIENT, "cl_movement_replay", "1", "use engine prediction"};
cvar_t cl_movement_nettimeout = {CF_CLIENT | CF_ARCHIVE, "cl_movement_nettimeout", "0.3", "stops predicting moves when server is lagging badly (avoids major performance problems), timeout in seconds"};
cvar_t cl_movement_minping = {CF_CLIENT | CF_ARCHIVE, "cl_movement_minping", "0", "whether to use prediction when ping is lower than this value in milliseconds"};
cvar_t cl_movement_track_canjump = {CF_CLIENT | CF_ARCHIVE, "cl_movement_track_canjump", "1", "track if the player released the jump key between two jumps to decide if he is able to jump or not; when off, this causes some \"sliding\" slightly above the floor when the jump key is held too long; if the mod allows repeated jumping by holding space all the time, this has to be set to zero too"};
cvar_t cl_movement_maxspeed = {CF_CLIENT, "cl_movement_maxspeed", "320", "how fast you can move (should match sv_maxspeed)"};
cvar_t cl_movement_maxairspeed = {CF_CLIENT, "cl_movement_maxairspeed", "30", "how fast you can move while in the air (should match sv_maxairspeed)"};
cvar_t cl_movement_stopspeed = {CF_CLIENT, "cl_movement_stopspeed", "100", "speed below which you will be slowed rapidly to a stop rather than sliding endlessly (should match sv_stopspeed)"};
cvar_t cl_movement_friction = {CF_CLIENT, "cl_movement_friction", "4", "how fast you slow down (should match sv_friction)"};
cvar_t cl_movement_wallfriction = {CF_CLIENT, "cl_movement_wallfriction", "1", "how fast you slow down while sliding along a wall (should match sv_wallfriction)"};
cvar_t cl_movement_waterfriction = {CF_CLIENT, "cl_movement_waterfriction", "-1", "how fast you slow down (should match sv_waterfriction), if less than 0 the cl_movement_friction variable is used instead"};
cvar_t cl_movement_edgefriction = {CF_CLIENT, "cl_movement_edgefriction", "1", "how much to slow down when you may be about to fall off a ledge (should match edgefriction)"};
cvar_t cl_movement_stepheight = {CF_CLIENT, "cl_movement_stepheight", "18", "how tall a step you can step in one instant (should match sv_stepheight)"};
cvar_t cl_movement_accelerate = {CF_CLIENT, "cl_movement_accelerate", "10", "how fast you accelerate (should match sv_accelerate)"};
cvar_t cl_movement_airaccelerate = {CF_CLIENT, "cl_movement_airaccelerate", "-1", "how fast you accelerate while in the air (should match sv_airaccelerate), if less than 0 the cl_movement_accelerate variable is used instead"};
cvar_t cl_movement_wateraccelerate = {CF_CLIENT, "cl_movement_wateraccelerate", "-1", "how fast you accelerate while in water (should match sv_wateraccelerate), if less than 0 the cl_movement_accelerate variable is used instead"};
cvar_t cl_movement_jumpvelocity = {CF_CLIENT, "cl_movement_jumpvelocity", "270", "how fast you move upward when you begin a jump (should match the quakec code)"};
cvar_t cl_movement_airaccel_qw = {CF_CLIENT, "cl_movement_airaccel_qw", "1", "ratio of QW-style air control as opposed to simple acceleration (reduces speed gain when zigzagging) (should match sv_airaccel_qw); when < 0, the speed is clamped against the maximum allowed forward speed after the move"};
cvar_t cl_movement_airaccel_sideways_friction = {CF_CLIENT, "cl_movement_airaccel_sideways_friction", "0", "anti-sideways movement stabilization (should match sv_airaccel_sideways_friction); when < 0, only so much friction is applied that braking (by accelerating backwards) cannot be stronger"};
cvar_t cl_nopred = {CF_CLIENT | CF_ARCHIVE, "cl_nopred", "0", "(QWSV only) disables player movement prediction when playing on QWSV servers (this setting is separate from cl_movement because player expectations are different when playing on DP vs QW servers)"};

cvar_t in_pitch_min = {CF_CLIENT, "in_pitch_min", "-90", "how far you can aim upward (quake used -70)"};
cvar_t in_pitch_max = {CF_CLIENT, "in_pitch_max", "90", "how far you can aim downward (quake used 80)"};

cvar_t m_filter = {CF_CLIENT | CF_ARCHIVE, "m_filter","0", "smoothes mouse movement, less responsive but smoother aiming"}; 
cvar_t m_accelerate = {CF_CLIENT | CF_ARCHIVE, "m_accelerate","1", "linear mouse acceleration factor (set to 1 to disable the linear acceleration and use only the power or natural acceleration; set to 0 to disable all acceleration)"};
cvar_t m_accelerate_minspeed = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_minspeed","5000", "below this speed in px/s, no acceleration is done, with a linear slope between (applied only on linear acceleration)"};
cvar_t m_accelerate_maxspeed = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_maxspeed","10000", "above this speed in px/s, full acceleration is done, with a linear slope between (applied only on linear acceleration)"};
cvar_t m_accelerate_filter = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_filter","0", "linear mouse acceleration factor filtering lowpass constant in seconds (set to 0 for no filtering)"};
cvar_t m_accelerate_power_offset = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_offset","0", "below this speed in px/ms, no power acceleration is done"};
cvar_t m_accelerate_power = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power","2", "acceleration power (must be above 1 to be useful)"};
cvar_t m_accelerate_power_senscap = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_senscap", "0", "maximum acceleration factor generated by power acceleration; use 0 for unbounded"};
cvar_t m_accelerate_power_strength = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_strength", "0", "strength of the power mouse acceleration effect"};
cvar_t m_accelerate_natural_strength = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_strength", "0", "How quickly the accelsensitivity approaches the m_accelerate_natural_accelsenscap, values are compressed between 0 and 1 but higher numbers are allowed"};
cvar_t m_accelerate_natural_accelsenscap = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_accelsenscap", "0", "Horizontal asymptote that sets the maximum value for the natural mouse acceleration curve, value 2, for example, means that the maximum sensitivity is 2 times the base sensitivity"};
cvar_t m_accelerate_natural_offset = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_offset", "0", "below this speed in px/ms, no natural acceleration is done"};

cvar_t cl_netfps = {CF_CLIENT | CF_ARCHIVE, "cl_netfps","72", "how many input packets to send to server each second"};
cvar_t cl_netrepeatinput = {CF_CLIENT | CF_ARCHIVE, "cl_netrepeatinput", "1", "how many packets in a row can be lost without movement issues when using cl_movement (technically how many input messages to repeat in each packet that have not yet been acknowledged by the server), only affects DP7 and later servers (Quake uses 0, QuakeWorld uses 2, and just for comparison Quake3 uses 1)"};
cvar_t cl_netimmediatebuttons = {CF_CLIENT | CF_ARCHIVE, "cl_netimmediatebuttons", "1", "sends extra packets whenever your buttons change or an impulse is used (basically: whenever you click fire or change weapon)"};

cvar_t cl_nodelta = {CF_CLIENT, "cl_nodelta", "0", "disables delta compression of non-player entities in QW network protocol"};

cvar_t cl_csqc_generatemousemoveevents = {CF_CLIENT, "cl_csqc_generatemousemoveevents", "1", "enables calls to CSQC_InputEvent with type 2, for compliance with EXT_CSQC spec"};

extern cvar_t v_flipped;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (in_speed.state & 1)
		speed = cl.realframetime * cl_anglespeedkey.value;
	else
		speed = cl.realframetime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed.value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed.value*CL_KeyState (&in_left);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed.value * CL_KeyState (&in_back);
	}

	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed.value * down;

	if (up || down)
		V_StopPitchDrift ();

	cl.viewangles[YAW] = ANGLEMOD(cl.viewangles[YAW]);
	cl.viewangles[PITCH] = ANGLEMOD(cl.viewangles[PITCH]);
	if (cl.viewangles[YAW] >= 180)
		cl.viewangles[YAW] -= 360;
	if (cl.viewangles[PITCH] >= 180)
		cl.viewangles[PITCH] -= 360;
        // TODO: honor serverinfo minpitch and maxpitch values in PROTOCOL_QUAKEWORLD
        // TODO: honor proquake pq_fullpitch cvar when playing on proquake server (server stuffcmd's this to 0 usually)
	cl.viewangles[PITCH] = bound(in_pitch_min.value, cl.viewangles[PITCH], in_pitch_max.value);
	cl.viewangles[ROLL] = bound(-180, cl.viewangles[ROLL], 180);
}

int cl_ignoremousemoves = 2;

/*
================
CL_Input

Send the intended movement message to the server
================
*/
void CL_Input (void)
{
	float mx, my;
	static float old_mouse_x = 0, old_mouse_y = 0;

	// clamp before the move to prevent starting with bad angles
	CL_AdjustAngles ();

	if(v_flipped.integer)
		cl.viewangles[YAW] = -cl.viewangles[YAW];

	// reset some of the command fields
	cl.cmd.forwardmove = 0;
	cl.cmd.sidemove = 0;
	cl.cmd.upmove = 0;

	// get basic movement from keyboard
	if (in_strafe.state & 1)
	{
		cl.cmd.sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
		cl.cmd.sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
	}

	cl.cmd.sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
	cl.cmd.sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);

	cl.cmd.upmove += cl_upspeed.value * CL_KeyState (&in_up);
	cl.cmd.upmove -= cl_upspeed.value * CL_KeyState (&in_down);

	if (! (in_klook.state & 1) )
	{
		cl.cmd.forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
		cl.cmd.forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
	}

	// adjust for speed key
	if (in_speed.state & 1)
	{
		cl.cmd.forwardmove *= cl_movespeedkey.value;
		cl.cmd.sidemove *= cl_movespeedkey.value;
		cl.cmd.upmove *= cl_movespeedkey.value;
	}

	// allow mice or other external controllers to add to the move
	IN_Move ();

	// send mouse move to csqc
	if (CLVM_prog->loaded && cl_csqc_generatemousemoveevents.integer)
	{
		if (cl.csqc_wantsmousemove)
		{
			// event type 3 is a DP_CSQC thing
			static int oldwindowmouse[2];
			if (oldwindowmouse[0] != in_windowmouse_x || oldwindowmouse[1] != in_windowmouse_y)
			{
				CL_VM_InputEvent(3, in_windowmouse_x * vid_conwidth.value / vid.mode.width, in_windowmouse_y * vid_conheight.value / vid.mode.height);
				oldwindowmouse[0] = in_windowmouse_x;
				oldwindowmouse[1] = in_windowmouse_y;
			}
		}
		else
		{
			if (in_mouse_x || in_mouse_y)
				CL_VM_InputEvent(2, in_mouse_x, in_mouse_y);
		}
	}

	// apply m_accelerate if it is on
	if(m_accelerate.value > 0)
	{
		float mouse_deltadist = sqrtf(in_mouse_x * in_mouse_x + in_mouse_y * in_mouse_y);
		float speed = mouse_deltadist / cl.realframetime;
		static float averagespeed = 0;
		float f, mi, ma;
		if(m_accelerate_filter.value > 0)
			f = bound(0, cl.realframetime / m_accelerate_filter.value, 1);
		else
			f = 1;
		averagespeed = speed * f + averagespeed * (1 - f);

		// Note: this check is technically unnecessary, as everything in here cancels out if it is zero.
		if (m_accelerate.value != 1.0f)
		{
			// First do linear slope acceleration which was ripped "in
			// spirit" from many classic mouse driver implementations.
			// If m_accelerate.value == 1, this code does nothing at all.

			mi = max(1, m_accelerate_minspeed.value);
			ma = max(m_accelerate_minspeed.value + 1, m_accelerate_maxspeed.value);

			if(averagespeed <= mi)
			{
				f = 1;
			}
			else if(averagespeed >= ma)
			{
				f = m_accelerate.value;
			}
			else
			{
				f = averagespeed;
				f = (f - mi) / (ma - mi) * (m_accelerate.value - 1) + 1;
			}
			in_mouse_x *= f;
			in_mouse_y *= f;
		}

		// Note: this check is technically unnecessary, as everything in here cancels out if it is zero.
		if (m_accelerate_power_strength.value != 0.0f)
		{
			// Then do Quake Live-style power acceleration.
			// Note that this behavior REPLACES the usual
			// sensitivity, so we apply it but then divide by
			// sensitivity.value so that the later multiplication
			// restores it again.
			float accelsens = 1.0f;
			float adjusted_speed_pxms = (averagespeed * 0.001f - m_accelerate_power_offset.value) * m_accelerate_power_strength.value;
			float inv_sensitivity = 1.0f / sensitivity.value;
			if (adjusted_speed_pxms > 0)
			{
				if (m_accelerate_power.value > 1.0f)
				{
					// TODO: How does this interact with sensitivity changes? Is this intended?
					// Currently: more sensitivity = less acceleration at same pixel speed.
					accelsens += expf((m_accelerate_power.value - 1.0f) * logf(adjusted_speed_pxms)) * inv_sensitivity;
				}
				else
				{
					// The limit of the then-branch for m_accelerate_power -> 1.
					accelsens += inv_sensitivity;
					// Note: QL had just accelsens = 1.0f.
					// This is mathematically wrong though.
				}
			}
			else
			{
				// The limit of the then-branch for adjusted_speed -> 0.
				// accelsens += 0.0f;
			}
			if (m_accelerate_power_senscap.value > 0.0f && accelsens > m_accelerate_power_senscap.value * inv_sensitivity)
			{
				// TODO: How does this interact with sensitivity changes? Is this intended?
				// Currently: senscap is in absolute sensitivity units, so if senscap < sensitivity, it overrides.
				accelsens = m_accelerate_power_senscap.value * inv_sensitivity;
			}

			in_mouse_x *= accelsens;
			in_mouse_y *= accelsens;
		}

		if (m_accelerate_natural_strength.value > 0.0f && m_accelerate_natural_accelsenscap.value >= 0.0f)
		{
			float accelsens = 1.0f;
			float adjusted_speed_pxms = (averagespeed * 0.001f - m_accelerate_natural_offset.value);

			if (adjusted_speed_pxms > 0 && m_accelerate_natural_accelsenscap.value != 1.0f)
			{
				float adjusted_accelsenscap = m_accelerate_natural_accelsenscap.value - 1.0f;
				// This equation is made to multiply the sensitivity for a factor between 1 and m_accelerate_natural_accelsenscap
				// this means there is no need to divide it for the sensitivity.value as the whole
				// expression needs to be multiplied by the sensitivity at the end instead of only having the sens multiplied
				accelsens += (adjusted_accelsenscap - adjusted_accelsenscap * exp( - ((adjusted_speed_pxms * m_accelerate_natural_strength.value) / fabs(adjusted_accelsenscap) )));
			}

			in_mouse_x *= accelsens;
			in_mouse_y *= accelsens;
		}
	}

	// apply m_filter if it is on
	mx = in_mouse_x;
	my = in_mouse_y;
	if (m_filter.integer)
	{
		in_mouse_x = (mx + old_mouse_x) * 0.5;
		in_mouse_y = (my + old_mouse_y) * 0.5;
	}
	old_mouse_x = mx;
	old_mouse_y = my;

	// ignore a mouse move if mouse was activated/deactivated this frame
	if (cl_ignoremousemoves)
	{
		cl_ignoremousemoves--;
		in_mouse_x = old_mouse_x = 0;
		in_mouse_y = old_mouse_y = 0;
	}

	// if not in menu, apply mouse move to viewangles/movement
	if (!key_consoleactive && key_dest == key_game && !cl.csqc_wantsmousemove && cl_prydoncursor.integer <= 0)
	{
		float modulatedsensitivity = sensitivity.value * cl.sensitivityscale;
		if (in_strafe.state & 1)
		{
			// strafing mode, all looking is movement
			V_StopPitchDrift();
			cl.cmd.sidemove += m_side.value * in_mouse_x * modulatedsensitivity;
			if (noclip_anglehack)
				cl.cmd.upmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
			else
				cl.cmd.forwardmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
		}
		else if ((in_mlook.state & 1) || freelook.integer)
		{
			// mouselook, lookstrafe causes turning to become strafing
			V_StopPitchDrift();
			if (lookstrafe.integer)
				cl.cmd.sidemove += m_side.value * in_mouse_x * modulatedsensitivity;
			else
				cl.viewangles[YAW] -= m_yaw.value * in_mouse_x * modulatedsensitivity * cl.viewzoom;
			cl.viewangles[PITCH] += m_pitch.value * in_mouse_y * modulatedsensitivity * cl.viewzoom;
		}
		else
		{
			// non-mouselook, yaw turning and forward/back movement
			cl.viewangles[YAW] -= m_yaw.value * in_mouse_x * modulatedsensitivity * cl.viewzoom;
			cl.cmd.forwardmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
		}
	}
	else // don't pitch drift when csqc is controlling the mouse
	{
		// mouse interacting with the scene, mostly stationary view
		V_StopPitchDrift();
		// update prydon cursor
		cl.cmd.cursor_screen[0] = in_windowmouse_x * 2.0 / vid.mode.width - 1.0;
		cl.cmd.cursor_screen[1] = in_windowmouse_y * 2.0 / vid.mode.height - 1.0;
	}

	if(v_flipped.integer)
	{
		cl.viewangles[YAW] = -cl.viewangles[YAW];
		cl.cmd.sidemove = -cl.cmd.sidemove;
	}

	// clamp after the move to prevent rendering with bad angles
	CL_AdjustAngles ();

	if(cl_movecliptokeyboard.integer)
	{
		vec_t f = 1;
		if (in_speed.state & 1)
			f *= cl_movespeedkey.value;
		if(cl_movecliptokeyboard.integer == 2)
		{
			// digital direction, analog amount
			vec_t wishvel_x, wishvel_y;
			wishvel_x = fabs(cl.cmd.forwardmove);
			wishvel_y = fabs(cl.cmd.sidemove);
			if(wishvel_x != 0 && wishvel_y != 0 && wishvel_x != wishvel_y)
			{
				vec_t wishspeed = sqrt(wishvel_x * wishvel_x + wishvel_y * wishvel_y);
				if(wishvel_x >= 2 * wishvel_y)
				{
					// pure X motion
					if(cl.cmd.forwardmove > 0)
						cl.cmd.forwardmove = wishspeed;
					else
						cl.cmd.forwardmove = -wishspeed;
					cl.cmd.sidemove = 0;
				}
				else if(wishvel_y >= 2 * wishvel_x)
				{
					// pure Y motion
					cl.cmd.forwardmove = 0;
					if(cl.cmd.sidemove > 0)
						cl.cmd.sidemove = wishspeed;
					else
						cl.cmd.sidemove = -wishspeed;
				}
				else
				{
					// diagonal
					if(cl.cmd.forwardmove > 0)
						cl.cmd.forwardmove = 0.70710678118654752440 * wishspeed;
					else
						cl.cmd.forwardmove = -0.70710678118654752440 * wishspeed;
					if(cl.cmd.sidemove > 0)
						cl.cmd.sidemove = 0.70710678118654752440 * wishspeed;
					else
						cl.cmd.sidemove = -0.70710678118654752440 * wishspeed;
				}
			}
		}
		else if(cl_movecliptokeyboard.integer)
		{
			// digital direction, digital amount
			if(cl.cmd.sidemove >= cl_sidespeed.value * f * 0.5)
				cl.cmd.sidemove = cl_sidespeed.value * f;
			else if(cl.cmd.sidemove <= -cl_sidespeed.value * f * 0.5)
				cl.cmd.sidemove = -cl_sidespeed.value * f;
			else
				cl.cmd.sidemove = 0;
			if(cl.cmd.forwardmove >= cl_forwardspeed.value * f * 0.5)
				cl.cmd.forwardmove = cl_forwardspeed.value * f;
			else if(cl.cmd.forwardmove <= -cl_backspeed.value * f * 0.5)
				cl.cmd.forwardmove = -cl_backspeed.value * f;
			else
				cl.cmd.forwardmove = 0;
		}
	}
}

#include "cl_collision.h"

static void CL_UpdatePrydonCursor(void)
{
	vec3_t temp;

	if (cl_prydoncursor.integer <= 0)
		VectorClear(cl.cmd.cursor_screen);

	/*
	if (cl.cmd.cursor_screen[0] < -1)
	{
		cl.viewangles[YAW] -= m_yaw.value * (cl.cmd.cursor_screen[0] - -1) * vid.width * sensitivity.value * cl.viewzoom;
		cl.cmd.cursor_screen[0] = -1;
	}
	if (cl.cmd.cursor_screen[0] > 1)
	{
		cl.viewangles[YAW] -= m_yaw.value * (cl.cmd.cursor_screen[0] - 1) * vid.width * sensitivity.value * cl.viewzoom;
		cl.cmd.cursor_screen[0] = 1;
	}
	if (cl.cmd.cursor_screen[1] < -1)
	{
		cl.viewangles[PITCH] += m_pitch.value * (cl.cmd.cursor_screen[1] - -1) * vid.height * sensitivity.value * cl.viewzoom;
		cl.cmd.cursor_screen[1] = -1;
	}
	if (cl.cmd.cursor_screen[1] > 1)
	{
		cl.viewangles[PITCH] += m_pitch.value * (cl.cmd.cursor_screen[1] - 1) * vid.height * sensitivity.value * cl.viewzoom;
		cl.cmd.cursor_screen[1] = 1;
	}
	*/
	cl.cmd.cursor_screen[0] = bound(-1, cl.cmd.cursor_screen[0], 1);
	cl.cmd.cursor_screen[1] = bound(-1, cl.cmd.cursor_screen[1], 1);
	cl.cmd.cursor_screen[2] = 1;

	// calculate current view matrix
	Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, cl.cmd.cursor_start);
	// calculate direction vector of cursor in viewspace by using frustum slopes
	VectorSet(temp, cl.cmd.cursor_screen[2] * 1000000, (v_flipped.integer ? -1 : 1) * cl.cmd.cursor_screen[0] * -r_refdef.view.frustum_x * 1000000, cl.cmd.cursor_screen[1] * -r_refdef.view.frustum_y * 1000000);
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, cl.cmd.cursor_end);
	// trace from view origin to the cursor
	if (cl_prydoncursor_notrace.integer)
	{
		cl.cmd.cursor_fraction = 1.0f;
		VectorCopy(cl.cmd.cursor_end, cl.cmd.cursor_impact);
		VectorClear(cl.cmd.cursor_normal);
		cl.cmd.cursor_entitynumber = 0;
	}
	else
		cl.cmd.cursor_fraction = CL_SelectTraceLine(cl.cmd.cursor_start, cl.cmd.cursor_end, cl.cmd.cursor_impact, cl.cmd.cursor_normal, &cl.cmd.cursor_entitynumber, (chase_active.integer || cl.intermission) ? &cl.entities[cl.playerentity].render : NULL);
}

#define NUMOFFSETS 27
static vec3_t offsets[NUMOFFSETS] =
{
// 1 no nudge (just return the original if this test passes)
	{ 0.000,  0.000,  0.000},
// 6 simple nudges
	{ 0.000,  0.000,  0.125}, { 0.000,  0.000, -0.125},
	{-0.125,  0.000,  0.000}, { 0.125,  0.000,  0.000},
	{ 0.000, -0.125,  0.000}, { 0.000,  0.125,  0.000},
// 4 diagonal flat nudges
	{-0.125, -0.125,  0.000}, { 0.125, -0.125,  0.000},
	{-0.125,  0.125,  0.000}, { 0.125,  0.125,  0.000},
// 8 diagonal upward nudges
	{-0.125,  0.000,  0.125}, { 0.125,  0.000,  0.125},
	{ 0.000, -0.125,  0.125}, { 0.000,  0.125,  0.125},
	{-0.125, -0.125,  0.125}, { 0.125, -0.125,  0.125},
	{-0.125,  0.125,  0.125}, { 0.125,  0.125,  0.125},
// 8 diagonal downward nudges
	{-0.125,  0.000, -0.125}, { 0.125,  0.000, -0.125},
	{ 0.000, -0.125, -0.125}, { 0.000,  0.125, -0.125},
	{-0.125, -0.125, -0.125}, { 0.125, -0.125, -0.125},
	{-0.125,  0.125, -0.125}, { 0.125,  0.125, -0.125},
};

static qbool CL_ClientMovement_Unstick(cl_clientmovement_state_t *s)
{
	int i;
	vec3_t neworigin;
	for (i = 0;i < NUMOFFSETS;i++)
	{
		VectorAdd(offsets[i], s->origin, neworigin);
		if (!CL_TraceBox(neworigin, cl.playercrouchmins, cl.playercrouchmaxs, neworigin, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true).startsolid)
		{
			VectorCopy(neworigin, s->origin);
			return true;
		}
	}
	// if all offsets failed, give up
	return false;
}

static void CL_ClientMovement_UpdateStatus(cl_clientmovement_state_t *s)
{
	vec_t f;
	vec3_t origin1, origin2;
	trace_t trace;

	// make sure player is not stuck
	CL_ClientMovement_Unstick(s);

	// set crouched
	if (s->cmd.crouch)
	{
		// wants to crouch, this always works..
		if (!s->crouched)
			s->crouched = true;
	}
	else
	{
		// wants to stand, if currently crouching we need to check for a
		// low ceiling first
		if (s->crouched)
		{
			trace = CL_TraceBox(s->origin, cl.playerstandmins, cl.playerstandmaxs, s->origin, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
			if (!trace.startsolid)
				s->crouched = false;
		}
	}
	if (s->crouched)
	{
		VectorCopy(cl.playercrouchmins, s->mins);
		VectorCopy(cl.playercrouchmaxs, s->maxs);
	}
	else
	{
		VectorCopy(cl.playerstandmins, s->mins);
		VectorCopy(cl.playerstandmaxs, s->maxs);
	}

	// set onground
	VectorSet(origin1, s->origin[0], s->origin[1], s->origin[2] + 1);
	VectorSet(origin2, s->origin[0], s->origin[1], s->origin[2] - 1); // -2 causes clientside doublejump bug at above 150fps, raising that to 300fps :)
	trace = CL_TraceBox(origin1, s->mins, s->maxs, origin2, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
	if(trace.fraction < 1 && trace.plane.normal[2] > 0.7)
	{
		s->onground = true;

		// this code actually "predicts" an impact; so let's clip velocity first
		f = DotProduct(s->velocity, trace.plane.normal);
		if(f < 0) // only if moving downwards actually
			VectorMA(s->velocity, -f, trace.plane.normal, s->velocity);
	}
	else
		s->onground = false;

	// set watertype/waterlevel
	VectorSet(origin1, s->origin[0], s->origin[1], s->origin[2] + s->mins[2] + 1);
	s->waterlevel = WATERLEVEL_NONE;
	s->watertype = CL_TracePoint(origin1, MOVE_NOMONSTERS, s->self, 0, 0, 0, true, false, NULL, false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK;
	if (s->watertype)
	{
		s->waterlevel = WATERLEVEL_WETFEET;
		origin1[2] = s->origin[2] + (s->mins[2] + s->maxs[2]) * 0.5f;
		if (CL_TracePoint(origin1, MOVE_NOMONSTERS, s->self, 0, 0, 0, true, false, NULL, false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK)
		{
			s->waterlevel = WATERLEVEL_SWIMMING;
			origin1[2] = s->origin[2] + 22;
			if (CL_TracePoint(origin1, MOVE_NOMONSTERS, s->self, 0, 0, 0, true, false, NULL, false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK)
				s->waterlevel = WATERLEVEL_SUBMERGED;
		}
	}

	// water jump prediction
	if (s->onground || s->velocity[2] <= 0 || s->waterjumptime <= 0)
		s->waterjumptime = 0;
}

static void CL_ClientMovement_Move(cl_clientmovement_state_t *s)
{
	int bump;
	double t;
	vec_t f;
	vec3_t neworigin;
	vec3_t currentorigin2;
	vec3_t neworigin2;
	vec3_t primalvelocity;
	trace_t trace;
	trace_t trace2;
	trace_t trace3;
	CL_ClientMovement_UpdateStatus(s);
	VectorCopy(s->velocity, primalvelocity);
	for (bump = 0, t = s->cmd.frametime;bump < 8 && VectorLength2(s->velocity) > 0;bump++)
	{
		VectorMA(s->origin, t, s->velocity, neworigin);
		trace = CL_TraceBox(s->origin, s->mins, s->maxs, neworigin, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
		if (trace.fraction < 1 && trace.plane.normal[2] == 0)
		{
			// may be a step or wall, try stepping up
			// first move forward at a higher level
			VectorSet(currentorigin2, s->origin[0], s->origin[1], s->origin[2] + cl.movevars_stepheight);
			VectorSet(neworigin2, neworigin[0], neworigin[1], s->origin[2] + cl.movevars_stepheight);
			trace2 = CL_TraceBox(currentorigin2, s->mins, s->maxs, neworigin2, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
			if (!trace2.startsolid)
			{
				// then move down from there
				VectorCopy(trace2.endpos, currentorigin2);
				VectorSet(neworigin2, trace2.endpos[0], trace2.endpos[1], s->origin[2]);
				trace3 = CL_TraceBox(currentorigin2, s->mins, s->maxs, neworigin2, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
				//Con_Printf("%f %f %f %f : %f %f %f %f : %f %f %f %f\n", trace.fraction, trace.endpos[0], trace.endpos[1], trace.endpos[2], trace2.fraction, trace2.endpos[0], trace2.endpos[1], trace2.endpos[2], trace3.fraction, trace3.endpos[0], trace3.endpos[1], trace3.endpos[2]);
				// accept the new trace if it made some progress
				if (fabs(trace3.endpos[0] - trace.endpos[0]) >= 0.03125 || fabs(trace3.endpos[1] - trace.endpos[1]) >= 0.03125)
				{
					trace = trace2;
					VectorCopy(trace3.endpos, trace.endpos);
				}
			}
		}

		// check if it moved at all
		if (trace.fraction >= 0.001)
			VectorCopy(trace.endpos, s->origin);

		// check if it moved all the way
		if (trace.fraction == 1)
			break;

		// this is only really needed for nogravityonground combined with gravityunaffectedbyticrate
		// <LadyHavoc> I'm pretty sure I commented it out solely because it seemed redundant
		// this got commented out in a change that supposedly makes the code match QW better
		// so if this is broken, maybe put it in an if(cls.protocol != PROTOCOL_QUAKEWORLD) block
		if (trace.plane.normal[2] > 0.7)
			s->onground = true;

		t -= t * trace.fraction;

		f = DotProduct(s->velocity, trace.plane.normal);
		VectorMA(s->velocity, -f, trace.plane.normal, s->velocity);
	}
	if (s->waterjumptime > 0)
		VectorCopy(primalvelocity, s->velocity);
}


static void CL_ClientMovement_Physics_Swim(cl_clientmovement_state_t *s)
{
	vec_t wishspeed;
	vec_t f;
	vec3_t wishvel;
	vec3_t wishdir;

	// water jump only in certain situations
	// this mimics quakeworld code
	if (s->cmd.jump && s->waterlevel == 2 && s->velocity[2] >= -180)
	{
		vec3_t forward;
		vec3_t yawangles;
		vec3_t spot;
		VectorSet(yawangles, 0, s->cmd.viewangles[1], 0);
		AngleVectors(yawangles, forward, NULL, NULL);
		VectorMA(s->origin, 24, forward, spot);
		spot[2] += 8;
		if (CL_TracePoint(spot, MOVE_NOMONSTERS, s->self, 0, 0, 0, true, false, NULL, false).startsolid)
		{
			spot[2] += 24;
			if (!CL_TracePoint(spot, MOVE_NOMONSTERS, s->self, 0, 0, 0, true, false, NULL, false).startsolid)
			{
				VectorScale(forward, 50, s->velocity);
				s->velocity[2] = 310;
				s->waterjumptime = 2;
				s->onground = false;
				s->cmd.canjump = false;
			}
		}
	}

	if (!(s->cmd.forwardmove*s->cmd.forwardmove + s->cmd.sidemove*s->cmd.sidemove + s->cmd.upmove*s->cmd.upmove))
	{
		// drift towards bottom
		VectorSet(wishvel, 0, 0, -60);
	}
	else
	{
		// swim
		vec3_t forward;
		vec3_t right;
		vec3_t up;
		// calculate movement vector
		AngleVectors(s->cmd.viewangles, forward, right, up);
		VectorSet(up, 0, 0, 1);
		VectorMAMAM(s->cmd.forwardmove, forward, s->cmd.sidemove, right, s->cmd.upmove, up, wishvel);
	}

	// split wishvel into wishspeed and wishdir
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);
	wishspeed = min(wishspeed, cl.movevars_maxspeed) * 0.7;

	if (s->crouched)
		wishspeed *= 0.5;

	if (s->waterjumptime <= 0)
	{
		// water friction
		f = 1 - s->cmd.frametime * cl.movevars_waterfriction * (cls.protocol == PROTOCOL_QUAKEWORLD ? s->waterlevel : 1);
		f = bound(0, f, 1);
		VectorScale(s->velocity, f, s->velocity);

		// water acceleration
		f = wishspeed - DotProduct(s->velocity, wishdir);
		if (f > 0)
		{
			f = min(cl.movevars_wateraccelerate * s->cmd.frametime * wishspeed, f);
			VectorMA(s->velocity, f, wishdir, s->velocity);
		}

		// holding jump button swims upward slowly
		if (s->cmd.jump)
		{
			if (s->watertype & SUPERCONTENTS_LAVA)
				s->velocity[2] =  50;
			else if (s->watertype & SUPERCONTENTS_SLIME)
				s->velocity[2] =  80;
			else
			{
				if (IS_NEXUIZ_DERIVED(gamemode))
					s->velocity[2] = 200;
				else
					s->velocity[2] = 100;
			}
		}
	}

	CL_ClientMovement_Move(s);
}

static vec_t CL_IsMoveInDirection(vec_t forward, vec_t side, vec_t angle)
{
	if(forward == 0 && side == 0)
		return 0; // avoid division by zero
	angle -= RAD2DEG(atan2(side, forward));
	angle = (ANGLEMOD(angle + 180) - 180) / 45;
	if(angle >  1)
		return 0;
	if(angle < -1)
		return 0;
	return 1 - fabs(angle);
}

static vec_t CL_GeomLerp(vec_t a, vec_t lerp, vec_t b)
{
	if(a == 0)
	{
		if(lerp < 1)
			return 0;
		else
			return b;
	}
	if(b == 0)
	{
		if(lerp > 0)
			return 0;
		else
			return a;
	}
	return a * pow(fabs(b / a), lerp);
}

static void CL_ClientMovement_Physics_CPM_PM_Aircontrol(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed)
{
	vec_t zspeed, speed, dot, k;

#if 0
	// this doesn't play well with analog input
	if(s->cmd.forwardmove == 0 || s->cmd.sidemove != 0)
		return;
	k = 32;
#else
	k = 32 * (2 * CL_IsMoveInDirection(s->cmd.forwardmove, s->cmd.sidemove, 0) - 1);
	if(k <= 0)
		return;
#endif

	k *= bound(0, wishspeed / cl.movevars_maxairspeed, 1);

	zspeed = s->velocity[2];
	s->velocity[2] = 0;
	speed = VectorNormalizeLength(s->velocity);

	dot = DotProduct(s->velocity, wishdir);

	if(dot > 0) { // we can't change direction while slowing down
		k *= pow(dot, cl.movevars_aircontrol_power)*s->cmd.frametime;
		speed = max(0, speed - cl.movevars_aircontrol_penalty * sqrt(max(0, 1 - dot*dot)) * k/32);
		k *= cl.movevars_aircontrol;
		VectorMAM(speed, s->velocity, k, wishdir, s->velocity);
		VectorNormalize(s->velocity);
	}

	VectorScale(s->velocity, speed, s->velocity);
	s->velocity[2] = zspeed;
}

static float CL_ClientMovement_Physics_AdjustAirAccelQW(float accelqw, float factor)
{
	return
		(accelqw < 0 ? -1 : +1)
		*
		bound(0.000001, 1 - (1 - fabs(accelqw)) * factor, 1);
}

static void CL_ClientMovement_Physics_PM_Accelerate(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed, vec_t wishspeed0, vec_t accel, vec_t accelqw, vec_t stretchfactor, vec_t sidefric, vec_t speedlimit)
{
	vec_t vel_straight;
	vec_t vel_z;
	vec3_t vel_perpend;
	vec_t step;
	vec3_t vel_xy;
	vec_t vel_xy_current;
	vec_t vel_xy_backward, vel_xy_forward;
	vec_t speedclamp;

	if(stretchfactor > 0)
		speedclamp = stretchfactor;
	else if(accelqw < 0)
		speedclamp = 1;
	else
		speedclamp = -1; // no clamping

	if(accelqw < 0)
		accelqw = -accelqw;

	if(cl.moveflags & MOVEFLAG_Q2AIRACCELERATE)
		wishspeed0 = wishspeed; // don't need to emulate this Q1 bug

	vel_straight = DotProduct(s->velocity, wishdir);
	vel_z = s->velocity[2];
	VectorCopy(s->velocity, vel_xy); vel_xy[2] -= vel_z;
	VectorMA(vel_xy, -vel_straight, wishdir, vel_perpend);

	step = accel * s->cmd.frametime * wishspeed0;

	vel_xy_current  = VectorLength(vel_xy);
	if(speedlimit > 0)
		accelqw = CL_ClientMovement_Physics_AdjustAirAccelQW(accelqw, (speedlimit - bound(wishspeed, vel_xy_current, speedlimit)) / max(1, speedlimit - wishspeed));
	vel_xy_forward  = vel_xy_current + bound(0, wishspeed - vel_xy_current, step) * accelqw + step * (1 - accelqw);
	vel_xy_backward = vel_xy_current - bound(0, wishspeed + vel_xy_current, step) * accelqw - step * (1 - accelqw);
	if(vel_xy_backward < 0)
		vel_xy_backward = 0; // not that it REALLY occurs that this would cause wrong behaviour afterwards

	vel_straight    = vel_straight   + bound(0, wishspeed - vel_straight,   step) * accelqw + step * (1 - accelqw);

	if(sidefric < 0 && VectorLength2(vel_perpend))
		// negative: only apply so much sideways friction to stay below the speed you could get by "braking"
	{
		vec_t f, fmin;
		f = max(0, 1 + s->cmd.frametime * wishspeed * sidefric);
		fmin = (vel_xy_backward*vel_xy_backward - vel_straight*vel_straight) / VectorLength2(vel_perpend);
		// assume: fmin > 1
		// vel_xy_backward*vel_xy_backward - vel_straight*vel_straight > vel_perpend*vel_perpend
		// vel_xy_backward*vel_xy_backward > vel_straight*vel_straight + vel_perpend*vel_perpend
		// vel_xy_backward*vel_xy_backward > vel_xy * vel_xy
		// obviously, this cannot be
		if(fmin <= 0)
			VectorScale(vel_perpend, f, vel_perpend);
		else
		{
			fmin = sqrt(fmin);
			VectorScale(vel_perpend, max(fmin, f), vel_perpend);
		}
	}
	else
		VectorScale(vel_perpend, max(0, 1 - s->cmd.frametime * wishspeed * sidefric), vel_perpend);

	VectorMA(vel_perpend, vel_straight, wishdir, s->velocity);

	if(speedclamp >= 0)
	{
		vec_t vel_xy_preclamp;
		vel_xy_preclamp = VectorLength(s->velocity);
		if(vel_xy_preclamp > 0) // prevent division by zero
		{
			vel_xy_current += (vel_xy_forward - vel_xy_current) * speedclamp;
			if(vel_xy_current < vel_xy_preclamp)
				VectorScale(s->velocity, (vel_xy_current / vel_xy_preclamp), s->velocity);
		}
	}

	s->velocity[2] += vel_z;
}

static void CL_ClientMovement_Physics_PM_AirAccelerate(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed)
{
    vec3_t curvel, wishvel, acceldir, curdir;
    float addspeed, accelspeed, curspeed;
    float dot;

    float airforwardaccel = cl.movevars_warsowbunny_airforwardaccel;
    float bunnyaccel = cl.movevars_warsowbunny_accel;
    float bunnytopspeed = cl.movevars_warsowbunny_topspeed;
    float turnaccel = cl.movevars_warsowbunny_turnaccel;
    float backtosideratio = cl.movevars_warsowbunny_backtosideratio;

    if( !wishspeed )
        return;

    VectorCopy( s->velocity, curvel );
    curvel[2] = 0;
    curspeed = VectorLength( curvel );

    if( wishspeed > curspeed * 1.01f )
    {
        float faccelspeed = curspeed + airforwardaccel * cl.movevars_maxairspeed * s->cmd.frametime;
        if( faccelspeed < wishspeed )
            wishspeed = faccelspeed;
    }
    else
    {
        float f = ( bunnytopspeed - curspeed ) / ( bunnytopspeed - cl.movevars_maxairspeed );
        if( f < 0 )
            f = 0;
        wishspeed = max( curspeed, cl.movevars_maxairspeed ) + bunnyaccel * f * cl.movevars_maxairspeed * s->cmd.frametime;
    }
    VectorScale( wishdir, wishspeed, wishvel );
    VectorSubtract( wishvel, curvel, acceldir );
    addspeed = VectorNormalizeLength( acceldir );

    accelspeed = turnaccel * cl.movevars_maxairspeed /* wishspeed */ * s->cmd.frametime;
    if( accelspeed > addspeed )
        accelspeed = addspeed;

    if( backtosideratio < 1.0f )
    {
        VectorNormalize2( curvel, curdir );
        dot = DotProduct( acceldir, curdir );
        if( dot < 0 )
            VectorMA( acceldir, -( 1.0f - backtosideratio ) * dot, curdir, acceldir );
    }

    VectorMA( s->velocity, accelspeed, acceldir, s->velocity );
}

static void CL_ClientMovement_Physics_CheckJump(cl_clientmovement_state_t *s)
{
	// jump if on ground with jump button pressed but only if it has been
	// released at least once since the last jump
	if (s->cmd.jump)
	{
		if (s->onground && (s->cmd.canjump || !cl_movement_track_canjump.integer))
		{
			s->velocity[2] += cl.movevars_jumpvelocity;
			s->onground = false;
			s->cmd.canjump = false;
		}
	}
	else
		s->cmd.canjump = true;
}

static void CL_ClientMovement_Physics_Walk(cl_clientmovement_state_t *s)
{
	vec_t friction;
	vec_t wishspeed;
	vec_t addspeed;
	vec_t accelspeed;
	vec_t speed;
	vec_t gravity;
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	vec3_t wishvel;
	vec3_t wishdir;
	vec3_t yawangles;
	trace_t trace;

	CL_ClientMovement_Physics_CheckJump(s);

	// calculate movement vector
	VectorSet(yawangles, 0, s->cmd.viewangles[1], 0);
	AngleVectors(yawangles, forward, right, up);
	VectorMAM(s->cmd.forwardmove, forward, s->cmd.sidemove, right, wishvel);

	// split wishvel into wishspeed and wishdir
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);

	// check if onground
	if (s->onground)
	{
		wishspeed = min(wishspeed, cl.movevars_maxspeed);
		if (s->crouched)
			wishspeed *= 0.5;

		// apply edge friction
		speed = VectorLength2(s->velocity);
		if (speed > 0)
		{
			friction = cl.movevars_friction;
			if (cl.movevars_edgefriction != 1)
			{
				vec3_t neworigin2;
				vec3_t neworigin3;
				// note: QW uses the full player box for the trace, and yet still
				// uses s->origin[2] + s->mins[2], which is clearly an bug, but
				// this mimics it for compatibility
				VectorSet(neworigin2, s->origin[0] + s->velocity[0]*(16/speed), s->origin[1] + s->velocity[1]*(16/speed), s->origin[2] + s->mins[2]);
				VectorSet(neworigin3, neworigin2[0], neworigin2[1], neworigin2[2] - 34);
				if (cls.protocol == PROTOCOL_QUAKEWORLD)
					trace = CL_TraceBox(neworigin2, s->mins, s->maxs, neworigin3, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true);
				else
					trace = CL_TraceLine(neworigin2, neworigin3, MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true, true, NULL, true, false);
				if (trace.fraction == 1 && !trace.startsolid)
					friction *= cl.movevars_edgefriction;
			}
			// apply ground friction
			speed = 1 - s->cmd.frametime * friction * ((speed < cl.movevars_stopspeed) ? (cl.movevars_stopspeed / speed) : 1);
			speed = max(speed, 0);
			VectorScale(s->velocity, speed, s->velocity);
		}
		addspeed = wishspeed - DotProduct(s->velocity, wishdir);
		if (addspeed > 0)
		{
			accelspeed = min(cl.movevars_accelerate * s->cmd.frametime * wishspeed, addspeed);
			VectorMA(s->velocity, accelspeed, wishdir, s->velocity);
		}
		gravity = cl.movevars_gravity * cl.movevars_entgravity * s->cmd.frametime;
		if(!(cl.moveflags & MOVEFLAG_NOGRAVITYONGROUND))
		{
			if(cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
				s->velocity[2] -= gravity * 0.5f;
			else
				s->velocity[2] -= gravity;
		}
		if (cls.protocol == PROTOCOL_QUAKEWORLD)
			s->velocity[2] = 0;
		if (VectorLength2(s->velocity))
			CL_ClientMovement_Move(s);
		if(!(cl.moveflags & MOVEFLAG_NOGRAVITYONGROUND) || !s->onground)
		{
			if(cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
				s->velocity[2] -= gravity * 0.5f;
		}
	}
	else
	{
		if (s->waterjumptime <= 0)
		{
			// apply air speed limit
			vec_t accel, wishspeed0, wishspeed2, accelqw, strafity;
			qbool accelerating;

			accelqw = cl.movevars_airaccel_qw;
			wishspeed0 = wishspeed;
			wishspeed = min(wishspeed, cl.movevars_maxairspeed);
			if (s->crouched)
				wishspeed *= 0.5;
			accel = cl.movevars_airaccelerate;

			accelerating = (DotProduct(s->velocity, wishdir) > 0);
			wishspeed2 = wishspeed;

			// CPM: air control
			if(cl.movevars_airstopaccelerate != 0)
			{
				vec3_t curdir;
				curdir[0] = s->velocity[0];
				curdir[1] = s->velocity[1];
				curdir[2] = 0;
				VectorNormalize(curdir);
				accel = accel + (cl.movevars_airstopaccelerate - accel) * max(0, -DotProduct(curdir, wishdir));
			}
			strafity = CL_IsMoveInDirection(s->cmd.forwardmove, s->cmd.sidemove, -90) + CL_IsMoveInDirection(s->cmd.forwardmove, s->cmd.sidemove, +90); // if one is nonzero, other is always zero
			if(cl.movevars_maxairstrafespeed)
				wishspeed = min(wishspeed, CL_GeomLerp(cl.movevars_maxairspeed, strafity, cl.movevars_maxairstrafespeed));
			if(cl.movevars_airstrafeaccelerate)
				accel = CL_GeomLerp(cl.movevars_airaccelerate, strafity, cl.movevars_airstrafeaccelerate);
			if(cl.movevars_airstrafeaccel_qw)
				accelqw =
					(((strafity > 0.5 ? cl.movevars_airstrafeaccel_qw : cl.movevars_airaccel_qw) >= 0) ? +1 : -1)
					*
					(1 - CL_GeomLerp(1 - fabs(cl.movevars_airaccel_qw), strafity, 1 - fabs(cl.movevars_airstrafeaccel_qw)));
			// !CPM

			if(cl.movevars_warsowbunny_turnaccel && accelerating && s->cmd.sidemove == 0 && s->cmd.forwardmove != 0)
				CL_ClientMovement_Physics_PM_AirAccelerate(s, wishdir, wishspeed2);
			else
				CL_ClientMovement_Physics_PM_Accelerate(s, wishdir, wishspeed, wishspeed0, accel, accelqw, cl.movevars_airaccel_qw_stretchfactor, cl.movevars_airaccel_sideways_friction / cl.movevars_maxairspeed, cl.movevars_airspeedlimit_nonqw);

			if(cl.movevars_aircontrol)
				CL_ClientMovement_Physics_CPM_PM_Aircontrol(s, wishdir, wishspeed2);
		}
		gravity = cl.movevars_gravity * cl.movevars_entgravity * s->cmd.frametime;
		if(cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
			s->velocity[2] -= gravity * 0.5f;
		else
			s->velocity[2] -= gravity;
		CL_ClientMovement_Move(s);
		if(!(cl.moveflags & MOVEFLAG_NOGRAVITYONGROUND) || !s->onground)
		{
			if(cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
				s->velocity[2] -= gravity * 0.5f;
		}
	}
}

static void CL_ClientMovement_PlayerMove(cl_clientmovement_state_t *s)
{
	//Con_Printf(" %f", frametime);
	if (!s->cmd.jump)
		s->cmd.canjump = true;
	s->waterjumptime -= s->cmd.frametime;
	CL_ClientMovement_UpdateStatus(s);
	if (s->waterlevel >= WATERLEVEL_SWIMMING)
		CL_ClientMovement_Physics_Swim(s);
	else
		CL_ClientMovement_Physics_Walk(s);
}

extern cvar_t host_timescale;
void CL_UpdateMoveVars(void)
{
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
	{
		cl.moveflags = 0;
	}
	else if (cl.stats[STAT_MOVEVARS_TICRATE])
	{
		cl.moveflags = cl.stats[STAT_MOVEFLAGS];
		cl.movevars_ticrate = cl.statsf[STAT_MOVEVARS_TICRATE];
		cl.movevars_timescale = cl.statsf[STAT_MOVEVARS_TIMESCALE];
		cl.movevars_gravity = cl.statsf[STAT_MOVEVARS_GRAVITY];
		cl.movevars_stopspeed = cl.statsf[STAT_MOVEVARS_STOPSPEED] ;
		cl.movevars_maxspeed = cl.statsf[STAT_MOVEVARS_MAXSPEED];
		cl.movevars_spectatormaxspeed = cl.statsf[STAT_MOVEVARS_SPECTATORMAXSPEED];
		cl.movevars_accelerate = cl.statsf[STAT_MOVEVARS_ACCELERATE];
		cl.movevars_airaccelerate = cl.statsf[STAT_MOVEVARS_AIRACCELERATE];
		cl.movevars_wateraccelerate = cl.statsf[STAT_MOVEVARS_WATERACCELERATE];
		cl.movevars_entgravity = cl.statsf[STAT_MOVEVARS_ENTGRAVITY];
		cl.movevars_jumpvelocity = cl.statsf[STAT_MOVEVARS_JUMPVELOCITY];
		cl.movevars_edgefriction = cl.statsf[STAT_MOVEVARS_EDGEFRICTION];
		cl.movevars_maxairspeed = cl.statsf[STAT_MOVEVARS_MAXAIRSPEED];
		cl.movevars_stepheight = cl.statsf[STAT_MOVEVARS_STEPHEIGHT];
		cl.movevars_airaccel_qw = cl.statsf[STAT_MOVEVARS_AIRACCEL_QW];
		cl.movevars_airaccel_qw_stretchfactor = cl.statsf[STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR];
		cl.movevars_airaccel_sideways_friction = cl.statsf[STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION];
		cl.movevars_friction = cl.statsf[STAT_MOVEVARS_FRICTION];
		cl.movevars_wallfriction = cl.statsf[STAT_MOVEVARS_WALLFRICTION];
		cl.movevars_waterfriction = cl.statsf[STAT_MOVEVARS_WATERFRICTION];
		cl.movevars_airstopaccelerate = cl.statsf[STAT_MOVEVARS_AIRSTOPACCELERATE];
		cl.movevars_airstrafeaccelerate = cl.statsf[STAT_MOVEVARS_AIRSTRAFEACCELERATE];
		cl.movevars_maxairstrafespeed = cl.statsf[STAT_MOVEVARS_MAXAIRSTRAFESPEED];
		cl.movevars_airstrafeaccel_qw = cl.statsf[STAT_MOVEVARS_AIRSTRAFEACCEL_QW];
		cl.movevars_aircontrol = cl.statsf[STAT_MOVEVARS_AIRCONTROL];
		cl.movevars_aircontrol_power = cl.statsf[STAT_MOVEVARS_AIRCONTROL_POWER];
		cl.movevars_aircontrol_penalty = cl.statsf[STAT_MOVEVARS_AIRCONTROL_PENALTY];
		cl.movevars_warsowbunny_airforwardaccel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL];
		cl.movevars_warsowbunny_accel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_ACCEL];
		cl.movevars_warsowbunny_topspeed = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED];
		cl.movevars_warsowbunny_turnaccel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL];
		cl.movevars_warsowbunny_backtosideratio = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO];
		cl.movevars_airspeedlimit_nonqw = cl.statsf[STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW];
	}
	else
	{
		cl.moveflags = 0;
		cl.movevars_ticrate = 0; // bones_was_here: no guessing, unavailable ticrate triggers better fallbacks
		cl.movevars_timescale = (cls.demoplayback || host_timescale.value <= 0) ? 1.0f : host_timescale.value;
		cl.movevars_gravity = sv_gravity.value;
		cl.movevars_stopspeed = cl_movement_stopspeed.value;
		cl.movevars_maxspeed = cl_movement_maxspeed.value;
		cl.movevars_spectatormaxspeed = cl_movement_maxspeed.value;
		cl.movevars_accelerate = cl_movement_accelerate.value;
		cl.movevars_airaccelerate = cl_movement_airaccelerate.value < 0 ? cl_movement_accelerate.value : cl_movement_airaccelerate.value;
		cl.movevars_wateraccelerate = cl_movement_wateraccelerate.value < 0 ? cl_movement_accelerate.value : cl_movement_wateraccelerate.value;
		cl.movevars_friction = cl_movement_friction.value;
		cl.movevars_wallfriction = cl_movement_wallfriction.value;
		cl.movevars_waterfriction = cl_movement_waterfriction.value < 0 ? cl_movement_friction.value : cl_movement_waterfriction.value;
		cl.movevars_entgravity = 1;
		cl.movevars_jumpvelocity = cl_movement_jumpvelocity.value;
		cl.movevars_edgefriction = cl_movement_edgefriction.value;
		cl.movevars_maxairspeed = cl_movement_maxairspeed.value;
		cl.movevars_stepheight = cl_movement_stepheight.value;
		cl.movevars_airaccel_qw = cl_movement_airaccel_qw.value;
		cl.movevars_airaccel_qw_stretchfactor = 0;
		cl.movevars_airaccel_sideways_friction = cl_movement_airaccel_sideways_friction.value;
		cl.movevars_airstopaccelerate = 0;
		cl.movevars_airstrafeaccelerate = 0;
		cl.movevars_maxairstrafespeed = 0;
		cl.movevars_airstrafeaccel_qw = 0;
		cl.movevars_aircontrol = 0;
		cl.movevars_aircontrol_power = 2;
		cl.movevars_aircontrol_penalty = 0;
		cl.movevars_warsowbunny_airforwardaccel = 0;
		cl.movevars_warsowbunny_accel = 0;
		cl.movevars_warsowbunny_topspeed = 0;
		cl.movevars_warsowbunny_turnaccel = 0;
		cl.movevars_warsowbunny_backtosideratio = 0;
		cl.movevars_airspeedlimit_nonqw = 0;
	}

	if(!(cl.moveflags & MOVEFLAG_VALID))
	{
		if(gamemode == GAME_NEXUIZ)  // Legacy hack to work with old servers of Nexuiz.
			cl.moveflags = MOVEFLAG_Q2AIRACCELERATE;
	}

	if(cl.movevars_aircontrol_power <= 0)
		cl.movevars_aircontrol_power = 2; // CPMA default
}

void CL_ClientMovement_PlayerMove_Frame(cl_clientmovement_state_t *s)
{
	// if a move is more than 50ms, do it as two moves (matching qwsv)
	//Con_Printf("%i ", s.cmd.msec);
	if(s->cmd.frametime > 0.0005)
	{
		if (s->cmd.frametime > 0.05)
		{
			s->cmd.frametime /= 2;
			CL_ClientMovement_PlayerMove(s);
		}
		CL_ClientMovement_PlayerMove(s);
	}
	else
	{
		// we REALLY need this handling to happen, even if the move is not executed
		if (!s->cmd.jump)
			s->cmd.canjump = true;
	}
}

void CL_ClientMovement_Replay(void)
{
	int i;
	double totalmovemsec;
	cl_clientmovement_state_t s;

	VectorCopy(cl.mvelocity[0], cl.movement_velocity);

	if (cl.movement_predicted && !cl.movement_replay)
		return;

	if (!cl_movement_replay.integer)
		return;

	// set up starting state for the series of moves
	memset(&s, 0, sizeof(s));
	VectorCopy(cl.entities[cl.playerentity].state_current.origin, s.origin);
	VectorCopy(cl.mvelocity[0], s.velocity);
	s.crouched = true; // will be updated on first move
	//Con_Printf("movement replay starting org %f %f %f vel %f %f %f\n", s.origin[0], s.origin[1], s.origin[2], s.velocity[0], s.velocity[1], s.velocity[2]);

	totalmovemsec = 0;
	for (i = 0;i < CL_MAX_USERCMDS;i++)
		if (cl.movecmd[i].sequence > cls.servermovesequence)
			totalmovemsec += cl.movecmd[i].msec;
	cl.movement_predicted = totalmovemsec >= cl_movement_minping.value && cls.servermovesequence && (cl_movement.integer && !cls.demoplayback && cls.signon == SIGNONS && cl.stats[STAT_HEALTH] > 0 && !cl.intermission);
	//Con_Printf("%i = %.0f >= %.0f && %u && (%i && %i && %i == %i && %i > 0 && %i\n", cl.movement_predicted, totalmovemsec, cl_movement_minping.value, cls.servermovesequence, cl_movement.integer, !cls.demoplayback, cls.signon, SIGNONS, cl.stats[STAT_HEALTH], !cl.intermission);
	if (cl.movement_predicted)
	{
		//Con_Printf("%ims\n", cl.movecmd[0].msec);

		// replay the input queue to predict current location
		// note: this relies on the fact there's always one queue item at the end

		// find how many are still valid
		for (i = 0;i < CL_MAX_USERCMDS;i++)
			if (cl.movecmd[i].sequence <= cls.servermovesequence)
				break;
		// now walk them in oldest to newest order
		for (i--;i >= 0;i--)
		{
			s.cmd = cl.movecmd[i];
			if (i < CL_MAX_USERCMDS - 1)
				s.cmd.canjump = cl.movecmd[i+1].canjump;

			CL_ClientMovement_PlayerMove_Frame(&s);

			cl.movecmd[i].canjump = s.cmd.canjump;
		}
		//Con_Printf("\n");
		CL_ClientMovement_UpdateStatus(&s);
	}
	else
	{
		// get the first movement queue entry to know whether to crouch and such
		s.cmd = cl.movecmd[0];
	}

	if (!cls.demoplayback) // for bob, speedometer
	{
		cl.movement_replay = false;
		// update the interpolation target position and velocity
		VectorCopy(s.origin, cl.movement_origin);
		VectorCopy(s.velocity, cl.movement_velocity);
	}

	// update the onground flag if appropriate
	if (cl.movement_predicted)
	{
		// when predicted we simply set the flag according to the UpdateStatus
		cl.onground = s.onground;
	}
	else
	{
		// when not predicted, cl.onground is cleared by cl_parse.c each time
		// an update packet is received, but can be forced on here to hide
		// server inconsistencies in the onground flag
		// (which mostly occur when stepping up stairs at very high framerates
		//  where after the step up the move continues forward and not
		//  downward so the ground is not detected)
		//
		// such onground inconsistencies can cause jittery gun bobbing and
		// stair smoothing, so we set onground if UpdateStatus says so
		if (s.onground)
			cl.onground = true;
	}
}

static void QW_MSG_WriteDeltaUsercmd(sizebuf_t *buf, usercmd_t *from, usercmd_t *to)
{
	int bits;

	bits = 0;
	if (to->viewangles[0] != from->viewangles[0])
		bits |= QW_CM_ANGLE1;
	if (to->viewangles[1] != from->viewangles[1])
		bits |= QW_CM_ANGLE2;
	if (to->viewangles[2] != from->viewangles[2])
		bits |= QW_CM_ANGLE3;
	if (to->forwardmove != from->forwardmove)
		bits |= QW_CM_FORWARD;
	if (to->sidemove != from->sidemove)
		bits |= QW_CM_SIDE;
	if (to->upmove != from->upmove)
		bits |= QW_CM_UP;
	if (to->buttons != from->buttons)
		bits |= QW_CM_BUTTONS;
	if (to->impulse != from->impulse)
		bits |= QW_CM_IMPULSE;

	MSG_WriteByte(buf, bits);
	if (bits & QW_CM_ANGLE1)
		MSG_WriteAngle16i(buf, to->viewangles[0]);
	if (bits & QW_CM_ANGLE2)
		MSG_WriteAngle16i(buf, to->viewangles[1]);
	if (bits & QW_CM_ANGLE3)
		MSG_WriteAngle16i(buf, to->viewangles[2]);
	if (bits & QW_CM_FORWARD)
		MSG_WriteShort(buf, (short) to->forwardmove);
	if (bits & QW_CM_SIDE)
		MSG_WriteShort(buf, (short) to->sidemove);
	if (bits & QW_CM_UP)
		MSG_WriteShort(buf, (short) to->upmove);
	if (bits & QW_CM_BUTTONS)
		MSG_WriteByte(buf, to->buttons);
	if (bits & QW_CM_IMPULSE)
		MSG_WriteByte(buf, to->impulse);
	MSG_WriteByte(buf, to->msec);
}

void CL_NewFrameReceived(int num)
{
	if (developer_networkentities.integer >= 10)
		Con_Printf("recv: svc_entities %i\n", num);
	cl.latestframenums[cl.latestframenumsposition] = num;
	cl.latestsendnums[cl.latestframenumsposition] = cl.cmd.sequence;
	cl.latestframenumsposition = (cl.latestframenumsposition + 1) % LATESTFRAMENUMS;
}

void CL_RotateMoves(const matrix4x4_t *m)
{
	// rotate viewangles in all previous moves
	vec3_t v;
	vec3_t f, r, u;
	int i;
	for (i = 0;i < CL_MAX_USERCMDS;i++)
	{
		if (cl.movecmd[i].sequence > cls.servermovesequence)
		{
			usercmd_t *c = &cl.movecmd[i];
			AngleVectors(c->viewangles, f, r, u);
			Matrix4x4_Transform(m, f, v); VectorCopy(v, f);
			Matrix4x4_Transform(m, u, v); VectorCopy(v, u);
			AnglesFromVectors(c->viewangles, f, u, false);
		}
	}
}

/*
==============
CL_SendMove
==============
*/
usercmd_t nullcmd; // for delta compression of qw moves
void CL_SendMove(void)
{
	int i, j, packetloss;
	int checksumindex;
	int bits;
	int maxusercmds;
	usercmd_t *cmd;
	sizebuf_t buf;
	unsigned char data[1024];
	float packettime, sv_frametime, lag, frames_per_tic;
	qbool opportune_moment;
	qbool quemove;
	qbool important;

	// if playing a demo, do nothing
	if (!cls.netcon)
		return;

	// we don't que moves during a lag spike (potential network timeout)
	quemove = host.realtime - cl.last_received_message < cl_movement_nettimeout.value;

	// we build up cl.cmd and then decide whether to send or not
	// we store this into cl.movecmd[0] for prediction each frame even if we
	// do not send, to make sure that prediction is instant
	cl.cmd.time = cl.time;
	cl.cmd.sequence = cls.netcon->outgoing_unreliable_sequence;

	// set button bits
	// LadyHavoc: added 6 new buttons and use and chat buttons, and prydon cursor active button
	bits = 0;
	if (in_attack.state   & 3) bits |=   1;
	if (in_jump.state     & 3) bits |=   2;
	if (in_button3.state  & 3) bits |=   4;
	if (in_button4.state  & 3) bits |=   8;
	if (in_button5.state  & 3) bits |=  16;
	if (in_button6.state  & 3) bits |=  32;
	if (in_button7.state  & 3) bits |=  64;
	if (in_button8.state  & 3) bits |= 128;
	if (in_use.state      & 3) bits |= 256;
	if (key_dest != key_game || key_consoleactive || !vid_activewindow) bits |= 512;
	if (cl_prydoncursor.integer > 0) bits |= 1024;
	if (in_button9.state  & 3)  bits |=   2048;
	if (in_button10.state  & 3) bits |=   4096;
	if (in_button11.state  & 3) bits |=   8192;
	if (in_button12.state  & 3) bits |=  16384;
	if (in_button13.state  & 3) bits |=  32768;
	if (in_button14.state  & 3) bits |=  65536;
	if (in_button15.state  & 3) bits |= 131072;
	if (in_button16.state  & 3) bits |= 262144;
	// button bits 19-31 unused currently
	// rotate/zoom view serverside if PRYDON_CLIENTCURSOR cursor is at edge of screen
	if(cl_prydoncursor.integer > 0)
	{
		if (cl.cmd.cursor_screen[0] <= -1) bits |= 8;
		if (cl.cmd.cursor_screen[0] >=  1) bits |= 16;
		if (cl.cmd.cursor_screen[1] <= -1) bits |= 32;
		if (cl.cmd.cursor_screen[1] >=  1) bits |= 64;
	}

	// set buttons and impulse
	cl.cmd.buttons = bits;
	cl.cmd.impulse = in_impulse;

	// set viewangles
	VectorCopy(cl.viewangles, cl.cmd.viewangles);

	// bones_was_here: previously cl.cmd.frametime was floored to nearest millisec
	// this meant the smoothest async movement required integer millisec
	// client and server frame times (eg 125fps)
	cl.cmd.frametime = bound(0.0, cl.cmd.time - cl.movecmd[1].time, 0.255);
	// ridiculous value rejection (matches qw)
	if (cl.cmd.frametime > 0.25)
		cl.cmd.frametime = 0.1;
	cl.cmd.msec = (unsigned char)floor(cl.cmd.frametime * 1000);

	switch(cls.protocol)
	{
	case PROTOCOL_QUAKEWORLD:
		// quakeworld uses a different cvar with opposite meaning, for compatibility
		cl.cmd.predicted = cl_nopred.integer == 0;
		break;
	case PROTOCOL_DARKPLACES6:
	case PROTOCOL_DARKPLACES7:
	case PROTOCOL_DARKPLACES8:
		cl.cmd.predicted = cl_movement.integer != 0;
		break;
	default:
		cl.cmd.predicted = false;
		break;
	}

	// movement is set by input code (forwardmove/sidemove/upmove)
	// always dump the first two moves, because they may contain leftover inputs from the last level
	if (cl.cmd.sequence <= 2)
		cl.cmd.forwardmove = cl.cmd.sidemove = cl.cmd.upmove = cl.cmd.impulse = cl.cmd.buttons = 0;

	cl.cmd.jump = (cl.cmd.buttons & 2) != 0;
	cl.cmd.crouch = 0;
	switch (cls.protocol)
	{
	case PROTOCOL_QUAKEWORLD:
	case PROTOCOL_QUAKE:
	case PROTOCOL_QUAKEDP:
	case PROTOCOL_NEHAHRAMOVIE:
	case PROTOCOL_NEHAHRABJP:
	case PROTOCOL_NEHAHRABJP2:
	case PROTOCOL_NEHAHRABJP3:
	case PROTOCOL_DARKPLACES1:
	case PROTOCOL_DARKPLACES2:
	case PROTOCOL_DARKPLACES3:
	case PROTOCOL_DARKPLACES4:
	case PROTOCOL_DARKPLACES5:
		break;
	case PROTOCOL_DARKPLACES6:
	case PROTOCOL_DARKPLACES7:
	case PROTOCOL_DARKPLACES8:
		// FIXME: cl.cmd.buttons & 16 is +button5, Nexuiz/Xonotic specific
		cl.cmd.crouch = (cl.cmd.buttons & 16) != 0;
		break;
	case PROTOCOL_UNKNOWN:
		break;
	}

	if (quemove)
		cl.movecmd[0] = cl.cmd;

	/* Accumulating cl.realframetime to prevent low packet rates,
	 * previously with cl_maxfps == cl_netfps it did not send every frame because
	 * host.realtime - cl.lastpackettime was often well below (or above) cl_packetinterval.
	 */
	cl.timesincepacket += cl.realframetime;

	// don't predict more than 256fps
	if (cl.timesincepacket >= 1/256)
		cl.movement_replay = true; // redo the prediction

	// now decide whether to actually send this move
	// (otherwise it is only for prediction)

	// do not send 0ms packets because they mess up physics
	if(cl.cmd.msec == 0 && cl.time > cl.oldtime && (cls.protocol == PROTOCOL_QUAKEWORLD || cls.signon == SIGNONS))
		return;

	// don't send too often or else network connections can get clogged by a
	// high renderer framerate
	packettime = 1.0f / bound(10.0f, cl_netfps.value, 1000.0f);
	if (cl.movevars_ticrate)
	{
		packettime = bound(cl.movevars_ticrate * 0.5f, packettime, cl.movevars_ticrate);
		sv_frametime = cl.movevars_ticrate;
	}
	else // fallback, may be affected by server->client network problems
		sv_frametime = (cl.mtime[0] - cl.mtime[1]) / cl.movevars_timescale;

	// always send if buttons changed or an impulse is pending
	// even if it violates the rate limit!
	important = (cl.cmd.impulse || (cl_netimmediatebuttons.integer && cl.cmd.buttons != cl.movecmd[1].buttons));

	lag = cl.mtime[0] - cl.cmd.time;
	frames_per_tic = sv_frametime / cl.realframetime;
//	opportune_moment = lag <= cl.realframetime * 2; // FAIL: can miss the moment with uncapped fps
//	opportune_moment = lag <= cl.realframetime * frames_per_tic * 0.5; // FAIL: too early at high fps, reducing multi causes misses at moderate fps
	opportune_moment = lag <= cl.realframetime * (frames_per_tic <= 1 ? 1 : sqrt(frames_per_tic)); // perfect

	// Two methods for deciding when to send
	if (!important)
	{
		// Traditional time interval, now used as fallback
		if (sv_frametime <= 0 || lag > sv_frametime || lag < 0) // unknown ticrate || lag/PL || still connecting
		{
			if (cl.timesincepacket < packettime * 0.99999f)
			{
//				Con_Printf("^6moveft %f realft %f lag %f tic %f inputsince %d\n", cl.cmd.frametime, cl.realframetime, lag, sv_frametime, cl.opt_inputs_since_update);
				return;
			}
		}
		// Server-synchronised, for better pings
		// Sends at least once per server frame
		else // cl.opt_inputs_since_update is usable
		{
			if (!opportune_moment || cl.opt_inputs_since_update >= sv_frametime / packettime)
			{
//				Con_Printf("^1moveft %f realft %f lag %f tic %f inputsince %d\n", cl.cmd.frametime, cl.realframetime, lag, sv_frametime, cl.opt_inputs_since_update);
				return;
			}
		}
	}

	// don't choke the connection with packets (obey rate limit)
	// it is important that this check be last, because it adds a new
	// frame to the shownetgraph output and any cancelation after this
	// will produce a nasty spike-like look to the netgraph
	// we also still send if it is important
	if (!NetConn_CanSend(cls.netcon) && !important)
		return;

//	Con_Printf("%smoveft %f realft %f lag %f tic %f inputsince %d important %d\n", (lag < 0.0005 || !opportune_moment) ? "^3" : "^2", cl.cmd.frametime, cl.realframetime, lag, sv_frametime, cl.opt_inputs_since_update, important);

	if (opportune_moment) // this check is needed for optimal timing especially with cl_netimmediatebuttons
		++cl.opt_inputs_since_update;

	// reset the packet timing accumulator
	cl.timesincepacket = 0;

	buf.maxsize = sizeof(data);
	buf.cursize = 0;
	buf.data = data;

	// send the movement message
	// PROTOCOL_QUAKE        clc_move = 16 bytes total
	// PROTOCOL_QUAKEDP      clc_move = 16 bytes total
	// PROTOCOL_NEHAHRAMOVIE clc_move = 16 bytes total
	// PROTOCOL_DARKPLACES1  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES2  clc_move = 25 bytes total
	// PROTOCOL_DARKPLACES3  clc_move = 25 bytes total
	// PROTOCOL_DARKPLACES4  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES5  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES6  clc_move = 52 bytes total
	// PROTOCOL_DARKPLACES7  clc_move = 56 bytes total per move (can be up to 16 moves)
	// PROTOCOL_DARKPLACES8  clc_move = 56 bytes total per move (can be up to 16 moves)
	// PROTOCOL_QUAKEWORLD   clc_move = 34 bytes total (typically, but can reach 43 bytes, or even 49 bytes with roll)

	// set prydon cursor info
	CL_UpdatePrydonCursor();

	if (cls.protocol == PROTOCOL_QUAKEWORLD || cls.signon == SIGNONS)
	{
		switch (cls.protocol)
		{
		case PROTOCOL_QUAKEWORLD:
			MSG_WriteByte(&buf, qw_clc_move);
			// save the position for a checksum byte
			checksumindex = buf.cursize;
			MSG_WriteByte(&buf, 0);
			// packet loss percentage
			for (j = 0, packetloss = 0;j < NETGRAPH_PACKETS;j++)
				if (cls.netcon->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET)
					packetloss++;
			packetloss = packetloss * 100 / NETGRAPH_PACKETS;
			MSG_WriteByte(&buf, packetloss);
			// write most recent 3 moves
			QW_MSG_WriteDeltaUsercmd(&buf, &nullcmd, &cl.movecmd[2]);
			QW_MSG_WriteDeltaUsercmd(&buf, &cl.movecmd[2], &cl.movecmd[1]);
			QW_MSG_WriteDeltaUsercmd(&buf, &cl.movecmd[1], &cl.cmd);
			// calculate the checksum
			buf.data[checksumindex] = COM_BlockSequenceCRCByteQW(buf.data + checksumindex + 1, buf.cursize - checksumindex - 1, cls.netcon->outgoing_unreliable_sequence);
			// if delta compression history overflows, request no delta
			if (cls.netcon->outgoing_unreliable_sequence - cl.qw_validsequence >= QW_UPDATE_BACKUP-1)
				cl.qw_validsequence = 0;
			// request delta compression if appropriate
			if (cl.qw_validsequence && !cl_nodelta.integer && cls.state == ca_connected && !cls.demorecording)
			{
				cl.qw_deltasequence[cls.netcon->outgoing_unreliable_sequence & QW_UPDATE_MASK] = cl.qw_validsequence;
				MSG_WriteByte(&buf, qw_clc_delta);
				MSG_WriteByte(&buf, cl.qw_validsequence & 255);
			}
			else
				cl.qw_deltasequence[cls.netcon->outgoing_unreliable_sequence & QW_UPDATE_MASK] = -1;
			break;
		case PROTOCOL_QUAKE:
		case PROTOCOL_QUAKEDP:
		case PROTOCOL_NEHAHRAMOVIE:
		case PROTOCOL_NEHAHRABJP:
		case PROTOCOL_NEHAHRABJP2:
		case PROTOCOL_NEHAHRABJP3:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.cmd.time); // last server packet time
			// 3 bytes (6 bytes in proquake)
			if (cls.proquake_servermod == 1) // MOD_PROQUAKE
			{
				for (i = 0;i < 3;i++)
					MSG_WriteAngle16i (&buf, cl.cmd.viewangles[i]);
			}
			else
			{
				for (i = 0;i < 3;i++)
					MSG_WriteAngle8i (&buf, cl.cmd.viewangles[i]);
			}
			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.cmd.forwardmove);
			MSG_WriteCoord16i (&buf, cl.cmd.sidemove);
			MSG_WriteCoord16i (&buf, cl.cmd.upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.cmd.buttons);
			MSG_WriteByte (&buf, cl.cmd.impulse);
			break;
		case PROTOCOL_DARKPLACES2:
		case PROTOCOL_DARKPLACES3:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.cmd.time); // last server packet time
			// 12 bytes
			for (i = 0;i < 3;i++)
				MSG_WriteAngle32f (&buf, cl.cmd.viewangles[i]);
			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.cmd.forwardmove);
			MSG_WriteCoord16i (&buf, cl.cmd.sidemove);
			MSG_WriteCoord16i (&buf, cl.cmd.upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.cmd.buttons);
			MSG_WriteByte (&buf, cl.cmd.impulse);
			break;
		case PROTOCOL_DARKPLACES1:
		case PROTOCOL_DARKPLACES4:
		case PROTOCOL_DARKPLACES5:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.cmd.time); // last server packet time
			// 6 bytes
			for (i = 0;i < 3;i++)
				MSG_WriteAngle16i (&buf, cl.cmd.viewangles[i]);
			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.cmd.forwardmove);
			MSG_WriteCoord16i (&buf, cl.cmd.sidemove);
			MSG_WriteCoord16i (&buf, cl.cmd.upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.cmd.buttons);
			MSG_WriteByte (&buf, cl.cmd.impulse);
		case PROTOCOL_DARKPLACES6:
		case PROTOCOL_DARKPLACES7:
		case PROTOCOL_DARKPLACES8:
			// set the maxusercmds variable to limit how many should be sent
			maxusercmds = bound(1, cl_netrepeatinput.integer + 1, min(3, CL_MAX_USERCMDS));
			// when movement prediction is off, there's not much point in repeating old input as it will just be ignored
			if (!cl.cmd.predicted)
				maxusercmds = 1;

			// send the latest moves in order, the old ones will be
			// ignored by the server harmlessly, however if the previous
			// packets were lost these moves will be used
			//
			// this reduces packet loss impact on gameplay.
			for (j = 0, cmd = &cl.movecmd[maxusercmds-1];j < maxusercmds;j++, cmd--)
			{
				// don't repeat any stale moves
				if (cmd->sequence && cmd->sequence < cls.servermovesequence)
					continue;
				// 5/9 bytes
				MSG_WriteByte (&buf, clc_move);
				if (cls.protocol != PROTOCOL_DARKPLACES6)
					MSG_WriteLong (&buf, cmd->predicted ? cmd->sequence : 0);
				MSG_WriteFloat (&buf, cmd->time); // last server packet time
				// 6 bytes
				for (i = 0;i < 3;i++)
					MSG_WriteAngle16i (&buf, cmd->viewangles[i]);
				// 6 bytes
				MSG_WriteCoord16i (&buf, cmd->forwardmove);
				MSG_WriteCoord16i (&buf, cmd->sidemove);
				MSG_WriteCoord16i (&buf, cmd->upmove);
				// 5 bytes
				MSG_WriteLong (&buf, cmd->buttons);
				MSG_WriteByte (&buf, cmd->impulse);
				// PRYDON_CLIENTCURSOR
				// 30 bytes
				MSG_WriteShort (&buf, (short)(cmd->cursor_screen[0] * 32767.0f));
				MSG_WriteShort (&buf, (short)(cmd->cursor_screen[1] * 32767.0f));
				MSG_WriteFloat (&buf, cmd->cursor_start[0]);
				MSG_WriteFloat (&buf, cmd->cursor_start[1]);
				MSG_WriteFloat (&buf, cmd->cursor_start[2]);
				MSG_WriteFloat (&buf, cmd->cursor_impact[0]);
				MSG_WriteFloat (&buf, cmd->cursor_impact[1]);
				MSG_WriteFloat (&buf, cmd->cursor_impact[2]);
				MSG_WriteShort (&buf, cmd->cursor_entitynumber);
			}
			break;
		case PROTOCOL_UNKNOWN:
			break;
		}
	}

	if (cls.protocol != PROTOCOL_QUAKEWORLD && buf.cursize)
	{
		// ack entity frame numbers received since the last input was sent
		// (redundent to improve handling of client->server packet loss)
		// if cl_netrepeatinput is 1 and client framerate matches server
		// framerate, this is 10 bytes, if client framerate is lower this
		// will be more...
		unsigned int oldsequence = cl.cmd.sequence;
		unsigned int delta = bound(1, cl_netrepeatinput.integer + 1, 3);
		if (oldsequence > delta)
			oldsequence = oldsequence - delta;
		else
			oldsequence = 1;
		for (i = 0;i < LATESTFRAMENUMS;i++)
		{
			j = (cl.latestframenumsposition + i) % LATESTFRAMENUMS;
			if (cl.latestsendnums[j] >= oldsequence)
			{
				if (developer_networkentities.integer >= 10)
					Con_Printf("send clc_ackframe %i\n", cl.latestframenums[j]);
				MSG_WriteByte(&buf, clc_ackframe);
				MSG_WriteLong(&buf, cl.latestframenums[j]);
			}
		}
	}

	// PROTOCOL_DARKPLACES6 = 67 bytes per packet
	// PROTOCOL_DARKPLACES7 = 71 bytes per packet

	// acknowledge any recently received data blocks
	for (i = 0;i < CL_MAX_DOWNLOADACKS && (cls.dp_downloadack[i].start || cls.dp_downloadack[i].size);i++)
	{
		MSG_WriteByte(&buf, clc_ackdownloaddata);
		MSG_WriteLong(&buf, cls.dp_downloadack[i].start);
		MSG_WriteShort(&buf, cls.dp_downloadack[i].size);
		cls.dp_downloadack[i].start = 0;
		cls.dp_downloadack[i].size = 0;
	}

	// send the reliable message (forwarded commands) if there is one
	if (buf.cursize || cls.netcon->message.cursize)
		NetConn_SendUnreliableMessage(cls.netcon, &buf, cls.protocol, max(20*(buf.cursize+40), cl_rate.integer), cl_rate_burstsize.integer, false);

	if (quemove)
	{
		// update the cl.movecmd array which holds the most recent moves,
		// because we now need a new slot for the next input
		for (i = CL_MAX_USERCMDS - 1;i >= 1;i--)
			cl.movecmd[i] = cl.movecmd[i-1];
		cl.movecmd[0].msec = 0;
		cl.movecmd[0].frametime = 0;
	}

	// clear button 'click' states
	in_attack.state  &= ~2;
	in_jump.state    &= ~2;
	in_button3.state &= ~2;
	in_button4.state &= ~2;
	in_button5.state &= ~2;
	in_button6.state &= ~2;
	in_button7.state &= ~2;
	in_button8.state &= ~2;
	in_use.state     &= ~2;
	in_button9.state  &= ~2;
	in_button10.state &= ~2;
	in_button11.state &= ~2;
	in_button12.state &= ~2;
	in_button13.state &= ~2;
	in_button14.state &= ~2;
	in_button15.state &= ~2;
	in_button16.state &= ~2;
	// clear impulse
	in_impulse = 0;

	if (cls.netcon->message.overflowed)
		CL_DisconnectEx(true, "Lost connection to server");
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand(CF_CLIENT, "+moveup",IN_UpDown, "swim upward");
	Cmd_AddCommand(CF_CLIENT, "-moveup",IN_UpUp, "stop swimming upward");
	Cmd_AddCommand(CF_CLIENT, "+movedown",IN_DownDown, "swim downward");
	Cmd_AddCommand(CF_CLIENT, "-movedown",IN_DownUp, "stop swimming downward");
	Cmd_AddCommand(CF_CLIENT, "+left",IN_LeftDown, "turn left");
	Cmd_AddCommand(CF_CLIENT, "-left",IN_LeftUp, "stop turning left");
	Cmd_AddCommand(CF_CLIENT, "+right",IN_RightDown, "turn right");
	Cmd_AddCommand(CF_CLIENT, "-right",IN_RightUp, "stop turning right");
	Cmd_AddCommand(CF_CLIENT, "+forward",IN_ForwardDown, "move forward");
	Cmd_AddCommand(CF_CLIENT, "-forward",IN_ForwardUp, "stop moving forward");
	Cmd_AddCommand(CF_CLIENT, "+back",IN_BackDown, "move backward");
	Cmd_AddCommand(CF_CLIENT, "-back",IN_BackUp, "stop moving backward");
	Cmd_AddCommand(CF_CLIENT, "+lookup", IN_LookupDown, "look upward");
	Cmd_AddCommand(CF_CLIENT, "-lookup", IN_LookupUp, "stop looking upward");
	Cmd_AddCommand(CF_CLIENT, "+lookdown", IN_LookdownDown, "look downward");
	Cmd_AddCommand(CF_CLIENT, "-lookdown", IN_LookdownUp, "stop looking downward");
	Cmd_AddCommand(CF_CLIENT, "+strafe", IN_StrafeDown, "activate strafing mode (move instead of turn)");
	Cmd_AddCommand(CF_CLIENT, "-strafe", IN_StrafeUp, "deactivate strafing mode");
	Cmd_AddCommand(CF_CLIENT, "+moveleft", IN_MoveleftDown, "strafe left");
	Cmd_AddCommand(CF_CLIENT, "-moveleft", IN_MoveleftUp, "stop strafing left");
	Cmd_AddCommand(CF_CLIENT, "+moveright", IN_MoverightDown, "strafe right");
	Cmd_AddCommand(CF_CLIENT, "-moveright", IN_MoverightUp, "stop strafing right");
	Cmd_AddCommand(CF_CLIENT, "+speed", IN_SpeedDown, "activate run mode (faster movement and turning)");
	Cmd_AddCommand(CF_CLIENT, "-speed", IN_SpeedUp, "deactivate run mode");
	Cmd_AddCommand(CF_CLIENT, "+attack", IN_AttackDown, "begin firing");
	Cmd_AddCommand(CF_CLIENT, "-attack", IN_AttackUp, "stop firing");
	Cmd_AddCommand(CF_CLIENT, "+jump", IN_JumpDown, "jump");
	Cmd_AddCommand(CF_CLIENT, "-jump", IN_JumpUp, "end jump (so you can jump again)");
	Cmd_AddCommand(CF_CLIENT, "impulse", IN_Impulse, "send an impulse number to server (select weapon, use item, etc)");
	Cmd_AddCommand(CF_CLIENT, "+klook", IN_KLookDown, "activate keyboard looking mode, do not recenter view");
	Cmd_AddCommand(CF_CLIENT, "-klook", IN_KLookUp, "deactivate keyboard looking mode");
	Cmd_AddCommand(CF_CLIENT, "+mlook", IN_MLookDown, "activate mouse looking mode, do not recenter view");
	Cmd_AddCommand(CF_CLIENT, "-mlook", IN_MLookUp, "deactivate mouse looking mode");

	// LadyHavoc: added lots of buttons
	Cmd_AddCommand(CF_CLIENT, "+use", IN_UseDown, "use something (may be used by some mods)");
	Cmd_AddCommand(CF_CLIENT, "-use", IN_UseUp, "stop using something");
	Cmd_AddCommand(CF_CLIENT, "+button3", IN_Button3Down, "activate button3 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button3", IN_Button3Up, "deactivate button3");
	Cmd_AddCommand(CF_CLIENT, "+button4", IN_Button4Down, "activate button4 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button4", IN_Button4Up, "deactivate button4");
	Cmd_AddCommand(CF_CLIENT, "+button5", IN_Button5Down, "activate button5 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button5", IN_Button5Up, "deactivate button5");
	Cmd_AddCommand(CF_CLIENT, "+button6", IN_Button6Down, "activate button6 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button6", IN_Button6Up, "deactivate button6");
	Cmd_AddCommand(CF_CLIENT, "+button7", IN_Button7Down, "activate button7 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button7", IN_Button7Up, "deactivate button7");
	Cmd_AddCommand(CF_CLIENT, "+button8", IN_Button8Down, "activate button8 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button8", IN_Button8Up, "deactivate button8");
	Cmd_AddCommand(CF_CLIENT, "+button9", IN_Button9Down, "activate button9 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button9", IN_Button9Up, "deactivate button9");
	Cmd_AddCommand(CF_CLIENT, "+button10", IN_Button10Down, "activate button10 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button10", IN_Button10Up, "deactivate button10");
	Cmd_AddCommand(CF_CLIENT, "+button11", IN_Button11Down, "activate button11 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button11", IN_Button11Up, "deactivate button11");
	Cmd_AddCommand(CF_CLIENT, "+button12", IN_Button12Down, "activate button12 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button12", IN_Button12Up, "deactivate button12");
	Cmd_AddCommand(CF_CLIENT, "+button13", IN_Button13Down, "activate button13 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button13", IN_Button13Up, "deactivate button13");
	Cmd_AddCommand(CF_CLIENT, "+button14", IN_Button14Down, "activate button14 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button14", IN_Button14Up, "deactivate button14");
	Cmd_AddCommand(CF_CLIENT, "+button15", IN_Button15Down, "activate button15 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button15", IN_Button15Up, "deactivate button15");
	Cmd_AddCommand(CF_CLIENT, "+button16", IN_Button16Down, "activate button16 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button16", IN_Button16Up, "deactivate button16");

	// LadyHavoc: added bestweapon command
	Cmd_AddCommand(CF_CLIENT, "bestweapon", IN_BestWeapon_f, "send an impulse number to server to select the first usable weapon out of several (example: 8 7 6 5 4 3 2 1)");
	Cmd_AddCommand(CF_CLIENT, "register_bestweapon", IN_BestWeapon_Register_f, "(for QC usage only) change weapon parameters to be used by bestweapon; stuffcmd this in ClientConnect");

	Cvar_RegisterVariable(&cl_movecliptokeyboard);
	Cvar_RegisterVariable(&cl_movement);
	Cvar_RegisterVariable(&cl_movement_replay);
	Cvar_RegisterVariable(&cl_movement_nettimeout);
	Cvar_RegisterVariable(&cl_movement_minping);
	Cvar_RegisterVariable(&cl_movement_track_canjump);
	Cvar_RegisterVariable(&cl_movement_maxspeed);
	Cvar_RegisterVariable(&cl_movement_maxairspeed);
	Cvar_RegisterVariable(&cl_movement_stopspeed);
	Cvar_RegisterVariable(&cl_movement_friction);
	Cvar_RegisterVariable(&cl_movement_wallfriction);
	Cvar_RegisterVariable(&cl_movement_waterfriction);
	Cvar_RegisterVariable(&cl_movement_edgefriction);
	Cvar_RegisterVariable(&cl_movement_stepheight);
	Cvar_RegisterVariable(&cl_movement_accelerate);
	Cvar_RegisterVariable(&cl_movement_airaccelerate);
	Cvar_RegisterVariable(&cl_movement_wateraccelerate);
	Cvar_RegisterVariable(&cl_movement_jumpvelocity);
	Cvar_RegisterVariable(&cl_movement_airaccel_qw);
	Cvar_RegisterVariable(&cl_movement_airaccel_sideways_friction);
	Cvar_RegisterVariable(&cl_nopred);

	Cvar_RegisterVariable(&in_pitch_min);
	Cvar_RegisterVariable(&in_pitch_max);
	Cvar_RegisterVariable(&m_filter);
	Cvar_RegisterVariable(&m_accelerate);
	Cvar_RegisterVariable(&m_accelerate_minspeed);
	Cvar_RegisterVariable(&m_accelerate_maxspeed);
	Cvar_RegisterVariable(&m_accelerate_filter);
	Cvar_RegisterVariable(&m_accelerate_power);
	Cvar_RegisterVariable(&m_accelerate_power_offset);
	Cvar_RegisterVariable(&m_accelerate_power_senscap);
	Cvar_RegisterVariable(&m_accelerate_power_strength);
	Cvar_RegisterVariable(&m_accelerate_natural_offset);
	Cvar_RegisterVariable(&m_accelerate_natural_accelsenscap);
	Cvar_RegisterVariable(&m_accelerate_natural_strength);

	Cvar_RegisterVariable(&cl_netfps);
	Cvar_RegisterVariable(&cl_netrepeatinput);
	Cvar_RegisterVariable(&cl_netimmediatebuttons);

	Cvar_RegisterVariable(&cl_nodelta);

	Cvar_RegisterVariable(&cl_csqc_generatemousemoveevents);
}

