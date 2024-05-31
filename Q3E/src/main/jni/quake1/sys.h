/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// sys.h -- non-portable functions

#ifndef SYS_H
#define SYS_H

#include "qtypes.h"
#include "qdefs.h"

/* Preprocessor macros to identify platform
    DP_OS_NAME 	- "friendly" name of the OS, for humans to read
    DP_OS_STR	- "identifier" of the OS, more suited for code to use
    DP_ARCH_STR	- "identifier" of the processor architecture
 */
#if defined(__ANDROID__) /* must come first because it also defines linux */
# define DP_OS_NAME		"Android"
# define DP_OS_STR		"android"
# define USE_GLES2		1
#ifdef _DIII4A
# define USE_RWOPS		0
#else
# define USE_RWOPS		1
#endif
# define LINK_TO_ZLIB	1
# define LINK_TO_LIBVORBIS 1
#ifdef USEXMP
# define LINK_TO_LIBXMP 1 // nyov: if someone can test with the android NDK compiled libxmp?
#endif
#ifdef _DIII4A
# define DP_MOBILETOUCH	0
#else
# define DP_MOBILETOUCH	1
#endif
# define DP_FREETYPE_STATIC 1
#elif defined(__linux__)
# define DP_OS_NAME		"Linux"
# define DP_OS_STR		"linux"
#elif defined(_WIN64)
# define DP_OS_NAME		"Windows64"
# define DP_OS_STR		"win64"
#elif defined(WIN32)
# define DP_OS_NAME		"Windows"
# define DP_OS_STR		"win32"
#elif defined(__FreeBSD__)
# define DP_OS_NAME		"FreeBSD"
# define DP_OS_STR		"freebsd"
#elif defined(__NetBSD__)
# define DP_OS_NAME		"NetBSD"
# define DP_OS_STR		"netbsd"
#elif defined(__OpenBSD__)
# define DP_OS_NAME		"OpenBSD"
# define DP_OS_STR		"openbsd"
#elif defined(__DragonFly__)
# define DP_OS_NAME		"DragonFlyBSD"
# define DP_OS_STR		"dragonflybsd"
#elif defined(__APPLE__)
# if TARGET_OS_IPHONE
#  define DP_OS_NAME "iOS"
#  define DP_OS_STR "ios"
#  define USE_GLES2              1
#  define LINK_TO_ZLIB   1
#  define LINK_TO_LIBVORBIS 1
#  define DP_MOBILETOUCH 1
#  define DP_FREETYPE_STATIC 1
# elif TARGET_OS_MAC
#  define DP_OS_NAME "macOS"
#  define DP_OS_STR "macos"
# endif 
#elif defined(__MORPHOS__)
# define DP_OS_NAME		"MorphOS"
# define DP_OS_STR		"morphos"
#elif defined (sun) || defined (__sun)
# if defined (__SVR4) || defined (__svr4__)
#  define DP_OS_NAME	"Solaris"
#  define DP_OS_STR		"solaris"
# else
#  define DP_OS_NAME	"SunOS"
#  define DP_OS_STR		"sunos"
# endif
#else
# define DP_OS_NAME		"Unknown"
# define DP_OS_STR		"unknown"
#endif

#if defined(__GNUC__) || (__clang__)
# if defined(__i386__)
#  define DP_ARCH_STR		"686"
#  define SSE_POSSIBLE
#  ifdef __SSE__
#   define SSE_PRESENT
#  endif
#  ifdef __SSE2__
#   define SSE2_PRESENT
#  endif
# elif defined(__x86_64__)
#  define DP_ARCH_STR		"x86_64"
#  define SSE_PRESENT
#  define SSE2_PRESENT
# elif defined(__powerpc__)
#  define DP_ARCH_STR		"ppc"
# endif
#elif defined(_WIN64)
# define DP_ARCH_STR		"x86_64"
# define SSE_PRESENT
# define SSE2_PRESENT
#elif defined(WIN32)
# define DP_ARCH_STR		"x86"
# define SSE_POSSIBLE
#endif

