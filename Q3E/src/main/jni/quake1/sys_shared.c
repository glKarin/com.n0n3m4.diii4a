#ifdef WIN32
# ifndef DONT_USE_SETDLLDIRECTORY
#  define _WIN32_WINNT 0x0502
# endif
#endif

#define SUPPORTDLL

#ifdef WIN32
# include <windows.h>
# include <mmsystem.h> // timeGetTime
# include <time.h> // localtime
# include <conio.h> // _kbhit, _getch, _putch
# include <io.h> // write; Include this BEFORE darkplaces.h because it uses strncpy which trips DP_STATIC_ASSERT
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#else
# ifdef __FreeBSD__
#  include <sys/sysctl.h>
# endif
# ifdef __ANDROID__
#  include <android/log.h>
# endif
# include <unistd.h>
# include <fcntl.h>
# include <sys/time.h>
# include <time.h>
# ifdef SUPPORTDLL
#  include <dlfcn.h>
# endif
#endif

#include <signal.h>

#include "quakedef.h"
#include "taskqueue.h"
#include "thread.h"
#include "libcurl.h"


sys_t sys;

static char sys_timestring[128];
char *Sys_TimeString(const char *timeformat)
{
	time_t mytime = time(NULL);
#if _MSC_VER >= 1400
	struct tm mytm;
	localtime_s(&mytm, &mytime);
	strftime(sys_timestring, sizeof(sys_timestring), timeformat, &mytm);
#else
	strftime(sys_timestring, sizeof(sys_timestring), timeformat, localtime(&mytime));
#endif
	return sys_timestring;
}


#ifdef __cplusplus
extern "C"
#endif
void Sys_AllowProfiling(qbool enable)
{
#ifdef __ANDROID__
#ifdef USE_PROFILER
	extern void monstartup(const char *libname);
	extern void moncleanup(void);
	if (enable)
		monstartup("libmain.so");
	else
		moncleanup();
#endif
#elif (defined(__linux__) && (defined(__GLIBC__) || defined(__GNU_LIBRARY__))) || defined(__FreeBSD__)
	extern int moncontrol(int);
	moncontrol(enable);
#endif
}


/*
===============================================================================

DLL MANAGEMENT

===============================================================================
*/

static qbool Sys_LoadDependencyFunctions(dllhandle_t dllhandle, const dllfunction_t *fcts, qbool complain, qbool has_next)
{
	const dllfunction_t *func;
	if(dllhandle)
	{
		for (func = fcts; func && func->name != NULL; func++)
			if (!(*func->funcvariable = (void *) Sys_GetProcAddress (dllhandle, func->name)))
			{
				if(complain)
				{
					Con_DPrintf (" - missing function \"%s\" - broken library!", func->name);
					if(has_next)
						Con_DPrintf("\nContinuing with");
				}
				goto notfound;
			}
		return true;

	notfound:
		for (func = fcts; func && func->name != NULL; func++)
			*func->funcvariable = NULL;
	}
	return false;
}

qbool Sys_LoadSelf(dllhandle_t *handle)
{
	dllhandle_t dllhandle = 0;

	if (handle == NULL)
		return false;
#ifdef WIN32
	dllhandle = LoadLibrary (NULL);
#else
	dllhandle = dlopen (NULL, RTLD_NOW | RTLD_GLOBAL);
#endif
	*handle = dllhandle;
	return true;
}

qbool Sys_LoadDependency (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts)
{
#ifdef SUPPORTDLL
	const dllfunction_t *func;
	dllhandle_t dllhandle = 0;
	unsigned int i;

	if (handle == NULL)
		return false;

#ifndef WIN32
#ifdef PREFER_PRELOAD
	dllhandle = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);
	if(Sys_LoadDependencyFunctions(dllhandle, fcts, false, false))
	{
		Con_DPrintf ("All of %s's functions were already linked in! Not loading dynamically...\n", dllnames[0]);
		*handle = dllhandle;
		return true;
	}
	else
		Sys_FreeLibrary(&dllhandle);
