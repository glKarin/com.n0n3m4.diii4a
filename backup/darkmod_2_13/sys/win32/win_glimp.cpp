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
/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_SwapBuffers
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#include "precompiled.h"
#pragma hdrstop

#include "win_local.h"
#include "rc/doom_resource.h"
#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"


/*
========================
GLimp_GetOldGammaRamp
========================
*/
static void GLimp_SaveGamma( void ) {
	HDC			hDC;
	BOOL		success;

	hDC = GetDC( GetDesktopWindow() );
	success = GetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->Printf( "...getting default gamma ramp: %s\n", success ? "success" : "failed" );
	ReleaseDC( GetDesktopWindow(), hDC );
}

/*
========================
GLimp_RestoreGamma
========================
*/
static void GLimp_RestoreGamma( void ) {
	HDC hDC;
	BOOL success;

	// if we never read in a reasonable looking
	// table, don't write it out
	if ( win32.oldHardwareGamma[0][255] == 0 ) {
		return;
	}
	hDC = GetDC( GetDesktopWindow() );
	success = SetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->Printf( "...restoring hardware gamma: %s\n", success ? "success" : "failed" );
	ReleaseDC( GetDesktopWindow(), hDC );
}


/*
========================
GLimp_SetGamma

The renderer calls this when the user adjusts gamma or brightness
========================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] ) {
	unsigned short table[3][256];

	if ( !win32.hDC ) {
		return;
	}

	for ( int i = 0; i < 256; i++ ) {
		table[0][i] = red[i];
		table[1][i] = green[i];
		table[2][i] = blue[i];
	}

	if ( !SetDeviceGammaRamp( win32.hDC, table ) ) {
		common->Warning( "SetDeviceGammaRamp failed." );
	}
}

/*
=============================================================================

WglExtension Grabbing

This is gross -- creating a window just to get a context to get the wgl extensions

=============================================================================
*/

