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

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#include "win_local.h"
#include "rc/doom_resource.h"

#define COPY_ID			1
#define QUIT_ID			2
#define CLEAR_ID		3

#define ERRORBOX_ID		10
#define ERRORTEXT_ID	11

#define EDIT_ID			100
#define INPUT_ID		101

#define	COMMAND_HISTORY	64

typedef struct {
	HWND		hWnd;
	HWND		hwndBuffer;

	HWND		hwndButtonClear;
	HWND		hwndButtonCopy;
	HWND		hwndButtonQuit;

	HWND		hwndErrorBox;
	HWND		hwndErrorText;

	HBITMAP		hbmLogo;
	HBITMAP		hbmClearBitmap;

	HBRUSH		hbrEditBackground;
	HBRUSH		hbrErrorBackground;

	HFONT		hfBufferFont;

	HWND		hwndInputLine;

	char		consoleText[512], returnedText[512];
	bool		quitOnClose;
	int			windowWidth, windowHeight;
	 
	WNDPROC		SysInputLineWndProc;

	idEditField	historyEditLines[COMMAND_HISTORY];

	int			nextHistoryLine;// the last line in the history buffer, not masked
	int			historyLine;	// the line being displayed from history buffer
								// will be <= nextHistoryLine

	idEditField	consoleField;

} WinConData;

static WinConData s_wcd;

static LRESULT WINAPI ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char *cmdString;
	static bool s_timePolarity;

	switch (uMsg) {
		case WM_ACTIVATE:
			if ( LOWORD( wParam ) != WA_INACTIVE ) {
				SetFocus( s_wcd.hwndInputLine );
			}
		break;
		case WM_CLOSE:
			if ( cvarSystem->IsInitialized() && com_skipRenderer.GetBool() ) {
				cmdString = Mem_CopyString( "quit" );
                Sys_QueEvent(0, SE_CONSOLE, 0, 0, static_cast<int>(strlen(cmdString)) + 1, cmdString);
			} else if ( s_wcd.quitOnClose ) {
				PostQuitMessage( 0 );
			} else {
				Sys_ShowConsole( 0, false );
				win32.win_viewlog.SetBool( false );
			}
			return 0;
		case WM_CTLCOLORSTATIC:
			if ( ( HWND ) lParam == s_wcd.hwndBuffer ) {
				SetBkColor( ( HDC ) wParam, RGB( 0x00, 0x00, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0xff, 0xff, 0x00 ) );
				return (LRESULT) s_wcd.hbrEditBackground;
			} else if ( ( HWND ) lParam == s_wcd.hwndErrorBox ) {
				SetBkColor( (HDC) wParam, RGB( 0, 0, 0 ) );
				if ( s_timePolarity & 1 ) {
					SetTextColor( ( HDC ) wParam, RGB( 0xff, 0, 0 ) );
				} else {
					SetTextColor( ( HDC ) wParam, RGB( 0xff, 0xff, 0xff ) );
				}
				return (LRESULT) s_wcd.hbrErrorBackground;
			}
			break;
		case WM_SYSCOMMAND:
			if ( wParam == SC_CLOSE ) {
				PostQuitMessage( 0 );
			}
			break;
		case WM_COMMAND:
			if ( wParam == COPY_ID ) {
				SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
				SendMessage( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
			} else if ( wParam == QUIT_ID ) {
				if ( s_wcd.quitOnClose ) {
					PostQuitMessage( 0 );
				} else {
					cmdString = Mem_CopyString( "quit" );
                    Sys_QueEvent(0, SE_CONSOLE, 0, 0, static_cast<int>(strlen(cmdString)) + 1, cmdString);
				}
			} else if ( wParam == CLEAR_ID ) {
				SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
				SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) "" );
				UpdateWindow( s_wcd.hwndBuffer );
			}
			break;
		case WM_CREATE:
			s_wcd.hbrEditBackground = CreateSolidBrush( RGB( 0x00, 0x00, 0x80 ) );
			s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x0, 0x0, 0x0 ) );
			SetTimer( hWnd, 1, 1000, NULL );
			break;
		case WM_TIMER:
			if ( wParam == 1 ) {
				s_timePolarity = (bool)!s_timePolarity;
				if ( s_wcd.hwndErrorBox ) {
					InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
				}
			}
			break;
    }
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

