
#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__


/*
===============================================================================

	Non-portable system services.

===============================================================================
*/


// Win32
#ifdef _WINDOWS 

#define	BUILD_STRING					"win-x86"
#define BUILD_OS_ID						0
#define	CPUSTRING						"x86"
#define CPU_EASYARGS					1

#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define ID_INLINE						__forceinline
#define ID_STATIC_TEMPLATE				static

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

#endif

#ifdef __GNUC__
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)  
#endif

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)

#include <sys/types.h>

#if __GNUC__ < 4
#include "osx/apple_bool.h"
#endif

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
#else
#define PACKED							__attribute__((packed))
#endif

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)alloca( (x)+15 )) + 15) & ~15))

#define __cdecl
#define ASSERT							assert

#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#endif


// Linux
#ifdef __linux__

#ifdef __i386__
	#define	BUILD_STRING				"linux-x86"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#elif defined(__ppc__)
	#define	BUILD_STRING				"linux-ppc"
	#define CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#endif

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)alloca( (x)+15 )) + 15) & ~15))

#define ALIGN16( x )					x
#define PACKED							__attribute__((packed))

#define __cdecl
#define ASSERT							assert

#define ID_INLINE						inline
#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

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
	CPUID_DAZ							= 0x08000,	// Denormals-Are-Zero mode (denormal source operands are set to zero)
	CPUID_EM64T							= 0x10000,	// 64-bit Memory Extensions
	CPUID_PENTIUMM						= 0x20000,	// Pentium M technology - high performance per MHz
#ifdef MACOS_X
	CPUID_PPC							= 0x40000,	// PowerPC G4/G5
#endif
// RAVEN BEGIN
	CPUID_XENON							= 0x80000	// Xenon PPC processor
// RAVEN END
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
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an ascii char
	SE_MOUSE,				// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,		// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
// RAVEN BEGIN
// rjohnson: debug event overflow stuff
	,
	SE_MAX
// RAVEN END
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

// RAVEN BEGIN
// rjohnson: new joystick code
#define MAX_AXIS_RANGE	127
#define JOY_TO_CURSOR_SPEED		( idMath::M_MS2SEC * common->GetUserCmdMSec() * 1.5f * 140.f )

typedef enum {
	AXIS_LEFT_HORIZONTAL,
	AXIS_LEFT_VERTICAL,
	AXIS_RIGHT_HORIZONTAL,
	AXIS_RIGHT_VERTICAL,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	J_ACTION_BUTTON_LEFT_SHOULDER,		// K_JOY1
	J_ACTION_BUTTON_RIGHT_SHOULDER,		// K_JOY2
	J_ACTION_BUTTON_A,					// K_JOY3
	J_ACTION_BUTTON_B,					// K_JOY4
	J_ACTION_BUTTON_Y,					// K_JOY5
	J_ACTION_BUTTON_X,					// K_JOY6
	J_ACTION_BUTTON_START,				// K_JOY7
	J_ACTION_BUTTON_BACK,				// K_JOY8
	J_ACTION_BUTTON_DPAD_UP,			// K_JOY9
	J_ACTION_BUTTON_DPAD_DOWN,			// K_JOY10
	J_ACTION_BUTTON_DPAD_RIGHT,			// K_JOY11
	J_ACTION_BUTTON_DPAD_LEFT,			// K_JOY12
	J_ACTION_BUTTON_AXIS_LEFT,			// K_JOY13
	J_ACTION_BUTTON_AXIS_RIGHT,			// K_JOY14
	J_ACTION_BUTTON_LEFT_TRIGGER,		// K_JOY16
	J_ACTION_BUTTON_RIGHT_TRIGGER,		// K_JOY15

	J_DELTA_LEFT_HORIZONTAL,
	J_DELTA_LEFT_VERTICAL,
	J_DELTA_RIGHT_HORIZONTAL,
	J_DELTA_RIGHT_VERTICAL,
	
	J_ACTION_BUTTON_GARBAGE,
} sys_jEvents;
// RAVEN END

