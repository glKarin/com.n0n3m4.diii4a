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

#ifndef __WIN_LOCAL_H__
#define __WIN_LOCAL_H__

#include <windows.h>
#include <thread>


#define	MAX_OSPATH		256

#define	WINDOW_STYLE	(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_THICKFRAME)

void	Sys_CreateConsole( void );
void	Sys_DestroyConsole( void );

char	*Sys_ConsoleInput (void);

void	Win_SetErrorText( const char *text );

int		MapKey (int key);


// Input subsystem

// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void	IN_DeactivateMouseIfWindowed( void );
void	IN_DeactivateMouse( void );
void	IN_ActivateMouse( void );

void	IN_Frame( void );

int		IN_DIMapKey( int key );


// window procedure
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Conbuf_AppendText( const char *msg );

typedef struct Win32Vars_s {
	HWND			hWnd;
	HINSTANCE		hInstance;

	bool			activeApp;			// changed with WM_ACTIVATE messages
	bool			mouseReleased;		// when the game has the console down or is doing a long operation
	bool			movingWindow;		// inhibit mouse grab when dragging the window
	bool			mouseGrabbed;		// current state of grab and hide

	OSVERSIONINFOEX	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event (not really needed now that we use async direct input)
	int				sysMsgTime;

	bool			windowClassRegistered;

	WNDPROC			wndproc;

	HDC				hDC;				// handle to device context
	HGLRC			hGLRC;				// handle to GL rendering context
	PIXELFORMATDESCRIPTOR pfd;
	int				pixelformat;

	int				desktopBitsPixel;
	int				desktopWidth, desktopHeight;

	bool			cdsFullscreen;

	FILE			*log_fp;

	unsigned short	oldHardwareGamma[3][256];
	// desktop gamma is saved here for restoration at exit

	static idCVar	in_mouse;
	static idCVar	win_xpos;			// archived X coordinate of window position
	static idCVar	win_ypos;			// archived Y coordinate of window position
	static idCVarBool win_maximized;
	static idCVar	win_outputDebugString;
	static idCVar	win_outputEditString;
	static idCVar	win_viewlog;
	static idCVar	win_timerUpdate;
	static idCVarBool win_topmost;
	const std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();

	CRITICAL_SECTION criticalSections[MAX_CRITICAL_SECTIONS];
	HANDLE			events[MAX_TRIGGER_EVENTS];

	HINSTANCE		hInstDI;			// direct input

	LPDIRECTINPUT8			g_pdi;
	LPDIRECTINPUTDEVICE8	g_pMouse;
	LPDIRECTINPUTDEVICE8	g_pKeyboard;

	//windows 10 dpi scaling api
	HMODULE hShcoreDll = NULL;
	typedef HRESULT(WINAPI* GetDpiForMonitor_t)(HMONITOR hmonitor, int dpiType, UINT *dpiX, UINT *dpiY);
	GetDpiForMonitor_t pfGetDpiForMonitor = NULL;
	//mouse settings in Control Panel: sensitivity, acceleration
	int cp_mouseSpeed = 10;
	int cp_mouseAccel[3] = { 0, 0, 0 };
	//effective DPI scaling on the current monitor
	//note: currently it is updated only when user moves mouse in menu
	int effectiveScreenDpi[2] = { 96, 96 };

	HANDLE			renderCommandsEvent;
	HANDLE			renderCompletedEvent;
	HANDLE			renderActiveEvent;
	HANDLE			renderThreadHandle;
	unsigned long	renderThreadId;
	void			(*glimpRenderThread)( void );
	void			*smpData;
	int				wglErrors;
	// SMP acceleration vars

} Win32Vars_t;

extern Win32Vars_t	win32;

#endif /* !__WIN_LOCAL_H__ */
