/* Android functions */

#if !defined(Q3E_GAME_NAME)
#error "Q3E_GAME_NAME must defined for as game name!"
#endif

#if !defined(Q3E_IS_INITIALIZED)
#error "Q3E_IS_INITIALIZED must defined for using check engine is initialized!"
#endif

#if !defined(Q3E_THREAD_MAIN)
#error "Q3E_THREAD_MAIN must defined for using thread main function!"
#endif

#if !defined(Q3E_INIT_WINDOW)
#error "Q3E_INIT_WINDOW must defined for using initialize window!"
#endif

#if !defined(Q3E_QUIT_WINDOW)
#error "Q3E_QUIT_WINDOW must defined for using quit window!"
#endif

#if !defined(Q3E_CHANGE_WINDOW)
#error "Q3E_CHANGE_WINDOW must defined for using change window!"
#endif

#if !defined(Q3E_PRINTF)
#warning "Q3E_PRINTF defined for using print to stdout!"
#include <stdio.h>
#define Q3E_PRINTF printf
#endif

#if !defined(Q3E_WID_RESTART)
#warning "Q3E_WID_RESTART defined for using restart OpenGL context!"
#define Q3E_WID_RESTART
#endif

#if !defined(Q3E_DRAW_FRAME)
#warning "Q3E_DRAW_FRAME defined for using run frame!"
#define Q3E_DRAW_FRAME
#endif

#if !defined(Q3E_SHUTDOWN_GAME)
#warning "Q3E_SHUTDOWN_GAME defined for using shutdown game!"
#define Q3E_SHUTDOWN_GAME
#endif

#if !defined(Q3Ebool)
#warning "Q3Ebool defined for as bool type!"
#ifdef __cplusplus
#define Q3Ebool bool
#else
#define Q3Ebool _Bool
#endif
#endif

#if !defined(Q3E_TRUE)
#warning "Q3E_TRUE defined for as true value!"
#ifdef __cplusplus
#define Q3E_TRUE true
#else
#define Q3E_TRUE true
#endif
#endif

#if !defined(Q3E_FALSE)
#warning "Q3E_FALSE defined for as false value!"
#ifdef __cplusplus
#define Q3E_FALSE false
#else
#define Q3E_FALSE false
#endif
#endif

#if !defined(dp_strlcpy) // Quake1
#define dp_strlcpy strncpy
#endif

// if need init threads, define this
// #define Q3E_REQUIRE_THREAD

// These functions must declared.
extern int main(int argc, char **argv);
extern void Q3E_KeyEvent(int state, int key, int character);
extern void Q3E_MotionEvent(float dx, float dy);

#include <errno.h>
#include <string.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <pthread.h>

#include "q3e_android.h"

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
#define _ANDROID_DLL_PATH "/data/data/" _ANDROID_PACKAGE_NAME "/lib"
#define _ANDROID_GAME_DATA_PATH "/sdcard/Android/data/" _ANDROID_PACKAGE_NAME

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
int (*pull_input_event)(int num);

// Grab mouse
void (*grab_mouse)(int grab);

// Attach current thread to JNI
void (*attach_thread)(void);

// Access Android clipboard
void (*copy_to_clipboard)(const char *text);
char * (*get_clipboard_text)(void);

// Show a Android toast as dialog
void (*show_toast)(const char *text);

// Access Android keyboard
void (*open_keyboard)(void);
void (*close_keyboard)(void);

// Surface window size
int screen_width = 640;
int screen_height = 480;

// Redirect stdout/stderr file
FILE *f_stdout = NULL;
FILE *f_stderr = NULL;

// Analog
float analogx = 0.0f;
float analogy = 0.0f;
int analogenabled = 0;

// OpenGL color format
int gl_format = 0x8888;
// OpenGL multisamples
int gl_msaa = 0;

// Screen refresh rate
int refresh_rate = 60;

// APK's native library path on Android.
char *native_library_dir = NULL;

// Do not catch signal
Q3Ebool no_handle_signals = Q3E_FALSE;

// Enable redirect stdout/stderr to file
static Q3Ebool redirect_output_to_file = Q3E_TRUE;

// Continue when missing OpenGL context
volatile Q3Ebool continue_when_no_gl_context = Q3E_FALSE;

