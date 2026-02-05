/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

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

#include <SDL.h>
#include <SDL_syswm.h>

#include "sys/platform.h"
#include "framework/Licensee.h"

#include "renderer/tr_local.h"

idCVar in_nograb("in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing");
idCVar r_waylandcompat("r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer");
idCVar r_debugGLContext( "r_debugGLContext", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE | CVAR_BOOL, "Enable a debug OpenGL context" );

static bool grabbed = false;

#ifdef _WIN32 // blendo eric TODO: unsure if this works
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
static SDL_Window *window = NULL;
static SDL_GLContext context = NULL;
#else
static SDL_Surface *window = NULL;
#define SDL_WINDOW_OPENGL SDL_OPENGL
#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
#endif

static void SetSDLIcon()
{
	Uint32 rmask, gmask, bmask, amask;

	// ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use a void* for the data,
	// but it's really treated as endian-specific Uint32* ...
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	#include "doom_icon.h" // contains the struct d3_icon

	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom((void*)d3_icon.pixel_data, d3_icon.width, d3_icon.height,
			d3_icon.bytes_per_pixel*8, d3_icon.bytes_per_pixel*d3_icon.width,
			rmask, gmask, bmask, amask);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetWindowIcon(window, icon);
#else
	SDL_WM_SetIcon(icon, NULL);
#endif

	SDL_FreeSurface(icon);
}

extern idStrList displayVidModes;
#ifdef _DIII4A //karin: setup RGBA and depth to special value
extern int gl_format;
extern int gl_depth_bits;
extern int gl_msaa;
extern int screen_width;
extern int screen_height;

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

static bool GLimp_InitSpecial(glimpParms_t parms) {
#if 0 //karin: always use special config first
    const SDL_bool UsingSpecialConfig = SDL_GetHintBoolean("SDL_HINT_EGL_Q3E_SPECIAL_CONFIG", SDL_FALSE);
    if(!UsingSpecialConfig)
        return false;
#endif        

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

    if (parms.fullScreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    if (parms.borderless)
        flags |= SDL_WINDOW_BORDERLESS;

    int colorbits = 24;
    int depthbits = 24;
    int stencilbits = 8;
    int redbits = 8;
    int greenbits = 8;
    int bluebits = 8;
    int alphabits = 8;
	parms.stereo = 0;

    switch (gl_format) {
        case GLFORMAT_RGB565:
            redbits = 5;
            greenbits = 6;
            bluebits = 5;
            alphabits = 0;
            break;
        case GLFORMAT_RGBA4444:
            redbits = 4;
            greenbits = 4;
            bluebits = 4;
            alphabits = 4;
            break;
        case GLFORMAT_RGBA5551:
            redbits = 5;
            greenbits = 5;
            bluebits = 5;
            alphabits = 1;
            break;
        case GLFORMAT_RGBA1010102:
            redbits = 10;
            greenbits = 10;
            bluebits = 10;
            alphabits = 2;
            break;
        case GLFORMAT_RGBA8888:
        default:
            redbits = 8;
            greenbits = 8;
            bluebits = 8;
            alphabits = 8;
            break;
    }

    switch(gl_depth_bits)
    {
        case 16:
            depthbits = 16;
            break;
        case 32:
            depthbits = 32;
            break;
        case 24:
        default:
            depthbits = 24;
            break;
    }

	parms.multiSamples = gl_msaa;
    colorbits = redbits + greenbits + bluebits;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redbits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenbits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bluebits);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SM: We don't need to request a depth/stencil here because
    // we don't render to default framebuffer anymore except a blit at the end
    //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, tdepthbits);
    //SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, tstencilbits);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthbits);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilbits);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphabits);

    SDL_GL_SetAttribute(SDL_GL_STEREO, parms.stereo ? 1 : 0);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples);

        // SM: Option to create a debug context