notfound:
#endif
#endif

	// Initializations
	for (func = fcts; func && func->name != NULL; func++)
		*func->funcvariable = NULL;

	// Try every possible name
	Con_DPrintf ("Trying to load library...");
	for (i = 0; dllnames[i] != NULL; i++)
	{
		Con_DPrintf (" \"%s\"", dllnames[i]);
#ifdef WIN32
# ifndef DONT_USE_SETDLLDIRECTORY
#  ifdef _WIN64
		SetDllDirectory("bin64");
#  else
		SetDllDirectory("bin32");
#  endif
# endif
#endif
		if(Sys_LoadLibrary(dllnames[i], &dllhandle))
		{
			if (Sys_LoadDependencyFunctions(dllhandle, fcts, true, (dllnames[i+1] != NULL) || (strrchr(sys.argv[0], '/'))))
				break;
			else
				Sys_FreeLibrary (&dllhandle);
		}
	}

	// see if the names can be loaded relative to the executable path
	// (this is for Mac OSX which does not check next to the executable)
	if (!dllhandle && strrchr(sys.argv[0], '/'))
	{
		char path[MAX_OSPATH];
		dp_strlcpy(path, sys.argv[0], sizeof(path));
		strrchr(path, '/')[1] = 0;
		for (i = 0; dllnames[i] != NULL; i++)
		{
			char temp[MAX_OSPATH];
			dp_strlcpy(temp, path, sizeof(temp));
			dp_strlcat(temp, dllnames[i], sizeof(temp));
			Con_DPrintf (" \"%s\"", temp);

			if(Sys_LoadLibrary(temp, &dllhandle))
			{
				if (Sys_LoadDependencyFunctions(dllhandle, fcts, true, (dllnames[i+1] != NULL) || (strrchr(sys.argv[0], '/'))))
					break;
				else
					Sys_FreeLibrary (&dllhandle);
			}
		}
	}

	// No DLL found
	if (! dllhandle)
	{
		Con_DPrintf(" - failed.\n");
		return false;
	}

	Con_DPrintf(" - loaded.\n");
	Con_Printf("Loaded library \"%s\"\n", dllnames[i]);

	*handle = dllhandle;
	return true;
#else
	return false;
#endif
}

qbool Sys_LoadLibrary(const char *name, dllhandle_t *handle)
{
	dllhandle_t dllhandle = 0;

	if(handle == NULL)
		return false;

#ifdef SUPPORTDLL
# ifdef WIN32
	dllhandle = LoadLibrary (name);
# else
	dllhandle = dlopen (name, RTLD_LAZY | RTLD_GLOBAL);
# endif
#endif
	if(!dllhandle)
		return false;

	*handle = dllhandle;
	return true;
}

void Sys_FreeLibrary (dllhandle_t* handle)
{
#ifdef SUPPORTDLL
	if (handle == NULL || *handle == NULL)
		return;

#ifdef WIN32
	FreeLibrary (*handle);
#else
	dlclose (*handle);
#endif

	*handle = NULL;
#endif
}

void* Sys_GetProcAddress (dllhandle_t handle, const char* name)
{
#ifdef SUPPORTDLL
#ifdef WIN32
	return (void *)GetProcAddress (handle, name);
#else
	return (void *)dlsym (handle, name);
#endif
#else
	return NULL;
#endif
}



#ifdef WIN32
# define HAVE_TIMEGETTIME 1
# define HAVE_QUERYPERFORMANCECOUNTER 1
# define HAVE_WIN32_USLEEP 1
# define HAVE_Sleep 1
#else
# if defined(CLOCK_MONOTONIC) || defined(CLOCK_HIRES)
#  define HAVE_CLOCKGETTIME 1
# endif
# if _POSIX_VERSION >= 200112L
#  define HAVE_CLOCK_NANOSLEEP 1
# endif
#endif

#ifdef FD_SET
# define HAVE_SELECT 1
#endif

// these are referenced elsewhere
cvar_t sys_usenoclockbutbenchmark = {CF_SHARED, "sys_usenoclockbutbenchmark", "0", "don't use ANY real timing, and simulate a clock (for benchmarking); the game then runs as fast as possible. Run a QC mod with bots that does some stuff, then does a quit at the end, to benchmark a server. NEVER do this on a public server."};
cvar_t sys_libdir = {CF_READONLY | CF_SHARED, "sys_libdir", "", "Default engine library directory"};

// these are not
static cvar_t sys_debugsleep = {CF_SHARED, "sys_debugsleep", "0", "write requested and attained sleep times to standard output, to be used with gnuplot"};
static cvar_t sys_usesdlgetticks = {CF_SHARED, "sys_usesdlgetticks", "0", "use SDL_GetTicks() timer (low precision, for debugging)"};
static cvar_t sys_usesdldelay = {CF_SHARED, "sys_usesdldelay", "0", "use SDL_Delay() (low precision, for debugging)"};
#if HAVE_QUERYPERFORMANCECOUNTER
static cvar_t sys_usequeryperformancecounter = {CF_SHARED | CF_ARCHIVE, "sys_usequeryperformancecounter", "1", "use windows QueryPerformanceCounter timer (which has issues on systems lacking constant-rate TSCs synchronised across all cores, such as ancient PCs or VMs) for timing rather than timeGetTime function (which is low precision and had issues on some old motherboards)"};
#endif

