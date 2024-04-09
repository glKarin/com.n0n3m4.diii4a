/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/


// Win32
#if defined(WIN32) || defined(_WIN32)

#define BUILD_OS_ID						0

#ifdef _WIN64
//stgatilov: see idClass::ProcessEventArgPtr for info
#define CPU_EASYARGS					0
#else
#define CPU_EASYARGS					1
#endif

#define PACKED


#define PATHSEPERATOR_STR				"\\"
#define PATHSEPERATOR_CHAR				'\\'

#define ID_STATIC_TEMPLATE				static

#define ID_NOINLINE						__declspec(noinline)
//anon begin
#define ID_INLINE_EXTERN				extern inline
#define ID_FORCE_INLINE_EXTERN			extern __forceinline
//anon end

//stgatilov begin
#ifdef _WIN64
    #define __SSE__
    #define __SSE2__        //SSE/SSE2 arithmetic is always used in x64
#else
    #if _M_IX86_FP >= 1
        #define __SSE__     //at least /arch:SSE
    #endif
    #if _M_IX86_FP >= 2
        #define __SSE2__    //at least /arch:SSE2
    #endif
#endif
//stgatilov end

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
//#define ID_LITTLE_ENDIAN			1
#endif

#endif

#if defined(__APPLE__)
	//note: previously this macro was set in XCode projects, so it is used all over the code
	//here we try to detect MacOS build and automatically set it
	#define MACOS_X
#endif
// Mac OSX
#if defined(MACOS_X)

#define BUILD_OS_ID					1
#ifdef __ppc__
	#define CPU_EASYARGS				0
#elif defined(__i386__)
	#define CPU_EASYARGS				1
#endif

#ifdef __MWERKS__
#define PACKED
#include <alloca.h>
#else
#define PACKED							__attribute__((packed))
#endif

#define _alloca							alloca

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_STATIC_TEMPLATE

#define ID_INLINE_EXTERN				extern inline //anon
#define assertmem( x, y )

#define THREAD_RETURN_TYPE				void *

#endif


// Linux
#ifdef __linux__

#define BUILD_OS_ID					2

#ifdef __i386__
	#define CPU_EASYARGS				1
#elif defined(__x86_64__)
	#define CPU_EASYARGS				0
#elif defined(__ppc__)
	#define CPU_EASYARGS				0
#endif

#define _alloca							alloca

#define PACKED							__attribute__((packed))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_NOINLINE						__attribute__((noinline))

#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#define THREAD_RETURN_TYPE				void *

#endif

// FreeBSD
#ifdef __FreeBSD__

#define BUILD_OS_ID					3

#ifdef __i386__
	#define CPU_EASYARGS				1
#elif defined(__x86_64__)
	#define CPU_EASYARGS				0
#elif defined(__ppc__)
	#define CPU_EASYARGS				0
#endif

#define _alloca							alloca

#define PACKED							__attribute__((packed))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_NOINLINE						__attribute__((noinline))

#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#define THREAD_RETURN_TYPE				void *

#endif

#if !defined(ID_LITTLE_ENDIAN) && !defined(ID_BIG_ENDIAN)
	#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
		#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			#define ID_LITTLE_ENDIAN
		#endif
	#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
		#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			#define ID_BIG_ENDIAN
		#endif
	#endif
#endif

#if !defined(ID_LITTLE_ENDIAN) && !defined(ID_BIG_ENDIAN)
	#if defined(__i386__) || defined(__x86_64__)
		#define ID_LITTLE_ENDIAN		1
	#elif defined(__ppc__)
		#define ID_BIG_ENDIAN			1
	#endif
#endif

#ifdef __GNUC__
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)
#endif

#ifdef EXPLICIT_OPTIMIZATION
//stgatilov: force optimization of some function in "Debug with Inlines" MSVC configuration
#define DEBUG_OPTIMIZE_ON __pragma(optimize("gt", on))
#define DEBUG_OPTIMIZE_OFF __pragma(optimize("", on))
#else
#define DEBUG_OPTIMIZE_ON
#define DEBUG_OPTIMIZE_OFF
#endif

