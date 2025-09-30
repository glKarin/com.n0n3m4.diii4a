/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../client/client.h"
#include "../sys/sys_local.h"
#include "../qcommon/q_shared.h"

static qboolean mouse_avail = qfalse;
static qboolean mouse_active = qfalse;

static cvar_t *in_mouse;
static cvar_t *in_disablemacosxmouseaccel;
cvar_t *in_subframe;
cvar_t *in_nograb; // this is strictly for developers

cvar_t   *in_joystick      = NULL;
cvar_t   *in_joystickDebug = NULL;
cvar_t   *joy_threshold    = NULL;

static int vidRestartTime = 0;

static int in_eventTime = 0;

#define CTRL(a) ((a)-'a'+1)

extern void Android_GrabMouseCursor(qboolean grabIt);
extern void Android_PollInput(void);
extern void Sys_SyncState(void);
extern void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

#define IN_IsConsoleKey( key, character ) ( (key) == '`' || (character) == '`' )

void Q3E_KeyEvent(int state,int key,int character)
{
	const int t = 0;  // always just use the current time.
/*	if( IN_IsConsoleKey( key, character ) )
	{
		// Console keys can't be bound or generate characters
		key = K_CONSOLE;
	}*/
	if(state)
	{
		if (key!=0)
		{
			Sys_QueEvent( t, SE_KEY, key, qtrue, 0, NULL );
			if (character != 0)
			{
				Sys_QueEvent( t, SE_CHAR, character, 0, 0, NULL );
			}
		}
	}
	else
	{
		if (key!=0)
		{
			Sys_QueEvent( t, SE_KEY, key, qfalse, 0, NULL );
        }
	}
}

void Q3E_MotionEvent(float dx, float dy)
{
    const int t = 0;  // always just use the current time.
	if (mouse_active)
	{
		if (dx != 0.0 || dy != 0.0)
		{
            Sys_QueEvent( t, SE_MOUSE, dx, dy, 0, NULL );
		}
	}
}

static void install_grabs(void)
{
    Android_GrabMouseCursor( 1 );
}

static void uninstall_grabs(void)
{
    Android_GrabMouseCursor( 0 );
}

/*
===============
IN_ActivateMouse
===============
*/
void IN_ActivateMouse( void )
{
	if (!mouse_avail)
		return;

    if (!mouse_active)
    {
        if (!in_nograb->value)
            install_grabs();
        mouse_active = qtrue;
    }
}

/*
===============
IN_DeactivateMouse
===============
*/
void IN_DeactivateMouse( void )
{
	if( !mouse_avail )
		return;

    if (mouse_active)
    {
        if (!in_nograb->value)
            uninstall_grabs();
        mouse_active = qfalse;
    }
}

void IN_Frame(void)
{
	// bk001130 - from cvs 1.17 (mkv)
	// IN_JoyMove(); // FIXME: disable if on desktop?

	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		// temporarily deactivate if not in the game and
		// running on the desktop
		// voodoo always counts as full screen
		if (Cvar_VariableValue ("r_fullscreen") == 0
			&& strcmp( Cvar_VariableString("r_glDriver"), _3DFX_DRIVER_NAME ) )
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse();
}

void IN_Activate(void)
{
}

// bk001130 - cvs1.17 joystick code (mkv) was here, no linux_joystick.c

void Sys_SendKeyEvents (void) {
	Android_PollInput();

	Sys_SyncState();
	//HandleEvents();
}

/*
===============
IN_Init
===============
*/
void IN_Init(void)
{
	Com_DPrintf ("\n------- Input Initialization -------\n");
	// mouse variables
	in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	in_disablemacosxmouseaccel = Cvar_Get ("in_disablemacosxmouseaccel", "0", CVAR_ARCHIVE);

	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get ("in_subframe", "1", CVAR_ARCHIVE);

	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get ("in_nograb", "0", 0);

	// bk001130 - from cvs.17 (mkv), joystick variables
	in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH);
	// bk001130 - changed this to match win32
	in_joystickDebug = Cvar_Get ("in_debugjoystick", "0", CVAR_TEMP);
	joy_threshold = Cvar_Get ("joy_threshold", "0.15", CVAR_ARCHIVE); // FIXME: in_joythreshold

	Cvar_Set( "cl_platformSensitivity", "2.0" );

	if (in_mouse->value)
		mouse_avail = qtrue;
	else
		mouse_avail = qfalse;

	//IN_StartupJoystick( ); // bk001130 - from cvs1.17 (mkv)
	Com_DPrintf ("------------------------------------\n");
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse();

	mouse_avail = qfalse;

/*	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);*/
}

extern float analogx;
extern float analogy;
extern int analogenabled;
extern kbutton_t	in_moveleft, in_moveright, in_forward, in_back;
void IN_Analog(const kbutton_t *key, float *val)
{
	if (analogenabled)
	{
		if (key==&in_moveright)
			*val = fmax(0,analogx);
		else if (key==&in_moveleft)
			*val = fmax(0,-analogx);
		else if (key==&in_forward)
			*val = fmax(0,analogy);
		else if (key==&in_back)
			*val = fmax(0,-analogy);
	}
}

/*
===============
IN_SyncMousePosition
===============
*/
void IN_SyncMousePosition( void ) {
	// set UI's menu cursor position to 0,0
/*	Com_QueueEvent( 0, SE_MOUSE, -10000, -10000, 0, NULL );

	mouse640X = 0;
	mouse640Y = 0;

	{
		int x, y, xrel, yrel;

		if ( mouseActive ) {
			// ignore the real mouse position if in relative mode
			x = mouseLastX;
			y = mouseLastY;
		} else {
			SDL_GetMouseState( &x, &y );
			mouseLastX = x;
			mouseLastY = y;
		}

		IN_UpdateMouseMenuPosition( x, y, &xrel, &yrel );
		if ( xrel != 0 || yrel != 0 ) {
			Com_QueueEvent( 0, SE_MOUSE, xrel, yrel, 0, NULL );
		}
	}*/
}
