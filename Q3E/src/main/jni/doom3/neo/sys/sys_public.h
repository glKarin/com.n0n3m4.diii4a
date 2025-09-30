/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

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

#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/

// Win32
#if defined(WIN32) || defined(_WIN32)

#ifdef _WIN64
#define	BUILD_STRING					"win-x64"
#define BUILD_OS_ID						0
#define	CPUSTRING						"x64"
#else
#define	BUILD_STRING					"win-x86"
#define BUILD_OS_ID						0
#define	CPUSTRING						"x86"
#endif
#define CPU_EASYARGS					1

#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#define _alloca16( x )					((void *)((((intptr_t)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPERATOR_STR				"\\"
#define PATHSEPERATOR_CHAR				'\\'

#ifdef _MSC_VER
#define ID_STATIC_TEMPLATE				static
#define ID_INLINE						__forceinline
#else
#define ID_STATIC_TEMPLATE
#define ID_INLINE						inline
#endif

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

bool Sys_IsMainThread();
#endif

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)

#define BUILD_STRING				"MacOSX-universal"
#define BUILD_OS_ID					1
#ifdef __ppc__
	#define	CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#elif defined(__i386__)
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#endif

#define ALIGN16( x )					x __attribute__ ((aligned (16)))

#ifdef __MWERKS__
#define PACKED
#include <alloca.h>
#else
#define PACKED							__attribute__((packed))
#endif

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_INLINE						inline
#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#endif

// Linux
#ifdef __linux__

#ifdef __ANDROID__ //karin: Android arch defines

#if defined(__i386__)
#define	BUILD_STRING				"android-x86"
#define BUILD_OS_ID					2
#define CPUSTRING					"x86"
#elif defined(__x86_64__)
#define	BUILD_STRING				"android-x86_64"
#define BUILD_OS_ID					2
#define CPUSTRING					"x86_64"
#elif defined(__aarch64__)
#define	BUILD_STRING				"android-arm64"
#define CPUSTRING					"arm64"
#define BUILD_OS_ID					2
#elif defined(__arm__)
#define	BUILD_STRING				"android-arm"
#define CPUSTRING					"arm"
#define BUILD_OS_ID					2
#endif

#else

#if defined(__i386__)
#define	BUILD_STRING				"linux-x86"
#define BUILD_OS_ID					2
#define CPUSTRING					"x86"
#elif defined(__x86_64__)
#define	BUILD_STRING				"linux-x86_64"
#define BUILD_OS_ID					2
#define CPUSTRING					"x86_64"
#elif defined(__ppc__)
#define	BUILD_STRING				"linux-ppc"
#define CPUSTRING					"ppc"
#elif defined(__arm__)
#define	BUILD_STRING				"linux-arm"
#define BUILD_OS_ID					2
#define CPUSTRING					"arm"
#elif defined(__aarch64__)
#define	BUILD_STRING				"linux-arm64"
#define CPUSTRING					"arm64"
#define BUILD_OS_ID					2
#elif defined(__e2k__)
#define	BUILD_STRING				"linux-e2k"
#define CPUSTRING					"e2k"
#define BUILD_OS_ID					2
#endif

#endif

#if 0 //#ifdef _K_DEV
#define _alloca( x )							(Sys_Printf("_alloca(%d)\n", x),alloca( x ))
#define _alloca16( x )					(Sys_Printf("_alloca16(%d)\n", x),((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15)))
#else
#define _alloca							alloca
#define _alloca16( x )					((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15))
#endif

#define ALIGN16( x )					x
#define PACKED							__attribute__((packed))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_INLINE						inline
#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#endif

#ifdef __GNUC__
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)
#endif

#ifdef _SDL
enum sysPath_t {
    PATH_BASE,
    PATH_CONFIG,
    PATH_SAVE,
    PATH_EXE
};
#endif
typedef enum {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000	// Denormals-Are-Zero mode (denormal source operands are set to zero)
} cpuid_t;

typedef enum {
	FPU_EXCEPTION_INVALID_OPERATION		= 1,
	FPU_EXCEPTION_DENORMALIZED_OPERAND	= 2,
	FPU_EXCEPTION_DIVIDE_BY_ZERO		= 4,
	FPU_EXCEPTION_NUMERIC_OVERFLOW		= 8,
	FPU_EXCEPTION_NUMERIC_UNDERFLOW		= 16,
	FPU_EXCEPTION_INEXACT_RESULT		= 32
} fpuExceptions_t;

