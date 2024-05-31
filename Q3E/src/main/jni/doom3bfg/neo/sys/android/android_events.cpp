/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.	If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include "renderer/RenderCommon.h"
#include "android_local.h"
#include "../posix/posix_public.h"
#include "../../framework/Common_local.h"

// DG: those are needed for moving/resizing windows
extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;
// DG end

extern char * Android_GetClipboardData(void);
extern void Android_SetClipboardData(const char *text);
extern void Android_ClearEvents(void);
extern void Android_PollInput(void);
extern int Android_PollEvents(int num);
extern float analogx;
extern float analogy;
extern int analogenabled;

const char* kbdNames[] =
{
	"english", "french", "german", "italian", "spanish", "turkish", "norwegian", NULL
};

idCVar in_keyboard( "in_keyboard", "english", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "keyboard layout", kbdNames, idCmdSystem::ArgCompletion_String<kbdNames> );

struct kbd_poll_t
{
	int key;
	bool state;

	kbd_poll_t()
	{
	}

	kbd_poll_t( int k, bool s )
	{
		key = k;
		state = s;
	}
};

struct mouse_poll_t
{
	int action;
	int value;

	mouse_poll_t()
	{
	}

	mouse_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};

static idList<kbd_poll_t> kbd_polls;
static idList<mouse_poll_t> mouse_polls;

struct joystick_poll_t
{
	int action;
	int value;

	joystick_poll_t()
	{
	}

	joystick_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};
bool buttonStates[K_LAST_KEY];	// For keeping track of button up/down events



/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput()
{
	kbd_polls.SetGranularity( 64 );
	mouse_polls.SetGranularity( 64 );

	memset( buttonStates, 0, sizeof( buttonStates ) );

	// in_keyboard.SetModified();
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput()
{
	kbd_polls.Clear();
	mouse_polls.Clear();

	memset( buttonStates, 0, sizeof( buttonStates ) );
}

/*
===========
Sys_InitScanTable
===========
*/
// Windows has its own version due to the tools
void Sys_InitScanTable()
{
}

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted )
{
	static unsigned char keys[2] = { '`', '~' };

	if( in_keyboard.IsModified() )
	{
		idStr lang = in_keyboard.GetString();

		if( lang.Length() )
		{
			if( !lang.Icmp( "french" ) )
			{
				keys[0] = '<';
				keys[1] = '>';
			}
			else if( !lang.Icmp( "german" ) )
			{
				keys[0] = '^';
				keys[1] = 176; // °
			}
			else if( !lang.Icmp( "italian" ) )
			{
				keys[0] = '\\';
				keys[1] = '|';
			}
			else if( !lang.Icmp( "spanish" ) )
			{
				keys[0] = 186; // º
				keys[1] = 170; // ª
			}
			else if( !lang.Icmp( "turkish" ) )
			{
				keys[0] = '"';
				keys[1] = 233; // é
			}
			else if( !lang.Icmp( "norwegian" ) )
			{
				keys[0] = 124; // |
				keys[1] = 167; // §
			}
		}

		in_keyboard.ClearModified();
	}

	return shifted ? keys[1] : keys[0];
}

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int key )
{
	return key & 0xff;
}

/*
===============
Sys_GrabMouseCursor
===============
*/
void Sys_GrabMouseCursor( bool grabIt )
{
	int flags;

	if( grabIt )
	{
		// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled
		flags = GRAB_ENABLE | GRAB_SETSTATE;
		// DG end
	}
	else
	{
		flags = GRAB_SETSTATE;
	}
// SRS - Generalized Vulkan SDL platform
#if defined(VULKAN_USE_PLATFORM_SDL)
	VKimp_GrabInput( flags );
#else
	GLimp_GrabInput( flags );
#endif
}