static cvar_t sys_stdout = {CF_SHARED, "sys_stdout", "1", "0: nothing is written to stdout (-nostdout cmdline option sets this), 1: normal messages are written to stdout, 2: normal messages are written to stderr (-stderr cmdline option sets this)"};
#ifndef WIN32
static cvar_t sys_stdout_blocks = {CF_SHARED, "sys_stdout_blocks", "0", "1: writes to stdout and stderr streams will block (causing a stutter or complete halt) if the buffer is full, ensuring no messages are lost at a price"};
#endif

static double benchmark_time; // actually always contains an integer amount of milliseconds, will eventually "overflow"

/*
================
Sys_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int Sys_CheckParm (const char *parm)
{
	int i;

	for (i=1 ; i<sys.argc ; i++)
	{
		if (!sys.argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!strcmp (parm,sys.argv[i]))
			return i;
	}

	return 0;
}

static void Sys_UpdateOutFD_c(cvar_t *var)
{
	switch (sys_stdout.integer)
	{
		case 0: sys.outfd = -1; break;
		default:
		case 1: sys.outfd = fileno(stdout); break;
		case 2: sys.outfd = fileno(stderr); break;
	}
}

void Sys_Init_Commands (void)
{
	Cvar_RegisterVariable(&sys_debugsleep);
	Cvar_RegisterVariable(&sys_usenoclockbutbenchmark);
	Cvar_RegisterVariable(&sys_libdir);
#if HAVE_TIMEGETTIME || HAVE_QUERYPERFORMANCECOUNTER || HAVE_CLOCKGETTIME
	if(sys_supportsdlgetticks)
	{
		Cvar_RegisterVariable(&sys_usesdlgetticks);
		Cvar_RegisterVariable(&sys_usesdldelay);
	}
#endif
#if HAVE_QUERYPERFORMANCECOUNTER
	Cvar_RegisterVariable(&sys_usequeryperformancecounter);
#endif

	Cvar_RegisterVariable(&sys_stdout);
	Cvar_RegisterCallback(&sys_stdout, Sys_UpdateOutFD_c);
#ifndef WIN32
	Cvar_RegisterVariable(&sys_stdout_blocks);
#endif
}

#ifdef WIN32
static LARGE_INTEGER PerformanceFreq;
/// Windows default timer resolution is only 15.625ms,
/// this affects (at least) timeGetTime() and all forms of sleeping.
static void Sys_SetTimerResolution(void)
{
	NTSTATUS(NTAPI *qNtQueryTimerResolution)(OUT PULONG MinRes, OUT PULONG MaxRes, OUT PULONG CurrentRes);
	NTSTATUS(NTAPI *qNtSetTimerResolution)(IN ULONG DesiredRes, IN BOOLEAN SetRes, OUT PULONG ActualRes);
	const char* ntdll_names [] =
	{
		"ntdll.dll",
		NULL
	};
	dllfunction_t ntdll_funcs[] =
	{
		{"NtQueryTimerResolution", (void **) &qNtQueryTimerResolution},
		{"NtSetTimerResolution",   (void **) &qNtSetTimerResolution},
		{NULL, NULL}
	};
	dllhandle_t ntdll;
	unsigned long WorstRes, BestRes, CurrentRes;

	timeBeginPeriod(1); // 1ms, documented

	// the best Windows can manage (typically 0.5ms)
	// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FTime%2FNtSetTimerResolution.html
	if (Sys_LoadDependency(ntdll_names, &ntdll, ntdll_funcs))
	{
		qNtQueryTimerResolution(&WorstRes, &BestRes, &CurrentRes); // no pointers may be NULL
		if (CurrentRes > BestRes)
			qNtSetTimerResolution(BestRes, true, &CurrentRes);

		Sys_FreeLibrary(&ntdll);
	}

	// Microsoft says the freq is fixed at boot and consistent across all processors
	// and that it need only be queried once and cached.
	QueryPerformanceFrequency (&PerformanceFreq);
}
#endif // WIN32

double Sys_DirtyTime(void)
{
	// first all the OPTIONAL timers

	// benchmark timer (fake clock)
	if(sys_usenoclockbutbenchmark.integer)
	{
		double old_benchmark_time = benchmark_time;
		benchmark_time += 1;
		if(benchmark_time == old_benchmark_time)
			Sys_Error("sys_usenoclockbutbenchmark cannot run any longer, sorry");
		return benchmark_time * 0.000001;
	}

	if(sys_supportsdlgetticks && sys_usesdlgetticks.integer)
		return (double) Sys_SDL_GetTicks() / 1000.0;

#if HAVE_QUERYPERFORMANCECOUNTER
	if (sys_usequeryperformancecounter.integer)
	{
		// QueryPerformanceCounter
		// platform:
		// Windows 95/98/ME/NT/2000/XP
		// features:
		// + very accurate (constant-rate TSCs on modern systems)
		// known issues:
		// - does not necessarily match realtime too well (tends to get faster and faster in win98)
		// - wraps around occasionally on some platforms (depends on CPU speed and probably other unknown factors)
		// - higher access latency on Vista
		// Microsoft says on Win 7 or later, latency and overhead are very low, synchronisation is excellent.
		LARGE_INTEGER PerformanceCount;

		if (PerformanceFreq.QuadPart)
		{
			QueryPerformanceCounter (&PerformanceCount);
			return (double)PerformanceCount.QuadPart * (1.0 / (double)PerformanceFreq.QuadPart);
		}
		else
		{
			Con_Printf("No hardware timer available\n");
			// fall back to other clock sources
			Cvar_SetValueQuick(&sys_usequeryperformancecounter, false);
		}
	}
#endif

#if HAVE_CLOCKGETTIME
	{
		struct timespec ts;
#  ifdef CLOCK_MONOTONIC_RAW
		// Linux-specific, SDL_GetPerformanceCounter() uses it
		clock_gettime(CLOCK_MONOTONIC, &ts);
#  elif defined(CLOCK_MONOTONIC)
		// POSIX
		clock_gettime(CLOCK_MONOTONIC, &ts);
#  else
		// sunos
		clock_gettime(CLOCK_HIGHRES, &ts);
#  endif
		return (double) ts.tv_sec + ts.tv_nsec / 1000000000.0;
	}
#endif

	// now all the FALLBACK timers
#if HAVE_TIMEGETTIME
	{
		// timeGetTime
		// platform:
		// Windows 95/98/ME/NT/2000/XP
		// features:
		// reasonable accuracy (millisecond)
		// issues:
		// wraps around every 47 days or so (but this is non-fatal to us, odd times are rejected, only causes a one frame stutter)
		// requires Sys_SetTimerResolution()
		return (double) timeGetTime() / 1000.0;
	}
#else
	// fallback for using the SDL timer if no other timer is available
	// this calls Sys_Error() if not linking against SDL
	return (double) Sys_SDL_GetTicks() / 1000.0;
#endif
}

double Sys_Sleep(double time)
{
	double dt;
	uint32_t msec, usec, nsec;

	if (time < 1.0/1000000.0 || host.restless)
		return 0; // not sleeping this frame
	if (time >= 1)
		time = 0.999999; // ensure passed values are in range
	msec = time * 1000;
	usec = time * 1000000;
	nsec = time * 1000000000;

	if(sys_usenoclockbutbenchmark.integer)
	{
		double old_benchmark_time = benchmark_time;
		benchmark_time += usec;
		if(benchmark_time == old_benchmark_time)
			Sys_Error("sys_usenoclockbutbenchmark cannot run any longer, sorry");
		return 0;
	}

	if(sys_debugsleep.integer)
		Con_Printf("sys_debugsleep: requesting %u ", usec);
	dt = Sys_DirtyTime();

	// less important on newer libcurl so no need to disturb dedicated servers
	if (cls.state != ca_dedicated && Curl_Select(msec))
	{
		// a transfer is ready or we finished sleeping
	}
	else if(sys_supportsdlgetticks && sys_usesdldelay.integer)
		Sys_SDL_Delay(msec);
#if HAVE_SELECT
	else if (cls.state == ca_dedicated && sv_checkforpacketsduringsleep.integer)
	{
		struct timeval tv;
		lhnetsocket_t *s;
		fd_set fdreadset;
		int lastfd = -1;

		FD_ZERO(&fdreadset);
		List_For_Each_Entry(s, &lhnet_socketlist.list, lhnetsocket_t, list)
		{
			if (s->address.addresstype == LHNETADDRESSTYPE_INET4 || s->address.addresstype == LHNETADDRESSTYPE_INET6)
			{
				if (lastfd < s->inetsocket)
					lastfd = s->inetsocket;
	#if defined(WIN32) && !defined(_MSC_VER)
				FD_SET((int)s->inetsocket, &fdreadset);
	#else
				FD_SET((unsigned int)s->inetsocket, &fdreadset);
	#endif
			}
		}
		tv.tv_sec = 0;
		tv.tv_usec = usec;
		// on Win32, select() cannot be used with all three FD list args being NULL according to MSDN
		// (so much for POSIX...), not with an empty fd_set either.
		select(lastfd + 1, &fdreadset, NULL, NULL, &tv);
	}
#endif
#if HAVE_CLOCK_NANOSLEEP
	else
	{
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = nsec;
		clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
	}
#elif HAVE_WIN32_USLEEP // Windows XP/2003 minimum
	else
	{
		HANDLE timer;
		LARGE_INTEGER sleeptime;

		// takes 100ns units, negative indicates relative time
		sleeptime.QuadPart = -((int64_t)nsec / 100);
		timer = CreateWaitableTimer(NULL, true, NULL);
		SetWaitableTimer(timer, &sleeptime, 0, NULL, NULL, 0);
		WaitForSingleObject(timer, INFINITE);
		CloseHandle(timer);
	}
#elif HAVE_Sleep
	else
		Sleep(msec);
#else
	else
		Sys_SDL_Delay(msec);
#endif

	dt = Sys_DirtyTime() - dt;
	if(sys_debugsleep.integer)
		Con_Printf(" got %u oversleep %d\n", (unsigned int)(dt * 1000000), (unsigned int)(dt * 1000000) - usec);
	return (dt < 0 || dt >= 1800) ? 0 : dt;
}


/*
===============================================================================

STDIO

===============================================================================
*/

