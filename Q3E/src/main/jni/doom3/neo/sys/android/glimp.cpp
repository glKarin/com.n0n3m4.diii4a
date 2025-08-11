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

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT EGL_OPENGL_ES3_BIT_KHR
#endif
#define DEFAULT_SYS_VIDEO_MEMORY 512 // 64
idCVar sys_videoRam("sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER,
					"Texture memory on the video card (in megabytes) - 0: autodetect", 0, 1024);

#ifdef _OPENGLES3
#define HARM_EGL_OPENGL_ES_BIT (gl_version != GL_VERSION_GL_ES2 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT)
#define HARM_EGL_CONTEXT_CLIENT_VERSION (gl_version != GL_VERSION_GL_ES2 ? 3 : 2)
#else
#define HARM_EGL_OPENGL_ES_BIT EGL_OPENGL_ES2_BIT
#define HARM_EGL_CONTEXT_CLIENT_VERSION 2
#endif
#define HARM_EGL_CONTEXT_OPENGL_DEBUG harm_r_debugOpenGL.GetBool() ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

// OpenGL attributes
// OpenGL color format
int gl_format = GLFORMAT_RGBA8888;
// OpenGL depth bits
int gl_depth_bits = 24;
// OpenGL multisamples
int gl_msaa = 0;



#define MAX_NUM_CONFIGS 1000
static volatile ANativeWindow *win;
//volatile bool has_gl_context = false;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLConfig configs[1];
static EGLConfig eglConfig = 0;
static EGLint format = WINDOW_FORMAT_RGBA_8888; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

#ifdef _IMGUI
#include "imgui.cpp"
#endif

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
	EGLint err = eglGetError();
	if(err == EGL_SUCCESS)
		return;

	if(exit)
		common->Error("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	else
		common->Printf("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
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

static EGLConfigInfo_t GLimp_FormatInfo(int gl_format, int gl_depth_bits)
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
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA4444:
			red_bits = 4;
			green_bits = 4;
			blue_bits = 4;
			alpha_bits = 4;
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA5551:
			red_bits = 5;
			green_bits = 5;
			blue_bits = 5;
			alpha_bits = 1;
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA1010102:
			red_bits = 10;
			green_bits = 10;
			blue_bits = 10;
			alpha_bits = 2;
			//depth_bits = 24;
			buffer_bits = 32;
			break;
		case GLFORMAT_RGBA8888:
		default:
			red_bits = 8;
			green_bits = 8;
			blue_bits = 8;
			alpha_bits = 8;
			//depth_bits = 24;
			buffer_bits = 32;
			break;
	}

	switch(gl_depth_bits)
	{
		case 16:
			depth_bits = 16;
			break;
		case 32:
			depth_bits = 32;
			break;
		case 24:
		default:
			depth_bits = 24;
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
	EGLConfigInfo_t info = GLimp_FormatInfo(gl_format, gl_depth_bits);
	int r = info.red;
	int g = info.green;
	int b = info.blue;
	int a = info.alpha;
	int d = info.depth;
	int s = info.samples;

	int lr, lg, lb, la, ld, ls;
	int rr, rg, rb, ra, rd, rs;
	int rat1, rat2;
	eglGetConfigAttrib(eglDisplay, lhs, EGL_RED_SIZE, &lr);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_GREEN_SIZE, &lg);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_BLUE_SIZE, &lb);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_ALPHA_SIZE, &la);

	eglGetConfigAttrib(eglDisplay, rhs, EGL_RED_SIZE, &rr);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_GREEN_SIZE, &rg);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_BLUE_SIZE, &rb);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_ALPHA_SIZE, &ra);

	rat1 = (abs(lr - r) + abs(lg - g) + abs(lb - b));//*1000000-(ld*10000+la*100+ls);
	rat2 = (abs(rr - r) + abs(rg - g) + abs(rb - b));//*1000000-(rd*10000+ra*100+rs);

	if(rat1 == rat2)
	{
        eglGetConfigAttrib(eglDisplay, lhs, EGL_DEPTH_SIZE, &ld);
        //eglGetConfigAttrib(eglDisplay, lhs, EGL_STENCIL_SIZE, &ls);

        eglGetConfigAttrib(eglDisplay, rhs, EGL_DEPTH_SIZE, &rd);
        //eglGetConfigAttrib(eglDisplay, rhs, EGL_STENCIL_SIZE, &rs);

        rat1 = abs(ld - d);
        rat2 = abs(rd - d);
	}

    if(rat1 == rat2)
    {
        eglGetConfigAttrib(eglDisplay, lhs, EGL_SAMPLES, &ls);

        eglGetConfigAttrib(eglDisplay, rhs, EGL_SAMPLES, &rs);

        rat1 = /*abs*/(ls - s);
        rat2 = /*abs*/(rs - s);
    }

	return rat1 - rat2;
}

