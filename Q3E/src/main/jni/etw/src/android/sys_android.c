#include "../client/client.h"

extern qboolean com_fullyInitialized;

#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "ETW"
#define Q3E_IS_INITIALIZED (com_fullyInitialized)
#define Q3E_PRINTF Com_Printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool qboolean
#define Q3E_TRUE qtrue
#define Q3E_FALSE qfalse
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

extern void GLimp_AndroidOpenWindow(volatile ANativeWindow *win);
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

#include "q3e/q3e_android.inc"

qboolean GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

/**
 * @brief SDL_main
 * @param[in] argc
 * @param[in] argv
 * @return
 */
int main(int argc, char **argv)
{
	char commandLine[MAX_STRING_CHARS] = { 0 };

	Sys_PlatformInit();

	// Set the initial time base
	Sys_Milliseconds();

	// TODO : check if we shouldn't just decide to skip this call when we build
	// the Android target
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

	return 0;
}

void ShutdownGame(void)
{
	if(q3e_running && com_fullyInitialized)
	{
        q3e_running = false;
        NOTIFY_EXIT;
	}
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


