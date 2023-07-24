// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/

#include "../common/common.h"
#include "keynum.h"

class sdSysEvent;
class idStr;
class idWStr;
class sdLogitechLCDSystem;

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
	AXIS_1,
	AXIS_2,
	AXIS_3,
	AXIS_4,
	AXIS_5,
	AXIS_6,
	AXIS_7,
	AXIS_8,
	MAX_CONTROLLER_AXIS,
} controllerAxis_t;

typedef enum {
	SE_NONE,				// evTime is still valid
	SE_KEY,					// value is a scan code, value2 is the down flag
	SE_CHAR,				// value is a scan code, value2 is an ASCII char
	SE_MOUSE,				// value and value2 are relative signed x / y moves
	SE_MOUSE_BUTTON,		// value is a mouse button number
	SE_CONTROLLER_MOUSE,	// like a mouse movement, but caused by a controller, will not be used by usercmdgen
	SE_CONTROLLER_AXIS,		// value is an axis number and value2 is the current state (-127 to 127)
	SE_CONTROLLER_BUTTON,	// value is the button number, value2 is the controller hash << 1 OR'ed with the down flag
	SE_CONSOLE,				// evPtr is a char*, from typing something at a non-game console
	SE_GUI,					// an event generated specifically for guis
	SE_IME,					// value is the event number, 
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

struct sysTime_t {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
};

// values are in KB, except for memoryLoad, which is a percent
struct sysMemoryStats_t {
	int memoryLoad;
	int totalPhysical;
	int availPhysical;
	int totalPageFile;
	int availPageFile;
	int totalVirtual;
	int availVirtual;
	int availExtendedVirtual;
};

struct sysProcessMemoryStats_t {
	sysMemoryStats_t globalStats;
	int memoryLoad;
};

#define HT_NOT_CAPABLE				0
#define HT_ENABLED					1
#define HT_DISABLED					2
#define HT_SUPPORTED_NOT_ENABLED	3
#define HT_CANNOT_DETECT			4

struct cpuInfo_t {
	int logicalNum;				// logical number of CPUs
	int physicalNum;			// physical number of CPUs
	int hyperThreadedStatus;	// one of the above HT_ constants
};

typedef size_t	address_t;

template<class type> class idList;		// for Sys_ListFiles

void			Sys_Init( void );
void			Sys_Shutdown( void );
void			Sys_Error( const char *error, ...);
void			Sys_Quit( void );

bool			Sys_AlreadyRunning( void );

void			Sys_CPUInfo( cpuInfo_t& info );

// note that this isn't journaled...
wchar_t *		Sys_GetClipboardData( void );
void			Sys_SetClipboardData( const wchar_t *string );

// will go to the various text consoles
// NOT thread safe - never use in the async paths
void			Sys_Printf( const char *msg, ... );

// guaranteed to be thread-safe
void			Sys_DebugPrintf( const char *fmt, ... );
void			Sys_DebugVPrintf( const char *fmt, va_list arg );

// a decent minimum sleep time to avoid going below the process scheduler speeds
#define			SYS_MINSLEEP	20

// allow game to yield CPU time
// NOTE: due to SYS_MINSLEEP this is very bad portability karma, and should be completely removed
void			Sys_Sleep( int msec );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int				Sys_Milliseconds( void );
unsigned long	Sys_TimeBase( void );

time_t			Sys_TimeDiff( const sysTime_t& from, const sysTime_t& to );
void			Sys_SecondsToTime( time_t t, sysTime_t& out, bool localTime = false );
const char*		Sys_TimeToStr( const sysTime_t& t );
const char*		Sys_SecondsToStr( const time_t t, bool localTime = false );
const char*		Sys_TimeToSystemStr( const sysTime_t& sysTime );
const char*		Sys_TimeAndDateToSystemStr( const sysTime_t& sysTime );


// for getting current system (real world) time
time_t			Sys_RealTime( sysTime_t* sysTime );

// for accurate performance testing
double			Sys_GetClockTicks( void );
double			Sys_GetClockTicksNoFlush( void );
double			Sys_ClockTicksPerSecond( void );

// returns a selection of the CPUID_* flags
cpuid_t			Sys_GetProcessorId( void );
const char *	Sys_GetProcessorString( void );

// returns true if the FPU stack is empty
bool			Sys_FPU_StackIsEmpty( void );

// empties the FPU stack
void			Sys_FPU_ClearStack( void );

// returns the FPU state as a string
const char *	Sys_FPU_GetState( void );

// enables the given FPU exceptions
void			Sys_FPU_EnableExceptions( int exceptions );

// sets the FPU precision
void			Sys_FPU_SetPrecision( int precision );

// sets the FPU rounding mode
void			Sys_FPU_SetRounding( int rounding );

// sets Flush-To-Zero mode (only available when CPUID_FTZ is set)
void			Sys_FPU_SetFTZ( bool enable );

// sets Denormals-Are-Zero mode (only available when CPUID_DAZ is set)
void			Sys_FPU_SetDAZ( bool enable );

// returns amount of system ram
int				Sys_GetSystemRam( void );

// returns amount of video ram
int				Sys_GetVideoRam( void );

bool			Sys_GetGfxDeviceIdentification( idStr &vendorID, idStr &deviceID );

// returns amount of drive space in path
int				Sys_GetDriveFreeSpace( const char *path );

// returns memory stats
void			Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats );
void			Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats );
void			Sys_GetProcessMemoryStatus( sysProcessMemoryStats_t& stats );

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes );

