/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };
static char libPath[ MAX_OSPATH ] = { 0 };

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

/*
=================
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath(void)
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
=================
Sys_DefaultInstallPath
=================
*/
char *Sys_DefaultInstallPath(void)
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

/*
=================
Sys_SetDefaultLibPath
=================
*/
void Sys_SetDefaultLibPath(const char *path)
{
	Q_strncpyz(libPath, path, sizeof(libPath));
}

/*
=================
Sys_DefaultLibPath
=================
*/
char *Sys_DefaultLibPath(void)
{
	if (*libPath)
		return libPath;
	else
		return Sys_Cwd();
}

/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath(void)
{
	return Sys_BinaryPath();
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void )
{
	IN_Shutdown();
	IN_Init();
}

/*
=================
Sys_ConsoleInput

Handle new console input
=================
*/
char *Sys_ConsoleInput(void)
{
	return CON_Input( );
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
void Sys_Exit( int ex )
{
	CON_Shutdown( );

#ifdef NDEBUG
	exit( ex );
#else
	// Cause a backtrace on error exits
	assert( ex == 0 );
	exit( ex );
#endif
}

/*
=================
Sys_Quit
=================
*/
void Sys_Quit( void )
{
	CL_Shutdown( );
	Sys_Exit( 0 );
}

/*
=================
Sys_Init
=================
*/
void Sys_Init(void)
{
	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser( ) );
}

/*
=================
Sys_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
void Sys_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAXPRINTMSG ];
	int         length = 0;
	static int  q3ToAnsi[ 8 ] =
			{
					30, // COLOR_BLACK
					31, // COLOR_RED
					32, // COLOR_GREEN
					33, // COLOR_YELLOW
					34, // COLOR_BLUE
					36, // COLOR_CYAN
					35, // COLOR_MAGENTA
					0   // COLOR_WHITE
			};

	while( *msg )
	{
		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if( *msg == '\n' )
			{
				// Issue a reset and then the newline
				fputs( "\033[0m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				Com_sprintf( buffer, sizeof( buffer ), "\033[%dm",
							 q3ToAnsi[ ColorIndex( *( msg + 1 ) ) ] );
				fputs( buffer, stderr );
				msg += 2;
			}
		}
		else
		{
			if( length >= MAXPRINTMSG - 1 )
				break;

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

/*
=================
Sys_Print
=================
*/
void Sys_Print( const char *msg )
{
	CON_LogWrite( msg );
	CON_Print( msg );
}

/*
=================
Sys_Error
=================
*/
void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char    string[1024];

	CL_Shutdown ();

	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	Sys_ErrorDialog( string );

	Sys_Exit( 1 );
}

