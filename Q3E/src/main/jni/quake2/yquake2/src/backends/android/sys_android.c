#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "Quake2"
#define Q3E_IS_INITIALIZED (IsInitialized)
#define Q3E_PRINTF Com_Printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool qboolean
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit
#define Q3E_PRE_START { IsInitialized = true; }
#define Q3E_PRE_END { IsInitialized = false; }

extern void GLimp_AndroidOpenWindow(volatile ANativeWindow *win);
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

#include "q3e/q3e_android.inc"

void Q3E_KeyEvent(int state,int key,int character)
{
    if (key!=0)
    {
        //printf("xxx %d %c | %d %c\n", key, key, character,character);
        if(key == '`' || key == '~')
            key = K_CONSOLE;
        qboolean isChar = isprint(character);
        Key_Event(key, state, !isChar);
        if(state && isChar && key != K_CONSOLE)
            Char_Event(character);
    }
}

extern float mouse_x, mouse_y;
void Q3E_MotionEvent(float dx, float dy)
{
    if (cls.key_dest == key_game && (int) cl_paused->value == 0)
    {
        mouse_x += dx;
        mouse_y += dy;
    }
}

#include "../../client/header/client.h"
extern kbutton_t in_forward, in_back, in_moveleft, in_moveright;
void IN_Analog(const kbutton_t *key, float *val)
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
