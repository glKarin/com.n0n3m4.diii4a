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
#include "gles_system.h"
#endif

#ifdef HAVE_VULKAN
#include "vulkan/system/vk_renderdevice.h"
#include <zvulkan/vulkaninstance.h>
#include <zvulkan/vulkansurface.h>
#include <zvulkan/vulkandevice.h>
#include <zvulkan/vulkanbuilders.h>
//#define VK_USE_PLATFORM_ANDROID_KHR
#include "../../../../../libraries/ZVulkan/include/vulkan/vulkan.h"
#include "../../../../../src/common/rendering/vulkan/textures/vk_framebuffer.h"
#include <zvulkan/vulkanswapchain.h>

EXTERN_CVAR (Int, vid_preferbackend)
#endif

#undef EGL_NO_DISPLAY
#undef EGL_NO_SURFACE
#undef EGL_NO_CONTEXT
#undef EGL_DEFAULT_DISPLAY

#define EGL_NO_DISPLAY nullptr
#define EGL_NO_SURFACE nullptr
#define EGL_NO_CONTEXT nullptr
#define EGL_DEFAULT_DISPLAY nullptr

#define Q3E_PRINTF printf
#define Q3E_ERRORF I_FatalError
#define Q3E_DEBUGF printf
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false

#include "q3e/q3e_glimp.inc"

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

CVAR(Int, harm_gl_es, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL); // 0 = auto(GLES2.0), 2 = GLES2.0, 3 = GLES3.2(GLES 100 shader), 4 = GLES3.2(GLES 320 shader)
CVAR(Int, harm_gl_version, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL); // 0 = auto(4.5), 330 = GL 3.3, 420 = GL 4.2, 430 = GL 4.3, 450 = GL 4.5

class SDLVideo : public IVideo
{
public:
	SDLVideo ();
	~SDLVideo ();
	
	DFrameBuffer *CreateFrameBuffer ();

private:
#ifdef HAVE_VULKAN
public:
	std::shared_ptr<VulkanSurface> surface;
	VulkanRenderDevice *_fb = nullptr; //karin: created from SDLVideo::CreateFrameBuffer
#endif
};

// CODE --------------------------------------------------------------------

#ifdef HAVE_VULKAN

void I_GetVulkanDrawableSize(int *width, int *height)
{
	*width = screen_width;
	*height = screen_height;
}

bool I_GetVulkanPlatformExtensions(unsigned int *count, const char **names)
{
	// from SDL2
	static const char *const extensionsForAndroid[] = {
			VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
	};
	const int num = 2;

	Printf("Require Vulkan extensions:\n");
	unsigned int i = 0;
	for(; i < *count; i++)
	{
		if(i >= num)
			break;
		names[i] = extensionsForAndroid[i];
		Printf("  %2d: %s\n", i + 1, extensionsForAndroid[i]);
	}

	*count = i;

	return true;
}

bool I_CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface)
{
	// Create Android surface from ANativeWindow of SurfaceView
	VkAndroidSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.window = (ANativeWindow *)win; // pointer of ANativeWindow
	VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, surface);
	Printf("Create Android Vulkan surface......%s\n", result == VK_SUCCESS ? "success" : "fail");
	return result == VK_SUCCESS;
}
#endif

