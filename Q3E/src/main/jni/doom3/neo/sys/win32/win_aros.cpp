#include "../../idlib/precompiled.h"

#define DEFAULT_SYS_VIDEO_MEMORY 512 // 64
idCVar sys_videoRam("sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER,
                    "Texture memory on the video card (in megabytes) - 0: autodetect", 0, 1024);
int Sys_GetVideoRam( void ) {
	// return 64;
    static int run_once = 0;

    if (run_once) {
        return run_once;
    }

    if (sys_videoRam.GetInteger()) {
        run_once = sys_videoRam.GetInteger();
        return sys_videoRam.GetInteger();
    }

    // try a few strategies to guess the amount of video ram
    common->Printf("guessing video ram ( use +set sys_videoRam to force ) ..\n");

    run_once = DEFAULT_SYS_VIDEO_MEMORY;
    return run_once;
}


/*
==================
Sys_SetFatalError
==================
*/
void Sys_SetFatalError( const char *error ) {
    // idStr::Copynz( fatalError, error, sizeof( fatalError ) );
}

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols(void)
{
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackCurStr(int depth)
{
/*    address_t array[ 32 ];
    size_t size;

    size = backtrace((void **)array, Min(32, depth));
    return Sys_GetCallStackStr(array, (int)size);*/
return "";
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackStr(const address_t *callStack, const int callStackSize)
{
    return "";
}

/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack(address_t *callStack, const int callStackSize)
{
    /*int i;
    i = backtrace((void **)callStack, callStackSize);

    while (i < callStackSize) {
        callStack[i++] = 0;
    }*/
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
================
Sys_AlreadyRunning
return true if there is a copy of D3 running already
================
*/
bool Sys_AlreadyRunning(void)
{
    return false;
}

/*
 ==================
 Sys_DoPreferences
 ==================
 */
void Sys_DoPreferences(void) { }

/*
==============
Sys_DefaultBasePath
==============
*/
const char *Sys_DefaultBasePath( void ) {
    extern const char *Sys_Cwd(void);
    return Sys_Cwd();
}

/*
==============
Sys_DefaultSavePath
==============
*/
const char *Sys_DefaultSavePath( void ) {
    return cvarSystem->GetCVarString( "fs_basepath" );
}

/*
==============
Sys_DefaultCDPath
==============
*/
const char *Sys_DefaultCDPath( void ) {
    return "";
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
