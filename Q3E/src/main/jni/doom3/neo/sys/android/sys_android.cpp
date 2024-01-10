/* Android functions */

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
extern void (*copy_to_clipboard)(const char *text);
extern char * (*get_clipboard_text)(void);

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
        if(sessLocal.insideExecuteMapChange
#ifdef _RAVEN
				&& !sessLocal.FinishedLoading()
#endif
				)
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

void Android_SetClipboardData(const char *text)
{
    if(copy_to_clipboard)
        copy_to_clipboard(text);
}

char * Android_GetClipboardData(void)
{
    if(!get_clipboard_text)
        return NULL;
    char *text = get_clipboard_text();
    if(!text)
        return NULL;
    size_t len = strlen(text);
    char *ptr = (char *)Mem_Alloc(len + 1);
    strncpy(ptr, text, len);
    ptr[len] = '\0';
    free(text);
    return ptr;
}

void AndroidSetResolution(int32_t width, int32_t height)
{
    cvarSystem->SetCVarBool("r_fullscreen",  true);
    cvarSystem->SetCVarInteger("r_mode", -1);

    cvarSystem->SetCVarInteger("r_customWidth", width);
    cvarSystem->SetCVarInteger("r_customHeight", height);

    float r = (float) width / (float) height;

    if (r > 1.7f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 1);    // 16:9
    } else if (r > 1.55f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 2);    // 16:10
    } else {
        cvarSystem->SetCVarInteger("r_aspectRatio", 0);    // 4:3
    }

    Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i)",
               -1, width, height);
}