// PRIVATE DATA DEFINITIONS ------------------------------------------------
#ifdef HAVE_VULKAN
namespace Priv
{
	bool vulkanEnabled = false;
}
#endif

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
#ifdef HAVE_VULKAN
	if (Priv::vulkanEnabled)
	{
	    Printf("Start Android Vulkan.\n");
		extern IVideo *Video;
		if(!Video)
		{
	        Printf("IVideo not initialized.\n");
			return;
		}
		SDLVideo *sdl_video = (SDLVideo *)Video;

		if(!sdl_video->_fb)
		{
	        Printf("SDLVideo::CreateFrameBuffer not called.\n");
			return;
		}
		if(!sdl_video->_fb->device)
		{
	        Printf("VulkanDevice not initialized.\n");
			return;
		}
		if(!sdl_video->_fb->device->Instance)
		{
	        Printf("VulkanInstance not initialized.\n");
			return;
		}
		if(sdl_video->_fb->device->Instance->Instance == VK_NULL_HANDLE)
		{
	        Printf("vkCreateInstance not called.\n");
			return;
		}
		auto fbm = sdl_video->_fb->GetFramebufferManager();
		if(!fbm)
		{
	        Printf("VkFramebufferManager not initialized.\n");
			return;
		}

		if(!Q3E_RequireWindow(w))
			return;

		if(fbm->SwapChain && !fbm->SwapChain->Lost())
		{
	        Printf("VulkanSwapChain::SwapChain make lost.\n");
			fbm->SwapChain->MakeLost();
		}

		if(sdl_video->surface && sdl_video->surface->Surface != VK_NULL_HANDLE)
		{
			Printf("Destroy old Vulkan surface.\n");
			vkDestroySurfaceKHR(sdl_video->_fb->device->Instance->Instance, sdl_video->surface->Surface, NULL);
			sdl_video->surface->Surface = VK_NULL_HANDLE;
		}

		VkSurfaceKHR surfacehandle = VK_NULL_HANDLE;
		if (!I_CreateVulkanSurface(sdl_video->_fb->device->Instance->Instance, &surfacehandle))
			VulkanError("I_CreateVulkanSurface failed");

	    Printf("Create Vulkan surface: %zu.\n", surfacehandle);
		sdl_video->surface->Surface = surfacehandle;
		sdl_video->_fb->device->Surface = sdl_video->surface;
	    Printf("VulkanSwapChain::AcquireImage.\n");
		fbm->AcquireImage();

		return;
	}
#endif
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
		Q3E_RestoreGL();
}

void GLimp_AndroidQuit(void)
{
#ifdef HAVE_VULKAN
	if (Priv::vulkanEnabled)
	{
	    Printf("Stop Android Vulkan.\n");
		extern IVideo *Video;
		if(!Video)
		{
			Printf("IVideo not initialized.\n");
			return;
		}
		SDLVideo *sdl_video = (SDLVideo *)Video;
		if(!sdl_video->_fb->device)
		{
			Printf("VulkanDevice not initialized.\n");
			return;
		}
		if(sdl_video->_fb->device->device)
		{
			Printf("vkDeviceWaitIdle......\n");
			vkDeviceWaitIdle(sdl_video->_fb->device->device);
			Printf("vkDeviceWaitIdle done.\n");
		}
	    return;
	}
#endif
	Q3E_DestroyGL(true);
}

SDLVideo::SDLVideo ()
{
#ifdef HAVE_VULKAN
	Priv::vulkanEnabled = V_GetBackend() == 1;

	if (Priv::vulkanEnabled)
	{
	    Printf("Enable Android Vulkan.\n");
	}
#endif
}

SDLVideo::~SDLVideo ()
{
#ifdef HAVE_VULKAN
    _fb = nullptr;
#endif
}


