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

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

//#include <vulkan/vulkan.h>
// SRS - optinally needed for VK_MVK_MOLTENVK_EXTENSION_NAME visibility
#include <vector>

#include "renderer/RenderCommon.h"
#include "android_local.h"

#include <dlfcn.h>
#define VULKAN_LIBRARY "libvulkan.so"
#define LoadLibrary(x) dlopen(x, 0)
#define UnloadLibrary dlclose
#define GetProcAddress dlsym
typedef void * LibraryHandle_t;
typedef void * WindowHandle_t;
LibraryHandle_t vkdll;

#define VK_PROC(name) PFN_##name q##name;
#include "renderer/Vulkan/vkproc.h"

void VK_LoadLibrary()
{
    if(vkdll)
    {
        printf("Vulkan library has loaded!\n");
        return;
    }
    vkdll = LoadLibrary(VULKAN_LIBRARY);
    if(!vkdll)
    {
        printf("Vulkan library load fail!\n");
        exit(1);
    }
    printf("Vulkan library: %p\n", vkdll);
}

void VK_UnloadLibrary()
{
    if(!vkdll)
    {
        printf("Vulkan library not load!\n");
        return;
    }
    UnloadLibrary(vkdll);
    vkdll = NULL;
}

void * VK_GetProcAddress(const char *name)
{
    if(!vkdll)
    {
        printf("Vulkan library not load!\n");
        exit(2);
    }
    void *ret = (void *)GetProcAddress(vkdll, name);
	printf("Vulkan get proc address: %s -> %p\n", name, ret);
	return ret;
}

void VK_LoadSymbols()
{
    VK_LoadLibrary();

#define VK_PROC(name) q##name = (PFN_##name)VK_GetProcAddress(#name);
#include "renderer/Vulkan/vkproc.h"

    if(!vkCreateInstance)
    {
        abort();
    }
}

extern idCVar in_nograb;
extern idCVar r_useOpenGL32;

static bool grabbed = false;

//vulkanContext_t vkcontext; // Eric: I added this to pass SDL_Window* window to the SDL_Vulkan_* methods that are used else were.

extern ANativeWindow * Android_GetVulkanWindow();
extern int screen_width;
extern int screen_height;
extern int refresh_rate;
extern void Android_GrabMouseCursor(bool grabIt);
extern void Sys_ForceResolution(void);

// Eric: Integrate this into RBDoom3BFG's source code ecosystem.
// Helper function for using SDL2 and Vulkan on Linux.
std::vector<const char*> get_required_extensions()
{
	uint32_t                 sdlCount = 0;
	std::vector<const char*> sdlInstanceExtensions = {};

	sdlInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	sdlInstanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

	// SRS - Report enabled instance extensions in CreateVulkanInstance() vs. doing it here
	/*
	if( enableValidationLayers )
	{
		idLib::Printf( "\nNumber of availiable instance extensions\t%i\n", sdlCount );
		idLib::Printf( "Available Extension List: \n" );
		for( auto ext : sdlInstanceExtensions )
		{
			idLib::Printf( "\t%s\n", ext );
		}
	}
	*/

	// SRS - needed for MoltenVK portability implementation and optionally for MoltenVK configuration on OSX

	// SRS - Add debug instance extensions in CreateVulkanInstance() vs. hardcoding them here
	/*
	if( enableValidationLayers )
	{
		sdlInstanceExtensions.push_back( "VK_EXT_debug_report" );
		sdlInstanceExtensions.push_back( "VK_EXT_debug_utils" );

		idLib::Printf( "Number of active instance extensions\t%zu\n", sdlInstanceExtensions.size() );
		idLib::Printf( "Active Extension List: \n" );
		for( auto const& ext : sdlInstanceExtensions )
		{
			idLib::Printf( "\t%s\n", ext );
		}
	}
	*/

	return sdlInstanceExtensions;
}

