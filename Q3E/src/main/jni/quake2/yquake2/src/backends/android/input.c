/*
 * Copyright (C) 2010 Yamagi Burmeister
 * Copyright (C) 1997-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Joystick reading and deadzone handling is based on:
 * http://joshsutphin.com/2013/04/12/doing-thumbstick-dead-zones-right.html
 * ...and implementation is partially based on code from:
 * - http://quakespasm.sourceforge.net
 * - https://github.com/Minimuino/thumbstick-deadzones
 *
 * Flick Stick handling is based on:
 * http://gyrowiki.jibbsmart.com/blog:good-gyro-controls-part-2:the-flick-stick
 *
 * =======================================================================
 *
 * This is the Quake II input system backend, implemented with SDL.
 *
 * =======================================================================
 */

#include "../../client/input/header/input.h"
#include "../../client/header/keyboard.h"
#include "../../client/header/client.h"

// ----

// Maximal mouse move per frame
#define MOUSE_MAX 3000

// Minimal mouse move per frame
#define MOUSE_MIN 40

extern void Android_PollInput(void);
extern void Sys_SyncState(void);

// ----

enum {
	LAYOUT_DEFAULT			= 0,
	LAYOUT_SOUTHPAW,
	LAYOUT_LEGACY,
	LAYOUT_LEGACY_SOUTHPAW,
	LAYOUT_FLICK_STICK,
	LAYOUT_FLICK_STICK_SOUTHPAW
};

typedef struct
{
	float x;
	float y;
} thumbstick_t;

typedef enum
{
	REASON_NONE,
	REASON_CONTROLLERINIT,
	REASON_GYROCALIBRATION
} updates_countdown_reasons;

// ----

// These are used to communicate the events collected by
// IN_Update() called at the beginning of a frame to the
// actual movement functions called at a later time.
float mouse_x, mouse_y;
static int joystick_left_x, joystick_left_y, joystick_right_x, joystick_right_y;
static float gyro_yaw, gyro_pitch;
static qboolean mlooking;

// The last time input events were processed.
// Used throughout the client.
int sys_frame_time;

// the joystick altselector that turns K_BTN_X into K_BTN_X_ALT
// is pressed
qboolean joy_altselector_pressed = false;

// Console Variables
cvar_t *freelook;
cvar_t *lookstrafe;
cvar_t *m_forward;
cvar_t *m_pitch;
cvar_t *m_side;
cvar_t *m_up;
cvar_t *m_yaw;
static cvar_t *sensitivity;

static cvar_t *exponential_speedup;
static cvar_t *in_grab;
static cvar_t *m_filter;
static cvar_t *windowed_pauseonfocuslost;
static cvar_t *windowed_mouse;
static cvar_t *haptic_feedback_filter;

// ----

typedef struct haptic_effects_cache {
	int effect_volume;
	int effect_duration;
	int effect_delay;
	int effect_attack;
	int effect_fade;
	int effect_id;
	int effect_x;
	int effect_y;
	int effect_z;
} haptic_effects_cache_t;

qboolean show_gamepad = false, show_haptic = false, show_gyro = false;

#define HAPTIC_EFFECT_LIST_SIZE 16

static int last_haptic_volume = 0;
static int last_haptic_effect_size = HAPTIC_EFFECT_LIST_SIZE;
static int last_haptic_effect_pos = 0;
static haptic_effects_cache_t last_haptic_effect[HAPTIC_EFFECT_LIST_SIZE];

// Joystick sensitivity
static cvar_t *joy_yawsensitivity;
static cvar_t *joy_pitchsensitivity;
static cvar_t *joy_forwardsensitivity;
static cvar_t *joy_sidesensitivity;

// Joystick's analog sticks configuration
cvar_t *joy_layout;
static cvar_t *joy_left_expo;
static cvar_t *joy_left_snapaxis;
static cvar_t *joy_left_deadzone;
static cvar_t *joy_right_expo;
static cvar_t *joy_right_snapaxis;
static cvar_t *joy_right_deadzone;
static cvar_t *joy_flick_threshold;
static cvar_t *joy_flick_smoothed;

