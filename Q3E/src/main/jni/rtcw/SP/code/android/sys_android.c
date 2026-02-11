#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "RTCW"
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

extern void GLimp_AndroidOpenWindow(volatile ANativeWindow *w);
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

#if 0
		int state = (((cl.snap.ps.serverCursorHint==HINT_DOOR_ROTATING)||(cl.snap.ps.serverCursorHint==HINT_DOOR)
				  ||(cl.snap.ps.serverCursorHint==HINT_BUTTON)||(cl.snap.ps.serverCursorHint==HINT_ACTIVATE)) << 0) | ((clc.state == CA_ACTIVE && Key_GetCatcher() == 0) << 1) | ((cl.snap.ps.serverCursorHint==HINT_BREAKABLE) << 2);
//cl.snap.ps.pm_flags & PMF_DUCKED;
//    else
//        state = 0;
#else
		int state = STATE_NONE;
		//Com_Printf("xxx %d %d %d\n", clc.state,CA_ACTIVE,Key_GetCatcher());

		if(clc.state == CA_ACTIVE)
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
		else if(clc.state == CA_CINEMATIC)
		{
			state |= STATE_GAME;
		}
#endif

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