typedef enum {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD

	//CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_SSE							= 0x00020,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00040,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00080,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_SSSE3							= 0x00100,	// Supplemental Streaming SIMD Extentions (Core 2)
	CPUID_SSE41							= 0x00200,	// Streaming SIMD Extentions 4.1 (Penryn)
	CPUID_AVX							= 0x00400,	// AVX extenstions (SandyBridge)
	CPUID_AVX2							= 0x00800,	// AVX2 extenstions (Haswell)
	
	CPUID_FMA3							= 0x01000,	// FMA3 instruction (Haswell)
//	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000	// Denormals-Are-Zero mode (denormal source operands are set to zero)
} cpuid_t;

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
	SE_PAD_BUTTON,			// evValue is a button code, evValue2 is the down flag
	SE_PAD_AXIS,			// evValue is an axis number and evValue2 is the current state (-127 to 127)
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
	void *			evPtr;				// this must be manually freed if not NULL
} sysEvent_t;

template<class type> class idList;		// for Sys_ListFiles


void			Sys_Init( void );
void			Sys_Shutdown( void );
void			Sys_Error( const char *error, ...);
void			Sys_Quit( void );

// note that this isn't journaled...
char *			Sys_GetClipboardData( void );
void			Sys_SetClipboardData( const char *string );

// will go to the various text consoles
// NOT thread safe - never use in the async paths
void			Sys_Printf( const char *msg, ... )id_attribute((format(printf,1,2)));

// guaranteed to be thread-safe
void			Sys_DebugPrintf( const char *fmt, ... )id_attribute((format(printf,1,2)));
void			Sys_DebugVPrintf( const char *fmt, va_list arg );

// a decent minimum sleep time to avoid going below the process scheduler speeds
#define			SYS_MINSLEEP	20

// allow game to yield CPU time
// NOTE: due to SYS_MINSLEEP this is very bad portability karma, and should be completely removed
void			Sys_Sleep( int msec );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int				Sys_Milliseconds( void );

// for accurate performance testing
double			Sys_GetClockTicks( void );
double			Sys_ClockTicksPerSecond( void );

// timing equivalent to boost::posix_time::microsec_clock
// returns number of microseconds passed after 1970-Jan-01
// uses GetSystemTimeAsFileTime on Windows and gettimeofday on other platforms
uint64_t Sys_GetTimeMicroseconds( void );
#define Sys_Microseconds Sys_GetTimeMicroseconds

// stgatilov: called once on initialization to initialize CPUID info and sys_cpustring
void Sys_InitCPUID();

// returns a selection of the CPUID_* flags
cpuid_t			Sys_GetProcessorId( void );
const char *	Sys_GetProcessorString( void );

// sets the FPU precision to double
void			Sys_FPU_SetPrecision();

// sets Flush-To-Zero mode (only available when CPUID_FTZ is set)
void			Sys_FPU_SetFTZ( bool enable );

// sets Denormals-Are-Zero mode (only available when CPUID_DAZ is set)
void			Sys_FPU_SetDAZ( bool enable );

// enable/disable floating point exceptions when operations produce NaN or Inf
void			Sys_FPU_SetExceptions(bool enable);

// returns amount of drive space in path
int				Sys_GetDriveFreeSpace( const char *path );

// lock and unlock memory
bool			Sys_LockMemory( void *ptr, int bytes );
bool			Sys_UnlockMemory( void *ptr, int bytes );

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes );

// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
uintptr_t		Sys_DLL_Load(const char *dllName);
void *			Sys_DLL_GetProcAddress(uintptr_t dllHandle, const char *procName);
void			Sys_DLL_Unload(uintptr_t dllHandle);

// event generation
void			Sys_GenerateEvents( void );
sysEvent_t		Sys_GetEvent( void );
void			Sys_ClearEvents( void );
void			Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