// Joystick haptic
static cvar_t *joy_haptic_magnitude;
static cvar_t *joy_haptic_distance;

// Gyro mode (0=off, 3=on, 1-2=uses button to enable/disable)
cvar_t *gyro_mode;
cvar_t *gyro_turning_axis;	// yaw or roll

// Gyro sensitivity
static cvar_t *gyro_yawsensitivity;
static cvar_t *gyro_pitchsensitivity;

// Gyro is being used in this very moment
static qboolean gyro_active = false;

// Gyro calibration
static float gyro_accum[3];
static cvar_t *gyro_calibration_x;
static cvar_t *gyro_calibration_y;
static cvar_t *gyro_calibration_z;

// To ignore SDL_JOYDEVICEADDED at game init. Allows for hot plugging of game controller afterwards.
static qboolean first_init = true;

// Countdown of calls to IN_Update(), needed for controller init and gyro calibration
static unsigned short int updates_countdown = 30;

// Reason for the countdown
static updates_countdown_reasons countdown_reason = REASON_CONTROLLERINIT;

// Flick Stick
#define FLICK_TIME 6		// number of frames it takes for a flick to execute
static float target_angle;	// angle to end up facing at the end of a flick
static unsigned short int flick_progress = FLICK_TIME;

// Flick Stick's rotation input samples to smooth out
#define MAX_SMOOTH_SAMPLES 8
static float flick_samples[MAX_SMOOTH_SAMPLES];
static unsigned short int front_sample = 0;

extern void CalibrationFinishedCallback(void);

/* ------------------------------------------------------------------ */

/*
 * This creepy function translates SDL keycodes into
 * the id Tech 2 engines interal representation.
 */

static void IN_Controller_Init(qboolean notify_user);
static void IN_Controller_Shutdown(qboolean notify_user);

qboolean IN_NumpadIsOn()
{
    return true;
}

/* ------------------------------------------------------------------ */

/*
 * Updates the input queue state. Called every
 * frame by the client and does nearly all the
 * input magic.
 */
void
IN_Update(void)
{
	qboolean want_grab;

	Sys_SyncState();
	Android_PollInput();

	/* Grab and ungrab the mouse if the console or the menu is opened */
	if (in_grab->value == 3)
	{
		want_grab = windowed_mouse->value;
	}
	else
	{
		want_grab = (vid_fullscreen->value || in_grab->value == 1 ||
			(in_grab->value == 2 && windowed_mouse->value));
	}

	// calling GLimp_GrabInput() each frame is a bit ugly but simple and should work.
	// The called SDL functions return after a cheap check, if there's nothing to do.
	GLimp_GrabInput(want_grab);

	// We need to save the frame time so other subsystems
	// know the exact time of the last input events.
	sys_frame_time = Sys_Milliseconds();
}

/*
 * Move handling
 */
void
IN_Move(usercmd_t *cmd)
{
	// Factor used to transform from SDL joystick input ([-32768, 32767])  to [-1, 1] range
	static const float normalize_sdl_axis = 1.0f / 32768.0f;

	// Flick Stick's factors to change to the target angle with a feeling of "ease out"
	static const float rotation_factor[FLICK_TIME] =
	{
		0.305555556f, 0.249999999f, 0.194444445f, 0.138888889f, 0.083333333f, 0.027777778f
	};

	static float old_mouse_x;
	static float old_mouse_y;

	if (m_filter->value)
	{
		if ((mouse_x > 1) || (mouse_x < -1))
		{
			mouse_x = (mouse_x + old_mouse_x) * 0.5;
		}

		if ((mouse_y > 1) || (mouse_y < -1))
		{
			mouse_y = (mouse_y + old_mouse_y) * 0.5;
		}
	}

	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;

	if (mouse_x || mouse_y)
	{
		if (!exponential_speedup->value)
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}
		else
		{
			if ((mouse_x > MOUSE_MIN) || (mouse_y > MOUSE_MIN) ||
				(mouse_x < -MOUSE_MIN) || (mouse_y < -MOUSE_MIN))
			{
				mouse_x = (mouse_x * mouse_x * mouse_x) / 4;
				mouse_y = (mouse_y * mouse_y * mouse_y) / 4;

				if (mouse_x > MOUSE_MAX)
				{
					mouse_x = MOUSE_MAX;
				}
				else if (mouse_x < -MOUSE_MAX)
				{
					mouse_x = -MOUSE_MAX;
				}

				if (mouse_y > MOUSE_MAX)
				{
					mouse_y = MOUSE_MAX;
				}
				else if (mouse_y < -MOUSE_MAX)
				{
					mouse_y = -MOUSE_MAX;
				}
			}
		}

		// add mouse X/Y movement to cmd
		if ((in_strafe.state & 1) || (lookstrafe->value && mlooking))
		{
			cmd->sidemove += m_side->value * mouse_x;
		}
		else
		{
			cl.viewangles[YAW] -= m_yaw->value * mouse_x;
		}

		if ((mlooking || freelook->value) && !(in_strafe.state & 1))
		{
			cl.viewangles[PITCH] += m_pitch->value * mouse_y;
		}
		else
		{
			cmd->forwardmove -= m_forward->value * mouse_y;
		}

		mouse_x = mouse_y = 0;
	}
}

