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
#include "win_local.h"

WinVars_t	g_wv;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported by the OS 
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN                  0x020B
#endif

#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP                    0x020C
#endif

#ifndef MK_XBUTTON1
#define MK_XBUTTON1         0x0020
#endif

#ifndef MK_XBUTTON2
#define MK_XBUTTON2         0x0040
#endif



static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
cvar_t		*vid_xpos;			// X coordinate of window position
cvar_t		*vid_ypos;			// Y coordinate of window position
cvar_t		*r_fullscreen;
cvar_t          *win_fastModeChange;

#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static qboolean s_alttab_disabled;

static void WIN_DisableAltTab( void )
{
	if ( s_alttab_disabled )
		return;

	if ( !Q_stricmp( Cvar_VariableString( "arch" ), "winnt" ) )
	{
		RegisterHotKey( g_wv.hWnd, 0, MOD_ALT, VK_TAB );
	}
	else
	{
		BOOL old;

		SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, 1, &old, 0 );
	}
	s_alttab_disabled = qtrue;
}

static void WIN_EnableAltTab( void )
{
	if ( s_alttab_disabled )
	{
		if ( !Q_stricmp( Cvar_VariableString( "arch" ), "winnt" ) )
		{
			UnregisterHotKey( g_wv.hWnd, 0 );
		}
		else
		{
			BOOL old;

			SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, 0, &old, 0 );
		}

		s_alttab_disabled = qfalse;
	}
}

/*
==================
VID_AppActivate
==================
*/
extern qboolean GLW_ResetFullScreenMode( void );
extern qboolean GLW_ResetDesktopMode( void );

static void VID_AppActivate(BOOL fActive, BOOL minimize)
{
	BOOL wasMinimized = g_wv.isMinimized;

	g_wv.isMinimized = minimize;

	Com_DPrintf("VID_AppActivate: %i\n", fActive );

	Key_ClearStates();	// FIXME!!!

	// we don't want to act like we're active if we're minimized
	if (fActive && !g_wv.isMinimized )
	{
		g_wv.activeApp = qtrue;
	}
	else
	{
		g_wv.activeApp = qfalse;
	}

	// minimize/restore mouse-capture on demand
	if (!g_wv.activeApp )
	{
		IN_Activate (qfalse);
		if ( r_fullscreen->integer )
		{
			ShowWindow ( g_wv.hWnd, SW_MINIMIZE );
			GLW_ResetDesktopMode();
			WIN_EnableAltTab();
		}
	}
	else
	{
		IN_Activate (qtrue);
		if ( r_fullscreen->integer && wasMinimized )
		{
			WIN_DisableAltTab();
			if( !win_fastModeChange->integer ||
				!GLW_ResetFullScreenMode() )
			{
				Cbuf_AddText( "vid_restart\n" );
			}
		}
	}
}

//==========================================================================

