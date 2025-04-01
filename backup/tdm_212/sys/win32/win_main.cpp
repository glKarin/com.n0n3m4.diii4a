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

#include "precompiled.h"
#pragma hdrstop

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <ShellAPI.h>

#ifndef __MRC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#pragma warning(push)
#pragma warning(disable: 4091)
#include "dbghelp.h"
#pragma warning(pop)

#include "../sys_local.h"
#include "win_local.h"
#include "rc/CreateResourceIDs.h"
#include "../../renderer/tr_local.h"

#include <string>
#include <vector>
#include "StdString.h"
#include <iostream>

idCVar Win32Vars_t::in_mouse( "in_mouse", "1", CVAR_SYSTEM | CVAR_BOOL, "enable mouse input" );
idCVar Win32Vars_t::win_xpos( "win_xpos", "3", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "horizontal position of window" );
idCVar Win32Vars_t::win_ypos( "win_ypos", "22", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "vertical position of window" );
idCVarBool Win32Vars_t::win_maximized( "win_maximized", "0", CVAR_SYSTEM | CVAR_ARCHIVE, "maximized state of window" );
idCVar Win32Vars_t::win_outputDebugString( "win_outputDebugString", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar Win32Vars_t::win_outputEditString( "win_outputEditString", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar Win32Vars_t::win_viewlog( "win_viewlog", "0", CVAR_SYSTEM | CVAR_INTEGER, "" );
idCVar Win32Vars_t::win_timerUpdate( "win_timerUpdate", "0", CVAR_SYSTEM | CVAR_BOOL, "allows the game to be updated while dragging the window" );
idCVarBool Win32Vars_t::win_topmost("win_topmost", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "add topmost flag to Doom 3 window during creation: no other window can occlude it");

Win32Vars_t	win32;

static char		sys_cmdline[MAX_STRING_CHARS];

static	HANDLE		hTimer;

#if 0
/*
==================
Sys_Createthread
==================
*/
typedef std::pair<xthread_t, void *> CreateThreadStartParams;
DWORD WINAPI CreateThreadStartRoutine( LPVOID lpThreadParameter ) {
	auto arg = *( ( CreateThreadStartParams * )lpThreadParameter );
	delete ( ( CreateThreadStartParams * )lpThreadParameter );
	return arg.first( arg.second );
}

void Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, xthreadInfo &info, const char *name, xthreadInfo *threads[MAX_THREADS], int *thread_count ) {
	//stgatilov: it is illegal to reinterpret cdecl function as stdcall function (in 32-bit case)
	//so we have to pass a helper function here, which would in turn call original callback
	auto helperParam = new CreateThreadStartParams( function, parms );
	HANDLE temp = CreateThread(	NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	                            0,		// DWORD cbStack,
	                            CreateThreadStartRoutine,	// LPTHREAD_START_ROUTINE lpStartAddr,
	                            helperParam,	// LPVOID lpvThreadParm,
	                            0,		//   DWORD fdwCreate,
	                            &info.threadId );
	info.threadHandle = ( intptr_t ) temp;
	if ( priority == THREAD_HIGHEST ) {
		SetThreadPriority( ( HANDLE )info.threadHandle, THREAD_PRIORITY_HIGHEST );		//  we better sleep enough to do this
	} else if ( priority == THREAD_ABOVE_NORMAL ) {
		SetThreadPriority( ( HANDLE )info.threadHandle, THREAD_PRIORITY_ABOVE_NORMAL );
	}
	info.name = name;
	if ( *thread_count < MAX_THREADS ) {
		threads[( *thread_count )++] = &info;
	} else {
		common->DPrintf( "WARNING: MAX_THREADS reached\n" );
	}
}

/*
==================
Sys_DestroyThread
==================
*/
void Sys_DestroyThread( xthreadInfo &info ) {
	WaitForSingleObject( ( HANDLE )info.threadHandle, INFINITE );
	CloseHandle( ( HANDLE )info.threadHandle );
	info.threadHandle = 0;
}

#endif

/*
==================
Sys_Sentry
==================
*/
void Sys_Sentry() {
	int j = 0;
}

/*
==================
Sys_EnterCriticalSection
==================
*/
void Sys_EnterCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_CRITICAL_SECTIONS );
	if ( TryEnterCriticalSection( &win32.criticalSections[index] ) == 0 ) {
		EnterCriticalSection( &win32.criticalSections[index] );
		//		Sys_DebugPrintf( "busy lock '%s' in thread '%s'\n", lock->name, Sys_GetThreadName() );
	}
}