// Using mouse
Q3Ebool mouse_available = Q3E_FALSE;

// Game data directory.
char *game_data_dir = NULL;

// Command line arguments
static int q3e_argc = 0;
static char **q3e_argv = NULL;

// Game main thread
static pthread_t main_thread;

// App paused
volatile Q3Ebool paused = Q3E_FALSE;

// Surface window
volatile ANativeWindow *window = NULL;
static volatile Q3Ebool window_changed = Q3E_FALSE;

// App exit
volatile Q3Ebool q3e_running = false;

#include "q3e_thread.inc"
#if !defined(INIT_Q3E_THREADS)
#warning "INIT_Q3E_THREADS defined for using initialize threads!"
#define INIT_Q3E_THREADS
#endif

#if !defined(WAIT_WINDOW_DESTROYED)
#error "WAIT_WINDOW_DESTROYED must defined for using wait window destroyed!"
#endif

#if !defined(TRIGGER_WINDOW_DESTROYED)
#error "TRIGGER_WINDOW_DESTROYED must defined for using trigger window destroyed!"
#endif

#if !defined(WAIT_WINDOW_CREATED)
#error "WAIT_WINDOW_CREATED must defined for using wait window created!"
#endif

#if !defined(TRIGGER_WINDOW_CREATED)
#error "TRIGGER_WINDOW_CREATED must defined for using trigger window created!"
#endif

static void Q3E_DumpArgs(int argc, char **argv)
{
    q3e_argc = argc;
    q3e_argv = (char **) malloc(sizeof(char *) * argc);
    for (int i = 0; i < argc; i++)
    {
        q3e_argv[i] = strdup(argv[i]);
    }
}

static void Q3E_FreeArgs(void)
{
    for(int i = 0; i < q3e_argc; i++)
    {
        free(q3e_argv[i]);
    }
    free(q3e_argv);
    q3e_argv = NULL;
    q3e_argc = 0;
}

// start game main thread from Android Surface thread
static void Q3E_StartGameMainThread(void)
{
    if(main_thread)
        return;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
        Q3E_PRINTF("[Harmattan]: ERROR: pthread_attr_setdetachstate quake1 main thread failed\n");
        exit(1);
    }

    if (pthread_create((pthread_t *)&main_thread, &attr, Q3E_THREAD_MAIN, NULL) != 0) {
        Q3E_PRINTF("[Harmattan]: ERROR: pthread_create quake1 main thread failed\n");
        exit(1);
    }

    pthread_attr_destroy(&attr);

    q3e_running = true;
    Q3E_PRINTF("[Harmattan]: " Q3E_GAME_NAME " main thread start.\n");
}

// shutdown game main thread
static void Q3E_ShutdownGameMainThread(void)
{
    if(!main_thread)
        return;

    q3e_running = false;
    if (pthread_join(main_thread, NULL) != 0) {
        Q3E_PRINTF("[Harmattan]: ERROR: pthread_join " Q3E_GAME_NAME " main thread failed\n");
    }
    main_thread = 0;
    Q3E_PRINTF("[Harmattan]: " Q3E_GAME_NAME " main thread quit.\n");
}