#ifdef _DEBUG
    if ( r_debugGLContext.GetBool() ) {
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
    }
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
    window = SDL_CreateWindow(GAME_NAME,
									SDL_WINDOWPOS_UNDEFINED,
									SDL_WINDOWPOS_UNDEFINED,
									parms.width, parms.height, flags);

    if (!window) {
        common->DPrintf("Couldn't set GL mode %d/%d/%d: %s",
                        colorbits, depthbits, stencilbits, SDL_GetError());
        return false;
    }

    context = SDL_GL_CreateContext(window);

    if (SDL_GL_SetSwapInterval(r_swapInterval.GetInteger()) < 0)
        common->Warning("SDL_GL_SWAP_CONTROL not supported");

    { // blendo eric: get max resolution
        SDL_DisplayMode displayMode;
        SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
        glConfig.displayWidth = displayMode.w;
        glConfig.displayHeight = displayMode.h;
    }

    // SM: Build list of available video modes (for full screen)
    const float ASPECT_RATIO_DIFF = 0.2f; // Used to decide how much different aspect ratio is allowed
    const int MIN_WIDTH = 1024; // minimum width required
    const int MIN_HEIGHT = 720; // minimum height required
    const float ASPECT_RATIO_WIDESCREEN = 1.5f;
    float aspectRatio = glConfig.displayWidth / static_cast<float>(glConfig.displayHeight);
    displayVidModes.Clear();

    int displayIndex = SDL_GetWindowDisplayIndex(window);
    int numDisplayModes = SDL_GetNumDisplayModes(displayIndex);
    for (int i = 0; i < numDisplayModes; i++)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayIndex, i, &mode);
        float modeRatio = mode.w / static_cast<float>(mode.h);
        if (mode.w >= MIN_WIDTH && mode.h >= MIN_HEIGHT &&
            (fabs(aspectRatio - modeRatio) < ASPECT_RATIO_DIFF || modeRatio >= ASPECT_RATIO_WIDESCREEN))
        {
            idStr resString = idStr::Format("%dx%d", mode.w, mode.h);
            displayVidModes.AddUnique(resString);
        }
    }

    SDL_GetWindowSize(window, &glConfig.vidWidth, &glConfig.vidHeight);

    SetSDLIcon(); // for SDL2  this must be done after creating the window

    glConfig.isFullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN;
    glConfig.isBorderless = (SDL_GetWindowFlags(window) & SDL_WINDOW_BORDERLESS) == SDL_WINDOW_BORDERLESS;
#else
    SDL_WM_SetCaption(GAME_NAME, GAME_NAME);

    SetSDLIcon(); // for SDL1.2  this must be done before creating the window

    if (SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger()) < 0)
        common->Warning("SDL_GL_SWAP_CONTROL not supported");

    window = SDL_SetVideoMode(parms.width, parms.height, colorbits, flags);
    if (!window) {
        common->DPrintf("Couldn't set GL mode %d/%d/%d: %s",
                        colorbits, depthbits, stencilbits, SDL_GetError());
        return false;
    }

    glConfig.vidWidth = window->w;
    glConfig.vidHeight = window->h;

    glConfig.isFullscreen = (window->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN;
#endif

    common->Printf("Using special %d color bits, %d depth, %d stencil display, size %dx%d\n",
                   colorbits, depthbits, stencilbits, glConfig.vidWidth, glConfig.vidHeight);

    glConfig.colorBits = colorbits;
    glConfig.depthBits = depthbits;
    glConfig.stencilBits = stencilbits;

    glConfig.displayFrequency = 0;

    return true;
}
#endif

/*
===================
GLimp_Init
===================
*/
bool GLimp_Init(glimpParms_t parms) {
	common->Printf("Initializing OpenGL subsystem\n");

	assert(SDL_WasInit(SDL_INIT_VIDEO));

#ifdef __ANDROID__ //karin: using EGL and OpenGLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

#ifdef _DIII4A //karin: try special RGBA and depth value
    parms.fullScreen = true;
    parms.width = screen_width;
    parms.height = screen_height;

    if(GLimp_InitSpecial(parms))
        return true;
#endif

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

	if (parms.fullScreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	if (parms.borderless)
		flags |= SDL_WINDOW_BORDERLESS;

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;

	for (int i = 0; i < 16; i++) {
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

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, channelcolorbits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, channelcolorbits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, channelcolorbits);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		// SM: We don't need to request a depth/stencil here because
		// we don't render to default framebuffer anymore except a blit at the end
		//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, tdepthbits);
		//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, tstencilbits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

		if (r_waylandcompat.GetBool())
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
		else
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, channelcolorbits);

		SDL_GL_SetAttribute(SDL_GL_STEREO, parms.stereo ? 1 : 0);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples);

		// SM: Option to create a debug context
