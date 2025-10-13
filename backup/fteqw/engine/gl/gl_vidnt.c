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
// gl_vidnt.c -- NT GL vid component
// note that this file also handles win32 vulkan window things
// and can also use either EGL or WGL.

#include "quakedef.h"
#if defined(_WIN32) && (defined(GLQUAKE) || defined(VKQUAKE))
#include "glquake.h"
#include "winquake.h"
#include "resource.h"
#include "shader.h"
#include "vr.h"
#include <commctrl.h>

#ifdef USE_EGL
	#ifdef GLQUAKE
		#include "gl_videgl.h"
	#else
		#undef USE_EGL
	#endif
#endif
#ifdef GLQUAKE
	#define USE_WGL
#endif
#ifdef VKQUAKE
	#include "../vk/vkrenderer.h"
#endif

static enum
{
#ifdef USE_WGL
	MODE_WGL,
#endif
#ifdef USE_EGL
	MODE_EGL,
#endif
#ifdef VKQUAKE
	MODE_VULKAN,	//proper vulkan
	#ifdef USE_WGL
		MODE_NVVULKAN,	//vulkan accessed via nvidia's render-to-image-then-copy-to-gl-backbuffer-then-copy-that opengl extension.
	#endif
#endif
} platform_rendermode;


void STT_Event(void);

#ifndef SetWindowLongPtr	//yes its a define, for unicode support
#define SetWindowLongPtr SetWindowLong
#endif

#ifndef CDS_FULLSCREEN
	#define CDS_FULLSCREEN 4
#endif

#ifndef WM_XBUTTONDOWN
   #define WM_XBUTTONDOWN      0x020B
   #define WM_XBUTTONUP      0x020C
#endif
#ifndef MK_XBUTTON1
   #define MK_XBUTTON1         0x0020
#endif
#ifndef MK_XBUTTON2
   #define MK_XBUTTON2         0x0040
#endif
// copied from DarkPlaces in an attempt to grab more buttons
#ifndef MK_XBUTTON3
   #define MK_XBUTTON3         0x0080
#endif
#ifndef MK_XBUTTON4
   #define MK_XBUTTON4         0x0100
#endif
#ifndef MK_XBUTTON5
   #define MK_XBUTTON5         0x0200
#endif
#ifndef MK_XBUTTON6
   #define MK_XBUTTON6         0x0400
#endif
#ifndef MK_XBUTTON7
   #define MK_XBUTTON7         0x0800
#endif

#ifndef WM_INPUT
	#define WM_INPUT 255
#endif

#ifndef WS_EX_LAYERED
	#define WS_EX_LAYERED 0x00080000
#endif
#ifndef LWA_ALPHA
	#define LWA_ALPHA 0x00000002
#endif
typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

extern cvar_t vid_conwidth, vid_conautoscale;


#define WINDOW_CLASS_NAME_W L"FTEGLQuake"
#define WINDOW_CLASS_NAME_A "FTEGLQuake"

extern cvar_t vid_width;
extern cvar_t vid_height;
extern cvar_t vid_wndalpha;
extern cvar_t vid_winthread;

static qboolean VID_SetWindowedMode (rendererstate_t *info);	//-1 on bpp or hz for default.
static qboolean VID_SetFullDIBMode (rendererstate_t *info);	//-1 on bpp or hz for default.

#ifdef MULTITHREAD
#define WTHREAD	//While the user is resizing a window, the entire thread that owns said window becomes frozen. in order to cope with window resizing, its easiest to just create a separate thread to be microsoft's plaything. our main game thread can then just keep rendering. hopefully that won't bug out on the present.
#endif
#ifdef WTHREAD
static HANDLE	windowthread;
#endif

static DEVMODE	gdevmode;
static qboolean	vid_initialized = false;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;
extern qboolean	mouseactive;  // from in_win.c
static HICON	hIcon;
extern qboolean vid_isfullscreen;

static unsigned short originalgammaramps[3][256];

static qboolean vid_initializing;

static int			DIBWidth, DIBHeight;
static RECT		WindowRect;
static DWORD		WindowStyle, ExWindowStyle;

static HWND dibwindow;

static HDC		maindc;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

//unsigned short	d_8to16rgbtable[256];
//unsigned	d_8to24rgbtable[256];
//unsigned short	d_8to16bgrtable[256];
//unsigned	d_8to24bgrtable[256];

static enum {MS_WINDOWED, MS_FULLDIB, MS_FULLWINDOW, MS_UNINIT}	modestate = MS_UNINIT;

extern float gammapending;


//====================================
// Note that 0 is MODE_WINDOWED
extern cvar_t	vid_mode;
// Note that 3 is MODE_FULLSCREEN_DEFAULT
extern cvar_t		vid_vsync;
extern cvar_t		vid_hardwaregamma;
extern cvar_t		vid_desktopgamma;
extern cvar_t		gl_lateswap;
extern cvar_t		vid_preservegamma;

static int			window_x, window_y;
static int			window_width, window_height;


static LONG WINAPI GLMainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static qboolean GLAppActivate(BOOL fActive, BOOL minimize);
static void ClearAllStates (void);
static void VID_UpdateWindowStatus (HWND hWnd);

static BOOL (WINAPI *qGetDeviceGammaRamp)(HDC hDC, void *ramp);
static BOOL (WINAPI *qSetDeviceGammaRamp)(HDC hDC, void *ramp);

//==========================================================
#ifdef USE_WGL
static HGLRC	baseRC;
static const char *wgl_extensions;

static qboolean VID_AttachGL (rendererstate_t *info);
static BOOL bSetupPixelFormat(HDC hDC, rendererstate_t *info);
static BOOL CheckForcePixelFormat(rendererstate_t *info);

extern cvar_t		vid_gl_context_version;
extern cvar_t		vid_gl_context_debug;
extern cvar_t		vid_gl_context_es;
extern cvar_t		vid_gl_context_forwardcompatible;
extern cvar_t		vid_gl_context_compatibility;
extern cvar_t		vid_gl_context_robustness;
extern cvar_t		vid_gl_context_selfreset;
extern cvar_t		vid_gl_context_noerror;

static dllhandle_t *hInstGL = NULL;
static dllhandle_t *hInstwgl = NULL;
static qboolean usingminidriver;
static char reqminidriver[MAX_OSPATH];
static char opengldllname[MAX_OSPATH];


