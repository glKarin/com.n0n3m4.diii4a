/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Doom you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

#pragma hdrstop

#include <float.h>
#include "win_local.h"
#include "../../renderer/RenderSystem_local.h"


int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   ( WINAPI * qwglGetPixelFormat)(HDC);
BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC ( WINAPI * qwglCreateContext)(HDC);
HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void )
{
	
}

#define GR_NUM_BOARDS 0x0f


#pragma warning (disable : 4113 4133 4047 )
#define GPA( a ) GetProcAddress( win32.hinstOpenGL, a )

/*
** QGL_Init
**
** This is responsible for binding our gl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
bool QGL_Init( const char *dllname )
{
	assert( win32.hinstOpenGL == 0 );

	common->Printf( "...initializing QGL\n" );

	common->Printf( "...calling LoadLibrary( '%s' ): ", dllname );

	if ( ( win32.hinstOpenGL = LoadLibrary( dllname ) ) == 0 )
	{
		common->Printf( "failed\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	qwglCopyContext              = wglCopyContext;
	qwglCreateContext            = wglCreateContext;
	qwglCreateLayerContext       = wglCreateLayerContext;
	qwglDeleteContext            = wglDeleteContext;
	qwglDescribeLayerPlane       = wglDescribeLayerPlane;
	qwglGetCurrentContext        = wglGetCurrentContext;
	qwglGetCurrentDC             = wglGetCurrentDC;
	qwglGetLayerPaletteEntries   = wglGetLayerPaletteEntries;
	qwglGetProcAddress           = wglGetProcAddress;
	qwglMakeCurrent              = wglMakeCurrent;
	qwglRealizeLayerPalette      = wglRealizeLayerPalette;
	qwglSetLayerPaletteEntries   = wglSetLayerPaletteEntries;
	qwglShareLists               = wglShareLists;
	qwglSwapLayerBuffers         = wglSwapLayerBuffers;
	qwglUseFontBitmaps           = wglUseFontBitmapsA;
	qwglUseFontOutlines          = wglUseFontOutlinesA;

	qwglChoosePixelFormat        = ChoosePixelFormat;
	qwglDescribePixelFormat      = DescribePixelFormat;
	qwglGetPixelFormat           = GetPixelFormat;
	qwglSetPixelFormat           = SetPixelFormat;
	qwglSwapBuffers              = SwapBuffers;

	return true;
}


/*
==================
GLimp_EnableLogging

==================
*/
void GLimp_EnableLogging( bool enable ) {
	
}