typedef enum {
	FPU_PRECISION_SINGLE				= 0,
	FPU_PRECISION_DOUBLE				= 1,
	FPU_PRECISION_DOUBLE_EXTENDED		= 2
} fpuPrecision_t;

typedef enum {
	FPU_ROUNDING_TO_NEAREST				= 0,
	FPU_ROUNDING_DOWN					= 1,
	FPU_ROUNDING_UP						= 2,
	FPU_ROUNDING_TO_ZERO				= 3
} fpuRounding_t;

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an ascii char
	SE_MOUSE,				// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,		// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
} sysEventType_t;

typedef enum {
	M_ACTION1,
	M_ACTION2,
	M_ACTION3,
	M_ACTION4,
	M_ACTION5,
	M_ACTION6,
	M_ACTION7,
	M_ACTION8,
	M_DELTAX,
	M_DELTAY,
	M_DELTAZ
} sys_mEvents;

typedef struct sysEvent_s {
	sysEventType_t	evType;
	int				evValue;
	int				evValue2;
	int				evPtrLength;		// bytes of data pointed to by evPtr, for journaling
	void 			*evPtr;				// this must be manually freed if not NULL
} sysEvent_t;

typedef struct sysMemoryStats_s {
	int memoryLoad;
	int totalPhysical;
	int availPhysical;
	int totalPageFile;
	int availPageFile;
	int totalVirtual;
	int availVirtual;
	int availExtendedVirtual;
} sysMemoryStats_t;

typedef unsigned long address_t;

template<class type> class idList;		// for Sys_ListFiles


void			Sys_Init(void);
void			Sys_Shutdown(void);
void			Sys_Error(const char *error, ...);
void			Sys_Quit(void);

bool			Sys_AlreadyRunning(void);

// note that this isn't journaled...
char 			*Sys_GetClipboardData(void);
void			Sys_SetClipboardData(const char *string);

// will go to the various text consoles
// NOT thread safe - never use in the async paths
void			Sys_Printf(const char *msg, ...)id_attribute((format(printf,1,2)));

// guaranteed to be thread-safe
void			Sys_DebugPrintf(const char *fmt, ...)id_attribute((format(printf,1,2)));
void			Sys_DebugVPrintf(const char *fmt, va_list arg);

// a decent minimum sleep time to avoid going below the process scheduler speeds
#define			SYS_MINSLEEP	20

// allow game to yield CPU time
// NOTE: due to SYS_MINSLEEP this is very bad portability karma, and should be completely removed
void			Sys_Sleep(int msec);

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int				Sys_Milliseconds(void);

// for accurate performance testing
double			Sys_GetClockTicks(void);
double			Sys_ClockTicksPerSecond(void);

// returns a selection of the CPUID_* flags
int			    Sys_GetProcessorId(void);
const char 	*   Sys_GetProcessorString(void);

// returns true if the FPU stack is empty
bool			Sys_FPU_StackIsEmpty(void);

// empties the FPU stack
void			Sys_FPU_ClearStack(void);

// returns the FPU state as a string
const char 	*Sys_FPU_GetState(void);

// enables the given FPU exceptions
void			Sys_FPU_EnableExceptions(int exceptions);

// sets the FPU precision
void			Sys_FPU_SetPrecision(int precision);

// sets the FPU rounding mode
void			Sys_FPU_SetRounding(int rounding);

// sets Flush-To-Zero mode (only available when CPUID_FTZ is set)
void			Sys_FPU_SetFTZ(bool enable);

// sets Denormals-Are-Zero mode (only available when CPUID_DAZ is set)
void			Sys_FPU_SetDAZ(bool enable);

// returns amount of system ram
int				Sys_GetSystemRam(void);

// returns amount of video ram
int				Sys_GetVideoRam(void);

// returns amount of drive space in path
int				Sys_GetDriveFreeSpace(const char *path);

// returns memory stats
void			Sys_GetCurrentMemoryStatus(sysMemoryStats_t &stats);
void			Sys_GetExeLaunchMemoryStatus(sysMemoryStats_t &stats);

// lock and unlock memory
bool			Sys_LockMemory(void *ptr, int bytes);
bool			Sys_UnlockMemory(void *ptr, int bytes);

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory(int minBytes, int maxBytes);

