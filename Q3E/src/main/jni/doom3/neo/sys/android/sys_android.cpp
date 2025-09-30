/* Android functions */

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "../../framework/Session_local.h"

#ifdef _IMGUI
#include "../../renderer/imgui/r_imgui.h"
#endif

#ifdef _MULTITHREAD
extern bool multithreadActive;
extern bool Sys_ShutdownRenderThread(void);
#endif

extern void GLimp_ActivateContext();
extern void GLimp_DeactivateContext();
extern void GLimp_AndroidOpenWindow(volatile ANativeWindow *win);
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);



/* Macros */
// Game states
#define STATE_NONE 0
#define STATE_ACT 1 // RTCW4A-specific, keep
#define STATE_GAME (1 << 1) // map spawned
#define STATE_KICK (1 << 2) // RTCW4A-specific, keep
#define STATE_LOADING (1 << 3) // current GUI is guiLoading
#define STATE_CONSOLE (1 << 4) // fullscreen
#define STATE_MENU (1 << 5) // any menu excludes guiLoading
#define STATE_DEMO (1 << 6) // demo

// Dialog result type
#define DIALOG_RESULT_ERROR -1 // open dialog fail
#define DIALOG_RESULT_CANCEL 0 // user cancel
#define DIALOG_RESULT_YES 1 // user accept
#define DIALOG_RESULT_NO 2 // user reject
#define DIALOG_RESULT_OTHER 3 // other result

// Which thread change ANativeWindow
#define WINDOW_CHANGE_THREAD_SURFACE_VIEW 0 // SurfaceView thread
#define WINDOW_CHANGE_THREAD_GAME 1 // Game main thread
#define WINDOW_CHANGE_THREAD_MAIN 2 // Activity main thread

// Android files default path
//#define _ANDROID_PACKAGE_NAME "com.n0n3m4.DIII4A"
#define _ANDROID_PACKAGE_NAME "com.karin.idTech4Amm"
#define _ANDROID_DLL_PATH "/data/data/" _ANDROID_PACKAGE_NAME "/lib/"
#define _ANDROID_GAME_DATA_PATH "/sdcard/Android/data/" _ANDROID_PACKAGE_NAME
#define _ANDROID_APP_HOME_PATH "/sdcard/Android/data/" _ANDROID_PACKAGE_NAME "/files"



/* Function pointers from JNI */
// Android AudioTrack(init, write, shutdown)
void (*initAudio)(void *buffer, int size);
int (*writeAudio)(int offset, int length);
void (*shutdownAudio)(void);

// Sync game state
void (*setState)(int st);

// Android tmpfile
FILE * (*itmpfile)(void);

// Pull input event
// num = 0: only clear; > 0: max num; -1: all.
// return pull event num
int (*pull_input_event)(int num);

// Setup smooth joystick
void (*setup_smooth_joystick)(int enable);

// Grab mouse
void (*grab_mouse)(int grab);

// Attach current native thread to JNI
void (*attach_thread)(void);

// Access Android clipboard
void (*copy_to_clipboard)(const char *text);
char * (*get_clipboard_text)(void);

// Show a Android toast or dialog
void (*show_toast)(const char *text);
int (*open_dialog)(const char *title, const char *message, int num, const char *buttons[]);

// Control Android keyboard
void (*open_keyboard)(void);
void (*close_keyboard)(void);

// Open URL
void (*open_url)(const char *url);

// Finish activity
void (*exit_finish)(void);

// Show cursor
void (*show_cursor)(int on);



/* Global context variables */
// Surface window size
int screen_width = 640;
int screen_height = 480;

// Screen refresh rate
int refresh_rate = 60;

// Smooth joystick
bool smooth_joystick = false;

// max console height frac
float console_max_height_frac = -1.0f;

// Using mouse
bool mouse_available = false;

// Do not catch signal
bool no_handle_signals = false;

// APK's native library path on Android.
char *native_library_dir = NULL;

// Game data directory.
char *game_data_dir = NULL;

// Application home directory.
char *app_home_dir = NULL;

// Enable redirect stdout/stderr to file
static bool redirect_output_to_file = true;

// Using external libraries
bool using_external_libs = false;

// Continue when missing OpenGL context
volatile bool continue_when_no_gl_context = false;

// Surface window
volatile ANativeWindow *window = NULL;
static volatile bool window_changed = false;

// App paused
volatile bool paused = false;

