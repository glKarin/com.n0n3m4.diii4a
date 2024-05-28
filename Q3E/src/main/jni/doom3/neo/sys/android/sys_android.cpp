/* Android functions */

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "../../framework/Session_local.h"

#define STATE_NONE 0
#define STATE_ACT 1 // RTCW4A-specific, keep
#define STATE_GAME (1 << 1) // map spawned
#define STATE_KICK (1 << 2) // RTCW4A-specific, keep
#define STATE_LOADING (1 << 3) // current GUI is guiLoading
#define STATE_CONSOLE (1 << 4) // fullscreen
#define STATE_MENU (1 << 5) // any menu excludes guiLoading
#define STATE_DEMO (1 << 6) // demo

//#define _ANDROID_PACKAGE_NAME "com.n0n3m4.DIII4A"
#define _ANDROID_PACKAGE_NAME "com.karin.idTech4Amm"
#define _ANDROID_DLL_PATH "/data/data/" _ANDROID_PACKAGE_NAME "/lib/"
#define _ANDROID_GAME_DATA_PATH "/sdcard/Android/data/" _ANDROID_PACKAGE_NAME

#ifdef _MULTITHREAD
extern bool multithreadActive;
extern bool Sys_ShutdownRenderThread(void);
#endif

extern void GLimp_ActivateContext();
extern void GLimp_DeactivateContext();
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

// Android AudioTrack
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
int (*pull_input_event)(int execCmd);

// Grab mouse
void (*grab_mouse)(int grab);

// Attach current thread to JNI
void (*attach_thread)(void);

// Access Android clipboard
void (*copy_to_clipboard)(const char *text);
char * (*get_clipboard_text)(void);

// Show a Android toast as dialog
void (*show_toast)(const char *text);

// Surface window size
int screen_width = 640;
int screen_height = 480;

// Redirect stdout/stderr file
FILE *f_stdout = NULL;
FILE *f_stderr = NULL;

// Screen refresh rate
int refresh_rate = 60;

// APK's native library path on Android.
char *native_library_dir = NULL;

// Do not catch signal
bool no_handle_signals = false;

// Enable redirect stdout/stderr to file
static bool redirect_output_to_file = true;

// Using mouse
bool mouse_available = false;

// Game data directory.
char *game_data_dir = NULL;

// Surface window
volatile ANativeWindow *window = NULL;
static volatile bool window_changed = false;

// App paused
volatile bool paused = false;

// Continue when missing OpenGL context
volatile bool continue_when_no_gl_context = false;

// App exit
extern volatile bool q3e_running;

void Android_GrabMouseCursor(bool grabIt)
{
    if(mouse_available/* && grab_mouse*/)
        grab_mouse(grabIt);
}

void Android_PollInput(void)
{
    //if(pull_input_event)
        pull_input_event(-1);
}

