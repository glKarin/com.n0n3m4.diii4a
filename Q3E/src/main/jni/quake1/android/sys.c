/*
 * Include this BEFORE darkplaces.h because it breaks wrapping
 * _Static_assert. Cloudwalk has no idea how or why so don't ask.
 */

#include "../darkplaces.h"
#include "../keys.h"
#include "../input.h"

#include <unistd.h>

#define host_initialized (host.state = host_active)

extern void Android_PollInput(void);
extern void Q3E_CheckNativeWindowChanged(void);
int main_time, main_oldtime, main_newtime;
void main_frame()
{
	Android_PollInput();
	Q3E_CheckNativeWindowChanged();
}

/* Android */

#include "sys_android.c"

// command line arguments
static int q3e_argc = 0;
static char **q3e_argv = NULL;

// game main thread
static pthread_t				main_thread;

// app exit
volatile qbool q3e_running = false;

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

static void Q3E_DumpArgs(int argc, const char **argv)
{
	q3e_argc = argc;
	q3e_argv = (char **) malloc(sizeof(char *) * argc);
	for (int i = 0; i < argc; i++)
	{
		q3e_argv[i] = strdup(argv[i]);
	}
}

static void Q3E_FreeArgs(void)
{
	for(int i = 0; i < q3e_argc; i++)
	{
		free(q3e_argv[i]);
	}
	free(q3e_argv);
}

// quake1 game main thread loop
static void * quake1_main(void *data)
{
	attach_thread(); // attach current to JNI for call Android code

	Q3E_Start();

	Con_Printf("[Harmattan]: Enter quake1 main thread -> %s\n", "main");

	Sys_Main(q3e_argc, q3e_argv);

	Q3E_End();
	main_thread = 0;
	Con_Printf("[Harmattan]: Leave quake1 main thread.\n");
	return 0;
}

// start game main thread from Android Surface thread
static void Q3E_StartGameMainThread(void)
{
	if(main_thread)
		return;

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		Con_Printf("[Harmattan]: ERROR: pthread_attr_setdetachstate quake1 main thread failed\n");
		exit(1);
	}

	if (pthread_create((pthread_t *)&main_thread, &attr, quake1_main, NULL) != 0) {
		Con_Printf("[Harmattan]: ERROR: pthread_create quake1 main thread failed\n");
		exit(1);
	}

	pthread_attr_destroy(&attr);

	q3e_running = true;
	Con_Printf("[Harmattan]: quake1 main thread start.\n");
}

// shutdown game main thread
static void Q3E_ShutdownGameMainThread(void)
{
	if(!main_thread)
		return;

	q3e_running = false;
	if (pthread_join(main_thread, NULL) != 0) {
		Con_Printf("[Harmattan]: ERROR: pthread_join quake1 main thread failed\n");
	}
	main_thread = 0;
	Con_Printf("[Harmattan]: quake1 main thread quit.\n");
}

void ShutdownGame(void)
{
	if(host_initialized)
	{
		Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_CREATED); // if quake1 main thread is waiting new window
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
	if (setState)
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
	Q3E_DumpArgs(argc, (const char **)argv);

	Q3E_RedirectOutput();

	Q3E_PrintInitialContext(argc, (const char **)argv);

	Sys_InitThreads();

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
