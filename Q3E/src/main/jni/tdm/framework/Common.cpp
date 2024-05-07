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



#include "../idlib/RevisionTracker.h"
#include "renderer/resources/Image.h"
#include "Session_local.h"
#include "Debug.h"
#include <iostream>

#include "GamepadInput.h"
#include "renderer/backend/RenderBackend.h"
#include "LoadStack.h"
#include "../game/Missions/MissionManager.h"
#include "../tests/TestRun.h"

#define MAX_WARNING_LIST	256

typedef enum {
	ERP_NONE,
	ERP_FATAL,						// exit the entire game with a popup window
	ERP_DROP,						// print to console and disconnect from game
	ERP_DISCONNECT					// don't kill server
} errorParm_t;

#if defined( _DEBUG )
	#define BUILD_DEBUG " debug"
#else
	#define BUILD_DEBUG ""
#endif

class EngineVersion
{
public:
	EngineVersion()
	{
		memset(string, 0, sizeof(string));
	}

	const char* Get()
	{
		if (string[0] == 0x0)
		{
			// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
			sprintf( string, "%s/%u #%d (%s)%s %s %s %s",
				ENGINE_VERSION,
				(int)sizeof(void*) * 8,
				RevisionTracker::Instance().GetHighestRevision(), RevisionTracker::Instance().GetRevisionString(),
				BUILD_DEBUG, BUILD_STRING, __DATE__, __TIME__
			);
		}

		return string;
	}

private:
	char	string[256];
};

EngineVersion engineVersion;

