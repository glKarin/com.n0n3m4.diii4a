/*
 * Include this BEFORE darkplaces.h because it breaks wrapping
 * _Static_assert. Cloudwalk has no idea how or why so don't ask.
 */

#include "../darkplaces.h"
#include "../keys.h"
#include "../input.h"

#include <unistd.h>

/* Android */
#include "sys_android.c"

qbool GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

/*
=================
main
=================
*/
int main( int argc, char* argv[] ) {
	return Sys_Main(argc, argv);
}

void ShutdownGame(void)
{
	if(q3e_running && Q3E_IS_INITIALIZED)
	{
		Host_Shutdown();
		NOTIFY_EXIT;
		q3e_running = false;
	}
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
