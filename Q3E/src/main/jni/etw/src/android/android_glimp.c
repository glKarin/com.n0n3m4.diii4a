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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../sys/sys_local.h"
#include "../client/client.h"

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

#include <android/native_window.h>
#include <android/native_window_jni.h>

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

// OpenGL attributes
extern int gl_format;
extern int gl_msaa;
extern int screen_width;
extern int screen_height;

#define MAX_NUM_CONFIGS 1000
static bool window_seted = false;
static volatile ANativeWindow *win;
//volatile bool has_gl_context = false;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLConfig configs[1];
static EGLConfig eglConfig = 0;
static EGLint format = WINDOW_FORMAT_RGBA_8888; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

static void GLimp_HandleError(const char *func, qboolean exit)
{
	static const char *GLimp_StringErrors[] = {
		"EGL_SUCCESS",
		"EGL_NOT_INITIALIZED",
		"EGL_BAD_ACCESS",
		"EGL_BAD_ALLOC",
		"EGL_BAD_ATTRIBUTE",
		"EGL_BAD_CONFIG",
		"EGL_BAD_CONTEXT",
		"EGL_BAD_CURRENT_SURFACE",
		"EGL_BAD_DISPLAY",
		"EGL_BAD_MATCH",
		"EGL_BAD_NATIVE_PIXMAP",
		"EGL_BAD_NATIVE_WINDOW",
		"EGL_BAD_PARAMETER",
		"EGL_BAD_SURFACE",
		"EGL_CONTEXT_LOST",
	};
	GLint err = eglGetError();
	if(err == EGL_SUCCESS)
		return;

	if(exit)
		Com_Error(ERR_VID_FATAL, "[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	else
		Com_Printf("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
}

typedef struct EGLConfigInfo_s
{
	EGLint red;
	EGLint green;
	EGLint blue;
	EGLint alpha;
	EGLint buffer;
	EGLint depth;
	EGLint stencil;
	EGLint samples;
	EGLint sample_buffers;
} EGLConfigInfo_t;

static EGLConfigInfo_t GLimp_FormatInfo(int format)
{
	int red_bits = 8;
	int green_bits = 8;
	int blue_bits = 8;
	int alpha_bits = 8;
	int buffer_bits = 32;
	int depth_bits = 24;
	int stencil_bits = 8;

	switch(gl_format)
	{
		case GLFORMAT_RGB565:
			red_bits = 5;
			green_bits = 6;
			blue_bits = 5;
			alpha_bits = 0;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA4444:
			red_bits = 4;
			green_bits = 4;
			blue_bits = 4;
			alpha_bits = 4;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA5551:
			red_bits = 5;
			green_bits = 5;
			blue_bits = 5;
			alpha_bits = 1;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA1010102:
			red_bits = 10;
			green_bits = 10;
			blue_bits = 10;
			alpha_bits = 2;
			depth_bits = 24;
			buffer_bits = 32;
			break;
		case GLFORMAT_RGBA8888:
		default:
			red_bits = 8;
			green_bits = 8;
			blue_bits = 8;
			alpha_bits = 8;
			depth_bits = 24;
			buffer_bits = 32;
			break;
	}
	EGLConfigInfo_t info;
	info.red = red_bits;
	info.green = green_bits;
	info.blue = blue_bits;
	info.alpha = alpha_bits;
	info.buffer = buffer_bits;
	info.depth = depth_bits;
	info.stencil = stencil_bits;
	return info;
}

static int GLimp_EGLConfigCompare(const void *left, const void *right)
{
	const EGLConfig lhs = *(EGLConfig *)left;
	const EGLConfig rhs = *(EGLConfig *)right;
	EGLConfigInfo_t info = GLimp_FormatInfo(gl_format);
	int r = info.red;
	int g = info.green;
	int b = info.blue;
	int a = info.alpha;
	int d = info.depth;
	int s = info.stencil;

	int lr, lg, lb, la, ld, ls;
	int rr, rg, rb, ra, rd, rs;
	int rat1, rat2;
	eglGetConfigAttrib(eglDisplay, lhs, EGL_RED_SIZE, &lr);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_GREEN_SIZE, &lg);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_BLUE_SIZE, &lb);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_ALPHA_SIZE, &la);
	//eglGetConfigAttrib(eglDisplay, lhs, EGL_DEPTH_SIZE, &ld);
	//eglGetConfigAttrib(eglDisplay, lhs, EGL_STENCIL_SIZE, &ls);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_RED_SIZE, &rr);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_GREEN_SIZE, &rg);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_BLUE_SIZE, &rb);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_ALPHA_SIZE, &ra);
	//eglGetConfigAttrib(eglDisplay, rhs, EGL_DEPTH_SIZE, &rd);
	//eglGetConfigAttrib(eglDisplay, rhs, EGL_STENCIL_SIZE, &rs);
	rat1 = (abs(lr - r) + abs(lg - g) + abs(lb - b));//*1000000-(ld*10000+la*100+ls);
	rat2 = (abs(rr - r) + abs(rg - g) + abs(rb - b));//*1000000-(rd*10000+ra*100+rs);
	return rat1 - rat2;
}