static HGLRC (WINAPI *qwglCreateContext)(HDC);
static BOOL  (WINAPI *qwglDeleteContext)(HGLRC);
static HGLRC (WINAPI *qwglGetCurrentContext)(VOID);
static HDC   (WINAPI *qwglGetCurrentDC)(VOID);
static PROC  (WINAPI *qwglGetProcAddress)(LPCSTR);
static BOOL  (WINAPI *qwglMakeCurrent)(HDC, HGLRC);
static BOOL  (WINAPI *qSwapBuffers)(HDC);
static BOOL  (WINAPI *qwglSwapLayerBuffers)(HDC, UINT);
static int   (WINAPI *qChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *);
static BOOL  (WINAPI *qSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
static int   (WINAPI *qDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);

static BOOL (WINAPI *qwglSwapIntervalEXT) (int);

static BOOL (APIENTRY *qwglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
static BOOL (APIENTRY *qwglGetPixelFormatAttribfvARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);

static HGLRC (APIENTRY *qwglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int *attribList);
#define WGL_CONTEXT_MAJOR_VERSION_ARB					0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB					0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB						0x2093
#define WGL_CONTEXT_FLAGS_ARB							0x2094
#define		WGL_CONTEXT_DEBUG_BIT_ARB						0x0001
#define		WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB			0x0002
#define		WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB				0x0004 /*WGL_ARB_create_context_robustness*/
#define WGL_CONTEXT_PROFILE_MASK_ARB					0x9126		/*WGL_ARB_create_context_profile*/
#define		WGL_CONTEXT_CORE_PROFILE_BIT_ARB				0x00000001
#define		WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB		0x00000002
#define		WGL_CONTEXT_ES2_PROFILE_BIT_EXT					0x00000004	/*WGL_CONTEXT_ES2_PROFILE_BIT_EXT*/
#define ERROR_INVALID_VERSION_ARB						0x2095
#define	ERROR_INVALID_PROFILE_ARB						0x2096
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB		0x8256	/*WGL_ARB_create_context_robustness*/
#define		WGL_NO_RESET_NOTIFICATION_ARB					0x8261
#define		WGL_LOSE_CONTEXT_ON_RESET_ARB					0x8252
#define	WGL_CONTEXT_OPENGL_NO_ERROR_ARB					0x31B3 /*WGL_ARB_create_context_no_error*/


//pixel format stuff
#define 	WGL_DRAW_TO_WINDOW_ARB		0x2001
#define 	WGL_ACCELERATION_ARB		0x2003
#define		WGL_SWAP_LAYER_BUFFERS_ARB	0x2006
#define 	WGL_SUPPORT_OPENGL_ARB		0x2010
#define 	WGL_DOUBLE_BUFFER_ARB		0x2011
#define		WGL_STEREO_ARB				0x2012
#define 	WGL_COLOR_BITS_ARB			0x2014
#define 	WGL_ALPHA_BITS_ARB			0x201B
#define 	WGL_DEPTH_BITS_ARB			0x2022
#define 	WGL_STENCIL_BITS_ARB		0x2023
#define 	WGL_FULL_ACCELERATION_ARB	0x2027
#define		WGL_PIXEL_TYPE_ARB			0x2013
#define			WGL_TYPE_RGBA_ARB		0x202B
#define			WGL_TYPE_RGBA_FLOAT_ARB	0x21A0
#define		WGL_RED_BITS_ARB			0x2015
#define		WGL_GREEN_BITS_ARB			0x2017
#define		WGL_BLUE_BITS_ARB			0x2019



#ifdef _DEBUG
//this is a list of the functions that exist in opengles2, as well as wglCreateContextAttribsARB.
//functions not in this list *should* be stubs that just return errors, but we can't always depend on drivers for that... they shouldn't get called.
//this list is just to make it easier to test+debug android gles2 stuff using windows.
static char *gles2funcs[] =
{
#define f(n) #n,
		f(glActiveTexture)
		f(glAttachShader)
		f(glBindAttribLocation)
		f(glBindBuffer)
		f(glBindFramebuffer)
		f(glBindRenderbuffer)
		f(glBindTexture)
		f(glBlendColor)
		f(glBlendEquation)
		f(glBlendEquationSeparate)
		f(glBlendFunc)
		f(glBlendFuncSeparate)
		f(glBufferData)
		f(glBufferSubData)
		f(glCheckFramebufferStatus)
		f(glClear)
		f(glClearColor)
		f(glClearDepthf)
		f(glClearStencil)
		f(glColorMask)
		f(glCompileShader)
		f(glCompressedTexImage2D)
		f(glCompressedTexSubImage2D)
		f(glCopyTexImage2D)
		f(glCopyTexSubImage2D)
		f(glCreateProgram)
		f(glCreateShader)
		f(glCullFace)
		f(glDeleteBuffers)
		f(glDeleteFramebuffers)
		f(glDeleteProgram)
		f(glDeleteRenderbuffers)
		f(glDeleteShader)
		f(glDeleteTextures)
		f(glDepthFunc)
		f(glDepthMask)
		f(glDepthRangef)
		f(glDetachShader)
		f(glDisable)
		f(glDisableVertexAttribArray)
		f(glDrawArrays)
		f(glDrawElements)
		f(glEnable)
		f(glEnableVertexAttribArray)
		f(glFinish)
		f(glFlush)
		f(glFramebufferRenderbuffer)
		f(glFramebufferTexture2D)
		f(glFrontFace)
		f(glGenBuffers)
		f(glGenerateMipmap)
		f(glGenFramebuffers)
 		f(glGenRenderbuffers)
		f(glGenTextures)
		f(glGetActiveAttrib)
		f(glGetActiveUniform)
		f(glGetAttachedShaders)
		f(glGetAttribLocation)
		f(glGetBooleanv)
		f(glGetBufferParameteriv)
		f(glGetError)
		f(glGetFloatv)
		f(glGetFramebufferAttachmentParameteriv)
		f(glGetIntegerv)
		f(glGetProgramiv)
		f(glGetProgramInfoLog)
		f(glGetRenderbufferParameteriv)
		f(glGetShaderiv)
		f(glGetShaderInfoLog)
		f(glGetShaderPrecisionFormat)
		f(glGetShaderSource)
		f(glGetString)
		f(glGetTexParameterfv)
		f(glGetTexParameteriv)
		f(glGetUniformfv)
		f(glGetUniformiv)
		f(glGetUniformLocation)
		f(glGetVertexAttribfv)
		f(glGetVertexAttribiv)
		f(glGetVertexAttribPointerv)
		f(glHint)
		f(glIsBuffer)
		f(glIsEnabled)
		f(glIsFramebuffer)
		f(glIsProgram)
		f(glIsRenderbuffer)
		f(glIsShader)
		f(glIsTexture)
		f(glLineWidth)
		f(glLinkProgram)
		f(glPixelStorei)
		f(glPolygonOffset)
		f(glReadPixels)
		f(glReleaseShaderCompiler)
		f(glRenderbufferStorage)
		f(glSampleCoverage)
		f(glScissor)
		f(glShaderBinary)
		f(glShaderSource)
		f(glStencilFunc)
		f(glStencilFuncSeparate)
		f(glStencilMask)
		f(glStencilMaskSeparate)
		f(glStencilOp)
		f(glStencilOpSeparate)
		f(glTexImage2D)
		f(glTexParameterf)
		f(glTexParameterfv)
		f(glTexParameteri)
		f(glTexParameteriv)
		f(glTexSubImage2D)
		f(glUniform1f)
		f(glUniform1fv)
		f(glUniform1i)
		f(glUniform1iv)
		f(glUniform2f)
		f(glUniform2fv)
		f(glUniform2i)
		f(glUniform2iv)
		f(glUniform3f)
		f(glUniform3fv)
		f(glUniform3i)
		f(glUniform3iv)
		f(glUniform4f)
		f(glUniform4fv)
		f(glUniform4i)
		f(glUniform4iv)
		f(glUniformMatrix2fv)
		f(glUniformMatrix3fv)
		f(glUniformMatrix4fv)
		f(glUseProgram)
		f(glValidateProgram)
		f(glVertexAttrib1f)
		f(glVertexAttrib1fv)
		f(glVertexAttrib2f)
		f(glVertexAttrib2fv)
		f(glVertexAttrib3f)
		f(glVertexAttrib3fv)
		f(glVertexAttrib4f)
		f(glVertexAttrib4fv)
		f(glVertexAttribPointer)
		f(glViewport)
		f(wglCreateContextAttribsARB)
		NULL
};

//this is a list of the functions that exist in opengles2, as well as wglCreateContextAttribsARB.
//functions not in this list *should* be stubs that just return errors, but we can't always depend on drivers for that... they shouldn't get called.
//this list is just to make it easier to test+debug android gles2 stuff using windows.
static char *gles1funcs[] =
{
#define f(n) #n,

		/* Available only in Common profile */
		f(glAlphaFunc)
		f(glClearColor)
		f(glClearDepthf)
		f(glClipPlanef)
		f(glColor4f)
		f(glDepthRangef)
		f(glFogf)
		f(glFogfv)
		f(glFrustumf)
		f(glGetClipPlanef)
		f(glGetFloatv)
		f(glGetLightfv)
		f(glGetMaterialfv)
		f(glGetTexEnvfv)
		f(glGetTexParameterfv)
		f(glLightModelf)
		f(glLightModelfv)
		f(glLightf)
		f(glLightfv)
		f(glLineWidth)
		f(glLoadMatrixf)
		f(glMaterialf)
		f(glMaterialfv)
		f(glMultMatrixf)
		f(glMultiTexCoord4f)
		f(glNormal3f)
		f(glOrthof)
		f(glPointParameterf)
		f(glPointParameterfv)
		f(glPointSize)
		f(glPolygonOffset)
		f(glRotatef)
		f(glScalef)
		f(glTexEnvf)
		f(glTexEnvfv)
		f(glTexParameterf)
		f(glTexParameterfv)
		f(glTranslatef)

		/* Available in both Common and Common-Lite profiles */
		f(glActiveTexture)
		f(glAlphaFuncx)
		f(glBindBuffer)
		f(glBindTexture)
		f(glBlendFunc)
		f(glBufferData)
		f(glBufferSubData)
		f(glClear)
		f(glClearColorx)
		f(glClearDepthx)
		f(glClearStencil)
		f(glClientActiveTexture)
		f(glClipPlanex)
		f(glColor4ub)
		f(glColor4x)
		f(glColorMask)
		f(glColorPointer)
		f(glCompressedTexImage2D)
		f(glCompressedTexSubImage2D)
		f(glCopyTexImage2D)
		f(glCopyTexSubImage2D)
		f(glCullFace)
		f(glDeleteBuffers)
		f(glDeleteTextures)
		f(glDepthFunc)
		f(glDepthMask)
		f(glDepthRangex)
		f(glDisable)
		f(glDisableClientState)
		f(glDrawArrays)
		f(glDrawElements)
		f(glEnable)
		f(glEnableClientState)
		f(glFinish)
		f(glFlush)
		f(glFogx)
		f(glFogxv)
		f(glFrontFace)
		f(glFrustumx)
		f(glGetBooleanv)
		f(glGetBufferParameteriv)
		f(glGetClipPlanex)
		f(glGenBuffers)
		f(glGenTextures)
		f(glGetError)
		f(glGetFixedv)
		f(glGetIntegerv)
		f(glGetLightxv)
		f(glGetMaterialxv)
		f(glGetPointerv)
		f(glGetString)
		f(glGetTexEnviv)
		f(glGetTexEnvxv)
		f(glGetTexParameteriv)
		f(glGetTexParameterxv)
		f(glHint)
		f(glIsBuffer)
		f(glIsEnabled)
		f(glIsTexture)
		f(glLightModelx)
		f(glLightModelxv)
		f(glLightx)
		f(glLightxv)
		f(glLineWidthx)
		f(glLoadIdentity)
		f(glLoadMatrixx)
		f(glLogicOp)
		f(glMaterialx)
		f(glMaterialxv)
		f(glMatrixMode)
		f(glMultMatrixx)
		f(glMultiTexCoord4x)
		f(glNormal3x)
		f(glNormalPointer)
		f(glOrthox)
		f(glPixelStorei)
		f(glPointParameterx)
		f(glPointParameterxv)
		f(glPointSizex)
		f(glPolygonOffsetx)
		f(glPopMatrix)
		f(glPushMatrix)
		f(glReadPixels)
		f(glRotatex)
		f(glSampleCoverage)
		f(glSampleCoveragex)
		f(glScalex)
		f(glScissor)
		f(glShadeModel)
		f(glStencilFunc)
		f(glStencilMask)
		f(glStencilOp)
		f(glTexCoordPointer)
		f(glTexEnvi)
		f(glTexEnvx)
		f(glTexEnviv)
		f(glTexEnvxv)
		f(glTexImage2D)
		f(glTexParameteri)
		f(glTexParameterx)
		f(glTexParameteriv)
		f(glTexParameterxv)
		f(glTexSubImage2D)
		f(glTranslatex)
		f(glVertexPointer)
		f(glViewport)

		/*required to switch stuff around*/
		f(wglCreateContextAttribsARB)
		NULL
};
#endif

//just GetProcAddress with a safty net.
static void *getglfunc(char *name)
{
	FARPROC proc;
	proc = qwglGetProcAddress?qwglGetProcAddress(name):NULL;
	if (!proc)
	{
		proc = GetProcAddress(hInstGL, name);
		TRACE(("dbg: getglfunc: gpa %s: success %i\n", name, !!proc));
	}
	else
	{
		TRACE(("dbg: getglfunc: glgpa %s: success %i\n", name, !!proc));
	}

#ifdef _DEBUG
	if (vid_gl_context_es.ival == 3)
	{
		int i;
		for (i = 0; gles1funcs[i]; i++)
		{
			if (!strcmp(name, gles1funcs[i]))
				return proc;
		}
		return NULL;
	}
	if (vid_gl_context_es.ival == 2)
	{
		int i;
		for (i = 0; gles2funcs[i]; i++)
		{
			if (!strcmp(name, gles2funcs[i]))
				return proc;
		}
		return NULL;
	}
#endif
	return proc;
}
static void *getwglfunc(char *name)
{
	FARPROC proc;
	TRACE(("dbg: getwglfunc: %s: getting\n", name));

	proc = GetProcAddress(hInstGL, name);
	if (!proc)
	{
		if (!hInstwgl)
		{
			TRACE(("dbg: getwglfunc: explicitly loading opengl32.dll\n", name));
			hInstwgl = LoadLibraryA("opengl32.dll");
		}
		TRACE(("dbg: getwglfunc: %s: wglgetting\n", name));
		proc = GetProcAddress(hInstwgl, name);
		TRACE(("dbg: getwglfunc: gpa %s: success %i\n", name, !!proc));
		if (!proc)
			Sys_Error("GL function %s was not found in %s\nPossibly you do not have a full enough gl implementation", name, opengldllname);
	}
	TRACE(("dbg: getwglfunc: glgpa %s: success %i\n", name, !!proc));
	return proc;
}

static qboolean shouldforcepixelformat;
static int forcepixelformat;
static int currentpixelformat;

static qboolean GLInitialise (char *renderer)
{
	if (!hInstGL || strcmp(reqminidriver, renderer))
	{
		usingminidriver = false;
		if (hInstGL)
			Sys_CloseLibrary(hInstGL);
		hInstGL=NULL;
		if (hInstwgl)
			Sys_CloseLibrary(hInstwgl);
		hInstwgl=NULL;

		Q_strncpyz(reqminidriver, renderer, sizeof(reqminidriver));
		Q_strncpyz(opengldllname, renderer, sizeof(opengldllname));

		if (*renderer && stricmp(renderer, "opengl32.dll") && stricmp(renderer, "opengl32"))
		{
			unsigned int emode = SetErrorMode(SEM_FAILCRITICALERRORS); /*no annoying errors if they use glide*/
			Con_DPrintf ("Loading renderer dll \"%s\"", renderer);
			hInstGL = Sys_LoadLibrary(opengldllname, NULL);
			SetErrorMode(emode);
			if (hInstGL)
			{
				usingminidriver = true;
				Con_DPrintf (" Success\n");
			}
			else
				Con_DPrintf (" Failed\n");
		}
		else
			hInstGL = NULL;

		if (!hInstGL)
		{	//gog has started shipping glquake using a 3dfxopengl->nglide->direct3d chain of wrappers.
			//this bypasses issues with (not that) recent gl drivers giving up on limiting extension string lengths and paletted textures
			//instead, we explicitly try to use the opengl32.dll from the windows system32 directory to try and avoid using the wrapper.
			unsigned int emode;
			wchar_t wbuffer[MAX_OSPATH];
			GetSystemDirectoryW(wbuffer, countof(wbuffer));
			narrowen(opengldllname, sizeof(opengldllname), wbuffer);
			Q_strncatz(opengldllname, "\\opengl32.dll", sizeof(opengldllname));
			Con_DPrintf ("Loading renderer dll \"%s\"", opengldllname);
			emode = SetErrorMode(SEM_FAILCRITICALERRORS);
			hInstGL = Sys_LoadLibrary(opengldllname, NULL);
			SetErrorMode(emode);

			if (hInstGL)
				Con_DPrintf (" Success\n");
			else
				Con_DPrintf (" Failed\n");
		}

		if (!hInstGL)
		{
			unsigned int emode;
			strcpy(opengldllname, "opengl32");
			Con_DPrintf ("Loading renderer dll \"%s\"", opengldllname);
			emode = SetErrorMode(SEM_FAILCRITICALERRORS); /*no annoying errors if they use glide*/
			hInstGL = Sys_LoadLibrary(opengldllname, NULL);
			SetErrorMode(emode);

			if (hInstGL)
				Con_DPrintf (" Success\n");
			else
				Con_DPrintf (" Failed\n");
		}
		if (!hInstGL)
		{
			if (*renderer)
				Con_Printf ("Couldn't load %s or %s\n", renderer, opengldllname);
			else
				Con_Printf ("Couldn't load %s\n", opengldllname);
			return false;
		}
	}
	else
	{
		Con_DPrintf ("Reusing renderer dll %s\n", opengldllname);
	}

	Con_DPrintf ("Loaded renderer dll %s\n", opengldllname);

	// windows dependant
	qwglCreateContext		= (void *)getwglfunc("wglCreateContext");
	qwglDeleteContext		= (void *)getwglfunc("wglDeleteContext");
	qwglGetCurrentContext	= (void *)getwglfunc("wglGetCurrentContext");
	qwglGetCurrentDC		= (void *)getwglfunc("wglGetCurrentDC");
	qwglGetProcAddress		= (void *)getwglfunc("wglGetProcAddress");
	qwglMakeCurrent			= (void *)getwglfunc("wglMakeCurrent");

	if (usingminidriver)
	{
		qwglSwapLayerBuffers	= NULL;
		qSwapBuffers			= (void *)getglfunc("wglSwapBuffers");
		qChoosePixelFormat		= (void *)getglfunc("wglChoosePixelFormat");
		qSetPixelFormat			= (void *)getglfunc("wglSetPixelFormat");
		qDescribePixelFormat	= (void *)getglfunc("wglDescribePixelFormat");
	}
	else
	{
		qwglSwapLayerBuffers	= NULL;//(void *)getwglfunc("wglSwapLayerBuffers");
		qSwapBuffers			= SwapBuffers;
		qChoosePixelFormat		= ChoosePixelFormat;
		qSetPixelFormat			= SetPixelFormat;
		qDescribePixelFormat	= DescribePixelFormat;
	}

	qGetDeviceGammaRamp			= (void *)getglfunc("wglGetDeviceGammaRamp3DFX");
	qSetDeviceGammaRamp			= (void *)getglfunc("wglSetDeviceGammaRamp3DFX");

	TRACE(("dbg: GLInitialise: got wgl funcs\n"));

	return true;
}

static void ReleaseGL(void)
{
	HGLRC	hRC;
	HDC		hDC = NULL;

	if (qwglGetCurrentContext)
	{
		hRC = qwglGetCurrentContext();
		hDC = qwglGetCurrentDC();

		qwglMakeCurrent(NULL, NULL);

		if (hRC)
			qwglDeleteContext(hRC);
	}
	qwglGetCurrentContext=NULL;

	if (hDC && dibwindow)
		ReleaseDC(dibwindow, hDC);
}
#endif	//USE_WGL

#ifdef VKQUAKE
static dllhandle_t *hInstVulkan = NULL;
static qboolean Win32VK_CreateSurface(void)
{
	VkResult err;
	VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	createInfo.flags = 0;
	createInfo.hinstance = GetModuleHandle(NULL);
	createInfo.hwnd = mainwindow;

	err = vkCreateWin32SurfaceKHR(vk.instance, &createInfo, NULL, &vk.surface);
	switch(err)
	{
	default:
		Con_Printf("Unknown vulkan device creation error: %x\n", err);
		return false;
	case VK_SUCCESS:
		break;
	}
	return true;
}

#ifdef WTHREAD
static void Win32VK_Present(struct vkframe *theframe)
{
//	if (theframe)
//		PostMessage(mainwindow, WM_USER_VKPRESENT, 0, (LPARAM)theframe);
//	else
		SendMessage(mainwindow, WM_USER_VKPRESENT, 0, (LPARAM)theframe);
}
#else
#define Win32VK_Present NULL
#endif

static qboolean Win32VK_AttachVulkan (rendererstate_t *info)
{	//make sure we can get a valid renderer.
	const char *extnames[] = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME, NULL};
#ifdef VK_NO_PROTOTYPES
	hInstVulkan = NULL;
	if (!hInstVulkan)
		hInstVulkan = *info->subrenderer?LoadLibrary(info->subrenderer):NULL;
	if (!hInstVulkan)
		hInstVulkan = LoadLibrary("vulkan-1.dll");
	if (!hInstVulkan)
	{
		Con_Printf("Unable to load vulkan-1.dll\nNo Vulkan drivers are installed\n");
		return false;
	}
	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(hInstVulkan, "vkGetInstanceProcAddr");
#endif

	return VK_Init(info, extnames, Win32VK_CreateSurface, Win32VK_Present);
}
static qboolean Win32VK_EnumerateDevices(void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc))
{
	qboolean ret = false;
#ifdef VK_NO_PROTOTYPES
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	dllfunction_t func[] =
	{
		{(void*)&vkGetInstanceProcAddr,		"vkGetInstanceProcAddr"},
		{NULL,							NULL}
	};

	if (!hInstVulkan)
		hInstVulkan = Sys_LoadLibrary("vulkan-1.dll", func);
	if (!hInstVulkan)
		return false;
#endif
	ret = VK_EnumerateDevices(usercontext, callback, "Vulkan-", vkGetInstanceProcAddr);
	return ret;
}
#endif


