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
// quakedef.h -- primary header for client

#ifndef __QUAKEDEF_H__
#define __QUAKEDEF_H__

#include "../common/bothdefs.h"	//first thing included by ALL files.

//for msvc #pragma message lines
#if defined(_MSC_VER)
#define MSVC_LINE	__FILE__"("STRINGIFY(__LINE__)"):"
#define warningmsg(s) message(MSVC_LINE s)
#elif __GNUC__ >=4
#define warningmsg(s) message(s)
#endif

#ifdef MSVCDISABLEWARNINGS
//#pragma warning( disable : 4244 4127 4201 4214 4514 4305 4115 4018)
/*#pragma warning( disable : 4244)	//conversion from const double to float
#pragma warning( disable : 4305)	//truncation from const double to float
#pragma warning( disable : 4018)	//signed/unsigned mismatch... fix these?
#pragma warning( disable : 4706)	//assignment within conditional expression - watch for these in GCC where they can be fixed but still functional.
#pragma warning( disable : 4100)	//unreferenced formal parameter
#pragma warning( disable : 4201)	//nonstandard extension used : nameless struct/union
#pragma warning( disable : 4213)	//nonstandard extension used : cast on l-value
#pragma warning( disable : 4127)	//conditional expression is constant - fixme?
*/
#pragma warning( 4 : 4244)	//conversion from const double to float
#pragma warning( 4 : 4305)	//truncation from const double to float
#pragma warning( 4 : 4018)	//truncation from const double to float

#pragma warning( 2 : 4701)
#pragma warning(2:4132 4268)// const object not initialized

#pragma warning(2:4032)		// function arg has different type from declaration
#pragma warning(2:4092)		// 'sizeof' value too big
#pragma warning(2:4132 4268)// const object not initialized
//#pragma warning(2:4152)	// pointer conversion between function and data
#pragma warning(2:4239)		// standard doesn't allow this conversion
#pragma warning(2:4701)		// local variable used without being initialized
//#pragma warning(2:4706)	// if (a=b) instead of (if a==b)
#pragma warning(2:4709)		// comma in array subscript
#pragma warning(3:4061)		// not all enum values tested in switch statement
#pragma warning(3:4710)		// inline function was not inlined
#pragma warning(3:4121)		// space added for structure alignment
#pragma warning(3:4505)		// unreferenced local function removed
#pragma warning(3:4019)		// empty statement at global scope
//#pragma warning(3:4057)	// pointers refer to different base types
#pragma warning(3:4125)		// decimal digit terminates octal escape
#pragma warning(2:4131)		// old-style function declarator
#pragma warning(3:4211)		// extern redefined as static
//#pragma warning(3:4213)	// cast on left side of = is non-standard
#pragma warning(3:4222)		// member function at file scope shouldn't be static
#pragma warning(3:4234 4235)// keyword not supported or reserved for future
#pragma warning(3:4504)		// type ambiguous; simplify code
#pragma warning(3:4507)		// explicit linkage specified after default linkage
#pragma warning(3:4515)		// namespace uses itself
#pragma warning(3:4516 4517)// access declarations are deprecated
#pragma warning(3:4670)		// base class of thrown object is inaccessible
#pragma warning(3:4671)		// copy ctor of thrown object is inaccessible
#pragma warning(3:4673)		// thrown object cannot be handled in catch block
#pragma warning(3:4674)		// dtor of thrown object is inaccessible
#pragma warning(3:4705)		// statement has no effect (example: a+1;)

#pragma warning(3:4013)		// function undefined, assuming extern returning int


#pragma warning( 4 : 4267)	//truncation from const double to float
#pragma warning( 4 : 4710)	//function not inlined

#pragma warning( error : 4020)

#pragma warning(error:4013)

//msvc... shut the fuck up.
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif


#define QUAKEDEF_H__

#ifdef __linux__
#define PNG_SUCKS_WITH_SETJMP	//cos it does.
#endif

//define	PARANOID			// speed sapping error checking

#include <float.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(AVAIL_PNGLIB) && defined(PNG_SUCKS_WITH_SETJMP) && !defined(SERVERONLY)
	#if defined(MINGW)
		#include "./mingw-libs/png.h"
	#elif defined(_WIN32)
		#include "png.h"
	#else
		#include <png.h>
	#endif
#else
	#ifdef FTE_TARGET_WEB
		#include "web/ftejslib.h"
		//officially, emscripten supports longjmp.
		//unofficially, this makes firefox crash with memory issues.
		#define setjmp(x) (x=0,x)
		#define longjmp(b,r) emscriptenfte_abortmainloop(__func__, false)
		typedef int jmp_buf;
	#else
		#include <setjmp.h>
	#endif
