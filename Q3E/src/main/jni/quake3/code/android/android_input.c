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

static cvar_t *in_keyboardDebug     = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

static cvar_t *in_joystick          = NULL;
static cvar_t *in_joystickThreshold = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;

static int vidRestartTime = 0;

static int in_eventTime = 0;

#define CTRL(a) ((a)-'a'+1)

extern void Android_GrabMouseCursor(qboolean grabIt);
extern void Android_PollInput(void);
extern void Sys_SyncState(void);

#define IN_IsConsoleKey( key, character ) ( (key) == '`' || (character) == '`' )

void Q3E_KeyEvent(int state,int key,int character)
{
	static keyNum_t lastKeyDown = 0;
	if( IN_IsConsoleKey( key, character ) )
	{
		// Console keys can't be bound or generate characters
		key = K_CONSOLE;
	}
	if(state)
	{
		if (key!=0)
		{
			Com_QueueEvent(in_eventTime, SE_KEY, key, qtrue, 0, NULL);
			if (key == K_BACKSPACE)
				Com_QueueEvent( in_eventTime, SE_CHAR, CTRL('h'), 0, 0, NULL );
			else if (keys[K_CTRL].down && key >= 'a' && key <= 'z')
				Com_QueueEvent( in_eventTime, SE_CHAR, CTRL(key), 0, 0, NULL );
			else if (character != 0 && character != '`')
			{
				//Com_DPrintf("SE_CHAR key=%d state=%d\n", character, state);
				Com_QueueEvent(in_eventTime, SE_CHAR, character, 0, 0, NULL);
			}
		}
		lastKeyDown = key;
	}
	else
	{
		if (key!=0)
		{
			Com_QueueEvent(in_eventTime, SE_KEY, key, qfalse, 0, NULL);
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
			Com_QueueEvent(in_eventTime, SE_MOUSE, dx, dy, 0, NULL);
		}
	}
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( qboolean isFullscreen )
{
	if (!mouseAvailable)
		return;

	if (!mouseActive)
	{
		Android_GrabMouseCursor( 1 );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !isFullscreen )
	{
		if (in_nograb->modified || !mouseActive)
		{
			if (in_nograb->integer)
			{
				Android_GrabMouseCursor( 0 );
			}
			else
			{
				Android_GrabMouseCursor( 1 );
			}

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( qboolean isFullscreen )
{
	if( !mouseAvailable )
		return;

	if (mouseActive)
	{
		Android_GrabMouseCursor( 0 );

		mouseActive = qfalse;
	}
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( void )
{
	Android_PollInput();

	Sys_SyncState();
}

void IN_Frame(void)
{
	qboolean loading;

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = ( clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE );

	// update isFullscreen since it might of changed since the last vid_restart
	cls.glconfig.isFullscreen = Cvar_VariableIntegerValue( "r_fullscreen" ) != 0;

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( cls.glconfig.isFullscreen );
	}
	else if( !cls.glconfig.isFullscreen && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( cls.glconfig.isFullscreen );
	}
	else
		IN_ActivateMouse( cls.glconfig.isFullscreen );

	IN_ProcessEvents( );

	// Set event time for next frame to earliest possible time an event could happen
	in_eventTime = Sys_Milliseconds( );

	// In case we had to delay actual restart of video system
	if( ( vidRestartTime != 0 ) && ( vidRestartTime < Sys_Milliseconds( ) ) )
	{
		vidRestartTime = 0;
		Cbuf_AddText( "vid_restart\n" );
	}
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	int appState;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE );

	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_LATCH );
	in_joystickThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );

	mouseAvailable = ( in_mouse->value != 0 );
	IN_DeactivateMouse( Cvar_VariableIntegerValue( "r_fullscreen" ) != 0 );

	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse( Cvar_VariableIntegerValue( "r_fullscreen" ) != 0 );
	mouseAvailable = qfalse;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
	IN_Init( NULL );
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