#if defined(VKQUAKE) && defined(USE_WGL)
#define GLuint64 quint64_t
#define GLchar char
static PFN_vkVoidFunction	(WINAPI *qglGetVkProcAddrNV)		(const GLchar *name);
static void					(WINAPI *qglDrawVkImageNV)			(GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);
static void					(WINAPI *qglWaitVkSemaphoreNV)		(GLuint64 vkSemaphore);
static void					(WINAPI *qglSignalVkSemaphoreNV)	(GLuint64 vkSemaphore);
static void					(WINAPI *qglSignalVkFenceNV)		(GLuint64 vkFence);
static PFN_vkVoidFunction VKAPI_CALL nvvkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
	//nvidia do not make this easy.
	PFN_vkVoidFunction fnc;

//	qwglMakeCurrent(maindc, baseRC);
	fnc = qglGetVkProcAddrNV(pName);
//	qwglMakeCurrent(maindc, NULL);

	return fnc;
}
static qboolean Win32NVVK_CreateSurface(void)
{
	vk.surface = VK_NULL_HANDLE;
//	vk.allowsubmissionthread = false;	//must come on the main thread, because that's the one with the gl context.
										//I seem to be getting crashes on vulkan's fences if I try giving ownership of the gl context to a different thread (instead of main).
	return true;
}
static void Win32NVVK_Present(struct vkframe *theframe)
{
	SendMessage(mainwindow, WM_USER_NVVKPRESENT, 0, (LPARAM)theframe);
}
static void Win32NVVK_DoPresent(struct vkframe *theframe)
{
	VkFence fence;
	RSpeedLocals();
	if (!theframe)
		return;	//this is used to ensure some presentation thread has woken up. we're not threading this, hopefully the gl server will do any of that that's needed.

	RSpeedRemark();

	//this might be a submission thread, so make sure we're talking to the right opengl context...
//	qwglMakeCurrent(maindc, baseRC);

	//get the gl driver to wait for the vk driver to finish rendering the frame
	qglWaitVkSemaphoreNV((GLuint64)theframe->backbuf->presentsemaphore);

	//tell the gl driver to copy it over now
	qglDrawVkImageNV((GLuint64)theframe->backbuf->colour.image, (GLuint64)theframe->backbuf->colour.sampler, 
			0, 0, vid.pixelwidth, vid.pixelheight,	//xywh (window coords)
			0,	//z
			0, 1, 1, 0);	//stst (remember that gl textures are meant to be upside down)

	//and tell our code to expect it.
	vk.acquirebufferidx[vk.acquirelast%ACQUIRELIMIT] = vk.acquirelast%vk.backbuf_count;
	fence = vk.acquirefences[vk.acquirelast%ACQUIRELIMIT];
	vk.acquirelast++;
	//and actually signal it, so our code can wake up.
	qglSignalVkFenceNV((GLuint64)fence);


	//and the gl driver has its final image and should do something with it now.
	qSwapBuffers(maindc);

//	qwglMakeCurrent(maindc, NULL);

	RSpeedEnd(RSPEED_PRESENT);
}
static qboolean WGL_CheckExtension(char *extname);
static qboolean Win32NVVK_AttachVulkan (rendererstate_t *info)
{	//make sure we can get a valid renderer.

	if (!GL_CheckExtension("GL_NV_draw_vulkan_image"))
	{
		Con_Printf("GL_NV_draw_vulkan_image is not supported. Try using real vulkan instead.\n");
		return false;
	}
	qglGetVkProcAddrNV		= getglfunc("glGetVkProcAddrNV");
	qglDrawVkImageNV		= getglfunc("glDrawVkImageNV");
	qglWaitVkSemaphoreNV	= getglfunc("glWaitVkSemaphoreNV");
	qglSignalVkSemaphoreNV	= getglfunc("glSignalVkSemaphoreNV");
	qglSignalVkFenceNV		= getglfunc("glSignalVkFenceNV");

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)nvvkGetInstanceProcAddr(NULL, "vkGetInstanceProcAddr");
	if (!vkGetInstanceProcAddr)
		vkGetInstanceProcAddr = nvvkGetInstanceProcAddr;
//	qwglMakeCurrent(maindc, NULL);
	return VK_Init(info, NULL, Win32NVVK_CreateSurface, Win32NVVK_Present);
}
#endif

/*doesn't consider parent offsets*/
static RECT centerrect(unsigned int parentleft, unsigned int parenttop, unsigned int parentwidth, unsigned int parentheight, unsigned int cwidth, unsigned int cheight)
{
	RECT r;
	if (modestate!=MS_WINDOWED)
	{
		if (!vid_width.ival)
			cwidth = parentwidth;
		if (!vid_height.ival)
			cheight = parentwidth;
	}

	if (parentwidth < cwidth)
	{
		r.left = parentleft;
		r.right = r.left+parentwidth;
	}
	else
	{
		r.left = parentleft + (parentwidth - cwidth) / 2;
		r.right = r.left + cwidth;
	}

	if (parentheight < cheight)
	{
		r.top = parenttop;
		r.bottom = r.top + parentheight;
	}
	else
	{
		r.top = parenttop + (parentheight - cheight) / 2;
		r.bottom = r.top + cheight;
	}

	return r;
}

