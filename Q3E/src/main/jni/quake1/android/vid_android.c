/*
Copyright (C) 2003  T. Joseph Carter

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>

#include "../quakedef.h"
#include "../image.h"
#include "../utf8lib.h"


// Tell startup code that we have a client
int cl_available = true;

qbool vid_supportrefreshrate = false;

static qbool vid_usingmouse = false;
static qbool vid_usingmouse_relativeworks = false; // SDL2 workaround for unimplemented RelativeMouse mode
static qbool vid_usinghidecursor = false;
static qbool vid_hasfocus = false;
static qbool vid_wmborder_waiting, vid_wmborderless;
// GAME_STEELSTORM specific
static cvar_t *steelstorm_showing_map = NULL; // detect but do not create the cvar
static cvar_t *steelstorm_showing_mousecursor = NULL; // detect but do not create the cvar
extern void Android_OpenKeyboard(void);
extern void Android_CloseKeyboard(void);
extern void Android_PollInput(void);
extern void Sys_SyncState(void);
extern void Android_GrabMouseCursor(qbool grabIt);

qbool vid_restart = false;



#define Q3E_PRINTF Con_Printf
#define Q3E_ERRORF Sys_Error
#define Q3E_DEBUGF printf
#define Q3Ebool qbool
#define Q3E_TRUE true
#define Q3E_FALSE false

#include "q3e/q3e_glimp.inc"

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
		Q3E_RestoreGL();
}

void GLimp_AndroidQuit(void)
{
	Q3E_DestroyGL(true);
}

/*
===============
GLES_Init
===============
*/
/*
static void GLES_PostInit(void)
{
	gl_state.fullscreen = true;

	if (gl_state.fullscreen) {
		//Sys_GrabMouseCursor(true);
	}
}
*/

qbool GLimp_InitGL(qbool fullscreen)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(swap_interval, -1);
	Q3E_GL_CONFIG_ES_2_0();

	qbool res = Q3E_InitGL();
	return res;
}

/*
 * (Un)grab Input
 */
void
GLimp_GrabInput(qbool grab)
{
	if(Android_GrabMouseCursor)
		(*Android_GrabMouseCursor)(grab);
}



// Input handling

static void VID_SetMouse(qbool relative, qbool hidecursor)
{
	if (vid_usingmouse != relative)
	{
		vid_usingmouse = relative;
		cl_ignoremousemoves = 2;
/*		vid_usingmouse_relativeworks = SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
//		Con_Printf("VID_SetMouse(%i, %i) relativeworks = %i\n", (int)relative, (int)hidecursor, (int)vid_usingmouse_relativeworks);*/
	}
	if (vid_usinghidecursor != hidecursor)
	{
		vid_usinghidecursor = hidecursor;
		Android_GrabMouseCursor(hidecursor);
	}
}

void VID_BuildJoyState(vid_joystate_t *joystate)
{
	(void)joystate;
}

/////////////////////
// Movement handling
////

void IN_Move( void )
{
	//vid_joystate_t joystate;

	scr_numtouchscreenareas = 0;

	Sys_SyncState();

	//Con_Printf("Mouse position: in_mouse %f %f in_windowmouse %f %f\n", in_mouse_x, in_mouse_y, in_windowmouse_x, in_windowmouse_y);



	//VID_BuildJoyState(&joystate);
	//VID_ApplyJoyState(&joystate);
}

/////////////////////
// Message Handling
////

//#define DEBUGSDLEVENTS
void Sys_SDL_HandleEvents(void)
{
	VID_EnableJoystick(true);

	Android_PollInput();

	vid_activewindow = !vid_hidden && vid_hasfocus;

	if (!vid_activewindow || key_consoleactive || scr_loading)
		VID_SetMouse(false, false);
	else if (key_dest == key_menu || key_dest == key_menu_grabbed)
		VID_SetMouse(vid_mouse.integer && !in_client_mouse && !vid_touchscreen.integer, !vid_touchscreen.integer);
	else
		VID_SetMouse(vid_mouse.integer && !cl.csqc_wantsmousemove && cl_prydoncursor.integer <= 0 && (!cls.demoplayback || cl_demo_mousegrab.integer) && !vid_touchscreen.integer, !vid_touchscreen.integer);
}

/////////////////
// Video system
////

void *GL_GetProcAddress(const char *name)
{
	void *p = NULL;
	p = Q3E_GET_PROC_ADDRESS(name);
	return p;
}

qbool GL_ExtensionSupported(const char *name)
{
	const char *exts = (const char *)qglGetString (GL_EXTENSIONS);

	size_t size = strlen(exts) + 1 + 1;
	char *new_exts = malloc(size);
	dp_strlcpy(new_exts, exts, size - 2);
	new_exts[size - 2] = ' ';
	new_exts[size - 1] = '\0';

	size_t name_size = strlen(name) + 1 + 1;
	char *new_name = malloc(name_size);
	dp_strlcpy(new_name, name, name_size - 2);
	new_name[name_size - 2] = ' ';
	new_name[name_size - 1] = '\0';

	qbool has = strstr(new_exts, new_name) != NULL;
	Con_Printf("[Harmattan]: OpenGL extension '%s' -> %s\n", name, has ? "support" : "missing");
	free(new_exts);
	free(new_name);
	return has;
}