/*
==================
Sys_LeaveCriticalSection
==================
*/
void Sys_LeaveCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_CRITICAL_SECTIONS );
	LeaveCriticalSection( &win32.criticalSections[index] );
}

/*
==================
Sys_WaitForEvent
==================
*/
void Sys_WaitForEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	if ( !win32.events[index] ) {
		win32.events[index] = CreateEvent( NULL, TRUE, FALSE, NULL );
	}
	WaitForSingleObject( win32.events[index], INFINITE );
	ResetEvent( win32.events[index] );
}

/*
==================
Sys_TriggerEvent
==================
*/
void Sys_TriggerEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	SetEvent( win32.events[index] );
}


#ifdef DEBUG


static size_t debug_total_alloc = 0;
static size_t debug_total_alloc_count = 0;
static size_t debug_current_alloc = 0;
static size_t debug_current_alloc_count = 0;
static size_t debug_frame_alloc = 0;
static size_t debug_frame_alloc_count = 0;

idCVar sys_showMallocs( "sys_showMallocs", "0", CVAR_SYSTEM, "" );

// _HOOK_ALLOC, _HOOK_REALLOC, _HOOK_FREE

typedef struct CrtMemBlockHeader {
	struct _CrtMemBlockHeader *pBlockHeaderNext;	// Pointer to the block allocated just before this one:
	struct _CrtMemBlockHeader *pBlockHeaderPrev;	// Pointer to the block allocated just after this one
	char *szFileName;    // File name
	int nLine;           // Line number
	size_t nDataSize;    // Size of user block
	int nBlockUse;       // Type of block
	long lRequest;       // Allocation number
	byte		gap[4];								// Buffer just before (lower than) the user's memory:
} CrtMemBlockHeader;

#include <crtdbg.h>

/*
==================
Sys_AllocHook

	called for every malloc/new/free/delete
==================
*/
int Sys_AllocHook( int nAllocType, void *pvData, size_t nSize, int nBlockUse, long lRequest, const unsigned char *szFileName, int nLine ) {
	CrtMemBlockHeader	*pHead;
	byte				*temp;

	if ( nBlockUse == _CRT_BLOCK ) {
		return ( TRUE );
	}

	// get a pointer to memory block header
	temp = ( byte * )pvData;
	temp -= 32;
	pHead = ( CrtMemBlockHeader * )temp;

	switch ( nAllocType ) {
	case	_HOOK_ALLOC:
		debug_total_alloc += nSize;
		debug_current_alloc += nSize;
		debug_frame_alloc += nSize;
		debug_total_alloc_count++;
		debug_current_alloc_count++;
		debug_frame_alloc_count++;
		break;

	case	_HOOK_FREE:
		assert( pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd );

		debug_current_alloc -= pHead->nDataSize;
		debug_current_alloc_count--;
		debug_total_alloc_count++;
		debug_frame_alloc_count++;
		break;

	case	_HOOK_REALLOC:
		assert( pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd );

		debug_current_alloc -= pHead->nDataSize;
		debug_total_alloc += nSize;
		debug_current_alloc += nSize;
		debug_frame_alloc += nSize;
		debug_total_alloc_count++;
		debug_current_alloc_count--;
		debug_frame_alloc_count++;
		break;
	}
	return ( TRUE );
}

/*
==================
Sys_DebugMemory_f
==================
*/
void Sys_DebugMemory_f( void ) {
	common->Printf( "Total allocation %8dk in %d blocks\n", debug_total_alloc / 1024, debug_total_alloc_count );
	common->Printf( "Current allocation %8dk in %d blocks\n", debug_current_alloc / 1024, debug_current_alloc_count );
}

/*
==================
Sys_MemFrame
==================
*/
void Sys_MemFrame( void ) {
	if ( sys_showMallocs.GetInteger() ) {
		common->Printf( "Frame: %8dk in %5d blocks\n", debug_frame_alloc / 1024, debug_frame_alloc_count );
	}

	debug_frame_alloc = 0;
	debug_frame_alloc_count = 0;
}

