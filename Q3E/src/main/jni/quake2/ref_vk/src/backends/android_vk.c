// ref_vk

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../vk/header/local.h"
#include "../vk/header/qvk.h"
#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <GLES/gl.h>

#include <dlfcn.h>
#define VULKAN_LIBRARY "libvulkan.so"
#define LoadLibrary(x) dlopen(x, 0)
#define UnloadLibrary dlclose
#define GetProcAddress dlsym
typedef void * LibraryHandle_t;
typedef void * WindowHandle_t;
LibraryHandle_t vkdll;
void * vkGetInstanceProcAddr_f;

// OpenGL attributes
int gl_format = 0x8888;
int gl_depth_bits = 24;
int gl_msaa = 0;
int screen_width = 640;
int screen_height = 480;
int refresh_rate = 60;

#define Q3E_PRINTF(...) R_Printf(PRINT_ALL, __VA_ARGS__)
#define Q3E_ERRORF(...) ri.Sys_Error(ERR_FATAL, __VA_ARGS__)
#define Q3E_DEBUGF printf
#define Q3Ebool qboolean
#define Q3E_TRUE true
#define Q3E_FALSE false

#include "q3e/q3e_glimp.inc"

#pragma GCC visibility push(default)
void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(Q3E_RequireWindow(w))
	{
		ANativeWindow *new_win = (ANativeWindow *)w;
		QVk_SetWindow(new_win);
		// destroy old swapchain manually
		if(vk_swapchain.sc != VK_NULL_HANDLE)
		{
			printf("Destroy old Vulkan swapchain.\n");
			vkDestroySwapchainKHR(vk_device.logical, vk_swapchain.sc, NULL);
			vk_swapchain.sc = VK_NULL_HANDLE;
		}
		if(vk_surface != VK_NULL_HANDLE)
		{
			printf("Destroy old Vulkan surface.\n");
			vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
		}
		if (!Vkimp_CreateSurface(new_win))
			return;
		printf("Recreate Vulkan swapchain.\n");
		//vk_recreateSwapchainNeeded = true;
		QVk_RecreateSwapchain();
	}
}

void GLimp_AndroidQuit(void)
{
	if(vk_device.logical != VK_NULL_HANDLE)
	{
		printf("Waiting Vulkan device idle......\n");
		vkDeviceWaitIdle(vk_device.logical);
		printf("Vulkan device idle.\n");
	}
	Q3E_ReleaseWindow(true);
}

void RI_SetResolution(int aw,int ah)
{
	screen_width = aw;
	screen_height = ah;
	printf("[Harmattan]: RI_SetResolution(%d, %d).\n", aw, ah);
}

void RI_SetGLParms(int f, int msaa, int depthBits)
{
	gl_format = f;
	gl_msaa = msaa;
	gl_depth_bits = depthBits;
	printf( "[Harmattan]: RI_SetGLParms(0x%X, %d, %d).\n", gl_format, gl_msaa, gl_depth_bits);
}
#pragma GCC visibility pop

/*
 * Fills the actual size of the drawable into width and height.
 */
void Android_Vulkan_GetDrawableSize(int* width, int* height)
{
	*width = screen_width;
	*height = screen_height;
}

void * VK_GetProcAddress(const char *name)
{
	if(!vkdll)
	{
		printf("Vulkan library not load!\n");
		return NULL;;
	}
	void *ret = (void *)GetProcAddress(vkdll, name);
	printf("Vulkan get proc address: %s -> %p\n", name, ret);
	return ret;
}

int VK_LoadLibrary()
{
    if(vkdll)
    {
        printf("Vulkan library has loaded!\n");
        return 1;
    }
    vkdll = LoadLibrary(VULKAN_LIBRARY);
    if(!vkdll)
    {
        printf("Vulkan library load fail!\n");
        return 0;
    }
    printf("Vulkan library: %p\n", vkdll);

	vkGetInstanceProcAddr_f = VK_GetProcAddress("vkGetInstanceProcAddr");
	return 1;
}

void VK_UnloadLibrary(void)
{
    if(!vkdll)
    {
        printf("Vulkan library not load!\n");
        return;
    }
	vkGetInstanceProcAddr_f = NULL;
    UnloadLibrary(vkdll);
    vkdll = NULL;
}

int Android_Vulkan_CreateSurface(ANativeWindow *window, VkInstance instance, VkSurfaceKHR *surface)
{
	// Create Android surface from ANativeWindow of SurfaceView
	VkAndroidSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.window = window; // pointer of ANativeWindow
	VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, NULL, surface);
	printf("Create Android Vulkan surface......%s\n", result == VK_SUCCESS ? "success" : "fail");
	return result == VK_SUCCESS ? 1 : 0;
}

int Android_Vulkan_LoadLibrary(const char *path)
{
    (void)path;
    int res = VK_LoadLibrary();
	//if(res) atexit(VK_UnloadLibrary);
	return res ? 0 : 1;
}

void Android_Vulkan_UnloadLibrary(void)
{
	VK_UnloadLibrary();
}

void * Android_Vulkan_GetVkGetInstanceProcAddr(void)
{
    return vkGetInstanceProcAddr_f;
}

int Android_Vulkan_GetInstanceExtensions(unsigned int *count, const char **exts)
{
	// from SDL2
	static const char *const extensionsForAndroid[] = {
			VK_KHR_SURFACE_EXTENSION_NAME, 
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
	};
	const int num = 2;

	*count = num;
	if(exts)
	{
	    for(int i = 0; i < num; i++)
	    {
	        exts[i] = extensionsForAndroid[i];
	    }
	}
	return num;
}