static EGLConfig GLimp_ChooseConfig(EGLConfig configs[], int size)
{
	qsort(configs, size, sizeof(EGLConfig), GLimp_EGLConfigCompare);
	return configs[0];
}

static EGLConfigInfo_t GLimp_GetConfigInfo(const EGLConfig config)
{
	EGLConfigInfo_t info;

	eglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE, &info.red);
	eglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE, &info.green);
	eglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE, &info.blue);
	eglGetConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE, &info.alpha);
	eglGetConfigAttrib(eglDisplay, config, EGL_DEPTH_SIZE, &info.depth);
	eglGetConfigAttrib(eglDisplay, config, EGL_STENCIL_SIZE, &info.stencil);
	eglGetConfigAttrib(eglDisplay, config, EGL_BUFFER_SIZE, &info.buffer);
	eglGetConfigAttrib(eglDisplay, config, EGL_SAMPLES, &info.samples);
	eglGetConfigAttrib(eglDisplay, config, EGL_SAMPLE_BUFFERS, &info.sample_buffers);

	return info;
}

void GLimp_ActivateContext()
{
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

void GLimp_DeactivateContext()
{
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

#pragma GCC visibility push(default)
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

	if(eglDisplay == EGL_NO_DISPLAY)
		return;

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	win = w;
	ANativeWindow_acquire((ANativeWindow *)win);
    ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface", true);
		return;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent", true);
		return;
	}
	//has_gl_context = true;
	Com_Printf("[Harmattan]: EGL surface created and using EGL context.\n");
}

void GLimp_AndroidQuit(void)
{
	//has_gl_context = false;
	if(!win)
		return;
	if(eglDisplay != EGL_NO_DISPLAY)
		GLimp_DeactivateContext();
	if(eglSurface != EGL_NO_SURFACE)
	{
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}
	ANativeWindow_release((ANativeWindow *)win);
	win = NULL;
	Com_Printf("[Harmattan]: EGL surface destroyed and no EGL context.\n");
}
#pragma GCC visibility pop

qboolean GLimp_OpenDisplay(void)
{
	if(eglDisplay) {
		return true;
	}

	Com_Printf("Setup EGL display connection\n" );

	if ( !( eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY) ) ) {
		Com_Printf("Couldn't open the EGL display\n" );
		return false;
	}
	return true;
}

