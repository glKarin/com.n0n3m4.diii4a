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

#include "../idlib/precompiled.h"
#include "../sys/sys_local.h"
#pragma hdrstop

#include "TypeInfoGen.h"

idSession *			session = NULL;
idDeclManager *		declManager = NULL;
idEventLoop *		eventLoop = NULL;

int idEventLoop::JournalLevel( void ) const { return 0; }

/*
==============================================================

	idCommon

==============================================================
*/

#define STDIO_PRINT( pre, post )	\
	va_list argptr;					\
	va_start( argptr, fmt );		\
	printf( pre );					\
	vprintf( fmt, argptr );			\
	printf( post );					\
	va_end( argptr )


class idCommonLocal : public idCommon {
public:
							idCommonLocal( void ) {}

	virtual void			Init( int argc, const char **argv, const char *cmdline ) {}
	virtual void			Shutdown( void ) {}
	virtual void			Quit( void ) {}
	virtual bool			IsInitialized( void ) const { return true; }
	virtual void			Frame( void ) {}
	virtual void			GUIFrame( bool execCmd, bool network  ) {}
	virtual void			Async( void ) {}
	virtual void			StartupVariable( const char *match, bool once ) {}
	virtual void			InitTool( const toolFlag_t tool, const idDict *dict ) {}
	virtual void			ActivateTool( bool active ) {}
	virtual void			WriteConfigToFile( const char *filename, const char* basePath, const eConfigExport configexport = eConfigExport_all ) {}
	virtual void			WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) {}
	virtual void			BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) {}
	virtual void			EndRedirect( void ) {}
	virtual void			SetRefreshOnPrint( bool set ) {}
	virtual void			Printf( const char *fmt, ... ) { STDIO_PRINT( "", "" ); }
	virtual void			VPrintf( const char *fmt, va_list arg ) { vprintf( fmt, arg ); }
	virtual void			DPrintf( const char *fmt, ... ) { /*STDIO_PRINT( "", "" );*/ }
	virtual void			PrintCallStack() {}
	virtual void			Warning( const char *fmt, ... ) { STDIO_PRINT( "WARNING: ", "\n" ); }
	virtual void			DWarning( const char *fmt, ...) { /*STDIO_PRINT( "WARNING: ", "\n" );*/ }
	virtual void			PrintWarnings( void ) {}
	virtual void			ClearWarnings( const char *reason ) {}
	virtual void			PacifierUpdate( loadkey_t key, int count ) {} // grayman #3763
	virtual void			Error( const char *fmt, ... ) { STDIO_PRINT( "ERROR: ", "\n" ); exit(0); }
	virtual void			DoError( const char *msg, int code ) { printf( "ERROR: %s\n", msg ); exit( 0 ); }
	virtual void			FatalError( const char *fmt, ... ) { STDIO_PRINT( "FATAL ERROR: ", "\n" ); exit( 0 ); }
	virtual void			DoFatalError( const char *msg, int code ) { printf( "FATAL ERROR: %s\n", msg ); exit( 0 ); }
	virtual void			SetErrorIndirection( bool ) {}
	virtual const char *		Translate( const char* msg ) { return NULL; }
	virtual I18N*			GetI18N() { return NULL; }
	virtual const char *		KeysFromBinding( const char *bind ) { return NULL; }
	virtual const char *		BindingFromKey( const char *key ) { return NULL; }
	virtual int			ButtonState( int key ) { return 0; }
	virtual int			KeyState( int key ) { return 0; }
	virtual bool            WindowAvailable(void) { return true; } // Agent Jones #3766
	virtual int					GetConsoleMarker(void) { return 0; }
	virtual idStr				GetConsoleContents(int begin, int end) { return ""; }

};

idCVar com_developer( "developer", "0", CVAR_BOOL|CVAR_SYSTEM, "developer mode" );
idCVar com_fpexceptions;

idCommonLocal		commonLocal;
idCommon *			common = &commonLocal;

/*
==============================================================

	idSys

==============================================================
*/

void			Sys_Mkdir( const char *path ) {}
ID_TIME_T		Sys_FileTimeStamp( FILE *fp ) { return 0; }

#ifdef _WIN32

#include <io.h>
#include <direct.h>

const char *Sys_Cwd( void ) {
	static char cwd[2048];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[sizeof( cwd ) - 1] = 0;

	return cwd;
}

const char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

const char *Sys_DefaultSavePath( void ) {
	static const char *savePath = NULL;
	// default savepath changed to the mod dir.
	if ( !savePath ) {
		idStr buff = cvarSystem->GetCVarString("fs_basepath");
		// only append the mod if it isn't "darkmod"
		if ( idStr::Icmp( cvarSystem->GetCVarString("fs_mod"), BASE_TDM ) ) {
			buff.AppendPath(cvarSystem->GetCVarString("fs_mod"));
		}
		savePath = Mem_CopyString(buff.c_str());
	}

	return savePath;
}

