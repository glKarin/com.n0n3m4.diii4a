#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "Quake3"
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

void Sys_SyncState(void)
{
	//if (setState)
	{
		static int prev_state = -1;
		/* We are in game and neither console/ui is active */
		//if (cls.state == CA_ACTIVE && Key_GetCatcher() == 0)

		int state = ((clc.state == CA_ACTIVE) && (Key_GetCatcher() == 0)) << 1;

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}
