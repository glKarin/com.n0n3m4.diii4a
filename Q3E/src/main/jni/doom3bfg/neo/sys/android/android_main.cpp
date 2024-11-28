/* Android */

// called first thing. does InitSigs and various things
extern void		Posix_EarlyInit( );
// called after common has been initialized
extern void		Posix_LateInit( );

#if defined(USE_FFMPEG)
#ifdef _DL_FFMPEG
extern bool FFmpeg_Init(void);
extern void FFmpeg_Shutdown(void);
#endif
#endif

#include "sys_android.inc"

extern void Sys_SetArgs(int argc, const char** argv);

bool GLimp_CheckGLInitialized(void)
{
    return Q3E_CheckNativeWindowChanged();
}

/*
===============
main
===============
*/
int main( int argc, /*const */char** argv )
{
    // DG: needed for Sys_ReLaunch()
    Sys_SetArgs(argc, (const char**)argv);
    // DG end
#ifdef ID_MCHECK
    // must have -lmcheck linkage
	mcheck( abrt_func );
	Sys_Printf( "memory consistency checking enabled\n" );
#endif

    Posix_EarlyInit();

    if( argc > 1 )
    {
        common->Init( argc - 1, &argv[1], NULL );
    }
    else
    {
        common->Init( 0, NULL, NULL );
    }

    Posix_LateInit();

    while (1) {
        if(!q3e_running) // exit
            break;
        common->Frame();
    }

    common->Quit();

    return 0;
}

void ShutdownGame(void)
{
    if(q3e_running && common->IsInitialized())
    {
        common->Quit();
        NOTIFY_EXIT;
        q3e_running = false;
    }
}