/*
=================
Sys_Warn
=================
*/
void Sys_Warn( char *warning, ... )
{
	va_list argptr;
	char    string[1024];

	va_start (argptr,warning);
	Q_vsnprintf (string, sizeof(string), warning, argptr);
	va_end (argptr);

	CON_Print( va( "Warning: %s", string ) );
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime( char *path )
{
	struct stat buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}

/*
=================
Sys_UnloadDll
=================
*/
void Sys_UnloadDll( void *dllHandle )
{
	if( !dllHandle )
	{
		Com_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
}


/*
=================
Sys_TryLibraryLoad
=================
*/
static void* Sys_TryLibraryLoad(const char* base, const char* gamedir, const char* fname, char* fqpath )
{
	void* libHandle;
	char* fn;
	const char *name = fname;

	// Don't load any DLLs that end with the pk3 extension
	if (COM_CompareExtension(name, ".pk3"))
	{
		Com_Printf(S_COLOR_RED "Rejecting DLL named \"%s\"\n", name);
		return NULL;
	}

	*fqpath = 0;

	fn = FS_BuildOSPath( base, gamedir, fname );
	Com_Printf( "Sys_LoadDll(%s)... \n", fn );

	libHandle = Sys_LoadLibrary(fn);

	if(!libHandle) {
		Com_Printf( "Sys_LoadDll(%s) failed:\n\"%s\"\n", fn, Sys_LibraryError() );
		return NULL;
	}

	Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn );
	Q_strncpyz ( fqpath , fn , MAX_QPATH ) ;

	return libHandle;
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
#1 look down current path
#2 look in fs_homepath
#3 look in fs_basepath
#4 look in fs_libpath
=================
*/
void *Sys_LoadDll( const char *name, char *fqpath ,
				   intptr_t (**entryPoint)(int, ...),
				   intptr_t (*systemcalls)(intptr_t, ...) )
{
	void  *libHandle;
	void  (*dllEntry)( intptr_t (*syscallptr)(intptr_t, ...) );
	char  fname[MAX_OSPATH];
	char  *basepath;
	char  *libpath;
	char  *homepath;
	char  *pwdpath;
	char  *gamedir;

	assert( name );

	Q_snprintf (fname, sizeof(fname), "%s" ARCH_STRING DLL_EXT, name);

	// TODO: use fs_searchpaths from files.c
	pwdpath = Sys_Cwd();
	basepath = Cvar_VariableString( "fs_basepath" );
	libpath = Cvar_VariableString( "fs_libpath" );
	homepath = Cvar_VariableString( "fs_homepath" );
	gamedir = Cvar_VariableString( "fs_game" );

	libHandle = Sys_TryLibraryLoad(pwdpath, gamedir, fname, fqpath);

	if(!libHandle && homepath)
		libHandle = Sys_TryLibraryLoad(homepath, gamedir, fname, fqpath);

	if(!libHandle && libpath)
		libHandle = Sys_TryLibraryLoad(libpath, gamedir, fname, fqpath);

	if(!libHandle && basepath)
		libHandle = Sys_TryLibraryLoad(basepath, gamedir, fname, fqpath);

	if(!libHandle) {
		Com_Printf ( "Sys_LoadDll(%s) failed to load library\n", name );
		return NULL;
	}

	dllEntry = Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
		Com_Printf ( "Sys_LoadDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError( ) );
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	Com_Printf ( "Sys_LoadDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	return libHandle;
}

/*
=================
Sys_Idle
=================
*/
static void Sys_Idle( void )
{
}
#define Sys_Idle()

/*
=================
Sys_ParseArgs
=================
*/
void Sys_ParseArgs( int argc, char **argv )
{
	if( argc == 2 )
	{
		if( !strcmp( argv[1], "--version" ) ||
			!strcmp( argv[1], "-v" ) )
		{
			const char* date = __DATE__;
#ifdef DEDICATED
			fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", date );
#else
			fprintf( stdout, Q3_VERSION " client (%s)\n", date );
#endif
			Sys_Exit(0);
		}
	}
}

#ifndef DEFAULT_BASEDIR
#	ifdef MACOS_X
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
#	endif
#endif

#ifndef DEFAULT_LIBDIR
#	define DEFAULT_LIBDIR DEFAULT_BASEDIR
#endif

/*
=================
Sys_SigHandler
=================
*/
void Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;

	if( signalcaught )
	{
		fprintf( stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
				 signal );
	}
	else
	{
		signalcaught = qtrue;
		fprintf( stderr, "Received signal %d, exiting...\n", signal );
#ifndef DEDICATED
		CL_Shutdown();
#endif
		SV_Shutdown( "Signal caught" );
	}

	Sys_Exit( 0 ); // Exit with 0 to avoid recursive signals
}

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
	FS_Seek( f, offset, origin );
}



void Sys_BeginProfiling( void ) {
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
}

qboolean Sys_DetectAltivec( void )
{
	qboolean altivec = qfalse;

	return altivec;
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/

void Sys_SetEnv(const char *name, const char *value)
{
	if(value && *value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}



/*
========================================================================

EVENT LOOP

========================================================================
*/

// bk000306: upped this from 64
#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t  eventQue[MAX_QUED_EVENTS];
// bk000306: initialize
int   eventHead = 0;
int             eventTail = 0;
byte    sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t  *ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

	// bk000305 - was missing
	if ( eventHead - eventTail >= MAX_QUED_EVENTS )
	{
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr )
		{
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 )
	{
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t  ev;
	char    *s;
	msg_t   netmsg;
	netadr_t  adr;

	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;

        sysEvent_t *e = &eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	// in vga this calls KBD_Update, under X, it calls GetEvent
	Sys_SendKeyEvents ();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s )
	{
		char  *b;
		int   len;

		len = strlen( s ) + 1;
		b = Z_Malloc( len );
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for other input devices
	IN_Frame();

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) )
	{
		netadr_t    *buf;
		int       len;

		// copy out to a seperate buffer for qeueing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = Z_Malloc( len );
		*buf = adr;
		memcpy( buf+1, netmsg.data, netmsg.cursize );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

#include "../client/client.h"
extern clientStatic_t cls;

/* Android */

extern qboolean com_fullyInitialized;

#include "sys_android.c"

qboolean GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

extern const char *Sys_Dirname( char *path );
/*
=================
main
=================
*/
int main( int argc, char **argv )
{
	int   i;
	char  commandLine[ MAX_STRING_CHARS ] = { 0 };

	Sys_ParseArgs( argc, argv );
	char *argv0 = strdup(argv[ 0 ]);
	Sys_SetBinaryPath( Sys_Dirname( argv0 ) );
	free(argv0);
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );
	Sys_SetDefaultLibPath( DEFAULT_LIBDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < argc; i++ )
	{
		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );
		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	Com_Init( commandLine );
	NET_Init( );

	CON_Init( );

	// Windows doesn't have these signals
	// see CON_CtrlHandler() in con_win32.c
	Sys_PlatformInit( );

	if(!no_handle_signals)
	{
		signal(SIGILL, Sys_SigHandler);
		signal(SIGFPE, Sys_SigHandler);
		signal(SIGSEGV, Sys_SigHandler);
		signal(SIGTERM, Sys_SigHandler);
	}

	while( 1 )
	{
		if(!q3e_running) // exit
			break;
		if(!GLimp_CheckGLInitialized())
			break;
		Sys_Idle( );
		IN_Frame( );
		Com_Frame( );
	}

	return 0;
}

void ShutdownGame(void)
{
	if(q3e_running/* && com_fullyInitialized*/)
	{
		q3e_running = qfalse;
		NOTIFY_EXIT;
	}
}