static qboolean VID_SetWindowedMode (rendererstate_t *info)
//qboolean VID_SetWindowedMode (int modenum)
{
	int i;
	HDC				hdc;
	int				wwidth, wheight, pleft, ptop, pwidth, pheight;

	modestate = MS_WINDOWED;

	hdc = GetDC(NULL);
	if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
	{
		ReleaseDC(NULL, hdc);
		Con_Printf("Can't run GL in non-RGB mode\n");
		return false;
	}
	vid.dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
	vid.dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(NULL, hdc);

#ifndef FTE_SDL
	if (sys_parentwindow)
	{
		SetWindowLong(sys_parentwindow, GWL_STYLE, GetWindowLong(sys_parentwindow, GWL_STYLE)|WS_OVERLAPPED);
		WindowStyle = WS_CHILDWINDOW|WS_OVERLAPPED;
		ExWindowStyle = 0;

		pleft = sys_parentleft;
		ptop = sys_parenttop;
		pwidth = sys_parentwidth;
		pheight = sys_parentheight;

		wwidth = sys_parentwidth;
		wheight = sys_parentheight;
	}
	else
#endif
	{
		WindowStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU |
					  WS_MINIMIZEBOX;
		ExWindowStyle = 0;

		WindowStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

		pleft = 0;
		ptop = 0;
		pwidth = GetSystemMetrics(SM_CXSCREEN);
		pheight = GetSystemMetrics(SM_CYSCREEN);

		/*Assume dual monitors, and chop the width to try to put it on only one screen
		  "Because of app compatibility reasons these system metrics return the size of the primary monitor"
		  so we shouldn't need this...
		*/
//		if (pwidth >= pheight*2)
//			pwidth /= 2;

		wwidth = info->width;
		wheight = info->height;
		/*win8 code:
		HMONITOR mod = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
		if (mon != INVALID_HANDLE_VALUE)
			GetDpiForMonitor(mon, 0, &vid.dpi_x, &vid.dpi_y);
		*/
//		wwidth = (wwidth*vid.dpi_x)/96;
//		wheight = (wheight*vid.dpi_y)/96;
	}

	WindowRect = centerrect(pleft, ptop, pwidth, pheight, wwidth, wheight);
	if (!sys_parentwindow)
		AdjustWindowRectEx(&WindowRect, WindowStyle, FALSE, 0);

	// Create the DIB window
	if (WinNT)
	{
		dibwindow = CreateWindowExW (
			 ExWindowStyle,
			 WINDOW_CLASS_NAME_W,
			 _L(FULLENGINENAME),
			 WindowStyle,
			 WindowRect.left, WindowRect.top,
			 WindowRect.right - WindowRect.left,
			 WindowRect.bottom - WindowRect.top,
			 sys_parentwindow,
			 NULL,
			 global_hInstance,
			 NULL);
	}
	else
	{
		dibwindow = CreateWindowExA (
			 ExWindowStyle,
			 WINDOW_CLASS_NAME_A,
			 FULLENGINENAME,
			 WindowStyle,
			 WindowRect.left, WindowRect.top,
			 WindowRect.right - WindowRect.left,
			 WindowRect.bottom - WindowRect.top,
			 sys_parentwindow,
			 NULL,
			 global_hInstance,
			 NULL);
	}

	if (!dibwindow)
	{
		Con_Printf ("Couldn't create DIB window");
		return false;
	}

	GetClientRect(dibwindow, &WindowRect);
	WindowRect.right -= WindowRect.left;
	WindowRect.bottom -= WindowRect.top;
	DIBWidth = WindowRect.right;
	DIBHeight = WindowRect.bottom;

	SendMessage (dibwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (dibwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	if (!sys_parentwindow)
	{
#ifdef WS_EX_LAYERED
		int av;
		av = 255*vid_wndalpha.value;
		if (av < 70)
			av = 70;
		if (av < 255)
		{
			HMODULE hm = GetModuleHandleA("user32.dll");
			lpfnSetLayeredWindowAttributes pSetLayeredWindowAttributes;
			pSetLayeredWindowAttributes = (void*)GetProcAddress(hm, "SetLayeredWindowAttributes");

			if (pSetLayeredWindowAttributes)
			{
				// Set WS_EX_LAYERED on this window
				SetWindowLong(dibwindow, GWL_EXSTYLE, GetWindowLong(dibwindow, GWL_EXSTYLE) | WS_EX_LAYERED);

				// Make this window 70% alpha
				pSetLayeredWindowAttributes(dibwindow, 0, (BYTE)av, LWA_ALPHA);
			}
		}
#endif
	}

	ShowWindow (dibwindow, SW_SHOWNORMAL);
	SetFocus(dibwindow);

//	ShowWindow (dibwindow, SW_SHOWDEFAULT);
//	UpdateWindow (dibwindow);

// because we have set the background brush for the window to NULL
// (to avoid flickering when re-sizing the window on the desktop),
// we clear the window to black when created, otherwise it will be
// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.width = Q_atoi(com_argv[i+1]);
	else
	{
		vid.width = 640;
	}

	vid.width &= 0xfff8; // make it a multiple of eight

	if (vid.width < 320)
		vid.width = 320;

	// pick a conheight that matches with correct aspect
	vid.height = vid.width*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.height = Q_atoi(com_argv[i+1]);
	if (vid.height < 200)
		vid.height = 200;

	if (vid.height > info->height)
		vid.height = info->height;
	if (vid.width > info->width)
		vid.width = info->width;

	vid.numpages = 2;

	mainwindow = dibwindow;
	vid_isfullscreen=false;

	CL_UpdateWindowTitle();

	return true;
}

void GLVID_SetCaption(const char *text)
{
	wchar_t wide[2048];
	widen(wide, sizeof(wide), text);
	SetWindowTextW(mainwindow, wide);
}

static qboolean VID_SetFullDIBMode (rendererstate_t *info)
{
	int i;
	HDC				hdc;
	int				wwidth, wheight;
	RECT			rect;

	if (info->fullscreen != 2)
	{	//make windows change res.

		modestate = MS_FULLDIB;

		gdevmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
		if (info->bpp)
			gdevmode.dmFields |= DM_BITSPERPEL;
		if (info->rate)
			gdevmode.dmFields |= DM_DISPLAYFREQUENCY;
		if (info->bpp && (info->bpp < 15))
		{	//low values get you a warning. otherwise only 16 and 32bit are allowed.
			Con_Printf("Forcing at least 15bpp\n");
			gdevmode.dmBitsPerPel = 16;
		}
		else if (info->bpp == 16)
			gdevmode.dmBitsPerPel = 16;
		else
			gdevmode.dmBitsPerPel = 32;
		gdevmode.dmDisplayFrequency = info->rate;
		gdevmode.dmPelsWidth = info->width;
		gdevmode.dmPelsHeight = info->height;
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			Con_SafePrintf((gdevmode.dmFields&DM_DISPLAYFREQUENCY)?"Windows rejected mode %i*%i, %ibpp @%ihz\n":"Windows rejected mode %i*%i, %ibpp\n", (int)gdevmode.dmPelsWidth, (int)gdevmode.dmPelsHeight, (int)gdevmode.dmBitsPerPel, (int)gdevmode.dmDisplayFrequency);
			return false;
		}
	}
	else
	{
		modestate = MS_FULLWINDOW;
		
	}

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = info->width;
	WindowRect.bottom = info->height;

	DIBWidth = info->width;
	DIBHeight = info->height;

	WindowStyle = WS_POPUP;
	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	wwidth = rect.right - rect.left;
	wheight = rect.bottom - rect.top;

	// Create the DIB window
	if(WinNT)
	{
		dibwindow = CreateWindowExW (
			ExWindowStyle,
			WINDOW_CLASS_NAME_W,
			_L(FULLENGINENAME),
			WindowStyle,
			rect.left, rect.top,
			wwidth,
			wheight,
			NULL,
			NULL,
			global_hInstance,
			NULL);
	}
	else
	{
		dibwindow = CreateWindowExA (
			ExWindowStyle,
			WINDOW_CLASS_NAME_A,
			FULLENGINENAME,
			WindowStyle,
			rect.left, rect.top,
			wwidth,
			wheight,
			NULL,
			NULL,
			global_hInstance,
			NULL);
	}

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	{
		BOOL fDisable = TRUE;
		DWORD qDWMWA_TRANSITIONS_FORCEDISABLED = 3;
		HRESULT (WINAPI *pDwmSetWindowAttribute)(HWND hWnd,DWORD dwAttribute,LPCVOID pvAttribute,DWORD cbAttribute);
		dllfunction_t dwm[] =
		{
			{(void*)&pDwmSetWindowAttribute, "DwmSetWindowAttribute"},
			{NULL,NULL}
		};
		if (Sys_LoadLibrary("dwmapi.dll", dwm))
		{
			pDwmSetWindowAttribute(dibwindow, qDWMWA_TRANSITIONS_FORCEDISABLED, &fDisable, sizeof(fDisable));
		}
	}

	SendMessage (dibwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (dibwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	if (modestate == MS_FULLWINDOW)
		ShowWindow (dibwindow, SW_HIDE);//SW_SHOWMAXIMIZED);
	else
		ShowWindow (dibwindow, SW_HIDE);//SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	// Because we have set the background brush for the window to NULL
	// (to avoid flickering when re-sizing the window on the desktop), we
	// clear the window to black when created, otherwise it will be
	// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);


	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.width = Q_atoi(com_argv[i+1]);
	else
		vid.width = 640;

	vid.width &= 0xfff8; // make it a multiple of eight

	if (vid.width < 320)
		vid.width = 320;

	// pick a conheight that matches with correct aspect
	vid.height = vid.width*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.height = Q_atoi(com_argv[i+1]);
	if (vid.height < 200)
		vid.height = 200;

	if (vid.height > info->height)
		vid.height = info->height;
	if (vid.width > info->width)
		vid.width = info->width;

	vid.numpages = 2;

// needed because we're not getting WM_MOVE messages fullscreen on NT
	window_x = 0;
	window_y = 0;
	vid_isfullscreen=true;

	mainwindow = dibwindow;

	return true;
}

extern qboolean gammaworks;
static void Win_Touch_Init(HWND wnd);

#ifdef WTHREAD
static rendererstate_t *rs;
static qboolean CreateMainWindow(rendererstate_t *info, qboolean withthread);
static int GLVID_WindowThread(void *cond)
{
	extern qboolean mouseshowtoggle;
	int cursor = 1;
	MSG		msg;
	HWND wnd;
	CreateMainWindow(rs, false);
	wnd = mainwindow;
	Sys_ConditionSignal(cond);

	while (GetMessageW(&msg, NULL, 0, 0))
	{
//		TranslateMessageW (&msg);
		DispatchMessageW (&msg);

		//ShowCursor is thread-local.
		if (cursor != mouseshowtoggle)
		{
			cursor = mouseshowtoggle;
			ShowCursor(cursor);
		}
	}
	DestroyWindow(wnd);
	return 0;
}
#endif
static qboolean CreateMainWindow(rendererstate_t *info, qboolean withthread)
{
	qboolean		stat;

#ifdef WTHREAD
	if (withthread && vid_winthread.ival)
	{
		void *cond = Sys_CreateConditional();
		Sys_LockConditional(cond);
		rs = info;
		windowthread = Sys_CreateThread("windowthread", GLVID_WindowThread, cond, 0, 0);
		if (!Sys_ConditionWait(cond))
			Con_SafePrintf ("Looks like the window thread isn't starting up\n");
		Sys_UnlockConditional(cond);
		Sys_DestroyConditional(cond);

		return !!mainwindow;
	}
#endif

	if (WinNT)
	{
		WNDCLASSW		wc;
		/* Register the frame class */
		wc.style         = CS_OWNDC;
		wc.lpfnWndProc   = (WNDPROC)GLMainWndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = global_hInstance;
		wc.hIcon         = hIcon;
		wc.hCursor       = hArrowCursor;
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME_W;
		if (!RegisterClassW (&wc))	//this isn't really fatal, we'll let the CreateWindow fail instead.
			Con_DPrintf("RegisterClass failed\n");
	}
	else
	{
		WNDCLASSA		wc;
		/* Register the frame class */
		wc.style         = CS_OWNDC;
		wc.lpfnWndProc   = (WNDPROC)GLMainWndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = global_hInstance;
		wc.hIcon         = hIcon;
		wc.hCursor       = hArrowCursor;
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME_A;
		if (!RegisterClassA (&wc))	//this isn't really fatal, we'll let the CreateWindow fail instead.
			Con_DPrintf("RegisterClass failed\n");
	}

	if (!info->fullscreen)
	{
		TRACE(("dbg: GLVID_SetMode: VID_SetWindowedMode\n"));
		stat = VID_SetWindowedMode(info);
	}
	else
	{
		TRACE(("dbg: GLVID_SetMode: VID_SetFullDIBMode\n"));
		stat = VID_SetFullDIBMode(info);
	}
	VID_UpdateWindowStatus(mainwindow);

	Win_Touch_Init(mainwindow);

	INS_UpdateGrabs(info->fullscreen, vid.activeapp);

	return stat;
}

static void VID_UnSetMode (void);
static int GLVID_SetMode (rendererstate_t *info, unsigned char *palette)
{
	int				temp;
	qboolean		stat;
	MSG				msg;
//	HDC				hdc;

	vrsetup_t setup = {sizeof(setup)};

	TRACE(("dbg: GLVID_SetMode\n"));

// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();

	qGetDeviceGammaRamp = (void*)GetDeviceGammaRamp;
	qSetDeviceGammaRamp = (void*)SetDeviceGammaRamp;

	switch(platform_rendermode)
	{
#ifdef USE_WGL
	case MODE_WGL:
#ifdef VKQUAKE
	case MODE_NVVULKAN:
#endif
		setup.vrplatform = VR_WIN_WGL;
		if (info->vr && !info->vr->Prepare(&setup))
		{
			info->vr->Shutdown();
			info->vr = NULL;
		}

		// Set either the fullscreen or windowed mode
		qwglChoosePixelFormatARB = NULL;
		qwglGetPixelFormatAttribfvARB = NULL;
		qwglCreateContextAttribsARB = NULL;
		stat = CreateMainWindow(info, true);

		if (stat)
		{
			stat = VID_AttachGL(info);
			if (stat)
			{
				TRACE(("dbg: GLVID_SetMode: attaching gl okay\n"));
				if (CheckForcePixelFormat(info))
				{
					HMODULE oldgl = hInstGL;
					hInstGL = NULL;	//don't close the gl library, just in case
					VID_UnSetMode();	//SetPixelFormat may only be used once per window. which means we *MUST* destroy the window if we're using a different pixelformat.
					hInstGL = oldgl;

					if (CreateMainWindow(info, true) && VID_AttachGL(info))
					{
						//we have our multisample window
					}
					else
					{
						//multisample failed
						//try the origional way
						if (!CreateMainWindow(info, true) || !VID_AttachGL(info))
						{
							Con_Printf("Failed to undo antialising. Giving up.\n");
							return false;	//eek
						}
					}
				}
			}
			else
			{
				TRACE(("dbg: GLVID_SetMode: attaching gl failed\n"));
				return false;
			}
		}
		else
		{
			TRACE(("dbg: GLVID_SetMode: unable to create window\n"));
			return false;
		}

		if (modestate == MS_FULLWINDOW)
			ShowWindow (dibwindow, SW_SHOWMAXIMIZED);
		else
			ShowWindow (dibwindow, SW_SHOWNORMAL);	

		if (!GL_Init(info, getglfunc))
			return false;
		setup.wgl.hdc = maindc;
		setup.wgl.hglrc = baseRC;
		if (info->vr && !info->vr->Init(&setup, info))
		{
			info->vr->Shutdown();
			return false;
		}
		vid.vr = info->vr;

		if (qwglGetPixelFormatAttribfvARB)	//just for debugging info.
		{
			int iAttributeNames[] = {WGL_RED_BITS_ARB, WGL_GREEN_BITS_ARB, WGL_BLUE_BITS_ARB, WGL_ALPHA_BITS_ARB, WGL_PIXEL_TYPE_ARB, WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB};
			float fAttributeValues[countof(iAttributeNames)] = {0};
			if (qwglGetPixelFormatAttribfvARB(maindc, currentpixelformat, 0, countof(iAttributeNames), iAttributeNames, fAttributeValues))
			{
				Con_DPrintf("Colour buffer: GL_R%gG%gB%gA%g%s\n", fAttributeValues[0], fAttributeValues[1], fAttributeValues[2], fAttributeValues[3], fAttributeValues[5]==WGL_TYPE_RGBA_FLOAT_ARB?"F":((vid.flags & VID_SRGBAWARE)?"_SRGB":""));
				Con_DPrintf("Depth buffer: GL_DEPTH%g_STENCIL%g\n", fAttributeValues[5], fAttributeValues[6]);
			}
		}


		qSwapBuffers(maindc);

#ifdef VKQUAKE
		if (platform_rendermode == MODE_NVVULKAN && stat)
		{
			stat = Win32NVVK_AttachVulkan(info);
		}
#endif
		break;
#endif
#ifdef USE_EGL
	case MODE_EGL:
		stat = CreateMainWindow(info, true);
		if (stat)
		{
			EGLConfig cfg;
			maindc = GetDC(mainwindow);

			if (!EGL_InitDisplay(info, EGL_PLATFORM_WIN32, maindc, (EGLNativeDisplayType)maindc, &cfg))
			{
				Con_Printf("couldn't find suitable EGL config\n");
				return false;
			}
			if (!EGL_InitWindow(info, EGL_PLATFORM_WIN32, mainwindow, (EGLNativeWindowType)mainwindow, cfg))
			{
				Con_Printf("couldn't initialise EGL context\n");
				return false;
			}

			if (!GL_Init(info, &EGL_Proc))
				return false;
		}
		break;
#endif
#ifdef VKQUAKE
	case MODE_VULKAN:
		stat = CreateMainWindow(info, true);

		if (stat)
		{
			maindc = GetDC(mainwindow);
			stat = Win32VK_AttachVulkan(info);
		}
		break;
#endif
	default:
		stat = false;
		break;
	}

	if (!stat)
	{
		TRACE(("dbg: GLVID_SetMode: VID_Set... failed\n"));
		return false;
	}

	if (!qGetDeviceGammaRamp) qGetDeviceGammaRamp = (void*)GetDeviceGammaRamp;
	if (!qSetDeviceGammaRamp) qSetDeviceGammaRamp = (void*)SetDeviceGammaRamp;


	window_width = DIBWidth;
	window_height = DIBHeight;
	VID_UpdateWindowStatus (mainwindow);
	Cvar_ForceCallback(&vid_conautoscale);

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

// now we try to make sure we get the focus on the mode switch, because
// sometimes in some systems we don't.  We grab the foreground, then
// finish setting up, pump all our messages, and sleep for a little while
// to let messages finish bouncing around the system, then we put
// ourselves at the top of the z order, then grab the foreground again,
// Who knows if it helps, but it probably doesn't hurt
	SetForegroundWindow (mainwindow);

	/*I don't like this, but if we */
	Sleep (100);
	while (PeekMessage (&msg, mainwindow, 0, 0, PM_REMOVE))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	Sleep (100);

	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0,
				  SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW |
				  SWP_NOCOPYBITS);

	SetForegroundWindow (mainwindow);

	Sleep (100);
	while (PeekMessage (&msg, mainwindow, 0, 0, PM_REMOVE))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	Sleep (100);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

	gammaworks = FALSE;
	if (vid_desktopgamma.value)
	{
		HDC hDC = GetDC(GetDesktopWindow());
		if (qGetDeviceGammaRamp(hDC, originalgammaramps))
			gammaworks = qSetDeviceGammaRamp(hDC, originalgammaramps);
		ReleaseDC(GetDesktopWindow(), hDC);
	}
	else
	{
		if (qGetDeviceGammaRamp(maindc, originalgammaramps))
			gammaworks = qSetDeviceGammaRamp(maindc, originalgammaramps);
	}
	if (!gammaworks)
		Con_Printf("Hardware gamma is not supported\n");
	else
		Con_DPrintf("Hardware gamma appears to work\n");

	return true;
}

void VID_UnSetMode (void)
{
	if (mainwindow && vid_initialized)
	{
		GLAppActivate(false, false);

		vid_canalttab = false;

		switch(platform_rendermode)
		{
#if defined(VKQUAKE) && defined(USE_WGL)
		case MODE_NVVULKAN:
			qwglMakeCurrent(maindc, baseRC);
			VK_Shutdown();
			ReleaseGL();
			break;
#endif
#ifdef USE_WGL
		case MODE_WGL:
			ReleaseGL();
			break;
#endif
#ifdef USE_EGL
		case MODE_EGL:
			EGL_Shutdown();
			break;
#endif
#ifdef VKQUAKE
		case MODE_VULKAN:
			VK_Shutdown();
			break;
#endif
		}

		if (modestate == MS_FULLDIB)
			ChangeDisplaySettings (NULL, 0);

		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);
		maindc = NULL;
	}

	if (mainwindow)
	{
		dibwindow=NULL;
	//	ShowWindow(mainwindow, SW_HIDE);
	//	SetWindowLongPtr(mainwindow, GWL_WNDPROC, DefWindowProc);
	//	PostMessage(mainwindow, WM_CLOSE, 0, 0);
#ifdef WTHREAD
		if (windowthread)
		{
			SendMessage(mainwindow, WM_USER+4, 0, 0);
			Sys_WaitOnThread(windowthread);
			windowthread = NULL;
		}
		else
#endif
			DestroyWindow(mainwindow);
		mainwindow = NULL;
	}

	if (WinNT)
		UnregisterClassW(WINDOW_CLASS_NAME_W, global_hInstance);
	else
		UnregisterClassA(WINDOW_CLASS_NAME_A, global_hInstance);

#if 0
	//Logically this code should be active. However...
	//1: vid_restarts are slightly slower if we don't reuse the old dll
	//2: nvidia drivers crash if we shut it down+reload!
	if (hInstGL)
	{
		FreeLibrary(hInstGL);
		hInstGL=NULL;
	}
	if (hInstwgl)
	{
		FreeLibrary(hInstwgl);
		hInstwgl=NULL;
	}
	*opengldllname = 0;
#endif

#ifdef GLQUAKE
	GL_ForgetPointers();
#endif
}


/*
================
VID_UpdateWindowStatus
================
*/
static void VID_UpdateWindowStatus (HWND hWnd)
{
	POINT p;
	RECT nr;
	GetClientRect(hWnd, &nr);

	//if its bad then we're probably minimised
	if (nr.right <= nr.left)
		return;
	if (nr.bottom <= nr.top)
		return;

	WindowRect = nr;
	p.x = 0;
	p.y = 0;
	ClientToScreen(hWnd, &p);
	window_x = p.x;
	window_y = p.y;
	window_width = WindowRect.right - WindowRect.left;
	window_height = WindowRect.bottom - WindowRect.top;

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	INS_UpdateClipCursor ();

	switch(platform_rendermode)
	{
#ifdef VKQUAKE
#ifdef USE_WGL
	case MODE_NVVULKAN:
#endif
	case MODE_VULKAN:
		if (vid.pixelwidth != window_width || vid.pixelheight != window_height)
			vk.neednewswapchain = true;
		break;
#endif
	default:
		vid.pixelwidth = window_width;
		vid.pixelheight = window_height;
		break;
	}
}

#ifdef USE_WGL
static qboolean WGL_CheckExtension(char *extname)
{
//	int i;
	int len;
	const char *foo;
	cvar_t *v = Cvar_Get(va("gl_ext_%s", extname), "1", 0, "GL Extensions");
	if (v && !v->ival)
	{
		Con_Printf("Cvar %s is 0\n", v->name);
		return false;
	}

/*	if (gl_num_extensions && qglGetStringi)
	{
		for (i = 0; i < gl_num_extensions; i++)
			if (!strcmp(qglGetStringi(GL_EXTENSIONS, i), extname))
			{
				Con_DPrintf("Detected GL extension %s\n", extname);
				return true;
			}
	}
*/
	if (!wgl_extensions)
		return false;

	//the list is space delimited. we cannot just strstr lest we find leading/trailing _FOO_.
	len = strlen(extname);
	for (foo = wgl_extensions; *foo; )
	{
		if (!strncmp(foo, extname, len) && (foo[len] == ' ' || !foo[len]))
			return true;
		while(*foo && *foo != ' ')
			foo++;
		if (*foo == ' ')
			foo++;
	}
	return false;
}

//====================================

qboolean VID_AttachGL (rendererstate_t *info)
{	//make sure we can get a valid renderer.
	int iAttributeNames[2];
	FLOAT fAttributeValues[countof(iAttributeNames)];

	do
	{
		TRACE(("dbg: VID_AttachGL: GLInitialise\n"));
		if (GLInitialise(info->subrenderer))
		{
			maindc = GetDC(mainwindow);
			TRACE(("dbg: VID_AttachGL: bSetupPixelFormat\n"));
			if (bSetupPixelFormat(maindc, info))
				break;
			ReleaseDC(mainwindow, maindc);
		}

		if (!*info->subrenderer || !stricmp(info->subrenderer, "opengl32.dll") || !stricmp(info->subrenderer, "opengl32"))	//go for windows system dir if we failed with the default. Should help to avoid the 3dfx problem.
		{
			wchar_t systemglw[MAX_OSPATH+1];
			char systemgl[MAX_OSPATH+1];
			GetSystemDirectoryW(systemglw, countof(systemglw)-1);
			narrowen(systemgl, sizeof(systemgl), systemglw);
			Q_strncatz(systemgl, "\\", sizeof(systemgl));
			if (*info->subrenderer)
				Q_strncatz(systemgl, info->subrenderer, sizeof(systemgl));
			else
				Q_strncatz(systemgl, "opengl32.dll", sizeof(systemgl));
			TRACE(("dbg: VID_AttachGL: GLInitialise (system dir specific)\n"));
			if (GLInitialise(systemgl))
			{
				maindc = GetDC(mainwindow);
				TRACE(("dbg: VID_AttachGL: bSetupPixelFormat\n"));
				if (bSetupPixelFormat(maindc, info))
					break;
				ReleaseDC(mainwindow, maindc);
			}
		}

		TRACE(("dbg: VID_AttachGL: failed to find a valid dll\n"));
		return false;
	} while(1);

	TRACE(("dbg: VID_AttachGL: qwglCreateContext\n"));

	baseRC = qwglCreateContext(maindc);
	if (!baseRC)
	{
		Con_SafePrintf(CON_ERROR "Could not initialize GL (wglCreateContext failed).\n\nMake sure you in are 65535 color mode, and try running -window.\n");	//green to make it show.
		return false;
	}
	TRACE(("dbg: VID_AttachGL: qwglMakeCurrent\n"));
	if (!qwglMakeCurrent(maindc, baseRC))
	{
		Con_SafePrintf(CON_ERROR "wglMakeCurrent failed\n");	//green to make it show.
		return false;
	}

	{
		char *(WINAPI *wglGetExtensionsString)(HDC hdc) = NULL;
		if (!wglGetExtensionsString)
			wglGetExtensionsString = getglfunc("wglGetExtensionsString");
		if (!wglGetExtensionsString)
			wglGetExtensionsString = getglfunc("wglGetExtensionsStringARB");
		if (!wglGetExtensionsString)
			wglGetExtensionsString = getglfunc("wglGetExtensionsStringEXT");
		if (wglGetExtensionsString)
			wgl_extensions = wglGetExtensionsString(maindc);
		else
			wgl_extensions = NULL;
	}

	if (developer.ival)
		Con_SafePrintf("WGL_EXTENSIONS: %s\n", wgl_extensions?wgl_extensions:"NONE");

	qwglCreateContextAttribsARB = getglfunc("wglCreateContextAttribsARB");

	//attempt to promote that to opengl3.
	if (qwglCreateContextAttribsARB)
	{
		HGLRC opengl3;
		int attribs[11];
		char *mv;
		int i = 0;
		char *ver;

		ver = vid_gl_context_version.string;
		if (!*ver && vid_gl_context_es.ival && WGL_CheckExtension("WGL_EXT_create_context_es2_profile"))
			ver = "2.0";

		mv = ver;
		while (*mv)
		{
			if (*mv++ == '.')
				break;
		}

		if (*ver)
		{
			attribs[i++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
			attribs[i++] = atoi(ver);
		}
		if (*mv)
		{
			attribs[i++] = WGL_CONTEXT_MINOR_VERSION_ARB;
			attribs[i++] = atoi(mv);
		}

		//flags
		attribs[i+1] = 0;
		if (vid_gl_context_debug.ival)
			attribs[i+1] |= WGL_CONTEXT_DEBUG_BIT_ARB;
		if (vid_gl_context_forwardcompatible.ival)
			attribs[i+1] |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if (vid_gl_context_robustness.ival && WGL_CheckExtension("WGL_ARB_create_context_robustness"))
			attribs[i+1] |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;

		if (attribs[i+1])
		{
			attribs[i] = WGL_CONTEXT_FLAGS_ARB;
			i += 2;
		}

		if (vid_gl_context_selfreset.ival &&  WGL_CheckExtension("WGL_ARB_create_context_robustness"))
		{
			attribs[i++] = WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB;
			attribs[i++] = WGL_LOSE_CONTEXT_ON_RESET_ARB;
		}

		if (vid_gl_context_noerror.ival &&  WGL_CheckExtension("WGL_ARB_create_context_no_error"))
		{
			attribs[i++] = WGL_CONTEXT_OPENGL_NO_ERROR_ARB;
			attribs[i++] = !vid_gl_context_robustness.ival && !vid_gl_context_debug.ival;
		}

		/*only switch contexts if there's actually a point*/
		if (i || !vid_gl_context_compatibility.ival || vid_gl_context_es.ival)
		{
			if (WGL_CheckExtension("WGL_ARB_create_context_profile"))
			{
				attribs[i+1] = 0;
				if (vid_gl_context_es.ival && (WGL_CheckExtension("WGL_EXT_create_context_es_profile") || WGL_CheckExtension("WGL_EXT_create_context_es2_profile")))
					attribs[i+1] |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;
				else if (vid_gl_context_compatibility.ival)
					attribs[i+1] |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
				else
					attribs[i+1] |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
				attribs[i] = WGL_CONTEXT_PROFILE_MASK_ARB;
				//WGL_CONTEXT_PROFILE_MASK_ARB is ignored if < 3.2 - however, nvidia do not agree and return errors
				if (atof(ver) >= 3.2 || vid_gl_context_es.ival)
					i+=2;
			}

			attribs[i] = 0;

			if ((opengl3 = qwglCreateContextAttribsARB(maindc, NULL, attribs)))
			{
				qwglMakeCurrent(maindc, NULL);
				qwglDeleteContext(baseRC);

				baseRC = opengl3;
				if (!qwglMakeCurrent( maindc, baseRC ))
				{
					Con_SafePrintf(CON_ERROR "wglMakeCurrent failed\n");	//green to make it show.
					return false;
				}
			}
			else
			{
				DWORD error = GetLastError();
				if (error == (0xc0070000 | ERROR_INVALID_VERSION_ARB))
					Con_Printf("Unsupported OpenGL context version (%s).\n", vid_gl_context_version.string);
				else if (error == (0xc0070000 | ERROR_INVALID_PROFILE_ARB))
					Con_Printf("Unsupported OpenGL profile (%s).\n", vid_gl_context_es.ival?"gles":(vid_gl_context_compatibility.ival?"compat":"core"));
				else if (error == (0xc0070000 | ERROR_INVALID_OPERATION))
					Con_Printf("wglCreateContextAttribsARB returned invalid operation.\n");
				else if (error == (0xc0070000 | ERROR_DC_NOT_FOUND))
					Con_Printf("wglCreateContextAttribsARB returned dc not found.\n");
				else if (error == (0xc0070000 | ERROR_INVALID_PIXEL_FORMAT))
					Con_Printf("wglCreateContextAttribsARB returned dc not found.\n");
				else if (error == (0xc0070000 | ERROR_NO_SYSTEM_RESOURCES))
					Con_Printf("wglCreateContextAttribsARB ran out of system resources.\n");
				else if (error == (0xc0070000 | ERROR_INVALID_PARAMETER))
					Con_Printf("wglCreateContextAttribsARB reported invalid parameter.\n");
				else
					Con_Printf("Unknown error creating an OpenGL (%s) Context.\n", vid_gl_context_version.string);
			}
		}
	}

	qwglChoosePixelFormatARB	= getglfunc("wglChoosePixelFormatARB");
	qwglGetPixelFormatAttribfvARB = getglfunc("wglGetPixelFormatAttribfvARB");

	if (info->stereo)
	{
		GLboolean ster = false;
		qglGetBooleanv(GL_STEREO, &ster);
		if (!ster)
			Con_Printf("Unable to create stereoscopic/quad-buffered OpenGL context. Please use a different stereoscopic method.\n");
	}

	qwglSwapIntervalEXT		= getglfunc("wglSwapIntervalEXT");
	if (qwglSwapIntervalEXT && *vid_vsync.string)
	{
		TRACE(("dbg: VID_AttachGL: qwglSwapIntervalEXT\n"));
		qwglSwapIntervalEXT(vid_vsync.value);
	}

	vid.flags = 0;
	iAttributeNames[0] = WGL_PIXEL_TYPE_ARB;
	iAttributeNames[1] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
	if (qwglGetPixelFormatAttribfvARB && qwglGetPixelFormatAttribfvARB(maindc, currentpixelformat, 0, 1, iAttributeNames+0, fAttributeValues+0) && fAttributeValues[0] == WGL_TYPE_RGBA_FLOAT_ARB)
		vid.flags |= VID_FP16 | VID_SRGB_FB_LINEAR;
	if (qwglGetPixelFormatAttribfvARB && qwglGetPixelFormatAttribfvARB(maindc, currentpixelformat, 0, 1, iAttributeNames+1, fAttributeValues+1) && fAttributeValues[1] == TRUE)
		vid.flags |= VID_SRGB_CAPABLE;
	return true;
}
#endif

static void QDECL VID_Wait_Override_Callback(struct cvar_s *var, char *oldvalue)
{
	switch(platform_rendermode)
	{
#ifdef USE_WGL
	case MODE_WGL:
		if (qwglSwapIntervalEXT && *vid_vsync.string)
			qwglSwapIntervalEXT(vid_vsync.value);
		break;
#endif
	default:
		break;
	}
}

static void GLVID_Recenter_f(void)
{
	// 4 unused variables
	//int nw = vid_width.value;
	//int nh = vid_height.value;
	//int nx = 0;
	//int ny = 0;

	if (Cmd_Argc() > 1)
		sys_parentleft = atoi(Cmd_Argv(1));
	if (Cmd_Argc() > 2)
		sys_parenttop = atoi(Cmd_Argv(2));
	if (Cmd_Argc() > 3)
		sys_parentwidth = atoi(Cmd_Argv(3));
	if (Cmd_Argc() > 4)
		sys_parentheight = atoi(Cmd_Argv(4));
	if (Cmd_Argc() > 5)
	{
		HWND newparent = (HWND)(DWORD_PTR)strtoull(Cmd_Argv(5), NULL, 16);
		if (newparent != sys_parentwindow && mainwindow && modestate==MS_WINDOWED)
			SetParent(mainwindow, sys_parentwindow);
		sys_parentwindow = newparent;
	}

	if (sys_parentwindow && modestate==MS_WINDOWED)
	{
		WindowRect = centerrect(sys_parentleft, sys_parenttop, sys_parentwidth, sys_parentheight, sys_parentwidth, sys_parentheight);
		MoveWindow(mainwindow, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, FALSE);

		VID_UpdateWindowStatus (mainwindow);
		Cvar_ForceCallback(&vid_conautoscale);
	}
}

static void QDECL VID_WndAlpha_Override_Callback(struct cvar_s *var, char *oldvalue)
{
	//this code tells windows to use the alpha channel of the screen, but does really nasty things with the mouse such that its unplayable.
	//its not useful.
/*	if (modestate==MS_WINDOWED)
	{
		struct qDWM_BLURBEHIND 
		{
			  DWORD dwFlags;
			  BOOL  fEnable;
			  HRGN  hRgnBlur;
			  BOOL  fTransitionOnMaximized;
		} bb = {1, true, NULL, true};
		HRESULT (WINAPI *pDwmEnableBlurBehindWindow)(HWND hWnd,const struct qDWM_BLURBEHIND *pBlurBehind);
		dllfunction_t dwm[] =
		{
			{(void*)&pDwmEnableBlurBehindWindow, "DwmEnableBlurBehindWindow"},
			{NULL,NULL}
		};
		if (Sys_LoadLibrary("dwmapi.dll", dwm))
			pDwmEnableBlurBehindWindow(mainwindow, &bb);
	}
*/

#ifdef WS_EX_LAYERED
	//enable whole-window fixed transparency. should work in win2k+
	//note that this can destroy framerates, and they won't reset when the setting is reverted to 1.
	//be prepared to do a vid_restart.
	if (modestate==MS_WINDOWED)
	{
		int av;
		HMODULE hm = GetModuleHandleA("user32.dll");
		lpfnSetLayeredWindowAttributes pSetLayeredWindowAttributes;
		pSetLayeredWindowAttributes = (void*)GetProcAddress(hm, "SetLayeredWindowAttributes");

		av = 255 * var->value;
		if (av < 70)
			av = 70;
		if (av > 255)
			av = 255;

		if (pSetLayeredWindowAttributes)
		{
			// Set WS_EX_LAYERED on this window

			if (av < 255)
			{
				SetWindowLong(mainwindow, GWL_EXSTYLE, GetWindowLong(mainwindow, GWL_EXSTYLE) | WS_EX_LAYERED);

				// Make this window 70% alpha
				pSetLayeredWindowAttributes(mainwindow, 0, (BYTE)av, LWA_ALPHA);
			}
			else
			{
				SetWindowLong(mainwindow, GWL_EXSTYLE, GetWindowLong(mainwindow, GWL_EXSTYLE) & ~WS_EX_LAYERED);
				pSetLayeredWindowAttributes(mainwindow, 0, (BYTE)255, LWA_ALPHA);
			}
		}
	}
#endif
}

void GLVID_SwapBuffers (void)
{
	switch(platform_rendermode)
	{
#ifdef USE_WGL
	case MODE_WGL:
		if (qwglSwapLayerBuffers)
		{
			if (!qwglSwapLayerBuffers(maindc, WGL_SWAP_MAIN_PLANE))
				qwglSwapLayerBuffers = NULL;
		}
		else
			qSwapBuffers(maindc);
		break;
#endif
#ifdef USE_EGL
	case MODE_EGL:
		EGL_SwapBuffers();
		break;
#endif
#ifdef VKQUAKE
	case MODE_VULKAN:
#ifdef USE_WGL
	case MODE_NVVULKAN:
#endif
		//FIXME: force a buffer swap now (might be called while loading (eg: q3), where we won't get a chance to redraw for a bit)
		break;
#endif
	}

// handle the mouse state when windowed if that's changed

	INS_UpdateGrabs(modestate != MS_WINDOWED, vid.activeapp);
}

static void OblitterateOldGamma(void)
{
	int i;
	if (vid_preservegamma.value)
		return;

	for (i = 0; i < 256; i++)
	{
		originalgammaramps[0][i] = (i<<8) + i;
		originalgammaramps[1][i] = (i<<8) + i;
		originalgammaramps[2][i] = (i<<8) + i;
	}
}

qboolean GLVID_ApplyGammaRamps (unsigned int gammarampsize, unsigned short *ramps)
{
	if (ramps)
	{
		switch(vid_hardwaregamma.ival)
		{
		case 0:	//never use hardware/glsl gamma
		case 2:	//ALWAYS use glsl gamma
		case 4:	//scene-only gamma
			return false;
		default:
		case 1:	//no hardware gamma when windowed
			if (modestate == MS_WINDOWED)
				return false;
			break;
		case 3:	//ALWAYS try to use hardware gamma, even when it fails...
			break;
		}

		if (vid.activeapp)	//this is needed because ATI drivers don't work properly (or when task-switched out).
		{	//we have hardware gamma applied - if we're doing a BF, we don't want to reset to the default gamma (yuck)
			if (vid_desktopgamma.value)
			{
				HDC hDC = GetDC(GetDesktopWindow());
				qSetDeviceGammaRamp (hDC, ramps);
				ReleaseDC(GetDesktopWindow(), hDC);
			}
			else
			{
				qSetDeviceGammaRamp (maindc, ramps);
			}
			return true;
		}
		return false;
	}
	else if (gammaworks)
	{
		//revert to default
		if (qSetDeviceGammaRamp)
		{
			OblitterateOldGamma();

			if (vid_desktopgamma.value)
			{
				HDC hDC = GetDC(GetDesktopWindow());
				qSetDeviceGammaRamp (hDC, originalgammaramps);
				ReleaseDC(GetDesktopWindow(), hDC);
			}
			else
			{
				qSetDeviceGammaRamp(maindc, originalgammaramps);
			}
		}
		return true;
	}
	return false;
}

void GLVID_Crashed(void)
{
	if (qSetDeviceGammaRamp && gammaworks)
	{
		OblitterateOldGamma();
		qSetDeviceGammaRamp(maindc, originalgammaramps);
	}
}

void	GLVID_Shutdown (void)
{
	if (qSetDeviceGammaRamp)
	{
		OblitterateOldGamma();

		if (vid_desktopgamma.value)
		{
			HDC hDC = GetDC(GetDesktopWindow());
			qSetDeviceGammaRamp(hDC, originalgammaramps);
			ReleaseDC(GetDesktopWindow(), hDC);
		}
		else
		{
			qSetDeviceGammaRamp(maindc, originalgammaramps);
		}
	}
	qSetDeviceGammaRamp = NULL;
	qGetDeviceGammaRamp = NULL;

	gammaworks = false;

//	GLBE_Shutdown();
//	Image_Shutdown();
	VID_UnSetMode();
}


//==========================================================================

#ifdef USE_WGL
static BOOL CheckForcePixelFormat(rendererstate_t *info)
{
	if (qwglChoosePixelFormatARB && (info->multisample || info->srgb || info->bpp==30))
	{
		HDC hDC;
		int valid;
		float fAttribute[] = {0,0};
		UINT numFormats;
		int pixelformats[1];
		int iAttributes = 0;
		int iAttribute[16*2];

		iAttribute[iAttributes++] = WGL_DRAW_TO_WINDOW_ARB;				iAttribute[iAttributes++] = GL_TRUE;
		iAttribute[iAttributes++] = WGL_SUPPORT_OPENGL_ARB;				iAttribute[iAttributes++] = GL_TRUE;
		iAttribute[iAttributes++] = WGL_ACCELERATION_ARB;				iAttribute[iAttributes++] = WGL_FULL_ACCELERATION_ARB;

		if (info->srgb>=2 && modestate != MS_WINDOWED)
		{	//half-float backbuffers!

			//'as has been the case since Windows Vista, fp16 swap chains are expected to have linear color data'
			//if we try using WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, then we won't actually get a pixelformat.
			//we just have to assume that its a linear colour space.

			//we ONLY use this fullscreen, because its totally fucked on nvidia otherwise.
			//when windowed, its an scRGB image but with and srgb capable false, when fullscreen its always linear until something else takes focus...
			//so if windowed or unfocused or whatever, we would need to use custom glsl to fix the gamma settings.

			//additionally, a single program using floats will disable the desktop compositor, which can be a bit jarring.

			iAttribute[iAttributes++] = WGL_PIXEL_TYPE_ARB;					iAttribute[iAttributes++] = WGL_TYPE_RGBA_FLOAT_ARB;
			iAttribute[iAttributes++] = WGL_RED_BITS_ARB;					iAttribute[iAttributes++] = 16;
			iAttribute[iAttributes++] = WGL_GREEN_BITS_ARB;					iAttribute[iAttributes++] = 16;
			iAttribute[iAttributes++] = WGL_BLUE_BITS_ARB;					iAttribute[iAttributes++] = 16;
		}
		else
		{
			if (info->srgb)
			{
				iAttribute[iAttributes++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;	iAttribute[iAttributes++] = info->bpp<=32;
			}
			if (info->bpp==30)
			{	//10-bit backbuffers!
	//			iAttribute[iAttributes++] = WGL_PIXEL_TYPE_ARB;					iAttribute[iAttributes++] = WGL_TYPE_RGBA_FLOAT_ARB;
				iAttribute[iAttributes++] = WGL_RED_BITS_ARB;					iAttribute[iAttributes++] = 10;
				iAttribute[iAttributes++] = WGL_GREEN_BITS_ARB;					iAttribute[iAttributes++] = 10;
				iAttribute[iAttributes++] = WGL_BLUE_BITS_ARB;					iAttribute[iAttributes++] = 10;
			}
			else
			{
				iAttribute[iAttributes++] = WGL_COLOR_BITS_ARB;					iAttribute[iAttributes++] = (info->bpp>24)?24:info->bpp;
			}
		}
//		iAttribute[iAttributes++] = WGL_ALPHA_BITS_ARB;					iAttribute[iAttributes++] = 2;
		iAttribute[iAttributes++] = WGL_DEPTH_BITS_ARB;					iAttribute[iAttributes++] = info->depthbits?info->depthbits:24;
		iAttribute[iAttributes++] = WGL_STENCIL_BITS_ARB;				iAttribute[iAttributes++] = 8;
		iAttribute[iAttributes++] = WGL_DOUBLE_BUFFER_ARB;				iAttribute[iAttributes++] = GL_TRUE;
		iAttribute[iAttributes++] = WGL_STEREO_ARB;						iAttribute[iAttributes++] = info->stereo;
		if (info->multisample)
		{
			iAttribute[iAttributes++] = WGL_SAMPLE_BUFFERS_ARB;				iAttribute[iAttributes++] = GL_TRUE;
			iAttribute[iAttributes++] = WGL_SAMPLES_ARB,					iAttribute[iAttributes++] = info->multisample;						// Check For 4x Multisampling
		}
		iAttribute[iAttributes++] = 0;									iAttribute[iAttributes++] = 0;


		TRACE(("dbg: bSetupPixelFormat: attempting wglChoosePixelFormatARB (multisample 4)\n"));
		hDC = GetDC(mainwindow);

		valid = qwglChoosePixelFormatARB(hDC,iAttribute,fAttribute,countof(pixelformats),pixelformats,&numFormats);
/*		while ((!valid || numFormats < 1) && iAttribute[19] > 1)
		{	//failed, switch wgl_samples to 2
			iAttribute[19] /= 2;
			TRACE(("dbg: bSetupPixelFormat: attempting wglChoosePixelFormatARB (smaller multisample)\n"));
			valid = qwglChoosePixelFormatARB(hDC,iAttribute,fAttribute,1,&pixelformat,&numFormats);
		}
*/

#if 0
		for (iAttributes = -1; iAttributes < (int)numFormats; iAttributes++)
		{
			int j;
			struct
			{
				char *name;
				int id;
			} iAttributeTable[] = {
				{"WGL_DRAW_TO_WINDOW",				WGL_DRAW_TO_WINDOW_ARB},
				{"WGL_DRAW_TO_BITMAP",				0x2002},
				{"WGL_ACCELERATION",				WGL_ACCELERATION_ARB},
				{"WGL_NEED_PALETTE",				0x2004},
				{"WGL_NEED_SYSTEM_PALETTE",			0x2005},
				{"WGL_SWAP_LAYER_BUFFERS",			WGL_SWAP_LAYER_BUFFERS_ARB},
				{"WGL_SWAP_METHOD",					0x2007},
				{"WGL_NUMBER_OVERLAYS",				0x2008},
				{"WGL_NUMBER_UNDERLAYS",			0x2009},
				{"WGL_TRANSPARENT",					0x200A},
				{"WGL_TRANSPARENT_RED_VALUE",		0x2037},
				{"WGL_TRANSPARENT_GREEN_VALUE",		0x2038},
				{"WGL_TRANSPARENT_BLUE_VALUE",		0x2039},
				{"WGL_TRANSPARENT_ALPHA_VALUE",		0x203a},
				{"WGL_TRANSPARENT_INDEX_VALUE",		0x203B},
				{"WGL_SHARE_DEPTH",					0x200C},
				{"WGL_SHARE_STENCIL",				0x200D},
				{"WGL_SHARE_ACCUM",					0x200E},
				{"WGL_SUPPORT_GDI",					0x200F},
				{"WGL_SUPPORT_OPENGL",				WGL_SUPPORT_OPENGL_ARB},
				{"WGL_DOUBLE_BUFFER",				WGL_DOUBLE_BUFFER_ARB},
				{"WGL_STEREO",						WGL_STEREO_ARB},
				{"WGL_PIXEL_TYPE",					WGL_PIXEL_TYPE_ARB},
				{"WGL_COLOR_BITS",					WGL_COLOR_BITS_ARB},
				{"WGL_RED_BITS",					0x2015},
				{"WGL_RED_SHIFT",					0x2016},
				{"WGL_GREEN_BITS",					0x2017},
				{"WGL_GREEN_SHIFT",					0x2018},
				{"WGL_BLUE_BITS",					0x2019},
				{"WGL_BLUE_SHIFT",					0x201A},
				{"WGL_ALPHA_BITS",					WGL_ALPHA_BITS_ARB},
				{"WGL_ALPHA_SHIFT",					0x201C},
				{"WGL_ACCUM_BITS",					0x201D},
				{"WGL_ACCUM_RED_BITS",				0x201E},
				{"WGL_ACCUM_GREEN_BITS",			0x201F},
				{"WGL_ACCUM_BLUE_BITS",				0x2020},
				{"WGL_ACCUM_ALPHA_BITS",			0x2021},
				{"WGL_DEPTH_BITS",					WGL_DEPTH_BITS_ARB},
				{"WGL_STENCIL_BITS",				WGL_STENCIL_BITS_ARB},
				{"WGL_AUX_BUFFERS",					0x2024},
				
				//extra extensions
				{"WGL_SAMPLE_BUFFERS_ARB",			WGL_SAMPLE_BUFFERS_ARB},	//multisample
				{"WGL_SAMPLES_ARB",					WGL_SAMPLES_ARB},	//multisample

				{"WGL_DRAW_TO_PBUFFER_ARB",			0x202D},	//pbuffers
				{"WGL_MAX_PBUFFER_PIXELS_ARB",		0x202E},	//pbuffers
				{"WGL_MAX_PBUFFER_WIDTH_ARB",		0x202F},	//pbuffers
				{"WGL_MAX_PBUFFER_HEIGHT_ARB",		0x2030},	//pbuffers

				{"WGL_BIND_TO_TEXTURE_RGB_ARB",		0x2070},
				{"WGL_BIND_TO_TEXTURE_RGBA_ARB",	0x2071},
				{"WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB",WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB},
				{"WGL_COLORSPACE_EXT",				0x309D},	//WGL_EXT_colorspace

				//stuff my drivers don't support
//				{"WGL_DEPTH_FLOAT_EXT",				0x2040},
			};
			int iAttributeNames[countof(iAttributeTable)];
			float fAttributeValues[countof(iAttributeTable)];
			float basevalues[countof(iAttributeTable)];

			for (j = 0; j < countof(iAttributeTable); j++)
				iAttributeNames[j] = iAttributeTable[j].id;

			Sys_Printf("Pixel Format %i --------------------------\n", iAttributes<0?currentpixelformat:pixelformats[iAttributes]);
			if (qwglGetPixelFormatAttribfvARB(hDC, iAttributes<0?currentpixelformat:pixelformats[iAttributes], 0, countof(iAttributeTable), iAttributeNames, fAttributeValues))
			{
				if (iAttributes==-1)
					memcpy(basevalues, fAttributeValues, sizeof(basevalues));
				for (j = 0; j < countof(iAttributeTable); j++)
				{
					if (iAttributes==-1 || fAttributeValues[j] != basevalues[j])
					{
						if (iAttributeTable[j].id == 0x2007 && fAttributeValues[j] == 0x2028)
							Sys_Printf("%s: exchange\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == 0x2007 && fAttributeValues[j] == 0x2029)
							Sys_Printf("%s: copy\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == 0x309D && fAttributeValues[j] == 0x3089)
							Sys_Printf("%s: WGL_COLORSPACE_SRGB\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == 0x309D && fAttributeValues[j] == 0x308A)
							Sys_Printf("%s: WGL_COLORSPACE_LINEAR\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == WGL_PIXEL_TYPE_ARB && fAttributeValues[j] == WGL_TYPE_RGBA_FLOAT_ARB)
							Sys_Printf("%s: WGL_TYPE_RGBA_FLOAT_ARB\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == WGL_PIXEL_TYPE_ARB && fAttributeValues[j] == 0x20A8)
							Sys_Printf("%s: WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT\n", iAttributeTable[j].name);
						else if (iAttributeTable[j].id == 0x202E || iAttributeTable[j].id == WGL_PIXEL_TYPE_ARB)
							Sys_Printf("%s: %#x\n", iAttributeTable[j].name, (int)fAttributeValues[j]);
						else
							Sys_Printf("%s: %g\n", iAttributeTable[j].name, fAttributeValues[j]);
					}
				}
			}
		}
#endif

		ReleaseDC(mainwindow, hDC);
		if (valid && numFormats > 0 && pixelformats[0] != currentpixelformat)
		{
			shouldforcepixelformat = true;
			forcepixelformat = pixelformats[0];
			return true;
		}
	}
	return false;
}

static BYTE IntensityFromShifted(unsigned int index, unsigned int shift, unsigned int bits)
{
	unsigned int val;

	val = (index >> shift) & ((1 << bits) - 1);

	switch (bits)
	{
	case 1:
		val = val ? 0xFF : 0;
		break;
	case 2:
		val |= val << 2;
		val |= val << 4;
		break;
	case 3:
		val = val << (8 - bits);
		val |= val >> 3;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		val = val << (8 - bits);
		val |= val >> bits;
		break;
	case 8:
		break;
	default:
		return 0;
	}

	return val;
}

static void FixPaletteInDescriptor(HDC hDC, PIXELFORMATDESCRIPTOR *pfd)
{
	LOGPALETTE *ppal;
	HPALETTE hpal;
	int idx, clrs;

	if (pfd->dwFlags & PFD_NEED_PALETTE)
	{
		clrs = 1 << pfd->cColorBits;

		ppal = Z_Malloc(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * clrs);

		ppal->palVersion = 0x300;
		ppal->palNumEntries = clrs;

		for (idx = 0; idx < clrs; idx++)
		{
			ppal->palPalEntry[idx].peRed = IntensityFromShifted(idx, pfd->cRedShift, pfd->cRedBits);
			ppal->palPalEntry[idx].peGreen = IntensityFromShifted(idx, pfd->cGreenShift, pfd->cGreenBits);
			ppal->palPalEntry[idx].peBlue = IntensityFromShifted(idx, pfd->cBlueShift, pfd->cBlueBits);
			ppal->palPalEntry[idx].peFlags = 0;
		}

		hpal = CreatePalette(ppal);
		SelectPalette(hDC, hpal, FALSE);
		RealizePalette(hDC);
		Z_Free(ppal);
	}
}

static BOOL bSetupPixelFormat(HDC hDC, rendererstate_t *info)
{
	PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
	1,				// version number
	PFD_DRAW_TO_WINDOW 		// support window
	|  PFD_SUPPORT_OPENGL 	// support OpenGL
	|  PFD_DOUBLEBUFFER,		// double buffered
	PFD_TYPE_RGBA,			// RGBA type
	24,				// 24-bit color depth
	0, 0, 0, 0, 0, 0,		// color bits ignored
	0,				// no alpha buffer
	0,				// shift bit ignored
	0,				// no accumulation buffer
	0, 0, 0, 0, 			// accum bits ignored
#ifndef RTLIGHTS
	24,				// 24-bit z-buffer
	0,				// 0 stencil, don't need it unless we're using rtlights
#else
	24,				// 24-bit z-buffer
	8,				// stencil buffer
#endif
	0,				// no auxiliary buffer
	PFD_MAIN_PLANE,			// main layer
	0,				// reserved
	0, 0, 0				// layer masks ignored
	};

	TRACE(("dbg: bSetupPixelFormat: ChoosePixelFormat\n"));

	if (info->stereo)
		pfd.dwFlags |= PFD_STEREO;
	if (info->bpp == 15 || info->bpp == 16)
		pfd.cColorBits = 16;

	if (shouldforcepixelformat && qwglChoosePixelFormatARB && 
		qDescribePixelFormat(hDC, forcepixelformat, sizeof(pfd), &pfd) &&
		qSetPixelFormat(hDC, forcepixelformat, &pfd))	//the extra && is paranoia
	{
		shouldforcepixelformat = false;
		currentpixelformat = forcepixelformat;
	}
	else if ((currentpixelformat = qChoosePixelFormat(hDC, &pfd)) && qDescribePixelFormat(hDC, currentpixelformat, sizeof(pfd), &pfd) && qSetPixelFormat(hDC, currentpixelformat, &pfd))
		;	//we got our desired pixel format, or close enough
	else
	{
		pfd.cStencilBits = 0;
		if ((currentpixelformat = qChoosePixelFormat(hDC, &pfd)) && qDescribePixelFormat(hDC, currentpixelformat, sizeof(pfd), &pfd) && qSetPixelFormat(hDC, currentpixelformat, &pfd))
			;
		else
		{
			Con_Printf("Unable to find suitable pixel format: %i\n", (int)GetLastError());
			return FALSE;
		}
	}
	shouldforcepixelformat = false;

	if ((pfd.dwFlags & PFD_GENERIC_FORMAT) && !(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
	{
		Con_Printf(CON_WARNING "WARNING: software-rendered opengl context\nPlease install appropriate graphics drivers, or try d3d rendering instead\n");
	}
	else if (pfd.dwFlags & PFD_SWAP_COPY)
		Con_Printf(CON_WARNING "WARNING: buffer swaps will use copy operations\n");

	FixPaletteInDescriptor(hDC, &pfd);
	return TRUE;
}
#endif

/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
static void ClearAllStates (void)
{
	IN_KeyEvent(0, false, -1, 0);	//-1 means to clear all keys.
	Key_ClearStates ();
	INS_ClearStates ();
}

static qboolean GLAppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

//	Con_Printf("GLAppActivate: %i %i\n", fActive, minimize);

	if (vid.activeapp == fActive && Minimized == minimize)
		return false;	//so windows doesn't crash us over and over again.

	vid.activeapp = fActive;// && (foregroundwindow==mainwindow);
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!vid.activeapp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (vid.activeapp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	INS_UpdateGrabs(modestate != MS_WINDOWED, vid.activeapp);

	if (fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			if (vid_canalttab && vid_wassuspended)
			{
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);

								// Fix for alt-tab bug in NVidia drivers
				MoveWindow (mainwindow, 0, 0, gdevmode.dmPelsWidth, gdevmode.dmPelsHeight, false);
			}
		}
		else if (modestate == MS_FULLWINDOW)
		{
			ShowWindow (mainwindow, SW_SHOWMAXIMIZED);
			UpdateWindow (mainwindow);
		}

		gammapending = 0.5;				//delayed gamma force
		Cvar_ForceCallback(&v_gamma);	//so the delay isn't so blatent when you have decent graphics drivers that don't break things.
	}

	if (!fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			if (vid_canalttab)
			{
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}

		Cvar_ForceCallback(&v_gamma);	//wham bam thanks.
	}

	return true;
}

#ifndef TWF_WANTPALM
typedef struct _TOUCHINPUT {
  LONG      x;
  LONG      y;
  HANDLE    hSource;
  DWORD     dwID;
  DWORD     dwFlags;
  DWORD     dwMask;
  DWORD     dwTime;
  ULONG_PTR dwExtraInfo;
  DWORD     cxContact;
  DWORD     cyContact;
} TOUCHINPUT, *PTOUCHINPUT;
DECLARE_HANDLE(HTOUCHINPUT);

#define WM_TOUCH					0x0240 
#define TOUCHINPUTMASKF_CONTACTAREA	0x0004
#define TOUCHEVENTF_DOWN			0x0002
#define TOUCHEVENTF_UP				0x0004
#define TWF_WANTPALM				0x00000002
#endif

static BOOL (WINAPI *pRegisterTouchWindow)(HWND hWnd, ULONG ulFlags);
static BOOL (WINAPI *pGetTouchInputInfo)(HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize);
static BOOL (WINAPI *pCloseTouchInputHandle)(HTOUCHINPUT hTouchInput);
static void Win_Touch_Init(HWND wnd)
{
	HMODULE lib;
	lib = LoadLibraryA("user32.dll");
	pRegisterTouchWindow = (void*)GetProcAddress(lib, "RegisterTouchWindow");
	pGetTouchInputInfo = (void*)GetProcAddress(lib, "GetTouchInputInfo");
	pCloseTouchInputHandle = (void*)GetProcAddress(lib, "CloseTouchInputHandle");

	if (pRegisterTouchWindow && pGetTouchInputInfo && pCloseTouchInputHandle)
		pRegisterTouchWindow(wnd, TWF_WANTPALM);
}
static void Win_Touch_Event(int points, HTOUCHINPUT ti)
{
	float sz;
	int i;
	TOUCHINPUT *inputs = malloc(points * sizeof(*inputs)), *input;
	if (inputs)
	{
		if (pGetTouchInputInfo(ti, points, inputs, sizeof(*inputs)))
		{
			for (i = 0, input = inputs; i < points; i++, input++)
			{
				int id = input->dwID+1;	//googling implies the id is generally a low 0-based index. I can't test this. the +1 ensures that mouselook is not broken by someone trying to use a touchscreen at the same time.
				if (input->dwMask & TOUCHINPUTMASKF_CONTACTAREA)
					sz = sqrt((input->cxContact*input->cxContact + input->cyContact*input->cyContact) / 10000.0);
				else
					sz = 0;

				//the web seems to imply that the ids should be low values, <16 or so. hurrah.

				//movement *then* buttons. this should ensure that the cursor is positioned correctly.
				IN_MouseMove(id, true, input->x/100.0f, input->y/100.0f, 0, sz);

				if (input->dwFlags & TOUCHEVENTF_DOWN)
					IN_KeyEvent(id, true, K_MOUSE1, 0);
				if (input->dwFlags & TOUCHEVENTF_UP)
					IN_KeyEvent(id, false, K_MOUSE1, 0);
			}
		}
		free(inputs);
	}

	pCloseTouchInputHandle(ti);
}

#ifdef WTHREAD
//runs on the main/render thread, forwarded from the worker thread.
//these events are the ones that would cause race conditions, but need to be able to cope with a little bit of a delay (and shouldn't need to trigger other window messages, as that would cause other races).
void MainThreadWndProc(void *ctx, void *data, size_t msg, size_t ex)
{
	switch(msg)
	{
	case WM_COPYDATA:
		Host_RunFile(data, ex, NULL);
		Z_Free(data);
		break;
	case WM_CLOSE:
		M_Window_ClosePrompt();
		break;
	case WM_SIZE:
	case WM_MOVE:
		Cvar_ForceCallback(&vid_conautoscale);
		break;
	case WM_KILLFOCUS:
		GLAppActivate(FALSE, Minimized);
		ClearAllStates ();
		break;
	case WM_SETFOCUS:
		if (!GLAppActivate(TRUE, Minimized))
			break;
		ClearAllStates ();
		break;

#ifdef HAVE_CDPLAYER
	case MM_MCINOTIFY:
		CDAudio_MessageHandler (mainwindow, msg, (WPARAM)ctx, (LPARAM)data);
		break;
#endif
	}
}
#endif

/* main window procedure
due to moving the main window over to a different thread, we gain access to input timestamps (as well as video refreshes when dragging etc)
however, we have to tread carefully. the main/render thread will be running the whole time, and may trigger messages that we need to respond to _now_.
this means that the main and window thread cannot be allowed to contest any mutexes where anything but memory is touched before its unlocked.
(or in other words, we can't have the main thread near-perma-lock any mutexes that can be locked-to-sync here)
*/
static LONG WINAPI GLMainWndProc (
	HWND	hWnd,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	LONG	lRet = 1;
//	int		fActive, fMinimized;
	int 	temp;
	extern unsigned int uiWheelMessage;

	if ( uMsg == uiWheelMessage )
		uMsg = WM_MOUSEWHEEL;

	switch (uMsg)
	{
		case WM_COPYDATA:
			{
				COPYDATASTRUCT *cds = (COPYDATASTRUCT*)lParam;
#ifdef WTHREAD
				COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, memcpy(Z_Malloc(cds->cbData), cds->lpData, cds->cbData), uMsg, cds->cbData);
#else
				Host_RunFile(cds->lpData, cds->cbData, NULL);
#endif
				lRet = 1;
			}
			break;
		case WM_KILLFOCUS:
#ifdef WTHREAD
			COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
#else
			GLAppActivate(FALSE, Minimized);//FIXME: thread
			ClearAllStates ();	//FIXME: thread
#endif
			if (modestate != MS_WINDOWED)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			break;
		case WM_SETFOCUS:
#ifdef WTHREAD
			COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
#else
			if (!GLAppActivate(TRUE, Minimized))//FIXME: thread
				break;
			ClearAllStates ();	//FIXME: thread
#endif
			break;

		case WM_TOUCH:
			Win_Touch_Event(LOWORD(wParam), (HTOUCHINPUT)lParam);
			return 0;	//return 0 if we handled it.

		case WM_CREATE:
			break;

		case WM_MOVE:
			VID_UpdateWindowStatus (hWnd);
#ifdef WTHREAD
			COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
#else
			Cvar_ForceCallback(&vid_conautoscale);
#endif
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (!vid_initializing)
				INS_TranslateKeyEvent(wParam, lParam, true, 0, false);
			break;

//		case WM_UNICHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		case WM_CHAR:
		case WM_SYSCHAR:
//			if (!vid_initializing)
//				INS_TranslateKeyEvent(wParam, lParam, true);
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (!vid_initializing)
				INS_TranslateKeyEvent(wParam, lParam, false, 0, false);
			break;

		case WM_APPCOMMAND:
			if (!INS_AppCommand(lParam))
				lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);	//otherwise it won't get handled by background apps, like media players.
			break;

		case WM_MOUSEACTIVATE:
			lRet = MA_ACTIVATEANDEAT;
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			temp = 0;

			if (wParam & MK_LBUTTON)
			{
				temp |= 1;
				if (sys_parentwindow && modestate == MS_WINDOWED)
					SetFocus(hWnd);
			}

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			if (wParam & MK_XBUTTON1)
				temp |= 8;

			if (wParam & MK_XBUTTON2)
				temp |= 16;

			if (wParam & MK_XBUTTON3)
				temp |= 32;

			if (wParam & MK_XBUTTON4)
				temp |= 64;

			if (wParam & MK_XBUTTON5)
				temp |= 128;

			if (wParam & MK_XBUTTON6)
				temp |= 256;

			if (wParam & MK_XBUTTON7)
				temp |= 512;

			if (!vid_initializing)
				INS_MouseEvent (temp);	//FIXME: thread (halflife)

			break;

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL:
			if (!vid_initializing)
			{
				if ((short) HIWORD(wParam&0xffffffff) > 0)
				{
					IN_KeyEvent(0, true, K_MWHEELUP, 0);
					IN_KeyEvent(0, false, K_MWHEELUP, 0);
				}
				else
				{
					IN_KeyEvent(0, true, K_MWHEELDOWN, 0);
					IN_KeyEvent(0, false, K_MWHEELDOWN, 0);
				}
			}
			break;

		case WM_INPUT:
			// raw input handling
			if (!vid_initializing)
			{
				INS_RawInput_Read((HANDLE)lParam);
				lRet = 0;
			}
			break;
		case WM_DEVICECHANGE:
			COM_AddWork(WG_MAIN, INS_DeviceChanged, NULL, NULL, uMsg, 0);
			lRet = TRUE;
			break;

#ifdef VKQUAKE
		case WM_USER_VKPRESENT:
			VK_DoPresent((struct vkframe*)lParam);
			break;
#endif
#if defined(VKQUAKE) && defined(USE_WGL)
		case WM_USER_NVVKPRESENT:
			Win32NVVK_DoPresent((struct vkframe*)lParam);
			break;
#endif
		case WM_USER_VIDSHUTDOWN:
			PostQuitMessage(0);
			break;
		case WM_USER_SPEECHTOTEXT:
#ifdef HAVE_SPEECHTOTEXT
			STT_Event();
#endif
			break;

		case WM_GETMINMAXINFO:
			{
				RECT windowrect;
				RECT clientrect;
				MINMAXINFO *mmi = (MINMAXINFO *) lParam;

				GetWindowRect (hWnd, &windowrect);
				GetClientRect (hWnd, &clientrect);

				mmi->ptMinTrackSize.x = 320 + ((windowrect.right - windowrect.left) - (clientrect.right - clientrect.left));
				mmi->ptMinTrackSize.y = 200 + ((windowrect.bottom - windowrect.top) - (clientrect.bottom - clientrect.top));
			}
			return 0;
		case WM_SIZE:
			vid.isminimized  = (wParam==SIZE_MINIMIZED);
			if (!vid_initializing)
			{
				VID_UpdateWindowStatus (hWnd);
#ifdef WTHREAD
				COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
#else
				Cvar_ForceCallback(&vid_conautoscale);
#endif
			}
			break;

		case WM_CLOSE:
			if (!vid_initializing)
			{
#if 1
	#ifdef WTHREAD
				COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
	#else
				M_Window_ClosePrompt();
	#endif
				SetFocus(hWnd);
#else
				if (wantquit)
				{
					//urr, this would be the second time that they've told us to quit.
					//assume the main thread has deadlocked
					if (MessageBoxW (hWnd, L"Terminate process?", L"Confirm Exit",
							MB_YESNO | MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_DEFBUTTON2) == IDYES)
					{
						//abrupt process termination is never nice, but sometimes drivers suck.
						//or qc code runs away, or ...
						exit(1);
					}
				}

				else if (MessageBoxW (hWnd, L"Are you sure you want to quit?", L"Confirm Exit",
							MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES)
				{
#ifdef WTHREAD
					COM_AddWork(WG_MAIN, MainThreadWndProc, NULL, NULL, uMsg, 0);
#else
					Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
#endif
					wantquit = true;
				}
#endif
			}
			break;

		case WM_ERASEBKGND:
			lRet = TRUE;
			break;
/*
		case WM_ACTIVATE:
//			fActive = LOWORD(wParam);
//			fMinimized = (BOOL) HIWORD(wParam);
//			if (!GLAppActivate(!(fActive == WA_INACTIVE), fMinimized))
				break;//so, urm, tell me microsoft, what changed?
			if (modestate == MS_FULLDIB)
				ShowWindow(hWnd, SW_SHOWNORMAL);

#ifdef WTHREAD
#else
		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			ClearAllStates ();	//FIXME: thread

			Cvar_ForceCallback(&vid_conautoscale);	//FIXME: thread
#endif
			break;
*/
		case WM_DESTROY:
			break;
		case WM_SETCURSOR:
			//only use a custom cursor if the cursor is inside the client area
			switch(lParam&0xffff)
			{
			case 0:
				break;
			case HTCLIENT:
				if (hCustomCursor)	//custom cursor enabled
					SetCursor(hCustomCursor);
				else				//fallback on an arrow cursor, just so we have something visible at startup or so
					SetCursor(hArrowCursor);
				lRet = TRUE;
				break;
			default:
				lRet = DefWindowProcW (hWnd, uMsg, wParam, lParam);
				break;
			}
			break;

		case WM_DROPFILES:
			{
				HDROP p = (HDROP)wParam;
				wchar_t fnamew[MAX_PATH];
				char fname[MAX_PATH];
				vfsfile_t *f;
				int i, count = DragQueryFile(p, ~0, NULL, 0);
				for(i = 0; i < count; i++)
				{
					if (WinNT)
					{
						DragQueryFileW(p, i, fnamew, countof(fnamew));
						narrowen(fname, sizeof(fname), fnamew);
					}
					else
						DragQueryFileA(p, i, fname, countof(fname));
					f = FS_OpenVFS(fname, "rb", FS_SYSTEM);
					if (f)
						Host_RunFile(fname, strlen(fname), f);
				}
				DragFinish(p);
			}
			return 0;	//An application should return zero if it processes this message.

		case WM_SYSCOMMAND:	//this only works when we're the active app. which is fine.
			if (GET_SC_WPARAM(wParam)==SC_SCREENSAVE)
				lRet = 0;	//try to block sscreensavers, if enabled... will likely fail due to passwords and security stuff etc...
			else if (GET_SC_WPARAM(wParam)==SC_MONITORPOWER && (lParam == 1 || lParam == 2))
				lRet = 0;	//try to block monitor power saving.
			else if (WinNT)
				lRet = DefWindowProcW (hWnd, uMsg, wParam, lParam);
			else
				lRet = DefWindowProcA (hWnd, uMsg, wParam, lParam);
			break;
#ifdef HAVE_CDPLAYER
		case MM_MCINOTIFY:
#ifdef WTHREAD
			COM_AddWork(WG_MAIN, MainThreadWndProc, (void*)wParam, (void*)lParam, uMsg, 0);
			lRet = 0;
#else
			lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);	//FIXME: thread
#endif
			break;
#endif

		default:
			/* pass all unhandled messages to DefWindowProc */
			if (WinNT)
				lRet = DefWindowProcW (hWnd, uMsg, wParam, lParam);
			else
				lRet = DefWindowProcA (hWnd, uMsg, wParam, lParam);
			break;
	}

	/* return 1 if handled message, 0 if not */
	return lRet;
}

/*
void VID_Init8bitPalette(void)
{
#ifdef GL_USE8BITTEX
#ifdef GL_EXT_paletted_texture
#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB

	// Check for 8bit Extensions and initialize them.
	int i;
	char thePalette[256*3];
	char *oldPalette, *newPalette;

	qglColorTableEXT = (void *)qwglGetProcAddress("glColorTableEXT");
	if (!qglColorTableEXT || !GL_CheckExtension("GL_EXT_shared_texture_palette") || COM_CheckParm("-no8bit"))
		return;

	Con_SafePrintf("8-bit GL extensions enabled.\n");
	qglEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
	oldPalette = (char *) d_8to24rgbtable; //d_8to24table3dfx;
	newPalette = thePalette;
	for (i=0;i<256;i++)
	{
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		oldPalette++;
	}
	qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE,
		(void *) thePalette);
	is8bit = TRUE;

#endif
#endif
}
*/

void GLVID_DeInit (void)
{
	GLVID_Shutdown();
	vid.activeapp = false;

	Cvar_Unhook(&vid_vsync);
	Cvar_Unhook(&vid_wndalpha);
	Cmd_RemoveCommand("vid_recenter");
}

/*
===================
VID_Init
===================
*/
qboolean Win32VID_Init (rendererstate_t *info, unsigned char *palette, int mode)
{
	extern int isPlugin;
//	qbyte	*ptmp;
	DEVMODE	devmode;

	platform_rendermode = mode;

	memset(&devmode, 0, sizeof(devmode));

	hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON1));
	hArrowCursor = LoadCursor (NULL,IDC_ARROW);

	rf->VID_CreateCursor = WIN_CreateCursor;
	rf->VID_DestroyCursor = WIN_DestroyCursor;
	rf->VID_SetCursor = WIN_SetCursor;

	vid_initialized = false;
	vid_initializing = true;

	if (!GLVID_SetMode (info, palette))
	{
		VID_UnSetMode();
		return false;
	}

	// Check for 3DFX Extensions and initialize them.
	//VID_Init8bitPalette();

	vid_canalttab = true;

	Cvar_Hook(&vid_vsync, VID_Wait_Override_Callback);
	Cvar_Hook(&vid_wndalpha, VID_WndAlpha_Override_Callback);

	Cmd_AddCommand("vid_recenter", GLVID_Recenter_f);

	if (isPlugin >= 2)
	{
		fprintf(stdout, "refocuswindow %"PRIxPTR"\n", (quintptr_t)mainwindow);
		fflush(stdout);
	}

	vid_initialized = true;
	vid_initializing = false;

	WIN_WindowCreated(mainwindow);

	return true;
}

#ifdef USE_WGL
qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return Win32VID_Init(info, palette, MODE_WGL);
}
#endif	//USE_WGL





#ifdef USE_EGL

static qboolean EGLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	if (!EGL_LoadLibrary(info->subrenderer))
		return false;
	return Win32VID_Init(info, palette, MODE_EGL);
}



#include "shader.h"
#include "gl_draw.h"
rendererinfo_t eglrendererinfo =
{
	"EGL(win32)",
	{
		"egl"
	},
	QR_OPENGL,

	GLDraw_Init,
	GLDraw_DeInit,

	GL_UpdateFiltering,
	GL_LoadTextureMips,
	GL_DestroyTexture,

	GLR_Init,
	GLR_DeInit,
	GLR_RenderView,

	EGLVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,

	NULL,
	NULL,
	NULL,
	GLVID_SetCaption,       //setcaption
	GLVID_GetRGBInfo,


	GLSCR_UpdateScreen,

	GLBE_SelectMode,
	GLBE_DrawMesh_List,
	GLBE_DrawMesh_Single,
	GLBE_SubmitBatch,
	GLBE_GetTempBatch,
	GLBE_DrawWorld,
	GLBE_Init,
	GLBE_GenBrushModelVBO,
	GLBE_ClearVBO,
	GLBE_UpdateLightmaps,
	GLBE_SelectEntity,
	GLBE_SelectDLight,
	GLBE_Scissor,
	GLBE_LightCullModel,

	GLBE_VBO_Begin,
	GLBE_VBO_Data,
	GLBE_VBO_Finish,
	GLBE_VBO_Destroy,

	GLBE_RenderToTextureUpdate2d,

	""
};
#endif	//USE_EGL


#ifdef VKQUAKE
static qboolean VKVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return Win32VID_Init(info, palette, MODE_VULKAN);
}

