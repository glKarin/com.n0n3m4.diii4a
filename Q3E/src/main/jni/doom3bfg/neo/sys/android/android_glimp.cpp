/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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
#include <GL/glew.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include "renderer/RenderCommon.h"
#include "android_local.h"

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );
// Linux open source drivers suck
idCVar r_useOpenGL32( "r_useOpenGL32", "2", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
// RB end

extern int screen_width;
extern int screen_height;
extern int refresh_rate;

// OpenGL attributes
extern int gl_format;
extern int gl_msaa;

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

static bool grabbed = false;

extern void Android_GrabMouseCursor(bool grabIt);
extern void Sys_ForceResolution(void);

/*
===============
GLES_Init
===============
*/

void GLimp_ActivateContext() {
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

void GLimp_DeactivateContext() {
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

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
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
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
				EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
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
	ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return false;
	}

	EGLint ctxAttrib[] = {
			//EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
			EGL_CONTEXT_MINOR_VERSION_KHR, 2,
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

	common->Printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
					info.red, info.green,
					info.blue, info.alpha,
					info.depth,
					info.stencil
			, info.samples, info.sample_buffers
	);

	glConfig.isFullscreen = true;
	glConfig.isStereoPixelFormat = false;
	glConfig.nativeScreenWidth = screen_width;
	glConfig.nativeScreenHeight = screen_height;
	glConfig.displayFrequency = refresh_rate;
	glConfig.multisamples = gl_msaa > 1 ? gl_msaa / 2 : 0;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	if (glConfig.isFullscreen) {
		Sys_GrabMouseCursor(true);
	}

	return true;
}

/*
===================
GLimp_PreInit

 R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
 So do that in GLimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void GLimp_PreInit() // DG: added this function for SDL compatibility
{
#ifdef __ANDROID__ //karin: force setup resolution on Android
	Sys_ForceResolution();
#endif
}


/*
===================
GLimp_Init
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#endif

	common->Printf( "Initializing OpenGL subsystem\n" );

	common->Printf("  size=%d x %d; fullscreen=%d; displayHz=%d; multiSamples=%d; stereo=%d\n", parms.width, parms.height, parms.fullScreen, parms.displayHz, parms.multiSamples, parms.stereo);

	GLimp_PreInit(); // DG: make sure SDL is initialized

	if (!GLimp_OpenDisplay()) {
		return false;
	}

	if (!GLES_Init()) {
		return false;
	}

	glewExperimental = GL_TRUE; // GLES3.2
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

	return true;
}
/*
===================
 Helper functions for GLimp_SetScreenParms()
===================
*/

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#endif

	glConfig.isFullscreen = true;
	glConfig.isStereoPixelFormat = false;
	glConfig.nativeScreenWidth = screen_width;
	glConfig.nativeScreenHeight = screen_height;
	glConfig.displayFrequency = refresh_rate;
	glConfig.multisamples = gl_msaa > 1 ? gl_msaa / 2 : 0;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown()
{
#ifdef USE_VULKAN
	common->Printf( "Shutting down Vulkan subsystem\n" );
	return;
#else
	common->Printf( "Shutting down OpenGL subsystem\n" );
#endif

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

	Sys_Printf("[Harmattan]: EGL destroyed.\n");
}

/*
===================
GLimp_SwapBuffers
===================
*/
#ifndef USE_VULKAN
void GLimp_SwapBuffers()
{
	eglSwapBuffers(eglDisplay, eglSurface);
}
#endif

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
}

/*
===================
GLimp_ExtensionPointer
===================
*/
/*
GLExtension_t GLimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void GLimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	Android_GrabMouseCursor(grab);
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->DPrintf( "TODO: DumpAllDisplayDevices\n" );
}



class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode >
{
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

// RB: resolutions supported by XreaL
static void FillStaticVidModes( idList<vidMode_t>& modeList )
{
	modeList.AddUnique( vidMode_t( 320,   240, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 400,   300, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 512,   384, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 640,   480, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 800,   600, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 960,   720, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1024,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1152,  864, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  720, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280,  800, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1360,  768, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1440,  900, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, refresh_rate ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, refresh_rate ) );

	modeList.SortWithTemplate( idSort_VidMode() );
}

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	assert( requestedDisplayNum >= 0 );

	modeList.Clear();

	// DG: SDL1 only knows of one display - some functions rely on
	// R_GetModeListForDisplay() returning false for invalid displaynum to iterate all displays
	if( requestedDisplayNum >= 1 )
	{
		return false;
	}
	// DG end

	FillStaticVidModes( modeList );

	return true;
}
