/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file sdl_glimp.c
 */

#include <GLES/gl.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../sys/sys_local.h"
#include "../client/client.h"

static float         displayAspect = 0.f;

cvar_t *r_sdlDriver;
cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable

// Window cvars
cvar_t *r_fullscreen = 0;
cvar_t *r_noBorder;
cvar_t *r_centerWindow;
cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *r_swapInterval;
cvar_t *r_mode;
cvar_t *r_customaspect;
cvar_t *r_displayRefresh;
cvar_t *r_windowLocation;

// Window surface cvars
cvar_t *r_stencilbits;  // number of desired stencil bits
cvar_t *r_depthbits;  // number of desired depth bits
cvar_t *r_colorbits;  // number of desired color bits, only relevant for fullscreen
cvar_t *r_ignorehwgamma;

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_OLD_GL,

	RSERR_UNKNOWN
} rserr_t;

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define Q3E_PRINTF Com_Printf
#define Q3E_ERRORF(...) Com_Error(ERR_VID_FATAL, __VA_ARGS__)
#define Q3E_DEBUGF(...) fprintf(stderr, __VA_ARGS__)
#define Q3Ebool qboolean
#define Q3E_TRUE qtrue
#define Q3E_FALSE qfalse

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
    Q3E_DestroyGL(qtrue);
}

static void GLES_PostInit(glconfig_t *glConfig)
{
    glConfig->colorBits = Q3E_GL_CONFIG(red) + Q3E_GL_CONFIG(green) + Q3E_GL_CONFIG(blue);
    glConfig->stencilBits = Q3E_GL_CONFIG(stencil);
    glConfig->depthBits = Q3E_GL_CONFIG(depth);

    glConfig->isFullscreen = Q3E_GL_CONFIG(fullscreen);

	if (glConfig->isFullscreen) {
		//Sys_GrabMouseCursor(true);
	}
}

qboolean GLimp_InitGL(glconfig_t *glConfig, qboolean fullscreen)
{
    Q3E_GL_CONFIG_SET(fullscreen, 1);
    Q3E_GL_CONFIG_SET(swap_interval, r_swapInterval->integer);
    Q3E_GL_CONFIG_ES_1_1();

    qboolean res = Q3E_InitGL();
    if(res)
    {
        GLES_PostInit(glConfig);
    }
    return res;
}

typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t glimp_vidModes[] =        // keep in sync with LEGACY_RESOLUTIONS
{
	{ "Mode  0: 320x240",           320,  240,  1 },
	{ "Mode  1: 400x300",           400,  300,  1 },
	{ "Mode  2: 512x384",           512,  384,  1 },
	{ "Mode  3: 640x480",           640,  480,  1 },
	{ "Mode  4: 800x600",           800,  600,  1 },
	{ "Mode  5: 960x720",           960,  720,  1 },
	{ "Mode  6: 1024x768",          1024, 768,  1 },
	{ "Mode  7: 1152x864",          1152, 864,  1 },
	{ "Mode  8: 1280x1024",         1280, 1024, 1 },
	{ "Mode  9: 1600x1200",         1600, 1200, 1 },
	{ "Mode 10: 2048x1536",         2048, 1536, 1 },
	{ "Mode 11: 856x480 (16:9)",    856,  480,  1 },
	{ "Mode 12: 1366x768 (16:9)",   1366, 768,  1 },
	{ "Mode 13: 1440x900 (16:10)",  1440, 900,  1 },
	{ "Mode 14: 1680x1050 (16:10)", 1680, 1050, 1 },
	{ "Mode 15: 1600x1200",         1600, 1200, 1 },
	{ "Mode 16: 1920x1080 (16:9)",  1920, 1080, 1 },
	{ "Mode 17: 1920x1200 (16:10)", 1920, 1200, 1 },
	{ "Mode 18: 2560x1440 (16:9)",  2560, 1440, 1 },
	{ "Mode 19: 2560x1600 (16:10)", 2560, 1600, 1 },
	{ "Mode 20: 3840x2160 (16:9)",  3840, 2160, 1 },
};
static int s_numVidModes = ARRAY_LEN(glimp_vidModes);