/*
================
Sys_GetEvent
================
*/
enum
{
	Q3E_EVENT_NONE = 0,
	Q3E_EVENT_KEY = 1,
	Q3E_EVENT_MOUSE = 2,
};
struct Q3E_Event_s
{
	int type; // 1 key; 2 mouse
	int state;
	int key;
	int character;
	float dx;
	float dy;
};
static Q3E_Event_s q3e_event;
#define Q3E_KEY_EVENT(state, key, character) { q3e_event.type = Q3E_EVENT_KEY; q3e_event.state = state; q3e_event.key = key; q3e_event.character = character; }
#define Q3E_MOUSE_EVENT(dx, dy) { q3e_event.type = Q3E_EVENT_MOUSE; q3e_event.dx = dx; q3e_event.dy = dy; }
#define Q3E_NONE_EVENT() { q3e_event.type = Q3E_EVENT_NONE; }
#define Q3E_HAS_EVENT() (q3e_event.type != Q3E_EVENT_NONE)
sysEvent_t Sys_GetEvent()
{
	sysEvent_t res = { };

	// when this is returned, it's assumed that there are no more events!
	static const sysEvent_t no_more_events = { SE_NONE, 0, 0, 0, NULL };
	static int32 uniChar = 0;

	if( uniChar )
	{
		res.evType = SE_CHAR;
		res.evValue = uniChar;

		uniChar = 0;

		return res;
	}

	Q3E_NONE_EVENT();
	while(Android_PollEvents(1) > 0)
	{
		switch(q3e_event.type)
		{
			case Q3E_EVENT_KEY:
				if(q3e_event.character)
				{
					if(q3e_event.state)
					{
						uniChar = q3e_event.character;
					}
				}

				{
					const char a = Sys_GetConsoleKey( false );
					const char b = Sys_GetConsoleKey( true );
					if( q3e_event.character == a || q3e_event.character == b
						|| q3e_event.key == a || q3e_event.key == b
					)
						q3e_event.key = K_GRAVE;

					res.evType = SE_KEY;
					res.evValue = q3e_event.key;
					res.evValue2 = q3e_event.state ? 1 : 0;
					kbd_polls.Append( kbd_poll_t( q3e_event.key, q3e_event.state ) );
					return res;
				}
				break;

			case Q3E_EVENT_MOUSE:
				res.evType = SE_MOUSE;
				res.evValue = q3e_event.dx;
				res.evValue2 = q3e_event.dy;
				mouse_polls.Append( mouse_poll_t( M_DELTAX, q3e_event.dx ) );
				mouse_polls.Append( mouse_poll_t( M_DELTAY, q3e_event.dy ) );
				return res;
				break;
			default:
				break;
		}

		Q3E_NONE_EVENT();
	}
	Q3E_NONE_EVENT();

	// Android_PollInput();

	res = no_more_events;
	return res;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents()
{
	Android_ClearEvents();

	kbd_polls.SetNum( 0 );
	mouse_polls.SetNum( 0 );
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents()
{
#if 0
	char* s = Posix_ConsoleInput();

	if( s )
	{
		PushConsoleEvent( s );
	}

	SDL_PumpEvents();
#endif
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents()
{
	return kbd_polls.Num();
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int& key, bool& state )
{
	if( n >= kbd_polls.Num() )
	{
		return 0;
	}

	key = kbd_polls[n].key;
	state = kbd_polls[n].state;
	return 1;
}

/*
================
Sys_EndKeyboardInputEvents
================
*/
void Sys_EndKeyboardInputEvents()
{
	kbd_polls.SetNum( 0 );
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] )
{
	int numEvents = mouse_polls.Num();

	if( numEvents > MAX_MOUSE_EVENTS )
	{
		numEvents = MAX_MOUSE_EVENTS;
	}

	for( int i = 0; i < numEvents; i++ )
	{
		const mouse_poll_t& mp = mouse_polls[i];

		mouseEvents[i][0] = mp.action;
		mouseEvents[i][1] = mp.value;
	}

	mouse_polls.SetNum( 0 );

	return numEvents;
}

const char* Sys_GetKeyName( keyNum_t keynum )
{
	return NULL;
}

char* Sys_GetClipboardData()
{
	return Android_GetClipboardData();
}

void Sys_SetClipboardData( const char* string )
{
	Android_SetClipboardData(string);
}


//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

void Sys_SetRumble( int device, int low, int hi )
{
	// TODO;
	// SDL 2.0 required (SDL Haptic subsystem)
}

int Sys_PollJoystickInputEvents( int deviceNum )
{
	return 0;
}

// This funcion called by void idUsercmdGenLocal::Joystick( int deviceNum ) in
// file UsercmdGen.cpp
// action - must have values belonging to enum sys_jEvents (sys_public.h)
// value - must be 1/0 for button or DPAD pressed/released
//         for joystick axes must be in the range (-32769, 32768)
//         for joystick trigger must be in the range (0, 32768)
int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{
	return 0;
}

// This funcion called by void idUsercmdGenLocal::Joystick( int deviceNum ) in
// file UsercmdGen.cpp
void Sys_EndJoystickInputEvents()
{
}

void Q3E_KeyEvent(int state,int key,int character)
{
	Q3E_KEY_EVENT(state, key, character);
}

void Q3E_MotionEvent(float dx, float dy)
{
	Q3E_MOUSE_EVENT(dx, dy);
}

void Sys_Analog(int &side, int &forward, const int &KEY_MOVESPEED)
{
	if (analogenabled)
	{
		side = (int)(KEY_MOVESPEED * analogx);
		forward = (int)(KEY_MOVESPEED * analogy);
	}
}

