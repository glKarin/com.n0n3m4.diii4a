/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file sdl_input.c
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../client/client.h"
#include "../sys/sys_local.h"

static cvar_t *in_keyboardDebug = NULL;

static cvar_t *in_mouse = NULL;
static cvar_t *in_nograb;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive    = qfalse;

static cvar_t             *in_joystick          = NULL;
static cvar_t             *in_joystickThreshold = NULL;
static cvar_t             *in_joystickNo        = NULL;
static cvar_t             *in_joystickUseAnalog = NULL;

static int vidRestartTime = 0;

// Used for giving the engine better (= unlagged) input timestamps
// We give the engine the time we last polled inputs as the timestamp to the current inputs.
// As long as the input backend doesn't have reliable timestamps, this is the right thing to do!
// The engine simulates *the period from the last frame's beginning to the current frame's beginning* when it simulates the current frame.
// It's not simulating "the next 1/60th or 1/125th" of a second. If you give it "now" as input timestamps, your inputs do not occur until the *next* simulated chunk of time.
// Try it. com_maxfps 1, press "forward", have fun.
static int lasttime = 0; // if 0, Com_QueueEvent will use the current time. This is for the first frame.

extern void Android_GrabMouseCursor(qboolean grabIt);
extern void Android_PollInput(void);
extern char * Android_GetClipboardData(void);
extern void Android_SetClipboardData(const char *text);
extern void Sys_SyncState(void);

void Q3E_KeyEvent(int state,int key,int character)
{
	static keyNum_t lastKeyDown = 0;
	if(state)
	{
		if (key!=0)
		{
			Com_QueueEvent(lasttime, SE_KEY, key, qtrue, 0, NULL);
			if (key == K_BACKSPACE)
			{
				// This was added to keep mod comp, mods do not check K_BACKSPACE but instead use char 8 which is backspace in ascii
				// 8 == CTRL('h') == BS aka Backspace from ascii table
				Com_QueueEvent(lasttime, SE_CHAR, CTRL('h'), 0, 0, NULL);
			}
			else if (key == K_ESCAPE)
			{
				Com_QueueEvent(lasttime, SE_CHAR, key, 0, 0, NULL);
			}
			else if ((keys[K_LCTRL].down || keys[K_RCTRL].down) && key >= 'a' && key <= 'z')
			{
				Com_QueueEvent(lasttime, SE_CHAR, CTRL(key), 0, 0, NULL);
			}
		}
		if ((state==1)&&(character!=0))
		{
			//Com_DPrintf("SE_CHAR key=%d state=%d\n", character, state);
			Com_QueueEvent(lasttime, SE_CHAR, character, 0, 0, NULL);
		}
		lastKeyDown = key;
	}
	else
	{
		if (key!=0)
		{
			Com_QueueEvent(lasttime, SE_KEY, key, qfalse, 0, NULL);
		}
		lastKeyDown = 0;
	}
}

void Q3E_MotionEvent(float dx, float dy)
{
	if (mouseActive)
	{
		if (dx != 0.0 || dy != 0.0)
		{
			Com_QueueEvent(lasttime, SE_MOUSE, dx, dy, 0, NULL);
		}
	}
}

/**
 * @brief IN_GetClipboardData
 * @return
 */
char *IN_GetClipboardData(void)
{
	return Android_GetClipboardData();
}

void IN_SetClipboardData(const char *text)
{
	Android_SetClipboardData(text);
}

/**
 * @brief Check if Num key is lock down
 * @return qtrue if Num key is lock down, qfalse if not
 */
qboolean IN_IsNumLockDown(void)
{
	return qfalse;
}

#define MAX_CONSOLE_KEYS 16

/**
 * @brief IN_IsConsoleKey
 * @param[in] key
 * @param[in] character
 * @return
 */