typedef struct sysEvent_s {
	sysEventType_t	evType;
	int				evValue;
	int				evValue2;
	int				evPtrLength;		// bytes of data pointed to by evPtr, for journaling
	void *			evPtr;				// this must be manually freed if not NULL
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

void			Sys_Init( void );
void			Sys_Shutdown( void );
void			Sys_Error( const char *error, ...);
void			Sys_Quit( void );

bool			Sys_AlreadyRunning( void );

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

// returns whether the main rendering window has focus
bool			Sys_IsAppActive( void );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int				Sys_Milliseconds( void );

// for accurate performance testing
double			Sys_GetClockTicks( void );
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

// returns amount of drive space in path
int				Sys_GetDriveFreeSpace( const char *path );

// returns memory stats
void			Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats );
void			Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats );

// lock and unlock memory
bool			Sys_LockMemory( void *ptr, int bytes );
bool			Sys_UnlockMemory( void *ptr, int bytes );

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes );

// allows retrieving the call stack at execution points
void			Sys_GetCallStack( address_t *callStack, const int callStackSize );
const char *	Sys_GetCallStackStr( const address_t *callStack, const int callStackSize );
const char *	Sys_GetCallStackCurStr( int depth );
const char *	Sys_GetCallStackCurAddressStr( int depth );
void			Sys_ShutdownSymbols( void );

#ifdef _LOAD_DLL
// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
int				Sys_DLL_Load( const char *dllName );
void *			Sys_DLL_GetProcAddress( int dllHandle, const char *procName );
void			Sys_DLL_Unload( int dllHandle );
#endif // _LOAD_DLL

// event generation
void			Sys_GenerateEvents( void );
sysEvent_t		Sys_GetEvent( void );
void			Sys_ClearEvents( void );

// input is tied to windows, so it needs to be started up and shut down whenever 
// the main window is recreated
void			Sys_InitInput( void );
void			Sys_ShutdownInput( void );
// keyboard input polling
int				Sys_PollKeyboardInputEvents( void );
int				Sys_ReturnKeyboardInputEvent( const int n, int &ch, bool &state );
void			Sys_EndKeyboardInputEvents( void );
int				Sys_MapKey( unsigned long key, unsigned short wParam );

// mouse input polling
int				Sys_PollMouseInputEvents( void );
int				Sys_ReturnMouseInputEvent( const int n, int &action, int &value );
void			Sys_EndMouseInputEvents( void );

// RAVEN BEGIN
// ksergent: joystick input polling
int				Sys_PollJoystickInputEvents( void );
bool			Sys_IsJoystickEnabled( void );
bool			Sys_IsJoystickConnected( int index );
int				Sys_ReturnJoystickInputEvent( const int n, int &action, int &value );
void			Sys_EndJoystickInputEvents( void );
// RAVEN END

// when the console is down, or the game is about to perform a lengthy
// operation like map loading, the system can release the mouse cursor
// when in windowed mode
bool			Sys_IsWindowVisible( void );
void			Sys_Mkdir( const char *path );

// RAVEN BEGIN
// jscott: thread handling
void			Sys_StartAsyncThread( void );
void			Sys_EndAsyncThread( void );

// jscott: VTune interface
#ifndef _FINAL
void			Sys_StartProfiling( void );
void			Sys_StopProfiling( void );
#endif
// RAVEN END

// NOTE: do we need to guarantee the same output on all platforms?
const char *	Sys_TimeStampToStr( long timeStamp );
const char *	Sys_DefaultCDPath( void );
const char *	Sys_DefaultBasePath( void );
const char *	Sys_DefaultSavePath( void );
const char *	Sys_EXEPath( void );

// for getting current system (real world) time
int				Sys_RealTime( sysTime_t* sysTime );

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles( const char *directory, const char *extension, idList<class idStr> &list );

// RAVEN BEGIN
// rjohnson: added block
bool			Sys_AppShouldSleep		( void );
// RAVEN END

/*
==============================================================

	Networking

==============================================================
*/

