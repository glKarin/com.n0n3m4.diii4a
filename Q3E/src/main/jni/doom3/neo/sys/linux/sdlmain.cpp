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

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <SDL_main.h>

#include "../platform.h"
#include "../../framework/Licensee.h"
#include "../../framework/FileSystem.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"
#include "../sys_public.h"

#include <locale.h>



#undef snprintf // no, I don't want to use idStr::snPrintf() here.

// lots of code following to get the current executable dir, taken from Yamagi Quake II
// and actually based on DG_Snippets.h

#if defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <unistd.h> // readlink(), amongst others
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#include <sys/sysctl.h> // for sysctl() to get path to executable
#endif

#ifdef _WIN32
#include <windows.h> // GetModuleFileNameA()
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifdef __HAIKU__
#include <FindDirectory.h>
#endif

#ifndef PATH_MAX
// this is mostly for windows. windows has a MAX_PATH = 260 #define, but allows
// longer paths anyway.. this might not be the maximum allowed length, but is
// hopefully good enough for realistic usecases
#define PATH_MAX 4096
#endif

#define D3_snprintfC99 SDL_snprintf

#ifdef _RAVEN //karin: win log file name
#define GAME_NAME_ID "quake4"
#elif defined(_HUMANHEAD)
#define GAME_NAME_ID "prey"
#else
#define GAME_NAME_ID "doom3"
#endif

static char path_argv[PATH_MAX];
static char path_exe[PATH_MAX];
static char save_path[PATH_MAX];

// in main.cpp
/*
===============
Sys_GetProcessorString
===============
*/
const char *Sys_GetProcessorString( void ) {
	return "generic";
}

/*
==============
Sys_EXEPath
==============
*/
const char *Sys_EXEPath( void ) {
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf( linkpath, "/proc/%d/exe", getpid() );
	len = readlink( linkpath.c_str(), buf, sizeof( buf ) );
	if ( len == -1 ) {
		Sys_Printf("couldn't stat exe path link %s\n", linkpath.c_str());
		buf[ len ] = '\0';
	}
	return buf;
}

const char * Sys_DLLDefaultPath(void)
{
	return "./";
}

/*
 ==============
 Sys_DefaultSavePath
 ==============
 */
const char *Sys_DefaultSavePath(void) {
	return save_path;
}