/* ------------------------------------------------------------------ */

/*
 * Look down
 */
static void
IN_MLookDown(void)
{
	mlooking = true;
}

/*
 * Look up
 */
static void
IN_MLookUp(void)
{
	mlooking = false;
	IN_CenterView();
}

static void
IN_JoyAltSelectorDown(void)
{
	joy_altselector_pressed = true;
}

static void
IN_JoyAltSelectorUp(void)
{
	joy_altselector_pressed = false;
}

static void
IN_GyroActionDown(void)
{
	switch ((int)gyro_mode->value)
	{
		case 1:
			gyro_active = true;
			return;
		case 2:
			gyro_active = false;
	}
}

static void
IN_GyroActionUp(void)
{
	switch ((int)gyro_mode->value)
	{
		case 1:
			gyro_active = false;
			return;
		case 2:
			gyro_active = true;
	}
}

/*
 * Removes all pending events from SDLs queue.
 */
void
In_FlushQueue(void)
{
	Key_MarkAllUp();
	IN_JoyAltSelectorUp();
}

/* ------------------------------------------------------------------ */

static void IN_Haptic_Shutdown(void);

/*
 * Init haptic effects
 */
static int
IN_Haptic_Effect_Init(int effect_x, int effect_y, int effect_z,
				 int period, int magnitude,
				 int delay, int attack, int fade)
{
	return 0;
}

/*
 * Shuts the backend down
 */
static void
IN_Haptic_Effect_Shutdown(int * effect_id)
{
	if (!effect_id)
	{
		return;
	}

	if (*effect_id >= 0)
	{
	}

	*effect_id = -1;
}

static void
IN_Haptic_Effects_Shutdown(void)
{
	for (int i=0; i<HAPTIC_EFFECT_LIST_SIZE; i++)
	{
		last_haptic_effect[i].effect_volume = 0;
		last_haptic_effect[i].effect_duration = 0;
		last_haptic_effect[i].effect_delay = 0;
		last_haptic_effect[i].effect_attack = 0;
		last_haptic_effect[i].effect_fade = 0;
		last_haptic_effect[i].effect_x = 0;
		last_haptic_effect[i].effect_y = 0;
		last_haptic_effect[i].effect_z = 0;

		IN_Haptic_Effect_Shutdown(&last_haptic_effect[i].effect_id);
	}
}