static void GLimp_ParseConfigString(const char *glConfigString, char *type, int *major, int *minor, int *context, int *samples)
{
	const char *value;

	// Default values
	type[0]  = '\0';
	*major   = 1;
	*minor   = 0;
	*context = 0;
	*samples = 0;

	Com_DPrintf("GLimp_ParseConfigString: %s\n", glConfigString);
	value = Info_ValueForKey(glConfigString, "type");

	if (value && *value)
	{
		Q_strncpyz(type, value, 32);
	}
	else
	{
		Q_strncpyz(type, "opengl", 32);
	}

	value  = Info_ValueForKey(glConfigString, "major");
	*major = value && *value ? Q_atoi(value) : 1;

	value  = Info_ValueForKey(glConfigString, "minor");
	*minor = value && *value ? Q_atoi(value) : 0;

	value    = Info_ValueForKey(glConfigString, "context");
	*context = value && *value ? Q_atoi(value) : 0;

	value    = Info_ValueForKey(glConfigString, "samples");
	*samples = value && *value ? Q_atoi(value) : 0;
}

/**
 * @brief GLimp_MainWindow
 * @return
 */
void *GLimp_MainWindow(void)
{
	return (void *)win;
}

/**
 * @brief Minimize the game so that user is back at the desktop
 */
void GLimp_Minimize(void)
{
}

/**
 * @brief Flash the game window in the taskbar to alert user of an event
 * @param[in] state - SDL_FlashOperation
 */
void GLimp_FlashWindow(int state)
{
}

/**
 * @brief GLimp_GetModeInfo
 * @param[in,out] width
 * @param[in,out] height
 * @param[out] windowAspect
 * @param[in] mode
 * @return
 */
