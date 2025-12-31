//#include "../zdoomsupport.h"

extern int PortableInMenu(void);

#include "q3e/q3e_android.h"

void ShutdownGame();

#define Q3E_GAME_NAME "Wolf3D"
#define Q3E_IS_INITIALIZED (q3e_initialized)
#define Q3E_PRINTF printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

bool q3e_initialized = false;

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

Q3Ebool Q3E_IsRunning()
{
	return q3e_running;
}

void ShutdownGame() {
	if(q3e_running/* && q3e_initialized*/)
	{
		q3e_running = false;
		NOTIFY_EXIT;
	}
}

void Sys_SyncState()
{
    int state = PortableInMenu() ? STATE_MENU : STATE_GAME;
    //if (setState)
    {
        static int prev_state = -1;

        if (state != prev_state)
        {
            setState(state);
            prev_state = state;
        }
    }
}

static bool Q3E_AnalogEvent(float *x, float *y)
{
    if(analogenabled)
    {
        if(x) *x = analogx;
        if(y) *y = analogy;
    }
    return analogenabled;
}

void Sys_Analog(int &side, int &forward, const int delta)
{
    if (analogenabled)
    {
        forward = (int)(delta * -analogy);
        side = (int)(delta * analogx);
    }
}