// NOTE: use only POSIX async-signal-safe library functions here (see: man signal-safety)
void Sys_Print(const char *text, size_t textlen)
{
#ifdef __ANDROID__
	if (developer.integer > 0)
	{
		__android_log_write(ANDROID_LOG_DEBUG, sys.argv[0], text);
	}
	printf("%s", text);
#else
	if(sys.outfd < 0)
		return;
  #ifndef WIN32
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl(sys.outfd, F_GETFL, 0);
		if (sys_stdout_blocks.integer)
			fcntl(sys.outfd, F_SETFL, origflags & ~O_NONBLOCK);
  #else
    #define write _write
  #endif
		while(*text)
		{
			fs_offset_t written = (fs_offset_t)write(sys.outfd, text, textlen);
			if(written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		}
  #ifndef WIN32
		if (sys_stdout_blocks.integer)
			fcntl(sys.outfd, F_SETFL, origflags);
	}
  #endif
	//fprintf(stdout, "%s", text);
#endif
}

void Sys_Printf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];
	int msglen;

	va_start(argptr,fmt);
	msglen = dpvsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (msglen >= 0)
		Sys_Print(msg, msglen);
}

/// Reads a line from POSIX stdin or the Windows console
char *Sys_ConsoleInput(void)
{
	static char text[MAX_INPUTLINE];
#ifdef WIN32
	static unsigned int len = 0;
	int c;

	// read a line out
	while (_kbhit ())
	{
		c = _getch ();
		if (c == '\r')
		{
			text[len] = '\0';
			_putch ('\n');
			len = 0;
			return text;
		}
		if (c == '\b')
		{
			if (len)
			{
				_putch (c);
				_putch (' ');
				_putch (c);
				len--;
			}
			continue;
		}
		if (len < sizeof (text) - 1)
		{
			_putch (c);
			text[len] = c;
			len++;
		}
	}
#else
	fd_set fdset;
	struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };

	FD_ZERO(&fdset);
	FD_SET(fileno(stdin), &fdset);
	if (select(1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(fileno(stdin), &fdset))
		return fgets(text, sizeof(text), stdin);
#endif
	return NULL;
}