static qboolean IN_IsConsoleKey(keyNum_t key, int character)
{
	typedef struct consoleKey_s
	{
		enum
		{
			KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[MAX_CONSOLE_KEYS];
	static int          numConsoleKeys = 0;
	int                 i;

	if (key == K_GRAVE
#ifdef __APPLE__
	    || key == 60 // Same as console key
#endif
	    )
	{
		return qtrue;
	}

	// Only parse the variable when it changes
	if (cl_consoleKeys->modified)
	{
		char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p                   = cl_consoleKeys->string;
		numConsoleKeys           = 0;

		while (numConsoleKeys < MAX_CONSOLE_KEYS)
		{
			consoleKey_t *c       = &consoleKeys[numConsoleKeys];
			int          charCode = 0;

			token = COM_Parse(&text_p);
			if (!token[0])
			{
				break;
			}

			if (strlen(token) == 4)
			{
				charCode = Com_HexStrToInt(token);
			}

			if (charCode > 0)
			{
				c->type        = CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type  = KEY;
				c->u.key = Key_StringToKeynum(token);

				// 0 isn't a key
				if (c->u.key <= 0)
				{
					continue;
				}
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if (key == character)
	{
		key = 0;
	}

	for (i = 0; i < numConsoleKeys; i++)
	{
		consoleKey_t *c = &consoleKeys[i];

		switch (c->type)
		{
		case KEY:
			if (key && c->u.key == key)
			{
				return qtrue;
			}
			break;
		case CHARACTER:
			if (c->u.character == character)
			{
				return qtrue;
			}
			break;
		}
	}

	return qfalse;
}

/**
 * @brief IN_GobbleMotionEvents
 */
static void IN_GobbleMotionEvents(void)
{
}

/**
 * @brief IN_GrabMouse
 * @param[in] grab
 * @param[in] relative
 */
static void IN_GrabMouse(qboolean grab, qboolean relative)
{
	static qboolean mouse_grabbed = qfalse, mouse_relative = qfalse;

	if (relative == !mouse_relative)
	{
		mouse_relative = relative;
	}

	if (grab == !mouse_grabbed)
	{
		Android_GrabMouseCursor( grab );
		mouse_grabbed = grab;
	}
}

/**
 * @brief IN_ActivateMouse
 */
static void IN_ActivateMouse(void)
{
	if (!mouseAvailable)
	{
		return;
	}

	if (!mouseActive)
	{
		IN_GrabMouse(qtrue, qtrue);

		IN_GobbleMotionEvents();
	}

	// in_nograb makes no sense in fullscreen mode
	if (!cls.glconfig.isFullscreen)
	{
		if (in_nograb->modified || !mouseActive)
		{
			if (in_nograb->integer)
			{
				IN_GrabMouse(qfalse, qtrue);
			}
			else
			{
				IN_GrabMouse(qtrue, qtrue);
			}

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/**
 * @brief IN_DeactivateMouse
 */
static void IN_DeactivateMouse(void)
{
	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if (!cls.glconfig.isFullscreen)
	{
		IN_GrabMouse(qfalse, qfalse);
	}

	if (!mouseAvailable)
	{
		return;
	}

	if (mouseActive)
	{
		IN_GobbleMotionEvents();
		IN_GrabMouse(qfalse, qfalse);

		mouseActive = qfalse;
	}
}

/**
 * @brief IN_ProcessEvents
 */
static void IN_ProcessEvents(void)
{
	Android_PollInput();

	Sys_SyncState();
}

/**
 * @brief IN_Frame
 */
void IN_Frame(void)
{
	// If not DISCONNECTED (main menu), ACTIVE (in game) or CINEMATIC (playing video), we're loading
	qboolean loading   = (cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE);
	qboolean cinematic = (cls.state == CA_CINEMATIC);

	// Get the timestamp to give the next frame's input events (not the ones we're gathering right now, though)
	int start = Sys_Milliseconds();

	if (!cls.glconfig.isFullscreen && (Key_GetCatcher() & KEYCATCH_CONSOLE))
	{
		// Console is down in windowed mode
		IN_DeactivateMouse();
	}
	else if (!cls.glconfig.isFullscreen && loading && !cinematic)
	{
		// Loading in windowed mode
		IN_DeactivateMouse();
	}
	else if (com_minimized->integer || com_unfocused->integer)
	{
		// Window not got focus
		IN_DeactivateMouse();
	}
	else
	{
		if (loading)
		{
			// Just eat up all the mouse events that are not used anyway
			IN_GobbleMotionEvents();
		}

		IN_ActivateMouse();
	}

	// in case we had to delay actual restart of video system...
	if ((vidRestartTime != 0) && (vidRestartTime < Sys_Milliseconds()))
	{
		vidRestartTime = 0;
		Cbuf_AddText("vid_restart\n");
	}

	IN_ProcessEvents();

	// Store the timestamp for the next frame's input events
	lasttime = start;
}

/**
 * @brief IN_InitKeyLockStates
 */
static void IN_InitKeyLockStates(void)
{
	keys[K_SCROLLOCK].down  = qfalse;
	keys[K_KP_NUMLOCK].down = qfalse;
	keys[K_CAPSLOCK].down   = qfalse;
}

/**
 * @brief IN_Init
 */
void IN_Init(void)
{
	int appState;

	//Com_Printf("\n------- Input Initialization -------\n");

	in_keyboardDebug = Cvar_Get("in_keyboardDebug", "0", CVAR_TEMP);

	// mouse variables
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE | CVAR_LATCH);

	in_nograb = Cvar_Get("in_nograb", "0", CVAR_ARCHIVE);

	in_joystick          = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	in_joystickThreshold = Cvar_Get("in_joystickThreshold", "0.25", CVAR_ARCHIVE_ND);

	mouseAvailable = (in_mouse->value != 0.f);
	IN_DeactivateMouse();

	IN_InitKeyLockStates();

	//Com_Printf("------------------------------------\n");
}

/**
 * @brief IN_Shutdown
 */
void IN_Shutdown(void)
{
	Com_Printf("SDL input devices shut down.\n");

	IN_DeactivateMouse();
	mouseAvailable = qfalse;
}

/**
 * @brief IN_Restart
 */
void IN_Restart(void)
{
	IN_Init();
}

extern float analogx;
extern float analogy;
extern int analogenabled;
void IN_Analog(const kbutton_t kb[], const kbutton_t *key, float *val)
{
	if (analogenabled)
	{
		if (key==&kb[KB_MOVERIGHT])
			*val = fmax(0,analogx);
		else if (key==&kb[KB_MOVELEFT])
			*val = fmax(0,-analogx);
		else if (key==&kb[KB_FORWARD])
			*val = fmax(0,analogy);
		else if (key==&kb[KB_BACK])
			*val = fmax(0,-analogy);
	}
}