idCVar com_version( "si_version", "not set", CVAR_SYSTEM|CVAR_ROM|CVAR_SERVERINFO, "engine version" );
idCVar com_skipRenderer( "com_skipRenderer", "0", CVAR_BOOL|CVAR_SYSTEM, "skip the renderer completely" );
idCVar com_purgeAll( "com_purgeAll", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_SYSTEM, "purge everything between level loads" );
idCVar com_memoryMarker( "com_memoryMarker", "-1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_INIT, "used as a marker for memory stats" );
idCVar com_preciseTic( "com_preciseTic", "1", CVAR_BOOL|CVAR_SYSTEM, "run one game tick every async thread update" );
idCVar com_asyncInput( "com_asyncInput", "0", CVAR_BOOL | CVAR_SYSTEM, "sample input from the async thread" );
idCVar com_warning_as_error( "com_warning_as_error", "0", CVAR_BOOL | CVAR_SYSTEM, "when enabled to 1, all warnings trigger error" );
idCVar com_error_crash( "com_error_crash", "0", CVAR_INTEGER | CVAR_SYSTEM, "crash the hard way on the following:\n"
	"  fatal errors   when >= 1\n"
	"  normal errors  when >= 2\n"
	"  warnings       when >= 3\n"
, 0, 3);

#define ASYNCSOUND_INFO "0: mix sound inline, 1: memory mapped async mix, 2: callback mixing, 3: write async mix"
#if defined( MACOS_X )
idCVar com_asyncSound( "com_asyncSound", "2", CVAR_INTEGER|CVAR_SYSTEM|CVAR_ROM, ASYNCSOUND_INFO );
#elif defined( __linux__ )
idCVar com_asyncSound( "com_asyncSound", "3", CVAR_INTEGER|CVAR_SYSTEM|CVAR_ROM, ASYNCSOUND_INFO );
#else
idCVar com_asyncSound( "com_asyncSound", "1", CVAR_INTEGER|CVAR_SYSTEM, ASYNCSOUND_INFO, 0, 1 );
#endif

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_SYSTEM | CVAR_NOCHEAT,
	"Force specified implementation of SIMD processor (if supported)\n"
	"Value 1 or Generic forces slow platform-independent implementation. "
	"Other options include: SSE, SSE2, SSSE3, AVX, AVX2, [Win32]:IdAsm)"
);
idCVar com_fpexceptions( "com_fpexceptions", "0", CVAR_BOOL | CVAR_SYSTEM, "enable FP exceptions: throw exception when NaN or Inf value is produced" );
idCVar com_developer( "developer", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "developer mode" );
#ifdef __ANDROID__ //karin: allow console default
idCVar com_allowConsole( "com_allowConsole", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "allow toggling console with the tilde key" );
#else
idCVar com_allowConsole( "com_allowConsole", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "allow toggling console with the tilde key" );
#endif
idCVar com_speeds( "com_speeds", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show engine timings" );
idCVar com_showFPS( "com_showFPS", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_ARCHIVE|CVAR_NOCHEAT, "show frames rendered per second" );
idCVar com_showFPSavg( "com_showFPSavg", "4", CVAR_INTEGER|CVAR_SYSTEM|CVAR_ARCHIVE|CVAR_NOCHEAT, "number of frames averaged in com_showFPS", 1.0f, 100.0f );
idCVar com_showMemoryUsage( "com_showMemoryUsage", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show total and per frame memory usage" );
idCVar com_showAsyncStats( "com_showAsyncStats", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show async network stats" );
idCVar com_showSoundDecoders( "com_showSoundDecoders", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show sound decoders" );

idCVar com_timestampPrints( "com_timestampPrints", "0", CVAR_SYSTEM, "print time with each console print, 1 = sec, 2 = msec", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar com_timescale( "timescale", "1", CVAR_SYSTEM | CVAR_FLOAT, "scales the time", 0.1f, 10.0f );
idCVar com_logFile( "logFile", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "1 = buffer log, 2 = flush after each print", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar com_logFileName( "logFileName", "qconsole.log", CVAR_SYSTEM | CVAR_NOCHEAT, "name of log file, if empty, qconsole.log will be used" );
idCVar com_makingBuild( "com_makingBuild", "0", CVAR_BOOL | CVAR_SYSTEM, "1 when making a build" );
idCVar com_updateLoadSize( "com_updateLoadSize", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "update the load size after loading a map" );
//idCVar com_videoRam( "com_videoRam", "128", CVAR_INTEGER | CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "holds the last amount of detected video ram" );

idCVar com_product_lang_ext( "com_product_lang_ext", "1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_ARCHIVE, "Extension to use when creating language files." );


// com_speeds times
int				time_gameFrame;
int				time_gameDraw;
int				time_frontend;			// renderSystem frontend time
int				time_backend;			// renderSystem backend time
int				time_frontendLast;
int				time_backendLast;

int				com_frameTime;			// time moment of the current frame in milliseconds
int				com_frameDelta;			// time elapsed since previous frame in milliseconds
int				com_frameNumber;		// variable frame number
std::atomic<int>	com_ticNumber;			// 60 hz tics
int				com_editors;			// currently opened editor(s)
bool			com_editorActive;		// true if an editor has focus

#ifdef _WIN32
HWND			com_hwndMsg = NULL;
bool			com_outputMsg = false;
unsigned int	com_msgID = -1;
#endif

#ifdef __DOOM_DLL__
idGame *		game = NULL;
idGameEdit *	gameEdit = NULL;
#endif

void Com_Crash_f( const idCmdArgs &args );

// writes si_version to the config file - in a kinda obfuscated way
//#define ID_WRITE_VERSION

class idCommonLocal : public idCommon {
public:
								idCommonLocal( void );

	virtual void				Init( int argc, const char **argv, const char *cmdline ) override;
	virtual void				Shutdown( void ) override;
	virtual void				Quit( void ) override;
	virtual bool				IsInitialized( void ) const override;
	virtual void				Frame( void ) override;
	virtual void				GUIFrame( bool execCmd, bool network ) override;
	virtual void				Async( void ) override;
	virtual void				StartupVariable( const char *match, bool once ) override;
	virtual void				InitTool( const toolFlag_t tool, const idDict *dict ) override;
	virtual void				ActivateTool( bool active ) override;
    virtual void				WriteConfigToFile( const char *filename, const char* basePath = "fs_savepath", const eConfigExport configexport = eConfigExport_all) override;
	virtual void				WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) override;
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) override;
	virtual void				EndRedirect( void ) override;
	virtual void				SetRefreshOnPrint( bool set ) override;
	virtual void				Printf( const char *fmt, ... ) override id_attribute((format(printf,2,3)));
	virtual void				VPrintf( const char *fmt, va_list arg ) override;
	virtual void				DPrintf( const char *fmt, ... ) override id_attribute((format(printf,2,3)));
	virtual void				PrintCallStack() override;
	virtual void				Warning( const char *fmt, ... ) override id_attribute((format(printf,2,3)));
	virtual void				DWarning( const char *fmt, ...) override id_attribute((format(printf,2,3)));
	virtual void				PrintWarnings( void ) override;
	virtual void				ClearWarnings( const char *reason ) override;
	virtual void				PacifierUpdate( loadkey_t key, int count ) override; // grayman #3763
	virtual void				Error( const char *fmt, ... ) override id_attribute((format(printf,2,3)));
	virtual void				DoError( const char *msg, int code ) override;
	virtual void				FatalError( const char *fmt, ... ) override id_attribute( ( format( printf, 2, 3 ) ) );
	virtual void				DoFatalError( const char *msg, int code ) override;
	virtual void				SetErrorIndirection( bool enable ) override;
	virtual const char*			Translate( const char* str ) override;
	virtual bool                WindowAvailable(void) override; // Agent Jones #3766

	virtual I18N*				GetI18N() override;

	virtual const char *		KeysFromBinding( const char *bind ) override;
	virtual const char *		BindingFromKey( const char *key ) override;

	virtual int					ButtonState( int key ) override;
	virtual int					KeyState( int key ) override;

	//stgatilov: for automation
	virtual int					GetConsoleMarker() override;
	virtual idStr				GetConsoleContents(int begin, int end) override;

	void						InitGame( void );
	void						ShutdownGame( bool reloading );

	// localization
	void						InitLanguageDict( void );
	void						LocalizeGui( const char *fileName, idLangDict &langDict );
	void						LocalizeMapData( const char *fileName, idLangDict &langDict );
	void						LocalizeSpecificMapData( const char *fileName, idLangDict &langDict, const idLangDict &replaceArgs );

	//void						SetMachineSpec( void );

private:
	void						InitCommands( void );
	void						InitRenderSystem( void );
	void						InitSIMD( void );
	bool						AddStartupCommands( void );
	void						ParseCommandLine( int argc, const char **argv );
	void						ClearCommandLine( void );
	bool						SafeMode( void );
	void						CheckToolMode( void );
	void						CloseLogFile( void );
	void						WriteConfiguration( void );
	void						DumpWarnings( void );
	void						SingleAsyncTic( void );
	void						LoadGameDLL( void );
	void						UnloadGameDLL( void );
	idStr						CallStackToString( int dropLastCalls = 1 );
	//void						PrintLoadingMessage( const char *msg );

	// greebo: used to initialise the fs_currentfm/fs_mod parameters
	void						InitGameArguments(idStrList *newModsList);

	bool						com_fullyInitialized;
	bool						com_refreshOnPrint;		// update the screen every print for dmap
	int							com_errorEntered;		// 0, ERP_DROP, etc
	bool						com_shuttingDown;

	bool						errorIndirection;

	idFile *					logFile;

	char						errorMessage[MAX_PRINT_MSG_SIZE];

	char *						rd_buffer;
	int							rd_buffersize;
	void						(*rd_flush)( const char *buffer );

	idStr						warningCaption;
	idStrList					warningList;
	idStrList					errorList;

    uintptr_t					gameDLL;

#ifdef ID_WRITE_VERSION
	idCompressor *				config_compressor;
#endif
};

idCommonLocal	commonLocal;
idCommon *		common = &commonLocal;

// The instance is located in I18N.cpp
extern I18N* i18n;

/*
==================
idCommonLocal::idCommonLocal
==================
*/
idCommonLocal::idCommonLocal( void ) {
	com_fullyInitialized = false;
	com_refreshOnPrint = false;
	com_errorEntered = 0;
	com_shuttingDown = false;
	errorIndirection = false;

	logFile = NULL;

	strcpy( errorMessage, "" );

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;

	gameDLL = 0;

#ifdef ID_WRITE_VERSION
	config_compressor = NULL;
#endif
}

/*
==================
idCommonLocal::BeginRedirect
==================
*/
void idCommonLocal::BeginRedirect( char *buffer, int buffersize, void (*flush)( const char *) ) {
	if ( !buffer || !buffersize || !flush ) {
		return;
	}

	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

/*
==================
idCommonLocal::EndRedirect
==================
*/
void idCommonLocal::EndRedirect( void ) {
	if ( rd_flush && rd_buffer[ 0 ] ) {
		rd_flush( rd_buffer );
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

#ifdef _WIN32

/*
==================
EnumWindowsProc
==================
*/
BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam ) {
	char buff[1024];

	::GetWindowText( hwnd, buff, sizeof( buff ) );
    if (idStr::Icmpn(buff, EDITOR_WINDOWTEXT, static_cast<int>(strlen(EDITOR_WINDOWTEXT))) == 0) {
		com_hwndMsg = hwnd;
		return FALSE;
	}

	return TRUE;
}

/*
==================
FindEditor
==================
*/
bool FindEditor( void ) {
	com_hwndMsg = NULL;
	EnumWindows( EnumWindowsProc, 0 );
	return !( com_hwndMsg == NULL );
}

#endif

/*
==================
idCommonLocal::CloseLogFile
==================
*/
void idCommonLocal::CloseLogFile( void ) {
	if ( logFile ) {
		com_logFile.SetBool( false ); // make sure no further VPrintf attempts to open the log file again
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}
}

/*
==================
idCommonLocal::SetRefreshOnPrint
==================
*/
void idCommonLocal::SetRefreshOnPrint( bool set ) {
	com_refreshOnPrint = set;
}

/*
==================
idCommonLocal::VPrintf

A raw string should NEVER be passed as fmt, because of "%f" type crashes.
==================
*/
void idCommonLocal::VPrintf( const char *fmt, va_list args ) {
	char		msg[MAX_PRINT_MSG_SIZE];
	int			timeLength;
	static bool	logFileFailed = false;

	// if the cvar system is not initialized
	if ( !cvarSystem->IsInitialized() ) {
		return;
	}

	// optionally put a timestamp at the beginning of each print,
	// so we can see how long different init sections are taking
	if ( com_timestampPrints.GetInteger() ) {
		unsigned int t = Sys_Milliseconds();
		if ( com_timestampPrints.GetInteger() == 1 ) {
			t /= 1000;
		}
		sprintf( msg, "[%i]", t );
        timeLength = static_cast<int>(strlen(msg));
	} else {
		timeLength = 0;
	}

	// don't overflow
	if ( idStr::vsnPrintf( msg+timeLength, MAX_PRINT_MSG_SIZE-timeLength-1, fmt, args ) < 0 ) {
		msg[sizeof(msg)-2] = '\n'; msg[sizeof(msg)-1] = '\0'; // avoid output garbling
		Sys_Printf( "idCommon::VPrintf: truncated to %zu characters\n", strlen(msg)-1 );
	}

	if ( rd_buffer ) {
		if ( (int)( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) ) {
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}
		strcat( rd_buffer, msg );
		return;
	}

	// echo to console buffer
	console->Print( msg );

	// remove any color codes
	idStr::RemoveColors( msg );

	// echo to dedicated console and early console
	Sys_Printf( "%s", msg );

	// print to script debugger server
	// DebuggerServerPrint( msg );

#if 0	// !@#
#if defined(_DEBUG) && defined(WIN32)
	if ( strlen( msg ) < 512 ) {
		TRACE( msg );
	}
#endif
#endif

	// logFile
	if ( com_logFile.GetInteger() && !logFileFailed && fileSystem->IsInitialized() ) {
		static bool recursing;

		if ( !logFile && !recursing ) {
			struct tm *newtime;
			ID_TIME_T aclock;
			const char *fileName = com_logFileName.GetString()[0] ? com_logFileName.GetString() : "qconsole.log";

			// fileSystem->OpenFileWrite can cause recursive prints into here
			recursing = true;

			logFile = fileSystem->OpenFileWrite( fileName );
			if ( !logFile ) {
				logFileFailed = true;
				FatalError( "failed to open log file '%s'\n", fileName );
			}

			recursing = false;

			if ( com_logFile.GetInteger() > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				logFile->ForceFlush();
			}

			time( &aclock );
			newtime = localtime( &aclock );
			Printf( "log file '%s' opened on %s\n", fileName, asctime( newtime ) );
		}
		if ( logFile ) {
            logFile->Write(msg, static_cast<int>(strlen(msg)));
			logFile->Flush();	// ForceFlush doesn't help a whole lot
		}
	}

	// don't trigger any updates if we are in the process of doing a fatal error
	if ( com_errorEntered != ERP_FATAL ) {
		// update the console if we are in a long-running command, like dmap
		if ( com_refreshOnPrint ) {
			session->UpdateScreen();
		}

		// let session redraw the animated loading screen if necessary
		//session->PacifierUpdate(); // grayman #3763 - no longer needed here
	}

#ifdef _WIN32

	if ( com_outputMsg ) {
		if ( com_msgID == -1 ) {
			com_msgID = ::RegisterWindowMessage( DMAP_MSGID );
			if ( !FindEditor() ) {
				com_outputMsg = false;
			} else {
				Sys_ShowWindow( false );
			}
		}
		if ( com_hwndMsg ) {
			ATOM atom = ::GlobalAddAtom( msg );
			::PostMessage( com_hwndMsg, com_msgID, 0, static_cast<LPARAM>(atom) );
		}
	}

#endif
}

int idCommonLocal::GetConsoleMarker() {
	if (!logFile) return 0;
	return logFile->Tell();
}
idStr idCommonLocal::GetConsoleContents(int begin, int end) {
	if (!logFile) return idStr();
	int original = logFile->Tell();
	assert(original >= end);
	int len = end - begin;
	idFile *f = fileSystem->OpenFileRead(logFile->GetName());
	f->Seek(begin, FS_SEEK_SET);
	idStr res;
	res.Fill(' ', len);
	f->Read((char*)res.c_str(), len);
	delete f;
	return res;
}


/*
==================
idCommonLocal::Printf

Both client and server can use this, and it will output to the appropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
==================
*/
void idCommonLocal::Printf( const char *fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
==================
idCommonLocal::DPrintf

prints message that only shows up if the "developer" cvar is set
==================
*/
void idCommonLocal::DPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];
		
	if ( !cvarSystem->IsInitialized() || !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = '\0';
	
	// never refresh the screen, which could cause reentrency problems
	bool temp = com_refreshOnPrint;
	com_refreshOnPrint = false;

	Printf( S_COLOR_RED "%s", msg );

	com_refreshOnPrint = temp;
}

/*
==================
idCommonLocal::DWarning

prints warning message in yellow that only shows up if the "developer" cvar is set
==================
*/
void idCommonLocal::DWarning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];
		
	if ( !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = '\0';

	Printf( S_COLOR_YELLOW "WARNING:" S_COLOR_RED "%s\n", msg );
}


idStr idCommonLocal::CallStackToString(int dropLastCalls) {
	uint8_t traceData[4096];
	int traceSize = sizeof(traceData);
	uint32_t hash = idDebugSystem::GetStack(traceData, traceSize);

	idList<debugStackFrame_t> callstack;
	idDebugSystem::DecodeStack(traceData, traceSize, callstack);
	idDebugSystem::CleanStack(callstack);

	for (int t = 0; t < dropLastCalls; t++)
		callstack.RemoveIndex(0);
	char message[32<<10];
	idDebugSystem::StringifyStack(hash, callstack.Ptr(), callstack.Num(), message, sizeof(message));
	return message;
}
void idCommonLocal::PrintCallStack() {
	idStr message = CallStackToString(1);
	Printf("%s", message.c_str());
}

/*
==================
idCommonLocal::Warning

prints WARNING %s and adds the warning message to a queue to be printed later on
==================
*/
void idCommonLocal::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];
		
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = '\0';

	Printf( S_COLOR_YELLOW "WARNING:" S_COLOR_RED "%s\n", msg );

	if ( warningList.Num() < MAX_WARNING_LIST ) {
		warningList.AddUnique( msg );
	}
	if ( com_error_crash.GetInteger() >= 3 ) {
		Com_Crash_f(idCmdArgs());
	}
	if ( com_warning_as_error.GetBool() ) {
		Error("Promoted to error (com_warning_as_error = 1)");
	}
}

/*
==================
idCommonLocal::PrintWarnings
==================
*/
void idCommonLocal::PrintWarnings( void ) {
	if ( !warningList.Num() ) {
		return;
	}

	warningList.Sort();

	Printf( "------------- Warnings ---------------\n" );
	Printf( "during %s...\n", warningCaption.c_str() );

	for ( int i = 0; i < warningList.Num(); i++ ) {
		Printf( S_COLOR_YELLOW "WARNING:" S_COLOR_RED "%s\n", warningList[i].c_str() );
	}
	if ( warningList.Num() ) {
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			Printf( "more than %d warnings\n", MAX_WARNING_LIST );
		} else {
			Printf( "%d warnings\n", warningList.Num() );
		}
	}
}

/*
==================
idCommonLocal::ClearWarnings
==================
*/
void idCommonLocal::ClearWarnings( const char *reason ) {
	warningCaption = reason;
	warningList.ClearFree();
}

/*
==================
idCommonLocal::PacifierUpdate
==================
*/
void idCommonLocal::PacifierUpdate(loadkey_t key, int count) // grayman #3763
{
	session->PacifierUpdate(key, count);
}


/*
==================
idCommonLocal::DumpWarnings
==================
*/
void idCommonLocal::DumpWarnings( void ) {
	int			i;
	idFile		*warningFile;

	if ( !warningList.Num() ) {
		return;
	}

	warningFile = fileSystem->OpenFileWrite( "warnings.txt", "fs_savepath", "" );
	if ( warningFile ) {
		warningFile->Printf( "------------- Warnings ---------------\n\n" );
		warningFile->Printf( "during %s...\n", warningCaption.c_str() );
		warningList.Sort();
		for ( i = 0; i < warningList.Num(); i++ ) {
			warningList[i].RemoveColors();
			warningFile->Printf( "WARNING:%s\n", warningList[i].c_str() );
		}
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			warningFile->Printf( "\nmore than %d warnings!\n", MAX_WARNING_LIST );
		} else {
			warningFile->Printf( "\n%d warnings.\n", warningList.Num() );
		}

		warningFile->Printf( "\n-------------- Errors ---------------\n\n" );
		errorList.Sort();
		for ( i = 0; i < errorList.Num(); i++ ) {
			errorList[i].RemoveColors();
			warningFile->Printf( "ERROR:%s", errorList[i].c_str() );
		}

		warningFile->ForceFlush();

		fileSystem->CloseFile( warningFile );

#if defined(_WIN32) && !defined(_DEBUG)
		idStr	osPath;
		osPath = fileSystem->RelativePathToOSPath( "warnings.txt", "fs_savepath", "" );
		WinExec( va( "Notepad.exe %s", osPath.c_str() ), SW_SHOW );
#endif
	}
}

/*
==================
idCommonLocal::Error
==================
*/
void idCommonLocal::Error( const char *fmt, ... ) {
	va_list		argptr;

	if ( com_error_crash.GetInteger() >= 2 ) {
		Com_Crash_f(idCmdArgs());
	}

	int code = ERP_DROP;

	// if we don't have GL running, make it a fatal error
	if ( !renderSystem->IsOpenGLRunning() ) {
		code = ERP_FATAL;
	}

	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog
		if ( com_errorEntered == ERP_FATAL ) {
			if ( logFile ) {	//try to print something to log file
				static const char *UBER_ERROR = "Recursive fatal error!\n";
				logFile->Write(UBER_ERROR, idStr::Length(UBER_ERROR));
				logFile->Flush();
			}
			assert(false);		//break in debug build
			Sys_Quit();
		}
		code = ERP_FATAL;
	}

	com_errorEntered = code;

	char msgBuf[MAX_PRINT_MSG_SIZE];
	va_start( argptr, fmt );
	idStr::vsnPrintf( msgBuf, sizeof( msgBuf ), fmt, argptr );
	va_end (argptr);
	msgBuf[sizeof(msgBuf)-1] = '\0';

	if( errorIndirection )
		throw std::make_shared<ErrorReportedException>(msgBuf, code, false);

	DoError( msgBuf, code );
}

/*
==================
idCommonLocal::DoError
==================
*/
void idCommonLocal::DoError( const char *msg, int code ) {
	static int	lastErrorTime;
	static int	errorCount;

	// always turn this off after an error
	com_refreshOnPrint = false;

	// if we are getting a solid stream of ERP_DROP, do an ERP_FATAL
	int currentTime = Sys_Milliseconds();
	if( currentTime - lastErrorTime < 100 ) {
		if( ++errorCount > 3 ) {
			code = ERP_FATAL;
		}
	}
	else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	com_errorEntered = code;

	// copy the error message to the clip board
	Sys_SetClipboardData( msg );

	// add the message to the error list
	errorList.AddUnique( msg );

	// Dont shut down the session for gui editor or debugger
	if( !( com_editors & ( EDITOR_GUI ) ) ) {
		session->Stop();
	}

	if( code == ERP_DISCONNECT ) {
		com_errorEntered = 0;
		throw idException( msg );
		// The gui editor doesnt want thing to com_error so it handles exceptions instead
	}
	else if( com_editors & ( EDITOR_GUI ) ) {
		com_errorEntered = 0;
		throw idException( msg );
	}
	else if( code == ERP_DROP ) {
		Printf( S_COLOR_RED "ERROR:%s\n" S_COLOR_DEFAULT "--------------------------------------\n", msg );
		com_errorEntered = 0;
		throw idException( msg );
	}
	else {
		Printf( S_COLOR_RED "ERROR:%s\n" S_COLOR_DEFAULT "--------------------------------------\n", msg );
	}

	if( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Shutdown();

	Sys_Error( "%s", msg );
}

/*
==================
idCommonLocal::FatalError

Dump out of the game to a system dialog
==================
*/
void idCommonLocal::FatalError( const char *fmt, ... ) {
	va_list		argptr;
	char msgBuf[MAX_PRINT_MSG_SIZE];

	if ( com_error_crash.GetInteger() >= 1 ) {
		Com_Crash_f(idCmdArgs());
	}

	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog

		Sys_Printf( "FATAL:recursed fatal error:\n" );

		va_start( argptr, fmt );
		idStr::vsnPrintf( msgBuf, sizeof(msgBuf), fmt, argptr );
		va_end( argptr );
		msgBuf[sizeof(msgBuf)-1] = '\0';

		Sys_Printf( "%s\n", msgBuf );

		// write the console to a log file?
		Sys_Quit();
	}
	com_errorEntered = ERP_FATAL;

	va_start( argptr, fmt );
	idStr::vsnPrintf( msgBuf, sizeof(msgBuf), fmt, argptr );
	va_end( argptr );
	msgBuf[sizeof(msgBuf)-1] = '\0';

	if( errorIndirection )
		throw std::make_shared<ErrorReportedException>( msgBuf, ERP_FATAL, true );

	DoFatalError( msgBuf, ERP_FATAL );
}

/*
==================
idCommonLocal::DoFatalError

Dump out of the game to a system dialog
==================
*/
void idCommonLocal::DoFatalError( const char *msg, int code ) {
	com_errorEntered = ERP_FATAL;

	if( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Sys_SetFatalError( msg );

	Shutdown();

	Sys_Error( "%s", msg );
}

void idCommonLocal::SetErrorIndirection( bool enable ) {
	errorIndirection = enable;
}

/*
==================
idCommonLocal::Quit
==================
*/
void idCommonLocal::Quit( void ) {

#ifdef ID_ALLOW_TOOLS
	if ( com_editors & EDITOR_RADIANT ) {
		RadiantInit();
		return;
	}
#endif

	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		Shutdown();
	}

	Sys_Quit();
}


/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters separate the commandLine string into multiple console
command lines.

All of these are valid:

thedarkmod +set test blah +map test
thedarkmod set test blah+map test
thedarkmod set test blah + map test

============================================================================
*/

#define		MAX_CONSOLE_LINES	32
int			com_numConsoleLines;
idCmdArgs	com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
idCommonLocal::ParseCommandLine
==================
*/
void idCommonLocal::ParseCommandLine( int argc, const char **argv ) {

	com_numConsoleLines = 0;
	
    // API says no program path
	for ( int i = 0; i < argc; i++ ) {
		if ( argv[ i ][ 0 ] == '+' ) {
			com_numConsoleLines++;
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[ i ] + 1 );
		} else {
			if ( !com_numConsoleLines ) {
				com_numConsoleLines++;
			}
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[ i ] );
		}
	}
}