static int
IN_Haptic_GetEffectId(int effect_volume, int effect_duration,
				int effect_delay, int effect_attack, int effect_fade,
				int effect_x, int effect_y, int effect_z)
{
	int i, haptic_volume;

	// check effects for reuse
	for (i=0; i < last_haptic_effect_size; i++)
	{
		if (
			last_haptic_effect[i].effect_volume != effect_volume ||
			last_haptic_effect[i].effect_duration != effect_duration ||
			last_haptic_effect[i].effect_delay != effect_delay ||
			last_haptic_effect[i].effect_attack != effect_attack ||
			last_haptic_effect[i].effect_fade != effect_fade ||
			last_haptic_effect[i].effect_x != effect_x ||
			last_haptic_effect[i].effect_y != effect_y ||
			last_haptic_effect[i].effect_z != effect_z)
		{
			continue;
		}

		return last_haptic_effect[i].effect_id;
	}

	/* create new effect */
	haptic_volume = joy_haptic_magnitude->value * effect_volume; // 32767 max strength;

	/*
	Com_Printf("%d: volume %d: %d ms %d:%d:%d ms speed: %.2f\n",
		last_haptic_effect_pos,  effect_volume, effect_duration,
		effect_delay, effect_attack, effect_fade,
		(float)effect_volume / effect_fade);
	*/

	// FIFO for effects
	last_haptic_effect_pos = (last_haptic_effect_pos + 1) % last_haptic_effect_size;
	IN_Haptic_Effect_Shutdown(&last_haptic_effect[last_haptic_effect_pos].effect_id);
	last_haptic_effect[last_haptic_effect_pos].effect_volume = effect_volume;
	last_haptic_effect[last_haptic_effect_pos].effect_duration = effect_duration;
	last_haptic_effect[last_haptic_effect_pos].effect_delay = effect_delay;
	last_haptic_effect[last_haptic_effect_pos].effect_attack = effect_attack;
	last_haptic_effect[last_haptic_effect_pos].effect_fade = effect_fade;
	last_haptic_effect[last_haptic_effect_pos].effect_x = effect_x;
	last_haptic_effect[last_haptic_effect_pos].effect_y = effect_y;
	last_haptic_effect[last_haptic_effect_pos].effect_z = effect_z;
	last_haptic_effect[last_haptic_effect_pos].effect_id = IN_Haptic_Effect_Init(
		effect_x, effect_y, effect_z,
		effect_duration, haptic_volume,
		effect_delay, effect_attack, effect_fade);

	return last_haptic_effect[last_haptic_effect_pos].effect_id;
}

// Keep it same with rumble rules, look for descriptions to rumble
// filtering in Controller_Rumble
static char *default_haptic_filter = (
	// skipped files should be before wider rule
	"!weapons/*grenlb "     // bouncing grenades don't have feedback
	"!weapons/*hgrenb "     // bouncing grenades don't have feedback
	"!weapons/*open "       // rogue's items don't have feedback
	"!weapons/*warn "       // rogue's items don't have feedback
	// any weapons that are not in previous list
	"weapons/ "
	// player{,s} effects
	"player/*land "         // fall without injury
	"player/*burn "
	"player/*pain "
	"player/*fall "
	"player/*death "
	"players/*burn "
	"players/*pain "
	"players/*fall "
	"players/*death "
	// environment effects
	"doors/ "
	"plats/ "
	"world/*dish  "
	"world/*drill2a "
	"world/*dr_ "
	"world/*explod1 "
	"world/*rocks "
	"world/*rumble  "
	"world/*quake  "
	"world/*train2 "
);

/*
 * name: sound name
 * filter: sound name rule with '*'
 * return false for empty filter
 */
static qboolean
Haptic_Feedback_Filtered_Line(const char *name, const char *filter)
{
	const char *current_filter = filter;

	// skip empty filter
	if (!*current_filter)
	{
		return false;
	}

	while (*current_filter)
	{
		char part_filter[MAX_QPATH];
		const char *name_part;
		const char *str_end;

		str_end = strchr(current_filter, '*');
		if (!str_end)
		{
			if (!strstr(name, current_filter))
			{
				// no such part in string
				return false;
			}
			// have such part
			break;
		}
		// copy filter line
		if ((str_end - current_filter) >= MAX_QPATH)
		{
			return false;
		}
		memcpy(part_filter, current_filter, str_end - current_filter);
		part_filter[str_end - current_filter] = 0;
		// place part in name
		name_part = strstr(name, part_filter);
		if (!name_part)
		{
			// no such part in string
			return false;
		}
		// have such part
		name = name_part + strlen(part_filter);
		// move to next filter
		current_filter = str_end + 1;
	}

	return true;
}

/*
 * name: sound name
 * filter: sound names separated by space, and '!' for skip file
 */