// allows retrieving the call stack at execution points
// should work on all OSes, operate on the current stack. returns the correct stack item count on Linux, and the max stack size on _WIN32
int				Sys_GetCurCallStack( address_t *callStack, const int callStackSize );
// those use a static buffer to write out the stack
const char *	Sys_GetCurCallStackAddressStr( int depth );
const char *	Sys_GetCurCallStackStr( int depth );	// returns the same as AddressStr when/if symbols are not available
// only available on _WIN32 atm
const char *	Sys_GetCallStackStr( const address_t *callStack, const int callStackSize );
const char *	Sys_GetFunctionName( const address_t function );
const char *	Sys_GetFunctionSourceFile( const address_t function );
void			Sys_ShutdownSymbols( void );

// passing the address of the entry point to the game module, used for backtracing info by the posix code
void			Sys_SetGameOffset( uintptr_t gameAddress );

// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
void *			Sys_DLL_Load( const char *dllName, bool checkFullPathMatch );
void *			Sys_DLL_GetProcAddress( void* dllHandle, const char *procName );
void			Sys_DLL_Unload( void* dllHandle );

// event generation
void			Sys_GenerateEvents( void );
void			Sys_PumpEvents( void );

// input is tied to windows, so it needs to be started up and shut down whenever 
// the main window is recreated
void			Sys_InitInput( void );
void			Sys_ShutdownInput( void );

void			Sys_ShowWindow( bool show );
bool			Sys_IsWindowVisible( void );
bool			Sys_IsWindowFocused( void );
void			Sys_ShowConsole( int visLevel, bool quitOnClose );
void			Sys_UpdateConsole();

void			Sys_Mkdir( const char *path );
int				Sys_Rmdir( const char *path );
bool			Sys_CopyFile( const char* fromOSPath, const char* toOSPath, bool overwrite = false );
long			Sys_FileTimeStamp( FILE *fp );

const char *	Sys_DefaultCDPath( void );
const char *	Sys_DefaultBasePath( void );
const char *	Sys_DefaultSavePath( void );
const char *	Sys_DefaultUserPath( void );
const char *	Sys_EXEPath( void );

void			Sys_ShowSplashScreen( bool show );

void			Sys_GetDesktopSize( int& width, int& height );

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles( const char *directory, const char *extension, idList< idStr > &list );

// know early if we are performing a fatal error shutdown so the error message doesn't get lost
void			Sys_SetFatalError( const char *error );

// display preference dialog
void			Sys_DoPreferences( void );

cpuid_t			Sys_GetCPUId( void );
int				Sys_CPUCount( int &logicalNum, int &physicalNum );

int				Sys_GenerateDiag( const char *fileName );

#ifdef _WIN32
#define			Sys_WritePid()
#else
void			Sys_WritePid();
#endif

const char*		Sys_GetEnv( const char* varName );

FILE*			Sys_TempFile( void );

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

struct netadr_t {
	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned short	port;
};

ID_INLINE bool operator==( const netadr_t& lhs, const netadr_t& rhs ) {
	return ( ( lhs.type == rhs.type ) &&
			 ( lhs.ip[ 0 ] == rhs.ip[ 0 ] ) &&
			 ( lhs.ip[ 1 ] == rhs.ip[ 1 ] ) &&
			 ( lhs.ip[ 2 ] == rhs.ip[ 2 ] ) &&
			 ( lhs.ip[ 3 ] == rhs.ip[ 3 ] ) &&
			 ( lhs.port == rhs.port ) );
}

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

	void		SetSilent( bool silent );
	bool		GetSilent() const;

	int			packetsRead;
	int			bytesRead;

	int			packetsWritten;
	int			bytesWritten;

private:
	netadr_t	bound_to;		// interface and port
	int			netSocket;		// OS specific socket

	bool		silent;			// don't emit anything for a while
};