/*
===============================================================================

Startup and Shutdown

===============================================================================
*/

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];
	int i;

	// Disable Sys_HandleSignal() but not Sys_HandleCrash()
	host.state = host_shutdown;

	// set output to blocking stderr
	sys.outfd = fileno(stderr);
#ifndef WIN32
	fcntl(sys.outfd, F_SETFL, fcntl(sys.outfd, F_GETFL, 0) & ~O_NONBLOCK);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf(CON_ERROR "Engine Aborted: %s\n^9%s\n", string, engineversion);

	dp_strlcat(string, "\n\n", sizeof(string));
	dp_strlcat(string, engineversion, sizeof(string));

	// Most shutdown funcs can't be called here as they could error while we error.

	// DP8 TODO: send a disconnect message indicating we aborted, see Host_Error() and Sys_HandleCrash()

	if (cls.demorecording)
		CL_Stop_f(cmd_local);
	if (sv.active)
	{
		sv.active = false; // make SV_DropClient() skip the QC stuff to avoid recursive errors
		for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
			if (host_client->active)
				SV_DropClient(false, "Server aborted!"); // closes demo file
	}
	// don't want a dead window left blocking the OS UI or the abort dialog
	VID_Shutdown();
	S_StopAllSounds();

	host.state = host_failed; // make Sys_HandleSignal() call _Exit()
	Sys_SDL_Dialog("Engine Aborted", string);

	fflush(stderr);
	exit (1);
}