static qboolean
Haptic_Feedback_Filtered(const char *name, const char *filter)
{
	const char *current_filter = filter;

	while (*current_filter)
	{
		char line_filter[MAX_QPATH];
		const char *str_end;

		str_end = strchr(current_filter, ' ');
		// its end of filter
		if (!str_end)
		{
			// check rules inside line
			if (Haptic_Feedback_Filtered_Line(name, current_filter))
			{
				return true;
			}
			return false;
		}
		// copy filter line
		if ((str_end - current_filter) >= MAX_QPATH)
		{
			return false;
		}
		memcpy(line_filter, current_filter, str_end - current_filter);
		line_filter[str_end - current_filter] = 0;
		// check rules inside line
		if (*line_filter == '!')
		{
			// has invert rule
			if (Haptic_Feedback_Filtered_Line(name, line_filter + 1))
			{
				return false;
			}
		}
		else
		{
			if (Haptic_Feedback_Filtered_Line(name, line_filter))
			{
				return true;
			}
		}
		// move to next filter
		current_filter = str_end + 1;
	}
	return false;
}

/*
 * Haptic Feedback:
 *    effect_volume=0..SHRT_MAX
 *    effect{x,y,z} - effect direction
 *    effect{delay,attack,fade} - effect durations
 *    effect_distance - distance to sound source
 *    name - sound file name
 */
void
Haptic_Feedback(const char *name, int effect_volume, int effect_duration,
				int effect_delay, int effect_attack, int effect_fade,
				int effect_x, int effect_y, int effect_z, float effect_distance)
{
}

/*
 * Controller_Rumble:
 *	name = sound file name
 *	effect_volume = 0..USHRT_MAX
 *	effect_duration is in ms
 *	source = origin of audio
 *	from_player = if source is the client (player)
 */
void
Controller_Rumble(const char *name, vec3_t source, qboolean from_player,
		unsigned int duration, unsigned short int volume)
{
	vec_t intens = 0.0f, low_freq = 1.0f, hi_freq = 1.0f, dist_prop;
	unsigned short int max_distance = 4;
	unsigned int effect_volume;

	if (!show_haptic || joy_haptic_magnitude->value <= 0
		|| volume == 0 || duration == 0)
	{
		return;
	}

	if (strstr(name, "weapons/"))
	{
		intens = 1.75f;

		if (strstr(name, "/blastf") || strstr(name, "/hyprbf") || strstr(name, "/nail"))
		{
			intens = 0.125f;	// dampen blasters and nailgun's fire
			low_freq = 0.7f;
			hi_freq = 1.2f;
		}
		else if (strstr(name, "/shotgf") || strstr(name, "/rocklf"))
		{
			low_freq = 1.1f;	// shotgun & RL shouldn't feel so weak
			duration *= 0.7;
		}
		else if (strstr(name, "/sshotf"))
		{
			duration *= 0.6;	// the opposite for super shotgun
		}
		else if (strstr(name, "/machgf") || strstr(name, "/disint"))
		{
			intens = 1.125f;	// machine gun & disruptor fire
		}
		else if (strstr(name, "/grenlb") || strstr(name, "/hgrenb")	// bouncing grenades
			|| strstr(name, "open") || strstr(name, "warn"))	// rogue's items
		{
			return;	// ... don't have feedback
		}
		else if (strstr(name, "/plasshot"))	// phalanx cannon
		{
			intens = 1.0f;
			hi_freq = 0.3f;
			duration *= 0.5;
		}
		else if (strstr(name, "x"))		// explosions...
		{
			low_freq = 1.1f;
			hi_freq = 0.9f;
			max_distance = 550;		// can be felt far away
		}
		else if (strstr(name, "r"))		// reloads & ion ripper fire
		{
			low_freq = 0.1f;
			hi_freq = 0.6f;
		}
	}
	else if (strstr(name, "player/land"))
	{
		intens = 2.2f;	// fall without injury
		low_freq = 1.1f;
	}
	else if (strstr(name, "player/") || strstr(name, "players/"))
	{
		low_freq = 1.2f;	// exaggerate player damage
		if (strstr(name, "/burn") || strstr(name, "/pain100") || strstr(name, "/pain75"))
		{
			intens = 2.4f;
		}
		else if (strstr(name, "/fall") || strstr(name, "/pain50") || strstr(name, "/pain25"))
		{
			intens = 2.7f;
		}
		else if (strstr(name, "/death"))
		{
			intens = 2.9f;
		}
	}
	else if (strstr(name, "doors/"))
	{
		intens = 0.125f;
		low_freq = 0.4f;
		max_distance = 280;
	}
	else if (strstr(name, "plats/"))
	{
		intens = 1.0f;			// platforms rumble...
		max_distance = 200;		// when player near them
	}
	else if (strstr(name, "world/"))
	{
		max_distance = 3500;	// ambient events
		if (strstr(name, "/dish") || strstr(name, "/drill2a") || strstr(name, "/dr_")
			|| strstr(name, "/explod1") || strstr(name, "/rocks")
			|| strstr(name, "/rumble"))
		{
			intens = 0.28f;
			low_freq = 0.7f;
		}
		else if (strstr(name, "/quake"))
		{
			intens = 0.67f;		// (earth)quakes are more evident
			low_freq = 1.2f;
		}
		else if (strstr(name, "/train2"))
		{
			intens = 0.28f;
			max_distance = 290;	// just machinery
		}
	}

	if (intens == 0.0f)
	{
		return;
	}

	if (from_player)
	{
		dist_prop = 1.0f;
	}
	else
	{
		dist_prop = VectorLength(source);
		if (dist_prop > max_distance)
		{
			return;
		}
		dist_prop = (max_distance - dist_prop) / max_distance;
	}

	effect_volume = joy_haptic_magnitude->value * intens * dist_prop * volume;
	low_freq = Q_min(effect_volume * low_freq, USHRT_MAX);
	hi_freq = Q_min(effect_volume * hi_freq, USHRT_MAX);

	// Com_Printf("%-29s: vol %5u - %4u ms - dp %.3f l %5.0f h %5.0f\n",
	//	name, effect_volume, duration, dist_prop, low_freq, hi_freq);
}

