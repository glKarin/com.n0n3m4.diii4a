#ifdef __ANDROID__
// Android functions
#include "../../framework/Session_local.h"

#define STATE_NONE 0
#define STATE_ACT 1 // RTCW4A-specific, keep
#define STATE_GAME (1 << 1) // map spawned
#define STATE_KICK (1 << 2) // RTCW4A-specific, keep
#define STATE_LOADING (1 << 3) // current GUI is guiLoading
#define STATE_CONSOLE (1 << 4) // fullscreen
#define STATE_MENU (1 << 5) // any menu excludes guiLoading
#define STATE_DEMO (1 << 6) // demo

extern void (*grab_mouse)(int grab);
extern void (*pull_input_event)(int execCmd);
extern FILE * (*itmpfile)(void);

void Android_GrabMouseCursor(bool grabIt)
{
    if(grab_mouse)
        grab_mouse(grabIt);
}

void Android_PollInput(void)
{
    if(pull_input_event)
        pull_input_event(1);
}

FILE * Sys_tmpfile(void)
{
    common->Printf("Call JNI::tmpfile.\n");
    FILE *f = itmpfile ? itmpfile() : NULL;
    if (!f) {
        common->Warning("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
}

void Sys_SyncState(void)
{
    static int prev_state = -1;
    static int state = -1;
    if (setState)
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

#endif