/*
===================
VKimp_PreInit

 R_GetModeListForDisplay is called before VKimp_Init(), but SDL needs SDL_Init() first.
 So do that in VKimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void VKimp_PreInit() // DG: added this function for SDL compatibility
{
#ifdef __ANDROID__ //karin: force setup resolution on Android
	Sys_ForceResolution();
#endif
	VK_LoadSymbols();
}


/* Eric: Is the majority of this function not needed since switching from GL to Vulkan?
===================
VKimp_Init
===================
*/
bool VKimp_Init( glimpParms_t parms )
{

	common->Printf( "Initializing Vulkan subsystem\n" );

	VKimp_PreInit(); // DG: make sure SDL is initialized

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;

	for( int i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2 :
					if( colorbits == 24 )
					{
						colorbits = 16;
					}
					break;
				case 1 :
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3 :
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int channelcolorbits = 4;
		if( tcolorbits == 24 )
		{
			channelcolorbits = 8;
		}

		vkcontext.sdlWindow = Android_GetVulkanWindow();
		// RB begin
		glConfig.nativeScreenWidth = screen_width;
		glConfig.nativeScreenHeight = screen_height;
		// RB end

		glConfig.isFullscreen = true;

		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		// RB begin
		glConfig.displayFrequency = 60;
		glConfig.isStereoPixelFormat = parms.stereo;
		glConfig.multisamples = parms.multiSamples;

		glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
		// should side-by-side stereo modes be consider aspect 0.5?

		// RB end

		break;
	}

	return true;
}

/*
===================
VKimp_SetScreenParms
===================
*/
bool VKimp_SetScreenParms( glimpParms_t parms )
{
	glConfig.isFullscreen = true;
	glConfig.isStereoPixelFormat = false;
	glConfig.nativeScreenWidth = screen_width;
	glConfig.nativeScreenHeight = screen_height;
	glConfig.displayFrequency = refresh_rate;
	glConfig.multisamples = 0; //Q3E_GL_CONFIG(samples) > 1 ? Q3E_GL_CONFIG(samples) / 2 : 0;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	return true;
}

/*
===================
VKimp_Shutdown
===================
*/
void VKimp_Shutdown()
{
	common->Printf( "Shutting down Vulkan subsystem\n" );
	VK_UnloadLibrary();
}

/* Eric: Is this needed/used for Vulkan?
=================
VKimp_SetGamma
=================
*/
void VKimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
}

/*
===================
VKimp_ExtensionPointer
===================
*/
/*
GLExtension_t VKimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void VKimp_GrabInput( int flags )
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

void SDL_Vulkan_GetDrawableSize( void *sdlWindow, int *width, int *height )
{
	(void)sdlWindow;
	if(width)
		*width = screen_width;
	if(height)
		*height = screen_height;
}

extern void VK_RecreateSwapchain( void );
extern void VK_CreateSurface( void );
void VKimp_RecreateSurfaceAndSwapchain(void)
{
	printf("Start Android Vulkan.\n");
	if(vkcontext.instance == VK_NULL_HANDLE)
	{
		printf("vkCreateInstance not called.\n");
		return;
	}

    if(vkcontext.surface != VK_NULL_HANDLE)
    {
        printf("Destroy old Vulkan surface.\n");
        vkDestroySurfaceKHR( vkcontext.instance, vkcontext.surface, NULL );
        vkcontext.surface = VK_NULL_HANDLE;
    }

	vkcontext.sdlWindow = Android_GetVulkanWindow();
	vkcontext.surface = VK_NULL_HANDLE;
	VK_CreateSurface();
	VK_RecreateSwapchain();
}

void VKimp_WaitForStop(void)
{
	printf("Stop Android Vulkan.\n");
	if(vkcontext.device == VK_NULL_HANDLE)
	{
		printf("VulkanDevice not initialized.\n");
		return;
	}
	printf("vkDeviceWaitIdle......\n");
	vkDeviceWaitIdle(vkcontext.device);
	printf("vkDeviceWaitIdle done.\n");
}

#if 0
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
	modeList.AddUnique( vidMode_t( 320,   240, 60 ) );
	modeList.AddUnique( vidMode_t( 400,   300, 60 ) );
	modeList.AddUnique( vidMode_t( 512,   384, 60 ) );
	modeList.AddUnique( vidMode_t( 640,   480, 60 ) );
	modeList.AddUnique( vidMode_t( 800,   600, 60 ) );
	modeList.AddUnique( vidMode_t( 960,   720, 60 ) );
	modeList.AddUnique( vidMode_t( 1024,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1152,  864, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  720, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  800, 60 ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, 60 ) );
	modeList.AddUnique( vidMode_t( 1360,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1440,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );

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

	// DG: SDL2 implementation
	if( requestedDisplayNum >= 1 )
	{
		// requested invalid displaynum
		return false;
	}

	FillStaticVidModes( modeList );

	return true;
	// DG end
}
#endif