/*
 * Gyro calibration functions, called from menu
 */
void
StartCalibration(void)
{
	gyro_accum[0] = 0.0;
	gyro_accum[1] = 0.0;
	gyro_accum[2] = 0.0;
	updates_countdown = 300;
	countdown_reason = REASON_GYROCALIBRATION;
}

qboolean
IsCalibrationZero(void)
{
	return (!gyro_calibration_x->value && !gyro_calibration_y->value && !gyro_calibration_z->value);
}

/*
 * Game Controller
 */
static void
IN_Controller_Init(qboolean notify_user)
{
}

/*
 * Initializes the backend
 */
void
IN_Init(void)
{
	Com_Printf("------- input initialization -------\n");

	mouse_x = mouse_y = 0;
	joystick_left_x = joystick_left_y = joystick_right_x = joystick_right_y = 0;
	gyro_yaw = gyro_pitch = 0;

	exponential_speedup = Cvar_Get("exponential_speedup", "0", CVAR_ARCHIVE);
	freelook = Cvar_Get("freelook", "1", CVAR_ARCHIVE);
	in_grab = Cvar_Get("in_grab", "2", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get("lookstrafe", "0", CVAR_ARCHIVE);
	m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE);
	m_up = Cvar_Get("m_up", "1", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "1", CVAR_ARCHIVE);
	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_side = Cvar_Get("m_side", "0.8", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);

	joy_haptic_magnitude = Cvar_Get("joy_haptic_magnitude", "0.0", CVAR_ARCHIVE);
	joy_haptic_distance = Cvar_Get("joy_haptic_distance", "100.0", CVAR_ARCHIVE);
	haptic_feedback_filter = Cvar_Get("joy_haptic_filter", default_haptic_filter, CVAR_ARCHIVE);

	joy_yawsensitivity = Cvar_Get("joy_yawsensitivity", "1.0", CVAR_ARCHIVE);
	joy_pitchsensitivity = Cvar_Get("joy_pitchsensitivity", "1.0", CVAR_ARCHIVE);
	joy_forwardsensitivity = Cvar_Get("joy_forwardsensitivity", "1.0", CVAR_ARCHIVE);
	joy_sidesensitivity = Cvar_Get("joy_sidesensitivity", "1.0", CVAR_ARCHIVE);

	joy_layout = Cvar_Get("joy_layout", "0", CVAR_ARCHIVE);
	joy_left_expo = Cvar_Get("joy_left_expo", "2.0", CVAR_ARCHIVE);
	joy_left_snapaxis = Cvar_Get("joy_left_snapaxis", "0.15", CVAR_ARCHIVE);
	joy_left_deadzone = Cvar_Get("joy_left_deadzone", "0.16", CVAR_ARCHIVE);
	joy_right_expo = Cvar_Get("joy_right_expo", "2.0", CVAR_ARCHIVE);
	joy_right_snapaxis = Cvar_Get("joy_right_snapaxis", "0.15", CVAR_ARCHIVE);
	joy_right_deadzone = Cvar_Get("joy_right_deadzone", "0.16", CVAR_ARCHIVE);
	joy_flick_threshold = Cvar_Get("joy_flick_threshold", "0.65", CVAR_ARCHIVE);
	joy_flick_smoothed = Cvar_Get("joy_flick_smoothed", "8.0", CVAR_ARCHIVE);

	gyro_calibration_x = Cvar_Get("gyro_calibration_x", "0.0", CVAR_ARCHIVE);
	gyro_calibration_y = Cvar_Get("gyro_calibration_y", "0.0", CVAR_ARCHIVE);
	gyro_calibration_z = Cvar_Get("gyro_calibration_z", "0.0", CVAR_ARCHIVE);

	gyro_yawsensitivity = Cvar_Get("gyro_yawsensitivity", "1.0", CVAR_ARCHIVE);
	gyro_pitchsensitivity = Cvar_Get("gyro_pitchsensitivity", "1.0", CVAR_ARCHIVE);
	gyro_turning_axis = Cvar_Get("gyro_turning_axis", "0", CVAR_ARCHIVE);

	gyro_mode = Cvar_Get("gyro_mode", "2", CVAR_ARCHIVE);
	if ((int)gyro_mode->value == 2)
	{
		gyro_active = true;
	}

	windowed_pauseonfocuslost = Cvar_Get("vid_pauseonfocuslost", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	windowed_mouse = Cvar_Get("windowed_mouse", "1", CVAR_USERINFO | CVAR_ARCHIVE);

	Cmd_AddCommand("+mlook", IN_MLookDown);
	Cmd_AddCommand("-mlook", IN_MLookUp);

	Cmd_AddCommand("+joyaltselector", IN_JoyAltSelectorDown);
	Cmd_AddCommand("-joyaltselector", IN_JoyAltSelectorUp);
	Cmd_AddCommand("+gyroaction", IN_GyroActionDown);
	Cmd_AddCommand("-gyroaction", IN_GyroActionUp);

	IN_Controller_Init(false);

	Com_Printf("------------------------------------\n\n");
}

/*
 * Shuts the backend down
 */
static void
IN_Haptic_Shutdown(void)
{
	IN_Haptic_Effects_Shutdown();
}

static void
IN_Controller_Shutdown(qboolean notify_user)
{
	if (notify_user)
	{
		Com_Printf("- Game Controller disconnected -\n");
	}

	IN_Haptic_Shutdown();

	show_gamepad = show_gyro = show_haptic = false;
	joystick_left_x = joystick_left_y = joystick_right_x = joystick_right_y = 0;
	gyro_yaw = gyro_pitch = 0;
}

/*
 * Shuts the backend down
 */
void
IN_Shutdown(void)
{
	Cmd_RemoveCommand("force_centerview");
	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");

	Cmd_RemoveCommand("+joyaltselector");
	Cmd_RemoveCommand("-joyaltselector");
	Cmd_RemoveCommand("+gyroaction");
	Cmd_RemoveCommand("-gyroaction");

	Com_Printf("Shutting down input.\n");

	IN_Controller_Shutdown(false);
}

/* ------------------------------------------------------------------ */