void idCommonLocal::InitGameArguments(idStrList *newModsList) {
	bool fsGameDefined = false;
	bool fsGameBaseDefined = false;
	bool fsBasePathDefined = false;
	idStr basePath = Sys_DefaultBasePath(); // might be overridden by the arguments below
	
    // taaaki - while we want to be able to support mods in the future, I want to limit
    //          this ability until the TDM and D3 source merge is in a better state.
    bool enableMods = true;
    idStr fsGameBase = BASE_TDM;
    fsGameBaseDefined = true;
    
	// Search the command line arguments for certain override parameters
	for (int line = 0; line < com_numConsoleLines; ++line) {
		const idCmdArgs& args = com_consoleLines[line];

		for (int arg = 0; arg < args.Argc(); ++arg) {
			fsGameDefined |= idStr::Cmp(args.Argv(arg), "fs_currentfm") == 0;

			if (enableMods && idStr::Cmp(args.Argv(arg), "fs_mod") == 0) {
				fsGameBaseDefined = true;
				fsGameBase = (args.Argc() > arg + 1) ? args.Argv(arg + 1) : "";
			}
			
			if (idStr::Cmp(args.Argv(arg), "fs_basepath") == 0) {
				fsBasePathDefined = true;
				basePath = (args.Argc() > arg + 1) ? args.Argv(arg + 1) : "";
			}
		}
	}

    // Construct the darkmod path - use fs_mod, if specified and different from "darkmod"
	idStr darkmodPath = basePath;
    if ( idStr::Icmp( fsGameBase.c_str(), BASE_TDM ) ) {
        darkmodPath.AppendPath(fsGameBase);
    }
	
	if ( !fsGameDefined ) {
		// no fs_currentfm defined, try to load the currentfm.txt from darkmod
		idStr currentModFile = darkmodPath;
		currentModFile.AppendPath("currentfm.txt");
		idStr mod;

		// this will just read the content of a file - in this case, to get the fm name from
		// the aforementioned currentModFile
		{
			FILE* file = fopen(currentModFile.c_str(), "r");
			if ( file )	{
				fseek(file, 0, SEEK_END);
				size_t len = ftell(file);
				fseek(file, 0, SEEK_SET);

				char* buf = (char*)malloc(len+1);
				if ( buf ) {
					size_t bytesRead = fread(buf, 1, len, file);

					if (bytesRead != len) {
						common->Warning("Reading currentfm: (%s) bytes read mismatches file length?", currentModFile.c_str());
					}

					buf[len] = 0;
					mod = buf;
					mod.StripTrailingWhitespace();

					free(buf);
					fclose(file);
				} else {
					free(buf);
					fclose(file);
					common->FatalError("Reading currentfm:: Failed to allocate memory for read.");
				}
			}
		}

		if (!mod.IsEmpty()) {
			// Set the fs_currentfm CVAR to the value found in the .txt file
			cvarSystem->SetCVarString("fs_currentfm", mod.c_str());
			fsGameDefined = true;

			// Set the fs_mod parameter too
			cvarSystem->SetCVarString("fs_mod", fsGameBase);
			fsGameBaseDefined = true;
		}
	}

    // If we still don't have fs_mod, fall back to "darkmod"
    if ( !fsGameBaseDefined ) {
        cvarSystem->SetCVarString("fs_mod", BASE_TDM);
		fsGameBaseDefined = true;
	}

    // look for fms folder
    idStrList fmList;
    idStr fmPath = darkmodPath;

    Sys_ListFiles( fmPath.c_str(), "/", fmList );
    fmPath.AppendPath("fms");

    if ( fmList.FindIndex( "fms" ) < 0 ) {
        // didn't find the fms folder
        // do we create one? I don't see why not
        Sys_Mkdir( fmPath.c_str() );
        fsGameDefined = false;
        common->Warning("Fan missions directory does not exist");
    }

	// stgatilov #5661: look for new pk4 files in fms/ directory
	// and copy them to fms/{modname}/ subdirectory if found
	// Note: pk4 of the current FM can only be replaced before fileSystem is initialized!
	idStrList newMods = CMissionManager::SearchForNewMods(fmPath);
	if (newModsList)
		*newModsList = newMods;

    if ( fsGameDefined ) {
        idStr curFm = idStr( cvarSystem->GetCVarString("fs_currentfm") );
        Sys_ListFiles( fmPath.c_str(), "/", fmList );

        // check if the currently selected fm folder exists in <fs_mod>/fms/
        if ( fmList.FindIndex( curFm ) < 0 ) {
            fsGameDefined = false;
            common->Warning("Fan missions path does not exist for installed fm: %s", curFm.c_str());
        } else {
            idStrList pk4List, mapList;
            fmPath.AppendPath(curFm);
            Sys_ListFiles( fmPath.c_str(), ".pk4", pk4List );

            fmPath.AppendPath("maps");
            Sys_ListFiles( fmPath.c_str(), ".map", mapList );

            if ( pk4List.Num() == 0 && mapList.Num() == 0 ) {
                // fanmission folder exists, but didn't find a pk4 or map file
                fsGameDefined = false;
                common->Warning("Fan mission map does not exist for installed fm: %s", curFm.c_str());
            }
        }
    }

	// If we still don't have fs_currentfm or the fm directory does not 
    // exist, fall back to "darkmod"
	if ( !fsGameDefined ) {
		cvarSystem->SetCVarString("fs_currentfm", BASE_TDM);
		fsGameDefined = true;
	}
}