ID_INLINE void idPort::SetSilent( bool silent ) { this->silent = silent; }
ID_INLINE bool idPort::GetSilent() const { return silent; }


				// parses the port number
				// can also do DNS resolve if you ask for it.
				// NOTE: DNS resolve is a slow/blocking call, think before you use
				// ( could be exploited for server DoS )
bool			Sys_StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve );
const char *	Sys_NetAdrToString( const netadr_t& a );
bool			Sys_NetAdrToHostName( const netadr_t& a, char** s );
bool			Sys_IsLANAddress( const netadr_t& a );
bool			Sys_CompareNetAdrBase( const netadr_t& a, const netadr_t& b );

int				Sys_GetLocalIPCount( void );
const char *	Sys_GetLocalIP( int i );

void			Sys_InitNetworking( void );
void			Sys_ShutdownNetworking( void );

typedef enum { MB_INFORMATION, MB_WARNING, MB_FATALERROR } messageBoxType_t;
int				Sys_MessageBox( const char* title, const char* buffer, messageBoxType_t type );

sdLogitechLCDSystem* Sys_GetLogitechLCDSystem( void );

void			Sys_SetConsoleName( const char* name );

// read proxy information from environment
#define			MAX_PROXY_LENGTH 128
bool			Sys_GetHTTPProxyAddress( char proxy[ MAX_PROXY_LENGTH ] );

void			Sys_SetSystemLocale();
void			Sys_SetDefaultLocale();

int				Sys_GetLanguageIndex( const char* langName );


/*
==============================================================

	Multi-threading

==============================================================
*/

typedef unsigned int (*xthread_t)( void * );

typedef enum {
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
} threadPriority_e;

typedef struct {
	const char *	name;
	int				threadHandle;
	unsigned long	threadId;
} xthreadInfo;

enum {
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE,		// sound -> network syncing
	CRITICAL_SECTION_FOUR,
	CRITICAL_SECTION_FIVE,
	CRITICAL_SECTION_SIX,
	CRITICAL_SECTION_SINGLETON	// sdSingleton, thread event creation
};

/*
==============================================================

Performance timers

==============================================================
*/

// This class provides some circular buffer stuff, the platform dependent one should then implement the actual performance queries
class sdPerformanceQuery { 

public:
	virtual ~sdPerformanceQuery () {}

	virtual int GetCapacity( void ) const = 0;
	virtual void SetCapacity( int capacity ) = 0;
	virtual int GetSize( void ) const = 0;
	virtual float GetMin( void ) const = 0;
	virtual float GetMax( void ) const = 0;

	/*
		Index 0 is always the most recent one, higher indexes are progressively older
	*/
	virtual float GetSample( int i ) const = 0;

	/*
		Call this every frame or whatever, to add a new sample to the front of the list
		Should return false if sampling failed
	*/
	virtual bool Sample( void ) = 0;
};

// Fixme: Not all systems may have identical semantics for performance query results? 
// and some will definately be unsupported on certain systems
typedef enum {
	PQT_CPU0,
	PQT_CPU1,
	PQT_CPU2,
	PQT_CPU3,

	PQT_CPU_TOTAL,
	PQT_GPU_FPS,
	PQT_GPU_IDLE,
	PQT_GPU_AGPMEM,
	PQT_GPU_VIDMEM,
	PQT_GPU_DRIVERWAIT,
	PQT_GPU_VS,
	PQT_GPU_PS,
	PQT_GPU_TEX,
	PQT_GPU_ROP,
	PQT_GPU_TEXPS,
	PQT_GPU_TRIS,
	PQT_GPU_VERTS,
	PQT_GPU_PIXELS,
	PQT_GPU_FASTZ,

	PQT_OSDEPENDENT,
	PQT_END
} sdPerformanceQueryType;

/*
==============================================================

	idSys

==============================================================
*/

class idKeyboard;
class idMouse;
class sdControllerManager;
class sdIME;

class idSys {
public:
	virtual					~idSys( void ) { }

							// Init and Shutdown are to properly deal with idLib dependant code
	virtual void			Init( void ) = 0;
	virtual void			PostGameInit( void ) = 0;
	virtual void			Shutdown( void ) = 0;