rendererinfo_t vkrendererinfo =
{
	"Vulkan",
	{
		"vk",
		"Vulkan"
	},
	QR_VULKAN,

	VK_Draw_Init,
	VK_Draw_Shutdown,

	VK_UpdateFiltering,
	VK_LoadTextureMips,
	VK_DestroyTexture,

	VK_R_Init,
	VK_R_DeInit,
	VK_R_RenderView,

	VKVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,
	WIN_CreateCursor,
	WIN_SetCursor,
	WIN_DestroyCursor,
	GLVID_SetCaption,
	VKVID_GetRGBInfo,

	VK_SCR_UpdateScreen,

	VKBE_SelectMode,
	VKBE_DrawMesh_List,
	VKBE_DrawMesh_Single,
	VKBE_SubmitBatch,
	VKBE_GetTempBatch,
	VKBE_DrawWorld,
	VKBE_Init,
	VKBE_GenBrushModelVBO,
	VKBE_ClearVBO,
	VKBE_UploadAllLightmaps,
	VKBE_SelectEntity,
	VKBE_SelectDLight,
	VKBE_Scissor,
	VKBE_LightCullModel,

	VKBE_VBO_Begin,
	VKBE_VBO_Data,
	VKBE_VBO_Finish,
	VKBE_VBO_Destroy,

	VKBE_RenderToTextureUpdate2d,

	"no more",
	NULL,	//getpriority
	NULL,	//enummodes
	Win32VK_EnumerateDevices,	//enumdevices
};