static EGLConfig GLimp_ChooseConfig(EGLConfig configs[], int size)
{
	qsort(configs, size, sizeof(EGLConfig), GLimp_EGLConfigCompare);
	return configs[0];
}

static EGLConfigInfo_t GLimp_GetConfigInfo(const EGLConfig eglConfig)
{
	EGLConfigInfo_t info;

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RED_SIZE, &info.red);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_GREEN_SIZE, &info.green);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BLUE_SIZE, &info.blue);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_ALPHA_SIZE, &info.alpha);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_DEPTH_SIZE, &info.depth);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_STENCIL_SIZE, &info.stencil);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BUFFER_SIZE, &info.buffer);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &info.samples);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLE_BUFFERS, &info.sample_buffers);

	return info;
}

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	if(!w)
		return;
	win = w;
	ANativeWindow_acquire((ANativeWindow *)win);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(!w)
		return;

	if(eglDisplay == EGL_NO_DISPLAY)
		return;

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	win = w;
	ANativeWindow_acquire((ANativeWindow *)win);
	ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent");
		return;
	}
	//has_gl_context = true;
	Sys_Printf("[Harmattan]: EGL surface created and using EGL context.\n");
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
	Sys_Printf("[Harmattan]: EGL surface destroyed and no EGL context.\n");
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
		common->FatalError("Unable to initialize OpenGL (%s)", #name); \
	else common->Printf("GetProc (%s) -> %p\n", #name, q##name);

#include "../../renderer/qgl_proc.h"
}

static bool GLimp_dlopen()
{
#ifdef _OPENGLES3
	const char *driverName = /*r_glDriver.GetString()[0] ? r_glDriver.GetString() : */(
			gl_version != 0x00020000 ? "libGLESv3.so" : "libGLESv2.so"
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

static void GLimp_dlclose()
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

	Sys_Printf("[Harmattan]: EGL destroyed.\n");
}

static void GLimp_UpdateSwapInterval( void )
{
	EGLint swapIntervalRange[2] = { 0, 0 };
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_MIN_SWAP_INTERVAL, &swapIntervalRange[0]);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_MAX_SWAP_INTERVAL, &swapIntervalRange[1]);
	common->Printf("Swap interval range: %d - %d\n", swapIntervalRange[0], swapIntervalRange[1]);
	int swapInterval = r_swapInterval.GetInteger();
	if(swapInterval < swapIntervalRange[0])
		swapInterval = swapIntervalRange[0];
	else if(swapInterval > swapIntervalRange[1])
		swapInterval = swapIntervalRange[1];
	if(swapInterval >= 0)
	{
		if(eglSwapInterval(eglDisplay, swapInterval))
			common->Printf("Setup swap interval: %d\n", swapInterval);
		else
			common->Warning("Setup swap interval to %d fail", swapInterval);
		// GLimp_HandleError("eglSwapInterval", false);
	}
	if(swapInterval != r_swapInterval.GetInteger())
	{
		r_swapInterval.SetInteger(swapInterval);
		r_swapInterval.ClearModified();
	}

	eglSwapBuffers(eglDisplay, eglSurface);
}

void GLimp_SwapBuffers()
{
	if(r_swapInterval.IsModified())
	{
		r_swapInterval.ClearModified();
		GLimp_UpdateSwapInterval();
	}
	eglSwapBuffers(eglDisplay, eglSurface);
}