/*
====================
FakeWndProc

Only used to get wglExtensions
====================
*/
LONG WINAPI FakeWndProc(
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam ) {

	if ( uMsg == WM_DESTROY ) {
		PostQuitMessage( 0 );
	}

	if ( uMsg != WM_CREATE ) {
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	const static PIXELFORMATDESCRIPTOR pfd = {
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		8, 0,
		0, 0, 0, 0,
		24, 8,
		0,
		PFD_MAIN_PLANE,
		0,
		0,
		0,
		0,
	};
	int pixelFormat;
	HDC hDC;
	HGLRC hGLRC;

	hDC = GetDC( hWnd );

	// Set up OpenGL
	pixelFormat = ChoosePixelFormat( hDC, &pfd );
	SetPixelFormat( hDC, pixelFormat, &pfd );
	hGLRC = qwglCreateContext( hDC );
	qwglMakeCurrent( hDC, hGLRC );

	// free things
	qwglMakeCurrent( NULL, NULL );
	qwglDeleteContext( hGLRC );
	ReleaseDC( hWnd, hDC );

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
static void GLW_GetWGLExtensionsWithFakeWindow( void ) {
	HWND	hWnd;
	MSG		msg;

	//load basic functions like e.g. wglMakeCurrent
	GLimp_LoadFunctions(false);

	// Create a window for the sole purpose of getting
	// a valid context to get the wglextensions
	hWnd = CreateWindow( WIN32_FAKE_WINDOW_CLASS_NAME, GAME_NAME,
	                     WS_OVERLAPPEDWINDOW,
	                     40, 40,
	                     640,
	                     480,
	                     NULL, NULL, win32.hInstance, NULL );
	if ( hWnd ) {
		HDC hDC = GetDC( hWnd );
		HGLRC gRC = qwglCreateContext( hDC );
		qwglMakeCurrent( hDC, gRC );
		//reload context creation functions like e.g. wglCreateContextAttribsARB
		win32.hDC = hDC;
		GLimp_LoadFunctions();
		win32.hDC = NULL;
		qwglDeleteContext( gRC );
		ReleaseDC( hWnd, hDC );

		DestroyWindow( hWnd );
		while ( GetMessage( &msg, NULL, 0, 0 ) ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	} else {
		common->FatalError( "GLW_GetWGLExtensionsWithFakeWindow: Couldn't create fake window" );
	}
}

//=============================================================================

/*
====================
GLW_WM_CREATE
====================
*/
void GLW_WM_CREATE( HWND hWnd ) {
}

/*
====================
GLW_InitDriver

Set the pixelformat for the window before it is
shown, and create the rendering context
====================
*/
static bool GLW_InitDriver( glimpParms_t parms ) {
	PIXELFORMATDESCRIPTOR src = {
		sizeof( PIXELFORMATDESCRIPTOR ),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,	// support window, OpenGL, double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 32-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		8,								// 8 bit destination alpha
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};

	common->Printf( "Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if ( win32.hDC == NULL ) {
		common->Printf( "...getting DC: " );

		if ( ( win32.hDC = GetDC( win32.hWnd ) ) == NULL ) {
			common->Printf( S_COLOR_YELLOW "failed\n" S_COLOR_DEFAULT );
			return false;
		}
		common->Printf( "succeeded\n" );
	}

	// the multisample path uses the wgl
	// duzenko #4425: AA needs to be setup elsewhere
#if 1	// ChoosePixelFormatARB is the only way to get sRGB with the default FBO?
	UINT	numFormats;
	int		iAttributes[] = {
		WGL_SAMPLE_BUFFERS_ARB,				1,
		WGL_SAMPLES_ARB,					1,
		WGL_DOUBLE_BUFFER_ARB,				TRUE,
		WGL_STENCIL_BITS_ARB,				8,
		WGL_DEPTH_BITS_ARB,					24,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,	TRUE,
		0,									0
	};
	FLOAT	fAttributes[] = { 0, 0 };
	if ( qwglChoosePixelFormatARB && r_fboSRGB ) {
		qwglChoosePixelFormatARB( win32.hDC, iAttributes, fAttributes, 1, &win32.pixelformat, &numFormats );
	} else
#else	// used to do this in older TDM versions
	if ( qwglChoosePixelFormatARB && ( parms.multiSamples > 1 && !r_useFbo.GetBool() ) ) {
		int		iAttributes[20];
		FLOAT	fAttributes[] = {0, 0};
		UINT	numFormats;

		// FIXME: specify all the other stuff
		iAttributes[0] = WGL_SAMPLE_BUFFERS_ARB;
		iAttributes[1] = 1;
		iAttributes[2] = WGL_SAMPLES_ARB;
		iAttributes[3] = parms.multiSamples;
		iAttributes[4] = WGL_DOUBLE_BUFFER_ARB;
		iAttributes[5] = TRUE;
		iAttributes[6] = WGL_STENCIL_BITS_ARB;
		iAttributes[7] = 8;
		iAttributes[8] = WGL_DEPTH_BITS_ARB;
		iAttributes[9] = 24;
		iAttributes[10] = WGL_RED_BITS_ARB;
		iAttributes[11] = 8;
		iAttributes[12] = WGL_BLUE_BITS_ARB;
		iAttributes[13] = 8;
		iAttributes[14] = WGL_GREEN_BITS_ARB;
		iAttributes[15] = 8;
		iAttributes[16] = WGL_ALPHA_BITS_ARB;
		iAttributes[17] = 8;
		iAttributes[18] = 0;
		iAttributes[19] = 0;

		qinit_wglChoosePixelFormatARB( win32.hDC, iAttributes, fAttributes, 1, &win32.pixelformat, &numFormats );
	} else 
#endif
	{
		// this is the "classic" choose pixel format path
		// eventually we may need to have more fallbacks, but for
		// now, ask for everything
		if ( parms.stereo ) {
			common->Printf( "...attempting to use stereo\n" );
			src.dwFlags |= PFD_STEREO;
		}

		// choose, set, and describe our desired pixel format.  If we're
		// using a minidriver then we need to bypass the GDI functions,
		// otherwise use the GDI functions.
		if ( ( win32.pixelformat = ChoosePixelFormat( win32.hDC, &src ) ) == 0 ) {
			common->Warning( "GLW_ChoosePFD failed" );
			return false;
		}
		common->Printf( "...PIXELFORMAT %d selected\n", win32.pixelformat );
	}

	// get the full info
	DescribePixelFormat( win32.hDC, win32.pixelformat, sizeof( win32.pfd ), &win32.pfd );

	// the same SetPixelFormat is used either way
	if ( SetPixelFormat( win32.hDC, win32.pixelformat, &win32.pfd ) == FALSE ) {
		common->Warning( "SetPixelFormat failed", win32.hDC );
		return false;
	}

	if (GLAD_WGL_ARB_create_context && GLAD_WGL_ARB_create_context_profile) {
		// create OpenGL context for rendering (new GL3+ approach with context and debug)
		common->Printf( "...creating GL context: " );
		if( r_glCoreProfile.GetInteger() == 0 )
			common->Printf("compatibility ");
		else if( r_glCoreProfile.GetInteger() == 1 )
			common->Printf("core ");
		else if( r_glCoreProfile.GetInteger() == 2 )
			common->Printf("core-fc ");
		if( r_glDebugContext.GetBool() )
			common->Printf("debug ");
		common->Printf("\n");
		const int attribs[] = {
			// we want at least this version of GL
			WGL_CONTEXT_MAJOR_VERSION_ARB, QGL_REQUIRED_VERSION_MAJOR,
			WGL_CONTEXT_MINOR_VERSION_ARB, QGL_REQUIRED_VERSION_MINOR,
			// TODO: might want to (optionally) create a core profile once we got rid of the old stuff
			WGL_CONTEXT_PROFILE_MASK_ARB, r_glCoreProfile.GetInteger() > 0 ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			// special case for 3.1: even core profiles are still compatible :/
			// enable debug context if asked for
			WGL_CONTEXT_FLAGS_ARB, (r_glCoreProfile.GetInteger() > 1 ? WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB : 0) | (r_glDebugContext.GetBool() ? WGL_CONTEXT_DEBUG_BIT_ARB : 0),
			0
		};
		win32.hGLRC = qwglCreateContextAttribsARB( win32.hDC, NULL, attribs );
	}
	else {
		// create OpenGL context for rendering (deprecated GL1 approach)
		r_glCoreProfile.SetInteger(0);
		common->Printf( "...creating GL context: deprecated\n" );
		win32.hGLRC = qwglCreateContext( win32.hDC );
	}

	if ( win32.hGLRC == 0 ) {
		common->Printf( S_COLOR_YELLOW "Failed to create OpenGL context\n" S_COLOR_DEFAULT );
		return false;
	}

	common->Printf( "...making context current: " );
	if ( !qwglMakeCurrent( win32.hDC, win32.hGLRC ) ) {
		qwglDeleteContext( win32.hGLRC );
		win32.hGLRC = NULL;
		common->Printf( S_COLOR_YELLOW "failed\n" S_COLOR_DEFAULT );
		return false;
	}
	common->Printf( "succeeded\n" );

	return true;
}

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses( void ) {
	WNDCLASS wc;

	// register the window class if necessary
	if ( win32.windowClassRegistered ) {
		return;
	}
	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( struct HBRUSH__ * )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if ( !RegisterClass( &wc ) ) {
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered window class\n" );

	// now register the fake window class that is only used
	// to get wgl extensions
	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) FakeWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( struct HBRUSH__ * )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_FAKE_WINDOW_CLASS_NAME;

	if ( !RegisterClass( &wc ) ) {
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered fake window class\n" );

	win32.windowClassRegistered = true;
}

/*
=======================
GLW_CreateWindow

Responsible for creating the Win32 window.
If cdsFullscreen is true, it won't have a border
=======================
*/
static bool GLW_CreateWindow( glimpParms_t parms ) {
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	// compute width and height
	if ( parms.fullScreen ) {
		exstyle = 0;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;

		if (win32.win_topmost)
			 exstyle |= WS_EX_TOPMOST;

		x = 0;
		y = 0;

		if ( r_fullscreen.GetInteger() == 2 ) {
			//always adjust game resolution to desktop resolution in borderless mode
			parms.width = glConfig.vidWidth = win32.desktopWidth;
			parms.height = glConfig.vidHeight = win32.desktopHeight;
			r_customWidth.SetInteger( win32.desktopWidth );
			r_customHeight.SetInteger( win32.desktopHeight );
			//adding excessive lines above and below screen, so that OS does NOT put us into exclusive mode
			//this hack was found here: https://stackoverflow.com/q/22259067/556899
			y = -1;
			parms.height += 2;
			glConfig.vidHeight += 2;
		}

		w = parms.width;
		h = parms.height;
	} else {
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		if ( win32.win_maximized )
			stylebits |= WS_MAXIMIZE;
		AdjustWindowRect( &r, stylebits, FALSE );

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = win32.win_xpos.GetInteger();
		y = win32.win_ypos.GetInteger();

		// adjust window coordinates if necessary
		// so that the window is completely on screen
		if ( x + w > win32.desktopWidth ) {
			x = ( win32.desktopWidth - w );
		}
		if ( y + h > win32.desktopHeight ) {
			y = ( win32.desktopHeight - h );
		}
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
	}

	// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
	idStr title = va( "%s %d.%02d/%u", GAME_NAME, TDM_VERSION_MAJOR, TDM_VERSION_MINOR, sizeof( void * ) * 8 );

	win32.hWnd = CreateWindowEx(
	                 exstyle,
	                 WIN32_WINDOW_CLASS_NAME,
	                 title.c_str(),
	                 stylebits,
	                 x, y, w, h,
	                 NULL,
	                 NULL,
	                 win32.hInstance,
	                 NULL );

	if ( !win32.hWnd ) {
		common->Warning( "GLW_CreateWindow() Couldn't create window" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	ShowWindow( win32.hWnd, SW_SHOW );
	UpdateWindow( win32.hWnd );
	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );

	if ( !GLW_InitDriver( parms ) ) {
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
		return false;
	}
	SetForegroundWindow( win32.hWnd );
	SetFocus( win32.hWnd );

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}

static void PrintCDSError( int value ) {
	switch ( value ) {
	case DISP_CHANGE_RESTART:
		common->Printf( "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		common->Printf( "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		common->Printf( "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		common->Printf( "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		common->Printf( "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		common->Printf( "not updated\n" );
		break;
	default:
		common->Printf( "unknown error %d\n", value );
		break;
	}
}

/*
===================
GLW_SetFullScreen
===================
*/
static bool GLW_SetFullScreen( glimpParms_t parms ) {
	win32.cdsFullscreen = true;
	if ( r_fullscreen.GetInteger() == 2 )
		return true;

	DEVMODE		dm;
	int			cdsRet;
	DEVMODE		devmode;
	int			modeNum;
	bool		matched;

	// first make sure the user is not trying to select a mode that his card/monitor can't handle
	matched = false;

	for ( modeNum = 0 ; ; modeNum++ ) {
		if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
			if ( matched ) {
				// we got a resolution match, but not a frequency match
				// so disable the frequency requirement
				common->Printf( "..." S_COLOR_YELLOW "%dhz is unsupported at %dx%d\n" S_COLOR_DEFAULT, parms.displayHz, parms.width, parms.height );
				parms.displayHz = 0;
				break;
			}
			common->Printf( "..." S_COLOR_YELLOW "%dx%d is unsupported in 32 bit\n" S_COLOR_DEFAULT, parms.width, parms.height );
			return false;
		}

		if ( ( int )devmode.dmPelsWidth  >= parms.width &&
		        ( int )devmode.dmPelsHeight >= parms.height &&
		        ( int )devmode.dmBitsPerPel == 32 ) {

			matched = true;

			if ( parms.displayHz == 0 || devmode.dmDisplayFrequency == parms.displayHz ) {
				break;
			}
		}
	}
	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );

	dm.dmPelsWidth  = parms.width;
	dm.dmPelsHeight = parms.height;
	dm.dmBitsPerPel = 32;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	if ( parms.displayHz != 0 ) {
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}
	common->Printf( "...calling CDS: " );

	// try setting the exact mode requested, because some drivers don't report
	// the low res modes in EnumDisplaySettings, but still work
	if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
		common->Printf( "ok\n" );
		win32.cdsFullscreen = true;
		return true;
	}

	// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
	common->Printf( S_COLOR_YELLOW "failed" S_COLOR_DEFAULT ", " );

	PrintCDSError( cdsRet );

	common->Printf( "...trying next higher resolution:" );

	// we could do a better matching job here...
	for ( modeNum = 0 ; ; modeNum++ ) {
		if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
			break;
		}
		if ( ( int )devmode.dmPelsWidth  >= parms.width
		        && ( int )devmode.dmPelsHeight >= parms.height
		        && ( int )devmode.dmBitsPerPel == 32 ) {

			if ( ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
				common->Printf( "ok\n" );
				win32.cdsFullscreen = true;

				return true;
			}
			break;
		}
	}
	common->Warning( "No high res mode found" );

	return false;
}


/*
===================
GLimp_Init

This is the platform specific OpenGL initialization function.  It
is responsible for loading OpenGL, initializing it,
creating a window of the appropriate size, doing
fullscreen manipulations, etc.  Its overall responsibility is
to make sure that a functional OpenGL subsystem is operating
when it returns to the ref.

If there is any failure, the renderer will revert back to safe
parameters and try again.
===================
*/
bool GLimp_Init( glimpParms_t parms ) {
	HDC			hDC;

	common->Printf( "Initializing OpenGL subsystem\n" );

	// check our desktop attributes
	hDC = GetDC( GetDesktopWindow() );
	win32.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	win32.desktopWidth = GetDeviceCaps( hDC, HORZRES );
	win32.desktopHeight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	// we can't run in a window unless it is 32 bpp
	if ( win32.desktopBitsPixel < 32 && !parms.fullScreen ) {
		common->Warning( "GLimp_Init: Windowed mode requires 32 bit desktop depth" );
		return false;
	}

	// save the hardware gamma so it can be
	// restored on exit
	GLimp_SaveGamma();

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	// getting the wgl extensions involves creating a fake window to get a context,
	// which is pretty disgusting, and seems to mess with the AGP VAR allocation
	GLW_GetWGLExtensionsWithFakeWindow();

	// try to change to fullscreen
	if ( parms.fullScreen ) {
		if ( !GLW_SetFullScreen( parms ) ) {
			GLimp_Shutdown();
			return false;
		}
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
	if ( !GLW_CreateWindow( parms ) ) {
		GLimp_Shutdown();
		return false;
	}

	common->Printf( "...initializing QGL\n" );
	//load all function pointers available in the final context
	GLimp_LoadFunctions();

	return true;
}


/*
===================
GLimp_SetScreenParms

Sets up the screen based on passed parms..
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms ) {
	int x, y, w, h;
	DEVMODE dm;

	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	if ( parms.displayHz != 0 ) {
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	win32.cdsFullscreen = parms.fullScreen;
	glConfig.isFullscreen = parms.fullScreen;

	if ( parms.fullScreen ) {
		SetWindowLong( win32.hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_SYSMENU );
		SetWindowLong( win32.hWnd, GWL_EXSTYLE, 0 );
		dm.dmPelsWidth  = parms.width;
		dm.dmPelsHeight = parms.height;
		dm.dmBitsPerPel = 32;
		x = y = w = h = 0;
	} else {
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = win32.win_xpos.GetInteger();
		y = win32.win_ypos.GetInteger();

		// adjust window coordinates if necessary
		// so that the window is completely on screen
		if ( x + w > win32.desktopWidth ) {
			x = ( win32.desktopWidth - w );
		}
		if ( y + h > win32.desktopHeight ) {
			y = ( win32.desktopHeight - h );
		}
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
		dm.dmPelsWidth  = win32.desktopWidth;
		dm.dmPelsHeight = win32.desktopHeight;
		dm.dmBitsPerPel = win32.desktopBitsPixel;
		auto stylebits = WINDOW_STYLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		AdjustWindowRect( &r, stylebits, FALSE );
		SetWindowLong( win32.hWnd, GWL_STYLE, stylebits );
		SetWindowLong( win32.hWnd, GWL_EXSTYLE, 0 );
		common->Printf( "%i %i %i %i\n", x, y, w, h );
	}

	// duzenko #4425: always use desktop resolution when using fbo
	if ( parms.fullScreen ) {
		HMONITOR hMonitor = MonitorFromWindow( win32.hWnd, MONITOR_DEFAULTTOPRIMARY );
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof( lpmi );
		if ( GetMonitorInfo( hMonitor, &lpmi ) ) {
			SetWindowPos( win32.hWnd, 0, lpmi.rcMonitor.left, lpmi.rcMonitor.top,
						  lpmi.rcMonitor.right - lpmi.rcMonitor.left, lpmi.rcMonitor.bottom - lpmi.rcMonitor.top, SWP_SHOWWINDOW );
		} else {
			SetWindowPos( win32.hWnd, 0, 0, 0, win32.desktopWidth, win32.desktopHeight, SWP_SHOWWINDOW );
		}
	} else {
		SetWindowPos( win32.hWnd, 0, x, y, w, h, SWP_SHOWWINDOW );
	}
	return true;
}

/*
===================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void GLimp_Shutdown( void ) {
	const char *success[] = { "failed", "success" };
	int retVal;

	common->Printf( "Shutting down OpenGL subsystem\n" );

	// set current context to NULL
	if ( qwglMakeCurrent ) {
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;
		common->Printf( "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if ( win32.hGLRC && qwglDeleteContext ) {
		retVal = qwglDeleteContext( win32.hGLRC ) != 0;
		common->Printf( "...deleting GL context: %s\n", success[retVal] );
		win32.hGLRC = NULL;
	}

	// release DC
	if ( win32.hDC ) {
		retVal = ReleaseDC( win32.hWnd, win32.hDC ) != 0;
		common->Printf( "...releasing DC: %s\n", success[retVal] );
		win32.hDC = NULL;
	}

	// destroy window
	if ( win32.hWnd ) {
		common->Printf( "...destroying window\n" );
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
	}

	// reset display settings
	if ( win32.cdsFullscreen ) {
		common->Printf( "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		win32.cdsFullscreen = false;
	}

	// close the thread so the handle doesn't dangle
	if ( win32.renderThreadHandle ) {
		common->Printf( "...closing smp thread\n" );
		CloseHandle( win32.renderThreadHandle );
		win32.renderThreadHandle = NULL;
	}

	// restore gamma
	GLimp_RestoreGamma();

	// shutdown QGL subsystem
	common->Printf( "...shutting down QGL\n" );
	GLimp_UnloadFunctions();
}


/*
=====================
GLimp_SwapBuffers
=====================
*/
void GLimp_SwapBuffers( void ) {
	// wglSwapinterval is a windows-private extension,
	// so we must check for it here instead of portably
	if ( r_swapInterval.IsModified() ) {
		r_swapInterval.ClearModified();
		if ( qwglSwapIntervalEXT ) {
			qwglSwapIntervalEXT( r_swapInterval );
		}
	}
	SwapBuffers( win32.hDC );

#ifdef DEBUG_PRINTS
	//Sys_DebugPrintf( "*** SwapBuffers() ***\n" );
#endif
}

/*
===========================================================

SMP acceleration

===========================================================
*/

//#define	REALLOC_DC

/*
===================
GLimp_ActivateContext

===================
*/
void GLimp_ActivateContext( void ) {
	if ( !qwglMakeCurrent( win32.hDC, win32.hGLRC ) ) {
		win32.wglErrors++;
	}
}

/*
===================
GLimp_DeactivateContext

===================
*/
void GLimp_DeactivateContext( void ) {
	qglFinish();
	if ( !qwglMakeCurrent( win32.hDC, NULL ) ) {
		win32.wglErrors++;
	}

#ifdef REALLOC_DC
	// makeCurrent NULL frees the DC, so get another
	if ( ( win32.hDC = GetDC( win32.hWnd ) ) == NULL ) {
		win32.wglErrors++;
	}
#endif

}

/*
===================
GLimp_RenderThreadWrapper

===================
*/
static void GLimp_RenderThreadWrapper( void ) {
	win32.glimpRenderThread();

	// unbind the context before we die
	qwglMakeCurrent( win32.hDC, NULL );
}

/*
=======================
GLimp_SpawnRenderThread

Returns false if the system only has a single processor
=======================
*/
bool GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	SYSTEM_INFO info;

	// check number of processors
	GetSystemInfo( &info );
	if ( info.dwNumberOfProcessors < 2 ) {
		return false;
	}

	// create the IPC elements
	win32.renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	win32.renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	win32.renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	win32.glimpRenderThread = function;

	win32.renderThreadHandle = CreateThread(
	                               NULL,		// LPSECURITY_ATTRIBUTES lpsa,
	                               0,			// DWORD cbStack,
	                               ( LPTHREAD_START_ROUTINE )GLimp_RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
	                               0,			// LPVOID lpvThreadParm,
	                               0,			//   DWORD fdwCreate,
	                               &win32.renderThreadId );

	if ( win32.renderThreadHandle ) {
		SetThreadPriority( win32.renderThreadHandle, THREAD_PRIORITY_ABOVE_NORMAL );
	} else {
		common->Error( "GLimp_SpawnRenderThread: failed" );
	}

#if 0
	// make sure they always run on different processors
	SetThreadAffinityMask( GetCurrentThread, 1 );
	SetThreadAffinityMask( win32.renderThreadHandle, 2 );
#endif

	return true;
}

//#define	DEBUG_PRINTS

/*
===================
GLimp_BackEndSleep

===================
*/
void *GLimp_BackEndSleep( void ) {
	void	*data;

#ifdef DEBUG_PRINTS
	OutputDebugString( "-->GLimp_BackEndSleep\n" );
#endif
	ResetEvent( win32.renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( win32.renderCompletedEvent );

	WaitForSingleObject( win32.renderCommandsEvent, INFINITE );

	ResetEvent( win32.renderCompletedEvent );
	ResetEvent( win32.renderCommandsEvent );

	data = win32.smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( win32.renderActiveEvent );

#ifdef DEBUG_PRINTS
	OutputDebugString( "<--GLimp_BackEndSleep\n" );
#endif

	return data;
}

/*
===================
GLimp_FrontEndSleep

===================
*/
void GLimp_FrontEndSleep( void ) {
#ifdef DEBUG_PRINTS
	OutputDebugString( "-->GLimp_FrontEndSleep\n" );
#endif
	WaitForSingleObject( win32.renderCompletedEvent, INFINITE );

#ifdef DEBUG_PRINTS
	OutputDebugString( "<--GLimp_FrontEndSleep\n" );
#endif
}

volatile bool	renderThreadActive;

/*
===================
GLimp_WakeBackEnd

===================
*/
void GLimp_WakeBackEnd( void *data ) {
	int		r;

#ifdef DEBUG_PRINTS
	OutputDebugString( "-->GLimp_WakeBackEnd\n" );
#endif
	win32.smpData = data;

	if ( renderThreadActive ) {
		common->FatalError( "GLimp_WakeBackEnd: already active" );
	}
	r = WaitForSingleObject( win32.renderActiveEvent, 0 );

	if ( r == WAIT_OBJECT_0 ) {
		common->FatalError( "GLimp_WakeBackEnd: already signaled" );
	}
	r = WaitForSingleObject( win32.renderCommandsEvent, 0 );

	if ( r == WAIT_OBJECT_0 ) {
		common->FatalError( "GLimp_WakeBackEnd: commands already signaled" );
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent( win32.renderCommandsEvent );

	r = WaitForSingleObject( win32.renderActiveEvent, 5000 );

	if ( r == WAIT_TIMEOUT ) {
		common->FatalError( "GLimp_WakeBackEnd: WAIT_TIMEOUT" );
	}

#ifdef DEBUG_PRINTS
	OutputDebugString( "<--GLimp_WakeBackEnd\n" );
#endif
}