#ifndef WIN32
static const char *Sys_FindInPATH(const char *name, char namesep, const char *PATH, char pathsep, char *buf, size_t bufsize)
{
	const char *p = PATH;
	const char *q;
	if(p && name)
	{
		while((q = strchr(p, ':')))
		{
			dpsnprintf(buf, bufsize, "%.*s%c%s", (int)(q-p), p, namesep, name);
			if(FS_SysFileExists(buf))
				return buf;
			p = q + 1;
		}
		if(!q) // none found - try the last item
		{
			dpsnprintf(buf, bufsize, "%s%c%s", p, namesep, name);
			if(FS_SysFileExists(buf))
				return buf;
		}
	}
	return name;
}
#endif

static const char *Sys_FindExecutableName(void)
{
#if defined(WIN32)
	return sys.argv[0];
#else
	static char exenamebuf[MAX_OSPATH+1];
	ssize_t n = -1;
#if defined(__FreeBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	size_t exenamebuflen = sizeof(exenamebuf)-1;
	if (sysctl(mib, 4, exenamebuf, &exenamebuflen, NULL, 0) == 0)
	{
		n = exenamebuflen;
	}
#elif defined(__linux__)
	n = readlink("/proc/self/exe", exenamebuf, sizeof(exenamebuf)-1);
#endif
	if(n > 0 && (size_t)(n) < sizeof(exenamebuf))
	{
		exenamebuf[n] = 0;
		return exenamebuf;
	}
	if(strchr(sys.argv[0], '/'))
		return sys.argv[0]; // possibly a relative path
	else
		return Sys_FindInPATH(sys.argv[0], '/', getenv("PATH"), ':', exenamebuf, sizeof(exenamebuf));
#endif
}

void Sys_ProvideSelfFD(void)
{
	if(sys.selffd != -1)
		return;
	sys.selffd = FS_SysOpenFD(Sys_FindExecutableName(), "rb", false);
}

// for x86 cpus only...  (x64 has SSE2_PRESENT)
#if defined(SSE_POSSIBLE) && !defined(SSE2_PRESENT)
// code from SDL, shortened as we can expect CPUID to work
static int CPUID_Features(void)
{
	int features = 0;
# if (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__)) && defined(__i386__)
        __asm__ (
"        movl    %%ebx,%%edi\n"
"        xorl    %%eax,%%eax                                           \n"
"        incl    %%eax                                                 \n"
"        cpuid                       # Get family/model/stepping/features\n"
"        movl    %%edx,%0                                              \n"
"        movl    %%edi,%%ebx\n"
        : "=m" (features)
        :
        : "%eax", "%ecx", "%edx", "%edi"
        );
# elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
        __asm {
        xor     eax, eax
        inc     eax
        cpuid                       ; Get family/model/stepping/features
        mov     features, edx
        }
# else
#  error SSE_POSSIBLE set but no CPUID implementation
# endif
	return features;
}
#endif

#ifdef SSE_POSSIBLE
qbool Sys_HaveSSE(void)
{
	// COMMANDLINEOPTION: SSE: -nosse disables SSE support and detection
	if(Sys_CheckParm("-nosse"))
		return false;
#ifdef SSE_PRESENT
	return true;
#else
	// COMMANDLINEOPTION: SSE: -forcesse enables SSE support and disables detection
	if(Sys_CheckParm("-forcesse") || Sys_CheckParm("-forcesse2"))
		return true;
	if(CPUID_Features() & (1 << 25))
		return true;
	return false;
#endif
}

qbool Sys_HaveSSE2(void)
{
	// COMMANDLINEOPTION: SSE2: -nosse2 disables SSE2 support and detection
	if(Sys_CheckParm("-nosse") || Sys_CheckParm("-nosse2"))
		return false;
#ifdef SSE2_PRESENT
	return true;
#else
	// COMMANDLINEOPTION: SSE2: -forcesse2 enables SSE2 support and disables detection
	if(Sys_CheckParm("-forcesse2"))
		return true;
	if((CPUID_Features() & (3 << 25)) == (3 << 25)) // SSE is 1<<25, SSE2 is 1<<26
		return true;
	return false;
#endif
}
#endif

/// called to set process priority for dedicated servers
#if defined(__linux__)
#include <sys/resource.h>
#include <errno.h>