#endif
#include <time.h>

#ifdef USE_MSVCRT_DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif
#if defined(_WIN32) || defined(__DJGPP__)
	#include <malloc.h>
#elif defined(__unix__) && !defined(__linux__) // quick hack for the bsds and other unix systems
	#include<stdlib.h>
#elif !defined(alloca)	//alloca.h isn't present on bsd (stdlib.h should define it to __builtin_alloca, and we can check for that here).
	#include <alloca.h>
#endif

#ifdef FTE_TARGET_WEB //emscripten's filesystem is throwing all sorts of exceptions and making it hard to debug real bugs.
	#define NOSTDIO
#endif
#ifdef NOSTDIO
	#define stat stat_nolink
	#define fopen fopen_nolink
	#define fread fread_nolink
	#define fwrite fwrite_nolink
	#define fclose fclose_nolink
	#define fseek fseek_nolink
	#define open open_nolink
	#define read read_nolink
	#define write write_nolink
	#define close close_nolink
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/common.h"
#include "../common/bspfile.h"
#include "vid.h"
#include "../common/sys.h"
#include "../common/zone.h"
#include "../common/mathlib.h"
#include "../common/cvar.h"
#include "../common/translate.h"
#include "../common/net.h"
#ifndef WEBSVONLY
#include "../common/protocol.h"
#include "../common/cmd.h"
#include "../common/console.h"
#include "screen.h"
#include "wad.h"
#include "sbar.h"
#include "sound.h"
#include "merged.h"
#include "render.h"
#include "client.h"
#include "../gl/gl_model.h"

#include "../common/vm.h"

#include "input.h"
#include "keys.h"
#include "view.h"
#include "menu.h"
#include "cdaudio.h"
#include "../common/pmove.h"

#include "../qclib/progtype.h"
#include "../server/progdefs.h"
#include "../server/progs.h"
#include "../common/world.h"
#include "../server/q2game.h"
#include "../http/iweb.h"
#ifdef CLIENTONLY
#define SSV_IsSubServer() false
#else
#include "../server/server.h"
#endif
#endif

#if defined(Q3CLIENT) || defined(Q3SERVER)
#include "../common/q3api.h"
#endif

#ifdef __cplusplus
	#define q_max(a,b) ((a) > (b) ? (a) : (b))
	#define q_min(a,b) ((a) < (b) ? (a) : (b))
#else
	#ifndef max
		#define max(a,b) ((a) > (b) ? (a) : (b))
		#define min(a,b) ((a) < (b) ? (a) : (b))
	#endif
	#define max3(a,b,c) max(max(a,b),c)
#endif

//msvcrt lacks any and all c99 support.
#if defined(_WIN32)
	#ifdef __GNUC__
		#include <inttypes.h>
	#else
		#define PRIxPTR "p"
		//totally different from any other system
		#define PRIx64 "I64x"
		#define PRIu64 "I64u"
		#define PRIi64 "I64i"
	#endif

	#ifdef _WIN64
		#define PRIxSIZE PRIx64
		#define PRIuSIZE PRIu64
		#define PRIiSIZE PRIi64
	#else
		//don't use I, for the sake of older libcs
		#define PRIxSIZE "x"
		#define PRIuSIZE "u"
		#define PRIiSIZE "i"
	#endif
#else
	#include <inttypes.h>
	//these are non-standard. c99 would expect people to just use %zx etc
	#if FTE_WORDSIZE != 32 || __STDC_VERSION__ >= 199901L || defined(__GNUC__)
		//64bit systems are expected to have an awareness of c99
		#define PRIxSIZE "zx"
		#define PRIuSIZE "zu"
		#define PRIiSIZE "zi"
	#else
		//regular old c89 for 32bit platforms.
		#define PRIxSIZE PRIxPTR
		#define PRIuSIZE PRIuPTR
		#define PRIiSIZE PRIiPTR
	#endif
#endif


#ifdef _WIN32
	#if (_MSC_VER >= 1900)
		// MSVC 14 has standardized snprintf functions, hurrah!
	#elif (_MSC_VER >= 1400)
		//with MSVC 8, use microsoft's vsnprintf_s. return values are still wrong though.
		#define snprintf (void)linuxlike_snprintf_vc8
		int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...) LIKEPRINTF(3);
		#define vsnprintf(a, b, c, d) (void)(vsnprintf_s(a, b, _TRUNCATE, c, d))
	#else
		//msvc crap. return values are wrong but at least we can null terminate it safely.
		#define snprintf (void)linuxlike_snprintf
		int VARGS linuxlike_snprintf(char *buffer, int size, const char *format, ...) LIKEPRINTF(3);
		#define vsnprintf (void)linuxlike_vsnprintf
		int VARGS linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr);
	#endif

	#ifdef _MSC_VER
		//these are provided so we don't use them
		//but mingw has some defines elsewhere and makes gcc moan