typedef enum {
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
// RAVEN BEGIN
// rjohnson: add fake clients
	NA_FAKE,
// RAVEN END
	NA_GAME,				// bots, etc
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
				~idPort();

	// if the InitForPort fails, the idPort.port field will remain 0
// RAVEN BEGIN
// asalmon: option for xbox to create a VDP socket
#ifdef _XBOX
	bool		InitForPort( int portNumber, bool vdp = false );
#else
	bool		InitForPort( int portNumber );
#endif
// RAVEN END
	int			GetPort( void ) const { return port; }
// RAVEN BEGIN
// amccarthy: For Xbox this needs to be an unsigned int
#ifdef _XBOX
	unsigned int GetSocket( void ) const { return netSocket; }
#endif
// RAVEN END
	void		Close();

	bool		GetPacket( netadr_t &from, void *data, int &size, int maxSize );
	bool		GetPacketBlocking( netadr_t &from, void *data, int &size, int maxSize, int timeout );
	void		SendPacket( const netadr_t to, const void *data, int size );

//RAVEN BEGIN
//asalmon: second version of sendPacket for Xbox avoids netadr_t
#ifdef _XBOX
	bool		SendPacketVDP( const struct sockaddr *to, const void *data, int size );
#endif
//RAVEN END

	void		GetTrafficStats(  int &bytesSent, int &packetsSent, int &bytesReceived, int &packetsReceived ) const;

	void		SetSilent( bool silent );
	bool		GetSilent( void ) const;

private:
	int			packetsRead;
	int			bytesRead;

	int			packetsWritten;
	int			bytesWritten;

	int			port;			// UDP port
//RAVEN BEGIN
//amccarthy: For Xbox this needs to be an unsigned int
#ifdef _XBOX
	unsigned int			netSocket;		// OS specific socket
#else
	int						netSocket;
#endif
//RAVEN END

	bool		silent;			// don't emit anything for a while
};

/*
===============
idPort::GetTrafficStats
===============
*/
ID_INLINE void idPort::GetTrafficStats(  int &_bytesSent, int &_packetsSent, int &_bytesReceived, int &_packetsReceived ) const {
	_bytesSent = bytesWritten;
	_packetsSent = packetsWritten;
	_bytesReceived = bytesRead;
	_packetsReceived = packetsRead;
}

/*
===============
idPort::SetSilent
===============
*/
ID_INLINE void idPort::SetSilent( bool _silent ) { silent = _silent; }

/*
===============
idPort::GetSilent
===============
*/
ID_INLINE bool idPort::GetSilent( void ) const { return silent; }

class idTCP;
class idTCPServer;

const int IDPOLL_READ	= (1<<0);
const int IDPOLL_WRITE	= (1<<1);
const int IDPOLL_ERROR	= (1<<2);

class idPoller {
public:
				idPoller();

	void		Clear( void );

	// will replace existing entries
	void		Add( int fd, int which = IDPOLL_READ );
	void		Add( const idTCP &tcp, int which = IDPOLL_READ );
	void		Add( const idTCPServer &tcp, int which = IDPOLL_READ );

	void		Remove( int fd ) { Add(fd, 0); }
	void		Remove( const idTCP &tcp ) { Add(tcp, 0); }
	void		Remove( const idTCPServer &serv ) { Add(serv, 0); }

	// returns IDPOLL_ flags
	int			Check( int fd );
	int			Check( const idTCP &tcp );
	int			Check( const idTCPServer &serv );

	// returns the number of fds set, timeout is in ms, <0 is forever
	int			Poll( int timeout = -1 );

private:
	int			max_fd;
	fd_set		readfds, writefds, exceptfds;
	fd_set		rreadfds, rwritefds, rexceptfds;
};

class idTCPServer {
public:
				idTCPServer();
	virtual		~idTCPServer();

	bool		Listen( const char *net_interface, short port );
	bool		Accept( idTCP &client );
	void		Close();

	const netadr_t &GetAddress( void ) const { return address; }

private:
	netadr_t	address;		// local address
	int fd;

	friend void idPoller::Add( const idTCPServer &serv, int which );
	friend void idPoller::Remove( const idTCPServer &serv );
	friend int idPoller::Check( const idTCPServer &serv );
};