LONG WINAPI InputLineWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int key, cursor;
	switch ( uMsg ) {
	case WM_KILLFOCUS:
		if ( ( HWND ) wParam == s_wcd.hWnd || ( HWND ) wParam == s_wcd.hwndErrorBox ) {
			SetFocus( hWnd );
			return 0;
		}
		break;

	case WM_KEYDOWN:
		key = MapKey( lParam );

		// command history
		if ( ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ) {
			if ( s_wcd.nextHistoryLine - s_wcd.historyLine < COMMAND_HISTORY && s_wcd.historyLine > 0 ) {
				s_wcd.historyLine--;
			}
			s_wcd.consoleField = s_wcd.historyEditLines[ s_wcd.historyLine % COMMAND_HISTORY ];

			SetWindowText( s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer() );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.consoleField.GetCursor(), s_wcd.consoleField.GetCursor() );
			return 0;
		}

		if ( ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ) {
			if ( s_wcd.historyLine == s_wcd.nextHistoryLine ) {
				return 0;
			}
			s_wcd.historyLine++;
			s_wcd.consoleField = s_wcd.historyEditLines[ s_wcd.historyLine % COMMAND_HISTORY ];

			SetWindowText( s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer() );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.consoleField.GetCursor(), s_wcd.consoleField.GetCursor() );
			return 0;
		}
		break;

	case WM_CHAR:
		key = MapKey( lParam );

		GetWindowText( s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer(), MAX_EDIT_LINE );
		SendMessage( s_wcd.hwndInputLine, EM_GETSEL, (WPARAM) NULL, (LPARAM) &cursor );
		s_wcd.consoleField.SetCursor( cursor );

		// enter the line
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			strncat( s_wcd.consoleText, s_wcd.consoleField.GetBuffer(), sizeof( s_wcd.consoleText ) - strlen( s_wcd.consoleText ) - 5 );
			strcat( s_wcd.consoleText, "\n" );
			SetWindowText( s_wcd.hwndInputLine, "" );

			Sys_Printf( "]%s\n", s_wcd.consoleField.GetBuffer() );

			// copy line to history buffer
			s_wcd.historyEditLines[s_wcd.nextHistoryLine % COMMAND_HISTORY] = s_wcd.consoleField;
			s_wcd.nextHistoryLine++;
			s_wcd.historyLine = s_wcd.nextHistoryLine;

			s_wcd.consoleField.Clear();

			return 0;
		}

		// command completion
		if ( key == K_TAB ) {
			s_wcd.consoleField.AutoComplete();

			SetWindowText( s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer() );
			//s_wcd.consoleField.SetWidthInChars( strlen( s_wcd.consoleField.GetBuffer() ) );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.consoleField.GetCursor(), s_wcd.consoleField.GetCursor() );

			return 0;
		}

		// clear autocompletion buffer on normal key input
		if ( ( key >= K_SPACE && key <= K_BACKSPACE ) || 
			( key >= K_KP_SLASH && key <= K_KP_PLUS ) || ( key >= K_KP_STAR && key <= K_KP_EQUALS ) ) {
			s_wcd.consoleField.ClearAutoComplete();
		}
		break;
	}
	return CallWindowProc( s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam );
}