void Android_GrabMouseCursor(Q3Ebool grabIt)
{
    if(mouse_available/* && grab_mouse*/)
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

FILE * Sys_tmpfile(void)
{
    Q3E_PRINTF("Call JNI::tmpfile.\n");
    FILE *f = /*itmpfile ? */itmpfile()/* : NULL*/;
    if (!f) {
        Q3E_PRINTF("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
}

void Android_SetClipboardData(const char *text)
{
    //if(copy_to_clipboard)
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
    char *ptr = (char *)malloc(len + 1);
    dp_strlcpy(ptr, text, len);
    ptr[len] = '\0';
    free(text);
    return ptr;
}

void Q3E_PrintInitialContext(int argc, char **argv)
{
    printf("[Harmattan]: " Q3E_GAME_NAME " start\n\n");

    printf("Q3E initial context\n");
    printf("  OpenGL: \n");
    printf("    Format: 0x%X\n", gl_format);
    printf("    MSAA: %d\n", gl_msaa);
    printf("  Variables: \n");
    printf("    Native library directory: %s\n", native_library_dir);
    printf("    Redirect output to file: %d\n", redirect_output_to_file);
    printf("    No handle signals: %d\n", no_handle_signals);
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
    printf("    open_keyboard: %p\n", open_keyboard);
    printf("    close_keyboard: %p\n", close_keyboard);
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
    screen_width = width;
    screen_height = height;
}

void Q3E_OGLRestart()
{
    Q3E_WID_RESTART;
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
    open_keyboard = ptr->Sys_openKeyboard;
    close_keyboard = ptr->Sys_closeKeyboard;

    show_toast = ptr->Gui_ShowToast;

    setState = ptr->set_state;
}

void Q3E_DrawFrame()
{
    Q3E_DRAW_FRAME;
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

    native_library_dir = strdup(ptr->nativeLibraryDir);
    game_data_dir = strdup(ptr->gameDataDir);
    redirect_output_to_file = ptr->redirectOutputToFile ? Q3E_TRUE : Q3E_FALSE;
    no_handle_signals = ptr->noHandleSignals ? Q3E_TRUE : Q3E_FALSE;
    continue_when_no_gl_context = ptr->continueWhenNoGLContext ? Q3E_TRUE : Q3E_FALSE;
    mouse_available = ptr->mouseAvailable ? Q3E_TRUE : Q3E_FALSE;
    refresh_rate = ptr->refreshRate <= 0 ? 60 : ptr->refreshRate;
}

// View paused
void Q3E_OnPause(void)
{
    if(Q3E_IS_INITIALIZED)
        paused = Q3E_TRUE;
}

// View resume
void Q3E_OnResume(void)
{
    if(Q3E_IS_INITIALIZED)
        paused = Q3E_FALSE;
}

// because SurfaceView may be destroy or create new ANativeWindow in DOOM3 main thread
void Q3E_CheckNativeWindowChanged(void)
{
    if(window_changed)
    {
        if(window) // if set new window, create EGLSurface
        {
            Q3E_CHANGE_WINDOW(window);
            window_changed = Q3E_FALSE;
        }
        else // if window is null, release old window, and notify JNI, and wait new window set
        {
            Q3E_QUIT_WINDOW();
            window_changed = Q3E_FALSE;
            TRIGGER_WINDOW_DESTROYED;
#if 0
            if(continue_when_no_gl_context)
			{
            	return;
			}
#endif
            // wait new ANativeWindow created
            while(!window_changed)
            {
                WAIT_WINDOW_CREATED;
            }
            window_changed = Q3E_FALSE;
            Q3E_CHANGE_WINDOW(window);
        }
    }
}

// Request exit
void Q3E_exit(void)
{
    q3e_running = Q3E_FALSE;
    Q3E_SHUTDOWN_GAME;
    if(window)
        window = NULL;
    Q3E_QUIT_WINDOW();
    Q3E_PRINTF("[Harmattan]: " Q3E_GAME_NAME " exit.\n");
}

// Setup OpenGL context variables in Android SurfaceView's thread
void Q3E_SetGLContext(ANativeWindow *w)
{
    // if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
    if(Q3E_IS_INITIALIZED)
    {
        Q3E_PRINTF("[Harmattan]: ANativeWindow changed: %p\n", w);
        if(!w) // set window is null, and wait game main thread deactive OpenGL render context.
        {
            window = NULL;
            window_changed = Q3E_TRUE;
            while(window_changed)
            {
                WAIT_WINDOW_DESTROYED;
            }
        }
        else // set new window, notify game main thread active OpenGL render context
        {
            window = w;
            window_changed = Q3E_TRUE;
            TRIGGER_WINDOW_CREATED;
        }
    }
    else
        window = w;
}

void Q3E_Start(void)
{
    Q3E_INIT_WINDOW(window);
}

void Q3E_End(void)
{
    Q3E_QUIT_WINDOW();
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

volatile ANativeWindow * Android_GetWindow(void)
{
    return window;
}

#ifdef __cplusplus
extern "C" {
#endif
#pragma GCC visibility push(default)
void GetIDTechAPI(void *d3interface) {
    Q3E_Interface_t *ptr = (Q3E_Interface_t *) d3interface;
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
#ifdef __cplusplus
}
#endif