static idStr	basepath;
/*
================
Sys_DefaultBasePath

Get the default base path
- binary image path
- current directory
- hardcoded
Try to be intelligent: if there is no BASE_GAMEDIR, try the next path
================
*/
const char *Sys_DefaultBasePath(void) {
	struct stat st;
	idStr testbase;
	basepath = Sys_EXEPath();
	if ( basepath.Length() ) {
		basepath.StripFilename();
		testbase = basepath; testbase += "/"; testbase += BASE_GAMEDIR;
		if ( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) ) {
			return basepath.c_str();
		} else {
			common->Printf( "no '%s' directory in exe path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if ( basepath != Posix_Cwd() ) {
		basepath = Posix_Cwd();
		testbase = basepath; testbase += "/"; testbase += BASE_GAMEDIR;
		if ( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) ) {
			return basepath.c_str();
		} else {
			common->Printf("no '%s' directory in cwd path %s, skipping\n", BASE_GAMEDIR, basepath.c_str());
		}
	}
	common->Printf( "WARNING: using hardcoded default base path\n" );
#ifndef LINUX_DEFAULT_PATH
	#warning undefined data path
	return BASE_GAMEDIR;
#else
	return LINUX_DEFAULT_PATH;
#endif
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks( void ) {
	double t0, t1;

	t0 = Sys_GetClockTicks( );
	Sys_Sleep( 1000 );
	t1 = Sys_GetClockTicks( );	
	return t1 - t0;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond(void) {
	static bool		init = false;
	static double	ret;

	int		fd, len, pos, end;
	char	buf[ 4096 ];

	if ( init ) {
		return ret;
	}

	fd = open( "/proc/cpuinfo", O_RDONLY );
	if ( fd == -1 ) {
		common->Printf( "couldn't read /proc/cpuinfo\n" );
		ret = MeasureClockTicks();
		init = true;
		common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
		return ret;		
	}
	len = read( fd, buf, 4096 );
	close( fd );
	pos = 0;
	while ( pos < len ) {
		if ( !idStr::Cmpn( buf + pos, "cpu MHz", 7 ) ) {
			pos = strchr( buf + pos, ':' ) - buf + 2;
			end = strchr( buf + pos, '\n' ) - buf;
			if ( pos < len && end < len ) {
				buf[end] = '\0';
				ret = atof( buf + pos );
			} else {
				common->Printf( "failed parsing /proc/cpuinfo\n" );
				ret = MeasureClockTicks();
				init = true;
				common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
				return ret;		
			}
			common->Printf( "/proc/cpuinfo CPU frequency: %g MHz\n", ret );
			ret *= 1000000;
			init = true;
			return ret;
		}
		pos = strchr( buf + pos, '\n' ) - buf + 1;
	}
	common->Printf( "failed parsing /proc/cpuinfo\n" );
	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
	return ret;		
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks( void ) {
#if defined( __i386__ )
	unsigned long lo, hi;

	__asm__ __volatile__ (
						  "push %%ebx\n"			\
						  "xor %%eax,%%eax\n"		\
						  "cpuid\n"					\
						  "rdtsc\n"					\
						  "mov %%eax,%0\n"			\
						  "mov %%edx,%1\n"			\
						  "pop %%ebx\n"
						  : "=r" (lo), "=r" (hi) );
	return (double) lo + (double) 0xFFFFFFFF * hi;
#else
#warning unsupported CPU
	return 0;
#endif
}

/*
 ==================
 Sys_DoPreferences
 ==================
 */
void Sys_DoPreferences( void ) { }

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions( int exceptions ) {
}

idCVar sys_videoRam("sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER,
					"Texture memory on the video card (in megabytes) - 0: autodetect", 0, 512);
/*
================
Sys_GetVideoRam
returns in megabytes
open your own display connection for the query and close it
using the one shared with GLimp_Init is not stable
================
*/
int Sys_GetVideoRam(void)
{
	static int run_once = 0;
	int major, minor, value;

	if (run_once) {
		return run_once;
	}

	if (sys_videoRam.GetInteger()) {
		run_once = sys_videoRam.GetInteger();
		return sys_videoRam.GetInteger();
	}

	// try a few strategies to guess the amount of video ram
	common->Printf("guessing video ram ( use +set sys_videoRam to force ) ..\n");

	run_once = 512;
	return run_once;
}

extern char *Posix_ConsoleInput(void);
char *Sys_ConsoleInput( void ) {
	return Posix_ConsoleInput();
}

/*
===============
Sys_FPE_handler
===============
*/
void Sys_FPE_handler( int signum, siginfo_t *info, void *context ) {
	assert( signum == SIGFPE );
	Sys_Printf( "FPE\n" );
}

#include <pthread.h>

xthreadInfo asyncThread;

/*
=================
Posix_StartAsyncThread
=================
*/
void Posix_StartAsyncThread()
{
	if (asyncThread.threadHandle == 0) {
		Sys_CreateThread(Sys_AsyncThread, &asyncThread, THREAD_NORMAL, asyncThread, "Async", g_threads, &g_thread_count);
	} else {
		common->Printf("Async thread already running\n");
	}

	common->Printf("Async thread started\n");
}

/*
=================
Sys_AsyncThread
=================
*/
void * Sys_AsyncThread( void * ) {
	int now;
	int next;
	int	want_sleep;

	// multi tick compensate for poor schedulers (Linux 2.4)
	int ticked, to_ticked;
	now = Sys_Milliseconds();
	ticked = now >> 4;
	while (1) {
		// sleep
		now = Sys_Milliseconds();		
		next = ( now & 0xFFFFFFF0 ) + 0x10;
		want_sleep = ( next-now-1 ) * 1000;
		if ( want_sleep > 0 ) {
			usleep( want_sleep ); // sleep 1ms less than true target
		}
		
		// compensate if we slept too long
		now = Sys_Milliseconds();
		to_ticked = now >> 4;
		
		// show ticking statistics - every 100 ticks, print a summary
		#if 0
			#define STAT_BUF 100
			static int stats[STAT_BUF];
			static int counter = 0;
			// how many ticks to play
			stats[counter] = to_ticked - ticked;
			counter++;
			if (counter == STAT_BUF) {
				Sys_DebugPrintf("\n");
				for( int i = 0; i < STAT_BUF; i++) {
					if ( ! (i & 0xf) ) {
						Sys_DebugPrintf("\n");
					}
					Sys_DebugPrintf( "%d ", stats[i] );
				}
				Sys_DebugPrintf("\n");
				counter = 0;
			}
		#endif
		
		while ( ticked < to_ticked ) {
			common->Async();
			ticked++;
			Sys_TriggerEvent( TRIGGER_EVENT_ONE );
		}
		// thread exit
		pthread_testcancel();
	}
}


// end in main.cpp

const char* Posix_GetSavePath()
{
	return save_path;
}

static void SetSavePath()
{
	const char* s = getenv("XDG_DATA_HOME");
	if (s)
		D3_snprintfC99(save_path, sizeof(save_path), "%s/idtech4amm/" GAME_NAME_ID, s);
	else
		D3_snprintfC99(save_path, sizeof(save_path), "%s/.local/share/idtech4amm/" GAME_NAME_ID, getenv("HOME"));
}

const char* Posix_GetExePath()
{
	return path_exe;
}

static void SetExecutablePath(char* exePath)
{
	// !!! this assumes that exePath can hold PATH_MAX chars !!!

#ifdef _WIN32
	WCHAR wexePath[PATH_MAX];
	DWORD len;

	GetModuleFileNameW(NULL, wexePath, PATH_MAX);
	len = WideCharToMultiByte(CP_UTF8, 0, wexePath, -1, exePath, PATH_MAX, NULL, NULL);

	if(len <= 0 || len == PATH_MAX)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

#elif defined(__linux)

	// all the platforms that have /proc/$pid/exe or similar that symlink the
	// real executable - basiscally Linux and the BSDs except for FreeBSD which
	// doesn't enable proc by default and has a sysctl() for this. OpenBSD once
	// had /proc but removed it for security reasons.
	char buf[PATH_MAX] = {0};
	snprintf(buf, sizeof(buf), "/proc/%d/exe", getpid());
	// readlink() doesn't null-terminate!
	int len = readlink(buf, exePath, PATH_MAX-1);
	if (len <= 0)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}
	else
	{
		exePath[len] = '\0';
	}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

	// the sysctl should also work when /proc/ is not mounted (which seems to
	// be common on FreeBSD), so use it..
#if defined(__FreeBSD__) || defined(__DragonFly__)
	int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
#else
	int name[4] = {CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME};
#endif
	size_t len = PATH_MAX-1;
	int ret = sysctl(name, sizeof(name)/sizeof(name[0]), exePath, &len, NULL, 0);
	if(ret != 0)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

#elif defined(__APPLE__)

	uint32_t bufSize = PATH_MAX;
	if(_NSGetExecutablePath(exePath, &bufSize) != 0)
	{
		// WTF, PATH_MAX is not enough to hold the path?
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

	// TODO: realpath() ?
	// TODO: no idea what this is if the executable is in an app bundle
#elif defined(__HAIKU__)
	if (find_path(B_APP_IMAGE_SYMBOL, B_FIND_PATH_IMAGE_PATH, NULL, exePath, PATH_MAX) != B_OK)
	{
		exePath[0] = '\0';
	}

#else

	// Several platforms (for example OpenBSD) don't provide a
	// reliable way to determine the executable path. Just return
	// an empty string.
	exePath[0] = '\0';

// feel free to add implementation for your platform and send a pull request.
#warning "SetExecutablePath() is unimplemented on this platform"

#endif
}

bool Sys_GetPath(sysPath_t type, idStr &path) {
	const char *s;
	char buf[MAX_OSPATH];
	struct stat st;

	path.Clear();

	switch(type) {
	case PATH_BASE:
#if 0 //karin: setup in CMakeLists.txt set(datadir		"${CMAKE_INSTALL_FULL_DATADIR}/idtech4amm/" GAME_NAME_ID)
		if (stat(BUILD_DATADIR, &st) != -1 && S_ISDIR(st.st_mode)) {
			path = BUILD_DATADIR;
			return true;
		}

		common->Warning("base path '" BUILD_DATADIR "' does not exist");
#endif

		// try next to the executable..
		if (Sys_GetPath(PATH_EXE, path)) {
			path = path.StripFilename();
			// the path should have a base dir in it, otherwise it probably just contains the executable
			idStr testPath = path + "/" BASE_GAMEDIR;
			if (stat(testPath.c_str(), &st) != -1 && S_ISDIR(st.st_mode)) {
				common->Warning("using path of executable: %s", path.c_str());
				return true;
			} else {
				idStr testPath = path + "/demo/demo00.pk4";
				if(stat(testPath.c_str(), &st) != -1 && S_ISREG(st.st_mode)) {
					common->Warning("using path of executable (seems to contain demo game data): %s", path.c_str());
					return true;
				} else {
					path.Clear();
				}
			}
		}

		// fallback to vanilla doom3 install
		if (stat(LINUX_DEFAULT_PATH, &st) != -1 && S_ISDIR(st.st_mode)) {
			common->Warning("using hardcoded default base path: " LINUX_DEFAULT_PATH);

			path = LINUX_DEFAULT_PATH;
			return true;
		}

		return false;

	case PATH_CONFIG:
		s = getenv("XDG_CONFIG_HOME");
		if (s)
			idStr::snPrintf(buf, sizeof(buf), "%s/idtech4amm/" GAME_NAME_ID, s);
		else
			idStr::snPrintf(buf, sizeof(buf), "%s/.config/idtech4amm/" GAME_NAME_ID, getenv("HOME"));

		path = buf;
		return true;

	case PATH_SAVE:
		if(save_path[0] != '\0') {
			path = save_path;
			return true;
		}
		return false;

	case PATH_EXE:
		if (path_exe[0] != '\0') {
			path = path_exe;
			return true;
		}

		return false;
	}

	return false;
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown( void ) {
	Posix_Shutdown();
}

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam( void ) {
	long	count, page_size;
	int		mb;

	count = sysconf( _SC_PHYS_PAGES );
	if ( count == -1 ) {
		common->Printf( "GetSystemRam: sysconf _SC_PHYS_PAGES failed\n" );
		return 512;
	}
	page_size = sysconf( _SC_PAGE_SIZE );
	if ( page_size == -1 ) {
		common->Printf( "GetSystemRam: sysconf _SC_PAGE_SIZE failed\n" );
		return 512;
	}
	mb= (int)( (double)count * (double)page_size / ( 1024 * 1024 ) );
	// round to the nearest 16Mb
	mb = ( mb + 8 ) & ~15;
	return mb;
}

/*
==================
Sys_DoStartProcess
if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer

if the command contains spaces, system() is used. Otherwise the more straightforward execl ( system() blows though )
==================
*/
void Sys_DoStartProcess( const char *exeName, bool dofork ) {
	bool use_system = false;
	if ( strchr( exeName, ' ' ) ) {
		use_system = true;
	} else {
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if ( stat( exeName, &buf ) == -1 ) {
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		} else {
			if ( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 ) {
				printf( "cmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if ( dofork ) {
		switch ( fork() ) {
		case -1:
			printf( "fork failed: %s\n", strerror( errno ) );
			break;
		case 0:
			if ( use_system ) {
				printf( "system %s\n", exeName );
				if (system( exeName ) == -1)
					printf( "system failed: %s\n", strerror( errno ) );
				_exit( 0 );
			} else {
				printf( "execl %s\n", exeName );
				execl( exeName, exeName, NULL );
				printf( "execl failed: %s\n", strerror( errno ) );
				_exit( -1 );
			}
			break;
		default:
			break;
		}
	} else {
		if ( use_system ) {
			printf( "system %s\n", exeName );
			if (system( exeName ) == -1)
				printf( "system failed: %s\n", strerror( errno ) );
			else
				sleep( 1 );	// on some systems I've seen that starting the new process and exiting this one should not be too close
		} else {
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
}

/*
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL( const char *url, bool quit ) {
	const char	*script_path;
	idFile		*script_file;
	char		cmdline[ 1024 ];

	static bool	quit_spamguard = false;

	if ( quit_spamguard ) {
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), "", "openurl.sh" );
	script_file = fileSystem->OpenExplicitFileRead( script_path );
	if ( !script_file ) {
		script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_basepath" ), "", "openurl.sh" );
		script_file = fileSystem->OpenExplicitFileRead( script_path );
	}
	if ( !script_file ) {
		common->Printf( "Can't find URL script 'openurl.sh' in either savepath or basepath\n" );
		common->Printf( "OpenURL '%s' failed\n", url );
		return;
	}
	fileSystem->CloseFile( script_file );

	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if ( quit ) {
		quit_spamguard = true;
	}

	common->Printf( "URL script: %s\n", script_path );

	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf( cmdline, 1024, "%s '%s' &",  script_path, url );
	sys->StartProcess( cmdline, quit );
}


#include <termios.h>

static FILE* consoleLog = NULL;
void Sys_VPrintf(const char *msg, va_list arg);

extern idCVar in_tty;
static bool				tty_enabled = false;
static struct termios	tty_tc;

// ----------- lots of signal handling stuff ------------
static const int   crashSigs[]     = {  SIGILL,   SIGABRT,   SIGFPE,   SIGSEGV };
static const char* crashSigNames[] = { "SIGILL", "SIGABRT", "SIGFPE", "SIGSEGV" };

#if ( defined(__linux__) && defined(__GLIBC__) ) || defined(__FreeBSD__) || (defined(__APPLE__) && !defined(OSX_TIGER))
  #define D3_HAVE_BACKTRACE
  #include <execinfo.h>
#endif

// unlike Sys_Printf() this doesn't call tty_Hide(); and tty_Show();
// to minimize interaction with broken dhewm3 state
// (but unlike regular printf() it'll also write to dhewm3log.txt)
static void CrashPrintf(const char* msg, ...)
{
	va_list argptr;
	va_start( argptr, msg );
	Sys_VPrintf( msg, argptr );
	va_end( argptr );
}

#ifdef D3_HAVE_LIBBACKTRACE
// non-ancient versions of GCC and clang include libbacktrace
// for ancient versions it can be built from https://github.com/ianlancetaylor/libbacktrace
#include <backtrace.h>
#include <cxxabi.h> // for demangling C++ symbols

static struct backtrace_state *bt_state = NULL;

static void bt_error_callback( void *data, const char *msg, int errnum )
{
	CrashPrintf("libbacktrace ERROR: %d - %s\n", errnum, msg);
}

static void bt_syminfo_callback( void *data, uintptr_t pc, const char *symname,
								 uintptr_t symval, uintptr_t symsize )
{
	if (symname != NULL) {
		int status;
		// FIXME: sucks that __cxa_demangle() insists on using malloc().. but so does printf()
		char* name = abi::__cxa_demangle(symname, NULL, NULL, &status);
		if (name != NULL) {
			symname = name;
		}
		CrashPrintf("  %zu %s\n", pc, symname);
		free(name);
	} else {
		CrashPrintf("  %zu (unknown symbol)\n", pc);
	}
}

static int bt_pcinfo_callback( void *data, uintptr_t pc, const char *filename, int lineno, const char *function )
{
	if (data != NULL) {
		int* hadInfo = (int*)data;
		*hadInfo = (function != NULL);
	}

	if (function != NULL) {
		int status;
		// FIXME: sucks that __cxa_demangle() insists on using malloc()..
		char* name = abi::__cxa_demangle(function, NULL, NULL, &status);
		if (name != NULL) {
			function = name;
		}

		const char* fileNameNeo = strstr(filename, "/neo/");
		if (fileNameNeo != NULL) {
			filename = fileNameNeo+1; // I want "neo/bla/blub.cpp:42"
		}
		CrashPrintf("  %zu %s:%d %s\n", pc, filename, lineno, function);
		free(name);
	}

	return 0;
}

static void bt_error_dummy( void *data, const char *msg, int errnum )
{
	//CrashPrintf("ERROR-DUMMY: %d - %s\n", errnum, msg);
}

static int bt_simple_callback(void *data, uintptr_t pc)
{
	int pcInfoWorked = 0;
	// if this fails, the executable doesn't have debug info, that's ok (=> use bt_error_dummy())
	backtrace_pcinfo(bt_state, pc, bt_pcinfo_callback, bt_error_dummy, &pcInfoWorked);
	if (!pcInfoWorked) { // no debug info? use normal symbols instead
		// yes, it would be easier to call backtrace_syminfo() in bt_pcinfo_callback() if function == NULL,
		// but some libbacktrace versions (e.g. in Ubuntu 18.04's g++-7) don't call bt_pcinfo_callback
		// at all if no debug info was available - which is also the reason backtrace_full() can't be used..
		backtrace_syminfo(bt_state, pc, bt_syminfo_callback, bt_error_callback, NULL);
	}

	return 0;
}

#endif

static void signalhandlerCrash(int sig)
{
	const char* name = "";
	for(int i=0; i<sizeof(crashSigs)/sizeof(crashSigs[0]); ++i) {
		if(crashSigs[i] == sig)
			name = crashSigNames[i];
	}

	// TODO: should probably use a custom print function around write(STDERR_FILENO, ...)
	//       because printf() could allocate which is not good if processes state is fscked
	//       (could use backtrace_symbols_fd() then)
	CrashPrintf("\n\nLooks like %s crashed with signal %s (%d) - sorry!\n", ENGINE_VERSION, name, sig);

#ifdef D3_HAVE_LIBBACKTRACE
	if (bt_state != NULL) {
		int skip = 1; // skip this function in backtrace
		backtrace_simple(bt_state, skip, bt_simple_callback, bt_error_callback, NULL);
	} else {
		CrashPrintf("(No backtrace because libbacktrace state is NULL)\n");
	}
#elif defined(D3_HAVE_BACKTRACE)
	// this is partly based on Yamagi Quake II code
	void* array[128];
	int size = backtrace(array, sizeof(array)/sizeof(array[0]));
	char** strings = backtrace_symbols(array, size);

	CrashPrintf("\nBacktrace:\n");

	for(int i = 0; i < size; i++) {
		CrashPrintf("  %s\n", strings[i]);
	}

	CrashPrintf("\n(Sorry it's not overly useful, build with libbacktrace support to get function names)\n");

	free(strings);

#else
	CrashPrintf("(No Backtrace on this platform)\n");
#endif

	fflush(stdout);
	if(consoleLog != NULL) {
		fflush(consoleLog);
		// TODO: fclose(consoleLog); ?
		//       consoleLog = NULL;
	}

	raise(sig); // pass it on to system
}

static bool disableTTYinput = false;

static void signalhandlerConsoleStuff(int sig)
{
	if(sig == SIGTTIN) {
		// we get this if dhewm3 was started in foreground, then put to sleep with ctrl-z
		// and afterwards set to background..
		// as it's in background now, disable console input
		// (if someone uses fg afterwards that's their problem, this is already obscure enough)
		if(tty_enabled) {
			Sys_Printf( "Sent to background, disabling terminal support.\n" );
			in_tty.SetBool( false );
			tty_enabled = false;

			tcsetattr(0, TCSADRAIN, &tty_tc);

			// Note: this is only about TTY input, we'll still print to stdout
			// (which, I think, is normal for processes running in the background)
		}
	}

	// apparently we get SIGTTOU from tcsetattr() in Posix_InitConsoleInput()
	// so we'll handle the disabling console there (it checks for disableTTYinput)

	disableTTYinput = true;
}

static void installSigHandler(int sig, int flags, void (*handler)(int))
{
	struct sigaction sigact = {0};
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = flags;
	sigaction(sig, &sigact, NULL);
}

static bool dirExists(const char* dirPath)
{
	struct stat buf = {};
	if(stat(dirPath, &buf) == 0) {
		return (buf.st_mode & S_IFMT) == S_IFDIR;
	}
	return false;
}

static bool createPathRecursive(char* path)
{
	if(!dirExists(path)) {
		char* lastDirSep = strrchr(path, '/');
		if(lastDirSep != NULL) {
			*lastDirSep = '\0'; // cut off last part of the path and try first with parent directory
			bool ok = createPathRecursive(path);
			*lastDirSep = '/'; // restore path
			// if parent dir was successfully created (or already existed), create this dir
			if(ok && mkdir(path, 0755) == 0) {
				return true;
			}
		}
		return false;
	}
	return true;
}

void Posix_InitSignalHandlers( void )
{
#ifdef D3_HAVE_LIBBACKTRACE
	// can't use idStr here and thus can't use Sys_GetPath(PATH_EXE) => added Posix_GetExePath()
	const char* exePath = Posix_GetExePath();
	bt_state = backtrace_create_state(exePath[0] ? exePath : NULL, 0, bt_error_callback, NULL);
#endif

	for(int i=0; i<sizeof(crashSigs)/sizeof(crashSigs[0]); ++i)
	{
		installSigHandler(crashSigs[i], SA_RESTART|SA_RESETHAND, signalhandlerCrash);
	}

	installSigHandler(SIGTTIN, 0, signalhandlerConsoleStuff);
	installSigHandler(SIGTTOU, 0, signalhandlerConsoleStuff);

	// this is also a good place to open dhewm3log.txt for Sys_VPrintf()

	const char* savePath = Posix_GetSavePath();
	size_t savePathLen = strlen(savePath);
	if(savePathLen > 0 && savePathLen < PATH_MAX) {
		char logPath[PATH_MAX] = {};
		if(savePath[savePathLen-1] == '/') {
			--savePathLen;
		}
		memcpy(logPath, savePath, savePathLen);
		logPath[savePathLen] = '\0';
		if(!createPathRecursive(logPath)) {
			printf("WARNING: Couldn't create save path '%s'!\n", logPath);
			return;
		}
		char logFileName[PATH_MAX] = {};
		int fullLogLen = snprintf(logFileName, sizeof(logFileName), "%s/idtech4amm.txt", logPath);
		// cast to size_t which is unsigned and would get really big if fullLogLen < 0 (=> error in snprintf())
		if((size_t)fullLogLen >= sizeof(logFileName)) {
			printf("WARNING: Couldn't create idtech4amm.txt at '%s' because its length would be '%d' which is > PATH_MAX (%zd) or < 0!\n",
			       logPath, fullLogLen, (size_t)PATH_MAX);
			return;
		}
		struct stat buf;
		if(stat(logFileName, &buf) == 0) {
			// logfile exists, rename to dhewm3log-old.txt
			char oldLogFileName[PATH_MAX] = {};
			if((size_t)snprintf(oldLogFileName, sizeof(oldLogFileName), "%s/idtech4amm-old.txt", logPath) < sizeof(logFileName))
			{
				rename(logFileName, oldLogFileName);
			}
		}
		consoleLog = fopen(logFileName, "w");
		if(consoleLog == NULL) {
			printf("WARNING: Couldn't open/create '%s', error was: %d (%s)\n", logFileName, errno, strerror(errno));
		} else {
			time_t tt = time(NULL);
			const struct tm* tms = localtime(&tt);
			char timeStr[64] = {};
			strftime(timeStr, sizeof(timeStr), "%F %H:%M:%S", tms);
			fprintf(consoleLog, "Opened this log at %s\n", timeStr);
		}

	} else {
		printf("WARNING: Posix_GetSavePath() returned path with invalid length '%zd'!\n", savePathLen);
	}
}

/*
===============
main
===============
*/
int main(int argc, char **argv) {
	// Prevent running Doom 3 as root
	// Borrowed from Yamagi Quake II
	if (getuid() == 0) {
		printf("Doom 3 shouldn't be run as root! Backing out to save your ass. If\n");
		printf("you really know what you're doing, edit neo/sys/linux/main.cpp and remove\n");
		printf("this check. But don't complain if an imp kills your bunny afterwards!:)\n");

		return 1;
	}
	// fallback path to the binary for systems without /proc
	// while not 100% reliable, its good enough
	if (argc > 0) {
		if (!realpath(argv[0], path_argv))
			path_argv[0] = 0;
	} else {
		path_argv[0] = 0;
	}

	SetExecutablePath(path_exe);
	if (path_exe[0] == '\0') {
		memcpy(path_exe, path_argv, sizeof(path_exe));
	}

	SetSavePath();

	// some ladspa-plugins (that may be indirectly loaded by doom3 if they're
	// used by alsa) call setlocale(LC_ALL, ""); This sets LC_ALL to $LANG or
	// $LC_ALL which usually is not "C" and will fuck up scanf, strtod
	// etc when using a locale that uses ',' as a float radix.
	// so set $LC_ALL to "C".
	setenv("LC_ALL", "C", 1);

	Posix_InitSignalHandlers();

	//karin: SDL init in here from idCommon::Init
    extern void Sys_InitSDL(void);
    Sys_InitSDL();

	Posix_EarlyInit( );

	if ( argc > 1 ) {
		common->Init( argc-1, (const char **)&argv[1], NULL );
	} else {
		common->Init( 0, NULL, NULL );
	}

	//karin: do not use SDL_Timer in framework/Common.cpp
	// set the base time
	Posix_LateInit( );

	while (1) {
		common->Frame();
	}
	return 0;
}
