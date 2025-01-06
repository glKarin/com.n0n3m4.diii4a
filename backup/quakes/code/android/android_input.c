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

#include "../client/client.h"
#include "android_glw.h"

static cvar_t *in_keyboardDebug;
static cvar_t *in_forceCharset;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse;

#define Com_QueueEvent Sys_QueEvent

static cvar_t *cl_consoleKeys;

static int in_eventTime = 0;
static qboolean mouse_focus = qtrue;

extern void Android_GrabMouseCursor(qboolean grabIt);
extern void Android_PollInput(void);
extern void Sys_SyncState(void);

#define CTRL(a) ((a)-'a'+1)


#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static qboolean IN_IsConsoleKey( keyNum_t key, int character )
{
	typedef struct consoleKey_s
	{
		enum
		{
			QUAKE_KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if ( cl_consoleKeys->modified )
	{
		const char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		while( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			token = COM_Parse( &text_p );
			if( !token[ 0 ] )
				break;

			charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = QUAKE_KEY;
				c->u.key = Key_StringToKeynum( token );

				// 0 isn't a key
				if ( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if ( key == character )
		key = 0;

	for ( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch ( c->type )
		{
			case QUAKE_KEY:
				if( key && c->u.key == key )
					return qtrue;
				break;

			case CHARACTER:
				if( c->u.character == character )
					return qtrue;
				break;
		}
	}

	return qfalse;
}

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
			else if ( key == K_ESCAPE )
				Com_QueueEvent( in_eventTime, SE_CHAR, key, 0, 0, NULL );
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


//#define DEBUG_EVENTS

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if ( !mouseAvailable )
		return;

	if ( !mouseActive )
	{
		Android_GrabMouseCursor( 1 );
#ifdef DEBUG_EVENTS
		Com_Printf( "%4i %s\n", Sys_Milliseconds(), __func__ );
#endif
	}

	// in_nograb makes no sense in fullscreen mode
	if ( !glw_state.isFullscreen )
	{
		if ( in_nograb->modified || !mouseActive )
		{
			if ( in_nograb->integer ) {
				Android_GrabMouseCursor( 0 );
			} else {
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
static void IN_DeactivateMouse( void )
{
	if ( !mouseAvailable )
		return;

	if ( mouseActive )
	{
#ifdef DEBUG_EVENTS
		Com_Printf( "%4i %s\n", Sys_Milliseconds(), __func__ );
#endif
		Android_GrabMouseCursor( 0 );

		mouseActive = qfalse;
	}
}



#ifdef DEBUG_EVENTS
static const char *eventName( SDL_WindowEventID event )
{
	static char buf[32];

	switch ( event )
	{
		case SDL_WINDOWEVENT_NONE: return "NONE";
		case SDL_WINDOWEVENT_SHOWN: return "SHOWN";
		case SDL_WINDOWEVENT_HIDDEN: return "HIDDEN";
		case SDL_WINDOWEVENT_EXPOSED: return "EXPOSED";
		case SDL_WINDOWEVENT_MOVED: return "MOVED";
		case SDL_WINDOWEVENT_RESIZED: return "RESIZED";
		case SDL_WINDOWEVENT_SIZE_CHANGED: return "SIZE_CHANGED";
		case SDL_WINDOWEVENT_MINIMIZED: return "MINIMIZED";
		case SDL_WINDOWEVENT_MAXIMIZED: return "MAXIMIZED";
		case SDL_WINDOWEVENT_RESTORED: return "RESTORED";
		case SDL_WINDOWEVENT_ENTER: return "ENTER";
		case SDL_WINDOWEVENT_LEAVE: return "LEAVE";
		case SDL_WINDOWEVENT_FOCUS_GAINED: return "FOCUS_GAINED";
		case SDL_WINDOWEVENT_FOCUS_LOST: return "FOCUS_LOST";
		case SDL_WINDOWEVENT_CLOSE: return "CLOSE";
		case SDL_WINDOWEVENT_TAKE_FOCUS: return "TAKE_FOCUS";
		case SDL_WINDOWEVENT_HIT_TEST: return "HIT_TEST"; 
		default:
			sprintf( buf, "EVENT#%i", event );
			return buf;
	}
}
#endif


/*
===============
HandleEvents
===============
*/

//static void IN_ProcessEvents( void )
void HandleEvents( void )
{
	in_eventTime = Sys_Milliseconds();

	Android_PollInput();

	Sys_SyncState();
}


/*
===============
IN_Minimize

Minimize the game so that user is back at the desktop
===============
*/
static void IN_Minimize( void )
{
}


/*
===============
IN_Frame
===============
*/
void IN_Frame( void )
{
#ifdef USE_JOYSTICK
	IN_JoyMove();
#endif

	if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
		// temporarily deactivate if not in the game and
		// running on the desktop with multimonitor configuration
		if ( !glw_state.isFullscreen || glw_state.monitorCount > 1 ) {
			IN_DeactivateMouse();
			return;
		}
	}

	if ( !gw_active || !mouse_focus || in_nograb->integer ) {
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();

	//IN_ProcessEvents();
	//HandleEvents();

	// Set event time for next frame to earliest possible time an event could happen
	//in_eventTime = Sys_Milliseconds();
}


/*
===============
IN_Restart
===============
*/
static void IN_Restart( void )
{
#ifdef USE_JOYSTICK
	IN_ShutdownJoystick();
#endif
	IN_Shutdown();
	IN_Init();
}


/*
===============
IN_Init
===============
*/
void IN_Init( void )
{
	Com_DPrintf( "\n------- Input Initialization -------\n" );

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE );
	Cvar_SetDescription( in_keyboardDebug, "Print keyboard debug info." );
	in_forceCharset = Cvar_Get( "in_forceCharset", "1", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( in_forceCharset, "Try to translate non-ASCII chars in keyboard input or force EN/US keyboard layout." );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	Cvar_CheckRange( in_mouse, "-1", "1", CV_INTEGER );
	Cvar_SetDescription( in_mouse,
		"Mouse data input source:\n" \
		"  0 - disable mouse input\n" \
		"  1 - di/raw mouse\n" \
		" -1 - win32 mouse" );

#ifdef USE_JOYSTICK
	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH );
	Cvar_SetDescription( in_joystick, "Whether or not joystick support is on." );
	in_joystickThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );
	Cvar_SetDescription( in_joystickThreshold, "Threshold of joystick moving distance." );

	j_pitch =        Cvar_Get( "j_pitch",        "0.022", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( j_pitch, "Joystick pitch rotation speed/direction." );
	j_yaw =          Cvar_Get( "j_yaw",          "-0.022", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( j_yaw, "Joystick yaw rotation speed/direction." );
	j_forward =      Cvar_Get( "j_forward",      "-0.25", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( j_forward, "Joystick forward movement speed/direction." );
	j_side =         Cvar_Get( "j_side",         "0.25", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( j_side, "Joystick side movement speed/direction." );
	j_up =           Cvar_Get( "j_up",           "0", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( j_up, "Joystick up movement speed/direction." );

	j_pitch_axis =   Cvar_Get( "j_pitch_axis",   "3", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( j_pitch_axis,   "0", va("%i",MAX_JOYSTICK_AXIS-1), CV_INTEGER );
	Cvar_SetDescription( j_pitch_axis, "Selects which joystick axis controls pitch." );
	j_yaw_axis =     Cvar_Get( "j_yaw_axis",     "2", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( j_yaw_axis,     "0", va("%i",MAX_JOYSTICK_AXIS-1), CV_INTEGER );
	Cvar_SetDescription( j_yaw_axis, "Selects which joystick axis controls yaw." );
	j_forward_axis = Cvar_Get( "j_forward_axis", "1", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( j_forward_axis, "0", va("%i",MAX_JOYSTICK_AXIS-1), CV_INTEGER );
	Cvar_SetDescription( j_forward_axis, "Selects which joystick axis controls forward/back." );
	j_side_axis =    Cvar_Get( "j_side_axis",    "0", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( j_side_axis,    "0", va("%i",MAX_JOYSTICK_AXIS-1), CV_INTEGER );
	Cvar_SetDescription( j_side_axis, "Selects which joystick axis controls left/right." );
	j_up_axis =      Cvar_Get( "j_up_axis",      "4", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( j_up_axis,      "0", va("%i",MAX_JOYSTICK_AXIS-1), CV_INTEGER );
	Cvar_SetDescription( j_up_axis, "Selects which joystick axis controls up/down." );
#endif

	// ~ and `, as keys and characters
	cl_consoleKeys = Cvar_Get( "cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE );
	Cvar_SetDescription( cl_consoleKeys, "Space delimited list of key names or characters that toggle the console." );

	mouseAvailable = ( in_mouse->value != 0 ) ? qtrue : qfalse;

	//IN_DeactivateMouse();

#ifdef USE_JOYSTICK
	IN_InitJoystick();
#endif

	Cmd_AddCommand( "minimize", IN_Minimize );
	Cmd_AddCommand( "in_restart", IN_Restart );

	Com_DPrintf( "------------------------------------\n" );
}


/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse();

	mouseAvailable = qfalse;

#ifdef USE_JOYSTICK
	IN_ShutdownJoystick();
#endif

	Cmd_RemoveCommand( "minimize" );
	Cmd_RemoveCommand( "in_restart" );
}
