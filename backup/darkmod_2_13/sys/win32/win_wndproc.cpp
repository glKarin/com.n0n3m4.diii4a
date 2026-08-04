/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "win_local.h"
#include "../../renderer/tr_local.h"

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


void WIN_Sizing(WORD side, RECT *rect)
{
	if ( !glConfig.isInitialized || glConfig.vidHeight <= 0 || glConfig.vidWidth <= 0 ) {
		return;
	}

	if ( idKeyInput::IsDown( K_CTRL ) ) {
		return;
	}

	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;

	// Adjust width/height for window decoration
	RECT decoRect = { 0, 0, 0, 0 };
	AdjustWindowRect( &decoRect, WINDOW_STYLE | WS_SYSMENU, FALSE );
	int decoWidth = decoRect.right - decoRect.left;
	int decoHeight = decoRect.bottom - decoRect.top;

	width -= decoWidth;
	height -= decoHeight;

	// Clamp to a minimum size
	int currentHeight = r_customHeight.GetInteger();
	int currentWidth = r_customWidth.GetInteger();
	int minWidth = 256;
	int minHeight = minWidth * currentHeight / currentWidth;

	if ( width < minWidth ) {
		width = minWidth;
	}
	if ( height < minHeight ) {
		height = minHeight;
	}

	// Set the new size
	switch ( side ) {
	case WMSZ_LEFT:
		rect->left = rect->right - width - decoWidth;
		rect->bottom = rect->top + ( width * currentHeight / currentWidth ) + decoHeight;
		break;
	case WMSZ_RIGHT:
		rect->right = rect->left + width + decoWidth;
		rect->bottom = rect->top + ( width * currentHeight / currentWidth ) + decoHeight;
		break;
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMRIGHT:
		rect->bottom = rect->top + height + decoHeight;
		rect->right = rect->left + ( height * currentWidth / currentHeight ) + decoWidth;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPRIGHT:
		rect->top = rect->bottom - height - decoHeight;
		rect->right = rect->left + ( height * currentWidth / currentHeight ) + decoWidth;
		break;
	case WMSZ_BOTTOMLEFT:
		rect->bottom = rect->top + height + decoHeight;
		rect->left = rect->right - ( height * currentWidth / currentHeight ) - decoWidth;
		break;
	case WMSZ_TOPLEFT:
		rect->top = rect->bottom - height - decoHeight;
		rect->left = rect->right - ( height * currentWidth / currentHeight ) - decoWidth;
		break;
	}
}

//==========================================================================

// Keep this in sync with the one in win_input.cpp
// This one is used in the menu, the other one is used in game

static byte s_scantokey[128] = 
{ 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,          27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '-',    '=',          K_BACKSPACE, 9, // 0
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '[',       ']',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      ';', 
	'\'',       '`',    K_SHIFT,   '\\',       'z',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '/',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 

static byte s_scantoshift[128] = 
{ 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '!',       '@',        '#',    '$',         '%',      '^', 
	'&',        '*',    '(',       ')',        '_',    '+',         K_BACKSPACE, 9,  // 0 
	'Q',        'W',    'E',       'R',        'T',    'Y',         'U',      'I', 
	'O',        'P',    '{',       '}',        K_ENTER,K_CTRL,      'A',      'S',   // 1 
	'D',        'F',    'G',       'H',        'J',    'K',         'L',      ':', 
	'|' ,       '~',    K_SHIFT,   '\\',       'Z',    'X',         'C',      'V',   // 2 
	'B',        'N',    'M',       '<',        '>',    '?',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 


/*
=======
MapKey

Map from windows to Doom keynums
=======
*/
int MapKey (int key)
{
	int result;
	int modified;
	bool is_extended;

	modified = ( key >> 16 ) & 255;

	if ( modified > 127 )
		return 0;

	if ( key & ( 1 << 24 ) ) {
		is_extended = true;
	}
	else {
		is_extended = false;
	}

	//Check for certain extended character codes.
	//The specific case we are testing is the numpad / is not being translated
	//properly for localized builds.
	if(is_extended) {
		switch(modified) {
			case 0x35: //Numpad /
				return K_KP_SLASH;
		}
	}

	const unsigned char *scanToKey = Sys_GetScanTable();
	result = scanToKey[modified];

	// common->Printf( "Key: 0x%08x Modified: 0x%02x Extended: %s Result: 0x%02x\n", key, modified, (is_extended?"Y":"N"), result);

	if ( is_extended ) {
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
		case K_KP_STAR:
			return K_PRINT_SCR;
		case K_ALT:
			return K_RIGHT_ALT;
		}
	}
	else {
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
		}
	}

	return result;
}


