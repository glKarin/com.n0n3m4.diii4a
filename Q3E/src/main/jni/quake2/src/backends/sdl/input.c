/*
 * Copyright (C) 2010 Yamagi Burmeister
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * =======================================================================
 *
 * This is the Quake II input system, written in SDL.
 *
 * =======================================================================
 */
 
#include "../../refresh/header/local.h"
#include "../../client/header/keyboard.h"
#include "../generic/header/input.h"

#define MOUSE_MAX 3000
#define MOUSE_MIN 40

static qboolean mouse_grabbed;
static cvar_t *windowed_mouse;
static cvar_t *in_grab;
static int mouse_x, mouse_y;
static int old_mouse_x, old_mouse_y;
static int mouse_buttonstate;
static int mouse_oldbuttonstate;

struct
{
	int key;
	int down;
} keyq[128];

int keyq_head = 0;
int keyq_tail = 0;
float fmx;
float fmy;

Key_Event_fp_t Key_Event_fp;

static in_state_t *in_state;
static qboolean mlooking;

static cvar_t *sensitivity;
static cvar_t *exponential_speedup;
static cvar_t *lookstrafe;
static cvar_t *m_side;
static cvar_t *m_yaw;
static cvar_t *m_pitch;
static cvar_t *m_forward;
static cvar_t *freelook;
static cvar_t *m_filter;
static cvar_t *in_mouse;

//cvar_t *vid_fullscreen;

/*
 * Input event processing
 */

void QueueEvent(int state,int key)
{
keyq[keyq_head].key = key;
keyq[keyq_head].down = state;
keyq_head = (keyq_head + 1) & 127;
}

#pragma GCC visibility push(default)
void KeyEventHidden(int state,int key,int character)
{
    if (key!=0)
    {
    QueueEvent(state, key);
    }
}

void MotionEventHidden(float dx, float dy)
{
     fmx += dx;
     fmy += dy;
}

/*
 * Updates the state of the input queue
 */

void
IN_Update(void)
{
	static int IN_Update_Flag;
	int bstate;

	/* Protection against multiple calls */
	if (IN_Update_Flag == 1)
	{
		return;
	}

	IN_Update_Flag = 1;

	/* Process the key events */
	while (keyq_head != keyq_tail)
	{
		in_state->Key_Event_fp(keyq[keyq_tail].key, keyq[keyq_tail].down);
		keyq_tail = (keyq_tail + 1) & 127;
	}

	IN_Update_Flag = 0;
}

/*
 * Closes all inputs and clears
 * the input queue.
 */
void
IN_Close(void)
{
	keyq_head = 0;
	keyq_tail = 0;

	memset(keyq, 0, sizeof(keyq));
}
#pragma GCC visibility pop

/*
 * Gets the mouse state
 */
void
IN_GetMouseState(int *x, int *y, int *state)
{
	*x = (int)fmx;
	*y = (int)fmy;
	*state = mouse_buttonstate;
}

/*
 * Cleares the mouse state
 */
void
IN_ClearMouseState()
{
	fmx = fmy = 0;
}

/*
 * Centers the view
 */
static void
IN_ForceCenterView(void)
{
	in_state->viewangles[PITCH] = 0;
}

/*
 * Look up
 */
static void
IN_MLookDown(void)
{
	mlooking = true;
}

/*
 * Look down
 */
static void
IN_MLookUp(void)
{
	mlooking = false;
	in_state->IN_CenterView_fp();
}

/*
 * Keyboard initialisation. Called
 * by the client via function pointer
 */
#pragma GCC visibility push(default)
void
IN_KeyboardInit(Key_Event_fp_t fp)
{
	Key_Event_fp = fp;
}

/*
 * Initializes the backend
 */
void
IN_BackendInit(in_state_t *in_state_p)
{
	in_state = in_state_p;
	m_filter = ri.Cvar_Get("m_filter", "0", CVAR_ARCHIVE);
	in_mouse = ri.Cvar_Get("in_mouse", "0", CVAR_ARCHIVE);

	freelook = ri.Cvar_Get("freelook", "1", 0);
	lookstrafe = ri.Cvar_Get("lookstrafe", "0", 0);
	sensitivity = ri.Cvar_Get("sensitivity", "3", 0);
	exponential_speedup = ri.Cvar_Get("exponential_speedup", "0", CVAR_ARCHIVE);

	m_pitch = ri.Cvar_Get("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get("m_forward", "1", 0);
	m_side = ri.Cvar_Get("m_side", "0.8", 0);

	ri.Cmd_AddCommand("+mlook", IN_MLookDown);
	ri.Cmd_AddCommand("-mlook", IN_MLookUp);
	ri.Cmd_AddCommand("force_centerview", IN_ForceCenterView);

	mouse_x = mouse_y = 0.0;

	mouse_grabbed = true;
	
	windowed_mouse = ri.Cvar_Get("windowed_mouse", "1",
			CVAR_USERINFO | CVAR_ARCHIVE);
	in_grab = ri.Cvar_Get("in_grab", "2", CVAR_ARCHIVE);

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);

	ri.Con_Printf(PRINT_ALL, "Input initialized.\n");
}

/*
 * Shuts the backend down
 */
void
IN_BackendShutdown(void)
{
	ri.Cmd_RemoveCommand("+mlook");
	ri.Cmd_RemoveCommand("-mlook");
	ri.Cmd_RemoveCommand("force_centerview");
	ri.Con_Printf(PRINT_ALL, "Input shut down.\n");
}

/*
 * Mouse button handling
 */
void
IN_BackendMouseButtons(void)
{
	int i;

	IN_GetMouseState(&mouse_x, &mouse_y, &mouse_buttonstate);

	for (i = 0; i < 3; i++)
	{
		if ((mouse_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
		{
			in_state->Key_Event_fp(K_MOUSE1 + i, true);
		}

		if (!(mouse_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
		{
			in_state->Key_Event_fp(K_MOUSE1 + i, false);
		}
	}

	if ((mouse_buttonstate & (1 << 3)) && !(mouse_oldbuttonstate & (1 << 3)))
	{
		in_state->Key_Event_fp(K_MOUSE4, true);
	}

	if (!(mouse_buttonstate & (1 << 3)) && (mouse_oldbuttonstate & (1 << 3)))
	{
		in_state->Key_Event_fp(K_MOUSE4, false);
	}

	if ((mouse_buttonstate & (1 << 4)) && !(mouse_oldbuttonstate & (1 << 4)))
	{
		in_state->Key_Event_fp(K_MOUSE5, true);
	}

	if (!(mouse_buttonstate & (1 << 4)) && (mouse_oldbuttonstate & (1 << 4)))
	{
		in_state->Key_Event_fp(K_MOUSE5, false);
	}

	mouse_oldbuttonstate = mouse_buttonstate;
}

/*
 * Move handling
 */
void
IN_BackendMove(usercmd_t *cmd)
{
	IN_GetMouseState(&mouse_x, &mouse_y, &mouse_buttonstate);

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

		/* add mouse X/Y movement to cmd */
		if ((*in_state->in_strafe_state & 1) ||
			(lookstrafe->value && mlooking))
		{
			cmd->sidemove += m_side->value * mouse_x;
		}
		else
		{
			in_state->viewangles[YAW] -= m_yaw->value * mouse_x;
		}

		if ((mlooking || freelook->value) &&
			!(*in_state->in_strafe_state & 1))
		{
			in_state->viewangles[PITCH] += m_pitch->value * mouse_y;
		}
		else
		{
			cmd->forwardmove -= m_forward->value * mouse_y;
		}

		IN_ClearMouseState();
	}
}
#pragma GCC visibility pop