bool GLimp_OpenDisplay(void)
{
	if(eglDisplay) {
		return true;
	}

	if (cvarSystem->GetCVarInteger("net_serverDedicated") == 1) {
		common->DPrintf("not opening the display: dedicated server\n");
		return false;
	}

	common->Printf( "Setup EGL display connection\n" );

	if ( !( eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY) ) ) {
		common->Printf( "Couldn't open the EGL display\n" );
		return false;
	}
	return true;
}

/*
===============
GLES_Init
===============
*/
static bool GLES_Init_special(const glimpParms_t &ap)
{
	EGLint config_count = 0;
	EGLConfigInfo_t info = GLimp_FormatInfo(gl_format, gl_depth_bits);
	int stencil_bits = info.stencil;
	int depth_bits = info.depth;
	int red_bits = info.red;
	int green_bits = info.green;
	int blue_bits = info.blue;
	int alpha_bits = info.alpha;
	int buffer_bits = info.buffer;
	int multisamples = gl_msaa < 0 ? ap.multiSamples : gl_msaa;

	EGLint attrib[] = {
			EGL_BUFFER_SIZE, buffer_bits,
			EGL_ALPHA_SIZE, alpha_bits,
			EGL_RED_SIZE, red_bits,
			EGL_BLUE_SIZE, green_bits,
			EGL_GREEN_SIZE, blue_bits,
			EGL_DEPTH_SIZE, depth_bits,
			EGL_STENCIL_SIZE, stencil_bits,
			EGL_SAMPLE_BUFFERS, multisamples > 1 ? 1 : 0,
			EGL_SAMPLES, multisamples,
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

	EGLConfig eglConfigs[MAX_NUM_CONFIGS];
	while(1)
	{
		if (!eglChooseConfig (eglDisplay, attrib, eglConfigs, MAX_NUM_CONFIGS, &config_count))
		{
			GLimp_HandleError("eglChooseConfig", false);

			if(multisamples > 1) {
				multisamples = (multisamples <= 2) ? 0 : (multisamples/2);

				attrib[7 * 2 + 1] = multisamples > 1 ? 1 : 0;
				attrib[8 * 2 + 1] = multisamples;
				continue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			common->Printf( "[Harmattan]: Get EGL context num -> %d.\n", config_count);
			for(int i = 0; i < config_count; i++)
			{
				EGLConfigInfo_t cinfo = GLimp_GetConfigInfo(eglConfigs[i]);
				common->Printf("\t%d EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
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

    EGLConfigInfo_t cinfo = GLimp_GetConfigInfo(configs[0]);
    common->Printf("Choose EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
                   cinfo.red, cinfo.green,
                   cinfo.blue, cinfo.alpha,
                   cinfo.depth,
                   cinfo.stencil
            , cinfo.samples, cinfo.sample_buffers
    );

	return true;
}

static bool GLES_Init_prefer(const glimpParms_t &ap)
{
	EGLint config_count = 0;
	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;
	bool suc;

	for (int i = 0; i < 16; i++) {

		int multisamples = gl_msaa < 0 ? ap.multiSamples : gl_msaa;
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

					attrib[7 * 2 + 1] = multisamples > 1 ? 1 : 0;
					attrib[8 * 2 + 1] = multisamples;
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

int GLES_Init(glimpParms_t &ap)
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
	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		GLimp_HandleError("eglInitialize");
		return false;
	}

	common->Printf("Initializing OpenGL display\n");

	if(!GLES_Init_special(ap))
	{
		if(!GLES_Init_prefer(ap))
		{
			common->Error("Initializing EGL error");
			return false;
		}
	}

	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;
	eglConfig = configs[0];

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &glConfig.multiSamples);
	// ap.multiSamples = glConfig.multiSamples;
	// r_multiSamples.SetInteger(glConfig.multiSamples);

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return false;
	}

#ifdef _OPENGLES3
	int gles3_version = 0;
	if(USING_GLES3)
	{
		// first try create OpenGLES3.2 context
		EGLint ctxAttrib32[] = {
				// EGL_CONTEXT_CLIENT_VERSION, HARM_EGL_CONTEXT_CLIENT_VERSION,
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 2,
                HARM_EGL_CONTEXT_OPENGL_DEBUG, harm_r_debugOpenGL.GetBool() ? 1 : 0,
				EGL_NONE
		};
		gles3_version = 2;
		eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib32);

		if (eglContext == EGL_NO_CONTEXT)
		{
			// second try create OpenGLES3.1 context
			EGLint ctxAttrib31[] = {
					// EGL_CONTEXT_CLIENT_VERSION, HARM_EGL_CONTEXT_CLIENT_VERSION,
					EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
					EGL_CONTEXT_MINOR_VERSION_KHR, 1,
                    HARM_EGL_CONTEXT_OPENGL_DEBUG, harm_r_debugOpenGL.GetBool() ? 1 : 0,
					EGL_NONE
			};
			gles3_version = 1;
			eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib31);

			if (eglContext == EGL_NO_CONTEXT) // finally try create OpenGLES3.0 context
			{
				EGLint ctxAttrib[] = {
						EGL_CONTEXT_CLIENT_VERSION, HARM_EGL_CONTEXT_CLIENT_VERSION,
                        HARM_EGL_CONTEXT_OPENGL_DEBUG, harm_r_debugOpenGL.GetBool() ? 1 : 0,
						EGL_NONE
				};
				gles3_version = 0;
				eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib);
			}
		}
	}
	else
#endif
	{
		EGLint ctxAttrib[] = {
				EGL_CONTEXT_CLIENT_VERSION, HARM_EGL_CONTEXT_CLIENT_VERSION,
                HARM_EGL_CONTEXT_OPENGL_DEBUG, harm_r_debugOpenGL.GetBool() ? 1 : 0,
				EGL_NONE
		};
		eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib);
	}
	if (eglContext == EGL_NO_CONTEXT)
	{
		GLimp_HandleError("eglCreateContext");
		return false;
	}
#ifdef _OPENGLES3
	if(USING_GLES3)
	{
		GLES3_VERSION = gles3_version;
		common->Printf("[Harmattan]: Create OpenGL ES3.%d context.\n", GLES3_VERSION);
	}
	else
#endif
	common->Printf("[Harmattan]: Create OpenGL ES2.0 context.\n");

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent");
		return false;
	}

	GLimp_UpdateSwapInterval();

	EGLConfigInfo_t info = GLimp_GetConfigInfo(eglConfig);
	int tcolorbits = info.red + info.green + info.blue;
	depth_bits = info.depth;
	stencil_bits = info.stencil;

	common->Printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
					info.red, info.green,
					info.blue, info.alpha,
					info.depth,
					info.stencil
			, info.samples, info.sample_buffers
	);

	glConfig.colorBits = tcolorbits;
	glConfig.depthBits = depth_bits;
	glConfig.stencilBits = stencil_bits;

	glConfig.isFullscreen = true;

	if (glConfig.isFullscreen) {
		Sys_GrabMouseCursor(true);
	}

	return true;
}

bool GLimp_ExtensionSupported(const char *name)
{
	if(!qglGetString)
	{
		common->Warning("OpenGL not initialized!");
		return false;
	}

    const char *exts = (const char *)qglGetString(GL_EXTENSIONS);
    if(!exts)
        return false;

	idStr new_exts(exts);
	new_exts.Append(" ");
	bool has = new_exts.Find(name) >= 0;

	common->Printf("[Harmattan]: OpenGL extension '%s' -> %s\n", name, has ? "support" : "missing");
	return has;
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

	if (run_once) {
		return run_once;
	}

	if (sys_videoRam.GetInteger()) {
		run_once = sys_videoRam.GetInteger();
		return sys_videoRam.GetInteger();
	}

	// try a few strategies to guess the amount of video ram
	common->Printf("guessing video ram ( use +set sys_videoRam to force ) ..\n");

	run_once = DEFAULT_SYS_VIDEO_MEMORY;
	return run_once;
}
