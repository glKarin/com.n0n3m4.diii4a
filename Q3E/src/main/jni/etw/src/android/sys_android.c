#include "../client/client.h"

extern qboolean com_fullyInitialized;

static void * game_main(int argc, char **argv);

#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "ETW"
#define Q3E_IS_INITIALIZED (com_fullyInitialized)
#define Q3E_PRINTF Com_Printf
#define Q3E_WID_RESTART CL_Vid_Restart_f()
#define Q3E_DRAW_FRAME { \
            Com_Frame( ); \
        }
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool qboolean
#define Q3E_TRUE qtrue
#define Q3E_FALSE qfalse
#define Q3E_REQUIRE_THREAD
#define Q3E_THREAD_MAIN game_main
#define Q3E_INIT_WINDOW GLimp_AndroidInit
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

extern void CL_Vid_Restart_f(void);

#include "q3e/q3e_android.inc"

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

// RTCW game main thread loop
void * game_main(int argc, char **argv)
{
	char commandLine[MAX_STRING_CHARS] = { 0 };

	attach_thread(); // attach current to JNI for call Android code
	Q3E_Start();

	Sys_PlatformInit();

	// Set the initial time base
	Sys_Milliseconds();

	Sys_ParseArgs(argc, argv);

	Sys_SetBinaryPath(Sys_Dirname(argv[0]));

	Sys_SetDefaultInstallPath(DEFAULT_BASEDIR); // Sys_BinaryPath() by default

	// Concatenate the command line for passing to Com_Init
	Sys_BuildCommandLine(argc, argv, commandLine, sizeof(commandLine));
	if(!no_handle_signals)
		Sys_SetUpConsoleAndSignals();

	Com_Init(commandLine);

	//FIXME: Lets not enable this yet for normal use
#if !defined(DEDICATED) && defined(FEATURE_SSL)
	// Check for certificates
	Com_CheckCaCertStatus();
#endif

	Sys_GameLoop();

	Q3E_FreeArgs();

	Q3E_End();
	main_thread = 0;
	//IsInitialized = false;
	Com_Printf("[Harmattan]: Leave " Q3E_GAME_NAME " main thread.\n");
	return 0;
}

void ShutdownGame(void)
{
	if(com_fullyInitialized)
	{
		TRIGGER_WINDOW_CREATED; // if RTCW main thread is waiting new window
		Q3E_ShutdownGameMainThread();
		//common->Quit();
	}
}

static void game_exit(void)
{
	Com_Printf("[Harmattan]: " Q3E_GAME_NAME " exit.\n");

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

		int state = STATE_NONE;
		//Com_Printf("xxx %d %d %d\n", clc.state,CA_ACTIVE,Key_GetCatcher());

		if(cls.state == CA_ACTIVE)
		{
			int c = Key_GetCatcher();
			if(c == 0)
				state |= STATE_GAME;
			else
			{
				if(c & KEYCATCH_CGAME)
					state |= STATE_GAME;
				if(c & KEYCATCH_UI)
					state |= STATE_MENU;
				if(c & KEYCATCH_CONSOLE)
					state |= STATE_CONSOLE;
			}
		}
		else if(cls.state == CA_CINEMATIC)
		{
			state |= STATE_GAME;
		}

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


