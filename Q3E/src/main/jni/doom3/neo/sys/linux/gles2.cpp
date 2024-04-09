/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Krzysztof Klinikowski <kkszysiu@gmail.com>
Copyright (C) 2012 Havlena Petr <havlenapetr@gmail.com>

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
#include "../../idlib/precompiled.h"
#include "../../renderer/tr_local.h"
#include "local.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT EGL_OPENGL_ES3_BIT_KHR
#endif
idCVar sys_videoRam("sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER,
					"Texture memory on the video card (in megabytes) - 0: autodetect", 0, 512);

#ifdef _OPENGLES3
const char	*r_openglesArgs[]	= {
		"GLES2",
		"GLES3.0",
		NULL };
idCVar harm_sys_openglVersion("harm_sys_openglVersion",
                              r_openglesArgs[1]
                              , CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INIT,
					"OpenGL version", r_openglesArgs, idCmdSystem::ArgCompletion_String<r_openglesArgs>);
#define DEFAULT_GLES_VERSION 0x00030000
#define HARM_EGL_OPENGL_ES_BIT (gl_version != 0x00020000 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT)
#define HARM_EGL_CONTEXT_CLIENT_VERSION (gl_version != 0x00020000 ? 3 : 2)
#else
#define DEFAULT_GLES_VERSION 0x00020000
#define HARM_EGL_OPENGL_ES_BIT EGL_OPENGL_ES2_BIT
#define HARM_EGL_CONTEXT_CLIENT_VERSION 2
#endif

#define MAX_NUM_CONFIGS 4

Display *dpy = NULL;
static int scrnum = 0;
Window win = 0;

bool dga_found = false;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLConfig configs[1];
static EGLConfig eglConfig = 0;
static EGLint format = 1;

int gl_format = 0x8888;
int gl_msaa = 1;
int gl_version = DEFAULT_GLES_VERSION;
bool USING_GLES3 = gl_version != 0x00020000;