#ifdef _DEBUG
		if ( r_debugGLContext.GetBool() ) {
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
		}
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
		window = SDL_CreateWindow(GAME_NAME,
									SDL_WINDOWPOS_UNDEFINED,
									SDL_WINDOWPOS_UNDEFINED,
									parms.width, parms.height, flags);

		if (!window) {
			common->DPrintf("Couldn't set GL mode %d/%d/%d: %s",
							channelcolorbits, tdepthbits, tstencilbits, SDL_GetError());
			continue;
		}

		context = SDL_GL_CreateContext(window);

		if (SDL_GL_SetSwapInterval(r_swapInterval.GetInteger()) < 0)
			common->Warning("SDL_GL_SWAP_CONTROL not supported");

		{ // blendo eric: get max resolution
			SDL_DisplayMode displayMode;
			SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
			glConfig.displayWidth = displayMode.w;
			glConfig.displayHeight = displayMode.h;
		}

		// SM: Build list of available video modes (for full screen)
		const float ASPECT_RATIO_DIFF = 0.2f; // Used to decide how much different aspect ratio is allowed
		const int MIN_WIDTH = 1024; // minimum width required
		const int MIN_HEIGHT = 720; // minimum height required
		const float ASPECT_RATIO_WIDESCREEN = 1.5f;
		float aspectRatio = glConfig.displayWidth / static_cast<float>(glConfig.displayHeight);
		displayVidModes.Clear();

		int displayIndex = SDL_GetWindowDisplayIndex(window);
		int numDisplayModes = SDL_GetNumDisplayModes(displayIndex);
		for (int i = 0; i < numDisplayModes; i++)
		{
			SDL_DisplayMode mode;
			SDL_GetDisplayMode(displayIndex, i, &mode);
			float modeRatio = mode.w / static_cast<float>(mode.h);
			if (mode.w >= MIN_WIDTH && mode.h >= MIN_HEIGHT && 
				(fabs(aspectRatio - modeRatio) < ASPECT_RATIO_DIFF || modeRatio >= ASPECT_RATIO_WIDESCREEN))
			{
				idStr resString = idStr::Format("%dx%d", mode.w, mode.h);
				displayVidModes.AddUnique(resString);
			}
		}

		SDL_GetWindowSize(window, &glConfig.vidWidth, &glConfig.vidHeight);

		SetSDLIcon(); // for SDL2  this must be done after creating the window

		glConfig.isFullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN;
		glConfig.isBorderless = (SDL_GetWindowFlags(window) & SDL_WINDOW_BORDERLESS) == SDL_WINDOW_BORDERLESS;