qboolean GLimp_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
{
	vidmode_t *vm;
	float     pixelAspect;

	if (mode < -1)
	{
		return qfalse;
	}
	if (mode >= s_numVidModes)
	{
		return qfalse;
	}

	if (mode == -1)
	{
		*width      = r_customwidth->integer;
		*height     = r_customheight->integer;
		pixelAspect = r_customaspect->value;
	}
	else
	{
		vm = &glimp_vidModes[mode];

		*width      = vm->width;
		*height     = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / (*height * pixelAspect);

	return qtrue;
}

#define GLimp_ResolutionToFraction(resolution) GLimp_RatioToFraction((double) resolution.w / (double) resolution.h, 150)

/**
 * @brief Figures out the best possible fraction string for the given resolution ratio
 * @param ratio resolution ratio (resolution width / resolution height)
 * @param iterations how many iterations should it be allowed to run
 * @return fraction value formatted into a string (static stack)
 */
static char *GLimp_RatioToFraction(const double ratio, const int iterations)
{
	static char  buff[64];
	int          i;
	double       bestDelta       = DBL_MAX;
	unsigned int numerator       = 1;
	unsigned int denominator     = 1;
	unsigned int bestNumerator   = 0;
	unsigned int bestDenominator = 0;

	for (i = 0; i < iterations; i++)
	{
		double delta = (double) numerator / (double) denominator - ratio;
		double newDelta;

		// Close enough for most resolutions
		if (fabs(delta) < 0.002)
		{
			break;
		}

		if (delta < 0)
		{
			numerator++;
		}
		else
		{
			denominator++;
		}

		newDelta = fabs((double) numerator / (double) denominator - ratio);
		if (newDelta < bestDelta)
		{
			bestDelta       = newDelta;
			bestNumerator   = numerator;
			bestDenominator = denominator;
		}
	}

	Com_sprintf(buff, sizeof(buff), "%u/%u", bestNumerator, bestDenominator);
	Com_DPrintf("%f -> %s\n", ratio, buff);
	return buff;
}

/**
* @brief Prints hardcoded screen resolutions
* @see r_availableModes for supported resolutions
*/
void GLimp_ModeList_f(void)
{
	Com_Printf("\n");
	Com_Printf((r_mode->integer == -1) ? "%s ^2(current)\n" : "%s\n",
	           "Mode -1: custom resolution");

	Com_Printf("\n");
}

/**
 * @brief GLimp_InitCvars
 */
static void GLimp_InitCvars(void)
{
	r_sdlDriver = Cvar_Get("r_sdlDriver", "", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	Cvar_GetAndDescribe("r_sdlDriver", "", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE,
	                    "Sets the video driver used by SDL, e.g. \"x11\" or \"wayland\". Restart required.");

	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_allowResize     = Cvar_Get("r_allowResize", "0", CVAR_ARCHIVE);

	// Window cvars
	r_fullscreen     = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_noBorder       = Cvar_Get("r_noborder", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_centerWindow   = Cvar_Get("r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth    = Cvar_Get("r_customwidth", "1280", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight   = Cvar_Get("r_customheight", "720", CVAR_ARCHIVE | CVAR_LATCH);
	r_swapInterval   = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_mode           = Cvar_Get("r_mode", "-1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_customaspect   = Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH);
	Cvar_CheckRange(r_displayRefresh, 0, 480, qtrue);
	r_windowLocation = Cvar_Get("r_windowLocation", "0,-1,-1", CVAR_ARCHIVE | CVAR_PROTECTED);

	// Window render surface cvars
	r_stencilbits = Cvar_Get("r_stencilbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_depthbits   = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	Cvar_CheckRange(r_depthbits, 0, 24, qtrue);
	r_colorbits     = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);

	// Old modes (these are used by the UI code)
	Cvar_Get("r_oldFullscreen", "", CVAR_ARCHIVE);
	Cvar_Get("r_oldMode", "", CVAR_ARCHIVE);

	Cmd_AddCommand("modelist", GLimp_ModeList_f, "Prints a list of available resolutions/modes.");
	Cmd_AddCommand("minimize", GLimp_Minimize, "Minimizes the game window.");
}

/**
 * @brief GLimp_Shutdown
 */
void GLimp_Shutdown(void)
{
	IN_Shutdown();

    Q3E_ShutdownGL();

	Cmd_RemoveCommand("modelist");
	Cmd_RemoveCommand("minimize");

	Com_Printf("[Harmattan]: EGL destroyed.\n");
}

/**
 * @brief GLimp_SetMode
 * @param[in,out] glConfig
 * @param[in] mode
 * @param[in] fullscreen
 * @param[in] noborder
 * @param[in] context
 * @return
 */
static int GLimp_SetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, const char *glConfigString)
{
	char            type[32];
	int             major, minor, contextVersion, samples;

	GLimp_ParseConfigString(glConfigString, type, &major, &minor, &contextVersion, &samples);

	glConfig->vidWidth  = screen_width;
	glConfig->vidHeight = screen_height;

	glConfig->windowAspect = (float)glConfig->vidWidth / (float)glConfig->vidHeight;

	glConfig->windowWidth  = glConfig->vidWidth;
	glConfig->windowHeight = glConfig->vidHeight;

	glConfig->isFullscreen = qtrue;

    Q3E_GL_CONFIG_SET(samples, samples);

	GLimp_InitGL(glConfig, glConfig->isFullscreen);

	return RSERR_OK;
}

/**
 * @brief GLimp_StartDriverAndSetMode
 * @param[in] glConfig
 * @param[in] mode
 * @param[in] fullscreen
 * @param[in] noborder
 * @param[in] context
 * @return
 */
static qboolean GLimp_StartDriverAndSetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, const char *glConfigString)
{
	rserr_t err;

	err = GLimp_SetMode(glConfig, mode, fullscreen, noborder, glConfigString);

	return err == RSERR_OK ? qtrue : qfalse;
}

#define R_MODE_FALLBACK 4 // 800 * 600

/**
 * @brief This routine is responsible for initializing the OS specific portions of OpenGL
 * @param[in,out] glConfig
 * @param[in] context
 */
void GLimp_Init(glconfig_t *glConfig, const char *glConfigString)
{
	GLimp_InitCvars();

	if (Cvar_VariableIntegerValue("com_abnormalExit"))
	{
		Cvar_Set("r_mode", "-1");
		Cvar_Set("r_fullscreen", "1");
		Cvar_Set("r_centerWindow", "0");
		Cvar_Set("com_abnormalExit", "0");
	}

	Sys_GLimpInit();

	// Create the window and set up the context
	if (GLimp_StartDriverAndSetMode(glConfig, r_mode->integer, (qboolean) !!r_fullscreen->integer, (qboolean) !!r_noBorder->integer, glConfigString))
	{
		goto success;
	}

	// Nothing worked, give up
	Com_Error(ERR_VID_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n");

success:
	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig->deviceSupportsGamma = qfalse;

	re.InitOpenGL();

	Cvar_Get("r_availableModes", "", CVAR_ROM);

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init();
}

/**
 * @brief Responsible for doing a swapbuffers
 */
void GLimp_EndFrame(void)
{
	// don't flip if drawing to front buffer
	//FIXME: remove this nonesense
	if (Q_stricmp(Cvar_VariableString("r_drawBuffer"), "GL_FRONT") != 0)
	{
        Q3E_SwapBuffers();
	}

	if (r_fullscreen->modified)
	{
		r_fullscreen->modified = qfalse;
	}
}

/**
 * @brief GLimp_SetGamma
 * @param[in] red
 * @param[in] green
 * @param[in] blue
 */
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
}

qboolean GLimp_SplashImage(qboolean (*LoadSplashImage)(const char *name, byte *data, unsigned int size, unsigned int width, unsigned int height, uint8_t bytes))
{
	return qfalse;
}