class idTCP {
public:
				idTCP();
				idTCP( const netadr_t &address, int fd );
				idTCP( const idTCP &tcp );
	virtual		~idTCP();

	idTCP &		operator = (const idTCP &tcp);

	// if host is host:port, the value of port is ignored
	bool		Init( const char *host, short port );
	void		Close();

	// returns -1 on failure (and closes socket)
	// those are non blocking, can be used for polling
	// there is no buffering, you are not guaranteed to Read or Write everything in a single call
	// (specially on win32, see recv and send documentation)
	int			Read( void *data, int size );
	int			Write( const void *data, int size );

	const netadr_t &GetAddress( void ) const { return address; }

private:
	netadr_t	address;		// remote address
	int			fd;				// OS specific socket

	friend void idPoller::Add( const idTCP &tcp, int which );
	friend void idPoller::Remove( const idTCP &tcp );
	friend int idPoller::Check( const idTCP &tcp );
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

// read proxy information from environment
#define			MAX_PROXY_LENGTH 128
bool			Sys_GetHTTPProxyAddress( char proxy[ MAX_PROXY_LENGTH ] );

// RAVEN BEGIN
// ddynerman: utility functions
// TTimo: FIXME if exposed, call them Sys_
int Net_GetNumInterfaces( void );
netadr_t Net_GetInterface( int index );

// asalmon: Xbox live related functions
#ifdef _XBOX
#define NONCE_SIZE  8
bool			Sys_CreateLiveMatch( void );
bool			Sys_CreateSystemLinkMatch( void );
bool			Sys_VerifyString(const char *string);
#endif
// RAVEN END


/*
==============================================================

	Multi-threading

==============================================================
*/

typedef unsigned int (*xthread_t)( void * );

typedef enum {
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
} xthreadPriority;

typedef struct {
	const char *	name;
	int				threadHandle;
	unsigned long	threadId;
// RAVEN BEGIN
// ksergent: included to track multiprocessor system
#ifdef _XBOX
	unsigned char cpuID;
#endif
// RAVEN END
} xthreadInfo;

const int MAX_THREADS				= 10;
extern xthreadInfo *g_threads[MAX_THREADS];
extern int			g_thread_count;

void				Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, xthreadInfo &info, const char *name, xthreadInfo *threads[MAX_THREADS], int *thread_count );
void				Sys_DestroyThread( xthreadInfo& info ); // sets threadHandle back to 0

// find the name of the calling thread
// if index != NULL, set the index in g_threads array (use -1 for "main" thread)
const char *		Sys_GetThreadName( int *index = 0 );
 
const int MAX_CRITICAL_SECTIONS		= 4;

enum {
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE
};

void				Sys_EnterCriticalSection( int index = CRITICAL_SECTION_ZERO );
void				Sys_LeaveCriticalSection( int index = CRITICAL_SECTION_ZERO );

const int MAX_TRIGGER_EVENTS		= 4;

enum {
	TRIGGER_EVENT_ZERO = 0,
	TRIGGER_EVENT_ONE,
	TRIGGER_EVENT_TWO,
	TRIGGER_EVENT_THREE
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
	virtual ~idSys() { }
	virtual void			DebugPrintf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) = 0;

	virtual double			GetClockTicks( void ) = 0;
	virtual double			ClockTicksPerSecond( void ) = 0;
	virtual cpuid_t			GetProcessorId( void ) = 0;
	virtual const char *	GetProcessorString( void ) = 0;
	virtual const char *	FPU_GetState( void ) = 0;
	virtual bool			FPU_StackIsEmpty( void ) = 0;
	virtual void			FPU_SetFTZ( bool enable ) = 0;
	virtual void			FPU_SetDAZ( bool enable ) = 0;
// RAVEN BEGIN
	virtual void			FPU_SetPrecision( int flags ) = 0;