#ifdef SSE_PRESENT
# define SSE_POSSIBLE
#endif

#ifdef NO_SSE
# undef SSE_PRESENT
# undef SSE_POSSIBLE
# undef SSE2_PRESENT
#endif

#ifdef SSE_POSSIBLE
// runtime detection of SSE/SSE2 capabilities for x86
qbool Sys_HaveSSE(void);
qbool Sys_HaveSSE2(void);
#else
#define Sys_HaveSSE() false
#define Sys_HaveSSE2() false
#endif

typedef struct sys_s
{
	int argc;
	const char **argv;
	int selffd;
	int outfd;
	int nicelevel;
	qbool nicepossible;
	qbool isnice;
} sys_t;

extern sys_t sys;


//
// DLL management
//

// Win32 specific
#ifdef WIN32
# include <windows.h>
typedef HMODULE dllhandle_t;

// Other platforms
#else
  typedef void* dllhandle_t;
#endif

typedef struct dllfunction_s
{
	const char *name;
	void **funcvariable;
}
dllfunction_t;

qbool Sys_LoadSelf(dllhandle_t *handle);

/*! Loads a dependency library. 
 * \param dllnames a NULL terminated array of possible names for the DLL you want to load.
 * \param handle
 * \param fcts
 */
qbool Sys_LoadDependency (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts);

/*! Loads a library.
 * \param name a string of the library filename
 * \param handle
 * \return true if library was loaded successfully
 */
qbool Sys_LoadLibrary(const char *name, dllhandle_t *handle);

void Sys_FreeLibrary (dllhandle_t* handle);
void* Sys_GetProcAddress (dllhandle_t handle, const char* name);

int Sys_CheckParm (const char *parm);

/// called after command system is initialized but before first Con_Print
void Sys_Init_Commands (void);


/// \returns current timestamp
char *Sys_TimeString(const char *timeformat);

//
// system IO interface (these are the sys functions that need to be implemented in a new driver atm)
//

/// Causes the entire program to exit ASAP.
/// Trailing \n should be omitted.
void Sys_Error (const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;

/// (may) output text to terminal which launched program
/// is POSIX async-signal-safe
/// textlen excludes any (optional) \0 terminator
void Sys_Print(const char *text, size_t textlen);
/// used to report failures inside Con_Printf()
void Sys_Printf(const char *fmt, ...);

/// INFO: This is only called by Host_Shutdown so we dont need testing for recursion
void Sys_SDL_Shutdown(void);

/*! on some build/platform combinations (such as Linux gcc with the -pg
 * profiling option) this can turn on/off profiling, used primarily to limit
 * profiling to certain areas of the code, such as ingame performance without
 * regard for loading/shutdown performance (-profilegameonly on commandline)
 */
#ifdef __cplusplus
extern "C"
#endif
void Sys_AllowProfiling (qbool enable);

typedef struct sys_cleantime_s
{
	double dirtytime; // last value gotten from Sys_DirtyTime()
	double cleantime; // sanitized linearly increasing time since app start
}
sys_cleantime_t;

double Sys_DirtyTime(void);

void Sys_ProvideSelfFD (void);

/// Reads a line from POSIX stdin or the Windows console
char *Sys_ConsoleInput (void);

/// called to yield for a little bit so as not to hog cpu when paused or debugging
double Sys_Sleep(double time);

void Sys_SDL_Dialog(const char *title, const char *string);
void Sys_SDL_Init(void);
/// Perform Key_Event () callbacks until the input que is empty
void Sys_SDL_HandleEvents(void);

char *Sys_SDL_GetClipboardData (void);

extern qbool sys_supportsdlgetticks;
unsigned int Sys_SDL_GetTicks (void); // wrapper to call SDL_GetTicks
void Sys_SDL_Delay (unsigned int milliseconds); // wrapper to call SDL_Delay

/// called to set process priority for dedicated servers
void Sys_InitProcessNice (void);
void Sys_MakeProcessNice (void);
void Sys_MakeProcessMean (void);

int Sys_Main(int argc, char *argv[]);

#endif