/*
==================
idCommonLocal::ClearCommandLine
==================
*/
void idCommonLocal::ClearCommandLine( void ) {
	com_numConsoleLines = 0;
}

/*
==================
idCommonLocal::SafeMode

Check for "safe" on the command line, which will
skip loading of config file (Darkmod.cfg)
==================
*/
bool idCommonLocal::SafeMode( void ) {
	for ( int i = 0 ; i < com_numConsoleLines ; i++ ) {
		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "safe" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "cvar_restart" ) ) {
			com_consoleLines[ i ].Clear();
			return true;
		}
	}
	return false;
}

/*
==================
idCommonLocal::CheckToolMode

Check for "renderbump", "dmap", or "editor" on the command line,
and force fullscreen off in those cases
==================
*/
void idCommonLocal::CheckToolMode( void ) {
	for ( int i = 0 ; i < com_numConsoleLines ; i++ ) {
		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "guieditor" ) ) {
			com_editors |= EDITOR_GUI;
		}
		else if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "editor" ) ) {
			com_editors |= EDITOR_RADIANT;
		}
		// Nerve: Add support for the material editor
		else if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "materialEditor" ) ) {
			com_editors |= EDITOR_MATERIAL;
		}
		
		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "renderbump" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "editor" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "guieditor" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "debugger" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "dmap" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "materialEditor" )
			) {
			cvarSystem->SetCVarBool( "r_fullscreen", false );
			return;
		}
	}
}

/*
==================
idCommonLocal::StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
==================
*/
void idCommonLocal::StartupVariable( const char *match, bool once ) {
	const char *s;

	for ( int i = 0; i < com_numConsoleLines; i++) {
		if ( strcmp( com_consoleLines[ i ].Argv( 0 ), "set" ) ) {
			continue;
		}

		s = com_consoleLines[ i ].Argv(1);
		if ( !match || !idStr::Icmp( s, match ) ) {
			cvarSystem->SetCVarString( s, com_consoleLines[ i ].Argv( 2 ) );
			if ( once ) {
				// kill the line
				for ( int j = i + 1; j < com_numConsoleLines; j++ ) {
					com_consoleLines[ j - 1 ] = com_consoleLines[ j ];
				}
				com_numConsoleLines--;
			}
		}
	}
}

/*
==================
idCommonLocal::AddStartupCommands

Adds command line parameters as script statements
Commands are separated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
==================
*/
bool idCommonLocal::AddStartupCommands( void ) {
	bool added = false;

	// quote every token, so args with semicolons can work
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		if ( !com_consoleLines[i].Argc() ) {
			continue;
		}

		// set commands won't override menu startup
		if ( idStr::Icmpn( com_consoleLines[i].Argv(0), "set", 3 ) ) {
			added = true;
		}

		// directly as tokenized so nothing gets screwed
		cmdSystem->BufferCommandArgs( CMD_EXEC_APPEND, com_consoleLines[i] );
	}

	return added;
}

/*
=================
idCommonLocal::InitTool
=================
*/
void idCommonLocal::InitTool( const toolFlag_t tool, const idDict *dict ) {
#ifdef ID_ALLOW_TOOLS
	if ( tool & EDITOR_SOUND ) {
		SoundEditorInit( dict );
	} else if ( tool & EDITOR_LIGHT ) {
		LightEditorInit( dict );
	} else if ( tool & EDITOR_PARTICLE ) {
		ParticleEditorInit( dict );
	} else if ( tool & EDITOR_AF ) {
		AFEditorInit( dict );
	}
#endif
}

/*
==================
idCommonLocal::ActivateTool

Activates or Deactivates a tool
==================
*/
void idCommonLocal::ActivateTool( bool active ) {
	com_editorActive = active;
	Sys_GrabMouseCursor( !active );
}

/*
==================
idCommonLocal::WriteFlaggedCVarsToFile
==================
*/
void idCommonLocal::WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) {
	idFile *f;

	f = fileSystem->OpenFileWrite( filename, "fs_savepath", "" );
	if ( !f ) {
		Printf( "Couldn't write %s.\n", filename );
		return;
	} else {
		cvarSystem->WriteFlaggedVariables( flags, setCmd, f );
		fileSystem->CloseFile( f );
	}
}

/*
==================
idCommonLocal::WriteConfigToFile
==================
*/
void idCommonLocal::WriteConfigToFile( 
	const char*						filename, 
	const char*						basePath, 
	const eConfigExport	configexport)
{
	idFile *f;
#ifdef ID_WRITE_VERSION
	ID_TIME_T t;
	char *curtime;
	idStr runtag;
	idFile_Memory compressed( "compressed" );
	idBase64 out;
#endif

	f = fileSystem->OpenFileWrite( filename, basePath, "" );
	if ( !f ) {
		Printf ("Couldn't write %s.\n", filename );
		return;
	}

#ifdef ID_WRITE_VERSION
	assert( config_compressor );
	t = time( NULL );
	curtime = ctime( &t );
	sprintf( runtag, "%s - %s", cvarSystem->GetCVarString( "si_version" ), curtime );
	config_compressor->Init( &compressed, true, 8 );
	config_compressor->Write( runtag.c_str(), runtag.Length() );
	config_compressor->FinishCompress( );
	out.Encode( (const byte *)compressed.GetDataPtr(), compressed.Length() );
	f->Printf( "// %s\n", out.c_str() );
#endif

	if (configexport == eConfigExport_all || configexport == eConfigExport_keybinds)
		idKeyInput::WriteBindings( f );
	if (configexport == eConfigExport_all || configexport == eConfigExport_padbinds)
		idGamepadInput::WriteBindings( f );
	if (configexport == eConfigExport_all || configexport == eConfigExport_cvars)
		cvarSystem->WriteFlaggedVariables( CVAR_ARCHIVE, "seta", f );

	fileSystem->CloseFile( f );
}

/*
===============
idCommonLocal::WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void idCommonLocal::WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !( cvarSystem->GetModifiedFlags() & CVAR_ARCHIVE ) ) {
		return;
	}

	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

	// disable printing out the "Writing to:" message
	bool developer = com_developer.GetBool();
	com_developer.SetBool( false );

	// STiFU #4797: Separate config files for cvars and keybinds
	WriteConfigToFile( CONFIG_FILE,	  "fs_savepath", idCommon::eConfigExport_cvars    );
	WriteConfigToFile( KEYBINDS_FILE, "fs_savepath", idCommon::eConfigExport_keybinds );
	WriteConfigToFile( PADBINDS_FILE, "fs_savepath", idCommon::eConfigExport_padbinds );

	// restore the developer cvar
	com_developer.SetBool( developer );
}

/*
===============
KeysFromBinding()
Returns the key bound to the command
===============
*/
const char* idCommonLocal::KeysFromBinding( const char *bind ) {
	return idKeyInput::KeysFromBinding( bind );
}

/*
===============
BindingFromKey()
Returns the binding bound to key
===============
*/
const char* idCommonLocal::BindingFromKey( const char *key ) {
	return idKeyInput::BindingFromKey( key );
}

/*
===============
ButtonState()
Returns the state of the button
===============
*/
int	idCommonLocal::ButtonState( int key ) {
	return usercmdGen->ButtonState(key);
}

/*
===============
ButtonState()
Returns the state of the key
===============
*/
int	idCommonLocal::KeyState( int key ) {
	return usercmdGen->KeyState(key);
}

//============================================================================

#ifdef ID_ALLOW_TOOLS
/*
==================
Com_Editor_f

  we can start the editor dynamically, but we won't ever get back
==================
*/
static void Com_Editor_f( const idCmdArgs &args ) {
	RadiantInit();
}

/*
=============
Com_EditGUIs_f
=============
*/
static void Com_EditGUIs_f( const idCmdArgs &args ) {
	GUIEditorInit();
}

/*
=============
Com_MaterialEditor_f
=============
*/
static void Com_MaterialEditor_f( const idCmdArgs &args ) {
	// Turn off sounds
	soundSystem->SetMute( true );
	MaterialEditorInit();
}
#endif // ID_ALLOW_TOOLS

/*
============
idCmdSystemLocal::PrintMemInfo_f

This prints out memory debugging data
============
*/
static void PrintMemInfo_f( const idCmdArgs &args ) {
	MemInfo_t mi;

	memset( &mi, 0, sizeof( mi ) );
	mi.filebase = session->GetCurrentMapName();

	renderSystem->PrintMemInfo( &mi );			// textures and models
	soundSystem->PrintMemInfo( &mi );			// sounds

	common->Printf( " Used image memory: %s bytes\n", idStr::FormatNumber( mi.imageAssetsTotal ).c_str() );
	mi.assetTotals += mi.imageAssetsTotal;

	common->Printf( " Used model memory: %s bytes\n", idStr::FormatNumber( mi.modelAssetsTotal ).c_str() );
	mi.assetTotals += mi.modelAssetsTotal;

	common->Printf( " Used sound memory: %s bytes\n", idStr::FormatNumber( mi.soundAssetsTotal ).c_str() );
	mi.assetTotals += mi.soundAssetsTotal;

	common->Printf( " Used asset memory: %s bytes\n", idStr::FormatNumber( mi.assetTotals ).c_str() );

	// write overview file
	idFile *f;

	f = fileSystem->OpenFileAppend( "maps/printmeminfo.txt" );
	if ( !f ) {
		return;
	}

	f->Printf( "total(%s ) image(%s ) model(%s ) sound(%s ): %s\n", idStr::FormatNumber( mi.assetTotals ).c_str(), idStr::FormatNumber( mi.imageAssetsTotal ).c_str(), 
		idStr::FormatNumber( mi.modelAssetsTotal ).c_str(), idStr::FormatNumber( mi.soundAssetsTotal ).c_str(), mi.filebase.c_str() );

	fileSystem->CloseFile( f );
}

#ifdef ID_ALLOW_TOOLS
/*
==================
Com_EditLights_f
==================
*/
static void Com_EditLights_f( const idCmdArgs &args ) {
	LightEditorInit( NULL );
	cvarSystem->SetCVarInteger( "g_editEntityMode", 1 );
}

/*
==================
Com_EditSounds_f
==================
*/
static void Com_EditSounds_f( const idCmdArgs &args ) {
	SoundEditorInit( NULL );
	cvarSystem->SetCVarInteger( "g_editEntityMode", 2 );
}

/*
==================
Com_EditDecls_f
==================
*/
static void Com_EditDecls_f( const idCmdArgs &args ) {
	DeclBrowserInit( NULL );
}

/*
==================
Com_EditAFs_f
==================
*/
static void Com_EditAFs_f( const idCmdArgs &args ) {
	AFEditorInit( NULL );
}

/*
==================
Com_EditParticles_f
==================
*/
static void Com_EditParticles_f( const idCmdArgs &args ) {
	ParticleEditorInit( NULL );
}

/*
==================
Com_EditScripts_f
==================
*/
static void Com_EditScripts_f( const idCmdArgs &args ) {
	ScriptEditorInit( NULL );
}

#endif // ID_ALLOW_TOOLS