void Sys_InitProcessNice (void)
{
	struct rlimit lim;
	sys.nicepossible = false;
	if(Sys_CheckParm("-nonice"))
		return;
	errno = 0;
	sys.nicelevel = getpriority(PRIO_PROCESS, 0);
	if(errno)
	{
		Con_Printf("Kernel does not support reading process priority - cannot use niceness\n");
		return;
	}
	if(getrlimit(RLIMIT_NICE, &lim))
	{
		Con_Printf("Kernel does not support lowering nice level again - cannot use niceness\n");
		return;
	}
	if(lim.rlim_cur != RLIM_INFINITY && sys.nicelevel < (int) (20 - lim.rlim_cur))
	{
		Con_Printf("Current nice level is below the soft limit - cannot use niceness\n");
		return;
	}
	sys.nicepossible = true;
	sys.isnice = false;
}
void Sys_MakeProcessNice (void)
{
	if(!sys.nicepossible)
		return;
	if(sys.isnice)
		return;
	Con_DPrintf("Process is becoming 'nice'...\n");
	if(setpriority(PRIO_PROCESS, 0, 19))
		Con_Printf(CON_ERROR "Failed to raise nice level to %d\n", 19);
	sys.isnice = true;
}
void Sys_MakeProcessMean (void)
{
	if(!sys.nicepossible)
		return;
	if(!sys.isnice)
		return;
	Con_DPrintf("Process is becoming 'mean'...\n");
	if(setpriority(PRIO_PROCESS, 0, sys.nicelevel))
		Con_Printf(CON_ERROR "Failed to lower nice level to %d\n", sys.nicelevel);
	sys.isnice = false;
}
#else
void Sys_InitProcessNice (void)
{
}
void Sys_MakeProcessNice (void)
{
}
void Sys_MakeProcessMean (void)
{
}
#endif


static const char *Sys_SigDesc(int sig)
{
	switch (sig)
	{
		// Windows only supports the C99 signals
		case SIGINT:  return "Interrupt";
		case SIGILL:  return "Illegal instruction";
		case SIGABRT: return "Aborted";
		case SIGFPE:  return "Floating point exception";
		case SIGSEGV: return "Segmentation fault";
		case SIGTERM: return "Termination";
#ifndef WIN32
		// POSIX has several others worth catching
		case SIGHUP:  return "Hangup";
		case SIGQUIT: return "Quit";
		case SIGBUS:  return "Bus error (bad memory access)";
		case SIGPIPE: return "Broken pipe";
#endif
		default:      return "Yo dawg, we bugged out while bugging out";
	}
}

/** Halt and try not to catch fire.
 * Writing to any file could corrupt it,
 * any uneccessary code could crash while we crash.
 * Try to use only POSIX async-signal-safe library functions here (see: man signal-safety).
 */
static void Sys_HandleCrash(int sig)
{
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
	// Before doing anything else grab the stack frame addresses
	#include <execinfo.h>
	void *stackframes[32];
	int framecount = backtrace(stackframes, 32);
	char **btstrings;
#endif
	char dialogtext[3072];
	const char *sigdesc;

	// Break any loop and disable Sys_HandleSignal()
	if (host.state == host_failing || host.state == host_failed)
		return;
	host.state = host_failing;

	sigdesc = Sys_SigDesc(sig);

	// set output to blocking stderr and print header, backtrace, version
	sys.outfd = fileno(stderr); // not async-signal-safe :(
#ifndef WIN32
	fcntl(sys.outfd, F_SETFL, fcntl(sys.outfd, F_GETFL, 0) & ~O_NONBLOCK);
	Sys_Print("\n\n\e[1;37;41m    Engine Crash: ", 30);
	Sys_Print(sigdesc, strlen(sigdesc));
	Sys_Print("    \e[m\n", 8);
  #if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
	// the first two addresses will be in this function and in signal() in libc
	backtrace_symbols_fd(stackframes + 2, framecount - 2, sys.outfd);
  #endif
	Sys_Print("\e[1m", 4);
	Sys_Print(engineversion, strlen(engineversion));
	Sys_Print("\e[m\n", 4);
#else // Windows console doesn't support colours
	Sys_Print("\n\nEngine Crash: ", 16);
	Sys_Print(sigdesc, strlen(sigdesc));
	Sys_Print("\n", 1);
	Sys_Print(engineversion, strlen(engineversion));
	Sys_Print("\n", 1);
#endif

	// DP8 TODO: send a disconnect message indicating we crashed, see Sys_Error() and Host_Error()

	// don't want a dead window left blocking the OS UI or the crash dialog
	VID_Shutdown();
	S_StopAllSounds();

	// prepare the dialogtext: signal, backtrace, version
	// the dp_st* funcs are POSIX async-signal-safe IF we don't trigger their warnings
	dp_strlcpy(dialogtext, sigdesc, sizeof(dialogtext));
	dp_strlcat(dialogtext, "\n\n", sizeof(dialogtext));
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
	btstrings = backtrace_symbols(stackframes + 2, framecount - 2); // calls malloc :(
	if (btstrings)
		for (int i = 0; i < framecount - 2; ++i)
		{
			dp_strlcat(dialogtext, btstrings[i], sizeof(dialogtext));
			dp_strlcat(dialogtext, "\n", sizeof(dialogtext));
		}
#endif
	dp_strlcat(dialogtext, "\n", sizeof(dialogtext));
	dp_strlcat(dialogtext, engineversion, sizeof(dialogtext));

	host.state = host_failed; // make Sys_HandleSignal() call _Exit()
	Sys_SDL_Dialog("Engine Crash", dialogtext);

	fflush(stderr); // not async-signal-safe :(

	// Continue execution with default signal handling.
	// A real crash will be re-triggered so the platform can handle it,
	// a fake crash (kill -SEGV) will cause a graceful shutdown.
	signal(sig, SIG_DFL);
}

