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

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include "../renderercommon/tr_common.h"
#include "../qcommon/qcommon.h"

extern SDL_Window *SDL_window;

#ifdef USE_ALTGAMMA
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

static XF86VidModeGamma origGamma;
static Display *disp;
static int scrNum;
static qboolean gammaChanged = qfalse;

static cvar_t *r_altgamma;
static cvar_t *r_gamma;

void GLimp_InitGamma( void )
{
	r_altgamma = ri.Cvar_Get("r_altgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_gamma = ri.Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);

	disp = XOpenDisplay(NULL);
	scrNum = DefaultScreen(disp);

	XF86VidModeGetGamma(disp, scrNum, &origGamma);
}

void GLimp_ShutdownGamma( void )
{
	if (gammaChanged)
		XF86VidModeSetGamma(disp, scrNum, &origGamma);

	XCloseDisplay(disp);
}
#endif

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
#ifdef USE_ALTGAMMA
	if (r_altgamma->integer) {
		float g = r_gamma->value;

		XF86VidModeGamma gamma;

		gamma.red = g;
		gamma.green = g;
		gamma.blue = g;

		XF86VidModeSetGamma(disp, scrNum, &gamma);
		XF86VidModeGetGamma(disp, scrNum, &gamma);

		Com_Printf("XF86VidModeSetGamma: %.3f, %.3f, %.3f.\n", gamma.red, gamma.green, gamma.blue);
		gammaChanged = qtrue;
		return;
	}
#endif

	Uint16 table[3][256];
	int i, j;

	if( !glConfig.deviceSupportsGamma || r_ignorehwgamma->integer > 0 )
		return;

	for (i = 0; i < 256; i++)
	{
		table[0][i] = ( ( ( Uint16 ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( Uint16 ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( Uint16 ) blue[i] ) << 8 ) | blue[i];
	}

#ifdef _WIN32
#include <windows.h>

	// Win2K and newer put this odd restriction on gamma ramps...
	{
		OSVERSIONINFO	vinfo;

		vinfo.dwOSVersionInfoSize = sizeof( vinfo );
		GetVersionEx( &vinfo );
		if( vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			ri.Printf( PRINT_DEVELOPER, "performing gamma clamp.\n" );
			for( j = 0 ; j < 3 ; j++ )
			{
				for( i = 0 ; i < 128 ; i++ )
				{
					if( table[ j ] [ i] > ( ( 128 + i ) << 8 ) )
						table[ j ][ i ] = ( 128 + i ) << 8;
				}

				if( table[ j ] [127 ] > 254 << 8 )
					table[ j ][ 127 ] = 254 << 8;
			}
		}
	}
#endif

	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i-1])
				table[j][i] = table[j][i-1];
		}
	}

	if (SDL_SetWindowGammaRamp(SDL_window, table[0], table[1], table[2]) < 0)
	{
		ri.Printf( PRINT_DEVELOPER, "SDL_SetWindowGammaRamp() failed: %s\n", SDL_GetError() );
	}
}