const char* Sys_ModSavePath() {
	// greebo: In Windows, we use the basepath + "darkmod/fms/" as savepath 
	// taaaki: changed this to savepath + "fms/"
	static const char *modSavePath = NULL;
	
	if ( !modSavePath ) {
		idStr buff = cvarSystem->GetCVarString("fs_savepath");
		buff.AppendPath("fms");
		modSavePath = Mem_CopyString(buff.c_str());
	}

	return modSavePath;
}

const char *Sys_EXEPath( void ) {
	return "";
}

int Sys_ListFiles( const char *directory, const char *extension, idStrList &list ) {
	if ( !extension) {
		extension = "";
	}

	int flag;
	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	idStr search;
	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	struct _finddata_t findinfo;
	auto findhandle = _findfirst( search, &findinfo );
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

void GetDeclLoadedFiles(idList<idStr> &) {}

#else

const char *	Sys_DefaultBasePath( void ) { return ""; }
const char *	Sys_DefaultSavePath( void ) { return ""; }
int				Sys_ListFiles( const char *directory, const char *extension, idStrList &list ) { return 0; }

#endif

#if 0
uintptr_t		Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority,
	const char* name, core_t core, int stackSize,
	bool suspended ) {
	return  0; 
}
void			Sys_DestroyThread( uintptr_t info ) {}
#endif

void			Sys_EnterCriticalSection( int index ) {}
void			Sys_LeaveCriticalSection( int index ) {}

void			Sys_WaitForEvent( int index ) {}
void			Sys_TriggerEvent( int index ) {}

/*
==============
idSysLocal stub
==============
*/
void			idSysLocal::DebugPrintf( const char *fmt, ... ) {}
void			idSysLocal::DebugVPrintf( const char *fmt, va_list arg ) {}

double			idSysLocal::GetClockTicks( void ) { return 0.0; }
double			idSysLocal::ClockTicksPerSecond( void ) { return 1.0; }
cpuid_t			idSysLocal::GetProcessorId( void ) { return (cpuid_t)0; }
const char *	idSysLocal::GetProcessorString( void ) { return ""; }
void			idSysLocal::FPU_SetFTZ( bool enable ) {}
void			idSysLocal::FPU_SetDAZ( bool enable ) {}
void			idSysLocal::FPU_SetExceptions( bool enable ) {}
void			idSysLocal::ThreadStartup( void ) {}
void			idSysLocal::ThreadHeartbeat( void ) {}

bool			idSysLocal::LockMemory( void *ptr, int bytes ) { return false; }
bool			idSysLocal::UnlockMemory( void *ptr, int bytes ) { return false; }

uintptr_t		idSysLocal::DLL_Load( const char *dllName ) { return 0; }
void *			idSysLocal::DLL_GetProcAddress( uintptr_t dllHandle, const char *procName ) { return NULL; }
void			idSysLocal::DLL_Unload( uintptr_t dllHandle ) { }
void			idSysLocal::DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) { }

sysEvent_t		idSysLocal::GenerateMouseButtonEvent( int button, bool down ) { sysEvent_t ev; memset( &ev, 0, sizeof( ev ) ); return ev; }
sysEvent_t		idSysLocal::GenerateMouseMoveEvent( int deltax, int deltay ) { sysEvent_t ev; memset( &ev, 0, sizeof( ev ) ); return ev; }

void			idSysLocal::OpenURL( const char *url, bool quit ) { }
void			idSysLocal::StartProcess( const char *exeName, bool quit ) { }

idSysLocal		sysLocal;
idSys *			sys = &sysLocal;


/*
==============================================================

	main

==============================================================
*/

int main( int argc, char** argv ) {
	idStr fileName, sourcePath;
	idTypeInfoGen *generator;

	idLib::common = common;
	idLib::cvarSystem = cvarSystem;
	idLib::fileSystem = fileSystem;
	idLib::sys = sys;

	idLib::Init();
	cmdSystem->Init();
	cvarSystem->Init();
	idCVar::RegisterStaticVars();
	fileSystem->Init();

	generator = new idTypeInfoGen;

	if ( argc > 1 ) {
		sourcePath = idStr( "../" SOURCE_CODE_BASE_FOLDER "/" ) + argv[1];
	} else {
		sourcePath = "../" SOURCE_CODE_BASE_FOLDER "/game";
	}

	if ( argc > 2 ) {
		fileName = idStr( "../" SOURCE_CODE_BASE_FOLDER "/" ) + argv[2];
	} else {
		fileName = "../" SOURCE_CODE_BASE_FOLDER "/game/gamesys/GameTypeInfo.h";
	}

	if ( argc > 3 ) {
		for ( int i = 3; i < argc; i++ ) {
			generator->AddDefine( argv[i] );
		}
	} else {
		generator->AddDefine( "__cplusplus" );
		generator->AddDefine( "_WIN32" );
		generator->AddDefine( "GAME_DLL" );
		generator->AddDefine( "ID_TYPEINFO" );
	}

	generator->CreateTypeInfo( sourcePath );
	generator->WriteTypeInfo( fileName );

	delete generator;

	fileName.Clear();
	sourcePath.Clear();

	fileSystem->Shutdown( false );
	cvarSystem->Shutdown();
	cmdSystem->Shutdown();
	idLib::ShutDown();

	return 0;
}