/*
==================
Com_Error_f

Just throw a fatal error to test error shutdown procedures.
==================
*/
static void Com_Error_f( const idCmdArgs &args ) {
	/*if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "error may only be used in developer mode\n" );
		return;
	}*/

	if ( args.Argc() > 1 ) {
		commonLocal.FatalError( "Testing fatal error" );
	} else {
		commonLocal.Error( "Testing drop error" );
	}
}

/*
==================
Com_Freeze_f

Just freeze in place for a given number of seconds to test error recovery.
==================
*/
static void Com_Freeze_f( const idCmdArgs &args ) {
	float	s;
	int		start, now;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "freeze <seconds>\n" );
		return;
	}

	/*if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "freeze may only be used in developer mode\n" );
		return;
	}*/

	s = atof( args.Argv(1) );

	start = eventLoop->Milliseconds();

	while ( 1 ) {
		now = eventLoop->Milliseconds();
		if ( ( now - start ) * 0.001f > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
void Com_Crash_f( const idCmdArgs &args ) {
	/*if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "crash may only be used in developer mode\n" );
		return;
	}*/

	* ( int * ) 0 = 0x12345678;
}

/*
=================
Com_Quit_f
=================
*/
static void Com_Quit_f( const idCmdArgs &args ) {
	commonLocal.Quit();
}

/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( const idCmdArgs &args ) {
	idStr	filename;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	filename = args.Argv(1);
	filename.DefaultFileExtension( ".cfg" );
	commonLocal.Printf( "Writing %s.\n", filename.c_str() );
	commonLocal.WriteConfigToFile( filename );
}

/*
=================
Com_ReloadEngine_f
=================
*/
void Com_ReloadEngine_f( const idCmdArgs &args ) {
	bool menu = false;

	if ( !commonLocal.IsInitialized() ) {
		return;
	}

	if ( args.Argc() > 1 && idStr::Icmp( args.Argv( 1 ), "menu" ) == 0 ) {
		menu = true;
	}

	common->Printf( "============= ReloadEngine start =============\n" );
	if ( !menu ) {
		Sys_ShowConsole( 1, false );
	}
	commonLocal.ShutdownGame( true );
	commonLocal.InitGame();
	if ( !menu ) {
		Sys_ShowConsole( 0, false );
	}
	common->Printf( "============= ReloadEngine end ===============\n" );

	if ( !cmdSystem->PostReloadEngine() ) {
		if ( menu ) {
			session->StartMenu( );
		}
	}
}

/*
===============
idCommonLocal::Translate

A shortcut to common->GetI18N()->Translate().
===============
*/
const char *idCommonLocal::Translate( const char* str ) {
	// Redirect the call to I18N
	return i18n->Translate(str);
}

I18N* idCommonLocal::GetI18N() {
	return i18n;
}

/*
===============
idCommonLocal::InitLanguageDict
===============
*/
void idCommonLocal::InitLanguageDict( void ) {

	StartupVariable( "sys_lang", false );	// let it be set on the command line - this is needed because this init happens very early

	i18n->SetLanguage( cvarSystem->GetCVarString( "sys_lang" ), false );	// false => not the first time

	Sys_InitScanTable();
}

/*
===============
idCommonLocal::LocalizeSpecificMapData
===============
*/
void idCommonLocal::LocalizeSpecificMapData( const char *fileName, idLangDict &langDict, const idLangDict &replaceArgs ) {
	idStr out, ws, work;

	idMapFile map;
	if ( map.Parse( fileName, false, false ) ) {
		int count = map.GetNumEntities();
		for ( int i = 0; i < count; i++ ) {
			idMapEntity *ent = map.GetEntity( i );
			if ( ent ) {
				for ( int j = 0; j < replaceArgs.GetNumKeyVals(); j++ ) {
					const idLangKeyValue *kv = replaceArgs.GetKeyVal( j );
					const char *temp = ent->epairs.GetString( kv->key );
					if ( temp && *temp ) {
						idStr val = kv->value;
						if ( val == temp ) {
							ent->epairs.Set( kv->key, langDict.AddString( temp ) );
						}
					}
				}
			}
		}
	map.Write( fileName, ".map" );
	}
}

/*
===============
idCommonLocal::LocalizeMapData
===============
*/
void idCommonLocal::LocalizeMapData( const char *fileName, idLangDict &langDict ) {
	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	common->SetRefreshOnPrint( true );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
        src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), fileName);
		if ( src.IsLoaded() ) {
			common->Printf( "Processing %s\n", fileName );
			idStr mapFileName;
			idToken token, token2;
			idLangDict replaceArgs;
			while ( src.ReadToken( &token ) ) {
				mapFileName = token;
				replaceArgs.Clear();
				src.ExpectTokenString( "{" );
				while ( src.ReadToken( &token) ) {
					if ( token == "}" ) {
						break;
					}
					if ( src.ReadToken( &token2 ) ) {
						if ( token2 == "}" ) {
							break;
						}
						replaceArgs.AddKeyVal( token, token2 );
					}
				}
				common->Printf( "  localizing map %s...\n", mapFileName.c_str() );
				LocalizeSpecificMapData( mapFileName, langDict, replaceArgs );
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}

	common->SetRefreshOnPrint( false );
}

/*
===============
idCommonLocal::LocalizeGui
===============
*/
void idCommonLocal::LocalizeGui( const char *fileName, idLangDict &langDict ) {
	idStr out, ws, work;
	const char *buffer = NULL;
	int k;
	char ch;
	char slash = '\\';
	char tab = 't';
	char nl = 'n';
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
        src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), fileName);
		if ( src.IsLoaded() ) {
			idFile *outFile = fileSystem->OpenFileWrite( fileName ); 
			common->Printf( "Processing %s\n", fileName );
			session->UpdateScreen();
			idToken token;
			while( src.ReadToken( &token ) ) {
				src.GetLastWhiteSpace( ws );
				out += ws;
				if ( token.type == TT_STRING ) {
					out += va( "\"%s\"", token.c_str() );
				} else {
					out += token;
				}
				if ( out.Length() > 200000 ) {
					outFile->Write( out.c_str(), out.Length() );
					out = "";
				}
				work = token.Right( 6 );
				if ( token.Icmp( "text" ) == 0 || work.Icmp( "::text" ) == 0 || token.Icmp( "choices" ) == 0 ) {
					if ( src.ReadToken( &token ) ) {
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						token = langDict.AddString( token );
						out += "\"";
						for ( k = 0; k < token.Length(); k++ ) {
							ch = token[k];
							if ( ch == '\t' ) {
								out += slash;
								out += tab;
							} else if ( ch == '\n' || ch == '\r' ) {
								out += slash;
								out += nl;
							} else {
								out += ch;
							}
						}
						out += "\"";
					}
				} else if ( token.Icmp( "comment" ) == 0 ) {
					if ( src.ReadToken( &token ) ) {
						// need to write these out by hand to preserve any \n's
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						out += "\"";
						for ( k = 0; k < token.Length(); k++ ) {
							ch = token[k];
							if ( ch == '\t' ) {
								out += slash;
								out += tab;
							} else if ( ch == '\n' || ch == '\r' ) {
								out += slash;
								out += nl;
							} else {
								out += ch;
							}
						}
						out += "\"";
					}
				}
			}
			outFile->Write( out.c_str(), out.Length() );
			fileSystem->CloseFile( outFile );
		}
		fileSystem->FreeFile( (void*)buffer );
	}
}

/*
=================
ReloadLanguage_f
=================
*/
void Com_ReloadLanguage_f( const idCmdArgs &args ) {
	commonLocal.InitLanguageDict();
}

typedef idHashTable<idStrList> ListHash;
void LoadMapLocalizeData(ListHash& listHash) {

	idStr fileName = "map_localize.cfg";
	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
        src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), fileName);
		if ( src.IsLoaded() ) {
			idStr classname;
			idToken token;



			while ( src.ReadToken( &token ) ) {
				classname = token;
				src.ExpectTokenString( "{" );

				idStrList list;
				while ( src.ReadToken( &token) ) {
					if ( token == "}" ) {
						break;
					}
					list.Append(token);
				}

				listHash.Set(classname, list);
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}

}

void LoadGuiParmExcludeList(idStrList& list) {

	idStr fileName = "guiparm_exclude.cfg";
	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
        src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), fileName);
		if ( src.IsLoaded() ) {
			idStr classname;
			idToken token;



			while ( src.ReadToken( &token ) ) {
				list.Append(token);
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}
}

bool TestMapVal(idStr& str) {
	//Already Localized?
	if(str.Find("#str_") != -1) {
		return false;
	}

	return true;
}

bool TestGuiParm(const char* parm, const char* value, idStrList& excludeList) {

	idStr testVal = value;

	//Already Localized?
	if(testVal.Find("#str_") != -1) {
		return false;
	}

	//Numeric
	else if(testVal.IsNumeric()) {
		return false;
	}

	//Contains ::
	else if(testVal.Find("::") != -1) {
		return false;
	}

	//Contains /
	else if(testVal.Find("/") != -1) {
		return false;
	}

	else if(excludeList.Find(testVal)) {
		return false;
	}

	return true;
}

void GetFileList(const char* dir, const char* ext, idStrList& list) {

	//Recurse Subdirectories
	idStrList dirList;
	Sys_ListFiles(dir, "/", dirList);
	for ( int i = 0; i < dirList.Num(); i++ ) {
		if(dirList[i] == "." || dirList[i] == "..") {
			continue;
		}
		idStr fullName = va("%s/%s", dir, dirList[i].c_str());
		GetFileList(fullName, ext, list);
	}

	idStrList fileList;
	Sys_ListFiles(dir, ext, fileList);
	for( int i = 0; i < fileList.Num(); i++ ) {
		idStr fullName = va("%s/%s", dir, fileList[i].c_str());
		list.Append(fullName);
	}
}

int LocalizeMap(const char* mapName, idLangDict &langDict, ListHash& listHash, idStrList& excludeList, bool writeFile) {

	common->Printf("Localizing Map '%s'\n", mapName);

	int strCount = 0;
	
	idMapFile map;
	if ( map.Parse(mapName, false, false ) ) {
		int count = map.GetNumEntities();
		for ( int j = 0; j < count; j++ ) {
			idMapEntity *ent = map.GetEntity( j );
			if ( ent ) {

				idStr classname = ent->epairs.GetString("classname");

				//Hack: for info_location
				bool hasLocation = false;

				idStrList* list;
				listHash.Get(classname, &list);
				if ( list ) {

					for( int k = 0; k < list->Num(); k++ ) {

						idStr val = ent->epairs.GetString((*list)[k], "");
						
						if ( val.Length() && classname == "info_location" && (*list)[k] == "location" ) {
							hasLocation = true;
						}

						if ( val.Length() && TestMapVal(val) && ( !hasLocation || (*list)[k] == "location" ) ) {
							//Localize it!!!
							strCount++;
							ent->epairs.Set( (*list)[k], langDict.AddString( val ) );
						}
					}
				}

				listHash.Get("all", &list);
				if ( list ) {
					for ( int k = 0; k < list->Num(); k++ ) {
						idStr val = ent->epairs.GetString((*list)[k], "");
						if ( val.Length() && TestMapVal(val) ) {
							//Localize it!!!
							strCount++;
							ent->epairs.Set( (*list)[k], langDict.AddString( val ) );
						}
					}
				}

				//Localize the gui_parms
				const idKeyValue* kv = ent->epairs.MatchPrefix("gui_parm");
				while ( kv ) {
					if ( TestGuiParm(kv->GetKey(), kv->GetValue(), excludeList) ) {
						//Localize It!
						strCount++;
						ent->epairs.Set( kv->GetKey(), langDict.AddString( kv->GetValue() ) );
					}
					kv = ent->epairs.MatchPrefix( "gui_parm", kv );
				}
			}
		}
		if(writeFile && strCount > 0)  {
			//Before we write the map file lets make a backup of the original
			idStr file = fileSystem->RelativePathToOSPath(mapName);
			idStr bak = file.Left(file.Length() - 4);
			bak.Append(".bak_loc");
			fileSystem->CopyFile( file, bak );
			
			map.Write( mapName, ".map" );
		}
	}

	common->Printf("Count: %d\n", strCount);
	return strCount;
}

