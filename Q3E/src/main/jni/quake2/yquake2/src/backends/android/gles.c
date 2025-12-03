/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016 Daniel Gibson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * SDL backend for the GL1 renderer.
 *
 * =======================================================================
 */
 
// ref_gl

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../client/refresh/gl1/header/local.h"

// OpenGL attributes
int gl_format = 0x8888;
int gl_depth_bits = 24;
int gl_msaa = 0;
int screen_width = 640;
int screen_height = 480;
int refresh_rate = 60;

#define Q3E_PRINTF Com_Printf
#define Q3E_ERRORF(...) ri.Sys_Error(ERR_FATAL, __VA_ARGS__)
#define Q3E_DEBUGF printf
#define Q3Ebool qboolean
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_POST_GL_INIT GLES_PostInit();

extern void GLES_PostInit(void);

#include "q3e/q3e_glimp.inc"

qboolean IsHighDPIaware = false;
static qboolean vsyncActive = false;
extern cvar_t *gl1_discardfb;

void GLimp_ActivateContext()
{
	Q3E_ActivateContext();
}

void GLimp_DeactivateContext()
{
	Q3E_DeactivateContext();
}

#pragma GCC visibility push(default)
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
===============
GLES_Init
===============
*/
void GLES_PostInit(void)
{
	gl_state.fullscreen = true;

	if (gl_state.fullscreen) {
		//Sys_GrabMouseCursor(true);
	}
}

qboolean GLimp_InitGL(qboolean fullscreen)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(swap_interval, -1);
	Q3E_GL_CONFIG_ES_1_1();

	int multiSamples = gl_msaa_samples->value;
	if (multiSamples > 0)
	{
		multiSamples /= 2;
	}

	Q3E_GL_CONFIG_SET(samples, multiSamples);

	qboolean res = Q3E_InitGL();

	return res;
}

// ----

/*
 * Swaps the buffers and shows the next frame.
 */
void
RI_EndFrame(void)
{
	R_ApplyGLBuffer();	// to draw buffered 2D text

#ifdef YQ2_GL1_GLES
	static const GLenum attachments[3] = {GL_COLOR_EXT, GL_DEPTH_EXT, GL_STENCIL_EXT};

	if (qglDiscardFramebufferEXT)
	{
		switch ((int)gl1_discardfb->value)
		{
			case 1:
				qglDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 3, &attachments[0]);
				break;
			case 2:
				qglDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 2, &attachments[1]);
				break;
			default:
				break;
		}
	}
#endif

	Q3E_SwapBuffers();
}

/*
 * Returns the adress of a GL function
 */
void *
RI_GetProcAddress(const char* proc)
{
	return Q3E_GET_PROC_ADDRESS(proc);
}

/*
 * Returns whether the vsync is enabled.
 */
qboolean RI_IsVSyncActive(void)
{
	return vsyncActive;
}

/*
 * This function returns the flags used at the SDL window
 * creation by GLimp_InitGraphics(). In case of error -1
 * is returned.
 */
int RI_PrepareForWindow(void)
{
	gl_state.stencil = true;
	// ri.Cvar_SetValue("r_msaa_samples", gl_multiSamples);

	return 1;
}

/*
 * Enables or disables the vsync.
 */
void RI_SetVsync(void)
{
	// Make sure that the user given
	// value is SDL compatible...
	int vsync = 0;

	if (r_vsync->value == 1)
	{
		vsync = 1;
	}
	else if (r_vsync->value == 2)
	{
		vsync = -1;
	}

	if (!Q3E_SwapInterval(vsync))
	{
		if (vsync == -1)
		{
			vsync = 1;
			// Not every system supports adaptive
			// vsync, fallback to normal vsync.
			Com_Printf("Failed to set adaptive vsync, reverting to normal vsync.\n");
			Q3E_SwapInterval(1);
		}
	}

	vsyncActive = vsync != 0 ? true : false;
}

/*
 * Updates the gamma ramp.
 */
void
RI_UpdateGamma(void)
{
}

/*
 * Initializes the OpenGL context. Returns true at
 * success and false at failure.
 */
int RI_InitContext(void* _win)
{
	win = _win;
	if(!GLimp_InitGL(true))
		return false;

	// Enable vsync if requested.
	RI_SetVsync();

	// Check if we've got 8 stencil bits.
	gl_state.stencil = true;

	// Initialize gamma.
	vid_gamma->modified = true;

#ifdef YQ2_GL1_GLES

	// Load GL pointers through GLAD and check context.
	if( !gladLoadGLES1Loader( (void * (*)(const char *)) Q3E_GET_PROC_ADDRESS ) )
	{
		Com_Printf("%s ERROR: loading OpenGL ES function pointers failed!\n", __func__);
		return false;
	}

	gl_config.major_version = GLVersion.major;
	gl_config.minor_version = GLVersion.minor;
	Com_Printf("Initialized OpenGL ES version %d.%d context\n", gl_config.major_version, gl_config.minor_version);

#endif

	return true;
}

/*
 * Fills the actual size of the drawable into width and height.
 */
void RI_GetDrawableSize(int* width, int* height)
{
	*width = screen_width;
	*height = screen_height;
}

/*
 * Shuts the GL context down.
 */
void
RI_ShutdownContext(void)
{
    GLimp_AndroidQuit();
}

/*
 * Returns the SDL major version. Implemented
 * here to not polute gl1_main.c with the SDL
 * headers.
 */
int RI_GetSDLVersion()
{
	return 2;
}
