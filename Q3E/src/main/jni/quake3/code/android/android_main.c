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
	IN_Restart( );
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

#ifdef DEDICATED
#	define PID_FILENAME PRODUCT_NAME "_server.pid"
#else
#	define PID_FILENAME PRODUCT_NAME ".pid"
#endif

/*
=================
Sys_PIDFileName
=================
*/
static char *Sys_PIDFileName( const char *gamedir )
{
	const char *homePath = Cvar_VariableString( "fs_homepath" );

	if( *homePath != '\0' )
		return va( "%s/%s/%s", homePath, gamedir, PID_FILENAME );

	return NULL;
}

/*
=================
Sys_RemovePIDFile
=================
*/
void Sys_RemovePIDFile( const char *gamedir )
{
	char *pidFile = Sys_PIDFileName( gamedir );

	if( pidFile != NULL )
		remove( pidFile );
}

/*
=================
Sys_WritePIDFile

Return qtrue if there is an existing stale PID file
=================
*/
static qboolean Sys_WritePIDFile( const char *gamedir )
{
	char      *pidFile = Sys_PIDFileName( gamedir );
	FILE      *f;
	qboolean  stale = qfalse;

	if( pidFile == NULL )
		return qfalse;

	// First, check if the pid file is already there
	if( ( f = fopen( pidFile, "r" ) ) != NULL )
	{
		char  pidBuffer[ 64 ] = { 0 };
		int   pid;

		pid = fread( pidBuffer, sizeof( char ), sizeof( pidBuffer ) - 1, f );
		fclose( f );

		if(pid > 0)
		{
			pid = atoi( pidBuffer );
			if( !Sys_PIDIsRunning( pid ) )
				stale = qtrue;
		}
		else
			stale = qtrue;
	}

	if( FS_CreatePath( pidFile ) ) {
		return 0;
	}

	if( ( f = fopen( pidFile, "w" ) ) != NULL )
	{
		fprintf( f, "%d", Sys_PID( ) );
		fclose( f );
	}
	else
		Com_Printf( S_COLOR_YELLOW "Couldn't write %s.\n", pidFile );

	return stale;
}

/*
=================
Sys_InitPIDFile
=================
*/
void Sys_InitPIDFile( const char *gamedir ) {
	if( Sys_WritePIDFile( gamedir ) ) {
#ifndef DEDICATED
		char message[1024];
		char modName[MAX_OSPATH];

		FS_GetModDescription( gamedir, modName, sizeof ( modName ) );
		Q_CleanStr( modName );

		Com_sprintf( message, sizeof (message), "The last time %s ran, "
			"it didn't exit properly. This may be due to inappropriate video "
			"settings. Would you like to start with \"safe\" video settings?", modName );

		if( Sys_Dialog( DT_YES_NO, message, "Abnormal Exit" ) == DR_YES ) {
			Cvar_Set( "com_abnormalExit", "1" );
		}
#endif
	}
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
static __attribute__ ((noreturn)) void Sys_Exit( int exitCode )
{
	CON_Shutdown( );

	if( exitCode < 2 && com_fullyInitialized )
	{
		// Normal exit
		Sys_RemovePIDFile( FS_GetCurrentGameDir() );
	}

	NET_Shutdown( );

	Sys_PlatformExit( );

	exit( exitCode );
}

/*
=================
Sys_Quit
=================
*/
void Sys_Quit( void )
{
	Sys_Exit( 0 );
}

/*
=================
Sys_GetProcessorFeatures
=================
*/
cpuFeatures_t Sys_GetProcessorFeatures( void )
{
	cpuFeatures_t features = 0;

	return features;
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

	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	Sys_ErrorDialog( string );

	Sys_Exit( 3 );
}

#if 0
/*
=================
Sys_Warn
=================
*/
static __attribute__ ((format (printf, 1, 2))) void Sys_Warn( char *warning, ... )
{
	va_list argptr;
	char    string[1024];

	va_start (argptr,warning);
	Q_vsnprintf (string, sizeof(string), warning, argptr);
	va_end (argptr);

	CON_Print( va( "Warning: %s", string ) );
}
#endif

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
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void *Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void *dllhandle = NULL;

	if(!Sys_DllExtension(name))
	{
		Com_Printf("Refusing to attempt to load library \"%s\": Extension not allowed.\n", name);
		return NULL;
	}

	if(useSystemLib)
	{
		Com_Printf("Trying to load \"%s\"...\n", name);
		dllhandle = Sys_LoadLibrary(name);
	}
	
	if(!dllhandle)
	{
		const char *topDir;
		char libPath[MAX_OSPATH];
		int len;

		topDir = Sys_BinaryPath();

		if(!*topDir)
			topDir = ".";

		len = Com_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);
		if(len < sizeof(libPath))
		{
			Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, topDir);
			dllhandle = Sys_LoadLibrary(libPath);
		}
		else
		{
			Com_Printf("Skipping trying to load \"%s\" from \"%s\", file name is too long.\n", name, topDir);
		}

		if(!dllhandle)
		{
			const char *basePath = Cvar_VariableString("fs_basepath");
			
			if(!basePath || !*basePath)
				basePath = ".";
			
			if(FS_FilenameCompare(topDir, basePath))
			{
				len = Com_sprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				if(len < sizeof(libPath))
				{
					Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
					dllhandle = Sys_LoadLibrary(libPath);
				}
				else
				{
					Com_Printf("Skipping trying to load \"%s\" from \"%s\", file name is too long.\n", name, basePath);
				}
			}
			
			if(!dllhandle)
				Com_Printf("Loading \"%s\" failed\n", name);
		}
	}
	
	return dllhandle;
}

