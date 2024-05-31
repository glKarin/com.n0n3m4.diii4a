/*
 * Copyright (C) 2010 Yamagi Burmeister
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This is the client side of the render backend, implemented trough SDL.
 * The SDL window and related functrion (mouse grap, fullscreen switch)
 * are implemented here, everything else is in the renderers.
 *
 * =======================================================================
 */

// client
#include "../../common/header/common.h"
#include "../../client/vid/header/ref.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>

float glimp_refreshRate = -1.0f;

static cvar_t *vid_displayrefreshrate;
static cvar_t *vid_displayindex;
static cvar_t *vid_highdpiaware;
static cvar_t *vid_rate;

static int last_flags = 0;
static int last_display = 0;
static int last_position_x = 0;
static int last_position_y = 0;
static volatile ANativeWindow *win;
static bool window_seted = false;
static qboolean initSuccessful = false;
static char **displayindices = NULL;
static int num_displays = 0;

extern int screen_width;
extern int screen_height;

extern int gl_format;
extern int gl_msaa;

extern volatile ANativeWindow * Android_GetWindow(void);

void (* GLref_AndroidInit)(volatile ANativeWindow *w);
void (* GLref_AndroidQuit)(void);
typedef void (*RI_SetGLParms)(int f, int msaa);
typedef void (*RI_SetResolution)(int aw,int ah);

void GLimp_SetRef(void *init, void *quit, void *setGLParms, void *setResolution)
{
    GLref_AndroidInit = init;
	GLref_AndroidQuit = quit;
    ((RI_SetGLParms)setGLParms)(gl_format, gl_msaa);
	((RI_SetResolution)setResolution)(screen_width, screen_height);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(!w)
		return;
	if(!window_seted)
	{
		win = w;
		ANativeWindow_acquire((ANativeWindow *)win);
		window_seted = true;
		return;
	}
}

void GLimp_AndroidQuit(void)
{
	win = NULL;
	if(GLref_AndroidQuit)
		GLref_AndroidQuit();
}

void GLimp_WindowChanged(volatile ANativeWindow *w)
{
	win = w;
    if(GLref_AndroidInit)
	    GLref_AndroidInit(win);
}

/*
 * Resets the display index Cvar if out of bounds
 */
static void
ClampDisplayIndexCvar(void)
{
	if (!vid_displayindex)
	{
		// uninitialized render?
		return;
	}

	if (vid_displayindex->value < 0 || vid_displayindex->value >= num_displays)
	{
		Cvar_SetValue("vid_displayindex", 0);
	}
}

static void
ClearDisplayIndices(void)
{
	if ( displayindices )
	{
		for ( int i = 0; i < num_displays; i++ )
		{
			free( displayindices[ i ] );
		}

		free( displayindices );
		displayindices = NULL;
	}
}

static qboolean
CreateAndroidWindow(int flags, int w, int h)
{
	if(!win)
		win = Android_GetWindow();
	if (win)
	{
		/* save current display as default */
		last_display = 0;

		/* Normally SDL stays at desktop refresh rate or chooses something
		   sane. Some player may want to override that.

		   Reminder: SDL_WINDOW_FULLSCREEN_DESKTOP implies SDL_WINDOW_FULLSCREEN! */
	}
	else
	{
		Com_Printf("Creating window failed: NULL\n");
		return false;
	}

	return true;
}

static int
GetFullscreenType()
{
	return 1;
}

static qboolean
GetWindowSize(int* w, int* h)
{
	*w = screen_width;
	*h = screen_height;

	return true;
}

static void
InitDisplayIndices()
{
	displayindices = malloc((num_displays + 1) * sizeof(char *));

	for ( int i = 0; i < num_displays; i++ )
	{
		/* There are a maximum of 10 digits in 32 bit int + 1 for the NULL terminator. */
		displayindices[ i ] = malloc(11 * sizeof( char ));
		YQ2_COM_CHECK_OOM(displayindices[i], "malloc()", 11 * sizeof( char ))

		snprintf( displayindices[ i ], 11, "%d", i );
	}

	/* The last entry is NULL to indicate the list of strings ends. */
	displayindices[ num_displays ] = 0;
}

/*
 * Lists all available display modes.
 */
static void
PrintDisplayModes(void)
{
}

// FIXME: We need a header for this.
// Maybe we could put it in vid.h.
void GLimp_GrabInput(qboolean grab);

/*
 * Shuts the SDL render backend down
 */