// allows retrieving the call stack at execution points
void			Sys_GetCallStack(address_t *callStack, const int callStackSize);
const char 	*Sys_GetCallStackStr(const address_t *callStack, const int callStackSize);
const char 	*Sys_GetCallStackCurStr(int depth);
const char 	*Sys_GetCallStackCurAddressStr(int depth);
void			Sys_ShutdownSymbols(void);

// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
uintptr_t		Sys_DLL_Load(const char *dllName);
void 			*Sys_DLL_GetProcAddress(uintptr_t dllHandle, const char *procName);
void			Sys_DLL_Unload(uintptr_t dllHandle);

// event generation
void			Sys_GenerateEvents(void);
sysEvent_t		Sys_GetEvent(void);
void			Sys_ClearEvents(void);

// input is tied to windows, so it needs to be started up and shut down whenever
// the main window is recreated
void			Sys_InitInput(void);
void			Sys_ShutdownInput(void);
void			Sys_InitScanTable(void);
const unsigned char *Sys_GetScanTable(void);
unsigned char	Sys_GetConsoleKey(bool shifted);
// map a scancode key to a char
// does nothing on win32, as SE_KEY == SE_CHAR there
// on other OSes, consider the keyboard mapping
unsigned char	Sys_MapCharForKey(int key);

// keyboard input polling
int				Sys_PollKeyboardInputEvents(void);
int				Sys_ReturnKeyboardInputEvent(const int n, int &ch, bool &state);
void			Sys_EndKeyboardInputEvents(void);

// mouse input polling
int				Sys_PollMouseInputEvents(void);
int				Sys_ReturnMouseInputEvent(const int n, int &action, int &value);
void			Sys_EndMouseInputEvents(void);

// when the console is down, or the game is about to perform a lengthy
// operation like map loading, the system can release the mouse cursor
// when in windowed mode
void			Sys_GrabMouseCursor(bool grabIt);

void			Sys_ShowWindow(bool show);
bool			Sys_IsWindowVisible(void);
void			Sys_ShowConsole(int visLevel, bool quitOnClose);


void			Sys_Mkdir(const char *path);
ID_TIME_T			Sys_FileTimeStamp(FILE *fp);
// NOTE: do we need to guarantee the same output on all platforms?
const char 	*Sys_TimeStampToStr(ID_TIME_T timeStamp);
const char 	*Sys_DefaultCDPath(void);
const char 	*Sys_DefaultBasePath(void);
const char 	*Sys_DefaultSavePath(void);
const char 	*Sys_EXEPath(void);

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles(const char *directory, const char *extension, idList<class idStr> &list);

// know early if we are performing a fatal error shutdown so the error message doesn't get lost
void			Sys_SetFatalError(const char *error);

// display perference dialog
void			Sys_DoPreferences(void);

/*
==============================================================

	Networking

==============================================================
*/

typedef enum {
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP
} netadrtype_t;

typedef struct {
	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned short	port;
} netadr_t;

#define	PORT_ANY			-1

class idPort
{
	public:
		idPort();				// this just zeros netSocket and port
		virtual		~idPort();

		// if the InitForPort fails, the idPort.port field will remain 0
		bool		InitForPort(int portNumber);
		int			GetPort(void) const {
			return bound_to.port;
		}
		netadr_t	GetAdr(void) const {
			return bound_to;
		}
		void		Close();

		bool		GetPacket(netadr_t &from, void *data, int &size, int maxSize);
		bool		GetPacketBlocking(netadr_t &from, void *data, int &size, int maxSize, int timeout);
		void		SendPacket(const netadr_t to, const void *data, int size);

		int			packetsRead;
		int			bytesRead;

		int			packetsWritten;
		int			bytesWritten;

	private:
		netadr_t	bound_to;		// interface and port
		int			netSocket;		// OS specific socket
};

class idTCP
{
	public:
		idTCP();
		virtual		~idTCP();

		// if host is host:port, the value of port is ignored
		bool		Init(const char *host, short port);
		void		Close();

		// returns -1 on failure (and closes socket)
		// those are non blocking, can be used for polling
		// there is no buffering, you are not guaranteed to Read or Write everything in a single call
		// (specially on win32, see recv and send documentation)
		int			Read(void *data, int size);
		int			Write(void *data, int size);

	private:
		netadr_t	address;		// remote address
		int			fd;				// OS specific socket
};

