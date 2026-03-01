#include "q3e/q3e_android.h"

void ShutdownGame();

#define Q3E_GAME_NAME "Icarus Starship Command Simulator"
#define Q3E_IS_INITIALIZED (common->IsInitialized())
#define Q3E_PRINTF Sys_Printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit
#define Q3E_MAIN_CONST_ARGS

#include "framework/Session_local.h"
#include "framework/Console.h"

void GLimp_AndroidOpenWindow(volatile ANativeWindow *win)
{
	if(!win)
		return;

	ANativeWindow_acquire((ANativeWindow *)win);
	Q3E_PRINTF("[Harmattan]: ANativeWindow acquired.\n");
}

void GLimp_AndroidInit(volatile ANativeWindow *win)
{
	if(!win)
		return;

	ANativeWindow_acquire((ANativeWindow *)win);
	Q3E_PRINTF("[Harmattan]: ANativeWindow acquired.\n");
}

void GLimp_AndroidQuit(void)
{
	//ANativeWindow_release((ANativeWindow *)win);
	//win = NULL;
	Q3E_PRINTF("[Harmattan]: ANativeWindow released.\n");
}

void Q3E_MotionEvent(float dx, float dy){}
void Q3E_KeyEvent(int state,int key,int character){}

#include "q3e/q3e_android.inc"

Q3Ebool GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

void ShutdownGame() {
    if(q3e_running && common->IsInitialized())
	{
        common->Quit();
        NOTIFY_EXIT;
        q3e_running = false;
	}
}

void Sys_SyncState(void)
{
    static int prev_state = -1;
    static int state = -1;
    //if (setState)
    {
        state = STATE_NONE;
        if(sessLocal.insideExecuteMapChange)
            state |= STATE_LOADING;
        else
        {
            idUserInterface *gui = sessLocal.GetActiveMenu();
            if(!gui)
                state |= STATE_GAME;
            else
                state |= STATE_MENU;
        }
        if(console->Active())
            state |= STATE_CONSOLE;
        if (state != prev_state)
        {
            setState(state);
            prev_state = state;
        }
    }
}

#if 0
void Q3E_KeyEvent(int state,int key,int character)
{
    Posix_QueEvent(SE_KEY, key, state, 0, NULL);
    if ((character != 0) && (state == 1)) {
        Posix_QueEvent(SE_CHAR, character, 0, 0, NULL);
    }
    Posix_AddKeyboardPollEvent(key, state);
}

void Q3E_MotionEvent(float dx, float dy)
{
    Posix_QueEvent(SE_MOUSE, dx, dy, 0, NULL);
    Posix_AddMousePollEvent(M_DELTAX, dx);
    Posix_AddMousePollEvent(M_DELTAY, dy);
}
#endif

void Sys_Analog(int &side, int &forward, const int &KEY_MOVESPEED)
{
    if (analogenabled)
    {
        side = (int)(KEY_MOVESPEED * analogx);
        forward = (int)(KEY_MOVESPEED * analogy);
    }
}

void Sys_ForceResolution(void)
{
    cvarSystem->SetCVarBool("r_fullscreen",  true);
    cvarSystem->SetCVarInteger("r_mode", -1);

    cvarSystem->SetCVarInteger("r_customWidth", screen_width);
    cvarSystem->SetCVarInteger("r_customHeight", screen_height);

#if 0
    if (r > 1.7f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 1);    // 16:9
    } else if (r > 1.55f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 2);    // 16:10
    } else {
        cvarSystem->SetCVarInteger("r_aspectRatio", 0);    // 4:3
    }
#endif

    Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i)",
               -1, screen_width, screen_height);
}