// RAVEN END

	virtual bool			LockMemory( void *ptr, int bytes ) = 0;
	virtual bool			UnlockMemory( void *ptr, int bytes ) = 0;

	virtual void			GetCallStack( address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackCurStr( int depth ) = 0;
	virtual void			ShutdownSymbols( void ) = 0;

	virtual int				DLL_Load( const char *dllName ) = 0;
	virtual void *			DLL_GetProcAddress( int dllHandle, const char *procName ) = 0;
	virtual void			DLL_Unload( int dllHandle ) = 0;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) = 0;

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) = 0;
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) = 0;

// RAVEN BEGIN
	virtual int				MapKey( unsigned long lParam, unsigned short wParam ) = 0;
	virtual void			AddKeyPress( int key, bool state ) = 0;
	virtual int				GetNumKeyPresses( void ) = 0;
	virtual	bool			GetKeyPress( const int n, int &key, bool &state ) = 0;

	virtual void *			CreateWindowEx( const char *className, const char *windowName, int style, int x, int y, int w, int h, void *parent, void *menu, void *instance, void *param, int extStyle = 0 ) = 0;
	virtual void *			GetDC( void *hWnd ) = 0;
	virtual	void			ReleaseDC( void *hWnd, void *hDC ) = 0;
	virtual	void			ShowWindow( void *hWnd, int show ) = 0;
	virtual	void			UpdateWindow( void *hWnd ) = 0;
	virtual bool			IsWindowVisible( void *hWnd ) = 0;
	virtual void			SetForegroundWindow( void *hWnd ) = 0;
	virtual void			SetFocus( void *hWnd ) = 0;
	virtual	void			DestroyWindow( void *hWnd ) = 0;

	virtual	void			ShowConsole( int visLevel, bool quitOnClose ) = 0;
	virtual	void			UpdateConsole( void ) = 0;
	virtual void			SetConsoleName( const char* consoleName ) = 0;
	virtual bool			IsAppActive( void ) const = 0;
	virtual	int				Milliseconds( void ) = 0;
	virtual void			InitInput( void ) = 0;
	virtual void			ShutdownInput( void ) = 0;
	virtual void			GenerateEvents( void ) = 0;
	virtual void			GrabMouseCursor( bool grabIt ) = 0;

	virtual FILE			*FOpen( const char *name, const char *mode ) = 0;
	virtual void			FPrintf( FILE *file, const char *fmt ) = 0;
	virtual int				FTell( FILE *file ) = 0;
	virtual int				FSeek( FILE *file, long offset, int mode ) = 0;
	virtual void			FClose( FILE *file ) = 0;
	virtual int				FRead( void *buffer, int size, int count, FILE *file ) = 0;
	virtual int				FWrite( void *buffer, int size, int count, FILE *file ) = 0;
	virtual	long			FileTimeStamp( FILE *file ) = 0;
	virtual int				FEof( FILE *stream  ) = 0;
	virtual char			*FGets( char *string, int n, FILE *stream ) = 0;
	virtual void			FFlush( FILE *f ) = 0;
	virtual int				SetVBuf( FILE *stream, char *buffer, int mode, size_t size  ) = 0;
// RAVEN END

	virtual void			OpenURL( const char *url, bool quit ) = 0;
	virtual void			StartProcess( const char *exePath, bool quit ) = 0;

	virtual int				GetGUID( char *buf, int buflen ) = 0;
};

extern idSys *				sys;

// RAVEN BEGIN
// jnewquist: Scope timing tools
#if defined(_XENON)
class ScopeAutoMeasure {
public:
	ID_INLINE ScopeAutoMeasure(const char *name) {
		mName = name;
		QueryPerformanceCounter( &mStartTime ); 
	}
	ID_INLINE ~ScopeAutoMeasure() {
		LARGE_INTEGER endTime;
		QueryPerformanceCounter( &endTime );
		double time = (double)(endTime.QuadPart - mStartTime.QuadPart) / (Sys_ClockTicksPerSecond() * 0.000001);
		printf("Time %s: %f us\n", mName, time);
	}
protected:
	LARGE_INTEGER	mStartTime;
	const char *	mName;
};

#if defined(TIME_CAPTURE) //&& defined(_PROFILE)