#endif

/*
==================
Sys_FlushCacheMemory

On windows, the vertex buffers are write combined, so they
don't need to be flushed from the cache
==================
*/
void Sys_FlushCacheMemory( void *base, int bytes ) {
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
	MSG        msg;

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Win_SetErrorText( text );
	Sys_ShowConsole( 1, true );

	timeEndPeriod( 1 );

	Sys_ShutdownInput();

	GLimp_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			common->Quit();
		}
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	Sys_DestroyConsole();

	exit( 1 );
}

/*
==============
Sys_Quit
==============
*/
void Sys_MfcHack();
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	Sys_ShutdownInput();
	Sys_DestroyConsole();
#ifndef NO_MFC
	//see comment near definition for explanation
	Sys_MfcHack();
#endif
	ExitProcess( 0 );
}


/*
==============
Sys_Printf
==============
*/
#define MAXPRINTMSG 4096
void Sys_Printf( const char *fmt, ... ) {
	char		msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg ) - 1] = '\0';

	if ( win32.win_outputDebugString.GetBool() ) {
		OutputDebugString( msg );
	}

	if ( win32.win_outputEditString.GetBool() ) {
		Conbuf_AppendText( msg );
	}
}

/*
==============
Sys_DebugPrintf
==============
*/
#define MAXPRINTMSG 4096
void Sys_DebugPrintf( const char *fmt, ... ) {
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr );
	msg[ sizeof( msg ) - 1 ] = '\0';
	va_end( argptr );

	OutputDebugString( msg );
}

/*
==============
Sys_DebugVPrintf
==============
*/
void Sys_DebugVPrintf( const char *fmt, va_list arg ) {
	char msg[MAXPRINTMSG];

	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, arg );
	msg[ sizeof( msg ) - 1 ] = '\0';

	OutputDebugString( msg );
}

/*
==============
Sys_Sleep
==============
*/
void Sys_Sleep( int msec ) {
	Sleep( msec );
}

/*
==============
Sys_ShowWindow
==============
*/
void Sys_ShowWindow( bool show ) {
	::ShowWindow( win32.hWnd, show ? SW_SHOW : SW_HIDE );
}

/*
==============
Sys_IsWindowVisible
==============
*/
bool Sys_IsWindowVisible( void ) {
	return ( ::IsWindowVisible( win32.hWnd ) != 0 );
}

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir( path );
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( FILE *fp ) {
	struct _stat st;
	_fstat( _fileno( fp ), &st );
	return ( long ) st.st_mtime;
}

/*
==============
Sys_Cwd
==============
*/
const char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultBasePath
==============
*/
const char *Sys_DefaultBasePath( void ) {
	static const char *basePath = NULL;
	if ( !basePath ) {
		// TheDarkMod.exe is now located under darkmod/ so we need to set basepath to the EXE path
		idStr buff = Sys_EXEPath();
		buff.StripFilename();
		basePath = Mem_CopyString( buff.c_str() );
	}
	return basePath;
}

/*
==============
Sys_DefaultSavePath
==============
*/
const char *Sys_DefaultSavePath( void ) {
	static const char *savePath = NULL;
	// default savepath changed to the mod dir.
	if ( !savePath ) {
		idStr buff = cvarSystem->GetCVarString( "fs_basepath" );
		// only append the mod if it isn't "darkmod"
		if ( idStr::Icmp( cvarSystem->GetCVarString( "fs_mod" ), BASE_TDM ) ) {
			buff.AppendPath( cvarSystem->GetCVarString( "fs_mod" ) );
		}
		savePath = Mem_CopyString( buff.c_str() );
	}
	return savePath;
}

/*
==============
Sys_ModSavePath
==============
*/
const char *Sys_ModSavePath() {
	// greebo: In Windows, we use the basepath + "darkmod/fms/" as savepath
	// taaaki: changed this to savepath + "fms/"
	static const char *modSavePath = NULL;

	if ( !modSavePath ) {
		idStr buff = cvarSystem->GetCVarString( "fs_savepath" );
		buff.AppendPath( "fms" );
		modSavePath = Mem_CopyString( buff.c_str() );
	}
	return modSavePath;
}