DFrameBuffer *SDLVideo::CreateFrameBuffer ()
{
	SystemBaseFrameBuffer *fb = nullptr;

#ifdef HAVE_VULKAN
    if (Priv::vulkanEnabled)
    {
        VkSurfaceKHR surfacehandle = VK_NULL_HANDLE;
        VkInstance vulkankInstance = VK_NULL_HANDLE;
        try
        {
            unsigned int count = 64;
            const char* names[64];
            if (!I_GetVulkanPlatformExtensions(&count, names))
                VulkanError("I_GetVulkanPlatformExtensions failed");

            VulkanInstanceBuilder builder;
            builder.DebugLayer(vk_debug);
            for (unsigned int i = 0; i < count; i++)
                builder.RequireExtension(names[i]);
            auto instance = builder.Create();
            vulkankInstance = instance->Instance;

            if (!I_CreateVulkanSurface(instance->Instance, &surfacehandle))
                VulkanError("I_CreateVulkanSurface failed");

            surface = std::make_shared<VulkanSurface>(instance, surfacehandle);

            fb = new VulkanRenderDevice(nullptr, vid_fullscreen, surface);
			Printf("Using Vulkan renderer.\n");

			this->_fb = (VulkanRenderDevice *)fb;
        }
        catch (CVulkanError const &error)
        {
            Printf(TEXTCOLOR_RED "Initialization of Vulkan failed: %s\n", error.what());
			Priv::vulkanEnabled = false;

			Printf("Destroy Vulkan......\n");
			if(vulkankInstance != VK_NULL_HANDLE)
			{
                if(surfacehandle != VK_NULL_HANDLE)
                {
			        Printf("Destroy Vulkan surface.\n");
                    vkDestroySurfaceKHR(vulkankInstance, surfacehandle, NULL);
                }
			    //Printf("Destroy Vulkan instance.\n");
                //vkDestroyInstance(vulkankInstance, NULL);
			}

            //Q3E_ReleaseWindow(false);
	        //Q3E_RequireCurrentWindow();
			Printf("Try OpenGL renderer......\n");
        }
    }
#endif

	if (fb == nullptr)
	{
#ifdef HAVE_GLES2
        if (V_GetBackend() != 0)
        {
			vid_preferbackend = 2;
            fb = new OpenGLESRenderer::OpenGLFrameBuffer(0, vid_fullscreen);
            Printf("Using OpenGLES renderer.\n");
        }
        else
#endif
        {
			vid_preferbackend = 0;
            fb = new OpenGLRenderer::OpenGLFrameBuffer(0, vid_fullscreen);
            Printf("Using OpenGL renderer.\n");
        }
	}

	return fb;
}


IVideo *gl_CreateVideo()
{
	return new SDLVideo();
}

bool GLimp_InitGL(void)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	if (V_GetBackend() == 0)
	{
		Q3E_GL_CONFIG_ES_3_2();
	}
	else
	{
		if(USING_GLES_2)
		{
			Q3E_GL_CONFIG_ES_2_0();
		}
		else if(USING_GLES_3)
		{
			Q3E_GL_CONFIG_ES_3_2();
		}
		else if(USING_GLES_32)
		{
			Q3E_GL_CONFIG_ES_3_2();
		}
		else
		{
			Q3E_GL_CONFIG_ES_3_0();
		}
	}

	bool res = Q3E_InitGL();
	return res;
}


// FrameBuffer Implementation -----------------------------------------------

SystemBaseFrameBuffer::SystemBaseFrameBuffer (void *, bool fullscreen)
: DFrameBuffer (screen_width, screen_height)
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
	if (!GLimp_InitGL()) {
		I_FatalError("Couldn't initial GLES context\n");
	}
}

SystemGLFrameBuffer::~SystemGLFrameBuffer ()
{
	if (Q3E_HasWindow())
	{
		Q3E_ShutdownGL();
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
	Q3E_SwapBuffers();
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
	if (V_GetBackend() == 0)
		return 3.20f;
	else
	{
		if(USING_GLES_2)
			return 1.00f;
		else if(USING_GLES_3)
			return 3.00f;
		else if(USING_GLES_32)
			return 3.20f;
		else
			return 1.00f; // 3.00f
	}
}

float GLimp_GetGLVersion(void)
{
	if (V_GetBackend() == 0)
	{
		int glVersion = harm_gl_version;
		if (glVersion <= 0)
			return 4.5f; // 4.2f;
		else if(glVersion == 330)
			return 3.3f;
		else if(glVersion == 420)
			return 4.2f;
		else if(glVersion == 430)
			return 4.3f;
		else if(glVersion == 450)
			return 4.5f;
		else
			return float(glVersion) / 100.0f;
	}
	else
	{
		if(USING_GLES_2)
			return 2.0f;
		else if(USING_GLES_3)
			return 3.2f;
		else if(USING_GLES_32)
			return 3.2f;
		else
			return 3.0f;
	}
}