#else
		SDL_WM_SetCaption(GAME_NAME, GAME_NAME);

		SetSDLIcon(); // for SDL1.2  this must be done before creating the window

		if (SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger()) < 0)
			common->Warning("SDL_GL_SWAP_CONTROL not supported");

		window = SDL_SetVideoMode(parms.width, parms.height, colorbits, flags);
		if (!window) {
			common->DPrintf("Couldn't set GL mode %d/%d/%d: %s",
							channelcolorbits, tdepthbits, tstencilbits, SDL_GetError());
			continue;
		}

		glConfig.vidWidth = window->w;
		glConfig.vidHeight = window->h;

		glConfig.isFullscreen = (window->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN;
#endif

		common->Printf("Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		glConfig.displayFrequency = 0;

		break;
	}

	if (!window) {
		common->Warning("No usable GL mode found: %s", SDL_GetError());
		return false;
	}

	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms(glimpParms_t parms) {
	// Whatever we want to do, exit fullscreen first
	// (because if we are currently in fullscreen, we need to exit for display change)
	SDL_SetWindowFullscreen(window, 0);

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isBorderless = parms.borderless;
	if (glConfig.isFullscreen)
	{
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &mode);
		mode.w = glConfig.vidWidth;
		mode.h = glConfig.vidHeight;
		SDL_SetWindowDisplayMode(window, &mode);

		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	}
	else
	{
		SDL_SetWindowBordered(window,SDL_bool(!glConfig.isBorderless));
		SDL_SetWindowSize(window, glConfig.vidWidth, glConfig.vidHeight);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown() {
	common->DPrintf("Shutting down OpenGL subsystem\n");

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (context) {
		SDL_GL_DeleteContext(context);
		context = NULL;
	}

	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
#endif
}

/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers() {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GL_SwapWindow(window);
#else
	SDL_GL_SwapBuffers();
#endif
}

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma(unsigned short red[256], unsigned short green[256], unsigned short blue[256]) {
	if (!window) {
		common->Warning("GLimp_SetGamma called without window");
		return;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (SDL_SetWindowGammaRamp(window, red, green, blue))
#else
	if (SDL_SetGammaRamp(red, green, blue))
#endif
		common->Warning("Couldn't set gamma ramp: %s", SDL_GetError());
}

/*
=================
GLimp_ActivateContext
=================
*/
void GLimp_ActivateContext() {

}

/*
=================
GLimp_DeactivateContext
=================
*/
void GLimp_DeactivateContext() {

}

#include <dlfcn.h>
const char *Fixed_Pipeline_Functions[] = {
	"glAlphaFunc",
	"glColor4f",
	"glColorPointer",
	"glDisableClientState",
	"glEnableClientState",
	"glGetLightfv",
	"glGetMaterialfv",
	"glGetPointerv",
	"glGetTexEnvfv",
	"glGetTexEnviv",
	"glLightf",
	"glLightfv",
	"glLightModelf",
	"glLightModelfv",
	"glLoadIdentity",
	"glLoadMatrixf",
	"glLogicOp",
	"glMaterialf",
	"glMaterialfv",
	"glMatrixMode",
	"glMultMatrixf",
	"glNormal3f",
	"glPointSize",
	"glPopMatrix",
	"glPushMatrix",
	"glRotatef",
	"glScalef",
	"glShadeModel",
	"glTexCoordPointer",
	"glTexEnvf",
	"glTranslatef",
	"glVertexPointer",
	"glColor4ub",
	"glFogf",
	"glFogfv",
	"glMultiTexCoord4f",
	"glNormalPointer",
	NULL,
};

bool GLimp_IsFixedPipelineFunction(const char *name)
{
	const char **ptr = &Fixed_Pipeline_Functions[0];
	while(*ptr)
	{
		if(!strcmp(name, *ptr))
			return true;
		ptr++;
	}
	return false;
}

static GLExtension_t GLimp_GetProcAddress(const char *name) {
	// 1. skip OpenGLES1.1 fixed-pipeline functions
	if(GLimp_IsFixedPipelineFunction(name))
	{
		Sys_Printf("%s -> skip fixed pipeline\n", name);
		return NULL;
	}

	// 2. find directly
	auto ptr = (GLExtension_t)SDL_GL_GetProcAddress(name);
    if(ptr)
	{
		Sys_Printf("%s -> found: %p\n", name, ptr);
        return ptr;
	}

	// 3. find from self
	ptr = (GLExtension_t)dlsym(RTLD_DEFAULT, name);
	if(ptr)
	{
		Sys_Printf("%s -> using debug\n", name);
		return ptr;
	}

    return NULL;
}

/*
===================
GLimp_ExtensionPointer
===================
*/
intptr_t GLimp_glStubFunction() {
    return 0;
}
GLExtension_t GLimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

#if 1
	auto ptr = GLimp_GetProcAddress(name);
    if(ptr)
        return ptr;
#else

	auto ptr = (GLExtension_t)SDL_GL_GetProcAddress(name);
    if(ptr)
        return ptr;
#endif
    Sys_Printf("%s -> missing, using stub\n", name);
    return (GLExtension_t)&GLimp_glStubFunction;
}

void GLimp_GrabInput(int flags) {
	bool grab = flags & GRAB_ENABLE;

	if (grab && (flags & GRAB_REENABLE))
		grab = false;

	if (flags & GRAB_SETSTATE)
		grabbed = grab;

	if (in_nograb.GetBool())
		grab = false;

	if (!window) {
		common->Warning("GLimp_GrabInput called without window");
		return;
	}

	// <blendo> eric : only grab mouse if the window is actually in focus	
	if (!GLimp_WindowActive())
	{
		grab = false;
		flags = flags & ~GRAB_HIDECURSOR;
	}
	// </blendo>
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_ShowCursor(flags & GRAB_HIDECURSOR ? SDL_DISABLE : SDL_ENABLE);
	SDL_SetRelativeMouseMode((grab && (flags & GRAB_HIDECURSOR)) ? SDL_TRUE : SDL_FALSE);
	SDL_SetWindowGrab(window, grab ? SDL_TRUE : SDL_FALSE);
#else
	SDL_ShowCursor(flags & GRAB_HIDECURSOR ? SDL_DISABLE : SDL_ENABLE);
	SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
}

// blendo eric: added window active code workaround
bool GLimp_WindowActive() {
#if _WIN64
	SDL_SysWMinfo sysWmInfo;
	SDL_VERSION(&sysWmInfo.version);
	SDL_GetWindowWMInfo(window, &sysWmInfo);
	auto sdlWindow = sysWmInfo.info.win.window;
	auto wnd = GetForegroundWindow();
	return sdlWindow == wnd;
#else // this doesn't work atm, sdl isn't getting updated state during transitions
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
#endif
}

// blendo eric: delete frame buffers for res changes
void GLimp_ClearFrameBuffers()
{
	backEnd.frameBufferId = FrameBufferIds_t::FRAME_DEFAULT_BACKBUFFER;
	for (int fbIdx = 1; fbIdx < (int)FrameBufferIds_t::FRAME_MAX; fbIdx++)
	{
		if (backEnd.frameBuffersIdsAllocated[fbIdx] != 0)
		{
			qglBindFramebuffer(GL_FRAMEBUFFER, backEnd.frameBuffersIdsAllocated[fbIdx]);
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			qglBindFramebuffer(GL_FRAMEBUFFER, 0);
			qglDeleteFramebuffers(1, &backEnd.frameBuffersIdsAllocated[fbIdx]);
			qglDeleteTextures(1, &backEnd.frameBuffersTextureIdsAllocated[fbIdx]);
			
			if (backEnd.frameBuffersDepthIdsAllocated[fbIdx] != 0)
			{
				qglDeleteTextures(1, &backEnd.frameBuffersDepthIdsAllocated[fbIdx]);
			}

			if (backEnd.frameBuffersCustomMaskAllocated[fbIdx] != 0)
			{
				qglDeleteTextures(1, &backEnd.frameBuffersCustomMaskAllocated[fbIdx]);
			}

			if (backEnd.frameBuffersLightAllocated[fbIdx] != 0)
			{
				qglDeleteTextures(1, &backEnd.frameBuffersLightAllocated[fbIdx]);
			}
		}
		backEnd.frameBuffersIdsAllocated[fbIdx] = 0;
		backEnd.frameBuffersTextureIdsAllocated[fbIdx] = 0;
		backEnd.frameBuffersDepthIdsAllocated[fbIdx] = 0;
		backEnd.frameBuffersCustomMaskAllocated[fbIdx] = 0;
		backEnd.frameBuffersLightAllocated[fbIdx] = 0;
	}
}

#if !defined(WIN32) //karin: in win_main.cpp
/*
==============
Sys_IsWindowActive
==============
*/
bool Sys_IsWindowActive( void ) {
    return GLimp_WindowActive();
}
#endif
