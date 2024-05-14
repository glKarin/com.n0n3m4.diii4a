#include "q3e/q3e_android.h"

#include "../quakedef.h"

#define Q3E_GAME_NAME "Quake1"
#define Q3E_IS_INITIALIZED (host.state == host_active)
#define Q3E_PRINTF Con_Printf
#define Q3E_WID_RESTART VID_Restart_f(NULL)
#define Q3E_DRAW_FRAME
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool qbool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_THREAD_MAIN game_main
#define Q3E_INIT_WINDOW GLimp_AndroidInit
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

extern void VID_Restart_f(cmd_state_t *cmd);

#include "q3e/q3e_android.inc"

void Q3E_KeyEvent(int state,int key,int character)
{
    Key_Event(key, character, state);
}

void Q3E_MotionEvent(float dx, float dy)
{
    in_mouse_x += dx;
    in_mouse_y += dy;

    in_windowmouse_x += dx;
    if (in_windowmouse_x < 0)
        in_windowmouse_x = 0;
    else if (in_windowmouse_x > screen_width - 1)
        in_windowmouse_x = screen_width - 1;

    in_windowmouse_y += dy;
    if (in_windowmouse_y < 0)
        in_windowmouse_y = 0;
    else if (in_windowmouse_y > screen_height - 1)
        in_windowmouse_y = screen_height - 1;
}

extern kbutton_t	in_moveleft, in_moveright, in_forward, in_back;
void CL_Analog(const kbutton_t *key, float *val)
{
    if (analogenabled)
    {
        if (key == &in_moveright)
            *val = fmax(0, analogx);
        if (key == &in_moveleft)
            *val = fmax(0, -analogx);
        if (key == &in_forward)
            *val = fmax(0, analogy);
        if (key == &in_back)
            *val = fmax(0, -analogy);
    }
}