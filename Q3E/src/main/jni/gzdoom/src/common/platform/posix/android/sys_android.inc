#include <stdio.h>

#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "GZDOOM"
#define Q3E_IS_INITIALIZED (true/*common->IsInitialized()*/)
#define Q3E_PRINTF printf
#define Q3E_WID_RESTART
#define Q3E_DRAW_FRAME { /*GameMain*/ }
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_THREAD_MAIN game_main
#define Q3E_INIT_WINDOW GLimp_AndroidInit
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit
#define Q3E_REQUIRE_THREAD

extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

#include "q3e/q3e_android.inc"

#include "common/engine/gamestate.h"
#include "common/console/c_console.h"
#include "common/menu/menustate.h"

void Sys_SyncState(void)
{
    static int prev_state = -1;
    static int state = -1;

    state = STATE_NONE;
    if(gamestate == GS_FULLCONSOLE)
        state |= STATE_CONSOLE;
    else if(menuactive != MENU_Off)
        state |= STATE_MENU;
    else if(gamestate == GS_LEVEL)
        state |= STATE_GAME;

    if(ConsoleState == c_down)
        state |= STATE_CONSOLE;

    if (state != prev_state)
    {
        setState(state);
        prev_state = state;
    }
}