// input is tied to windows, so it needs to be started up and shut down whenever
// the main window is recreated
void			Sys_InitInput( void );
void			Sys_ShutdownInput( void );
void			Sys_InitScanTable( void );
const unsigned char *Sys_GetScanTable( void );
unsigned char	Sys_GetConsoleKey( bool shifted );
// map a scancode key to a char
// does nothing on win32, as SE_KEY == SE_CHAR there
// on other OSes, consider the keyboard mapping
unsigned char	Sys_MapCharForKey( int key );

// keyboard input polling
int				Sys_PollKeyboardInputEvents( void );
int				Sys_ReturnKeyboardInputEvent( const int n, int &ch, bool &state );
void			Sys_EndKeyboardInputEvents( void );

// mouse input polling
int				Sys_PollMouseInputEvents( void );
int				Sys_ReturnMouseInputEvent( const int n, int &action, int &value );
void			Sys_EndMouseInputEvents( void );

// when the console is down, or the game is about to perform a lengthy
// operation like map loading, the system can release the mouse cursor
// when in windowed mode
void			Sys_GrabMouseCursor( bool grabIt );
//stgatilov #4768: apply OS adjustments to raw mouse cursor movement (for one frame)
//in case of Windows: sensitivity + acceleration from Control Panel, DPI scaling
//it is used to make mouse in menu GUIs feel more like in OS
void			Sys_AdjustMouseMovement(float &dx, float &dy);

void			Sys_ShowWindow( bool show );
bool			Sys_IsWindowVisible( void );
void			Sys_ShowConsole( int visLevel, bool quitOnClose );
bool			Sys_GetCurrentMonitorResolution( int &width, int &height );


void			Sys_Mkdir( const char *path );
ID_TIME_T		Sys_FileTimeStamp( FILE *fp );
// NOTE: do we need to guarantee the same output on all platforms?
const char *	Sys_TimeStampToStr( ID_TIME_T timeStamp );
const char *	Sys_DefaultBasePath( void );
const char *	Sys_HomeSavePath( void );
const char *	Sys_DefaultSavePath( void );
const char *	Sys_ModSavePath( void ); // greebo: added this for TDM mission handling
const char *	Sys_EXEPath( void );

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles( const char *directory, const char *extension, idList<class idStr> &list );

// know early if we are performing a fatal error shutdown so the error message doesn't get lost
void			Sys_SetFatalError( const char *error );

// display perference dialog
void			Sys_DoPreferences( void );


struct debugStackFrame_t {
	static const int MAX_LEN = 88;
	void *pointer;
	char functionName[MAX_LEN];
	char fileName[MAX_LEN];
	int lineNumber;
};

void Sys_CaptureStackTrace(int ignoreFrames, uint8_t *data, int &len);
int Sys_GetStackTraceFramesCount(uint8_t *data, int len);
void Sys_DecodeStackTrace(uint8_t *data, int len, debugStackFrame_t *frames);

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

class idPort {
public:
				idPort();				// this just zeros netSocket and port
	virtual		~idPort();

	// if the InitForPort fails, the idPort.port field will remain 0
	bool		InitForPort( int portNumber );
	int			GetPort( void ) const { return bound_to.port; }
	netadr_t	GetAdr( void ) const { return bound_to; }
	void		Close();

	bool		GetPacket( netadr_t &from, void *data, int &size, int maxSize );
	bool		GetPacketBlocking( netadr_t &from, void *data, int &size, int maxSize, int timeout );
	void		SendPacket( const netadr_t to, const void *data, int size );

	int			packetsRead;
	int			bytesRead;

	int			packetsWritten;
	int			bytesWritten;

private:
	netadr_t	bound_to;		// interface and port
	int			netSocket;		// OS specific socket
};

class idTCP {
public:
				idTCP();
	virtual		~idTCP();

	// if host is host:port, the value of port is ignored
	bool		Init( const char *host, short port );
	void		Close();

	// starts listening for incoming connections on specified port
	bool		Listen( short port );
	// if there is pending client, create direct connection with him
	// note: caller must delete returned object!
	idTCP*		Accept();

	bool		IsAlive() const { return fd > 0; }
	const netadr_t &GetAddress() const { return address; }

