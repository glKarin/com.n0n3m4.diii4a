/*
** sdlglvideo.cpp
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Christoph Oelckers et.al.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include "i_module.h"
#include "i_soundinternal.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "v_video.h"
#include "version.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "printf.h"

#include "hardware.h"
#include "gl_sysfb.h"
#include "gl_system.h"

#include "gl_renderer.h"
#include "gl_framebuffer.h"
#ifdef HAVE_GLES2
#include "gles_framebuffer.h"
#endif

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

extern int screen_width;
extern int screen_height;
extern int refresh_rate;

// OpenGL attributes
extern int gl_format;
extern int gl_msaa;

#define MAX_NUM_CONFIGS 1000
static volatile ANativeWindow *win;
//volatile bool has_gl_context = false;

#undef EGL_NO_DISPLAY
#undef EGL_NO_SURFACE
#undef EGL_NO_CONTEXT
#undef EGL_DEFAULT_DISPLAY

#define EGL_NO_DISPLAY nullptr
#define EGL_NO_SURFACE nullptr
#define EGL_NO_CONTEXT nullptr
#define EGL_DEFAULT_DISPLAY nullptr

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLConfig configs[1];
static EGLConfig eglConfig = 0;
static EGLint format = WINDOW_FORMAT_RGBA_8888; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

static int gl_multiSamples = 0;

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
		I_FatalError("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	else
		printf("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
}

void GLimp_ActivateContext() {
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

void GLimp_DeactivateContext() {
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
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
	printf("[Harmattan]: EGL surface created and using EGL context.\n");
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
	printf("[Harmattan]: EGL surface destroyed and no EGL context.\n");
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

bool GLimp_OpenDisplay(void)
{
	if(eglDisplay) {
		return true;
	}

	printf( "Setup EGL display connection\n" );

	if ( !( eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY) ) ) {
		printf( "Couldn't open the EGL display\n" );
		return false;
	}
	return true;
}

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
			EGL_SAMPLE_BUFFERS, gl_multiSamples > 1 ? 1 : 0,
			EGL_SAMPLES, gl_multiSamples,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE,
	};

	printf( "[Harmattan]: Request special EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
					red_bits, green_bits,
					blue_bits, alpha_bits,
					depth_bits,
					stencil_bits
			, attrib[15], attrib[17]
	);

	int multisamples = gl_multiSamples;
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
			printf( "[Harmattan]: Get EGL context num -> %d.\n", config_count);
			for(int i = 0; i < config_count; i++)
			{
				EGLConfigInfo_t cinfo = GLimp_GetConfigInfo(eglConfigs[i]);
				printf("\t%d EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
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

		int multisamples = gl_multiSamples;
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
				EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
				EGL_NONE,
		};

		printf( "[Harmattan]: Request EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
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

int GLES_Init() {
	EGLint major, minor;

	if (!GLimp_OpenDisplay()) {
		return false;
	}
	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		GLimp_HandleError("eglInitialize");
		return false;
	}

	printf("Initializing OpenGL display\n");

    if(gl_msaa >= 0)
        gl_multiSamples = gl_msaa;

	if(!GLES_Init_special())
	{
		if(!GLES_Init_prefer())
		{
			I_FatalError("Initializing EGL error");
			return false;
		}
	}

	eglConfig = configs[0];

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return false;
	}

	EGLint ctxAttrib[] = {
			EGL_CONTEXT_CLIENT_VERSION, 3,
			// EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
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

	EGLConfigInfo_t info = GLimp_GetConfigInfo(eglConfig);

	printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
					info.red, info.green,
					info.blue, info.alpha,
					info.depth,
					info.stencil
			, info.samples, info.sample_buffers
	);

	return true;
}

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------
extern IVideo *Video;

EXTERN_CVAR (Int, vid_adapter)
EXTERN_CVAR (Int, vid_displaybits)
EXTERN_CVAR (Int, vid_defwidth)
EXTERN_CVAR (Int, vid_defheight)
EXTERN_CVAR (Bool, cl_capfps)
EXTERN_CVAR(Bool, vk_debug)

// PUBLIC DATA DEFINITIONS -------------------------------------------------

CUSTOM_CVAR(Bool, gl_debug, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL)
{
	Printf("This won't take effect until " GAMENAME " is restarted.\n");
}
CUSTOM_CVAR(Bool, gl_es, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL)
{
	Printf("This won't take effect until " GAMENAME " is restarted.\n");
}

CUSTOM_CVAR(String, vid_sdl_render_driver, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL)
{
	Printf("This won't take effect until " GAMENAME " is restarted.\n");
}

CCMD(vid_list_sdl_render_drivers)
{
}

CUSTOM_CVAR(Int, vid_adapter, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL)
{
}

class SDLVideo : public IVideo
{
public:
	SDLVideo ();
	~SDLVideo ();
	
	DFrameBuffer *CreateFrameBuffer ();
};

// CODE --------------------------------------------------------------------


SDLVideo::SDLVideo ()
{
}

SDLVideo::~SDLVideo ()
{
}


DFrameBuffer *SDLVideo::CreateFrameBuffer ()
{
	SystemBaseFrameBuffer *fb = nullptr;

	if (fb == nullptr)
	{
		fb = new OpenGLESRenderer::OpenGLFrameBuffer(0, vid_fullscreen);
	}

	return fb;
}


IVideo *gl_CreateVideo()
{
	return new SDLVideo();
}


// FrameBuffer Implementation -----------------------------------------------

SystemBaseFrameBuffer::SystemBaseFrameBuffer (void *, bool fullscreen)
: DFrameBuffer (vid_defwidth, vid_defheight)
{
}

int SystemBaseFrameBuffer::GetClientWidth()
{
	return screen_width;
}

int SystemBaseFrameBuffer::GetClientHeight()
{
	return screen_height;
}

bool SystemBaseFrameBuffer::IsFullscreen ()
{
	return true;
}

void SystemBaseFrameBuffer::ToggleFullscreen(bool yes)
{
}

void SystemBaseFrameBuffer::SetWindowSize(int w, int h)
{
}


SystemGLFrameBuffer::SystemGLFrameBuffer(void *hMonitor, bool fullscreen)
: SystemBaseFrameBuffer(hMonitor, fullscreen)
{
	if (!GLimp_OpenDisplay()) {
		I_FatalError("Couldn't open the EGL display\n");
	}

	if (!GLES_Init()) {
		I_FatalError("Couldn't initial GLES context\n");
	}
}

SystemGLFrameBuffer::~SystemGLFrameBuffer ()
{
	if (win)
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

		printf("[Harmattan]: EGL destroyed.\n");
	}
}

int SystemGLFrameBuffer::GetClientWidth()
{
	return screen_width;
}

int SystemGLFrameBuffer::GetClientHeight()
{
	return screen_height;
}

void SystemGLFrameBuffer::SetVSync( bool vsync )
{
}

void SystemGLFrameBuffer::SwapBuffers()
{
	eglSwapBuffers(eglDisplay, eglSurface);
}


void ProcessSDLWindowEvent(const void *event)
{
}


// each platform has its own specific version of this function.
void I_SetWindowTitle(const char* caption)
{
}

float GLimp_GetGLSLVersion(void)
{
	return 3.00f;
}

float GLimp_GetGLVersion(void)
{
	return 3.0f;
}
