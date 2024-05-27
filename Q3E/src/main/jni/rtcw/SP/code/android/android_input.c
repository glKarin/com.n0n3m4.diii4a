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
#include "../renderer/tr_local.h"
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

#if 0
void Com_PushEvent( sysEvent_t *event );

void Com_QueueEvent(int evTime,
					sysEventType_t evType,
					int evValue, int evValue2,
					int evPtrLength,
					void *evPtr)
{
	sysEvent_t ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime=evTime;
	ev.evType=evType;
	ev.evValue=evValue;
	ev.evValue2=evValue2;
	ev.evPtrLength=evPtrLength;
	ev.evPtr=evPtr;
	Com_PushEvent(&ev);
}
#endif

void Q3E_KeyEvent(int state,int key,int character)
{
	int t = Sys_Milliseconds();
	if (key!=0)
	{
		Com_QueueEvent(t, SE_KEY, key, state, 0, NULL);
		//Com_DPrintf("SE_KEY key=%d state=%d\n", key, state);
	}
	if ((state==1)&&(character!=0))
	{
		//Com_DPrintf("SE_CHAR key=%d state=%d\n", character, state);
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
		int t = Sys_Milliseconds();
		Com_QueueEvent(t, SE_MOUSE, (int)(motion_dx), (int)(motion_dy), 0, NULL);
		motion_event = qfalse;
		motion_dx = 0;
		motion_dy = 0;
	}
}

void queueTrackballEvent(int action, float dx, float dy)
{
	int t = Sys_Milliseconds();
	static int keyPress=0;

	switch(action)
	{
		case ACTION_DOWN:
			Com_QueueEvent(t, SE_KEY, K_MOUSE1, 1, 0, NULL);
			keyPress=1;
			break;
		case ACTION_UP:
			if(keyPress)
				Com_QueueEvent(t, SE_KEY, K_MOUSE1, 0, 0, NULL);
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
		int t = Sys_Milliseconds(); //what time should we use?

		/* Trackball dx/dy are <1.0, so make them a bit bigger to prevent kilometers of scrolling */
		trackball_dx *= 50.0;
		trackball_dy *= 50.0;
		cursor_x += trackball_dx;
		cursor_y += trackball_dy;

		cursor_x = clamp_to_screen_width(cursor_x);
		cursor_y = clamp_to_screen_height(cursor_y);

		Com_QueueEvent(t, SE_MOUSE, (int)trackball_dx, (int)trackball_dy, 0, NULL);

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