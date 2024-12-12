// ref_gl

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../client/refresh/gl1/header/local.h"

// OpenGL attributes
int gl_format = 0x8888;
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
#define Q3E_POST_GL_INIT GLES_PostInit();

extern void GLES_PostInit(void);

#include "q3e/q3e_glimp.inc"

qboolean IsHighDPIaware = false;
static qboolean vsyncActive = false;

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

void RI_SetGLParms(int f, int msaa)
{
    gl_format = f;
    gl_msaa = msaa;
	printf( "[Harmattan]: RI_SetGLParms(0x%X, %d).\n", gl_format, msaa);
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
	vsyncActive = false;
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