/*
=================
LocalizeMaps_f
=================
*/
void Com_LocalizeMaps_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "Usage: localizeMaps <count | dictupdate | all> <map>\n" );
		return;
	}

	int strCount = 0;
	bool dictUpdate = false;
	bool write = false;

	if ( idStr::Icmp( args.Argv(1), "count" ) == 0 ) {
		// Serp - this has been left here to preserve expectations - count is/was always implied
	} else if ( idStr::Icmp( args.Argv(1), "dictupdate" ) == 0 ) {
		dictUpdate = true;
	} else if ( idStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		dictUpdate = true;
		write = true;
	} else {
		common->Printf( "Invalid Command\n" );
		common->Printf( "Usage: localizeMaps <count | dictupdate | all>\n" );
		return;
	}

	idLangDict strTable;
	idStr filename = va("strings/english%.3i.lang", com_product_lang_ext.GetInteger());
	if ( strTable.Load( filename ) == false ) {
		//This is a new file so set the base index
		strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
	}

	common->SetRefreshOnPrint( true );
	
	ListHash listHash;
	LoadMapLocalizeData(listHash);

	idStrList excludeList;
	LoadGuiParmExcludeList(excludeList);

	if ( args.Argc() == 3 ) {
		strCount += LocalizeMap(args.Argv(2), strTable, listHash, excludeList, write);
	} else {
		idStrList files;
		GetFileList("z:/d3xp/d3xp/maps/game", "*.map", files);
		for ( int i = 0; i < files.Num(); i++ ) {
			idStr file =  fileSystem->OSPathToRelativePath(files[i]);
			strCount += LocalizeMap(file, strTable, listHash, excludeList, write);		
		}
	}

	common->Printf("Localize String Count: %d\n", strCount);
	common->SetRefreshOnPrint( false );

	if ( dictUpdate ) {
		strTable.Save( filename );
	}
}