// App exit
extern volatile bool q3e_running;



/* Functions */
// window changed
static bool Q3E_WindowChanged(ANativeWindow *w, int fromNative)
{
    Sys_EnterCriticalSection(CRITICAL_SECTION_SYS);

    bool res = false;
    if(fromNative == WINDOW_CHANGE_THREAD_GAME) // native
    {
        if(window_changed)
        {
            Sys_Printf("[Native]: ANativeWindow changed: %p.\n", window);
#ifdef _MULTITHREAD
            // shutdown render thread
            Sys_ShutdownRenderThread();
#endif
            if(window)
            {
                GLimp_AndroidInit(window);
                Sys_Printf("[Native]: Notify GL context created.\n");
                window_changed = false;
                Sys_TriggerEvent(TRIGGER_EVENT_CONTEXT_CREATED);
            }
            else
            {
                if(q3e_running)
                {
                    GLimp_AndroidQuit();
                    Sys_Printf("[Native]: Notify GL context destroyed.\n");
                    window_changed = false;
                    Sys_TriggerEvent(TRIGGER_EVENT_CONTEXT_DESTROYED);
                    res = true;
                }
                else
                {
                    Sys_Printf("[Native]: GL context has shutdown.\n");
                    window_changed = false;
                }
            }
        }
    }
    else if(fromNative == WINDOW_CHANGE_THREAD_SURFACE_VIEW) // jni/java surface view
    {
        window = w;
        window_changed = true;
        if(window)
        {
            Sys_Printf("[Java/JNI]: Notify ANativeWindow created.\n");
            Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_CREATED);
        }
        res = true;
    }
    else // jni/java main
    {
        Sys_Printf("[Java/JNI]: Notify application destroyed.\n");
        window = NULL;
        window_changed = true;
        Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_CREATED);
        res = true;
    }

    Sys_LeaveCriticalSection(CRITICAL_SECTION_SYS);

    return res;
}

// because SurfaceView may be destroy or create new ANativeWindow in DOOM3 main thread
bool Q3E_CheckNativeWindowChanged(void)
{
    if(window_changed)
    {
        if(Q3E_WindowChanged(NULL, true))
        {
#if 0
            if(continue_when_no_gl_context)
            {
                    return;
            }
#endif
            Sys_Printf("[Native]: Waiting ANativeWindow create......\n");
            // wait new ANativeWindow created
            while(!window_changed)
            {
                Sys_WaitForEvent(TRIGGER_EVENT_WINDOW_CREATED);
            }
            Q3E_WindowChanged(NULL, true);
        }
        return window != NULL;
    }
    else
        return true;
}

// Setup OpenGL context variables in Android SurfaceView's thread
void Q3E_SetGLContext(ANativeWindow *w)
{
    // if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
    if(common->IsInitialized())
    {
        Sys_Printf("ANativeWindow changed: %p\n", w);
        if(w) // set new window, notify doom3 main thread active OpenGL render context
        {
            Q3E_WindowChanged(w, WINDOW_CHANGE_THREAD_SURFACE_VIEW);
            Sys_Printf("[Java/JNI]: Waiting GL context create......\n");
            while(window_changed)
            {
                Sys_WaitForEvent(TRIGGER_EVENT_CONTEXT_CREATED);
            }
            Sys_Printf("[Java/JNI]: GL context created.\n");
        }
        else // set window is null, and wait doom3 main thread deactive OpenGL render context.
        {
            Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_SURFACE_VIEW);
            Sys_Printf("[Java/JNI]: Waiting GL context destroy......\n");
            while(window_changed)
            {
                Sys_WaitForEvent(TRIGGER_EVENT_CONTEXT_DESTROYED);
            }
            Sys_Printf("[Java/JNI]: GL context destroyed.\n");
        }
    }
    else
        window = w;
}

// JNI request thread quit
void Q3E_RequestThreadQuit(void)
{
    // if idTech4 main thread is waiting new window
    q3e_running = false;
    Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_MAIN);
}

// start/end in main function
static void game_exit(void)
{
    printf(GAME_NAME " exit.\n");
}

void Q3E_Start(void)
{
    Sys_Printf("Enter " GAME_NAME " main thread.\n");
    atexit(game_exit);
    main_thread = pthread_self();
    q3e_running = true;

    GLimp_AndroidOpenWindow(window);
}

