/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "qcommon/qcommon.h"
#include "qcommon/q_shared.h"
#include "client/client.h"
#include "sys/sys_local.h"

static cvar_t *in_keyboardDebug     = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

cvar_t *in_joystick          		= NULL;
static cvar_t *in_joystickThreshold = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;

#define CTRL(a) ((a)-'a'+1)

#define MAX_CONSOLE_KEYS 16

extern void Android_GrabMouseCursor(qboolean grabIt);
extern void Android_PollInput(void);
extern void Sys_SyncState(void);

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
#define IN_IsConsoleKey( key, character ) ( (key) == '`' || (character) == '`' )

void Q3E_KeyEvent(int state,int key,int character)
{
	static fakeAscii_t lastKeyDown = A_NULL;
	if( IN_IsConsoleKey( key, character ) )
	{
		// Console keys can't be bound or generate characters
		key = A_CONSOLE;
	}
	if(state)
	{
		if (key!=0)
		{
			Sys_QueEvent(0, SE_KEY, key, qtrue, 0, NULL);
			if (key == A_BACKSPACE)
				Sys_QueEvent( 0, SE_CHAR, CTRL('h'), qfalse, 0, NULL );
			else if (kg.keys[A_CTRL].down && key >= 'a' && key <= 'z')
				Sys_QueEvent( 0, SE_CHAR, CTRL(tolower(key)), qfalse, 0, NULL );
			else if (character != 0 && character != '`')
			{
				//Com_DPrintf("SE_CHAR key=%d state=%d\n", character, state);
				Sys_QueEvent(0, SE_CHAR, character, qfalse, 0, NULL);
			}
		}
		lastKeyDown = (fakeAscii_t)key;
	}
	else
	{
		if (key!=0)
		{
			Sys_QueEvent(0, SE_KEY, key, qfalse, 0, NULL);
		}
		lastKeyDown = A_NULL;
	}
}

void Q3E_MotionEvent(float dx, float dy)
{
	if (mouseActive)
	{
		if (dx != 0.0 || dy != 0.0)
		{
			Sys_QueEvent(0, SE_MOUSE, dx, dy, 0, NULL);
		}
	}
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
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if (!mouseAvailable )
		return;

	if( !mouseActive )
	{
		Android_GrabMouseCursor( qtrue );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !Cvar_VariableIntegerValue("r_fullscreen") )
	{
		if( in_nograb->modified || !mouseActive )
		{
			if( in_nograb->integer )
				Android_GrabMouseCursor( qfalse );
			else
				Android_GrabMouseCursor( qtrue );

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
	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
		Android_GrabMouseCursor( qfalse );

		mouseActive = qfalse;
	}
}

void IN_Init( void *windowData )
{
	Com_DPrintf( "\n------- Input Initialization -------\n" );

	// joystick variables
	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE_ND );

	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE_ND|CVAR_LATCH );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE_ND );

	mouseAvailable = (qboolean)( in_mouse->value != 0 );
	IN_DeactivateMouse( );

	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_ProcessEvents
===============
*/
void SNDDMA_Activate( qboolean activate );
static void IN_ProcessEvents( void )
{
	Android_PollInput();

	Sys_SyncState();
}


void IN_Frame (void) {
	qboolean loading;

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = (qboolean)( cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE );

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !cls.glconfig.isFullscreen && loading && !cls.cursorActive )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( );
	}
	else
		IN_ActivateMouse( );

	IN_ProcessEvents( );
}