class TimedScope {
public:
	ID_INLINE TimedScope(const char *name) {
		mName = name;
		mNext = mFirst;
		mFirst = this;
		mTime.QuadPart = 0;
	}
	static void ComputeCost() {
		LARGE_INTEGER TicksPerSecond;
		QueryPerformanceFrequency( &TicksPerSecond );
		sTicksPerMicrosecond = (double)TicksPerSecond.QuadPart * 0.000001;

		// get a rough time estimate for the cost of a call to Sys_Milliseconds so that we can remove it from the function costs
		LARGE_INTEGER before;
		QueryPerformanceCounter( &before );

		LARGE_INTEGER test;
		for(int i=0; i<1000000; i++)
		{
			QueryPerformanceCounter( &test );
		}
		LARGE_INTEGER after;
		QueryPerformanceCounter( &after );

		__int64 Ticks = after.QuadPart - before.QuadPart;
		sQueryPerformanceCounterCost.QuadPart = (double)Ticks/1000000.0f;
	}

	// automatically deducts the cost of the Sys_Milliseconds() call used to gather the timing
	ID_INLINE void AddTime(unsigned long long ticks) {
		// add number of microseconds
		mTime.QuadPart += (ticks - sQueryPerformanceCounterCost.QuadPart)/sTicksPerMicrosecond;
	}
	ID_INLINE void AddCall() {
		mCalls++;
	}
	ID_INLINE static void PrintTimes(void) {
		if (!mFirst) {
			return;
		}
		printf("Start Frame\n");
		for (TimedScope* p=mFirst; p; p = p->mNext) {
			printf("\t%20s: %d us\t%d calls\t %f us/call\n", p->mName, p->mTime, p->mCalls, (p->mCalls)?(double)p->mTime.QuadPart/(double)p->mCalls:0.0f);
		}
		printf("End Frame\n\n");
	}
	ID_INLINE static void ClearTimes(void) {
		if (!mFirst) {
			return;
		}
		for (TimedScope* p=mFirst; p; p = p->mNext) {
			p->mTime.QuadPart = 0;
		}
	}
	ID_INLINE static void ClearCalls(void) {
		if (!mFirst) {
			return;
		}
		for (TimedScope* p=mFirst; p; p = p->mNext) {
			p->mCalls = 0;
		}
	}
protected:
	static TimedScope *		mFirst;
	TimedScope *			mNext;
	const char *			mName;
	LARGE_INTEGER			mTime;
	unsigned int			mCalls;
	static LARGE_INTEGER	sQueryPerformanceCounterCost;
	static double			sTicksPerMicrosecond;
};

class ScopeAutoTimer {
public:
	ID_INLINE ScopeAutoTimer(TimedScope *scope) {
		QueryPerformanceCounter( &mStartTime );
		//mStartTime = Sys_Milliseconds();
		mScope = scope;
	}
	ID_INLINE ~ScopeAutoTimer() {
		LARGE_INTEGER endTime;
		QueryPerformanceCounter( &endTime );

		//unsigned int time = (unsigned int)Sys_Milliseconds() - mStartTime;
		// pass in number of ticks used and TimedScope will adjust it to a number of microseconds
		mScope->AddTime(endTime.QuadPart - mStartTime.QuadPart);
		mScope->AddCall();
	}
protected:
	LARGE_INTEGER	mStartTime;
	TimedScope *	mScope;
};

#define TIME_THIS_SCOPE(name) \
	static TimedScope scopeTime(name); \
	ScopeAutoTimer autoTimer(&scopeTime)
#else
#define TIME_THIS_SCOPE(name)
#endif
#endif

#define STRINGIZE_INDIRECT(F, X) F(X)
#define STRINGIZE(X) #X
#define __LINESTR__ STRINGIZE_INDIRECT(STRINGIZE, __LINE__)
#define __FILELINEFUNC__ (__FILE__ " " __LINESTR__ " " __FUNCTION__)
#define __FUNCLINE__ ( __FUNCTION__ " " __LINESTR__ )

// RAVEN END

#endif /* !__SYS_PUBLIC__ */