// parses the port number
// can also do DNS resolve if you ask for it.
// NOTE: DNS resolve is a slow/blocking call, think before you use
// ( could be exploited for server DoS )
bool			Sys_StringToNetAdr(const char *s, netadr_t *a, bool doDNSResolve);
const char 	*Sys_NetAdrToString(const netadr_t a);
bool			Sys_IsLANAddress(const netadr_t a);
bool			Sys_CompareNetAdrBase(const netadr_t a, const netadr_t b);

void			Sys_InitNetworking(void);
void			Sys_ShutdownNetworking(void);


/*
==============================================================

	Multi-threading

==============================================================
*/

typedef void *(*xthread_t)(void *);

typedef enum {
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
} xthreadPriority;

typedef struct {
	const char 	*name;
	intptr_t	threadHandle;
	size_t		threadId;
#ifdef _NO_PTHREAD_CANCEL //karin: no pthread_cancel on Android
	bool		threadCancel;
#endif
} xthreadInfo;

const int MAX_THREADS				= 10;
extern xthreadInfo *g_threads[MAX_THREADS];
extern int			g_thread_count;

void				Sys_CreateThread(xthread_t function, void *parms, xthreadPriority priority, xthreadInfo &info, const char *name, xthreadInfo *threads[MAX_THREADS], int *thread_count);
void				Sys_DestroyThread(xthreadInfo &info);   // sets threadHandle back to 0

// find the name of the calling thread
// if index != NULL, set the index in g_threads array (use -1 for "main" thread)
const char 		*Sys_GetThreadName(int *index = 0);

const int MAX_CRITICAL_SECTIONS		= 4
//#ifdef _HUMANHEAD //karin: for subtitle in snd system
//+ 1
//#endif
#if defined(_MULTITHREAD) && defined(_IMGUI) //karin: for imgui in multithreading
+ 1
#endif
#if 1 //def _SDL
+ 1
#endif
;

#ifdef _HUMANHEAD //karin: for subtitle in snd system
#define CRITICAL_SECTION_SUBTITLE CRITICAL_SECTION_THREE
#endif

enum {
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE
//#ifdef _HUMANHEAD //karin: for subtitle in snd system
//    , CRITICAL_SECTION_SUBTITLE
//#endif
#if defined(_MULTITHREAD) && defined(_IMGUI) //karin: for imgui in multithreading
    , CRITICAL_SECTION_IMGUI
#endif
#if 1 //def _SDL
    , CRITICAL_SECTION_SYS
#endif
};

void				Sys_EnterCriticalSection(int index = CRITICAL_SECTION_ZERO);
void				Sys_LeaveCriticalSection(int index = CRITICAL_SECTION_ZERO);

class idCriticalSectionLockGuard
{
public:
    explicit idCriticalSectionLockGuard(int index = CRITICAL_SECTION_ZERO)
            : _index(index) {
        Sys_EnterCriticalSection(_index);
    }
    ~idCriticalSectionLockGuard() {
        Sys_LeaveCriticalSection(_index);
    }
private:
    int _index;
};

const int MAX_TRIGGER_EVENTS		= (
        4
#ifdef __ANDROID__
+ 3
#endif
#ifdef _MULTITHREAD
+ 4
#endif
#ifdef _OPENSLES
+ 2
#endif
);

enum {
	TRIGGER_EVENT_ZERO = 0,
	TRIGGER_EVENT_ONE,
	TRIGGER_EVENT_TWO,
	TRIGGER_EVENT_THREE
	,
#ifdef __ANDROID__
	TRIGGER_EVENT_CONTEXT_CREATED, // doom3 thread/render thread -> Android SurfaceView thread: notify OpenGL context created
	TRIGGER_EVENT_CONTEXT_DESTROYED, // doom3 thread/render thread -> Android SurfaceView thread: notify OpenGL context destroyed
	TRIGGER_EVENT_WINDOW_CREATED, // Android SurfaceView thread -> doom3/renderer thread: notify native window is created
#endif
#ifdef _MULTITHREAD
	TRIGGER_EVENT_RUN_BACKEND, // doom3 thread -> render thread: notify backend run render function
	TRIGGER_EVENT_BACKEND_FINISHED, // render thread -> doom3 thread: notify frontend rendering finished
	TRIGGER_EVENT_IMAGES_PROCESSES, // render thread -> doom3 thread: notify frontend texture's OpenGL function called
	TRIGGER_EVENT_RENDER_THREAD_FINISHED, // render thread -> doom3 thread: notify render thread finished
#endif
#ifdef _OPENSLES
	TRIGGER_EVENT_SOUND_FRONTEND_WRITE_FINISHED, // doom3 sound thread -> OpenSLES thread: frontend write data finished, and notify backend allow read data
	TRIGGER_EVENT_SOUND_BACKEND_READ_FINISHED, // OpenSLES thread -> doom3 sound thread: backend read data finished, and notify frontend allow write data
#endif
};

