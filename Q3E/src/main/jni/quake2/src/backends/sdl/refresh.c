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
 * This file implements an OpenGL context via SDL
 *
 * =======================================================================
 */

#include <stdbool.h>

#include "../../refresh/header/local.h"
#include "../generic/header/glwindow.h"
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#endif

#include "../android/gles.c"

/* The window icon */
#include "icon/q2icon.xbm"

/* X.org stuff */
#ifdef X11GAMMA
 #include <X11/Xos.h>
 #include <X11/Xlib.h>
 #include <X11/Xutil.h>
 #include <X11/extensions/xf86vmode.h>
#endif

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

glwstate_t glw_state;
qboolean have_stencil = false;

char *displayname = NULL;
int screen = -1;

#ifdef X11GAMMA
Display *dpy;
XF86VidModeGamma x11_oldgamma;
#endif

/*
 * Initialzes the SDL OpenGL context
 */
int
GLimp_Init(void)
{
	return true;
}

/*
 * Sets the window icon
 */
static void
SetSDLIcon()
{
}

/*
 * Sets the hardware gamma
 */
#ifdef X11GAMMA
void
UpdateHardwareGamma(void)
{
	float gamma;
	XF86VidModeGamma x11_gamma;

	gamma = vid_gamma->value;

	x11_gamma.red = gamma;
	x11_gamma.green = gamma;
	x11_gamma.blue = gamma;

	XF86VidModeSetGamma(dpy, screen, &x11_gamma);

	/* This forces X11 to update the gamma tables */
	XF86VidModeGetGamma(dpy, screen, &x11_gamma);
}

#else
void
UpdateHardwareGamma(void)
{
	float gamma;

	gamma = (vid_gamma->value);
	//TODO: GAMMA
}
#endif

/*
 * Initializes the OpenGL window
 */
static qboolean
GLimp_InitGraphics(qboolean fullscreen)
{
	int counter = 0;
	int flags;

	/* Create the window */
	ri.Vid_NewWindow(vid.width, vid.height);

	have_stencil = true;

	/* Initialize hardware gamma */
	gl_state.hwgamma = false;
	vid_gamma->modified = true;
	ri.Con_Printf(PRINT_ALL, "No hardware gamma.\n");

	GLimp_InitGL(fullscreen);

	return true;
}

/*
 * Swaps the buffers to show the new frame
 */
void GLimp_EndFrame(void)
{
	qglFlush();
	eglSwapBuffers(eglDisplay, eglSurface);
}

/*
 * Changes the video mode
 */
int
GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen)
{
	ri.Con_Printf(PRINT_ALL, "setting mode %d:", mode);

	/* mode -1 is not in the vid mode table - so we keep the values in pwidth
	   and pheight and don't even try to look up the mode info */
	if ((mode != -1) && !ri.Vid_GetModeInfo(pwidth, pheight, mode))
	{
		ri.Con_Printf(PRINT_ALL, " invalid mode\n");
		return rserr_invalid_mode;
	}

	ri.Con_Printf(PRINT_ALL, " %d %d\n", *pwidth, *pheight);

	if (!GLimp_InitGraphics(fullscreen))
	{
		return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
 * Shuts the SDL render backend down
 */
void
GLimp_Shutdown(void)
{
	/* Clear the backbuffer and make it
	   current. This may help some broken
	   video drivers like the AMD Catalyst
	   to avoid artifacts in unused screen
	   areas, */
	qglClearColor(0.0, 0.0, 0.0, 0.0);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLimp_EndFrame();

	gl_state.hwgamma = false;

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

	Com_Printf("[Harmattan]: EGL destroyed.\n");
}