/*
** Sys_CreateConsole
*/
RECT rect;
void Sys_CreateConsole( void ) {
	HDC hDC;
	WNDCLASS wc;
	const char *DEDCLASS = WIN32_CONSOLE_CLASS;
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_BORDER;
	int i;

	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__ *)COLOR_WINDOW;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClass (&wc) ) {
		return;
	}
	hDC = GetDC( GetDesktopWindow() );
	swidth = GetDeviceCaps( hDC, HORZRES );
	sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	rect.left = 0;
	rect.right = swidth/2;
	rect.top = 0;
	rect.bottom = sheight*2/3;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	// grayman #4539 - show whether this is a 32-bit or 64-bit binary
	// I had a problem building the 'title' string.
	// va() accepts the following:
	// idStr title = va( "%s%d.%02d/%u", GAME_NAME, TDM_VERSION_MAJOR, TDM_VERSION_MINOR, sizeof(void*) * 8);
	// but crashes when I add an extra character:
	// idStr title = va( "%s %d.%02d/%u", GAME_NAME, TDM_VERSION_MAJOR, TDM_VERSION_MINOR, sizeof(void*) * 8);
	// It's as if there's a limitation on the number of chars va will accept, which makes no sense.
	// So, for now, I'm going to change the title from "The Dark Mod 2.06" to "TDM 2.06" to give me room for the bit size.
	idStr title = va( "%s/%u", ENGINE_VERSION, sizeof(void*) * 8);

	s_wcd.hWnd = CreateWindowEx( 0,
							   DEDCLASS,
							   title.c_str(),
							   DEDSTYLE,
							   ( swidth - rect.right ) / 2, ( sheight - rect.bottom ) / 2 , rect.right - rect.left + 1, rect.bottom - rect.top + 1,
							   NULL,
							   NULL,
							   win32.hInstance,
							   NULL );

	if ( s_wcd.hWnd == NULL ) {
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	GetClientRect( s_wcd.hWnd, &rect );
	nHeight = MulDiv( 12, GetDeviceCaps( hDC, LOGPIXELSY ), 96 );

	s_wcd.hfBufferFont = CreateFont( -nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN | FIXED_PITCH, "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the buttons
	//
	int btnY = rect.bottom - nHeight * 2.3;
	s_wcd.hwndButtonCopy = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		nHeight*.5, btnY, nHeight*6, nHeight*2,
		s_wcd.hWnd, 
		( HMENU ) COPY_ID,	// child window ID
		win32.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "copy" );

	s_wcd.hwndButtonClear = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		nHeight*7, btnY, nHeight*6, nHeight * 2,
		s_wcd.hWnd, 
		( HMENU ) CLEAR_ID,	// child window ID
		win32.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM ) "clear" );

	s_wcd.hwndButtonQuit = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		rect.right - nHeight*6.5, btnY, nHeight*6, nHeight * 2,
		s_wcd.hWnd, 
		( HMENU ) QUIT_ID,	// child window ID
		win32.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "quit" );

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
		ES_LEFT | ES_AUTOHSCROLL,
		6, 6, rect.right - 12, 20,
		s_wcd.hWnd,
		(HMENU)INPUT_ID,	// child window ID
		win32.hInstance, NULL );

	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		0, nHeight*2.5, rect.right, rect.bottom - nHeight * 5,
		s_wcd.hWnd, 
		( HMENU ) EDIT_ID,	// child window ID
		win32.hInstance, NULL );
	SendMessage( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLongPtr( s_wcd.hwndInputLine, GWLP_WNDPROC, (LONG_PTR) InputLineWndProc );
	SendMessage( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	// don't show it now that we have a splash screen up
	if ( !strcmp( win32.win_viewlog.GetString(), "1" ) ) { // GetBool always false until cvar system initializes
		ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT);
		UpdateWindow( s_wcd.hWnd );
		SetForegroundWindow( s_wcd.hWnd );
		SetFocus( s_wcd.hwndInputLine );
	}
	s_wcd.consoleField.Clear();

	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		s_wcd.historyEditLines[i].Clear();
	}
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole( void ) {
	if ( s_wcd.hWnd ) {
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
}

/*
** Sys_ShowConsole
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose ) {

	s_wcd.quitOnClose = quitOnClose;

	if ( !s_wcd.hWnd ) {
		return;
	}

	switch ( visLevel ) {
		case 0:
			ShowWindow( s_wcd.hWnd, SW_HIDE );
		break;
		case 1:
			ShowWindow( s_wcd.hWnd, SW_SHOWNORMAL );
			SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		break;
		case 2:
			ShowWindow( s_wcd.hWnd, SW_MINIMIZE );
		break;
		default:
			Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}

/*
** Sys_GetCurrentMonitorResolution
*/
bool Sys_GetCurrentMonitorResolution( int &width, int &height ) {
	if ( win32.desktopWidth > 0 ) {
		width = win32.desktopWidth;
		height = win32.desktopHeight;
	} else {
		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect( hDesktop, &desktop );
		// The top left corner will have coordinates (0,0)
		// and the bottom right corner will have coordinates
		// (horizontal, vertical)
		width = desktop.right - desktop.left;
		height = desktop.bottom - desktop.top;
	}
	return true;
}

/*
** Sys_ConsoleInput
*/
char *Sys_ConsoleInput( void ) {
	
	if ( s_wcd.consoleText[0] == 0 ) {
		return NULL;
	}		
	strcpy( s_wcd.returnedText, s_wcd.consoleText );
	s_wcd.consoleText[0] = 0;
	
	return s_wcd.returnedText;
}

/*
** Conbuf_AppendText
*/
void Conbuf_AppendText( const char *pMsg )
{
#define CONSOLE_BUFFER_SIZE		16384

	char buffer[CONSOLE_BUFFER_SIZE*2];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 )	{
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	} else {
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) ) {
		if ( msg[i] == '\n' && msg[i+1] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		} else if ( msg[i] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( msg[i] == '\n' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( idStr::IsColor( &msg[i] ) ) {
			i++;
		} else {
			*b= msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	if ( s_totalChars > 0x7000 ) {
		PostMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
		s_totalChars = bufLen;
	}

	//
	// put this text into the windows console
	//
	// duzenko #4408 - changed from SendMessage to workaround thread deadlock
	// FIXME - buffer is unsafe
	if ( std::this_thread::get_id() == win32.MAIN_THREAD_ID ) {
		SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		SendMessage( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
		SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) buffer );
	} else {
		PostMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		PostMessage( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
		PostMessage( s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) buffer );
	}
}

/*
** Win_SetErrorText
*/
void Win_SetErrorText( const char *buf ) {
	if ( !s_wcd.hwndErrorBox ) {
		HDC hDC = GetDC( s_wcd.hWnd );
		int nHeight = MulDiv( 12, GetDeviceCaps( hDC, LOGPIXELSY ), 96 );
		s_wcd.hwndErrorBox = CreateWindow( "static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
			0, 0, rect.right, nHeight*2.5,
			s_wcd.hWnd, 
			( HMENU ) ERRORBOX_ID,	// child window ID
			win32.hInstance, NULL );
		SendMessage( s_wcd.hwndErrorBox, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
		SetWindowText( s_wcd.hwndErrorBox, buf );

		DestroyWindow( s_wcd.hwndInputLine );
		s_wcd.hwndInputLine = NULL;
	}
}
