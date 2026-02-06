/*
Copyright (C) 2006-2007 Mark Olsen

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

#include "quakedef.h"
#include "glquake.h"

extern float sys_dpi_x;
extern float sys_dpi_y;
extern cvar_t vid_vsync;

static dllhandle_t *sys_gl_module = NULL;
static rendererinfo_t gles1rendererinfo;

static void *GLES_GetSymbol(char *symname)
{
	//Can't use android's eglGetProcAddress
	//1) it gives less efficient stubs
	//2) it has a limited number of such stubs, and the limit is too low for core functions too
	void *ret;

	ret = Sys_GetAddressForName(sys_gl_module, symname);

	if (!ret)
		Sys_Warn("GLES_GetSymbol: couldn't find %s\n", symname);
	return ret;
}


#include <EGL/egl.h>
#include <android/native_window.h>
#include <jni.h>
#include <android/native_window_jni.h>

/*android is a real fucking pain*/


static EGLDisplay sys_display;
static EGLSurface sys_surface;
static EGLContext sys_context;
ANativeWindow *sys_nativewindow;

void GLVID_DeInit(void)
{
	if (sys_display)
	{
		eglMakeCurrent(sys_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (sys_context)
			eglDestroyContext(sys_display, sys_context);

		if (sys_surface != EGL_NO_SURFACE)
			eglDestroySurface(sys_display, sys_surface);

		eglTerminate(sys_display);
	}
	sys_display = EGL_NO_DISPLAY;
	sys_context = EGL_NO_CONTEXT;
	sys_surface = EGL_NO_SURFACE;

	GL_ForgetPointers();

Sys_Printf("GLVID_DeInited\n");
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
		if (eglGetConfigAttrib(egldpy, cfg, eglattrs[i].attr, &val))
			Sys_Printf("%i.%s: %i\n", (int)cfg, eglattrs[i].attrname, val);
		else
			Sys_Printf("%i.%s: UNKNOWN\n", (int)cfg, eglattrs[i].attrname);
	}
};
*/

qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
Sys_Printf("GLVID_Initing...\n");
	if (!sys_nativewindow)
	{
		Sys_Printf("GLVID_Init failed: no window known yet\n");
		return false;	//not at this time...
	}

//	vid.pixelwidth = ANativeWindow_getWidth(sys_window);
//	vid.pixelheight = ANativeWindow_getHeight(sys_window);

	vid.numpages = 3;

	EGLint attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, (info->bpp==16)?5:8,
		EGL_GREEN_SIZE, (info->bpp==16)?6:8,
		EGL_RED_SIZE, (info->bpp==16)?5:8,
		EGL_DEPTH_SIZE, 16,
//		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	int glesversion;

	sys_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (sys_display == EGL_NO_DISPLAY)
	{
		GLVID_DeInit();
		return false;
	}
	if (!eglInitialize(sys_display, NULL, NULL))
	{
		GLVID_DeInit();
		return false;
	}
#ifdef EGL_VERSION_1_5
	attribs[1] = EGL_OPENGL_ES3_BIT;
	if (info->renderer==&gles1rendererinfo || !eglChooseConfig(sys_display, attribs, &config, 1, &numConfigs))
#endif
	{
		attribs[1] = EGL_OPENGL_ES2_BIT;
		if (info->renderer==&gles1rendererinfo || !eglChooseConfig(sys_display, attribs, &config, 1, &numConfigs))
		{
			//gles2 was added in egl1.3
			attribs[1] = EGL_OPENGL_ES_BIT;
			if (!eglChooseConfig(sys_display, attribs, &config, 1, &numConfigs))
			{
				//EGL_RENDERABLE_TYPE added in egl1.2
				if (!eglChooseConfig(sys_display, attribs+2, &config, 1, &numConfigs))
				{
					GLVID_DeInit();
					return false;
				}
			}
		}
	}


	eglGetConfigAttrib(sys_display, config, EGL_RENDERABLE_TYPE, &format);
	if (info->renderer==&gles1rendererinfo && (format & EGL_OPENGL_ES_BIT))
		glesversion = 1;
#ifdef EGL_VERSION_1_5
	else if (format & EGL_OPENGL_ES3_BIT)
		glesversion = 3;
#endif
	else if (format & EGL_OPENGL_ES2_BIT)
		glesversion = 2;
	else
		glesversion = 1;

	Sys_Printf("Creating gles %i context\n", glesversion);

//	EGL_ShowConfig(sys_display, config);
	
	sys_surface = eglCreateWindowSurface(sys_display, config, sys_nativewindow, NULL);
	if (!sys_surface)
		return false;
	EGLint ctxattribs[] = {EGL_CONTEXT_CLIENT_VERSION, glesversion, EGL_NONE};
	sys_context = eglCreateContext(sys_display, config, NULL, ctxattribs);
	if (!sys_context)
		return false;


	if (eglMakeCurrent(sys_display, sys_surface, sys_surface, sys_context) == EGL_FALSE)
		return false;

	eglQuerySurface(sys_display, sys_surface, EGL_WIDTH, &w);
	eglQuerySurface(sys_display, sys_surface, EGL_HEIGHT, &h);
	vid.pixelwidth = w;
	vid.pixelheight = h;

	/*now that the context is created, load the dll so that we don't have to crash from eglGetProcAddress issues*/	
	unsigned int gl_major_version = 0;
	const char *(*eglGetString)(GLenum) = (void*)eglGetProcAddress("glGetString");
	const char *s = eglGetString(GL_VERSION);
	while (*s && (*s < '0' || *s > '9'))
		s++;
	gl_major_version = atoi(s);
	const char *driver;
	if ((glesversion<=1) != (gl_major_version<=1))
	{
		Con_Printf(CON_ERROR "Requested gles %i context, but got incompatible %i instead.\n", glesversion, gl_major_version);
		GLVID_DeInit();
		return false;
	}
	if (gl_major_version>=3)
		driver = "libGLESv3.so";
	else if (gl_major_version>=2)
		driver = "libGLESv2.so";
	else
		driver = "libGLESv1_CM.so";
	Sys_Printf("Loading %s\n", driver);
	sys_gl_module = Sys_LoadLibrary(driver, NULL);
	if (!sys_gl_module)
	{
		GLVID_DeInit();
		return false;
	}

	if (!GL_Init(info, GLES_GetSymbol))
		return false;
	Sys_Printf("GLVID_Inited...\n");
	vid_vsync.modified = true;
	return true;
}