FILE * Sys_tmpfile(void)
{
    common->Printf("Call JNI::tmpfile.\n");
    FILE *f = /*itmpfile ? */itmpfile()/* : NULL*/;
    if (!f) {
        common->Warning("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
}

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
    Mem_Free(text);
    return ptr;
}

void Q3E_PrintInitialContext(int argc, char **argv)
{
    printf("[Harmattan]: DOOM3 start\n\n");

    printf("Q3E initial context\n");
    printf("  OpenGL: \n");
    printf("    Format: 0x%X\n", gl_format);
    printf("    MSAA: %d\n", gl_msaa);
    printf("    Version: %08x\n", gl_version);
    printf("  Variables: \n");
    printf("    Native library directory: %s\n", native_library_dir);
    printf("    Redirect output to file: %d\n", redirect_output_to_file);
    printf("    No handle signals: %d\n", no_handle_signals);
#ifdef _MULTITHREAD
    printf("    Multi-thread: %d\n", multithreadActive);
#endif
    printf("    Using mouse: %d\n", mouse_available);
    printf("    Game data directory: %s\n", game_data_dir);
    printf("    Refresh rate: %d\n", refresh_rate);
    printf("    Continue when missing OpenGL context: %d\n", continue_when_no_gl_context);
    printf("\n");

    printf("Q3E callback\n");
    printf("  AudioTrack: \n");
    printf("    initAudio: %p\n", initAudio);
    printf("    writeAudio: %p\n", writeAudio);
    printf("    shutdownAudio: %p\n", shutdownAudio);
    printf("  Input: \n");
    printf("    grab_mouse: %p\n", grab_mouse);
    printf("    pull_input_event: %p\n", pull_input_event);
    printf("  System: \n");
    printf("    attach_thread: %p\n", attach_thread);
    printf("    tmpfile: %p\n", itmpfile);
    printf("    copy_to_clipboard: %p\n", copy_to_clipboard);
    printf("    get_clipboard_text: %p\n", get_clipboard_text);
    printf("  GUI: \n");
    printf("    show_toast: %p\n", show_toast);
    printf("  Other: \n");
    printf("    setState: %p\n", setState);
    printf("\n");

    printf("Q3E command line arguments: %d\n", argc);
    for(int i = 0; i < argc; i++)
    {
        printf("  %d: %s\n", i, argv[i]);
    }
    printf("\n");
}

void Q3E_SetResolution(int width, int height)
{
    screen_width=width;
    screen_height=height;
}

void Q3E_OGLRestart()
{
//	R_VidRestart_f();
}

void Q3E_SetCallbacks(const void *callbacks)
{
    const Q3E_Callback_t *ptr = (const Q3E_Callback_t *)callbacks;

    initAudio = ptr->AudioTrack_init;
    writeAudio = ptr->AudioTrack_write;
    shutdownAudio = ptr->AudioTrack_shutdown;

    pull_input_event = ptr->Input_pullEvent;
    grab_mouse = ptr->Input_grabMouse;

    attach_thread = ptr->Sys_attachThread;
    itmpfile = ptr->Sys_tmpfile;
    copy_to_clipboard = ptr->Sys_copyToClipboard;
    get_clipboard_text = ptr->Sys_getClipboardText;

    show_toast = ptr->Gui_ShowToast;

    setState = ptr->set_state;
}

void Q3E_KeyEvent(int state,int key,int character)
{
    Posix_QueEvent(SE_KEY, key, state, 0, NULL);
    if ((character != 0) && (state == 1))
    {
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

void Q3E_DrawFrame()
{
    common->Frame();
}

void Q3E_Analog(int enable,float x,float y)
{
    analogenabled = enable;
    analogx = x;
    analogy = y;
}

void Q3E_RedirectOutput(void)
{
    if(redirect_output_to_file)
    {
        f_stdout = freopen("stdout.txt","w",stdout);
        setvbuf(stdout, NULL, _IONBF, 0);
        f_stderr = freopen("stderr.txt","w",stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
}

void Q3E_CloseRedirectOutput(void)
{
    if(f_stdout != NULL)
    {
        fclose(f_stdout);
        f_stdout = NULL;
    }
    if(f_stderr != NULL)
    {
        fclose(f_stderr);
        f_stderr = NULL;
    }
}

// Setup initial environment variants
void Q3E_SetInitialContext(const void *context)
{
    const Q3E_InitialContext_t *ptr = (const Q3E_InitialContext_t *)context;

    gl_format = ptr->openGL_format;
    gl_msaa = ptr->openGL_msaa;
    gl_version = ptr->openGL_version;
#ifdef _OPENGLES3
    USING_GLES3 = gl_version != 0x00020000;
#else
    USING_GLES3 = false;
#endif

    native_library_dir = strdup(ptr->nativeLibraryDir);
    game_data_dir = strdup(ptr->gameDataDir);
    redirect_output_to_file = ptr->redirectOutputToFile ? true : false;
    no_handle_signals = ptr->noHandleSignals ? true : false;
#ifdef _MULTITHREAD
    multithreadActive = ptr->multithread ? true : false;
#endif
    continue_when_no_gl_context = ptr->continueWhenNoGLContext ? true : false;
    mouse_available = ptr->mouseAvailable ? true : false;
    refresh_rate = ptr->refreshRate <= 0 ? 60 : ptr->refreshRate;
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

// because SurfaceView may be destroy or create new ANativeWindow in DOOM3 main thread
void Q3E_CheckNativeWindowChanged(void)
{
    if(window_changed)
    {
#ifdef _MULTITHREAD
        // shutdown render thread
        Sys_ShutdownRenderThread();
#endif
        if(window) // if set new window, create EGLSurface
        {
            GLimp_AndroidInit(window);
            window_changed = false;
        }
        else // if window is null, release old window, and notify JNI, and wait new window set
        {
            GLimp_AndroidQuit();
            window_changed = false;
            Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_DESTROYED);
#if 0
            if(continue_when_no_gl_context)
			{
            	return;
			}
#endif
            // wait new ANativeWindow created
            while(!window_changed)
                Sys_WaitForEvent(TRIGGER_EVENT_WINDOW_CREATED);
            window_changed = false;
            GLimp_AndroidInit(window);
        }
    }
}

// Request exit
void Q3E_exit(void)
{
    q3e_running = false;
    ShutdownGame();
    if(window)
        window = NULL;
    GLimp_AndroidQuit();
    Sys_Printf("[Harmattan]: doom3 exit.\n");
}

// Setup OpenGL context variables in Android SurfaceView's thread
void Q3E_SetGLContext(ANativeWindow *w)
{
    // if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
    if(common->IsInitialized())
    {
        Sys_Printf("[Harmattan]: ANativeWindow changed: %p\n", w);
        if(!w) // set window is null, and wait doom3 main thread deactive OpenGL render context.
        {
            window = NULL;
            window_changed = true;
            while(window_changed)
                Sys_WaitForEvent(TRIGGER_EVENT_WINDOW_DESTROYED);
        }
        else // set new window, notify doom3 main thread active OpenGL render context
        {
            window = w;
            window_changed = true;
            Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_CREATED);
        }
    }
    else
        window = w;
}

void Q3E_Start(void)
{
    GLimp_AndroidInit(window);
}

void Q3E_End(void)
{
    GLimp_AndroidQuit();
    window = NULL;
}

const char * Sys_DLLDefaultPath(void)
{
    return native_library_dir ? native_library_dir : _ANDROID_DLL_PATH;
}

const char * Sys_GameDataDefaultPath(void)
{
    return game_data_dir ? game_data_dir : _ANDROID_GAME_DATA_PATH;
}

extern "C"
{

#pragma GCC visibility push(default)
void GetIDTechAPI(void *d3interface)
{
    Q3E_Interface_t *ptr = (Q3E_Interface_t *)d3interface;
    memset(ptr, 0, sizeof(*ptr));

    ptr->main = &main;
    ptr->setCallbacks = &Q3E_SetCallbacks;
    ptr->setInitialContext = &Q3E_SetInitialContext;
    ptr->setResolution = &Q3E_SetResolution;

    ptr->pause = &Q3E_OnPause;
    ptr->resume = &Q3E_OnResume;
    ptr->exit = &Q3E_exit;

    ptr->setGLContext = &Q3E_SetGLContext;

    ptr->frame = &Q3E_DrawFrame;
    ptr->vidRestart = &Q3E_OGLRestart;

    ptr->keyEvent = &Q3E_KeyEvent;
    ptr->analogEvent = &Q3E_Analog;
    ptr->motionEvent = &Q3E_MotionEvent;
}

#pragma GCC visibility pop
}
