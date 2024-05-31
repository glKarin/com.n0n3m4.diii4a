/* Android */

static void * game_main(void *data);

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

void GLimp_CheckGLInitialized(void)
{
    Q3E_CheckNativeWindowChanged();
}

// doom3 game main thread loop
void * game_main(void *data)
{
    attach_thread(); // attach current to JNI for call Android code

    // DG: needed for Sys_ReLaunch()
    Sys_SetArgs(q3e_argc, (const char**)q3e_argv);
    // DG end
#ifdef ID_MCHECK
    // must have -lmcheck linkage
	mcheck( abrt_func );
	Sys_Printf( "memory consistency checking enabled\n" );
#endif
    Q3E_Start();

#if defined(USE_FFMPEG)
#ifdef _DL_FFMPEG
    if(FFmpeg_Init())
        Sys_Printf("[Harmattan]: Using FFmpeg for playing bink cinematic.\n");
    else
        Sys_Printf("[Harmattan]: FFmpeg is disabled for playing bink cinematic!\n");
#endif
#endif

    Posix_EarlyInit();
    Sys_Printf("[Harmattan]: Enter doom3 main thread -> %s\n", "main");

    if( q3e_argc > 1 )
    {
        common->Init( q3e_argc - 1, &q3e_argv[1], NULL );
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
    Q3E_End();
    main_thread = 0;
    Sys_Printf("[Harmattan]: Leave doom3 main thread.\n");
    return 0;
}

void ShutdownGame(void)
{
    if(common->IsInitialized())
    {
        TRIGGER_WINDOW_CREATED; // if doom3 main thread is waiting new window
        Q3E_ShutdownGameMainThread();
        common->Quit();
    }
}

static void doom3_exit(void)
{
    Sys_Printf("[Harmattan]: doom3 exit.\n");

    Q3E_FreeArgs();

#if defined(USE_FFMPEG)
#ifdef _DL_FFMPEG
    FFmpeg_Shutdown();
#endif
#endif

    Q3E_CloseRedirectOutput();
}

int main(int argc, char **argv)
{
    Q3E_DumpArgs(argc, argv);

    Q3E_RedirectOutput();

    Q3E_PrintInitialContext(argc, argv);

    Q3E_StartGameMainThread();

    atexit(doom3_exit);

    return 0;
}