static void VID_SetVsync_c(cvar_t *var)
{
	int vsyncwanted = cls.timedemo ? 0 : vid_vsync.integer;
	if(vsyncwanted < 0)
	{
		Con_Printf("Don't support swap interval value %d, change to 1\n", vsyncwanted);
		vsyncwanted = 1;
	}
/*
Can't check first: on Wayland SDL_GL_GetSwapInterval() may initially return 0 when vsync is on.
On Xorg it returns the correct value.
	if (SDL_GL_GetSwapInterval() == vsyncwanted)
		return;
*/

	// __EMSCRIPTEN__ SDL_GL_SetSwapInterval() calls emscripten_set_main_loop_timing()
	if (Q3E_SwapInterval(vsyncwanted))
		Con_DPrintf("Vsync %s\n", vsyncwanted ? "activated" : "deactivated");
	else
		Con_Printf(CON_ERROR "ERROR: can't %s vsync because %s\n", vsyncwanted ? "activate" : "deactivate", Q3E_GetError());
}

void VID_Init (void)
{
	// DPI scaling prevents use of the native resolution, causing blurry rendering
	// and/or mouse cursor problems and/or incorrect render area, so we need to opt-out.
	// Must be set before first SDL_INIT_VIDEO. Documented in SDL_hints.h.
	Cvar_RegisterCallback(&vid_vsync,                  VID_SetVsync_c);
}

void VID_EnableJoystick(qbool enable)
{
	if (joy_active.integer)
		Cvar_SetValueQuick(&joy_active, 0);
}

static qbool VID_InitModeGL(const viddef_mode_t *mode)
{
	int i;

	// video display selection (multi-monitor)
	Cvar_SetValueQuick(&vid_info_displaycount, 0);
	vid.mode.display = bound(0, mode->display, vid_info_displaycount.integer - 1);
	vid.xPos = 0;
	vid.yPos = 0;
	vid.mode.fullscreen = vid.mode.desktopfullscreen = true;
	vid.mode.stereobuffer = false;
	vid.mode.width = mode->width;
	vid.mode.height = mode->height;
	vid.mode.bitsperpixel = mode->bitsperpixel;
	vid.mode.refreshrate  = mode->refreshrate && mode->fullscreen && !mode->desktopfullscreen ? 60 : 0;
	vid.stencil           = mode->bitsperpixel > 16;
	vid_wmborder_waiting = vid_wmborderless = false;

    Q3E_GL_CONFIG_SET(samples, mode->samples);

	if(!GLimp_InitGL(true))
		return false;

	GL_InitFunctions();

	// apply vid_vsync
	Cvar_Callback(&vid_vsync);

	vid_hidden = false;
	vid_activewindow = true;
	vid_hasfocus = true;
	vid_usingmouse = false;
	vid_usinghidecursor = false;

	// clear to black (loading plaque will be seen over this)
	GL_Clear(GL_COLOR_BUFFER_BIT, NULL, 1.0f, 0);
	VID_Finish(); // checks vid_hidden

	GL_Setup();

	// VorteX: set other info
	Cvar_SetQuick(&gl_info_vendor, gl_vendor);
	Cvar_SetQuick(&gl_info_renderer, gl_renderer);
	Cvar_SetQuick(&gl_info_version, gl_version);
	Cvar_SetQuick(&gl_info_driver, "");

	return true;
}

qbool VID_InitMode(const viddef_mode_t *mode)
{
	// GAME_STEELSTORM specific
	steelstorm_showing_map = Cvar_FindVar(&cvars_all, "steelstorm_showing_map", ~0);
	steelstorm_showing_mousecursor = Cvar_FindVar(&cvars_all, "steelstorm_showing_mousecursor", ~0);

	Cvar_SetValueQuick(&vid_touchscreen_supportshowkeyboard, 1);
	return VID_InitModeGL(mode);
}

void VID_Shutdown (void)
{
	VID_EnableJoystick(false);
	VID_SetMouse(false, false);

	Q3E_DestroyGL(!vid_restart);
	if(vid_restart)
	{
		vid_restart = false; // reset state
		printf("Keep ANativeWindow for vid restart.\n");
	}
}

void VID_Finish (void)
{
	VID_UpdateGamma();

	if (!vid_hidden)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if (r_speeds.integer == 2 || gl_finish.integer)
				GL_Finish();
			Q3E_SwapBuffers();
			break;
		}
	}
}

vid_mode_t VID_GetDesktopMode(void)
{
	int bpp = 1;
	vid_mode_t desktop_mode;

	desktop_mode.width = screen_width;
	desktop_mode.height = screen_height;
	desktop_mode.bpp = bpp;
	desktop_mode.refreshrate = 60;
	desktop_mode.pixelheight_num = 1;
	desktop_mode.pixelheight_denom = 1; // SDL does not provide this
	return desktop_mode;
}

size_t VID_ListModes(vid_mode_t *modes, size_t maxcount)
{
	(void)modes;
	(void)maxcount;
	return 0;
}
