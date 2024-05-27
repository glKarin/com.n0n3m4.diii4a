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

//#include "android_glimp.h"
#include "../client/client.h"
#include "../renderergl1/tr_local.h"
#include "../qcommon/q_shared.h"
#include <android/log.h>

/* These variables are for confining menu input to 640x480 */
static float scale_ratio;
static int offset_x;

/* Shared cursor position between touch and trackball code */
static float cursor_x=0, cursor_y=0;

static qboolean motion_event = qfalse;
static float motion_dx=0;
static float motion_dy=0;

static qboolean trackball_event = qfalse;
static float trackball_dx = 0;
static float trackball_dy = 0;

static qboolean mouseActive = qfalse;
static cvar_t *in_nograb;
extern void Android_GrabMouseCursor(qboolean grabIt);

#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey
===============
*/
static qboolean IN_IsConsoleKey( keyNum_t key, const unsigned char character )
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
			unsigned char character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if( cl_consoleKeys->modified )
	{
		char *text_p, *token;

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

			if( strlen( token ) == 4 )
				charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = CHARACTER;
				c->u.character = (unsigned char)charCode;
			}
			else
			{
				c->type = KEY;
				c->u.key = Key_StringToKeynum( token );

				// 0 isn't a key
				if( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if( key == character )
		key = 0;

	for( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch( c->type )
		{
			case KEY:
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
    int t = Sys_Milliseconds();
    if ((state==1)&&(IN_IsConsoleKey(key,character)))
    {
	Com_QueueEvent(t, SE_KEY, K_CONSOLE, state, 0, NULL);
	return;
    }
    if (key!=0)
    {
    Com_QueueEvent(t, SE_KEY, key, state, 0, NULL);
    Com_DPrintf("SE_KEY key=%d state=%d\n", key, state);
    }
    if ((state==1)&&(character!=0))
    {
        Com_DPrintf("SE_CHAR key=%d state=%d\n", character, state);
        Com_QueueEvent(t, SE_CHAR, character, 0, 0, NULL);
    }
}

enum Action
{
	ACTION_DOWN=0,
	ACTION_UP=1,
	ACTION_MOVE=2
};


void Q3E_MotionEvent(float dx, float dy)
{
	motion_event = qtrue;
	motion_dx += dx;
	motion_dy += dy;
}

/* The quake3 menu is 640x480, make sure the coordinates originating from this area are scaled.
 * The UI code already contains clamping to 640x480, so don't perform it here.
 */
inline int scale_x_input(float x)
{
	return (int)((x - offset_x)*scale_ratio);
}

inline int scale_y_input(float y)
{
	return (int)(y*scale_ratio);
}

static void processMotionEvents(void)
{
    if(motion_event)
    {
        Com_QueueEvent(0, SE_MOUSE, (int)(motion_dx), (int)(motion_dy), 0, NULL);
        motion_event = qfalse;
        motion_dx = 0;
        motion_dy = 0;
    }
}

void queueTrackballEvent(int action, float dx, float dy)
{
    static int keyPress=0;

    switch(action)
    {
        case ACTION_DOWN:
            Com_QueueEvent(0, SE_KEY, K_MOUSE1, 1, 0, NULL);
            keyPress=1;
            break;
        case ACTION_UP:
            if(keyPress) Com_QueueEvent(0, SE_KEY, K_MOUSE1, 0, 0, NULL);
            keyPress=0;
    }

    trackball_dx += dx;
    trackball_dy += dy;
    trackball_event = qtrue;
}

inline float clamp_to_screen_width(float x)
{
	if(x > SCREEN_WIDTH)
		return SCREEN_WIDTH;
	else if(x < 0)
		return 0;
	return x;
}

inline float clamp_to_screen_height(float y)
{
	if(y > SCREEN_HEIGHT)
		return SCREEN_HEIGHT;
	else if(y < 0)
		return 0;
	return y;
}

static void processTrackballEvents(void)
{
    if(trackball_event)
    {
        /* Trackball dx/dy are <1.0, so make them a bit bigger to prevent kilometers of scrolling */
        trackball_dx *= 50.0;
        trackball_dy *= 50.0;
        Com_QueueEvent(0, SE_MOUSE, (int)trackball_dx, (int)trackball_dy, 0, NULL);

        trackball_event = qfalse;
        trackball_dx = 0;
        trackball_dy = 0;
    }
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if(!in_nograb)
	{
		Android_GrabMouseCursor( qtrue );
	}
	else if( in_nograb->modified || !mouseActive )
	{
		if( in_nograb->integer ) {
			Android_GrabMouseCursor( qfalse );
		} else {
			Android_GrabMouseCursor( qtrue );
		}

		in_nograb->modified = qfalse;
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
	if( mouseActive )
	{
		Android_GrabMouseCursor( qfalse );

		mouseActive = qfalse;
	}
}

extern void Android_PollInput(void);
extern void Sys_SyncState(void);
void IN_Frame(void)
{
	if( ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
		IN_DeactivateMouse();
	else
		IN_ActivateMouse();

	Android_PollInput();

	processMotionEvents();
	processTrackballEvents();

	Sys_SyncState();
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	Com_DPrintf( "\n------- Input Initialization -------\n" );
	scale_ratio = (float)SCREEN_HEIGHT/glConfig.vidHeight;
	offset_x = (glConfig.vidWidth - ((float)SCREEN_WIDTH/scale_ratio))/2;
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE );
	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	Com_DPrintf( "\n------- Input Shutdown -------\n" );
	IN_DeactivateMouse();
	Com_DPrintf( "------------------------------------\n" );
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
