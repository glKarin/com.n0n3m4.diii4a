/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
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
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#if !defined(USE_VULKAN)
#include <GL/glew.h>
#else
#include <GLES3/gl3.h>
#endif

#define Q3E_PRINTF common->Printf
#define Q3E_ERRORF common->Error
#define Q3E_DEBUGF Sys_Printf
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_POST_GL_INIT GLES_PostInit();

extern void GLES_PostInit(void);

#include "q3e/q3e_glimp.inc"

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include "renderer/RenderCommon.h"
#include "android_local.h"

#ifdef USE_VULKAN
extern void VKimp_RecreateSurfaceAndSwapchain(void);
extern void VKimp_WaitForStop(void);
#endif

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );
// Linux open source drivers suck
idCVar r_useOpenGL32( "r_useOpenGL32", "0", CVAR_INTEGER, "0 = OpenGL ES 3.0, 1 = OpenGL ES 3.1, 2 = OpenGL ES 3.2", 0, 2 );
// RB end

static bool grabbed = false;

extern void Android_GrabMouseCursor(bool grabIt);
extern void Sys_ForceResolution(void);

ANativeWindow * Android_GetVulkanWindow( void )
{
	return (ANativeWindow *)win;
}

/*
===============
GLES_Init
===============
*/

#if !defined(USE_VULKAN)
void GLimp_ActivateContext() {
	Q3E_ActivateContext();
}

void GLimp_DeactivateContext() {
	Q3E_DeactivateContext();
}
#endif

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
#if !defined(USE_VULKAN)
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
		Q3E_RestoreGL();
#else
	if(Q3E_RequireWindow(w))
		VKimp_RecreateSurfaceAndSwapchain();
#endif
}

void GLimp_AndroidQuit(void)
{
#if !defined(USE_VULKAN)
	Q3E_DestroyGL(true);
#else
	VKimp_WaitForStop();
#endif
}

bool GLimp_OpenDisplay(void)
{
#if !defined(USE_VULKAN)
	if(Q3E_HasDisplay()) {
		return true;
	}

	if (cvarSystem->GetCVarInteger("net_serverDedicated") == 1) {
		common->DPrintf("not opening the display: dedicated server\n");
		return false;
	}

	return Q3E_OpenDisplay();
#else
	return true;
#endif
}

void GLES_PostInit(void) {
	glConfig.isFullscreen = true;
	glConfig.isStereoPixelFormat = false;
	glConfig.nativeScreenWidth = screen_width;
	glConfig.nativeScreenHeight = screen_height;
	glConfig.displayFrequency = refresh_rate;
	glConfig.multisamples = Q3E_GL_CONFIG(samples) > 1 ? Q3E_GL_CONFIG(samples) / 2 : 0;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	if (glConfig.isFullscreen) {
		Sys_GrabMouseCursor(true);
	}
}

/*
===================
GLimp_PreInit

 R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
 So do that in GLimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void GLimp_PreInit() // DG: added this function for SDL compatibility
{
#ifdef __ANDROID__ //karin: force setup resolution on Android
	Sys_ForceResolution();
#endif
}


/*
===================
GLimp_Init
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#else

	common->Printf( "Initializing OpenGL subsystem\n" );

	common->Printf("  size=%d x %d; fullscreen=%d; displayHz=%d; multiSamples=%d; stereo=%d\n", parms.width, parms.height, parms.fullScreen, parms.displayHz, parms.multiSamples, parms.stereo);

	GLimp_PreInit(); // DG: make sure SDL is initialized

	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(samples, parms.multiSamples);
	Q3E_GL_CONFIG_SET(swap_interval, r_swapInterval.GetInteger());
	if(r_useOpenGL32.GetInteger() == 2)
	{
		Q3E_GL_CONFIG_ES_3_2();
	}
	else if(r_useOpenGL32.GetInteger() == 1)
	{
		Q3E_GL_CONFIG_ES_3_1();
	}
	else
	{
		Q3E_GL_CONFIG_ES_3_0();
	}

	if (!GLimp_OpenDisplay()) {
		return false;
	}

	if (!GLES_Init()) {
		return false;
	}

	glewExperimental = GL_TRUE; // GLES3.2
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

	return true;
#endif
}
/*
===================
 Helper functions for GLimp_SetScreenParms()
===================
*/

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#endif

	glConfig.isFullscreen = true;
	glConfig.isStereoPixelFormat = false;
	glConfig.nativeScreenWidth = screen_width;
	glConfig.nativeScreenHeight = screen_height;
	glConfig.displayFrequency = refresh_rate;
	glConfig.multisamples = Q3E_GL_CONFIG(samples) > 1 ? Q3E_GL_CONFIG(samples) / 2 : 0;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown()
{
#ifdef USE_VULKAN
	common->Printf( "Shutting down Vulkan subsystem\n" );
#else
	common->Printf( "Shutting down OpenGL subsystem\n" );

	Q3E_ShutdownGL();
#endif
}

/*
===================
GLimp_SwapBuffers
===================
*/
#ifndef USE_VULKAN
void GLimp_SwapBuffers()
{
	Q3E_SwapBuffers();
}
#endif

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
}

/*
===================
GLimp_ExtensionPointer
===================
*/
/*
GLExtension_t GLimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void GLimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	Android_GrabMouseCursor(grab);
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->DPrintf( "TODO: DumpAllDisplayDevices\n" );
}



class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode >
{
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

// RB: resolutions supported by XreaL
static void FillStaticVidModes( idList<vidMode_t>& modeList )
{
	modeList.AddUnique( vidMode_t( 320,   240, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 400,   300, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 512,   384, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 640,   480, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 800,   600, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 960,   720, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1024,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1152,  864, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  720, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  800, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1360,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1440,  900, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, refresh_rate ) );

	modeList.SortWithTemplate( idSort_VidMode() );
}

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	assert( requestedDisplayNum >= 0 );

	modeList.Clear();

	// DG: SDL1 only knows of one display - some functions rely on
	// R_GetModeListForDisplay() returning false for invalid displaynum to iterate all displays
	if( requestedDisplayNum >= 1 )
	{
		return false;
	}
	// DG end

	FillStaticVidModes( modeList );

	return true;
}
