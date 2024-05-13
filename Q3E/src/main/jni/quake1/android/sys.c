/*
 * Include this BEFORE darkplaces.h because it breaks wrapping
 * _Static_assert. Cloudwalk has no idea how or why so don't ask.
 */

#include "../darkplaces.h"
#include "../keys.h"
#include "../input.h"

#include <unistd.h>

/* Android */

static void * game_main(void *data);

#include "sys_android.c"

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

// quake1 game main thread loop
void * game_main(void *data)
{
	attach_thread(); // attach current to JNI for call Android code

	Q3E_Start();

	Con_Printf("[Harmattan]: Enter " Q3E_GAME_NAME " main thread -> %s\n", "main");

	Sys_Main(q3e_argc, q3e_argv);

	Q3E_End();
	main_thread = 0;
	Con_Printf("[Harmattan]: Leave " Q3E_GAME_NAME " main thread.\n");
	return 0;
}

void ShutdownGame(void)
{
	if(Q3E_IS_INITIALIZED)
	{
		TRIGGER_WINDOW_CREATED; // if quake1 main thread is waiting new window
		Q3E_ShutdownGameMainThread();
		//common->Quit();
	}
}

static void game_exit(void)
{
	Con_Printf("[Harmattan]: quake1 exit.\n");

	sys.argc = 0;
	sys.argv = NULL;
	Q3E_FreeArgs();

	Q3E_CloseRedirectOutput();
}

void Sys_SyncState(void)
{
	//if (setState)
	{
		//keydest_t keydest = (key_consoleactive & KEY_CONSOLEACTIVE_USER) ? key_console : key_dest;
		static int prev_state = -1;
		static int state = -1;
		state = (key_dest == key_game) << 1;
		if (state != prev_state)
		{
			(*setState)(state);
			prev_state = state;
		}
	}
}

int
main(int argc, char **argv)
{
	Q3E_DumpArgs(argc, argv);

	Q3E_RedirectOutput();

	Q3E_PrintInitialContext(argc, argv);

	INIT_Q3E_THREADS;

	Q3E_StartGameMainThread();

	atexit(game_exit);

	return 0;
}


// =======================================================================
// General routines
// =======================================================================

void Sys_SDL_Shutdown(void)
{
	//SDL_Quit();
}

// Sys_Error early in startup might screw with automated
// workflows or something if we show the dialog by default.
static qbool nocrashdialog = true;
void Sys_SDL_Dialog(const char *title, const char *string)
{
	//if(!nocrashdialog)
	{
        extern void (*show_toast)(const char *text);
        if(show_toast)
            (*show_toast)(string);
	}
}

char *Sys_SDL_GetClipboardData (void)
{
    return Android_GetClipboardData();
}

void Sys_SDL_Init(void)
{
}

qbool sys_supportsdlgetticks = true;
unsigned int Sys_SDL_GetTicks (void)
{
	return 0; // SDL_GetTicks();
}

void Sys_SDL_Delay (unsigned int milliseconds)
{
	usleep(milliseconds * 1000);
}
