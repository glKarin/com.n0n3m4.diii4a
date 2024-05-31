#include <stdio.h>

#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "DOOM3-BFG"
#define Q3E_IS_INITIALIZED (common->IsInitialized())
#define Q3E_PRINTF common->Printf
#define Q3E_WID_RESTART
#define Q3E_DRAW_FRAME { common->Frame(); }
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_THREAD_MAIN game_main
#define Q3E_INIT_WINDOW GLimp_AndroidInit
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit
#define Q3E_REQUIRE_THREAD

#include "precompiled.h"
#include "../../framework/Common_local.h"
extern idCommonLocal commonLocal;
#if defined(USE_DOOMCLASSIC)
#include "../../../doomclassic/doom/globaldata.h"
static int classicdoom_gamestate = -1;
extern void Sys_SetClassicDOOMGameState(int state)
{
    classicdoom_gamestate = state;
}
#endif

extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

#include "q3e/q3e_android.inc"

void Sys_SyncState(void)
{
    static int prev_state = -1;
    static int state = -1;
    //if (setState)
    {
        state = STATE_NONE;
#if defined(USE_DOOMCLASSIC)
		currentGame_t gameType = commonLocal.GetCurrentGame();
		switch(gameType)
		{
			case DOOM3_BFG:
			default:
#endif
            if(commonLocal.insideExecuteMapChange)
                state |= STATE_LOADING;
            else
            {
                idGame *gameLocal = commonLocal.Game();
                if(gameLocal && gameLocal->Shell_IsActive())
                    state |= STATE_MENU;
                else
                    state |= STATE_GAME;
            }
            if(console->Active())
                state |= STATE_CONSOLE;

#if defined(USE_DOOMCLASSIC)
            break;

			case DOOM_CLASSIC:
			case DOOM2_CLASSIC:
                if(classicdoom_gamestate == GS_DEMOSCREEN)
                    state |= STATE_MENU;
                else if(classicdoom_gamestate == GS_LEVEL)
                    state |= STATE_GAME;
                if(console->Active())
                    state |= STATE_CONSOLE;
            break;
        }
#endif
        if (state != prev_state)
        {
            setState(state);
            prev_state = state;
        }
    }
}

void Sys_ForceResolution(void)
{
    cvarSystem->SetCVarBool("r_fullscreen",  true);
    cvarSystem->SetCVarInteger("r_vidMode", -1);

    cvarSystem->SetCVarInteger("r_customWidth", screen_width);
    cvarSystem->SetCVarInteger("r_customHeight", screen_height);

#if 0
    float r = (float) screen_width / (float) screen_height;

    if (r > 1.7f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 1);    // 16:9
    } else if (r > 1.55f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 2);    // 16:10
    } else {
        cvarSystem->SetCVarInteger("r_aspectRatio", 0);    // 4:3
    }
#endif
    cvarSystem->SetCVarInteger("r_displayRefresh", refresh_rate);
    cvarSystem->SetCVarInteger("r_antiAliasing", 0);

    Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i)",
               -1, screen_width, screen_height);
}

intptr_t Sys_GetMainThread(void)
{
    return main_thread;
}