//		#define _vsnprintf unsafe_vsnprintf
//		#define _snprintf unsafe_snprintf

		#ifndef strcasecmp
			#define strcasecmp _stricmp
		#endif
		#ifndef strncasecmp
			#define strncasecmp _strnicmp
		#endif
	#endif
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct quakeparms_s
{
	const char	*basedir;	//working directory
	const char	*binarydir;	//exe directory
	const char	*manifest;	//linked manifest data (for installer functionality etc)
	int			argc;
	const char	**argv;
} quakeparms_t;


//=============================================================================

#define MAX_NUM_ARGVS	128


extern qboolean noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		fs_gamename;
#ifdef PACKAGEMANAGER
extern	cvar_t		pkg_autoupdate;
#endif
extern	cvar_t		com_protocolname;
extern	cvar_t		com_protocolversion;
extern	cvar_t		com_gamedirnativecode;
extern	cvar_t		com_parseutf8;
#ifdef HAVE_LEGACY
extern	cvar_t		scr_usekfont;
extern	cvar_t		ezcompat_markup;
#endif
extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;
extern	cvar_t		host_mapname;

extern	cvar_t	password;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	qbyte		*host_basepal;
extern	qbyte		*h2playertranslations;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_FinishInit(void);
void Host_Shutdown(void);
NORETURN void VARGS Host_Error (const char *error, ...) LIKEPRINTF(1);
NORETURN void VARGS Host_EndGame (const char *message, ...) LIKEPRINTF(1);
qboolean Host_SimulationTime(float time);
double Host_Frame (double time);
qboolean Host_RunFile(const char *fname, int nlen, vfsfile_t *file);
void Host_Quit_f (void);
void VARGS Host_ClientCommands (char *fmt, ...) LIKEPRINTF(1);
void Host_ShutdownServer (qboolean crash);

#ifdef LOADERTHREAD
extern qboolean com_workererror;	//supresses shutdown prints+threads
extern cvar_t worker_flush;
qboolean COM_DoWork(int thread, qboolean leavelocked);
#define COM_MainThreadWork() while (COM_DoWork(0, false) && worker_flush.ival) /*called each frame to do any gl uploads or whatever*/
#define COM_MainThreadFlush() while (COM_DoWork(0, false))	/*make sure the main thread has done ALL work pending*/
unsigned int COM_HasWorkers(wgroup_t tg);
void COM_AddWork(wgroup_t thread, void(*func)(void *ctx, void *data, size_t a, size_t b), void *ctx, void *data, size_t a, size_t b);	//low priority
void COM_InsertWork(wgroup_t tg, void(*func)(void *ctx, void *data, size_t a, size_t b), void *ctx, void *data, size_t a, size_t b);	//high priority
qboolean COM_HasWork(void);
void COM_WorkerFullSync(void);
void COM_WorkerLock(void);	//callable on main thread to temporarily suspend workers (in a safe location)
void COM_WorkerUnlock(void);
void COM_DestroyWorkerThread(void);
void COM_WorkerPartialSync(void *priorityctx, int *address, int value); //aka: while(*address==value)wait();
extern void *com_resourcemutex;	//random mutex to simplify resource creation type stuff.
void COM_WorkerAbort(char *message);	//calls sys_error on the main thread, if running on a worker.
#ifdef _DEBUG
void COM_AssertMainThread(const char *msg);
#else
#define COM_AssertMainThread(msg)
#endif
#else
#define com_workererror false
#define COM_HasWorkers(t) 0
#define COM_AddWork(t,f,a,b,c,d) (f)((a),(b),(c),(d))
#define COM_InsertWork(t,f,a,b,c,d) (f)((a),(b),(c),(d))
#define COM_WorkerPartialSync(c,a,v)
#define COM_WorkerFullSync()
#define COM_WorkerLock()
#define COM_WorkerUnlock()
#define COM_HasWork() false
#define COM_DoWork(t,l) false
#define COM_AssertMainThread(msg)
#define COM_MainThreadWork() while(0)
#define COM_MainThreadFlush()
#define COM_DestroyWorkerThread()
#define COM_WorkerAbort(m)
#endif

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss

#ifndef HAVE_CLIENT
#define isDedicated true
#elif !defined(HAVE_SERVER)
#define isDedicated false
#else
extern qboolean isDedicated;
#endif
extern qboolean wantquit;	//flagged if we want to force a quit, safely breaking out of any modal stuff



#ifdef __cplusplus
}
#endif

#endif //__QUAKEDEF_H__