static void GLimp_HandleError(const char *func, bool exit = true)
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
		common->Error("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	else
		common->Printf("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
}

void GLimp_CheckGLInitialized(void)
{
	// Not need do anything
}

void GLimp_WakeBackEnd(void *a)
{
	common->DPrintf("GLimp_WakeBackEnd stub\n");
}

void GLimp_EnableLogging(bool log)
{
	tr.logFile = log ? stdout : NULL;
	//common->DPrintf("GLimp_EnableLogging stub\n");
}

void GLimp_FrontEndSleep()
{
	common->DPrintf("GLimp_FrontEndSleep stub\n");
}

void *GLimp_BackEndSleep()
{
	common->DPrintf("GLimp_BackEndSleep stub\n");
	return 0;
}

bool GLimp_SpawnRenderThread(void (*a)())
{
	common->DPrintf("GLimp_SpawnRenderThread stub\n");
	return false;
}

void GLimp_ActivateContext()
{
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

void GLimp_DeactivateContext()
{
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

#include <dlfcn.h>
static void *glHandle = NULL;
#define QGLPROC(name, rettype, args) rettype (GL_APIENTRYP q##name) args;
#include "../../renderer/qgl_proc.h"
#undef QGLPROC

static void R_LoadOpenGLFunc()
{
#define QGLPROC(name, rettype, args) \
	q##name = (rettype(GL_APIENTRYP)args)GLimp_ExtensionPointer(#name); \
	if (!q##name) \
		common->FatalError("Unable to initialize OpenGL (%s)", #name);

#include "../../renderer/qgl_proc.h"
}

bool GLimp_dlopen()
{
#ifdef _OPENGLES3
	const char *driverName = /*r_glDriver.GetString()[0] ? r_glDriver.GetString() : */(
			gl_version == 0x00030000 ? "libGLESv3.so" : "libGLESv2.so"
	);
#else
	const char *driverName = /*r_glDriver.GetString()[0] ? r_glDriver.GetString() : */ "libGLESv2.so";
#endif
	common->Printf("dlopen(%s)\n", driverName);
#if 0
	if ( !( glHandle = dlopen( driverName, RTLD_NOW | RTLD_GLOBAL ) ) ) {
		common->Printf("dlopen(%s) failed: %s\n", driverName, dlerror());
		return false;
	}
	common->Printf("dlopen(%s) done\n", driverName);
#endif
	R_LoadOpenGLFunc();
	return true;
}

void GLimp_dlclose()
{
#if 0
	if ( !glHandle ) {
		common->Printf("dlclose: GL handle is NULL\n");
	} else {
		common->Printf("dlclose(%s) done\n", glHandle);
		dlclose( glHandle );
		glHandle = NULL;
	}
#endif
}

/*
=================
GLimp_SaveGamma

save and restore the original gamma of the system
=================
*/
void GLimp_SaveGamma()
{
}

/*
=================
GLimp_RestoreGamma

save and restore the original gamma of the system
=================
*/
void GLimp_RestoreGamma()
{
}

/*
=================
GLimp_SetGamma

gamma ramp is generated by the renderer from r_gamma and r_brightness for 256 elements
the size of the gamma ramp can not be changed on X (I need to confirm this)
=================
*/
void GLimp_SetGamma(unsigned short red[256], unsigned short green[256], unsigned short blue[256])
{
}

void GLimp_Shutdown()
{
	assert( dpy );

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
	GLimp_dlclose();

	XDestroyWindow( dpy, win );

	XFlush( dpy );

	dpy = NULL;
	win = 0;

	Sys_Printf("[Harmattan]: EGL destroyed.\n");
}

void GLimp_SwapBuffers()
{
	eglSwapBuffers(eglDisplay, eglSurface);
}

/*
** XErrorHandler
**   the default X error handler exits the application
**   I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
**   but those don't seem to be fatal .. so the default would be to just ignore them
**   our implementation mimics the default handler behaviour (not completely cause I'm lazy)
*/
int idXErrorHandler(Display * l_dpy, XErrorEvent * ev) {
	char buf[1024];
	common->Printf( "Fatal X Error:\n" );
	common->Printf( "  Major opcode of failed request: %d\n", ev->request_code );
	common->Printf( "  Minor opcode of failed request: %d\n", ev->minor_code );
	common->Printf( "  Serial number of failed request: %lu\n", ev->serial );
	XGetErrorText( l_dpy, ev->error_code, buf, 1024 );
	common->Printf( "%s\n", buf );
	return 0;
}

bool GLimp_OpenDisplay(void)
{
	if ( dpy ) {
		return true;
	}

	if(eglDisplay) {
		return true;
	}

	if (cvarSystem->GetCVarInteger("net_serverDedicated") == 1) {
		common->DPrintf("not opening the display: dedicated server\n");
		return false;
	}

	// that should be the first call into X
	if ( !XInitThreads() ) {
		common->Printf("XInitThreads failed\n");
		return false;
	}

	// set up our custom error handler for X failures
	XSetErrorHandler( &idXErrorHandler );

	if ( !( dpy = XOpenDisplay(NULL) ) ) {
		common->Printf( "Couldn't open the X display\n" );
		return false;
	}
	scrnum = DefaultScreen( dpy );

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
	int stencil_bits = 8;
	int depth_bits = 24;
	int red_bits = 8;
	int green_bits = 8;
	int blue_bits = 8;
	int alpha_bits = 8;
	int buffer_bits = 32;

	switch(gl_format)
	{
		case 0x565:
			red_bits = 5;
			green_bits = 6;
			blue_bits = 5;
			alpha_bits = 0;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case 0x4444:
			red_bits = 4;
			green_bits = 4;
			blue_bits = 4;
			alpha_bits = 4;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case 0x5551:
			red_bits = 5;
			green_bits = 5;
			blue_bits = 5;
			alpha_bits = 1;
			depth_bits = 16;
			buffer_bits = 16;
			break;
		case 0x8888:
		default:
			red_bits = 8;
			green_bits = 8;
			blue_bits = 8;
			alpha_bits = 8;
			depth_bits = 24;
			buffer_bits = 32;
			break;
	}
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
            EGL_RENDERABLE_TYPE, HARM_EGL_OPENGL_ES_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE,
    };

	common->Printf( "[Harmattan]: Request special EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
			red_bits, green_bits,
			blue_bits, alpha_bits,
			depth_bits,
			stencil_bits
			, attrib[15], attrib[17]
			);

	int multisamples = gl_msaa;
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
				return false;
			}
		}
		break;
	}
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
			EGL_RENDERABLE_TYPE, HARM_EGL_OPENGL_ES_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE,
		};

		common->Printf( "[Harmattan]: Request EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
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

int GLES_Init(glimpParms_t ap)
{
	unsigned long mask;
	int actualWidth, actualHeight;
	int i;
	EGLint major, minor;
	EGLint stencil_bits;
	EGLint depth_bits;
	EGLint red_bits;
	EGLint green_bits;
	EGLint blue_bits;
	EGLint alpha_bits;
	EGLint buffer_bits;
	EGLint samples;
	EGLint sample_buffers;

	if (!GLimp_OpenDisplay()) {
		return false;
	}

	XSetWindowAttributes attr;
	XSizeHints sizehints;
	XVisualInfo *visinfo;
	Window root;

	root = RootWindow( dpy, scrnum );
	
	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;

	// window attributes
	int blackColour = BlackPixel(dpy, scrnum);
	win = XCreateSimpleWindow(dpy, /* root */DefaultRootWindow(dpy), 0, 0, actualWidth, actualHeight, 0, blackColour, blackColour);

	XWindowAttributes WinAttr;
	int XResult = BadImplementation;

	if (!(XResult = XGetWindowAttributes(dpy, win, &WinAttr)))
		;

	XSelectInput(dpy, win, X_MASK);

	XStoreName(dpy, win, GAME_NAME);

	XMapWindow( dpy, win );

	XFlush(dpy);
	XSync(dpy, False);

    bool fs = cvarSystem->GetCVarBool("r_fullscreen");
    if (fs) {
        XEvent xev;
        Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = win;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
        xev.xclient.data.l[1] = fullscreen;
        xev.xclient.data.l[2] = 0;
        XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, &xev);
        Screen *screen = ScreenOfDisplay(dpy, 0);
        glConfig.vidWidth = actualWidth = screen->width;
        glConfig.vidHeight = actualHeight = screen->height;
    }

	common->Printf( "Setup EGL display connection\n" );

	if ( !( eglDisplay = eglGetDisplay(dpy) ) ) {
		common->Printf( "Couldn't open the EGL display\n" );
		return false;
	}

	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		GLimp_HandleError("eglInitialize");
		return false;
	}

	common->Printf("Initializing OpenGL display\n");

	if(!GLES_Init_special())
	{
		if(!GLES_Init_prefer())
		{
			common->Error("Initializing EGL error");
			return false;
		}
	}

	eglConfig = configs[0];

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return false;
	}

	EGLint ctxAttrib[] = {
		EGL_CONTEXT_CLIENT_VERSION, HARM_EGL_CONTEXT_CLIENT_VERSION,
		EGL_NONE
	};
	if ((eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib)) == EGL_NO_CONTEXT)
	{
		GLimp_HandleError("eglCreateContext");
		return false;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent");
		return false;
	}

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_STENCIL_SIZE, &stencil_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_DEPTH_SIZE, &depth_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RED_SIZE, &red_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_GREEN_SIZE, &green_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BLUE_SIZE, &blue_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_ALPHA_SIZE, &alpha_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BUFFER_SIZE, &buffer_bits);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &samples);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLE_BUFFERS, &sample_buffers);
	int tcolorbits = red_bits + green_bits + blue_bits;

	common->Printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
			red_bits, green_bits,
			blue_bits, alpha_bits,
			depth_bits,
			stencil_bits
			, samples, sample_buffers
			);

	glConfig.colorBits = tcolorbits;
	glConfig.depthBits = depth_bits;
	glConfig.stencilBits = stencil_bits;

	glConfig.isFullscreen = fs;

	if (glConfig.isFullscreen) {
		Sys_GrabMouseCursor(true);
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
bool GLimp_Init(glimpParms_t a)
{

	if (!GLimp_OpenDisplay()) {
		return false;
	}

	if (!GLES_Init(a)) {
		return false;
	}

	// load qgl function pointers
	GLimp_dlopen();

	const char *glstring;
	glstring = (const char *) qglGetString(GL_RENDERER);
	common->Printf("GL_RENDERER: %s\n", glstring);

	glstring = (const char *) qglGetString(GL_EXTENSIONS);
	common->Printf("GL_EXTENSIONS: %s\n", glstring);

	glstring = (const char *) qglGetString(GL_VERSION);
	common->Printf("GL_VERSION: %s\n", glstring);

	glstring = (const char *) qglGetString(GL_SHADING_LANGUAGE_VERSION);
	common->Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glstring);

	//has_gl_context = true;
	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms(glimpParms_t parms)
{
	return true;
}

/*
================
Sys_GetVideoRam
returns in megabytes
open your own display connection for the query and close it
using the one shared with GLimp_Init is not stable
================
*/
int Sys_GetVideoRam(void)
{
	static int run_once = 0;
	int major, minor, value;

	if (run_once) {
		return run_once;
	}

	if (sys_videoRam.GetInteger()) {
		run_once = sys_videoRam.GetInteger();
		return sys_videoRam.GetInteger();
	}

	// try a few strategies to guess the amount of video ram
	common->Printf("guessing video ram ( use +set sys_videoRam to force ) ..\n");

	run_once = 64;
	return run_once;
}