/*
=================
LocalizeGuis_f
=================
*/
void Com_LocalizeGuis_f( const idCmdArgs &args ) {

	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: localizeGuis <all | gui>\n" );
		return;
	}

	idLangDict strTable;

	idStr filename = va("strings/english%.3i.lang", com_product_lang_ext.GetInteger());
	if( strTable.Load( filename ) == false ) {
		//This is a new file so set the base index
		strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
	}

	idFileList *files;
	if ( idStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		// taaaki: tels, should this reference darkmod or the specific mission?
        idStr curgame = cvarSystem->GetCVarString( "fs_mod" ); 
		if(curgame.Length()) {
			files = fileSystem->ListFilesTree( "guis", "*.gui", true, curgame );
		} else {
			files = fileSystem->ListFilesTree( "guis", "*.gui", true );
		}

		for ( int i = 0; i < files->GetNumFiles(); i++ ) {
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

		if(curgame.Length()) {
			files = fileSystem->ListFilesTree( "guis", "*.pd", true, curgame );
		}

		for ( int i = 0; i < files->GetNumFiles(); i++ ) {
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

	} else {
		commonLocal.LocalizeGui( args.Argv(1), strTable );
	}

	strTable.Save( filename );
}

void Com_LocalizeGuiParmsTest_f( const idCmdArgs &args ) {

	common->SetRefreshOnPrint( true );

	idFile *localizeFile = fileSystem->OpenFileWrite( "gui_parm_localize.csv" ); 
	idFile *noLocalizeFile = fileSystem->OpenFileWrite( "gui_parm_nolocalize.csv" ); 

	idStrList excludeList;
	LoadGuiParmExcludeList(excludeList);

	idStrList files;
	GetFileList("z:/d3xp/d3xp/maps/game", "*.map", files);

	for ( int i = 0; i < files.Num(); i++ ) {
		
		common->Printf("Testing Map '%s'\n", files[i].c_str());
		idMapFile map;

		idStr file =  fileSystem->OSPathToRelativePath(files[i]);
		if ( map.Parse(file, false, false ) ) {
			int count = map.GetNumEntities();
			for ( int j = 0; j < count; j++ ) {
				idMapEntity *ent = map.GetEntity( j );
				if ( ent ) {
					const idKeyValue* kv = ent->epairs.MatchPrefix("gui_parm");
					while( kv ) {
						if(TestGuiParm(kv->GetKey(), kv->GetValue(), excludeList)) {
							idStr out = va("%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str());
							localizeFile->Write( out.c_str(), out.Length() );
						} else {
							idStr out = va("%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str());
							noLocalizeFile->Write( out.c_str(), out.Length() );
						}
						kv = ent->epairs.MatchPrefix( "gui_parm", kv );
					}
				}
			}
		}
	}
	
	fileSystem->CloseFile( localizeFile );
	fileSystem->CloseFile( noLocalizeFile );

	common->SetRefreshOnPrint( false );
}


void Com_LocalizeMapsTest_f( const idCmdArgs &args ) {

	ListHash listHash;
	LoadMapLocalizeData(listHash);


	common->SetRefreshOnPrint( true );

	idFile *localizeFile = fileSystem->OpenFileWrite( "map_localize.csv" ); 
	
	idStrList files;
	GetFileList("z:/d3xp/d3xp/maps/game", "*.map", files);

	for ( int i = 0; i < files.Num(); i++ ) {

		common->Printf("Testing Map '%s'\n", files[i].c_str());
		idMapFile map;

		idStr file =  fileSystem->OSPathToRelativePath(files[i]);
		if ( map.Parse(file, false, false ) ) {
			int count = map.GetNumEntities();
			for ( int j = 0; j < count; j++ ) {
				idMapEntity *ent = map.GetEntity( j );
				if ( ent ) {
					
					//Temp code to get a list of all entity key value pairs
					/*idStr classname = ent->epairs.GetString("classname");
					if(classname == "worldspawn" || classname == "func_static" || classname == "light" || classname == "speaker" || classname.Left(8) == "trigger_") {
						continue;
					}
					for( int i = 0; i < ent->epairs.GetNumKeyVals(); i++) {
						const idKeyValue* kv = ent->epairs.GetKeyVal(i);
						idStr out = va("%s,%s,%s,%s\r\n", classname.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str(), file.c_str());
						localizeFile->Write( out.c_str(), out.Length() );
					}*/

					idStr classname = ent->epairs.GetString("classname");
					
					//Hack: for info_location
					bool hasLocation = false;

					idStrList* list;
					listHash.Get(classname, &list);
					if ( list ) {
						for ( int k = 0; k < list->Num(); k++ ) {

							idStr val = ent->epairs.GetString((*list)[k], "");
							
							if ( classname == "info_location" && (*list)[k] == "location" ) {
								hasLocation = true;
							}

							if (val.Length() && TestMapVal(val) && (!hasLocation || (*list)[k] == "location") ) {
								idStr out = va("%s,%s,%s\r\n", val.c_str(), (*list)[k].c_str(), file.c_str());
								localizeFile->Write( out.c_str(), out.Length() );
							}
						}
					}

					listHash.Get("all", &list);
					if ( list ) {
						for ( int k = 0; k < list->Num(); k++ ) {
							idStr val = ent->epairs.GetString((*list)[k], "");
							if ( val.Length() && TestMapVal(val) ) {
								idStr out = va("%s,%s,%s\r\n", val.c_str(), (*list)[k].c_str(), file.c_str());
								localizeFile->Write( out.c_str(), out.Length() );
							}
						}
					}
				}
			}
		}
	}

	fileSystem->CloseFile( localizeFile );

	common->SetRefreshOnPrint( false );
}

/*
=================
Com_StartBuild_f
=================
*/
void Com_StartBuild_f( const idCmdArgs &args ) {
	globalImages->StartBuild();
}

/*
=================
Com_FinishBuild_f
=================
*/
void Com_FinishBuild_f( const idCmdArgs &args ) {
	if ( game ) {
		game->CacheDictionaryMedia( NULL );
	}
	globalImages->FinishBuild( ( args.Argc() > 1 ) );
}

/*
=================
idCommonLocal::InitCommands
=================
*/
void idCommonLocal::InitCommands( void ) {
	cmdSystem->AddCommand( "error", Com_Error_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "causes an error" );
	cmdSystem->AddCommand( "crash", Com_Crash_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "causes a crash" );
	cmdSystem->AddCommand( "freeze", Com_Freeze_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "freezes the game for a number of seconds" );
	cmdSystem->AddCommand( "quit", Com_Quit_f, CMD_FL_SYSTEM, "quits the game" );
	cmdSystem->AddCommand( "exit", Com_Quit_f, CMD_FL_SYSTEM, "exits the game" );
	cmdSystem->AddCommand( "writeConfig", Com_WriteConfig_f, CMD_FL_SYSTEM, "writes a config file" );
	cmdSystem->AddCommand( "reloadEngine", Com_ReloadEngine_f, CMD_FL_SYSTEM, "reloads the engine down to including the file system" );

	// compilers
	cmdSystem->AddCommand( "dmap", Dmap_f, CMD_FL_TOOL, "compiles a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "renderbump", RenderBump_f, CMD_FL_TOOL, "renders a bump map", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "renderbumpFlat", RenderBumpFlat_f, CMD_FL_TOOL, "renders a flat bump map", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "runAAS", RunAAS_f, CMD_FL_TOOL, "compiles an AAS file for a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAASDir", RunAASDir_f, CMD_FL_TOOL, "compiles AAS files for all maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runReach", RunReach_f, CMD_FL_TOOL, "calculates reachability for an AAS file", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "roq", RoQFileEncode_f, CMD_FL_TOOL, "encodes a roq file" );
	cmdSystem->AddCommand( "runParticle", RunParticle_f, CMD_FL_TOOL, "calculates static collision for particle systems ('collisionStatic' in .prt files)", idCmdSystem::ArgCompletion_MapName );

#ifdef ID_ALLOW_TOOLS
	// editors
	cmdSystem->AddCommand( "editor", Com_Editor_f, CMD_FL_TOOL, "launches the level editor Radiant" );
	cmdSystem->AddCommand( "editLights", Com_EditLights_f, CMD_FL_TOOL, "launches the in-game Light Editor" );
	cmdSystem->AddCommand( "editSounds", Com_EditSounds_f, CMD_FL_TOOL, "launches the in-game Sound Editor" );
	cmdSystem->AddCommand( "editDecls", Com_EditDecls_f, CMD_FL_TOOL, "launches the in-game Declaration Editor" );
	cmdSystem->AddCommand( "editAFs", Com_EditAFs_f, CMD_FL_TOOL, "launches the in-game Articulated Figure Editor" );
	cmdSystem->AddCommand( "editParticles", Com_EditParticles_f, CMD_FL_TOOL, "launches the in-game Particle Editor" );
	cmdSystem->AddCommand( "editScripts", Com_EditScripts_f, CMD_FL_TOOL, "launches the in-game Script Editor" );
	cmdSystem->AddCommand( "editGUIs", Com_EditGUIs_f, CMD_FL_TOOL, "launches the GUI Editor" );

	//BSM Nerve: Add support for the material editor
	cmdSystem->AddCommand( "materialEditor", Com_MaterialEditor_f, CMD_FL_TOOL, "launches the Material Editor" );
#endif

	cmdSystem->AddCommand( "printMemInfo", PrintMemInfo_f, CMD_FL_SYSTEM, "prints memory debugging data" );

	// idLib commands
	cmdSystem->AddCommand( "memoryDump", Mem_Dump_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "creates a memory dump" );
	cmdSystem->AddCommand( "memoryDumpCompressed", Mem_DumpCompressed_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "creates a compressed memory dump" );
	cmdSystem->AddCommand( "showStringMemory", idStr::ShowMemoryUsage_f, CMD_FL_SYSTEM, "shows memory used by strings" );
	cmdSystem->AddCommand( "showDictMemory", idDict::ShowMemoryUsage_f, CMD_FL_SYSTEM, "shows memory used by dictionaries" );
	cmdSystem->AddCommand( "listDictKeys", idDict::ListKeys_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "lists all keys used by dictionaries" );
	cmdSystem->AddCommand( "listDictValues", idDict::ListValues_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "lists all values used by dictionaries" );
	cmdSystem->AddCommand( "showLoadStackMemory", LoadStack::ShowMemoryUsage_f, CMD_FL_SYSTEM, "shows memory used by load stack strings (see decl_stack)" );
	cmdSystem->AddCommand( "listLoadStackStrings", LoadStack::ListStrings_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "lists all strings stored in load stacks (see decl_stack)" );
	cmdSystem->AddCommand( "testSIMD", idSIMD::Test_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "test SIMD code" );

	// localization
	cmdSystem->AddCommand( "localizeGuis", Com_LocalizeGuis_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "localize guis" );
	cmdSystem->AddCommand( "localizeMaps", Com_LocalizeMaps_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "localize maps" );
	cmdSystem->AddCommand( "reloadLanguage", Com_ReloadLanguage_f, CMD_FL_SYSTEM, "reload language dict" );

	//D3XP Localization
	cmdSystem->AddCommand( "localizeGuiParmsTest", Com_LocalizeGuiParmsTest_f, CMD_FL_SYSTEM, "Create test files that show gui parms localized and ignored." );
	cmdSystem->AddCommand( "localizeMapsTest", Com_LocalizeMapsTest_f, CMD_FL_SYSTEM, "Create test files that shows which strings will be localized." );

	// build helpers
	cmdSystem->AddCommand( "startBuild", Com_StartBuild_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "prepares to make a build" );
	cmdSystem->AddCommand( "finishBuild", Com_FinishBuild_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "finishes the build process" );
}

/*
=================
idCommonLocal::InitRenderSystem
=================
*/
void idCommonLocal::InitRenderSystem( void ) {
	if ( com_skipRenderer.GetBool() ) {
		return;
	} else {
#ifdef __ANDROID__ //karin: force setup resolution on Android
        Sys_ForceResolution();
#endif
		renderSystem->InitOpenGL();
	}
}

/*
=================
idCommonLocal::PrintLoadingMessage
=================
*/
/*void idCommonLocal::PrintLoadingMessage( const char *msg ) {
	if ( !( msg && *msg ) ) {
		return;
	}
    int len = static_cast<int>(strlen(msg));
	renderSystem->BeginFrame( renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight() );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, declManager->FindMaterial( "splashScreen" ) );
	renderSystem->DrawSmallStringExt( ( 640 - len * SMALLCHAR_WIDTH ) / 2, 410, msg, idVec4( 0.0f, 0.81f, 0.94f, 1.0f ), true, declManager->FindMaterial( "textures/bigchars" ) );
	renderSystem->EndFrame( NULL, NULL );
} duzenko: useless because we have no splash window */

/*
=================
idCommonLocal::InitSIMD
=================
*/
void idCommonLocal::InitSIMD( void ) {
	const char *cvarStr = com_forceGenericSIMD.GetString();
	if (idStr::Cmp(cvarStr, "1") == 0)
		cvarStr = "Generic";
	if (idStr::Cmp(cvarStr, "0") == 0)
		cvarStr = nullptr;
	idSIMD::InitProcessor( "TDM", cvarStr );
	com_forceGenericSIMD.ClearModified();
}

/*
=================
idCommonLocal::Frame
=================
*/
void idCommonLocal::Frame( void ) {
	try {

		// pump all the events
		Sys_GenerateEvents();

		// write config file if anything changed
		WriteConfiguration(); 

		// stgatilov #4550: update FPU props (e.g. NaN exceptions)
		sys->ThreadHeartbeat( "Main Thread" );

		// change SIMD implementation if required
		if ( com_forceGenericSIMD.IsModified() ) {
			InitSIMD();
		}

		eventLoop->RunEventLoop();

		static int64_t com_frameTimeMicro = 0;		//same as com_frameTime, but in microseconds
		static int64_t lastFrameAstroTime = Sys_Microseconds();
		if (sessLocal.com_fixedTic.GetBool()) {
			//stgatilov #4865: impose artificial FPS limit
			int64_t minDeltaTime = 1000000 / sessLocal.com_maxFPS.GetInteger();
			int64_t currFrameAstroTime;
			while (1) {
				currFrameAstroTime = Sys_Microseconds();
				if (currFrameAstroTime - lastFrameAstroTime > minDeltaTime)
					break;
				//note: this is busy-wait loop
				#ifdef __SSE2__
					_mm_pause();
				#else
					currFrameAstroTime = currFrameAstroTime;	//NOP
				#endif
			}
			//see how much passed in microseconds
			int deltaTime = currFrameAstroTime - lastFrameAstroTime;
			lastFrameAstroTime = currFrameAstroTime;

			//update precise time in microseconds, then round it to milliseconds
			com_frameTimeMicro += deltaTime * com_timescale.GetFloat();
			int newFrameTime = com_frameTimeMicro / 1000;
			com_frameDelta = newFrameTime - com_frameTime;
			com_frameTime = newFrameTime;
		}
		else {
			//synchronize common time to async tic number
			//(which is synced to astronomical time in idCommonLocal::SingleAsyncTic)
			com_frameTime = com_ticNumber * USERCMD_MSEC;	//com_frameTime += USERCMD_MSEC;
			com_frameDelta = USERCMD_MSEC;
			//these variables are not used now, but they will be needed if we switch to uncapped FPS mode
			com_frameTimeMicro = com_frameTime * 1000;
			lastFrameAstroTime = Sys_Microseconds();
		}

		{
			session->Frame();

			// normal, in-sequence screen update
			session->UpdateScreen( false );
		}

		// report timing information
		if ( com_speeds.GetBool() ) {
			Printf("frame:%i all:%3i gfr:%3i fr:%3i(%d) br:%3i(%d)\n", com_frameNumber, com_frameDelta, time_gameFrame, time_frontend, time_frontendLast, time_backend, time_backendLast);
			time_gameFrame = 0;
			time_gameDraw = 0;
		}	

		com_frameNumber++;

		// set idLib frame number for frame based memory dumps
		idLib::frameNumber = com_frameNumber;
	}

	catch( idException & ) {
		return;			// an ERP_DROP was thrown
	}
}

/*
=================
idCommonLocal::GUIFrame
=================
*/
void idCommonLocal::GUIFrame( bool execCmd, bool network ) {
	Sys_GenerateEvents();
	eventLoop->RunEventLoop( execCmd );	// and execute any commands
	com_frameTime = com_ticNumber * USERCMD_MSEC;

	session->Frame();
	session->UpdateScreen( false );	
}

/*
=================
idCommonLocal::SingleAsyncTic

The system will asyncronously call this function 60 times a second to
handle the time-critical functions that we don't want limited to
the frame rate:

sound mixing
user input generation (conditioned by com_asyncInput)
packet server operation
packet client operation

We are not using thread safe libraries, so any functionality put here must
be VERY VERY careful about what it calls.
=================
*/

typedef struct {
	int				milliseconds;			// should always be incremeting by 60hz
	int				deltaMsec;				// should always be 16
	int				timeConsumed;			// msec spent in Com_AsyncThread()
	int				clientPacketsReceived;
	int				serverPacketsReceived;
	int				mostRecentServerPacketSequence;
} asyncStats_t;

#define MAX_ASYNC_STATS			1024

asyncStats_t	com_asyncStats[MAX_ASYNC_STATS];		// indexed by com_ticNumber
int64_t lastTicUsec;

void idCommonLocal::SingleAsyncTic( void ) {
	// main thread code can prevent this from happening while modifying
	// critical data structures
	Sys_EnterCriticalSection();

	asyncStats_t *stat = &com_asyncStats[com_ticNumber & (MAX_ASYNC_STATS-1)];
	memset( stat, 0, sizeof( *stat ) );
	stat->milliseconds = Sys_Milliseconds();
	stat->deltaMsec = stat->milliseconds - com_asyncStats[(com_ticNumber - 1) & (MAX_ASYNC_STATS-1)].milliseconds;

	if ( usercmdGen && com_asyncInput.GetBool() ) {
		usercmdGen->UsercmdInterrupt();
	}

	switch ( com_asyncSound.GetInteger() ) {
		case 1:
			soundSystem->AsyncUpdate( stat->milliseconds );
			break;
		case 3:
			soundSystem->AsyncUpdateWrite( stat->milliseconds );
			break;
	}

	// we update com_ticNumber after all the background tasks
	// have completed their work for this tic
	com_ticNumber++;

	stat->timeConsumed = Sys_Milliseconds() - stat->milliseconds;

	Sys_LeaveCriticalSection();
}

/*
=================
idCommonLocal::Async
=================
*/
void idCommonLocal::Async( void ) {
	
	if ( com_shuttingDown ) {
		return;
	}

	// stgatilov #4550: update FPU props (e.g. NaN exceptions)
	sys->ThreadHeartbeat( "Async" );

	if ( !com_preciseTic.GetBool() ) {
		// just run a single tic, even if the exact msec isn't precise
		SingleAsyncTic();
		return;
	}

	//stgatilov #4514: game tics happen even X microseconds (in cumulative sense)
	//to see how often this function is called, see Sys_StartAsyncThread
	static const int USERCMD_USEC = 16650;		// ~60.06 Hz --- a bit higher than vsync

	const int64_t usec = Sys_GetTimeMicroseconds();
	if ( !lastTicUsec ) {
		lastTicUsec = usec - USERCMD_USEC;
	}

	int64_t ticUsec = USERCMD_USEC;
	const float timescale = com_timescale.GetFloat();

	// don't skip too many
	if ( timescale == 1.0f ) {
		if ( lastTicUsec + (10 * USERCMD_USEC) < usec ) {
			lastTicUsec = usec - (10 * USERCMD_USEC);
		}
	}
	// the number of msec per tic can be varies with the timescale cvar
	else {								// i.e if ( timescale != 1.0f )
		ticUsec /= timescale;
		if ( ticUsec < 1 ) {
			ticUsec = 1;
		}
	}

	while ( lastTicUsec + ticUsec <= usec ) {
		SingleAsyncTic();
		lastTicUsec += ticUsec;
	}
	Sys_TriggerEvent( TRIGGER_EVENT_ONE );
}

/*
=================
idCommonLocal::LoadGameDLL
=================
*/
void idCommonLocal::LoadGameDLL( void ) {
#ifdef __DOOM_DLL__
	char			dllPath[ MAX_OSPATH ];
	gameImport_t	gameImport;
	gameExport_t	gameExport;
	GetGameAPI_t	GetGameAPI;

	memset(dllPath, 0, sizeof(dllPath));

	fileSystem->FindDLL( "game", dllPath, true );

	if ( !dllPath[0] ) {
		common->FatalError( "couldn't find game dynamic library" );
		return;
	}

	common->DPrintf( "Loading game DLL: '%s'\n", dllPath );
	gameDLL = sys->DLL_Load( dllPath );
	if ( !gameDLL ) {
		common->FatalError( "couldn't load game dynamic library" );
		return;
	}

	GetGameAPI = (GetGameAPI_t) Sys_DLL_GetProcAddress( gameDLL, "GetGameAPI" );
	if ( !GetGameAPI ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = NULL;
		common->FatalError( "couldn't find game DLL API" );
		return;
	}

	gameImport.version					= GAME_API_VERSION;
	gameImport.sys						= ::sys;
	gameImport.common					= ::common;
	gameImport.cmdSystem				= ::cmdSystem;
	gameImport.cvarSystem				= ::cvarSystem;
	gameImport.fileSystem				= ::fileSystem;
	gameImport.networkSystem			= ::networkSystem;
	gameImport.renderSystem				= ::renderSystem;
	gameImport.soundSystem				= ::soundSystem;
	gameImport.renderModelManager		= ::renderModelManager;
	gameImport.uiManager				= ::uiManager;
	gameImport.declManager				= ::declManager;
	gameImport.AASFileManager			= ::AASFileManager;
	gameImport.collisionModelManager	= ::collisionModelManager;

	gameExport							= *GetGameAPI( &gameImport );

	// Tels: The game DLL does its own check, and exits, so this check is probably never run.
	if ( gameExport.version != GAME_API_VERSION ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = NULL;
		common->FatalError( "wrong game DLL API version" );
		return;
	}

	game								= gameExport.game;
	gameEdit							= gameExport.gameEdit;

#endif

	// initialize the game object
	if ( game != NULL ) {
		game->Init();
	}
}

/*
=================
idCommonLocal::UnloadGameDLL
=================
*/
void idCommonLocal::UnloadGameDLL( void ) {

	// shut down the game object
	if ( game != NULL ) {
		game->Shutdown();
	}

#ifdef __DOOM_DLL__

	if ( gameDLL ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = NULL;
	}
	game = NULL;
	gameEdit = NULL;

#endif
}

/*
=================
idCommonLocal::IsInitialized
=================
*/
bool idCommonLocal::IsInitialized( void ) const {
	return com_fullyInitialized;
}

/*
=================
idCommonLocal::Init
=================
*/
void idCommonLocal::Init( int argc, const char **argv, const char *cmdline )
{
	try
	{
		// set interface pointers used by idLib
		idLib::sys			= sys;
		idLib::common		= common;
		idLib::cvarSystem	= cvarSystem;
		idLib::fileSystem	= fileSystem;

		// initialize idLib
		idLib::Init();

		// clear warning buffer
		ClearWarnings( GAME_NAME " initialization" );
		
		// parse command line options
		idCmdArgs args;
		if ( cmdline ) {
			// tokenize if the OS doesn't do it for us
			args.TokenizeString( cmdline, true );
			argv = args.GetArgs( &argc );
		}
		ParseCommandLine( argc, argv );

		// init console command system
		cmdSystem->Init();
		TestsInit();

		// init CVar system
		cvarSystem->Init();

		// potentially start trace profiler if requested
		InitTracing();

		// start file logging right away, before early console or whatever
		StartupVariable( "win_outputDebugString", false );

		// register all static CVars
		idCVar::RegisterStaticVars();

		// Store the engine version into the CVAR
		com_version.SetString(engineVersion.Get());

		// print engine version
		Printf( "%s\n", com_version.GetString() );

		// initialize key input/binding, done early so bind command exists
		idKeyInput::Init();
		idGamepadInput::Init();

		// init the console so we can take prints
		console->Init();

		// get architecture info
		Sys_Init();

		// initialize networking
		Sys_InitNetworking();

		// override cvars from command line
		StartupVariable( NULL, false );

		// stgatilov #4550: set FPU props (FTZ + DAZ, etc.)
		sys->ThreadStartup();

		// initialize processor specific SIMD implementation
		InitSIMD();

		// init commands
		InitCommands();

#ifdef ID_WRITE_VERSION
		config_compressor = idCompressor::AllocArithmetic();
#endif

		// game specific initialization
		InitGame();

		// add startup commands eg: game.exe +r_fullscreen 0
		if ( !AddStartupCommands() ) {
			// if the user didn't give any commands, run default action
			session->StartMenu( true );
		}

		Printf( "--- Common Initialization Complete ---\n" );

		// print all warnings queued during initialization
		PrintWarnings();

		// remove any prints from the notify lines
		console->ClearNotifyLines();
		
		// greebo: Keep the console lines around, we need it when reloading the engine
		//ClearCommandLine();

        // load the persistent console history
        console->LoadHistory();

		com_fullyInitialized = true;
	}

	catch( idException &e ) {
		Sys_Error( "Error during initialization. %s", e.error );
	}
}


/*
=================
idCommonLocal::Shutdown
=================
*/
void idCommonLocal::Shutdown( void ) {

	com_shuttingDown = true;

    // save persistent console history
    console->SaveHistory();

	// game specific shut down
	ShutdownGame(false);

	// shut down non-portable system services
	Sys_Shutdown();

	// shut down the console
	console->Shutdown();

	// shut down the key system
	idKeyInput::Shutdown();
	idGamepadInput::Shutdown();

	// shut down the cvar system
	cvarSystem->Shutdown();

	// shut down the console command system
	cmdSystem->Shutdown();

#ifdef ID_WRITE_VERSION
	delete config_compressor;
	config_compressor = NULL;
#endif

	// free any buffered warning messages
	ClearWarnings( GAME_NAME " shutdown" );
	warningCaption.ClearFree();
	errorList.ClearFree();

	// enable leak test
	Mem_EnableLeakTest( "tdm_main" );

	// shutdown idLib
	idLib::ShutDown();
}

/*
=================
idCommonLocal::InitGame
=================
*/
void idCommonLocal::InitGame( void )
{
	idStrList newModsList;

	// greebo: Check if we have fs_currentfm and/or fs_mod defined, if not fall back to default values
	// Do this before initialising the filesystem
	InitGameArguments(&newModsList);

	// initialize the file system
	fileSystem->Init();

	// init journalling, etc
	eventLoop->Init();

	// init the parallel job manager
	parallelJobManager->Init();

	// initialize the declaration manager
	declManager->Init();

	// force r_fullscreen 0 if running a tool
	CheckToolMode();

	// initialize the renderSystem data structures, but don't start OpenGL yet
	renderSystem->Init();

	// Init the i18n manager
	i18n->Init();

	//PrintLoadingMessage( Translate( "#str_04344" ) );

	// load the font, etc
	console->LoadGraphics();

	//PrintLoadingMessage( Translate( "#str_04345" ) );

	// Reset cvars. Needed when switching between missions.
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "cvar_restart\n" );

	// exec the startup scripts
#ifdef NO_MFC
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec editor.cfg\n" );
#endif
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );

	// skip the config file if "safe" is on the command line
	if (!SafeMode())
	{
		// if any config file does not exist, ignore it and only use "default.cfg".
		// Darkmod.cfg and DarkmodKeybinds.cfg will be written upon exit
		if (fileSystem->FindFile(CONFIG_FILE) != FIND_NO) {
			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "exec " CONFIG_FILE "\n");		// Darkmod.cfg
			
		}
		if (fileSystem->FindFile(KEYBINDS_FILE) != FIND_NO) { // STiFU #4797
			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "exec " KEYBINDS_FILE "\n");		// DarkmodKeybinds.cfg
		}
		if (fileSystem->FindFile(PADBINDS_FILE) != FIND_NO) {
			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "exec " PADBINDS_FILE "\n");
		}
	}

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec mission.cfg\n" );

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec autoexec.cfg\n" );

	// reload the language dictionary now that we've loaded config files
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reloadLanguage\n" );

	// run cfg execution
	cmdSystem->ExecuteCommandBuffer();

	// re-override anything from the config files with command line args
	StartupVariable( NULL, false );

	// if any archived cvars are modified after this, we will trigger a writing of the config file
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

	// cvars are initialized, but not the rendering system. Allow preference startup dialog
	Sys_DoPreferences();

	// init the user command input code
	usercmdGen->Init();

	//PrintLoadingMessage( Translate( "#str_04346" ) );

	// start the sound system, but don't do any hardware operations yet
	soundSystem->Init();

	//PrintLoadingMessage( Translate( "#str_04347" ) );

	// init OpenGL, which will open a window and connect sound and input hardware
	//PrintLoadingMessage( Translate( "#str_04348" ) );
	InitRenderSystem();

	// initialize the user interfaces
	uiManager->Init();

	// startup the script debugger
	// DebuggerServerInit();

	// load the game dll
	LoadGameDLL();
	// stgatilov #5661: notify MissionManager about already accepted new FMs
	gameLocal.m_MissionManager->AddToNewModList(newModsList);
	
	// init the session
	session->Init();

	// tels: #3199: now that the game DLL is loaded, we can execute another config, this
	// enables it to run f.i. dmap (dmap before DLL load produces no AAS):
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec autocommands.cfg\n" );
}

/*
=================
idCommonLocal::ShutdownGame
=================
*/
void idCommonLocal::ShutdownGame( bool reloading ) {

	// kill sound first
	idSoundWorld *sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->StopAllSounds();
	}

	// shutdown the script debugger
	// DebuggerServerShutdown();

	// shut down the session
	session->Shutdown();

	// shut down the user interfaces
	uiManager->Shutdown();

	// shut down the sound system
	soundSystem->Shutdown();

	// shut down the user command input code
	usercmdGen->Shutdown();

	// shut down the event loop
	eventLoop->Shutdown();


	// shut down the renderSystem
	renderBackend->Shutdown();
	renderSystem->Shutdown();

	// destroy the i18n manager
	i18n->Shutdown();

	// shutdown the decl manager
	declManager->Shutdown();

	// unload the game dll
	UnloadGameDLL();

	// dump warnings to "warnings.txt"
#ifdef DEBUG
	DumpWarnings();
#endif
	// only shut down the log file after all output is done
	CloseLogFile();

	// shut down the file system
	fileSystem->Shutdown( reloading );
}

// Agent Jones #3766
bool idCommonLocal::WindowAvailable(void)
{
#ifdef _WIN32
	return Sys_IsWindowVisible();
#endif
	return true;
}