static byte s_scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   K_CAPSLOCK  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
static int MapKey (int key)
{
	int result;
	int modified;
	qboolean is_extended;

//	Com_Printf( "0x%x\n", key);

	modified = ( key >> 16 ) & 255;

	if ( modified > 127 )
		return 0;

	if ( key & ( 1 << 24 ) )
	{
		is_extended = qtrue;
	}
	else
	{
		is_extended = qfalse;
	}

	result = s_scantokey[modified];

	if ( !is_extended )
	{
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else
	{
		switch ( result )
		{
		case K_PAUSE:
			return K_KP_NUMLOCK;
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

static __inline void HandleMouseButtons(WPARAM wParam)
{
	int	temp = 0;

	if (wParam & MK_LBUTTON) temp |= 1;
	if (wParam & MK_RBUTTON) temp |= 2;
	if (wParam & MK_MBUTTON) temp |= 4;
	if (wParam & MK_XBUTTON1) temp |= 8;
	if (wParam & MK_XBUTTON2) temp |= 16;

	IN_MouseEvent (temp);
}


/*
====================
MainWndProc

main window procedure
====================
*/
extern cvar_t *in_mouse;
extern cvar_t *in_logitechbug;

static HWND g_rawmouseinitializedhwnd = NULL;
void IN_InitRawMouseInput(HANDLE hWnd);
void IN_HandleRawMouseData(HRAWINPUT hndlmouse);

LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	static qboolean flip = qtrue;
	int zDelta, i;

	if (in_mouse->integer == 2 && g_rawmouseinitializedhwnd != hWnd)
	{
		IN_InitRawMouseInput(hWnd);
		g_rawmouseinitializedhwnd = hWnd;
	}

	switch (uMsg)
	{
	case WM_SETCURSOR:
		if (in_mouse->integer == 2) SetCursor(NULL);
		break;
	case WM_MOUSEHWHEEL:
		// Windows Vista and above...

		if (in_mouse->integer == -1 || in_mouse->integer == 2)
		{
			// 120 increments, might be 240 and multiples if wheel goes too fast
			zDelta = ( short ) HIWORD( wParam ) / 120;
			if ( zDelta > 0 )
			{
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELRIGHT, qtrue, 0, NULL );
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELRIGHT, qfalse, 0, NULL );
			}
			else
			{
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELLEFT, qtrue, 0, NULL );
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELLEFT, qfalse, 0, NULL );
			}
			
			if (in_mouse->integer == -1) HandleMouseButtons(wParam);

			// when an application processes the WM_MOUSEHWHEEL message, it must return zero
			return 0;
		}
		break;
		
		
	case WM_MOUSEWHEEL:
		// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/winui/windowsuserinterface/userinput/mouseinput/aboutmouseinput.asp
		// Windows 98/Me, Windows NT 4.0 and later - uses WM_MOUSEWHEEL
		// only relevant for non-DI input and when console is toggled in window mode
		//   if console is toggled in window mode (KEYCATCH_CONSOLE) then mouse is released and DI doesn't see any mouse wheel
		
		if (in_mouse->integer == -1 || (!r_fullscreen->integer && (cls.keyCatchers & KEYCATCH_CONSOLE)))
		{
			// 120 increments, might be 240 and multiples if wheel goes too fast
			// NOTE Logitech: logitech drivers are screwed and send the message twice?
			//   could add a cvar to interpret the message as successive press/release events
			zDelta = ( short ) HIWORD( wParam ) / 120;
			if ( zDelta > 0 )
			{
				for(i=0; i<zDelta; i++)
				{
					if (!in_logitechbug->integer)
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
					}
					else
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, flip, 0, NULL );
						flip = !flip;
					}
				}
			}
			else
			{
				for(i=0; i<-zDelta; i++)
				{
					if (!in_logitechbug->integer)
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
					}
					else
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, flip, 0, NULL );
						flip = !flip;
					}
				}
			}
		
			HandleMouseButtons(wParam);

		
			
			// when an application processes the WM_MOUSEWHEEL message, it must return zero
			return 0;
		}
		break;

	case WM_INPUT:
		if (in_mouse->integer == 2)
		{
			IN_HandleRawMouseData((HRAWINPUT)lParam);
		}
		break;

	case WM_CREATE:

		g_wv.hWnd = hWnd;

		vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
		vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
		r_fullscreen = Cvar_Get ("r_fullscreen", "1", CVAR_ARCHIVE );
		win_fastModeChange = Cvar_Get( "win_fastModeChange", "1", CVAR_ARCHIVE );
		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG"); 
		if ( r_fullscreen->integer )
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}

		break;
#if 0
	case WM_DISPLAYCHANGE:
		Com_DPrintf( "WM_DISPLAYCHANGE\n" );
		// we need to force a vid_restart if the user has changed
		// their desktop resolution while the game is running,
		// but don't do anything if the message is a result of
		// our own calling of ChangeDisplaySettings
		if ( com_insideVidInit ) {
			break;		// we did this on purpose
		}
		// something else forced a mode change, so restart all our gl stuff
		Cbuf_AddText( "vid_restart\n" );
		break;
#endif
	case WM_DESTROY:
		// let sound and input know about this?
		g_wv.hWnd = NULL;
		if ( r_fullscreen->integer )
		{
			WIN_EnableAltTab();
		}
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
		break;

	case WM_ACTIVATE:
		{
			int	fActive, fMinimized;

			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			VID_AppActivate( fActive != WA_INACTIVE, fMinimized);
			SNDDMA_Activate();
		}
		break;

	case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;
			int		style;

			if (!r_fullscreen->integer )
			{
				xPos = (short) LOWORD(lParam);    // horizontal position 
				yPos = (short) HIWORD(lParam);    // vertical position 

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				Cvar_SetValue( "vid_xpos", xPos + r.left);
				Cvar_SetValue( "vid_ypos", yPos + r.top);
				vid_xpos->modified = qfalse;
				vid_ypos->modified = qfalse;
				if ( g_wv.activeApp )
				{
					IN_Activate (qtrue);
				}
			}
		}
		break;

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEMOVE:
		if (in_mouse->integer == -1) HandleMouseButtons(wParam);
		break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE )
			return 0;
		break;

	case WM_HOTKEY:
		// RegisterHotKey 0 is for Alt-Tab, pass tab along
		if( wParam == 0 )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, VK_TAB,
				qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, VK_TAB,
				qfalse, 0, NULL );
		}
		break;
	case WM_SYSKEYDOWN:
		if ( wParam == 13 )
		{
			if ( r_fullscreen )
			{
				Cvar_SetValue( "r_fullscreen", !r_fullscreen->integer );
				Cbuf_AddText( "vid_restart\n" );
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qtrue, 0, NULL );
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qfalse, 0, NULL );
		break;

	case WM_CHAR:
		Sys_QueEvent( g_wv.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL );
		break;
   }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