	virtual void			DebugPrintf( const char *fmt, ... ) = 0;
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) = 0;

	virtual void			GetCPUInfo( cpuInfo_t& info ) = 0;
	virtual double			GetClockTicks( void ) = 0;
	virtual double			ClockTicksPerSecond( void ) = 0;
	virtual cpuid_t			GetProcessorId( void ) = 0;
	virtual void			Sleep( int msec ) = 0;
	virtual int				Milliseconds() = 0;
	virtual time_t			RealTime( sysTime_t* sysTime ) = 0;
	virtual const char*		TimeToSystemStr( const sysTime_t& sysTime ) = 0;
	virtual const char*		TimeAndDateToSystemStr( const sysTime_t& sysTime ) = 0;
	virtual time_t			TimeDiff( const sysTime_t& from, const sysTime_t& to ) = 0;
	virtual void			SecondsToTime( const time_t t, sysTime_t& out, bool localTime = false ) = 0;
	virtual const char *	GetProcessorString( void ) = 0;
	virtual const char *	TimeToStr( const sysTime_t& t ) = 0;
	virtual const char *	SecondsToStr( const time_t t, bool localTime = false ) = 0;
	virtual const char *	FPU_GetState( void ) = 0;
	virtual bool			FPU_StackIsEmpty( void ) = 0;
	virtual void			FPU_SetFTZ( bool enable ) = 0;
	virtual void			FPU_SetDAZ( bool enable ) = 0;

	virtual void			FPU_EnableExceptions( int exceptions ) = 0;

	virtual void			GetCurrentMemoryStatus( sysMemoryStats_t &stats ) = 0;
	virtual void			GetExeLaunchMemoryStatus( sysMemoryStats_t &stats ) = 0;
	virtual void			GetProcessMemoryStatus( sysProcessMemoryStats_t &stats ) = 0;

	virtual void			GetCurCallStack( address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCurCallStackStr( int depth ) = 0;
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetFunctionName( const address_t function ) = 0;
	virtual const char *	GetFunctionSourceFile( const address_t function ) = 0;
	virtual void			ShutdownSymbols( void ) = 0;

	virtual void *			DLL_Load( const char *dllName, bool checkFullPathMatch ) = 0;
	virtual void *			DLL_GetProcAddress( void* dllHandle, const char *procName ) = 0;
	virtual void			DLL_Unload( void* dllHandle ) = 0;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) = 0;

	virtual const char *	EXEPath( void ) = 0;

	virtual const sdSysEvent*	GenerateBlankEvent( void ) = 0;
	virtual const sdSysEvent*	GenerateCharEvent( int ch ) = 0;
	virtual const sdSysEvent*	GenerateKeyEvent( keyNum_e key, bool down ) = 0;
	virtual const sdSysEvent*	GenerateMouseButtonEvent( int button, bool down ) = 0;
	virtual const sdSysEvent*	GenerateMouseMoveEvent( int deltax, int deltay ) = 0;
	virtual const sdSysEvent*	GenerateGuiEvent( int value ) = 0;
	virtual void				FreeEvent( const sdSysEvent* event ) = 0;
	virtual const sdSysEvent*	GetEvent( void ) = 0;
	virtual void				QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) = 0;
	virtual void				ClearEvents( void ) = 0;

	virtual void			OpenURL( const char *url, bool quit ) = 0;
	virtual void			StartProcess( const char *exePath, bool quit ) = 0;

	virtual long			File_TimeStamp( FILE* f ) = 0;
	virtual int				File_Stat( const char* OSPath ) = 0;

	virtual int				MessageBox( const char* title, const char* buffer, messageBoxType_t type ) = 0;

	virtual void			ProcessOSEvents() = 0;

	virtual sdPerformanceQuery*	 GetPerformanceQuery( sdPerformanceQueryType pqType ) = 0;
	virtual void			 CollectPerformanceData( void ) = 0;

	virtual idWStr			GetClipboardData( void ) = 0;
	virtual void			SetClipboardData( const wchar_t *string ) = 0;

	virtual void			SetServerInfo( const char* key, const char* value ) = 0;
	virtual void			FlushServerInfo( void ) = 0;

	virtual void			InitInput() = 0;
	virtual void			ShutdownInput() = 0;

	virtual idKeyboard&		Keyboard() = 0;
	virtual idMouse&		Mouse() = 0;

	virtual sdIME&			IME() = 0;

	// switch to the user's locale
	virtual void			SetSystemLocale() = 0;

	// switch to the default C locale
	virtual void			SetDefaultLocale() = 0;

	virtual sdControllerManager&	GetControllerManager() = 0;
	virtual sdLogitechLCDSystem*	GetLCDSystem( void ) = 0;

	virtual const char *	NetAdrToString( const netadr_t& a ) const = 0;
	virtual bool			IsLANAddress( const netadr_t& a ) const = 0;
	virtual bool			StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve ) const = 0;

	virtual int				GetGUID( unsigned char* guid, const int len ) const = 0;
};

extern idSys *				sys;

#endif /* !__SYS_PUBLIC__ */
