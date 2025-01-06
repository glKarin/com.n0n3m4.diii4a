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

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
		Q3E_RestoreGL();
}

void GLimp_AndroidQuit(void)
{
	Q3E_DestroyGL(true);
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

bool GLimp_InitGL(void)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_ES_3_0();

	bool res = Q3E_InitGL();
	return res;
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
	return 3.00f;
}

float GLimp_GetGLVersion(void)
{
	return 3.0f;
}