void				Sys_WaitForEvent(int index = TRIGGER_EVENT_ZERO);
void				Sys_TriggerEvent(int index = TRIGGER_EVENT_ZERO);

/*
==============================================================

	idSys

==============================================================
*/

class idSys
{
	public:
		virtual void			DebugPrintf(const char *fmt, ...)id_attribute((format(printf,2,3))) = 0;
		virtual void			DebugVPrintf(const char *fmt, va_list arg) = 0;

		virtual double			GetClockTicks(void) = 0;
		virtual double			ClockTicksPerSecond(void) = 0;
		virtual int			    GetProcessorId(void) = 0;
		virtual const char 	*GetProcessorString(void) = 0;
		virtual const char 	*FPU_GetState(void) = 0;
		virtual bool			FPU_StackIsEmpty(void) = 0;
		virtual void			FPU_SetFTZ(bool enable) = 0;
		virtual void			FPU_SetDAZ(bool enable) = 0;

		virtual void			FPU_EnableExceptions(int exceptions) = 0;

		virtual bool			LockMemory(void *ptr, int bytes) = 0;
		virtual bool			UnlockMemory(void *ptr, int bytes) = 0;

		virtual void			GetCallStack(address_t *callStack, const int callStackSize) = 0;
		virtual const char 	*GetCallStackStr(const address_t *callStack, const int callStackSize) = 0;
		virtual const char 	*GetCallStackCurStr(int depth) = 0;
		virtual void			ShutdownSymbols(void) = 0;

		virtual uintptr_t		DLL_Load(const char *dllName) = 0;
		virtual void 			*DLL_GetProcAddress(uintptr_t dllHandle, const char *procName) = 0;
		virtual void			DLL_Unload(uintptr_t dllHandle) = 0;
		virtual void			DLL_GetFileName(const char *baseName, char *dllName, int maxLength) = 0;

		virtual sysEvent_t		GenerateMouseButtonEvent(int button, bool down) = 0;
		virtual sysEvent_t		GenerateMouseMoveEvent(int deltax, int deltay) = 0;

		virtual void			OpenURL(const char *url, bool quit) = 0;
		virtual void			StartProcess(const char *exePath, bool quit) = 0;
#ifdef _RAVEN
	    virtual int				Milliseconds(void) = 0;
#endif
#ifdef _HUMANHEAD
        //HUMANHEAD rww
        //logitech lcd keyboard interface functions
        virtual bool			LGLCD_Valid(void) = 0;
        virtual void			LGLCD_UploadImage(unsigned char *pixels, int w, int h, bool highPriority, bool flipColor) = 0;
        //HUMANHEAD END
#endif
};

extern idSys 				*sys;

bool Sys_LoadOpenAL(void);
void Sys_FreeOpenAL(void);

#ifdef _RAVEN
#define TIME_THIS_SCOPE(name) // placeholder

#define STRINGIZE_INDIRECT(F, X) F(X)
#define STRINGIZE(X) #X
#define __LINESTR__ STRINGIZE_INDIRECT(STRINGIZE, __LINE__)
#define __FILELINEFUNC__ (__FILE__ " " __LINESTR__ " " __FUNCTION__)
#define __FUNCLINE__ ( __FUNCTION__ " " __LINESTR__ )
#endif

#ifdef __ANDROID__ //karin: sys::public expose on Android
FILE * Sys_tmpfile(void);
void Sys_SyncState(void);
void Sys_ForceResolution(void);
void Sys_Analog(int &side, int &forward, const int &KEY_MOVESPEED);
#endif

#ifdef _MULTITHREAD
bool Sys_InThread(const xthreadInfo *thread);
bool Sys_ThreadIsRunning(const xthreadInfo *thread);
#ifdef _NO_PTHREAD_CANCEL
#define THREAD_CANCELED(x) (x).threadCancel
#else
#define THREAD_CANCELED(x) false
#endif
#endif

void			Sys_Trap(void);
void			Sys_Usleep(int usec);
void			Sys_Msleep(int msec);
const char *	Sys_DLLDefaultPath(void);
uint64_t		Sys_Microseconds(void);

#endif /* !__SYS_PUBLIC__ */