/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	int key;
	switch( uMsg ) {
		case WM_WINDOWPOSCHANGED:
			if ( glConfig.isInitialized || win32.win_maximized ) {
				RECT rect;
				if (::GetClientRect(win32.hWnd, &rect)) {
					int newWidth = rect.right - rect.left;
					int newHeight = rect.bottom - rect.top;
					if (glConfig.vidWidth != newWidth || glConfig.vidHeight != newHeight) {
						// stgatilov: when TDM is minimized, rect is [0 0 0 0]
						// we should never set 0, since we divide by vid[Width|Height] everywhere
						if (newWidth > 0 || newHeight > 0) {
							glConfig.vidWidth = newWidth;
							glConfig.vidHeight = newHeight;
							cvarSystem->Find( "r_fboResolution" )->SetModified();
						}
					}
				}
			}
			break;

		case WM_CREATE:

			win32.hWnd = hWnd;

			// do the OpenGL setup
			void GLW_WM_CREATE( HWND hWnd );
			GLW_WM_CREATE( hWnd );

			break;

		case WM_DESTROY:
			// let sound and input know about this?
			win32.hWnd = NULL;
			break;

		case WM_CLOSE:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
			break;

		case WM_ACTIVATE:
			// if we got here because of an alt-tab or maximize,
			// we should activate immediately.  If we are here because
			// the mouse was clicked on a title bar or drag control,
			// don't activate until the mouse button is released
			{
				int	fActive, fMinimized;

				fActive = LOWORD(wParam);
				fMinimized = (BOOL) HIWORD(wParam);

				win32.activeApp = (fActive != WA_INACTIVE);
				if ( win32.activeApp ) {
					idKeyInput::ClearStates();
					com_editorActive = false;
					Sys_GrabMouseCursor( true );
				}

				if ( fActive == WA_INACTIVE ) {
					win32.movingWindow = false;
				}

				// start playing the game sound world
				session->SetPlayingSoundWorld();

				// we do not actually grab or release the mouse here,
				// that will be done next time through the main loop
			}
			break;

		case WM_MOVE: {
			int		xPos, yPos;
			RECT r;
			int		style;

			if (!win32.cdsFullscreen )
			{
				xPos = (short) LOWORD(lParam);    // horizontal position 
				yPos = (short) HIWORD(lParam);    // vertical position 

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				win32.win_xpos.SetInteger( xPos + r.left );
				win32.win_ypos.SetInteger( yPos + r.top );
				win32.win_xpos.ClearModified();
				win32.win_ypos.ClearModified();
			}
			break;
		}
		case WM_TIMER: {
			if ( win32.win_timerUpdate.GetBool() ) {
				common->Frame();
			}
			break;
		}
		case WM_SYSCOMMAND:
			if ( wParam == SC_MAXIMIZE || wParam == 0xf032 ) // Maximized
				win32.win_maximized = true;
			if ( wParam == SC_RESTORE || wParam == 0xf012 ) // Restored
				win32.win_maximized = false;
			if ( wParam == SC_SCREENSAVE || wParam == SC_KEYMENU ) {
				return 0;
			}
			break;

		case WM_SYSKEYDOWN:
			if ( wParam == 13 ) {	// alt-enter toggles full-screen
				cvarSystem->SetCVarBool( "r_fullscreen", !renderSystem->IsFullScreen() );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart partial\n" );
				return 0;
			}
			// fall through for other keys
		case WM_KEYDOWN:
			key = MapKey( lParam );
			if ( key == K_CTRL || key == K_ALT || key == K_RIGHT_ALT ) {
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, true, 0, NULL );
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			key = MapKey( lParam );
			if ( key == K_PRINT_SCR ) {
				// don't queue printscreen keys.  Since windows doesn't send us key
				// down events for this, we handle queueing them with DirectInput
				break;
			} else if ( key == K_CTRL || key == K_ALT || key == K_RIGHT_ALT ) {
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, false, 0, NULL );
			break;

		case WM_CHAR:
			Sys_QueEvent( win32.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL );
			break;

		case WM_NCLBUTTONDOWN:
//			win32.movingWindow = true;
			break;

		case WM_ENTERSIZEMOVE:
			win32.movingWindow = true;
			break;

		case WM_EXITSIZEMOVE:
			win32.movingWindow = false;
			break;

		case WM_SIZING:
			WIN_Sizing(wParam, (RECT *)lParam);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE: {
			break;
		}
		case WM_MOUSEWHEEL: {
			int delta = GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA;
			int key = delta < 0 ? K_MWHEELDOWN : K_MWHEELUP;
			delta = abs( delta );
			while( delta-- > 0 ) {
				Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, true, 0, NULL );
				Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, false, 0, NULL );
			}
			break;
		}
	}

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