/*
==============
Sys_EXEPath
==============
*/
const char *Sys_EXEPath( void ) {
	static char exe[ MAX_OSPATH ];
	GetModuleFileName( NULL, exe, sizeof( exe ) - 1 );
	return exe;
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char *directory, const char *extension, idStrList &list ) {
	idStr		search;
	struct _finddata_t findinfo;
	intptr_t	findhandle;
	int			flag;

	if ( !extension ) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}
	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );

	if ( findhandle == -1 ) {
		return -1;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			list.Append( findinfo.name );
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return list.Num();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = ( char * )GlobalLock( hClipboardData ) ) != 0 ) {
				data = ( char * )Mem_Alloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
================
Sys_SetClipboardData
================
*/
void Sys_SetClipboardData( const char *string ) {
	HGLOBAL HMem;
	char *PMem;

	// allocate memory block
	HMem = ( char * )::GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, strlen( string ) + 1 );
	if ( HMem == NULL ) {
		return;
	}
	// lock allocated memory and obtain a pointer
	PMem = ( char * )::GlobalLock( HMem );
	if ( PMem == NULL ) {
		return;
	}
	// copy text into allocated memory block
	lstrcpy( PMem, string );
	// unlock allocated memory
	::GlobalUnlock( HMem );
	// open Clipboard
	if ( !OpenClipboard( 0 ) ) {
		::GlobalFree( HMem );
		return;
	}
	// remove current Clipboard contents
	EmptyClipboard();
	// supply the memory handle to the Clipboard
	SetClipboardData( CF_TEXT, HMem );
	HMem = 0;
	// close Clipboard
	CloseClipboard();
}

/*
========================================================================

DLL Loading

========================================================================
*/

namespace {
// Removes .\ and ..\ from the given path, converts forward slashes to backward slashes
idStr NormalisePath( const idStr &input ) {
	std::vector<std::string> parts;
	std::vector<std::string> resultParts;

	stdext::split( parts, std::string( input.c_str() ), "\\/" );

	for ( std::size_t i = 0; i < parts.size(); ++i ) {
		if ( parts[i] == ".." ) {
			if ( !resultParts.empty() ) {
				resultParts.pop_back();
			}
		} else if ( parts[i] == "." ) {
			// Ignore
		} else {
			// Just cat other path entries
			resultParts.push_back( parts[i] );
		}
	}
	return idStr( stdext::join( resultParts, "\\" ).c_str() );
}
}

/*
=====================
Sys_DLL_Load
=====================
*/
uintptr_t Sys_DLL_Load( const char *dllName ) {
	HINSTANCE	libHandle;
	libHandle = LoadLibrary( dllName );
	if ( libHandle ) {
		// since we can't have LoadLibrary load only from the specified path, check it did the right thing
		char loadedPath[ MAX_OSPATH ];
		GetModuleFileName( libHandle, loadedPath, sizeof( loadedPath ) - 1 );

		// greebo: Make sure to normalise the path before checking, there might be ".." in them
		idStr dllPath = NormalisePath( dllName );

		if ( idStr::IcmpPath( dllPath, loadedPath ) ) {
			Sys_Printf( "ERROR: LoadLibrary '%s' wants to load '%s'\n", dllPath.c_str(), loadedPath );
			Sys_DLL_Unload( ( uintptr_t )libHandle );
			return 0;
		}
	}
	return ( uintptr_t )libHandle;
}

/*
=====================
Sys_DLL_GetProcAddress
=====================
*/
void *Sys_DLL_GetProcAddress( uintptr_t dllHandle, const char *procName ) {
	return GetProcAddress( ( HINSTANCE )dllHandle, procName );
}

/*
=====================
Sys_DLL_Unload
=====================
*/
void Sys_DLL_Unload( uintptr_t dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( FreeLibrary( ( HINSTANCE )dllHandle ) == 0 ) {
		int lastError = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER,
		    NULL,
		    lastError,
		    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
		    ( LPTSTR ) &lpMsgBuf,
		    0,
		    NULL
		);
		Sys_Error( "Sys_DLL_Unload: FreeLibrary failed - %s (%d)", lpMsgBuf, lastError );
	}
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead = 0;
int			eventTail = 0;