/*
===============
GLES_Init
===============
*/
static bool GLES_Init_special(void)
{
	EGLint config_count = 0;
	EGLConfigInfo_t info = GLimp_FormatInfo(gl_format);
	int stencil_bits = info.stencil;
	int depth_bits = info.depth;
	int red_bits = info.red;
	int green_bits = info.green;
	int blue_bits = info.blue;
	int alpha_bits = info.alpha;
	int buffer_bits = info.buffer;

    EGLint attrib[] = {
            EGL_BUFFER_SIZE, buffer_bits,
            EGL_ALPHA_SIZE, alpha_bits,
            EGL_RED_SIZE, red_bits,
            EGL_BLUE_SIZE, green_bits,
            EGL_GREEN_SIZE, blue_bits,
            EGL_DEPTH_SIZE, depth_bits,
			EGL_STENCIL_SIZE, stencil_bits,
			EGL_SAMPLE_BUFFERS, gl_msaa > 1 ? 1 : 0,
			EGL_SAMPLES, gl_msaa,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE,
    };

	Com_Printf("[Harmattan]: Request special EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
			red_bits, green_bits,
			blue_bits, alpha_bits,
			depth_bits,
			stencil_bits
			, attrib[15], attrib[17]
			);

	int multisamples = gl_msaa;
	EGLConfig eglConfigs[MAX_NUM_CONFIGS];
	while(1)
	{
		if (!eglChooseConfig (eglDisplay, attrib, eglConfigs, MAX_NUM_CONFIGS, &config_count))
		{
			GLimp_HandleError("eglChooseConfig", false);

			if(multisamples > 1) {
				multisamples = (multisamples <= 2) ? 0 : (multisamples/2);

				attrib[7 + 1] = multisamples > 1 ? 1 : 0;
				attrib[8 + 1] = multisamples;
				continue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			Com_Printf("[Harmattan]: Get EGL context num -> %d.\n", config_count);
			for(int i = 0; i < config_count; i++)
			{
				EGLConfigInfo_t cinfo = GLimp_GetConfigInfo(eglConfigs[i]);
				Com_Printf("\t%d EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
								i + 1,
							  cinfo.red, cinfo.green,
							  cinfo.blue, cinfo.alpha,
							  cinfo.depth,
							  cinfo.stencil
						, cinfo.samples, cinfo.sample_buffers
				);
			}
		}
		break;
	}
	configs[0] = GLimp_ChooseConfig(eglConfigs, config_count);
	return true;
}

static bool GLES_Init_prefer(void)
{
	EGLint config_count = 0;
	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;
	bool suc;

	for (int i = 0; i < 16; i++) {

		int multisamples = gl_msaa;
		suc = false;

		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i) {
			// one pass, reduce
			switch (i / 4) {
			case 2 :
				if (colorbits == 24)
					colorbits = 16;
				break;
			case 1 :
				if (depthbits == 24)
					depthbits = 16;
				else if (depthbits == 16)
					depthbits = 8;
			case 3 :
				if (stencilbits == 24)
					stencilbits = 16;
				else if (stencilbits == 16)
					stencilbits = 8;
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if ((i % 4) == 3) {
			// reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2) {
			// reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1) {
			// reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		int channelcolorbits = 4;
		if (tcolorbits == 24)
			channelcolorbits = 8;

		int talphabits = channelcolorbits;

		EGLint attrib[] = {
			EGL_BUFFER_SIZE, channelcolorbits * 4,
			EGL_ALPHA_SIZE, talphabits,
			EGL_RED_SIZE, channelcolorbits,
			EGL_BLUE_SIZE, channelcolorbits,
			EGL_GREEN_SIZE, channelcolorbits,
			EGL_DEPTH_SIZE, tdepthbits,
			EGL_STENCIL_SIZE, tstencilbits,
			EGL_SAMPLE_BUFFERS, multisamples > 1 ? 1 : 0,
			EGL_SAMPLES, multisamples,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE,
		};

		Com_Printf("[Harmattan]: Request EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
				channelcolorbits, channelcolorbits,
				channelcolorbits, talphabits,
				tdepthbits,
				tstencilbits
				, attrib[15], attrib[17]
				);

		while(1)
		{
			if (!eglChooseConfig (eglDisplay, attrib, configs, 1, &config_count))
			{
				GLimp_HandleError("eglChooseConfig", false);

				if(multisamples > 1) {
					multisamples = (multisamples <= 2) ? 0 : (multisamples/2);

					attrib[7 + 1] = multisamples > 1 ? 1 : 0;
					attrib[8 + 1] = multisamples;
					continue;
				}
				else
				{
					break;
				}
			}
			suc = true;
			break;
		}

		if(suc)
			break;
	}
	return suc;
}

int GLES_Init(glconfig_t *glConfig, qboolean fullscreen)
{
	EGLint major, minor;

	if (!GLimp_OpenDisplay()) {
		return false;
	}
	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		GLimp_HandleError("eglInitialize", true);
		return false;
	}

	Com_Printf("Initializing OpenGL display\n");

	if(!GLES_Init_special())
	{
		if(!GLES_Init_prefer())
		{
			Com_Error(ERR_VID_FATAL, "Initializing EGL error");
			return false;
		}
	}

	eglConfig = configs[0];

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface", true);
		return false;
	}

	EGLint ctxAttrib[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};
	if ((eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib)) == EGL_NO_CONTEXT)
	{
		GLimp_HandleError("eglCreateContext", true);
		return false;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent", true);
		return false;
	}

	EGLConfigInfo_t info = GLimp_GetConfigInfo(eglConfig);

	Com_Printf("[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
					info.red, info.green,
					info.blue, info.alpha,
					info.depth,
					info.stencil
			, info.samples, info.sample_buffers
			);

    glConfig->colorBits = info.red + info.green + info.blue;
    glConfig->stencilBits = info.stencil;
    glConfig->depthBits = info.depth;

	glConfig->isFullscreen = qtrue;

	if (glConfig->isFullscreen) {
		//Sys_GrabMouseCursor(true);
	}

	return true;
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
qboolean GLimp_InitGL(glconfig_t *glConfig, qboolean fullscreen)
{

	if (!GLimp_OpenDisplay()) {
		return false;
	}

	if (!GLES_Init(glConfig, fullscreen)) {
		return false;
	}

	//has_gl_context = true;
	return true;
}

#pragma GCC visibility push(default)
void AndroidSetResolution(int aw,int ah)
{
	screen_width = aw;
	screen_height = ah;
}

void R_SetGLParms(int f, int msaa)
{
	gl_format = f;
	gl_msaa = msaa;
}
#pragma GCC visibility pop

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

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

		double newDelta = fabs((double) numerator / (double) denominator - ratio);
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

	// if (win)
	{
		//has_gl_context = false;
		GLimp_DeactivateContext();
		//eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if(eglContext != EGL_NO_CONTEXT)
		{
			eglDestroyContext(eglDisplay, eglContext);
			eglContext = EGL_NO_CONTEXT;
		}
		if(eglSurface != EGL_NO_SURFACE)
		{
			eglDestroySurface(eglDisplay, eglSurface);
			eglSurface = EGL_NO_SURFACE;
		}
		if(eglDisplay != EGL_NO_DISPLAY)
		{
			eglTerminate(eglDisplay);
			eglDisplay = EGL_NO_DISPLAY;
		}
	}

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
static int GLimp_SetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, windowContext_t *context)
{
	glConfig->vidWidth  = screen_width;
	glConfig->vidHeight = screen_height;

	glConfig->windowAspect = (float)glConfig->vidWidth / (float)glConfig->vidHeight;

	glConfig->windowWidth  = glConfig->vidWidth;
	glConfig->windowHeight = glConfig->vidHeight;

	glConfig->isFullscreen = qtrue;

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
static qboolean GLimp_StartDriverAndSetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, windowContext_t *context)
{
	rserr_t err;

	err = GLimp_SetMode(glConfig, mode, fullscreen, noborder, context);

	return err == RSERR_OK ? qtrue : qfalse;
}

#define R_MODE_FALLBACK 4 // 800 * 600

/**
 * @brief This routine is responsible for initializing the OS specific portions of OpenGL
 * @param[in,out] glConfig
 * @param[in] context
 */
void GLimp_Init(glconfig_t *glConfig, windowContext_t *context)
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
	if (GLimp_StartDriverAndSetMode(glConfig, r_mode->integer, (qboolean) !!r_fullscreen->integer, (qboolean) !!r_noBorder->integer, context))
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
		eglSwapBuffers(eglDisplay, eglSurface);
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
