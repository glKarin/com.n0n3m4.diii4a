// ref_gl3

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../client/refresh/gl3/header/local.h"

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

static qboolean vsyncActive = false;
qboolean IsHighDPIaware = false;

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
	gl3state.fullscreen = true;

	if (gl3state.fullscreen) {
		//Sys_GrabMouseCursor(true);
	}
}

qboolean GLimp_InitGL(qboolean fullscreen)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(debug_output, 1);
	Q3E_GL_CONFIG_ES_3_2();

	int multiSamples = gl_msaa_samples->value;
	if (multiSamples > 0)
	{
		multiSamples /= 2;
	}

	Q3E_GL_CONFIG_SET(samples, multiSamples);

	qboolean res = Q3E_InitGL();

	return res;
}

// --------

enum {
	// Not all GL.h header know about GL_DEBUG_SEVERITY_NOTIFICATION_*.
	// DG: yes, it's the same value in GLES3.2
	QGL_DEBUG_SEVERITY_NOTIFICATION = 0x826B
};

/*
 * Callback function for debug output.
 */
static void APIENTRY
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
              const GLchar *message, const void *userParam)
{
	const char* sourceStr = "Source: Unknown";
	const char* typeStr = "Type: Unknown";
	const char* severityStr = "Severity: Unknown";

	switch (severity)
	{
#ifdef YQ2_GL3_GLES
  #define SVRCASE(X, STR)  case GL_DEBUG_SEVERITY_ ## X ## _KHR : severityStr = STR; break;
#else // Desktop GL
  #define SVRCASE(X, STR)  case GL_DEBUG_SEVERITY_ ## X ## _ARB : severityStr = STR; break;
#endif

		case QGL_DEBUG_SEVERITY_NOTIFICATION: return;
		SVRCASE(HIGH, "Severity: High")
		SVRCASE(MEDIUM, "Severity: Medium")
		SVRCASE(LOW, "Severity: Low")
#undef SVRCASE
	}

	switch (source)
	{
#ifdef YQ2_GL3_GLES
  #define SRCCASE(X)  case GL_DEBUG_SOURCE_ ## X ## _KHR: sourceStr = "Source: " #X; break;
#else
  #define SRCCASE(X)  case GL_DEBUG_SOURCE_ ## X ## _ARB: sourceStr = "Source: " #X; break;
#endif
		SRCCASE(API);
		SRCCASE(WINDOW_SYSTEM);
		SRCCASE(SHADER_COMPILER);
		SRCCASE(THIRD_PARTY);
		SRCCASE(APPLICATION);
		SRCCASE(OTHER);
#undef SRCCASE
	}

	switch(type)
	{
#ifdef YQ2_GL3_GLES
  #define TYPECASE(X)  case GL_DEBUG_TYPE_ ## X ## _KHR: typeStr = "Type: " #X; break;
#else
  #define TYPECASE(X)  case GL_DEBUG_TYPE_ ## X ## _ARB: typeStr = "Type: " #X; break;
#endif
		TYPECASE(ERROR);
		TYPECASE(DEPRECATED_BEHAVIOR);
		TYPECASE(UNDEFINED_BEHAVIOR);
		TYPECASE(PORTABILITY);
		TYPECASE(PERFORMANCE);
		TYPECASE(OTHER);
#undef TYPECASE
	}

	// use PRINT_ALL - this is only called with gl3_debugcontext != 0 anyway.
	R_Printf(PRINT_ALL, "GLDBG %s %s %s: %s\n", sourceStr, typeStr, severityStr, message);
}

// ---------

/*
 * Swaps the buffers and shows the next frame.
 */
void GL3_EndFrame(void)
{
	if(gl3config.useBigVBO)
	{
		// I think this is a good point to orphan the VBO and get a fresh one
		GL3_BindVAO(gl3state.vao3D);
		GL3_BindVBO(gl3state.vbo3D);
		glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW);
		gl3state.vbo3DcurOffset = 0;
	}

	Q3E_SwapBuffers();
}

/*
 * Returns whether the vsync is enabled.
 */
qboolean GL3_IsVsyncActive(void)
{
	return vsyncActive;
}

/*
 * Enables or disables the vsync.
 */
void GL3_SetVsync(void)
{
	vsyncActive = false;
}

/*
 * This function returns the flags used at the SDL window
 * creation by GLimp_InitGraphics(). In case of error -1
 * is returned.
 */
int GL3_PrepareForWindow(void)
{
	gl3config.stencil = true;
	// ri.Cvar_SetValue("r_msaa_samples", gl_multiSamples);

	return 1;
}

/*
 * Initializes the OpenGL context. Returns true at
 * success and false at failure.
 */
int GL3_InitContext(void* _win)
{
	win = _win;
	if(!GLimp_InitGL(true))
        return false;

	// Check if we've got 8 stencil bits.
	gl3config.stencil = true;

	// Enable vsync if requested.
	GL3_SetVsync();

	// Load GL pointers through GLAD and check context.
	if( !gladLoadGLES2Loader((void *)Q3E_GET_PROC_ADDRESS))
	{
		R_Printf(PRINT_ALL, "GL3_InitContext(): ERROR: loading OpenGL function pointers failed!\n");

		return false;
	}

	gl3config.debug_output = Q3E_GL_CONFIG(debug_output);
	gl3config.anisotropic = false;

	gl3config.major_version = Q3E_GL_CONFIG(major_version);
	gl3config.minor_version = Q3E_GL_CONFIG(minor_version);

	// Debug context setup.
	if (gl3_debugcontext && gl3_debugcontext->value && gl3config.debug_output)
	{
		glDebugMessageCallbackKHR(DebugCallback, NULL);

		// Call GL3_DebugCallback() synchronously, i.e. directly when and
		// where the error happens (so we can get the cause in a backtrace)
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
	}

	// Initialize gamma.
	vid_gamma->modified = true;

	return true;
}

/*
 * Fills the actual size of the drawable into width and height.
 */
void GL3_GetDrawableSize(int* width, int* height)
{
	*width = screen_width;
	*height = screen_height;
}

/*
 * Shuts the GL context down.
 */
void GL3_ShutdownContext()
{
	GLimp_AndroidQuit();
}

/*
 * Returns the SDL major version. Implemented
 * here to not polute gl3_main.c with the SDL
 * headers.
 */
int GL3_GetSDLVersion()
{
	return 2;
}