	// returns -1 on failure (and closes socket)
	// those are non blocking, can be used for polling
	// there is no buffering, you are not guaranteed to Read or Write everything in a single call
	// (specially on win32, see recv and send documentation)
	int			Read( void *data, int size );
	int			Write( const void *data, int size );

private:
	netadr_t	address;		// remote address
	size_t		fd;				// OS specific socket
};

				// parses the port number
				// can also do DNS resolve if you ask for it.
				// NOTE: DNS resolve is a slow/blocking call, think before you use
				// ( could be exploited for server DoS )
bool			Sys_StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve );
const char *	Sys_NetAdrToString( const netadr_t a );
bool			Sys_IsLANAddress( const netadr_t a );
bool			Sys_CompareNetAdrBase( const netadr_t a, const netadr_t b );

void			Sys_InitNetworking( void );
void			Sys_ShutdownNetworking( void );


/*
==============================================================

	Multi-threading

==============================================================
*/

// find the name of the calling thread
// if index != NULL, set the index in g_threads array (use -1 for "main" thread)
const char *		Sys_GetThreadName( int *index = 0 );

void				Sys_EnterCriticalSection( int index = CRITICAL_SECTION_ZERO );
void				Sys_LeaveCriticalSection( int index = CRITICAL_SECTION_ZERO );

const int MAX_TRIGGER_EVENTS		= 4;

enum {
	TRIGGER_EVENT_ZERO = 0,
	TRIGGER_EVENT_ONE,
	TRIGGER_EVENT_TWO,
	TRIGGER_EVENT_THREE
#ifdef __ANDROID__ //karin: Surface change lock
,
    TRIGGER_EVENT_WINDOW_CREATED, // Android SurfaceView thread -> doom3/renderer thread: notify native window is set
    TRIGGER_EVENT_WINDOW_DESTROYED, // doom3 thread/render thread -> Android SurfaceView thread: notify released OpenGL context
#endif
};

void				Sys_WaitForEvent( int index = TRIGGER_EVENT_ZERO );
void				Sys_TriggerEvent( int index = TRIGGER_EVENT_ZERO );

/*
==============================================================

	idSys

==============================================================
*/

class idSys {
public:
	virtual void			DebugPrintf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) = 0;

	virtual double			GetClockTicks( void ) = 0;
	virtual double			ClockTicksPerSecond( void ) = 0;
	virtual cpuid_t			GetProcessorId( void ) = 0;
	virtual const char *	GetProcessorString( void ) = 0;
	virtual void			FPU_SetFTZ( bool enable ) = 0;
	virtual void			FPU_SetDAZ( bool enable ) = 0;
	virtual void			FPU_SetExceptions(bool enable) = 0;

	// stgatilov #4550: should be called when new thread starts: sets FPU properties
	virtual void			ThreadStartup() = 0;
	// stgatilov #4550: should be called regularly in every thread: updates FPU properties after cvar changes
	//                  also makes sure Tracy receives thread name even if it starts late
	virtual void			ThreadHeartbeat( const char *threadName ) = 0;

	virtual bool			LockMemory( void *ptr, int bytes ) = 0;
	virtual bool			UnlockMemory( void *ptr, int bytes ) = 0;

	virtual uintptr_t		DLL_Load(const char *dllName) = 0;
	virtual void *			DLL_GetProcAddress(uintptr_t dllHandle, const char *procName) = 0;
	virtual void			DLL_Unload(uintptr_t dllHandle) = 0;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) = 0;

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) = 0;
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) = 0;

	virtual void			OpenURL( const char *url, bool quit ) = 0;
	virtual void			StartProcess( const char *exePath, bool quit ) = 0;
};

extern idSys *				sys;

#ifdef __ANDROID__ //karin: sys::public expose on Android
FILE * Sys_tmpfile(void);
void Sys_SyncState(void);
void Sys_ForceResolution(void);
void Sys_Analog(int &side, int &forward, const int &KEY_MOVESPEED);
#endif

#endif /* !__SYS_PUBLIC__ */

typedef unsigned long long uint64;
