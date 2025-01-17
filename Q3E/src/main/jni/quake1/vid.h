/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// vid.h -- video driver defs

#ifndef VID_H
#define VID_H

#include <stddef.h>
#include "qtypes.h"
struct cmd_state_s;

#define ENGINE_ICON ( (gamemode == GAME_NEXUIZ) ? nexuiz_xpm : darkplaces_xpm )

extern int cl_available;

#define MAX_TEXTUREUNITS 32

typedef enum renderpath_e
{
	RENDERPATH_GL32,
	RENDERPATH_GLES2
}
renderpath_t;

typedef struct viddef_support_s
{
	int glversion; ///< this is at least 32
	int glshaderversion; ///< this is at least 150 (GL 3.2)
	qbool amd_texture_texture4;
	qbool arb_texture_gather;
	qbool ext_texture_compression_s3tc;
	qbool ext_texture_filter_anisotropic;
	qbool ext_texture_srgb;
	qbool arb_debug_output;
}
viddef_support_t;

typedef struct viddef_mode_s
{
	int display;
	qbool fullscreen;
	qbool desktopfullscreen; ///< whether the display hardware mode can be changed
	int width;
	int height;
	int bitsperpixel;
	float refreshrate;
	qbool stereobuffer;
	int samples;
}
viddef_mode_t;

typedef struct viddef_s
{
	// these are set by VID_Mode
	// used in many locations in the renderer
	viddef_mode_t mode; ///< currently active video mode
	qbool stencil;
	qbool sRGB2D; ///< whether 2D rendering is sRGB corrected (based on sRGBcapable2D)
	qbool sRGB3D; ///< whether 3D rendering is sRGB corrected (based on sRGBcapable3D)
	qbool sRGBcapable2D; ///< whether 2D rendering can be sRGB corrected (renderpath)
	qbool sRGBcapable3D; ///< whether 3D rendering can be sRGB corrected (renderpath)

	renderpath_t renderpath;
	qbool allowalphatocoverage; ///< indicates the GL_AlphaToCoverage function works on this renderpath and framebuffer

	unsigned int maxtexturesize_2d;
	unsigned int maxtexturesize_3d;
	unsigned int maxtexturesize_cubemap;
	unsigned int max_anisotropy;
	unsigned int maxdrawbuffers;

	viddef_support_t support;

	int forcetextype; ///< always use GL_BGRA for D3D, always use GL_RGBA for GLES, etc

	int xPos, yPos; ///< current virtual position of the top left corner of the SDL window
} viddef_t;

/// global video state
extern viddef_t vid;

#define MAXJOYAXIS 16
// if this is changed, the corresponding code in vid_shared.c must be updated
#define MAXJOYBUTTON 36
typedef struct vid_joystate_s
{
	float axis[MAXJOYAXIS]; ///< -1 to +1
	unsigned char button[MAXJOYBUTTON]; ///< 0 or 1
	qbool is360; ///< indicates this joystick is a Microsoft Xbox 360 Controller For Windows
}
vid_joystate_t;

extern vid_joystate_t vid_joystate;

extern cvar_t joy_index;
extern cvar_t joy_enable;
extern cvar_t joy_detected;
extern cvar_t joy_active;

float VID_JoyState_GetAxis(const vid_joystate_t *joystate, int axis, float sensitivity, float deadzone);
void VID_ApplyJoyState(vid_joystate_t *joystate);
void VID_BuildJoyState(vid_joystate_t *joystate);
void VID_Shared_BuildJoyState_Begin(vid_joystate_t *joystate);
void VID_Shared_BuildJoyState_Finish(vid_joystate_t *joystate);
int VID_Shared_SetJoystick(int index);
qbool VID_JoyBlockEmulatedKeys(int keycode);
void VID_EnableJoystick(qbool enable);

extern cvar_t cl_demo_mousegrab;
extern qbool scr_loading;

extern qbool vid_hidden;
extern qbool vid_activewindow;
extern qbool vid_supportrefreshrate;