static void
ShutdownGraphics(void)
{
	ClampDisplayIndexCvar();

	if (win)
	{
		/* save current display as default */
		last_display = 0;

		/* cleanly ungrab input (needs window) */
		GLimp_GrabInput(false);

		win = NULL;
	}

	// make sure that after vid_restart the refreshrate will be queried from SDL2 again.
	glimp_refreshRate = -1;

	initSuccessful = false; // not initialized anymore
}
// --------

/*
 * Initializes the SDL video subsystem. Must
 * be called before anything else.
 */
qboolean
GLimp_Init(void)
{
	vid_displayrefreshrate = Cvar_Get("vid_displayrefreshrate", "-1", CVAR_ARCHIVE);
	vid_displayindex = Cvar_Get("vid_displayindex", "0", CVAR_ARCHIVE);
	vid_highdpiaware = Cvar_Get("vid_highdpiaware", "0", CVAR_ARCHIVE);
	vid_rate = Cvar_Get("vid_rate", "-1", CVAR_ARCHIVE);

	num_displays = 1;
	InitDisplayIndices();
	ClampDisplayIndexCvar();
	Com_Printf("Android display modes:\n");

	PrintDisplayModes();
	Com_Printf("------------------------------------\n\n");

	return true;
}

/*
 * Shuts the SDL video subsystem down. Must
 * be called after evrything's finished and
 * clean up.
 */
void
GLimp_Shutdown(void)
{
	ShutdownGraphics();

	ClearDisplayIndices();
}

/*
 * Determine if we want to be high dpi aware. If
 * we are we must scale ourself. If we are not the
 * compositor might scale us.
 */
static int
Glimp_DetermineHighDPISupport(int flags)
{
	return flags;
}

/*
 * (Re)initializes the actual window.
 */
qboolean
GLimp_InitGraphics(int fullscreen, int *pwidth, int *pheight)
{
	int flags;
	int curWidth, curHeight;
	int width = *pwidth;
	int height = *pheight;
	unsigned int fs_flag = 0;

	/* Only do this if we already have a working window and a fully
	initialized rendering backend GLimp_InitGraphics() is also
	called when recovering if creating GL context fails or the
	one we got is unusable. */
	Cvar_SetValue("vid_fullscreen", fullscreen);

	/* Let renderer prepare things (set OpenGL attributes).
	   FIXME: This is no longer necessary, the renderer
	   could and should pass the flags when calling this
	   function. */
	flags = re.PrepareForWindow();

	if (flags == -1)
	{
		/* It's PrepareForWindow() job to log an error */
		return false;
	}

	if (fs_flag)
	{
		flags |= fs_flag;
	}

	/* Check for high dpi support. */
	flags = Glimp_DetermineHighDPISupport(flags);

	/* Mkay, now the hard work. Let's create the window. */
	cvar_t *gl_msaa_samples = Cvar_Get("r_msaa_samples", "0", CVAR_ARCHIVE);

	CreateAndroidWindow(flags, width, height);

	last_flags = flags;

	/* Now that we've got a working window print it's mode. */
	int curdisplay = 0;

	if (curdisplay < 0) {
		curdisplay = 0;
	}

    /* Initialize rendering context. */
	if (!re.InitContext(win))
	{
		/* InitContext() should have logged an error. */
		return false;
	}

	/* We need the actual drawable size for things like the
	   console, the menus, etc. This might be different to
	   the resolution due to high dpi awareness.

	   The fullscreen window is special. We want it to fill
	   the screen when native resolution is requestes, all
	   other cases should look broken. */
	re.GetDrawableSize(&viddef.width, &viddef.height);

	Com_Printf("Drawable size: %ix%i\n", viddef.width, viddef.height);

	/* No cursor */
	//SDL_ShowCursor(0);

	initSuccessful = true;

	return true;
}

/*
 * Shuts the window down.
 */
void
GLimp_ShutdownGraphics(void)
{
	ShutdownGraphics();
}

/*
 * (Un)grab Input
 */
void
GLimp_GrabInput(qboolean grab)
{
	Android_GrabMouseCursor(grab);
}

/*
 * Returns the current display refresh rate.
 */
float
GLimp_GetRefreshRate(void)
{
	return glimp_refreshRate;
}

/*
 * Detect current desktop mode
 */
qboolean
GLimp_GetDesktopMode(int *pwidth, int *pheight)
{
	*pwidth = screen_width;
	*pheight = screen_height;
	return true;
}

const char**
GLimp_GetDisplayIndices(void)
{
	return (const char**)displayindices;
}

int
GLimp_GetNumVideoDisplays(void)
{
	return 1;
}

int
GLimp_GetWindowDisplayIndex(void)
{
	return 0;
}

int
GLimp_GetFrameworkVersion(void)
{
	return 2;
}
