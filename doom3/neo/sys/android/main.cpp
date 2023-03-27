/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Krzysztof Klinikowski <kkszysiu@gmail.com>
Copyright (C) 2012 Havlena Petr <havlenapetr@gmail.com>

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"
#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef ID_MCHECK
#include <mcheck.h>
#endif

static idStr	basepath;
static idStr	savepath;

#if defined(__ANDROID__)
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit();

extern void Posix_EarlyInit();
extern void Posix_LateInit();
extern bool Posix_AddMousePollEvent(int action, int value);
extern void Posix_QueEvent(sysEventType_t type, int value, int value2,
                           int ptrLength, void *ptr);
#endif

/*
===========
Sys_InitScanTable
===========
*/

void * __attribute__((weak)) __dso_handle=0;

const char *workdir()
{
static char wd[256];
getcwd(wd,256);
return wd;
}

void Sys_InitScanTable(void)
{
	common->DPrintf("TODO: Sys_InitScanTable\n");
}

/*
=================
Sys_AsyncThread
=================
*/
void *Sys_AsyncThread(void *p)
{
	int now;
	int start, end;
	int ticked, to_ticked;

#if defined(__ANDROID__)
	xthreadInfo *threadInfo = static_cast<xthreadInfo *>(p);
	assert(threadInfo);
#endif

	start = Sys_Milliseconds();
	ticked = start >> 4;

	while (1) {
		start = Sys_Milliseconds();
		to_ticked = start >> 4;

		while (ticked < to_ticked) {
			common->Async();
			ticked++;
			Sys_TriggerEvent(TRIGGER_EVENT_ONE);
		}

		// sleep
		end = Sys_Milliseconds() - start;
		if (end < 16) {
			usleep(16 - end);
		}

		// thread exit
#if defined(__ANDROID__)
		if (threadInfo->threadCancel) {
			break;
		}
#else
		pthread_testcancel();
#endif
	}

	return NULL;
}

/*
 ==============
 Sys_DefaultSavePath
 ==============
 */
const char *Sys_DefaultSavePath(void)
{
#if defined(__ANDROID__)
	sprintf(savepath, workdir());
#else
#if defined( ID_DEMO_BUILD )
	sprintf(savepath, "%s/.doom3-demo", getenv("HOME"));
#else
	sprintf(savepath, "%s/.doom3", getenv("HOME"));
#endif
#endif
	return savepath.c_str();
}
/*
==============
Sys_EXEPath
==============
*/
const char *Sys_EXEPath(void)
{
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf(linkpath, "/proc/%d/exe", getpid());
	len = readlink(linkpath.c_str(), buf, sizeof(buf));

	if (len == -1) {
		Sys_Printf("couldn't stat exe path link %s\n", linkpath.c_str());
		buf[ len ] = '\0';
	}

	return buf;
}

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