void GLVID_SwapBuffers(void)
{
	if (vid_vsync.modified)
	{
		int interval;
		vid_vsync.modified = false;
		if (*vid_vsync.string)
			interval = vid_vsync.ival;
		else
			interval = 1;	//default is to always vsync, according to EGL docs, so lets just do that.
		eglSwapInterval(sys_display, interval);
		Sys_Printf("Swap interval changed\n");
	}

	eglSwapBuffers(sys_display, sys_surface);
	TRACE(("Swap Buffers\n"));

	EGLint w, h;
	eglQuerySurface(sys_display, sys_surface, EGL_WIDTH, &w);
	eglQuerySurface(sys_display, sys_surface, EGL_HEIGHT, &h);
	if (w != vid.pixelwidth || h != vid.pixelheight)
	{
		vid.pixelwidth = w;
		vid.pixelheight = h;
		extern cvar_t vid_conautoscale;
		Cvar_ForceCallback(&vid_conautoscale);
		Sys_Printf("Video Resized\n");
	}
}

qboolean GLVID_ApplyGammaRamps (unsigned int gammarampsize, unsigned short *ramps)
{
	return false;
}

void GLVID_SetCaption(const const char *caption)
{
	// :(
}

static int GLES1VID_GetPriority(void)
{	//assumed 1 when not defined.
	return 1;	//gles1 sucks, and lacks lots of features.
}
static int GLES2VID_GetPriority(void)
{   //assumed 1 when not defined.
	return 2;	//urgh, betterer than gles1 at least.
}
void VID_Register(void)
{
	//many android devices have drivers for both gles1 AND gles2.
	//we default to gles2 because its more capable, but some people might want to try using gles1. so register a renderer for that.
	//the init code explicitly checks for our gles1rendererinfo and tries to create a gles1 context instead.
	extern rendererinfo_t openglrendererinfo;
	gles1rendererinfo = openglrendererinfo;
	gles1rendererinfo.description = "OpenGL ES 1";
	memset(&gles1rendererinfo.name, 0, sizeof(gles1rendererinfo.name));	//make sure there's no 'gl' etc names.
	gles1rendererinfo.name[0] = "gles1";
	gles1rendererinfo.VID_GetPriority = GLES1VID_GetPriority;
	openglrendererinfo.VID_GetPriority = GLES2VID_GetPriority;
	R_RegisterRenderer(NULL, &gles1rendererinfo);
}


#ifdef VKQUAKE
#include "../vk/vkrenderer.h"
static qboolean VKVID_CreateSurface(void)
{
	VkResult err;
	VkAndroidSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
	createInfo.flags = 0;
	createInfo.window = sys_nativewindow;
	err = vkCreateAndroidSurfaceKHR(vk.instance, &createInfo, NULL, &vk.surface);
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

static qboolean VKVID_Init (rendererstate_t *info, unsigned char *palette)
{
	//this is simpler than most platforms, as the window itself is handled by java code, and we can't create/destroy it here
	//(android surfaces can be resized/resampled separately from their window, and are always 'fullscreen' anyway, so this isn't actually an issue for once)
	const char *extnames[] = {VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, NULL};
	Sys_Printf("initialising vulkan...\n");
	if (!sys_nativewindow)
	{
		Sys_Printf("VKVID_Init failed: no window known yet\n");
		return false;
	}
#ifdef VK_NO_PROTOTYPES
	dllhandle_t *hInstVulkan = NULL;
	if (!hInstVulkan)
		hInstVulkan = *info->subrenderer?Sys_LoadLibrary(info->subrenderer, NULL):NULL;
	if (!hInstVulkan)
		hInstVulkan = Sys_LoadLibrary("libvulkan.so", NULL);
	if (!hInstVulkan)
	{
		Con_Printf("Unable to load libvulkan.so\nNo Vulkan drivers are installed\n");
		return false;
	}
	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) Sys_GetAddressForName(hInstVulkan, "vkGetInstanceProcAddr");
#endif

	return VK_Init(info, extnames, VKVID_CreateSurface, NULL);
}
void VKVID_DeInit(void)
{
	VK_Shutdown();
	//that's all folks.
}
static void VKVID_SwapBuffers(void)
{
	// :(
}

static int VKVID_GetPriority(void)
{	//assumed 1 when not defined.
	return 3;	//make it a higher priority than our opengles implementation, because of the infamous black-screens bug that has been plaguing us for years.
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
	VKVID_DeInit,
	VKVID_SwapBuffers,
	GLVID_ApplyGammaRamps,
	NULL,//_CreateCursor,
	NULL,//_SetCursor,
	NULL,//_DestroyCursor,
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
	VKVID_GetPriority,
};
#endif
