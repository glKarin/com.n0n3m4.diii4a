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
extern cvar_t vid_multisample;

static dllhandle_t *sys_gl_module = NULL;
static rendererinfo_t gles1rendererinfo;

extern qboolean vid_isfullscreen;

#ifdef VKQUAKE
#include "../vk/vkrenderer.h"
static qboolean using_vulkan = false;
static qboolean VKVID_CreateSurface(void);
#endif

#define Q3E_PRINTF Sys_Printf
#define Q3E_ERRORF(...) Sys_Printf(__VA_ARGS__)
#define Q3E_DEBUGF Sys_Printf
#define Q3Ebool qboolean
#define Q3E_TRUE true
#define Q3E_FALSE false

#include "q3e/q3e_glimp.inc"

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
    Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
#ifdef VKQUAKE
	// Vulkan
	if(using_vulkan)
	{
		if(Q3E_RequireWindow(w))
		{
			if(vk.device != VK_NULL_HANDLE)
			{
				vkDeviceWaitIdle(vk.device);
                if (vk.surface)
                {
                    printf("Destroy old Vulkan surface.\n");
                    vkDestroySurfaceKHR(vk.instance, vk.surface, vkallocationcb);
                    vk.surface = VK_NULL_HANDLE;
                }
				if(VKVID_CreateSurface())
				{
					printf("Vulkan request recreate swapchain......\n");
					// TODO: crash if destroy swapchain in VK_DestroySwapChain
                    if (vk.swapchain)
                    {
                        printf("Destroy old Vulkan swapchain.\n");
                        vkDestroySwapchainKHR(vk.device, vk.swapchain, vkallocationcb);
                        vk.swapchain = VK_NULL_HANDLE;
                    }
					vk.neednewswapchain = true;
				}
				else
				{
					printf("Vulkan create surface fail when recreate swapchain.\n");
				}
			}
		}
		return;
	}
#endif
	// OpenGL
    if(Q3E_NoDisplay())
        return;

    if(Q3E_RequireWindow(w))
        Q3E_RestoreGL();
}

void GLimp_AndroidQuit(void)
{
#ifdef VKQUAKE
	// Vulkan
	if(using_vulkan)
	{
		if(vk.device != VK_NULL_HANDLE)
		{
			printf("vkDeviceWaitIdle......\n");
			vkDeviceWaitIdle(vk.device);
			printf("vkDeviceWaitIdle done.\n");
		}
		Q3E_ReleaseWindow(true);
		return;
	}
#endif
	// OpenGL
    Q3E_DestroyGL(true);
}

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

/*android is a real fucking pain*/

void GLVID_DeInit(void)
{
	Q3E_ShutdownGL();

	GL_ForgetPointers();

Sys_Printf("GLVID_DeInited\n");
}

qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
    Sys_Printf("GLVID_Initing...\n");
    vid_isfullscreen = true;
//	vid.pixelwidth = ANativeWindow_getWidth(sys_window);
//	vid.pixelheight = ANativeWindow_getHeight(sys_window);

	vid.numpages = 3;

	Q3E_GL_CONFIG_SET(samples, vid_multisample.ival/* * 2*/)
	Q3E_GL_CONFIG_SET(swap_interval, -1);

	int glesversion;

	if (info->renderer==&gles1rendererinfo)
	{
		Q3E_GL_CONFIG_ES_1_1();
	}
	else
	{
		Q3E_GL_CONFIG_ES_3_0();
	}


	if (info->renderer==&gles1rendererinfo)
		glesversion = 1;
	else
		glesversion = 3;

	qboolean res = Q3E_InitGL();
	if(!res)
	{
		return res;
	}

	Sys_Printf("Creating gles %i context\n", glesversion);

#if 1
	vid.pixelwidth = screen_width;
	vid.pixelheight = screen_height;
#else
	Q3E_QuerySurfaceSize(&w, &h);
	vid.pixelwidth = w;
	vid.pixelheight = h;
#endif

	/*now that the context is created, load the dll so that we don't have to crash from eglGetProcAddress issues*/	
	unsigned int gl_major_version = 0;
	const char *(*eglGetString)(GLenum) = (void*)Q3E_GET_PROC_ADDRESS("glGetString");
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
#ifdef VKQUAKE
	Sys_Printf("Using OpenGL backend.\n");
	using_vulkan = false;
#endif
	Sys_Printf("GLVID_Inited...\n");
	vid_vsync.modified = true;
	return true;
}

void GLVID_SwapBuffers(void)
{
	switch(qrenderer)
	{
		case QR_OPENGL:
		if (vid_vsync.modified)
		{
			vid_vsync.modified = false;
			if (*vid_vsync.string)
			{
				Q3E_SwapInterval(vid_vsync.ival);
			}
		}

		Q3E_SwapBuffers();
		break;
		default:
			break;
	}
}

qboolean GLVID_ApplyGammaRamps (unsigned int gammarampsize, unsigned short *ramps)
{
	return false;
}

void GLVID_SetCaption(const char *caption)
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
qboolean VKVID_CreateSurface(void)
{
	VkResult err;
	VkAndroidSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
	createInfo.flags = 0;
	createInfo.window = (ANativeWindow *)win;
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
	Sys_Printf("initialising vulkan...\n");
    vid_isfullscreen = true;
	//this is simpler than most platforms, as the window itself is handled by java code, and we can't create/destroy it here
	//(android surfaces can be resized/resampled separately from their window, and are always 'fullscreen' anyway, so this isn't actually an issue for once)
	const char *extnames[] = {VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, /*NULL*/};
	if (!win)
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

	qboolean res = VK_Init(info, extnames, countof(extnames), VKVID_CreateSurface, NULL);
#ifdef VKQUAKE
	if(res)
	{
		Sys_Printf("Using Vulkan backend.\n");
		using_vulkan = true;
	}
#endif
	return res;
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
	GLVID_SwapBuffers, // VKVID_SwapBuffers,
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