/*
================
Sys_QueEvent

Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		common->Printf( "Sys_QueEvent: overflow\n" );
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Mem_Free( ev->evPtr );
		}
		eventTail++;
	}
	eventHead++;

	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
=============
Sys_PumpEvents

This allows windows to be moved during renderbump
=============
*/
void Sys_PumpEvents( void ) {
	MSG msg;

	// pump the message loop
	while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			common->Quit();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		if ( win32.sysMsgTime && win32.sysMsgTime > ( int )msg.time ) {
			// don't ever let the event times run backwards
			//			common->Printf( "Sys_PumpEvents: win32.sysMsgTime (%i) > msg.time (%i)\n", win32.sysMsgTime, msg.time );
		} else {
			win32.sysMsgTime = msg.time;
		}

#ifdef ID_ALLOW_TOOLS
		if ( GUIEditorHandleMessage( &msg ) ) {
			continue;
		}
#endif

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents( void ) {
	static int entered = false;
	char *s;

	if ( entered ) {
		return;
	}
	entered = true;

	// pump the message loop
	Sys_PumpEvents();

	// make sure mouse and joystick are only called once a frame
	IN_Frame();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = static_cast<int>( strlen( s ) + 1 );
		b = ( char * )Mem_Alloc( len );
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}
	entered = false;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents( void ) {
	eventHead = eventTail = 0;
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// return the empty event
	memset( &ev, 0, sizeof( ev ) );

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( const idCmdArgs &args ) {
	Sys_ShutdownInput();
	Sys_InitInput();
}


/*
==================
Sys_AsyncThread
==================
*/
static void Sys_AsyncThread( void *parm ) {
	int		wakeNumber;
	int		startTime;

	startTime = Sys_Milliseconds();
	wakeNumber = 0;

	// stgatilov #4550: set FPU props (FTZ + DAZ, etc.)
	sys->ThreadStartup();

	while ( 1 ) {
#ifdef WIN32
		// this will trigger 60 times a second
		int r = WaitForSingleObject( hTimer, 100 );
		if ( r != WAIT_OBJECT_0 ) {
			OutputDebugString( "idPacketServer::PacketServerInterrupt: bad wait return" );
		}
#endif

#if 0
		wakeNumber++;
		int		msec = Sys_Milliseconds();
		int		deltaTime = msec - startTime;
		startTime = msec;

		char	str[1024];
		sprintf( str, "%i ", deltaTime );
		OutputDebugString( str );
#endif


		common->Async();
	}
}

/*
==============
Sys_StartAsyncThread

Start the thread that will call idCommon::Async()
==============
*/
void Sys_StartAsyncThread( void ) {
	// create an auto-reset event that happens 60 times a second
	hTimer = CreateWaitableTimer( NULL, false, NULL );
	if ( !hTimer ) {
		common->Error( "idPacketServer::Spawn: CreateWaitableTimer failed" );
	}

	//stgatilov #4514: run idCommonLocal::Async every 3 ms
	//ideally, game tic should be incremented every 16.66 ms, but we cannot specify interval up to microseconds
	//incrementing it every 16 ms causes double frames =( so we do it simply more often
	//Note: actual tics happen only every 16.66 ms on average (see idCommonLocal::Async)
	const int intervalMS = 3;		//USERCMD_MSEC;

	LARGE_INTEGER	t;
	t.HighPart = t.LowPart = 0;
	SetWaitableTimer( hTimer, &t, intervalMS, NULL, NULL, TRUE );

	auto threadInfo = Sys_CreateThread( (xthread_t)Sys_AsyncThread, NULL, THREAD_ABOVE_NORMAL, "Async" );

#ifdef SET_THREAD_AFFINITY
	// give the async thread an affinity for the second cpu
	SetThreadAffinityMask( ( HANDLE )threadInfo.threadHandle, 2 );
#endif

	if ( !threadInfo ) {
		common->Error( "Sys_StartAsyncThread: failed" );
	}
}

/*
================
Sys_Init

The cvar system must already be setup
Lots of this code is redundant, clean up sometime: Revelator
Added win 7 to code: Revelator
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {

	CoInitialize( NULL );

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	// get WM_TIMER messages pumped every millisecond
	//	SetTimer( NULL, 0, 100, NULL );

	cmdSystem->AddCommand( "in_restart", Sys_In_Restart_f, CMD_FL_SYSTEM, "restarts the input system" );
#ifdef DEBUG
	cmdSystem->AddCommand( "createResourceIDs", CreateResourceIDs_f, CMD_FL_TOOL, "assigns resource IDs in _resouce.h files" );
#endif
#if 0
	cmdSystem->AddCommand( "setAsyncSound", Sys_SetAsyncSound_f, CMD_FL_SYSTEM, "set the async sound option" );
#endif

	//
	// Windows version
	//
	win32.osversion.dwOSVersionInfoSize = sizeof( win32.osversion );

	if ( !GetVersionEx( ( LPOSVERSIONINFO )&win32.osversion ) ) {
		Sys_Error( "Couldn't get OS info" );
	}

	if ( win32.osversion.dwMajorVersion < 4 ) {
		Sys_Error( GAME_NAME " requires Windows version 4 (NT) or greater" );
	}

	if ( win32.osversion.dwPlatformId == VER_PLATFORM_WIN32s ) {
		Sys_Error( GAME_NAME " doesn't run on Win32s" );
	}
	//
	// CPU type
	//
	Sys_InitCPUID();
}

/*
================
Sys_Shutdown
================
*/
void Sys_Shutdown( void ) {
	CoUninitialize();
}

//=======================================================================

//#define SET_THREAD_AFFINITY


/*
====================
Win_Frame
====================
*/
void Win_Frame( void ) {
	// if "viewlog" has been modified, show or hide the log console
	if ( win32.win_viewlog.IsModified() ) {
		if ( !com_skipRenderer.GetBool() ) {
			Sys_ShowConsole( win32.win_viewlog.GetInteger(), false );
		}
		win32.win_viewlog.ClearModified();
	}
}

/*
==================
WinMain
==================
*/
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {

	const HCURSOR hcurSave = ::SetCursor( LoadCursor( 0, IDC_WAIT ) );

	Sys_SetPhysicalWorkMemory( 192 << 20, 1024 << 20 );

	win32.hInstance = hInstance;
	idStr::Copynz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	for ( int i = 0; i < MAX_CRITICAL_SECTIONS; i++ ) {
		InitializeCriticalSection( &win32.criticalSections[i] );
	}

	// get the initial time base
	Sys_Milliseconds();

#ifdef DEBUG
	// disable the painfully slow MS heap check every 1024 allocs
	_CrtSetDbgFlag( 0 );
#endif

	common->Init( 0, NULL, lpCmdLine );

	Sys_StartAsyncThread();

	// hide or show the early console as necessary
	if ( win32.win_viewlog.GetInteger() || com_skipRenderer.GetBool() ) {
		Sys_ShowConsole( 1, true );
	} else {
		Sys_ShowConsole( 0, false );
	}

#ifdef SET_THREAD_AFFINITY
	// give the main thread an affinity for the first cpu
	SetThreadAffinityMask( GetCurrentThread(), 1 );
#endif

	::SetCursor( hcurSave );

	// Launch the script debugger
	if ( strstr( lpCmdLine, "+debugger" ) ) {
		// DebuggerClientInit( lpCmdLine );
		return 0;
	}

	::SetFocus( win32.hWnd );

	// main game loop
	while ( 1 ) {

		Win_Frame();

#ifdef DEBUG
		Sys_MemFrame();
#endif

#ifdef ID_ALLOW_TOOLS
		if ( com_editors ) {
			if ( com_editors & EDITOR_GUI ) {
				// GUI editor
				GUIEditorRun();
			} else if ( com_editors & EDITOR_RADIANT ) {
				// Level Editor
				RadiantRun();
			} else if ( com_editors & EDITOR_MATERIAL ) {
				//BSM Nerve: Add support for the material editor
				MaterialEditorRun();
			} else {
				if ( com_editors & EDITOR_LIGHT ) {
					// in-game Light Editor
					LightEditorRun();
				}
				if ( com_editors & EDITOR_SOUND ) {
					// in-game Sound Editor
					SoundEditorRun();
				}
				if ( com_editors & EDITOR_DECL ) {
					// in-game Declaration Browser
					DeclBrowserRun();
				}
				if ( com_editors & EDITOR_AF ) {
					// in-game Articulated Figure Editor
					AFEditorRun();
				}
				if ( com_editors & EDITOR_PARTICLE ) {
					// in-game Particle Editor
					ParticleEditorRun();
				}
				if ( com_editors & EDITOR_SCRIPT ) {
					// in-game Script Editor
					ScriptEditorRun();
				}
			}
		}
#endif
		// run the game
		common->Frame();
	}

	// never gets here
	return 0;
}

/*
==================
idSysLocal::OpenURL
==================
*/
void idSysLocal::OpenURL( const char *url, bool doexit ) {
	static bool doexit_spamguard = false;
	HWND wnd;

	if ( doexit_spamguard ) {
		common->DPrintf( "OpenURL: already in an exit sequence, ignoring %s\n", url );
		return;
	}
	common->Printf( "Open URL: %s\n", url );

	if ( !ShellExecute( NULL, "open", url, NULL, NULL, SW_RESTORE ) ) {
		common->Error( "Could not open url: '%s' ", url );
		return;
	}
	wnd = GetForegroundWindow();

	if ( wnd ) {
		ShowWindow( wnd, SW_MAXIMIZE );
	}

	if ( doexit ) {
		doexit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
idSysLocal::StartProcess
==================
*/
void idSysLocal::StartProcess( const char *exePath, bool doexit ) {
	TCHAR				szPathOrig[_MAX_PATH];
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;

	ZeroMemory( &si, sizeof( si ) );
	si.cb = sizeof( si );

	strncpy( szPathOrig, exePath, _MAX_PATH );

	if ( !CreateProcess( NULL, szPathOrig, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ) {
		common->Error( "Could not start process: '%s' ", szPathOrig );
		return;
	}

	if ( doexit ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
Sys_SetFatalError
==================
*/
void Sys_SetFatalError( const char *error ) {
}

/*
==================
Sys_DoPreferences
==================
*/
void Sys_DoPreferences( void ) {
}


void Sys_CaptureStackTrace(int ignoreFrames, uint8_t *data, int &len) {
	int cnt = CaptureStackBackTrace(ignoreFrames, len / sizeof(PVOID), (PVOID*)data, NULL);
	len = cnt * sizeof(PVOID);
}

int Sys_GetStackTraceFramesCount(uint8_t *data, int len) {
	return len / sizeof(PVOID);
}

static bool AreSymbolsInitialized = false;

void Sys_DecodeStackTrace(uint8_t *data, int len, debugStackFrame_t *frames) {
	//interpret input blob as array of addresses
	PVOID *addresses = (PVOID*)data;
	int framesCount = Sys_GetStackTraceFramesCount(data, len);
	//fill output with zeros
	memset(frames, 0, framesCount * sizeof(frames[0]));

	HANDLE hProcess = GetCurrentProcess();

	if (!AreSymbolsInitialized) {
		AreSymbolsInitialized = true;
		BOOL ok = SymInitialize(hProcess, NULL, TRUE);
	}

	//allocate symbol structures
	int buff[(sizeof(IMAGEHLP_SYMBOL64) + sizeof(frames[0].functionName)) / 4 + 1] = {0};
	IMAGEHLP_SYMBOL64 *symbol = (IMAGEHLP_SYMBOL64*)buff;
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	symbol->MaxNameLength = sizeof(frames[0].functionName) - 1;
	IMAGEHLP_LINE64 line = {0};
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	for (int i = 0; i < framesCount; i++) {
		frames[i].pointer = addresses[i];
		sprintf(frames[i].functionName, "[%p]", frames[i].pointer);	//in case PDB not found

		if (!addresses[i])
			continue;	//null function?
		if (!SymGetSymFromAddr64(hProcess, DWORD64(addresses[i]), NULL, symbol))
			continue;	//cannot get symbol
		idStr::Copynz(frames[i].functionName, symbol->Name, sizeof(frames[0].functionName));

		DWORD displacement = DWORD(-1);
		if (!SymGetLineFromAddr64(hProcess, DWORD64(addresses[i]), &displacement, &line))
			continue;	//no code line info
		idStr::Copynz(frames[i].fileName, line.FileName, sizeof(frames[0].fileName));
		frames[i].lineNumber = line.LineNumber;
	}
}