void IN_Shutdown( void ) {
	IN_DeactivateMouse( );
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

uint8_t ConvertUTF32ToExpectedCharset( uint32_t utf32 )
{
	switch ( utf32 )
	{
		// Cyrillic characters - mapped to Windows-1251 encoding
		case 0x0410: return 192;
		case 0x0411: return 193;
		case 0x0412: return 194;
		case 0x0413: return 195;
		case 0x0414: return 196;
		case 0x0415: return 197;
		case 0x0416: return 198;
		case 0x0417: return 199;
		case 0x0418: return 200;
		case 0x0419: return 201;
		case 0x041A: return 202;
		case 0x041B: return 203;
		case 0x041C: return 204;
		case 0x041D: return 205;
		case 0x041E: return 206;
		case 0x041F: return 207;
		case 0x0420: return 208;
		case 0x0421: return 209;
		case 0x0422: return 210;
		case 0x0423: return 211;
		case 0x0424: return 212;
		case 0x0425: return 213;
		case 0x0426: return 214;
		case 0x0427: return 215;
		case 0x0428: return 216;
		case 0x0429: return 217;
		case 0x042A: return 218;
		case 0x042B: return 219;
		case 0x042C: return 220;
		case 0x042D: return 221;
		case 0x042E: return 222;
		case 0x042F: return 223;
		case 0x0430: return 224;
		case 0x0431: return 225;
		case 0x0432: return 226;
		case 0x0433: return 227;
		case 0x0434: return 228;
		case 0x0435: return 229;
		case 0x0436: return 230;
		case 0x0437: return 231;
		case 0x0438: return 232;
		case 0x0439: return 233;
		case 0x043A: return 234;
		case 0x043B: return 235;
		case 0x043C: return 236;
		case 0x043D: return 237;
		case 0x043E: return 238;
		case 0x043F: return 239;
		case 0x0440: return 240;
		case 0x0441: return 241;
		case 0x0442: return 242;
		case 0x0443: return 243;
		case 0x0444: return 244;
		case 0x0445: return 245;
		case 0x0446: return 246;
		case 0x0447: return 247;
		case 0x0448: return 248;
		case 0x0449: return 249;
		case 0x044A: return 250;
		case 0x044B: return 251;
		case 0x044C: return 252;
		case 0x044D: return 253;
		case 0x044E: return 254;
		case 0x044F: return 255;

		// Eastern european characters - polish, czech, etc use Windows-1250 encoding
		case 0x0160: return 138;
		case 0x015A: return 140;
		case 0x0164: return 141;
		case 0x017D: return 142;
		case 0x0179: return 143;
		case 0x0161: return 154;
		case 0x015B: return 156;
		case 0x0165: return 157;
		case 0x017E: return 158;
		case 0x017A: return 159;
		case 0x0141: return 163;
		case 0x0104: return 165;
		case 0x015E: return 170;
		case 0x017B: return 175;
		case 0x0142: return 179;
		case 0x0105: return 185;
		case 0x015F: return 186;
		case 0x013D: return 188;
		case 0x013E: return 190;
		case 0x017C: return 191;
		case 0x0154: return 192;
		case 0x00C1: return 193;
		case 0x00C2: return 194;
		case 0x0102: return 195;
		case 0x00C4: return 196;
		case 0x0139: return 197;
		case 0x0106: return 198;
		case 0x00C7: return 199;
		case 0x010C: return 200;
		case 0x00C9: return 201;
		case 0x0118: return 202;
		case 0x00CB: return 203;
		case 0x011A: return 204;
		case 0x00CD: return 205;
		case 0x00CE: return 206;
		case 0x010E: return 207;
		case 0x0110: return 208;
		case 0x0143: return 209;
		case 0x0147: return 210;
		case 0x00D3: return 211;
		case 0x00D4: return 212;
		case 0x0150: return 213;
		case 0x00D6: return 214;
		case 0x0158: return 216;
		case 0x016E: return 217;
		case 0x00DA: return 218;
		case 0x0170: return 219;
		case 0x00DC: return 220;
		case 0x00DD: return 221;
		case 0x0162: return 222;
		case 0x00DF: return 223;
		case 0x0155: return 224;
		case 0x00E1: return 225;
		case 0x00E2: return 226;
		case 0x0103: return 227;
		case 0x00E4: return 228;
		case 0x013A: return 229;
		case 0x0107: return 230;
		case 0x00E7: return 231;
		case 0x010D: return 232;
		case 0x00E9: return 233;
		case 0x0119: return 234;
		case 0x00EB: return 235;
		case 0x011B: return 236;
		case 0x00ED: return 237;
		case 0x00EE: return 238;
		case 0x010F: return 239;
		case 0x0111: return 240;
		case 0x0144: return 241;
		case 0x0148: return 242;
		case 0x00F3: return 243;
		case 0x00F4: return 244;
		case 0x0151: return 245;
		case 0x00F6: return 246;
		case 0x0159: return 248;
		case 0x016F: return 249;
		case 0x00FA: return 250;
		case 0x0171: return 251;
		case 0x00FC: return 252;
		case 0x00FD: return 253;
		case 0x0163: return 254;
		case 0x02D9: return 255;

		default: return (uint8_t)utf32;
	}
}