static void Sys_HandleSignal(int sig)
{
	const char *sigdesc;

	// Break any loop, eg if each Sys_Print triggers a SIGPIPE
	if (host.state == host_shutdown || host.state == host_failing)
		return;

	sigdesc = Sys_SigDesc(sig);
	Sys_Print("\nReceived ", 10);
	Sys_Print(sigdesc, strlen(sigdesc));
	Sys_Print(" signal, exiting...\n", 20);
	if (host.state == host_failed)
	{
		// user is trying to kill the process while the SDL dialog is open
		fflush(stderr); // not async-signal-safe :(
		_Exit(sig);
	}
	host.state = host_shutdown;
}

/// SDL2 only handles SIGINT and SIGTERM by default and doesn't log anything
static void Sys_InitSignals(void)
{
#ifdef _DIII4A //karin: not handle signals for debug
	extern bool no_handle_signals;
	if(no_handle_signals)
		return;
#endif
	// Windows only supports the C99 signals
	signal(SIGINT,  Sys_HandleSignal);
	signal(SIGILL,  Sys_HandleCrash);
	signal(SIGABRT, Sys_HandleCrash);
	signal(SIGFPE,  Sys_HandleCrash);
	signal(SIGSEGV, Sys_HandleCrash);
	signal(SIGTERM, Sys_HandleSignal);
#ifndef WIN32
	// POSIX has several others worth catching
	signal(SIGHUP,  Sys_HandleSignal);
	signal(SIGQUIT, Sys_HandleSignal);
	signal(SIGBUS,  Sys_HandleCrash);
	signal(SIGPIPE, Sys_HandleSignal);
#endif
}

/** main() but renamed so we can wrap it in sys_sdl.c and sys_null.c
 * to avoid needing to include SDL.h in this file (would make the dedicated server require SDL).
 * SDL builds need SDL.h in the file where main() is defined because SDL renames and wraps main().
 */
int Sys_Main(int argc, char *argv[])
{
	sys.argc = argc;
	sys.argv = (const char **)argv;

	// COMMANDLINEOPTION: Console: -nostdout disables text output to the terminal the game was launched from
	// COMMANDLINEOPTION: -noterminal disables console output on stdout
	if(Sys_CheckParm("-noterminal") || Sys_CheckParm("-nostdout"))
		sys_stdout.string = "0";
	// COMMANDLINEOPTION: -stderr moves console output to stderr
	else if(Sys_CheckParm("-stderr"))
		sys_stdout.string = "2";
	// too early for Cvar_SetQuick
	sys_stdout.value = sys_stdout.integer = atoi(sys_stdout.string);
	Sys_UpdateOutFD_c(&sys_stdout);
#ifndef WIN32
	fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) | O_NONBLOCK);
	// stdout/stderr will be set to blocking in Sys_Print() if so configured, or during a fatal error.
	fcntl(fileno(stdout), F_SETFL, fcntl(fileno(stdout), F_GETFL, 0) | O_NONBLOCK);
	fcntl(fileno(stderr), F_SETFL, fcntl(fileno(stderr), F_GETFL, 0) | O_NONBLOCK);
#endif

	sys.selffd = -1;
	Sys_ProvideSelfFD(); // may call Con_Printf() so must be after sys.outfd is set

#ifdef __ANDROID__
	Sys_AllowProfiling(true);
#endif

	Sys_InitSignals();

#ifdef WIN32
	Sys_SetTimerResolution();
#endif

	Host_Main();

#ifdef __ANDROID__
	Sys_AllowProfiling(false);
#endif

#ifndef WIN32
	fcntl(fileno(stdout), F_SETFL, fcntl(fileno(stdout), F_GETFL, 0) & ~O_NONBLOCK);
	fcntl(fileno(stderr), F_SETFL, fcntl(fileno(stderr), F_GETFL, 0) & ~O_NONBLOCK);
#endif
	fflush(stdout);
	fflush(stderr);

	return 0;
}