extern cvar_t vid_fullscreen;
extern cvar_t vid_borderless;
extern cvar_t vid_width;
extern cvar_t vid_height;
extern cvar_t vid_bitsperpixel;
extern cvar_t vid_samples;
extern cvar_t vid_refreshrate;
extern cvar_t vid_touchscreen_density;
extern cvar_t vid_touchscreen_xdpi;
extern cvar_t vid_touchscreen_ydpi;
extern cvar_t vid_vsync;
extern cvar_t vid_mouse;
extern cvar_t vid_mouse_clickthrough;
extern cvar_t vid_minimize_on_focus_loss;
extern cvar_t vid_grabkeyboard;
extern cvar_t vid_touchscreen;
extern cvar_t vid_touchscreen_showkeyboard;
extern cvar_t vid_touchscreen_supportshowkeyboard;
extern cvar_t vid_stick_mouse;
extern cvar_t vid_resizable;
extern cvar_t vid_desktopfullscreen;
extern cvar_t vid_display;
extern cvar_t vid_info_displaycount;
#ifdef WIN32
extern cvar_t vid_ignore_taskbar;
#endif
extern cvar_t vid_minwidth;
extern cvar_t vid_minheight;
extern cvar_t vid_sRGB;
extern cvar_t vid_sRGB_fallback;

extern cvar_t gl_finish;

extern cvar_t v_gamma;
extern cvar_t v_contrast;
extern cvar_t v_brightness;
extern cvar_t v_color_enable;
extern cvar_t v_color_black_r;
extern cvar_t v_color_black_g;
extern cvar_t v_color_black_b;
extern cvar_t v_color_grey_r;
extern cvar_t v_color_grey_g;
extern cvar_t v_color_grey_b;
extern cvar_t v_color_white_r;
extern cvar_t v_color_white_g;
extern cvar_t v_color_white_b;

extern cvar_t gl_info_vendor;
extern cvar_t gl_info_renderer;
extern cvar_t gl_info_version;
extern cvar_t gl_info_extensions;
extern cvar_t gl_info_driver;

/// brand of graphics chip
extern const char *gl_vendor;
/// graphics chip model and other information
extern const char *gl_renderer;
/// begins with 1.0.0, 1.1.0, 1.2.0, 1.2.1, 1.3.0, 1.3.1, or 1.4.0
extern const char *gl_version;


void *GL_GetProcAddress(const char *name);
qbool GL_CheckExtension(const char *name, const char *disableparm, int silent);
qbool GL_ExtensionSupported(const char *name);

void VID_Shared_Init(void);

void GL_InitFunctions(void);
void GL_Setup(void);

void VID_ClearExtensions(void);

/// Called at startup
void VID_Init (void);

/// Called at shutdown
void VID_Shutdown (void);

/// sets the mode; only used by the Quake engine for resetting to mode 0 (the
/// base mode) on memory allocation failures
int VID_SetMode (int modenum);

/// allocates and opens an appropriate OpenGL context (and its window)
qbool VID_InitMode(const viddef_mode_t *mode);


/// updates cachegamma variables and bumps vid_gammatables_serial if anything changed
/// (ONLY to be called from VID_Finish!)
void VID_UpdateGamma(void);

qbool VID_HasScreenKeyboardSupport(void);
void VID_ShowKeyboard(qbool show);
qbool VID_ShowingKeyboard(void);

void VID_Finish (void);

void VID_Restart_f(struct cmd_state_s *cmd);

void VID_Start(void);
void VID_Stop(void);

extern unsigned int vid_gammatables_serial; ///< so other subsystems can poll if gamma parameters have changed; this starts with 0 and gets increased by 1 each time the gamma parameters get changed and VID_BuildGammaTables should be called again
extern qbool vid_gammatables_trivial; ///< this is set to true if all color control values are at default setting, and it therefore would make no sense to use the gamma table
/// builds the current gamma tables into an array (needs 3*rampsize items)
void VID_BuildGammaTables(unsigned short *ramps, int rampsize);
/// applies current gamma settings to a color (0-1 range)
void VID_ApplyGammaToColor(const float *rgb, float *out);

typedef struct
{
	int width, height, bpp, refreshrate;
	int pixelheight_num, pixelheight_denom;
}
vid_mode_t;
vid_mode_t VID_GetDesktopMode(void);
size_t VID_ListModes(vid_mode_t *modes, size_t maxcount);
size_t VID_SortModes(vid_mode_t *modes, size_t count, qbool usebpp, qbool userefreshrate, qbool useaspect);
void VID_Soft_SharedSetup(void);

#endif

