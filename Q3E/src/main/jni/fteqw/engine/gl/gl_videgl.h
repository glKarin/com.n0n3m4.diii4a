#ifndef __GL_VIDEGL_H__
#define __GL_VIDEGL_H__

#include "quakedef.h"
#define NativeWindowType EGLNativeWindowType	//for old egl versions
#include <EGL/egl.h>
#ifndef _WIN32
	#include <dlfcn.h>
#endif

#ifndef EGL_PLATFORM_X11_KHR
#define EGL_PLATFORM_X11_KHR                    0x31D5	//EGL_KHR_platform_x11
#define EGL_PLATFORM_X11_SCREEN_KHR             0x31D6	//an attrib
#endif

#ifndef EGL_PLATFORM_WAYLAND_KHR
#define EGL_PLATFORM_WAYLAND_KHR                0x31D8	//EGL_KHR_platform_wayland
#endif

#ifndef EGL_PLATFORM_WIN32
#define EGL_PLATFORM_WIN32						0		//no meaningful value.
#endif

#ifndef EGL_PLATFORM_DEVICE_EXT
#define EGL_PLATFORM_DEVICE_EXT					0x313F	//we're using it for headless/pbuffer. oh well.
#endif

void *EGL_Proc(char *f);
void EGL_UnloadLibrary(void);
qboolean EGL_LoadLibrary(char *driver);
void EGL_Shutdown(void);
void EGL_SwapBuffers (void);

qboolean EGL_InitDisplay (rendererstate_t *info, int eglplat, void *ndpy, EGLNativeDisplayType dpyid, EGLConfig *outconfig);
qboolean EGL_InitWindow (rendererstate_t *info, int eglplat, void *nwindow, EGLNativeWindowType windowid, EGLConfig cfg);
//qboolean EGL_Init (rendererstate_t *info, unsigned char *palette, int eglplatform, void *nwindow, void *ndpy, EGLNativeWindowType owindow, EGLNativeDisplayType odpy);

//once you've created an egl display and got an egl config, some windowing systems require querying said egl config to create the window properly.
extern EGLDisplay egldpy;
extern EGLBoolean	(EGLAPIENTRY *qeglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);

#endif