#ifdef USE_WGL
static qboolean NVVKVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return Win32VID_Init(info, palette, MODE_NVVULKAN);
}
rendererinfo_t nvvkrendererinfo =
{
	"Vulkan via OpenGL (GL_NV_draw_vulkan_image)",
	{
		"nvvk",
		"GL_NV_draw_vulkan_image",
	},
	QR_VULKAN,

	VK_Draw_Init,
	VK_Draw_Shutdown,

	VK_UpdateFiltering,
	VK_LoadTextureMips,
	VK_DestroyTexture,

	VK_R_Init,
	VK_R_DeInit,
	VK_R_RenderView,

	NVVKVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,
	WIN_CreateCursor,
	WIN_SetCursor,
	WIN_DestroyCursor,
	GLVID_SetCaption,
	VKVID_GetRGBInfo,

	VK_SCR_UpdateScreen,

	VKBE_SelectMode,
	VKBE_DrawMesh_List,
	VKBE_DrawMesh_Single,
	VKBE_SubmitBatch,
	VKBE_GetTempBatch,
	VKBE_DrawWorld,
	VKBE_Init,
	VKBE_GenBrushModelVBO,
	VKBE_ClearVBO,
	VKBE_UploadAllLightmaps,
	VKBE_SelectEntity,
	VKBE_SelectDLight,
	VKBE_Scissor,
	VKBE_LightCullModel,

	VKBE_VBO_Begin,
	VKBE_VBO_Data,
	VKBE_VBO_Finish,
	VKBE_VBO_Destroy,

	VKBE_RenderToTextureUpdate2d,

	"no more",
	NULL,
	NULL,
	NULL,	//reminder that this is the OPENGL device to init.
};
#endif
#endif

#endif
