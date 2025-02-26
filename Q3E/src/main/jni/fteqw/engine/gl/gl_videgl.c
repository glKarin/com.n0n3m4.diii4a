#include "quakedef.h"
#if defined(GLQUAKE) && defined(USE_EGL)
#include "gl_videgl.h"
#include "vr.h"

//EGL_KHR_gl_colorspace
#ifndef EGL_GL_COLORSPACE_KHR
#define EGL_GL_COLORSPACE_KHR 0x309D
#endif
#ifndef EGL_GL_COLORSPACE_SRGB_KHR
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#endif
#ifndef EGL_GL_COLORSPACE_LINEAR_KHR
#define EGL_GL_COLORSPACE_LINEAR_KHR 0x308A
#endif

extern cvar_t vid_vsync;

EGLContext eglctx = EGL_NO_CONTEXT;
EGLDisplay egldpy = EGL_NO_DISPLAY;
EGLSurface eglsurf = EGL_NO_SURFACE;
static const char *eglexts;

static dllhandle_t *egllibrary;
static dllhandle_t *eslibrary;

static EGLint		(EGLAPIENTRY *qeglGetError)(void);

static EGLDisplay	(EGLAPIENTRY *qeglGetPlatformDisplay)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
static EGLDisplay	(EGLAPIENTRY *qeglGetDisplay)(EGLNativeDisplayType display_id);
static EGLBoolean	(EGLAPIENTRY *qeglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
static const char *	(EGLAPIENTRY *qeglQueryString)(EGLDisplay dpy, EGLint name);
static EGLBoolean	(EGLAPIENTRY *qeglTerminate)(EGLDisplay dpy);

static EGLBoolean	(EGLAPIENTRY *qeglGetConfigs)(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
static EGLBoolean	(EGLAPIENTRY *qeglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean	(EGLAPIENTRY *qeglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
static EGLBoolean	(EGLAPIENTRY *qeglBindAPI) (EGLenum api);

static EGLSurface	(EGLAPIENTRY *qeglCreatePlatformWindowSurface)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list);
static EGLSurface	(EGLAPIENTRY *qeglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
static EGLSurface	(EGLAPIENTRY *qeglCreatePbufferSurface) (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
static EGLBoolean	(EGLAPIENTRY *qeglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
static EGLBoolean	(EGLAPIENTRY *qeglQuerySurface)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);

static EGLBoolean	(EGLAPIENTRY *qeglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static EGLBoolean	(EGLAPIENTRY *qeglMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
static EGLContext	(EGLAPIENTRY *qeglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
static EGLBoolean	(EGLAPIENTRY *qeglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
static void *		(EGLAPIENTRY *qeglGetProcAddress) (const char *name);

static EGLBoolean 	(EGLAPIENTRY *qeglSwapInterval) (EGLDisplay display, EGLint interval);

static dllfunction_t qeglfuncs[] =
{
	{(void*)&qeglGetError, "eglGetError"},
	
	{(void*)&qeglGetDisplay, "eglGetDisplay"},
	{(void*)&qeglInitialize, "eglInitialize"},
	{(void*)&qeglQueryString, "eglQueryString"},
	{(void*)&qeglTerminate, "eglTerminate"},

	{(void*)&qeglGetConfigs, "eglGetConfigs"},
	{(void*)&qeglChooseConfig, "eglChooseConfig"},
	{(void*)&qeglGetConfigAttrib, "eglGetConfigAttrib"},

	{(void*)&qeglCreatePbufferSurface, "eglCreatePbufferSurface"},
	{(void*)&qeglCreateWindowSurface, "eglCreateWindowSurface"},
	{(void*)&qeglDestroySurface, "eglDestroySurface"},
	{(void*)&qeglQuerySurface, "eglQuerySurface"},

	{(void*)&qeglSwapBuffers, "eglSwapBuffers"},
	{(void*)&qeglMakeCurrent, "eglMakeCurrent"},
	{(void*)&qeglCreateContext, "eglCreateContext"},
	{(void*)&qeglDestroyContext, "eglDestroyContext"},

	{(void*)&qeglGetProcAddress, "eglGetProcAddress"},

	//EGL 1.1
	{(void*)&qeglSwapInterval,	"eglSwapInterval"},

	{NULL}
};


void *EGL_Proc(char *f)
{
	void *proc = NULL;

	/*
	char fname[512];
	{
		sprintf(fname, "wrap_%s", f);
		f = fname;
	}
	*/

	if (!proc)
		proc = Sys_GetAddressForName(eslibrary, f);
	if (!proc)
		proc = Sys_GetAddressForName(egllibrary, f);

	//eglGetProcAddress functions must work regardless of context...
	//FIXME: this means lots of false-positives.
	//as well as lots of thunks, which will result in slowdowns.
	if (qeglGetProcAddress)
		proc = qeglGetProcAddress(f);

	return proc;
}

static const char *EGL_GetErrorString(int error)
{
	switch(error)
	{
	case EGL_BAD_ACCESS:			return "BAD_ACCESS";
	case EGL_BAD_ALLOC:				return "BAD_ALLOC";
	case EGL_BAD_ATTRIBUTE:			return "BAD_ATTRIBUTE";
	case EGL_BAD_CONFIG:			return "BAD_CONFIG";
	case EGL_BAD_CONTEXT:			return "BAD_CONEXT";
	case EGL_BAD_CURRENT_SURFACE:	return "BAD_CURRENT_SURFACE";
	case EGL_BAD_DISPLAY:			return "BAD_DISPLAY";
	case EGL_BAD_MATCH:				return "BAD_MATCH";
	case EGL_BAD_NATIVE_PIXMAP:		return "BAD_NATIVE_PIXMAP";
	case EGL_BAD_NATIVE_WINDOW:		return "BAD_NATIVE_WINDOW";
	case EGL_BAD_PARAMETER:			return "BAD_PARAMETER";
	case EGL_BAD_SURFACE:			return "BAD_SURFACE";
	default:						return va("EGL:%#x", error);
	}
}

static qboolean EGL_CheckExtension(const char *extname)
{
	const char *x = eglexts, *n;
	size_t l;
	if (!x)
		return false;
	l = strlen(extname);
	for(;;)
	{
		n = strchr(x, ' ');
		if (!n)
		{
			if (!strcmp(x, extname))
				return true;
			return false;
		}
		else if (n-x==l && !strncmp(x, extname, l))
			return true;
		x = n+1;
	}
}

void EGL_UnloadLibrary(void)
{
	if (egllibrary)
		Sys_CloseLibrary(egllibrary);
	if (egllibrary == eslibrary)
		eslibrary = NULL;
	if (eslibrary)
		Sys_CloseLibrary(eslibrary);
	eslibrary = egllibrary = NULL;
}

qboolean EGL_LoadLibrary(char *driver)
{
	/*	linux seem to load glesv2 first for dependency issues.
		(most things are expected to statically link to their libs)
		strictly speaking, EGL says that functions should work regardless of context.
		(which of course makes portability a nightmare, especially on windows where static linking is basically impossible)
		(android's EGL bugs out if you use eglGetProcAddress for core functions too, note that EGL_KHR_get_all_proc_addresses fixes that.)
	*/
	Sys_Printf("Attempting to dlopen libGLESv2... ");
	eslibrary = Sys_LoadLibrary("libGLESv2", NULL);
	if (!eslibrary)
	{
		Sys_Printf("failed\n");
//		return false;
	}
	else
		Sys_Printf("success\n");
#ifndef _WIN32
	if (!eslibrary)
	{
		eslibrary = dlopen("libGL"ARCH_DL_POSTFIX".1.2", RTLD_NOW|RTLD_GLOBAL);
		if (eslibrary) Sys_Printf("Loaded libGL.so.1.2\n");
	}
	if (!eslibrary)
	{
		eslibrary = dlopen("libGL"ARCH_DL_POSTFIX".1", RTLD_NOW|RTLD_GLOBAL);
		if (eslibrary) Sys_Printf("Loaded libGL.so.1\n");
	}
	if (!eslibrary)
	{
		eslibrary = dlopen("libGL", RTLD_NOW|RTLD_GLOBAL);
		if (eslibrary) Sys_Printf("Loaded libGL\n");
	}
#endif
	if (!eslibrary)
		Sys_Printf("unable to load some libGL\n");

	Sys_Printf("Attempting to dlopen libEGL... ");
	egllibrary = Sys_LoadLibrary("libEGL", qeglfuncs);
	if (!egllibrary)
	{
		Sys_Printf("failed\n");
		Con_Printf("libEGL library not loadable\n");
		/* TODO: some implementations combine EGL/GLESv2 into single library... */
		Sys_CloseLibrary(eslibrary);
		return false;
	}
	Sys_Printf("success\n");


	//egl1.2
	qeglBindAPI = EGL_Proc("eglBindAPI");

	//these are from egl1.5
	qeglGetPlatformDisplay		= EGL_Proc("eglGetPlatformDisplay");
	qeglCreatePlatformWindowSurface	= EGL_Proc("eglCreatePlatformWindowSurface");
	//and in case they arn't defined...
	if (!qeglGetPlatformDisplay)		qeglGetPlatformDisplay		= EGL_Proc("eglGetPlatformDisplayEXT");
	if (!qeglCreatePlatformWindowSurface)	qeglCreatePlatformWindowSurface	= EGL_Proc("eglCreatePlatformWindowSurfaceEXT");

	return true;
}

void EGL_Shutdown(void)
{
	if (eglctx == EGL_NO_CONTEXT)
		return;

	qeglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	qeglDestroyContext(egldpy, eglctx);

	if (eglsurf != EGL_NO_SURFACE)
		qeglDestroySurface(egldpy, eglsurf);

	qeglTerminate(egldpy);

	eglctx = EGL_NO_CONTEXT;
	egldpy = EGL_NO_DISPLAY;
	eglsurf = EGL_NO_SURFACE;
	eglexts = NULL;
}

/*static void EGL_ShowConfig(EGLDisplay egldpy, EGLConfig cfg)
{
	struct
	{
		EGLint attr;
		const char *attrname;
	} eglattrs[] = 
	{
		{EGL_ALPHA_SIZE, "EGL_ALPHA_SIZE"},
		{EGL_ALPHA_MASK_SIZE, "EGL_ALPHA_MASK_SIZE"},
		{EGL_BIND_TO_TEXTURE_RGB, "EGL_BIND_TO_TEXTURE_RGB"},
		{EGL_BIND_TO_TEXTURE_RGBA, "EGL_BIND_TO_TEXTURE_RGBA"},
		{EGL_BLUE_SIZE, "EGL_BLUE_SIZE"},
		{EGL_BUFFER_SIZE, "EGL_BUFFER_SIZE"},
		{EGL_COLOR_BUFFER_TYPE, "EGL_COLOR_BUFFER_TYPE"},
		{EGL_CONFIG_CAVEAT, "EGL_CONFIG_CAVEAT"},
		{EGL_CONFIG_ID, "EGL_CONFIG_ID"},
		{EGL_CONFORMANT, "EGL_CONFORMANT"},
		{EGL_DEPTH_SIZE, "EGL_DEPTH_SIZE"},
		{EGL_GREEN_SIZE, "EGL_GREEN_SIZE"},
		{EGL_LEVEL, "EGL_LEVEL"},
		{EGL_LUMINANCE_SIZE, "EGL_LUMINANCE_SIZE"},
		{EGL_MAX_PBUFFER_WIDTH, "EGL_MAX_PBUFFER_WIDTH"},
		{EGL_MAX_PBUFFER_HEIGHT, "EGL_MAX_PBUFFER_HEIGHT"},
		{EGL_MAX_PBUFFER_PIXELS, "EGL_MAX_PBUFFER_PIXELS"},
		{EGL_MAX_SWAP_INTERVAL, "EGL_MAX_SWAP_INTERVAL"},
		{EGL_MIN_SWAP_INTERVAL, "EGL_MIN_SWAP_INTERVAL"},
		{EGL_NATIVE_RENDERABLE, "EGL_NATIVE_RENDERABLE"},
		{EGL_NATIVE_VISUAL_ID, "EGL_NATIVE_VISUAL_ID"},
		{EGL_NATIVE_VISUAL_TYPE, "EGL_NATIVE_VISUAL_TYPE"},
		{EGL_RED_SIZE, "EGL_RED_SIZE"},
		{EGL_RENDERABLE_TYPE, "EGL_RENDERABLE_TYPE"},
		{EGL_SAMPLE_BUFFERS, "EGL_SAMPLE_BUFFERS"},
		{EGL_SAMPLES, "EGL_SAMPLES"},
		{EGL_STENCIL_SIZE, "EGL_STENCIL_SIZE"},
		{EGL_SURFACE_TYPE, "EGL_SURFACE_TYPE"},
		{EGL_TRANSPARENT_TYPE, "EGL_TRANSPARENT_TYPE"},
		{EGL_TRANSPARENT_RED_VALUE, "EGL_TRANSPARENT_RED_VALUE"},
		{EGL_TRANSPARENT_GREEN_VALUE, "EGL_TRANSPARENT_GREEN_VALUE"},
		{EGL_TRANSPARENT_BLUE_VALUE, "EGL_TRANSPARENT_BLUE_VALUE"},
	};
	size_t i;
	EGLint val;

	for (i = 0; i < countof(eglattrs); i++)
	{
		if (qeglGetConfigAttrib(egldpy, cfg, eglattrs[i].attr, &val))
			Con_DPrintf("%p.%s: %i\n", cfg, eglattrs[i].attrname, val);
		else
			Con_DPrintf("%p.%s: UNKNOWN\n", cfg, eglattrs[i].attrname);
	}
}*/

static void EGL_UpdateSwapInterval(void)
{
	int interval;
	vid_vsync.modified = false;
	if (*vid_vsync.string)
		interval = vid_vsync.ival;
	else
		interval = 1;	//default is to always vsync, according to EGL docs, so lets just do that.

	if (qeglSwapInterval)
		qeglSwapInterval(egldpy, interval);
}

void EGL_SwapBuffers (void)
{
	if (vid_vsync.modified)
		EGL_UpdateSwapInterval();

	TRACE(("EGL_SwapBuffers\n"));
	TRACE(("swapping buffers\n"));
	qeglSwapBuffers(egldpy, eglsurf);
	/* TODO: check result? */
	TRACE(("EGL_SwapBuffers done\n"));
}



qboolean EGL_InitDisplay (rendererstate_t *info, int eglplat, void *ndpy, EGLNativeDisplayType dpyid, EGLConfig *outconfig)
{
	EGLint numconfig=0;
	EGLConfig cfg=0;
	EGLint major=0, minor=0;
	EGLint attrib[] =
	{
		EGL_SURFACE_TYPE, (eglplat==EGL_PLATFORM_DEVICE_EXT)?EGL_PBUFFER_BIT:EGL_WINDOW_BIT,
//		EGL_BUFFER_SIZE, info->bpp,
//		EGL_SAMPLES, info->multisample,
//		EGL_STENCIL_SIZE, 8,
//		EGL_ALPHA_MASK_SIZE, 0,
		EGL_DEPTH_SIZE, info->depthbits?info->depthbits:16,
		EGL_RED_SIZE, 4,
		EGL_GREEN_SIZE, 4,
		EGL_BLUE_SIZE, 4,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

/*	if (!EGL_LoadLibrary(""))
	{
		Con_Printf(CON_ERROR "EGL: unable to load library!\n");
		return false;
	}
*/

	if (qeglGetPlatformDisplay && eglplat)
		egldpy = qeglGetPlatformDisplay(eglplat, ndpy, NULL/*attribs*/);
	else
	{
		if (eglplat == EGL_PLATFORM_WAYLAND_KHR)
			Con_Printf(CON_ERROR "EGL: eglGetPlatformDisplay[EXT] not supported. Your EGL implementation is probably too old.\n");
		egldpy = qeglGetDisplay(dpyid);
	}
	if (egldpy == EGL_NO_DISPLAY)
	{
		Con_Printf(CON_WARNING "EGL: creating default display\n");
		egldpy = qeglGetDisplay(EGL_DEFAULT_DISPLAY);
	}
	if (egldpy == EGL_NO_DISPLAY)
	{
		Con_Printf(CON_ERROR "EGL: can't get display!\n");
		return false;
	}

	//NOTE: mesa's egl really loves to crash on this call, and I define crash as 'anything that fails to return to caller', which fucks everything up.
	if (!qeglInitialize(egldpy, &major, &minor))
	{
		Con_Printf(CON_ERROR "EGL: can't initialize display!\n");
		return false;
	}

	eglexts = qeglQueryString(egldpy, EGL_EXTENSIONS);

/*
	if (!qeglGetConfigs(egldpy, NULL, 0, &numconfigs) || !numconfigs)
	{
		Con_Printf(CON_ERROR "EGL: can't get configs!\n");
		return false;
	}
*/

	if (!qeglChooseConfig(egldpy, attrib, &cfg, 1, &numconfig))
	{
		Con_Printf(CON_ERROR "EGL: can't choose config!\n");
		return false;
	}
	if (!numconfig)
	{
		Con_Printf(CON_ERROR "EGL: no configs!\n");
		return false;
	}

//	EGL_ShowConfig(egldpy, cfg);
	*outconfig = cfg;
	return true;
}

qboolean EGL_InitWindow (rendererstate_t *info, int eglplat, void *nwindow, EGLNativeWindowType windowid, EGLConfig cfg)
{
	EGLint renderabletype;

	if (eglplat == EGL_PLATFORM_DEVICE_EXT && qeglCreatePbufferSurface)
	{
		EGLint wndattrib[] =
		{
			EGL_WIDTH, vid.pixelwidth,
			EGL_HEIGHT, vid.pixelheight,
//			EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
			EGL_NONE,EGL_NONE
		};
		eglsurf = qeglCreatePbufferSurface(egldpy, cfg, wndattrib);
	}
	else if (qeglCreatePlatformWindowSurface)
	{
		EGLAttrib wndattrib[3*2];
		size_t i = 0;

		if (info->srgb)
		{
			wndattrib[i++] = EGL_GL_COLORSPACE_KHR;
			wndattrib[i++] = EGL_GL_COLORSPACE_SRGB_KHR;
		}
		if (EGL_CheckExtension("EGL_EXT_present_opaque"))
		{	//try to avoid nasty surprises.
			#ifndef EGL_PRESENT_OPAQUE_EXT
				#define EGL_PRESENT_OPAQUE_EXT                  0x31DF
			#endif
			wndattrib[i++] = EGL_PRESENT_OPAQUE_EXT;
			wndattrib[i++] = EGL_TRUE;
		}
		wndattrib[i++] = EGL_NONE;
		wndattrib[i++] = EGL_NONE;
		eglsurf = qeglCreatePlatformWindowSurface(egldpy, cfg, nwindow, wndattrib);
	}
	else
	{
		EGLint wndattrib[] =
		{
//			EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,

			EGL_NONE,EGL_NONE
		};
		eglsurf = qeglCreateWindowSurface(egldpy, cfg, windowid, info->srgb?wndattrib:NULL);
	}
	if (eglsurf == EGL_NO_SURFACE)
	{
		int err = qeglGetError();
		if (eglplat == EGL_PLATFORM_WAYLAND_KHR && err == EGL_BAD_DISPLAY)	//slightly more friendly error that slags off nvidia for their refusal to implement existing standards, as is apparently appropriate.
			Con_Printf(CON_ERROR "EGL: eglCreateWindowSurface failed: Bad Display. Your wayland setup is probably not properly supported by your video drivers.\n");
		else
			Con_Printf(CON_ERROR "EGL: eglCreateWindowSurface failed: %s\n", EGL_GetErrorString(err));
		return false;
	}

	qeglGetConfigAttrib(egldpy, cfg, EGL_RENDERABLE_TYPE, &renderabletype);
	if ((renderabletype & EGL_OPENGL_BIT) && qeglBindAPI)
	{
		EGLint contextattr[] =
		{
//			EGL_CONTEXT_OPENGL_DEBUG, 1,
//			EGL_CONTEXT_OPENGL_PROFILE_MASK,  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
//			EGL_CONTEXT_CLIENT_VERSION, 2,	//requires EGL 1.3
//			EGL_TEXTURE_TARGET,EGL_NO_TEXTURE,	//just a rendertarget, not a texture.
			EGL_NONE,EGL_NONE
		};
		qeglBindAPI(EGL_OPENGL_API);

		eglctx = qeglCreateContext(egldpy, cfg, EGL_NO_SURFACE, contextattr);
	}
	else
	{
		EGLint contextattr[] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,	//requires EGL 1.3
			EGL_NONE, EGL_NONE
		};
		eglctx = qeglCreateContext(egldpy, cfg, EGL_NO_SURFACE, contextattr);
	}
	if (eglctx == EGL_NO_CONTEXT)
	{
		Con_Printf(CON_ERROR "EGL: no context!\n");
		return false;
	}


	if (!qeglMakeCurrent(egldpy, eglsurf, eglsurf, eglctx))
	{
		Con_Printf(CON_ERROR "EGL: can't make current!\n");
		return false;
	}

	EGL_UpdateSwapInterval();


	if (info->vr)
	{
		vrsetup_t vrsetup = {sizeof(vrsetup)};
		vrsetup.vrplatform = VR_EGL;
		vrsetup.egl.getprocaddr = qeglGetProcAddress;
		vrsetup.egl.egldisplay = egldpy;
		vrsetup.egl.eglconfig = cfg;
		vrsetup.egl.eglcontext = eglctx;
		if (!info->vr->Prepare(&vrsetup) ||
			!info->vr->Init(&vrsetup, info))
		{
			info->vr->Shutdown();
			info->vr = NULL;
		}
		else
			vid.vr = info->vr;
	}

	return true;
}



//code for headless/pbuffer rendering.
//in terms of energy efficiency its better to use the true headless renderer which stubs out ALL rendering.
//however that's not very useful for screenshots or demo captures, so we offer this route too.
#include "glquake.h"
#include "gl_draw.h"
#include "shader.h"
#include "EGL/eglext.h"
static void EGLHeadless_SwapBuffers(void)
{
	if (R2D_Flush)
		R2D_Flush();

	EGL_SwapBuffers();
}
static qboolean EGLHeadless_Init (rendererstate_t *info, unsigned char *palette)
{
	EGLConfig cfg;
	void *dpy = NULL;

	if (!EGL_LoadLibrary(info->subrenderer))
	{
		Con_Printf("couldn't load EGL library\n");
		return false;
	}

#ifdef EGL_EXT_device_base
	if (*info->devicename)
	{
		EGLDeviceEXT devs[64];
		EGLint count;
		EGLint idx = atoi(info->devicename);
		EGLBoolean (EGLAPIENTRY *qeglQueryDevicesEXT) (EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices) = EGL_Proc("eglQueryDevicesEXT");
		if (qeglQueryDevicesEXT)
		{
			qeglQueryDevicesEXT(countof(devs), devs, &count);
			if (idx >= 0 && idx < count)
				dpy = devs[idx];
		}
	}
#endif

	vid.pixelwidth = (info->width>0)?info->width:640;
	vid.pixelheight = (info->height>0)?info->height:480;
	vid.activeapp = true;

	if (!EGL_InitDisplay(info, EGL_PLATFORM_DEVICE_EXT, dpy, (EGLNativeDisplayType)EGL_NO_DISPLAY, &cfg))
	{
		Con_Printf("couldn't find suitable EGL config\n");
		return false;
	}

	if (!EGL_InitWindow(info, EGL_PLATFORM_DEVICE_EXT, EGL_NO_SURFACE, (EGLNativeWindowType)EGL_NO_SURFACE, cfg))
	{
		Con_Printf("couldn't initialise EGL context\n");
		return false;
	}

	if (GL_Init(info, &EGL_Proc))
		return true;
	Con_Printf(CON_ERROR "Unable to initialise egl_headless.\n");
	return false;
}
static void EGLHeadless_DeInit(void)
{
	EGL_Shutdown();
	EGL_UnloadLibrary();
	GL_ForgetPointers();
}
static qboolean EGLHeadless_ApplyGammaRamps(unsigned int gammarampsize, unsigned short *ramps)
{
	//not supported
	return false;
}
static void EGLHeadless_SetCaption(const char *text)
{
}

static int EGLHeadless_GetPriority(void)
{
	return -1;	//lowest priority, so its never auto-used..
}

rendererinfo_t rendererinfo_headless_egl =
{
	"Headless OpenGL (EGL)",
	{
		"egl_headless",
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

	EGLHeadless_Init,
	EGLHeadless_DeInit,
	EGLHeadless_SwapBuffers,
	EGLHeadless_ApplyGammaRamps,
	NULL,
	NULL,
	NULL,
	EGLHeadless_SetCaption,       //setcaption
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

	"",
	EGLHeadless_GetPriority
};

#endif