/*
=================
Sys_LoadGameDll

Used to load a development dll instead of a virtual machine
=================
*/
void *Sys_LoadGameDll(const char *name,
	vmMainProc *entryPoint,
	intptr_t (*systemcalls)(intptr_t, ...))
{
	void *libHandle;
	void (*dllEntry)(intptr_t (*syscallptr)(intptr_t, ...));

	assert(name);

	if(!Sys_DllExtension(name))
	{
		Com_Printf("Refusing to attempt to load library \"%s\": Extension not allowed.\n", name);
		return NULL;
	}

	Com_Printf( "Loading DLL file: %s\n", name);
	libHandle = Sys_LoadLibrary(name);

	if(!libHandle)
	{
		Com_Printf("Sys_LoadGameDll(%s) failed:\n\"%s\"\n", name, Sys_LibraryError());
		return NULL;
	}

	dllEntry = Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
		Com_Printf ( "Sys_LoadGameDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError( ) );
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	Com_Printf ( "Sys_LoadGameDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	return libHandle;
}

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
			const char* date = PRODUCT_DATE;
#ifdef DEDICATED
			fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", date );
#else
			fprintf( stdout, Q3_VERSION " client (%s)\n", date );
#endif
			Sys_Exit( 0 );
		}
	}
}

#ifdef PROTOCOL_HANDLER
/*
=================
Sys_ParseProtocolUri

This parses a protocol URI, e.g. "quake3://connect/example.com:27950"
to a string that can be run in the console, or a null pointer if the
operation is invalid or unsupported.
At the moment only the "connect" command is supported.
=================
*/
char *Sys_ParseProtocolUri( const char *uri )
{
	// Both "quake3://" and "quake3:" can be used
	if ( Q_strncmp( uri, PROTOCOL_HANDLER ":", strlen( PROTOCOL_HANDLER ":" ) ) )
	{
		Com_Printf( "Sys_ParseProtocolUri: unsupported protocol.\n" );
		return NULL;
	}
	uri += strlen( PROTOCOL_HANDLER ":" );
	if ( !Q_strncmp( uri, "//", strlen( "//" ) ) )
	{
		uri += strlen( "//" );
	}
	Com_Printf( "Sys_ParseProtocolUri: %s\n", uri );

	// At the moment, only "connect/hostname:port" is supported
	if ( !Q_strncmp( uri, "connect/", strlen( "connect/" ) ) )
	{
		int i, bufsize;
		char *out;

		uri += strlen( "connect/" );
		if ( *uri == '\0' || *uri == '?' )
		{
			Com_Printf( "Sys_ParseProtocolUri: missing argument.\n" );
			return NULL;
		}

		// Check for any unsupported characters
		// For safety reasons, the "hostname:port" part can only
		// contain characters from: a-zA-Z0-9.:-[]
		for ( i=0; uri[i] != '\0'; i++ )
		{
			if ( uri[i] == '?' )
			{
				// For forwards compatibility, any query string parameters are ignored (e.g. "?password=abcd")
				// However, these are not passed on macOS, so it may be a bad idea to add them.
				break;
			}

			if ( isalpha( uri[i] ) == 0 && isdigit( uri[i] ) == 0
				&& uri[i] != '.' && uri[i] != ':' && uri[i] != '-'
				&& uri[i] != '[' && uri[i] != ']' )
			{
				Com_Printf( "Sys_ParseProtocolUri: hostname contains unsupported character.\n" );
				return NULL;
			}
		}

		bufsize = strlen( "connect " ) + i + 1;
		out = malloc( bufsize );
		strcpy( out, "connect " );
		strncat( out, uri, i );
		return out;
	}
	else
	{
		Com_Printf( "Sys_ParseProtocolUri: unsupported command.\n" );
		return NULL;
	}
}
#endif