const char *Sys_DefaultBasePath(void)
{
#if defined(__ANDROID__)
	return workdir();
#else
	struct stat st;
	idStr testbase;
	basepath = Sys_EXEPath();

	if (basepath.Length()) {
		basepath.StripFilename();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;

		if (stat(testbase.c_str(), &st) != -1 && S_ISDIR(st.st_mode)) {
			return basepath.c_str();
		} else {
			common->Printf("no '%s' directory in exe path %s, skipping\n", BASE_GAMEDIR, basepath.c_str());
		}
	}

	if (basepath != Posix_Cwd()) {
		basepath = Posix_Cwd();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;

		if (stat(testbase.c_str(), &st) != -1 && S_ISDIR(st.st_mode)) {
			return basepath.c_str();
		} else {
			common->Printf("no '%s' directory in cwd path %s, skipping\n", BASE_GAMEDIR, basepath.c_str());
		}
	}

	common->Printf("WARNING: using hardcoded default base path\n");
	return LINUX_DEFAULT_PATH;
#endif
}

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey(bool shifted)
{
	return shifted ? '~' : '`';
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown(void)
{
	basepath.Clear();
	savepath.Clear();
	Posix_Shutdown();
}

/*
===============
Sys_GetProcessorId
===============
*/
cpuid_t Sys_GetProcessorId(void)
{
	return CPUID_GENERIC;
}

/*
===============
Sys_GetProcessorString
===============
*/
const char *Sys_GetProcessorString(void)
{
	return "generic";
}

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions(int exceptions)
{
}

/*
===============
Sys_FPE_handler
===============
*/
void Sys_FPE_handler(int signum, siginfo_t *info, void *context)
{
	assert(signum == SIGFPE);
	Sys_Printf("FPE\n");
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks(void)
{
#if defined( __i386__ )
	unsigned long lo, hi;

	__asm__ __volatile__(
	        "push %%ebx\n"			\
	        "xor %%eax,%%eax\n"		\
	        "cpuid\n"					\
	        "rdtsc\n"					\
	        "mov %%eax,%0\n"			\
	        "mov %%edx,%1\n"			\
	        "pop %%ebx\n"
	        : "=r"(lo), "=r"(hi));
	return (double) lo + (double) 0xFFFFFFFF * hi;
#else
#warning unsupported CPU
	return 0;
#endif
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks(void)
{
	double t0, t1;

	t0 = Sys_GetClockTicks();
	Sys_Sleep(1000);
	t1 = Sys_GetClockTicks();
	return t1 - t0;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond(void)
{
	static bool		init = false;
	static double	ret;

	int		fd, len, pos, end;
	char	buf[ 4096 ];

	if (init) {
		return ret;
	}

#if defined(__ANDROID__)
	fd = open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", O_RDONLY);
#else
	fd = open("/proc/cpuinfo", O_RDONLY);
#endif

	if (fd == -1) {
		//common->Printf("couldn't read /proc/cpuinfo\n");
		ret = MeasureClockTicks();
		init = true;
		//common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
		return ret;
	}

	len = read(fd, buf, 4096);
	close(fd);

#if defined(__ANDROID__)
	if (len > 0) {
		ret = atof(buf);
		common->Printf("/proc/cpuinfo CPU frequency: %g MHz", ret / 1000.0);
		ret *= 1000;
		init = true;
		return ret;
	}
#else
	pos = 0;

	while (pos < len) {
		if (!idStr::Cmpn(buf + pos, "cpu MHz", 7)) {
			pos = strchr(buf + pos, ':') - buf + 2;
			end = strchr(buf + pos, '\n') - buf;

			if (pos < len && end < len) {
				buf[end] = '\0';
				ret = atof(buf + pos);
			} else {
				common->Printf("failed parsing /proc/cpuinfo\n");
				ret = MeasureClockTicks();
				init = true;
				common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
				return ret;
			}

			common->Printf("/proc/cpuinfo CPU frequency: %g MHz\n", ret);
			ret *= 1000000;
			init = true;
			return ret;
		}

		pos = strchr(buf + pos, '\n') - buf + 1;
	}
#endif

	common->Printf("failed parsing /proc/cpuinfo\n");
	ret = MeasureClockTicks();
	init = true;
	common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
	return ret;
}

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam(void)
{
	long	count, page_size;
	int		mb;

	count = sysconf(_SC_PHYS_PAGES);

	if (count == -1) {
		common->Printf("GetSystemRam: sysconf _SC_PHYS_PAGES failed\n");
		return 512;
	}

	page_size = sysconf(_SC_PAGE_SIZE);

	if (page_size == -1) {
		common->Printf("GetSystemRam: sysconf _SC_PAGE_SIZE failed\n");
		return 512;
	}

	mb= (int)((double)count * (double)page_size / (1024 * 1024));
	// round to the nearest 16Mb
	mb = (mb + 8) & ~15;
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
void Sys_DoStartProcess(const char *exeName, bool dofork)
{
	bool use_system = false;

	if (strchr(exeName, ' ')) {
		use_system = true;
	} else {
		// set exec rights when it's about a single file to execute
		struct stat buf;

		if (stat(exeName, &buf) == -1) {
			printf("stat %s failed: %s\n", exeName, strerror(errno));
		} else {
			if (chmod(exeName, buf.st_mode | S_IXUSR) == -1) {
				printf("cmod +x %s failed: %s\n", exeName, strerror(errno));
			}
		}
	}

	if (dofork) {
		switch (fork()) {
			case -1:
				// main thread
				break;
			case 0:

				if (use_system) {
					printf("system %s\n", exeName);
					system(exeName);
					_exit(0);
				} else {
					printf("execl %s\n", exeName);
					execl(exeName, exeName, NULL);
					printf("execl failed: %s\n", strerror(errno));
					_exit(-1);
				}

				break;
		}
	} else {
		if (use_system) {
			printf("system %s\n", exeName);
			system(exeName);
			sleep(1);	// on some systems I've seen that starting the new process and exiting this one should not be too close
		} else {
			printf("execl %s\n", exeName);
			execl(exeName, exeName, NULL);
			printf("execl failed: %s\n", strerror(errno));
		}

		// terminate
		_exit(0);
	}
}

/*
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL(const char *url, bool quit)
{
	const char	*script_path;
	idFile		*script_file;
	char		cmdline[ 1024 ];

	static bool	quit_spamguard = false;

	if (quit_spamguard) {
		common->DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	common->Printf("Open URL: %s\n", url);
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath(cvarSystem->GetCVarString("fs_savepath"), "", "openurl.sh");
	script_file = fileSystem->OpenExplicitFileRead(script_path);

	if (!script_file) {
		script_path = fileSystem->BuildOSPath(cvarSystem->GetCVarString("fs_basepath"), "", "openurl.sh");
		script_file = fileSystem->OpenExplicitFileRead(script_path);
	}

	if (!script_file) {
		common->Printf("Can't find URL script 'openurl.sh' in either savepath or basepath\n");
		common->Printf("OpenURL '%s' failed\n", url);
		return;
	}

	fileSystem->CloseFile(script_file);

	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if (quit) {
		quit_spamguard = true;
	}

	common->Printf("URL script: %s\n", script_path);

	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf(cmdline, 1024, "%s '%s' &",  script_path, url);
	sys->StartProcess(cmdline, quit);
}

/*
 ==================
 Sys_DoPreferences
 ==================
 */
void Sys_DoPreferences(void) { }

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ(bool enable)
{
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ(bool enable)
{
}

/*
===============
mem consistency stuff
===============
*/

#ifdef ID_MCHECK

const char *mcheckstrings[] = {
	"MCHECK_DISABLED",
	"MCHECK_OK",
	"MCHECK_FREE",	// block freed twice
	"MCHECK_HEAD",	// memory before the block was clobbered
	"MCHECK_TAIL"	// memory after the block was clobbered
};

void abrt_func(mcheck_status status)
{
	Sys_Printf("memory consistency failure: %s\n", mcheckstrings[ status + 1 ]);
	Posix_SetExit(EXIT_FAILURE);
	common->Quit();
}

#endif

#if defined(__ANDROID__)
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define ANDROID_CALL_PROTOCOL_TMPFILE 0x10001
#define ANDROID_CALL_PROTOCOL_PULL_INPUT_EVENT 0x10002
#define ANDROID_CALL_PROTOCOL_ATTACH_THREAD 0x10003

#define ANDROID_CALL_PROTOCOL_NATIVE_LIBRARY_DIR 0x20001
#define ANDROID_CALL_PROTOCOL_REDIRECT_OUTPUT_TO_FILE 0x20002
#define ANDROID_CALL_PROTOCOL_NO_HANDLE_SIGNALS 0x20003
#define ANDROID_CALL_PROTOCOL_MULTITHREAD 0x20005

// APK's native library path on Android.
char *native_library_dir = NULL;

// Do not catch signal
bool no_handle_signals = false;

// DOOM library call Android JNI
intptr_t (*Android_Call)(int protocol, int size, ...);

// Surface window
volatile ANativeWindow *window = NULL;
static volatile bool window_changed = false;

// OpenGL attributes
int gl_format = 0x8888;
int gl_msaa = 0;

// command line arguments
static int argc = 0;
static char **argv = 0;

// game main thread
static pthread_t				main_thread;

// enable redirect stdout/stderr to file
static bool redirect_output_to_file = true;

// multi-thread
bool multithreadActive = false;
volatile bool backendThreadShutdown = false;

// app paused
bool paused = false;

// app exit
static volatile bool running = false;

extern void GLimp_ActivateContext();
extern void GLimp_DeactivateContext();

#ifdef _MULTITHREAD
#define RENDER_THREAD_STARTED() (render_thread.threadHandle && !render_thread.threadCancel)

// render thread
static xthreadInfo				render_thread = {0};

extern void BackendThreadWait();
extern void BackendThreadTask();

void BackendThreadShutdown(void)
{
	if(!multithreadActive)
		return;
	if (!RENDER_THREAD_STARTED())
		return;
	BackendThreadWait();
	backendThreadShutdown = true;
	Sys_TriggerEvent(TRIGGER_EVENT_RUN_BACKEND);
	Sys_DestroyThread(render_thread);
	GLimp_ActivateContext();
	common->Printf("[Harmattan]: Render thread shutdown -> %s\n", RENDER_THREAD_NAME);
}

static void BackendThread(void *data)
{
	Sys_Printf("[Harmattan]: Enter doom3 render thread -> %s\n", Sys_GetThreadName());
	GLimp_ActivateContext();
	while(true)
	{
		BackendThreadTask();
		if(render_thread.threadCancel)
			break;
		if(backendThreadShutdown)
			break;
	}
	GLimp_DeactivateContext();
	Sys_Printf("[Harmattan]: Leave doom3 render thread -> %s\n", RENDER_THREAD_NAME);
}

void BackendThreadExecute(void)
{
	if(!multithreadActive)
		return;
	if (RENDER_THREAD_STARTED())
		return;
	GLimp_DeactivateContext();
	backendThreadShutdown = false;
	Sys_CreateThread(BackendThread, common, THREAD_HIGHEST, render_thread, RENDER_THREAD_NAME, g_threads, &g_thread_count);
	common->Printf("[Harmattan]: Render thread start -> %lu(%s)\n", render_thread.threadHandle, RENDER_THREAD_NAME);
}
#endif

static void doom3_main(void *data)
{
	(void)Android_Call(ANDROID_CALL_PROTOCOL_ATTACH_THREAD, 0);
#ifdef ID_MCHECK
	// must have -lmcheck linkage
	mcheck(abrt_func);
	Sys_Printf("memory consistency checking enabled\n");
#endif
	GLimp_AndroidInit(window);

	Posix_EarlyInit();
	Sys_Printf("[Harmattan]: Enter doom3 main thread -> %s\n", Sys_GetThreadName());

	if (argc > 1) {
		common->Init(argc-1, &argv[1], NULL);
	} else {
		common->Init(0, NULL, NULL);
	}

	Posix_LateInit();

	for(int i = 0; i < argc; i++)
	{
		free(argv[i]);
	}
	free(argv);

	while (1) {
		if(window_changed)
		{
#ifdef _MULTITHREAD
			if(multithreadActive)
				BackendThreadShutdown();
#endif
			if(window) // if set new window, create EGLSurface
			{
				GLimp_AndroidInit(window);
				window_changed = false;
			}
			else // if window is null, release old window, and notify JNI, and wait new window set
			{
				GLimp_AndroidQuit();
				window_changed = false;
				Sys_TriggerEvent(TRIGGER_EVENT_DEACTIVATE_CONTEXT);
				Sys_WaitForEvent(TRIGGER_EVENT_ACTIVATE_CONTEXT);
				window_changed = false;
				GLimp_AndroidInit(window);
			}
		}
		if(!running) // exit
			break;
		common->Frame();
	}

	common->Quit();
	GLimp_AndroidQuit();
	main_thread = 0;
	window = NULL;
	Sys_Printf("[Harmattan]: Leave doom3 main thread.\n");
	return 0;
}

static void Q3E_StartGameMainThread(void)
{
	if(main_thread)
		return;

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		Sys_Printf("[Harmattan]: ERROR: pthread_attr_setdetachstate doom3 main thread failed\n");
		exit(1);
	}

	if (pthread_create((pthread_t *)&main_thread, &attr, doom3_main, NULL) != 0) {
		Sys_Printf("[Harmattan]: ERROR: pthread_create doom3 main thread failed\n");
		exit(1);
	}

	pthread_attr_destroy(&attr);

	running = true;
	Sys_Printf("[Harmattan]: doom3 main thread start.\n");
}

static void Q3E_StopGameMainThread(void)
{
	if(!main_thread)
		return;

	running = false;
	if (pthread_join(main_thread, NULL) != 0) {
		Sys_Printf("[Harmattan]: ERROR: pthread_join doom3 main thread failed\n");
	}
	main_thread = 0;
	Sys_Printf("[Harmattan]: doom3 main thread quit.\n");
}

extern "C"
{

#pragma GCC visibility push(default)
int main(int argc, const char **argv)
{
	::argc = argc;
	::argv = malloc(sizeof(char *) * argc);
	for(int i = 0; i < argc; i++)
	{
		::argv[i] = strdup(argv[i]);
	}

	if(redirect_output_to_file)
	{
		freopen("stdout.txt","w",stdout);
		setvbuf(stdout, NULL, _IONBF, 0);
		freopen("stderr.txt","w",stderr);
		setvbuf(stderr, NULL, _IONBF, 0);
	}

	Sys_Printf("[Harmattan]: doom3 command line arguments: %d.\n", argc);
	for(int i = 0; i < argc; i++)
	{
		Sys_Printf("    %d: %s\n", i, argv[i]);
	}
	Q3E_StartGameMainThread();
	return 0;
}

int screen_width=640;
int screen_height=480;

void Q3E_SetResolution(int width, int height)
{
	screen_width=width;
	screen_height=height;
}

void Q3E_OGLRestart()
{
//	R_VidRestart_f();
}

void (*initAudio)(void *buffer, int size);
int (*writeAudio)(int offset, int length);
void (*setState)(int st);
void Q3E_SetCallbacks(void *init_audio, void *write_audio, void *set_state)
{
#ifdef _K_CLANG
    setState=(void(*)(int))set_state;
    writeAudio = (int(*)(int, int))write_audio;
    initAudio = (void(*)(void *, int))init_audio;
#else
    setState=set_state;
    writeAudio = write_audio;
    initAudio = init_audio;
#endif
}

extern int m_buffer_size;

void Q3E_GetAudio()
{
#warning "UNUSED"
    writeAudio(0,m_buffer_size);
}

void Q3E_KeyEvent(int state,int key,int character)
{
	Posix_QueEvent(SE_KEY, key, state, 0, NULL);
	if ((character!=0)&&(state==1))
	{
		Posix_QueEvent(SE_CHAR, character, 0, 0, NULL);
	}
	Posix_AddKeyboardPollEvent(key, state);
}

void Q3E_MotionEvent(float dx, float dy)
{
	Posix_QueEvent(SE_MOUSE, dx, dy, 0, NULL);
	Posix_AddMousePollEvent(M_DELTAX, dx);
	Posix_AddMousePollEvent(M_DELTAY, dy);
}

void Q3E_DrawFrame()
{
	common->Frame();
}

float analogx=0.0f;
float analogy=0.0f;
int analogenabled=0;
void Q3E_Analog(int enable,float x,float y)
{
	analogenabled=enable;
	analogx=x;
	analogy=y;
}

intptr_t (*set_Android_Call(intptr_t (*func)(int, int, ...)))(int, int, ...)
{
	intptr_t (*cur)(int, int, ...);
	cur = Android_Call;
	Android_Call = func;
	return cur;
}

// Android JNI call DOOM library
intptr_t Q3E_Call(int protocol, int size, ...)
{
	intptr_t res = 0;
	va_list va;

	va_start(va, size);
	switch(protocol)
	{
		case ANDROID_CALL_PROTOCOL_NATIVE_LIBRARY_DIR:
			native_library_dir = strdup(va_arg(va, char *));
			res = (intptr_t)native_library_dir;
			break;
		case ANDROID_CALL_PROTOCOL_REDIRECT_OUTPUT_TO_FILE:
			redirect_output_to_file = va_arg(va, int) ? true : false;
			res = redirect_output_to_file;
			break;
		case ANDROID_CALL_PROTOCOL_NO_HANDLE_SIGNALS:
			no_handle_signals = va_arg(va, int) ? true : false;
			res = no_handle_signals;
			break;
		case ANDROID_CALL_PROTOCOL_MULTITHREAD:
			multithreadActive = va_arg(va, int) ? true : false;
			res = multithreadActive;
			break;
		default:
			break;
	}
	va_end(va);

	return res;
}

// Request exit
void Q3E_exit(void)
{
	running = false;
	if(common->IsInitialized())
	{
		Sys_TriggerEvent(TRIGGER_EVENT_ACTIVATE_CONTEXT); // if doom3 main thread is waiting new window
		Q3E_StopGameMainThread();
		common->Quit();
	}
	if(window)
		window = NULL;
	GLimp_AndroidQuit();
	Sys_Printf("[Harmattan]: doom3 exit.\n");
}

// View paused
void Q3E_OnPause(void)
{
	if(common->IsInitialized())
		paused = true;
}

// View resume
void Q3E_OnResume(void)
{
	if(common->IsInitialized())
		paused = false;
}

// Setup OpenGL context variables
void Android_SetGLContext(ANativeWindow *w, int size, ...)
{
	va_list va;

	if(size > 0)
	{
		va_start(va, size);
		gl_format = va_arg(va, int);
		gl_msaa = va_arg(va, int);
		va_end(va);
	}
	// if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
	if(common->IsInitialized())
	{
		if(!w) // set window is null, and wait doom3 main thread deactive OpenGL render context.
		{
			window = NULL;
			window_changed = true;
			while(window_changed)
				Sys_WaitForEvent(TRIGGER_EVENT_DEACTIVATE_CONTEXT);
		}
		else // set new window, notify doom3 main thread active OpenGL render context
		{
			window = w;
			window_changed = true;
			Sys_TriggerEvent(TRIGGER_EVENT_ACTIVATE_CONTEXT);
		}
	}
	else
		window = w;
}

#pragma GCC visibility pop
}

// For pull input event from Java when GLThread is looping in modal MessageBox of game.
void pull_input_event(int execCmd)
{
	if(!Android_Call)
		return;
	(void)Android_Call(ANDROID_CALL_PROTOCOL_PULL_INPUT_EVENT, 1, execCmd);
}

// tmpfile function symbol for Android.
FILE * itmpfile(void)
{
	if(!Android_Call)
		return NULL;
	return (FILE *)Android_Call(ANDROID_CALL_PROTOCOL_TMPFILE, 0);
}

#include "../../framework/Session_local.h"
#define STATE_NONE 0
#define STATE_ACT 1 // RTCW4A-specific, keep
#define STATE_GAME (1 << 1) // map spawned
#define STATE_KICK (1 << 2) // RTCW4A-specific, keep
#define STATE_LOADING (1 << 3) // current GUI is guiLoading
#define STATE_CONSOLE (1 << 4) // fullscreen
#define STATE_MENU (1 << 5) // any menu excludes guiLoading
#define STATE_DEMO (1 << 6) // demo

void sync_state(void)
{
	static int prev_state = -1;
	static int state = -1;
	if (setState)
	{
		state = STATE_NONE;
		if(sessLocal.insideExecuteMapChange)
			state |= STATE_LOADING;
		else
		{
			idUserInterface *gui = sessLocal.GetActiveMenu();
			if(!gui)
				state |= STATE_GAME;
			else
				state |= STATE_MENU;
		}
		if(console->Active())
			state |= STATE_CONSOLE;
		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

#else

#endif