void Q3E_End(void)
{
    q3e_running = false;
    GLimp_AndroidQuit();
    window = NULL;
    main_thread = 0;
    Sys_Printf("Leave " GAME_NAME " main thread.\n");
}



/* Functions of wrap JNI callback */
void Android_GrabMouseCursor(bool grabIt)
{
    //if(mouse_available/* && grab_mouse*/)
        grab_mouse(grabIt);
}

void Android_PollInput(void)
{
    //if(pull_input_event)
    pull_input_event(-1);
}

void Android_ClearEvents(void)
{
    //if(pull_input_event)
    pull_input_event(0);
}

int Android_PollEvents(int num)
{
    //if(pull_input_event)
    return pull_input_event(num);
}

void Android_EnableSmoothJoystick(bool enable)
{
    // if(smooth_joystick != enable)
    {
        setup_smooth_joystick(enable);
        smooth_joystick = enable;
    }
}

FILE * Sys_tmpfile(void)
{
    FILE *f = /*itmpfile ? */itmpfile()/* : NULL*/;
    if (!f) {
        common->Warning("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
}

void Android_SetClipboardData(const char *text)
{
    // if(copy_to_clipboard)
    copy_to_clipboard(text);
}

char * Android_GetClipboardData(void)
{
/*    if(!get_clipboard_text)
        return NULL;*/
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

void Android_OpenURL(const char *url)
{
    //if(open_url)
    open_url(url);
}

int Android_OpenDialog(const char *title, const char *message, int num, const char *buttons[])
{
    //if(open_dialog)
    return open_dialog(title, message, num, buttons);
}

void Android_OpenKeyboard(void)
{
    //if(open_keyboard)
    open_keyboard();
}

void Android_CloseKeyboard(void)
{
    //if(close_keyboard)
    close_keyboard();
}

void Android_ShowInfo(const char *info)
{
    //if(show_toast)
    show_toast(info);
}

void Android_ExitFinish(void)
{
    //if(exit_finish)
    exit_finish();
}

void Android_ShowCursor(int on)
{
    //if(show_cursor)
    show_cursor(on);
}

float Android_GetConsoleMaxHeightFrac(float frac)
{
    return console_max_height_frac > 0.0f && console_max_height_frac < frac ? console_max_height_frac : frac;
}

// always return full path of libraries
const char * Sys_DLLInternalPath(void)
{
    return native_library_dir ? native_library_dir : _ANDROID_DLL_PATH;
}

// return full path of libraries if using external libs; else return empty
const char * Sys_DLLDefaultPath(void)
{
    if(!using_external_libs)
        return "";
    return Sys_DLLInternalPath();
}

const char * Sys_GameDataDefaultPath(void)
{
    return game_data_dir ? game_data_dir : _ANDROID_GAME_DATA_PATH;
}

const char * Sys_ApplicationHomePath(void)
{
    return app_home_dir ? app_home_dir : _ANDROID_APP_HOME_PATH;
}



/* Functions of JNI call */
void Sys_SyncState(void)
{
    static int prev_state = -1;
    static int state = -1;
    // if (setState)
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

static bool ctrl_state = false;
void Q3E_KeyEvent(int state,int key,int character)
{
    if(key == K_CTRL)
        ctrl_state = bool(state);

    Posix_QueEvent(SE_KEY, key, state, 0, NULL);
    if ((character != 0) && (state == 1))
    {
        if(!ctrl_state || character == '\b')
        Posix_QueEvent(SE_CHAR, character, 0, 0, NULL);
    }
#ifdef _IMGUI
    if(!R_ImGui_IsRunning())
#endif
    Posix_AddKeyboardPollEvent(key, state);
}

void Q3E_MotionEvent(float dx, float dy)
{
    Posix_QueEvent(SE_MOUSE, dx, dy, 0, NULL);
#ifdef _IMGUI
    if(!R_ImGui_IsRunning())
    {
#endif
    Posix_AddMousePollEvent(M_DELTAX, dx);
    Posix_AddMousePollEvent(M_DELTAY, dy);
#ifdef _IMGUI
    }
#endif
}

void Q3E_Analog(int enable,float x,float y)
{
    analogenabled = enable;
    analogx = x;
    analogy = y;
}

// View paused
void Q3E_OnPause(void)
{
    if(common->IsInitialized())
        paused = true;
}

// View resume
void Q3E_OnResume(void)
{
    if(common->IsInitialized())
        paused = false;
}

// Request exit
void Q3E_exit(void)
{
    q3e_running = false;
    ShutdownGame();
    if(window)
        window = NULL;
    GLimp_AndroidQuit();
    printf(GAME_NAME " exit......\n");
    //Android_ExitFinish();
}

int Q3E_Main(int argc, char **argv)
{
    Q3E_Start();

    int res = main(argc, (const char **)argv);

    Q3E_End();

    return res;
}

// Setup JNI callback functions
void Q3E_SetCallbacks(const void *callbacks)
{
    const Q3E_Callback_t *ptr = (const Q3E_Callback_t *)callbacks;

    initAudio = ptr->AudioTrack_init;
    writeAudio = ptr->AudioTrack_write;
    shutdownAudio = ptr->AudioTrack_shutdown;

    grab_mouse = ptr->Input_grabMouse;
    pull_input_event = ptr->Input_pullEvent;
    setup_smooth_joystick = ptr->Input_setupSmoothJoystick;

    attach_thread = ptr->Sys_attachThread;
    itmpfile = ptr->Sys_tmpfile;
    copy_to_clipboard = ptr->Sys_copyToClipboard;
    get_clipboard_text = ptr->Sys_getClipboardText;
    open_keyboard = ptr->Sys_openKeyboard;
    close_keyboard = ptr->Sys_closeKeyboard;
    open_url = ptr->Sys_openURL;
    exit_finish = ptr->Sys_exitFinish;
    show_cursor = ptr->Sys_showCursor;

    show_toast = ptr->Gui_ShowToast;
    open_dialog = ptr->Gui_openDialog;

    setState = ptr->set_state;
}

// Setup initial environment variants
void Q3E_SetInitialContext(const void *context)
{
    const Q3E_InitialContext_t *ptr = (const Q3E_InitialContext_t *)context;

    gl_format = ptr->openGL_format;
    gl_depth_bits = ptr->openGL_depth;
    gl_msaa = ptr->openGL_msaa;
    gl_version = ptr->openGL_version;
#ifdef _OPENGLES3
    USING_GLES3 = gl_version != 0x00020000;
#else
    USING_GLES3 = false;
#endif

    native_library_dir = strdup(ptr->nativeLibraryDir ? ptr->nativeLibraryDir : "");
    game_data_dir = strdup(ptr->gameDataDir ? ptr->gameDataDir : "");
    app_home_dir = strdup(ptr->appHomeDir ? ptr->appHomeDir : "");
    redirect_output_to_file = ptr->redirectOutputToFile ? true : false;
    no_handle_signals = ptr->noHandleSignals ? true : false;
#ifdef _MULTITHREAD
    multithreadActive = ptr->multithread ? true : false;
#endif
    continue_when_no_gl_context = ptr->continueWhenNoGLContext ? true : false;
    mouse_available = ptr->mouseAvailable ? true : false;
    refresh_rate = ptr->refreshRate <= 0 ? 60 : ptr->refreshRate;
    smooth_joystick = ptr->smoothJoystick ? true : false;
    console_max_height_frac = ptr->consoleMaxHeightFrac > 0 && ptr->consoleMaxHeightFrac < 100 ? (float)ptr->consoleMaxHeightFrac / 100.0f : -1.0f;
    using_external_libs = ptr->usingExternalLibs ? true : false;

    window = ptr->window;
    screen_width = ptr->width;
    screen_height = ptr->height;
}

extern "C"
{
#pragma GCC visibility push(default)
void GetIDTechAPI(void *d3interface)
{
    Q3E_Interface_t *ptr = (Q3E_Interface_t *)d3interface;
    memset(ptr, 0, sizeof(*ptr));

    ptr->main = &Q3E_Main;
    ptr->setCallbacks = &Q3E_SetCallbacks;
    ptr->setInitialContext = &Q3E_SetInitialContext;

    ptr->pause = &Q3E_OnPause;
    ptr->resume = &Q3E_OnResume;
    ptr->exit = &Q3E_exit;

    ptr->setGLContext = &Q3E_SetGLContext;
    ptr->requestThreadQuit = Q3E_RequestThreadQuit;

    ptr->keyEvent = &Q3E_KeyEvent;
    ptr->analogEvent = &Q3E_Analog;
    ptr->motionEvent = &Q3E_MotionEvent;
}

#pragma GCC visibility pop
}