#ifndef DEFAULT_BASEDIR
#	ifdef __APPLE__
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
#	endif
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
		VM_Forced_Unload_Start();
#ifndef DEDICATED
		CL_Shutdown(va("Received signal %d", signal), qtrue, qtrue);
#endif
		SV_Shutdown(va("Received signal %d", signal) );
		VM_Forced_Unload_Done();
	}

	if( signal == SIGTERM || signal == SIGINT )
		Sys_Exit( 1 );
	else
		Sys_Exit( 2 );
}



#include "../client/client.h"
extern clientStatic_t cls;

/* Android */

extern qboolean com_fullyInitialized;

static void * game_main(void *data);

#include "sys_android.c"

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

// Quake3 game main thread loop
void * game_main(void *data)
{
	int   i;
	char  commandLine[ MAX_STRING_CHARS ] = { 0 };
#ifdef PROTOCOL_HANDLER
	char *protocolCommand = NULL;
#endif
	extern void Sys_LaunchAutoupdater(int argc, char **argv);

	attach_thread(); // attach current to JNI for call Android code
	Q3E_Start();

	Sys_LaunchAutoupdater(q3e_argc, q3e_argv);

	Sys_PlatformInit( );

	// Set the initial time base
	Sys_Milliseconds( );

	Sys_ParseArgs( q3e_argc, q3e_argv );
	Sys_SetBinaryPath( Sys_Dirname( q3e_argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < q3e_argc; i++ )
	{
		qboolean containsSpaces;

		// For security reasons we always detect --uri, even when PROTOCOL_HANDLER is undefined
		// Any arguments after "--uri quake3://..." is ignored
		if ( !strcmp( q3e_argv[i], "--uri" ) )
		{
#ifdef PROTOCOL_HANDLER
			if ( q3e_argc > i+1 )
			{
				protocolCommand = Sys_ParseProtocolUri( q3e_argv[i+1] );
			}
#endif
			break;
		}

		containsSpaces = strchr(q3e_argv[i], ' ') != NULL;
		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), q3e_argv[ i ] );

		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

#ifdef PROTOCOL_HANDLER
	if ( protocolCommand != NULL )
	{
		Q_strcat( commandLine, sizeof( commandLine ), "+" );
		Q_strcat( commandLine, sizeof( commandLine ), protocolCommand );
		free( protocolCommand );
	}
#endif

	CON_Init( );
	Com_Init( commandLine );
	NET_Init( );

	if(!no_handle_signals)
	{
		signal( SIGILL, Sys_SigHandler );
		signal( SIGFPE, Sys_SigHandler );
		signal( SIGSEGV, Sys_SigHandler );
		signal( SIGTERM, Sys_SigHandler );
		signal( SIGINT, Sys_SigHandler );
	}

	Q3E_FreeArgs();

	while( 1 )
	{
		if(!q3e_running) // exit
			break;
		Q3E_CheckNativeWindowChanged();
		Com_Frame( );
	}

	Q3E_End();
	main_thread = 0;
	//IsInitialized = false;
	Com_Printf("[Harmattan]: Leave Quake3 main thread.\n");
	return 0;
}

void ShutdownGame(void)
{
	if(com_fullyInitialized)
	{
		TRIGGER_WINDOW_CREATED; // if Quake3 main thread is waiting new window
		Q3E_ShutdownGameMainThread();
		//common->Quit();
	}
}

static void game_exit(void)
{
	Com_Printf("[Harmattan]: Quake3 exit.\n");

	Q3E_CloseRedirectOutput();
}

/*
==================
Sys_GetClipboardData
==================
*/
char *Sys_GetClipboardData(void)
{
#ifdef DEDICATED
    return NULL;
#else
    return Android_GetClipboardData();
#endif
}

void Sys_SyncState(void)
{
	//if (setState)
	{
		static int prev_state = -1;
		/* We are in game and neither console/ui is active */
		//if (cls.state == CA_ACTIVE && Key_GetCatcher() == 0)

		int state = ((clc.state == CA_ACTIVE) && (Key_GetCatcher() == 0)) << 1;

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

/*
=================
main
=================
*/
int main( int argc, char* argv[] ) {
	Q3E_DumpArgs(argc, argv);

	Q3E_RedirectOutput();

	Q3E_PrintInitialContext(argc, argv);

	INIT_Q3E_THREADS;

	Q3E_StartGameMainThread();

	atexit(game_exit);

	return 0;
}
