/* Android functions */

#include <errno.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <pthread.h>

#include "sys_android.h"

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

void (* GLimp_AndroidInit)(volatile ANativeWindow *win);
void (* GLimp_AndroidQuit)(void);
extern void ShutdownGame(void);

void (*initAudio)(void *buffer, int size);
int (*writeAudio)(int offset, int length);
void (*shutdownAudio)(void);
void (*setState)(int st);
FILE * (*itmpfile)(void);
void (*pull_input_event)(int execCmd);
void (*grab_mouse)(int grab);
void (*attach_thread)(void);
void (*copy_to_clipboard)(const char *text);
char * (*get_clipboard_text)(void);

int screen_width=640;
int screen_height=480;
FILE *f_stdout = NULL;
FILE *f_stderr = NULL;

float analogx=0.0f;
float analogy=0.0f;
int analogenabled=0;

int gl_format = 0x8888;
int gl_msaa = 0;

// APK's native library path on Android.
char *native_library_dir = NULL;

// Do not catch signal
qboolean no_handle_signals = false;

// enable redirect stdout/stderr to file
static qboolean redirect_output_to_file = true;

// app paused
volatile qboolean paused = false;

// Continue when missing OpenGL context
volatile qboolean continue_when_no_gl_context = false;

// Surface window
volatile ANativeWindow *window = NULL;
static volatile qboolean window_changed = false;

// app exit
extern volatile qboolean q3e_running;

// we use an extra lock for the local stuff
const int MAX_CRITICAL_SECTIONS		= 4;
#define MAX_LOCAL_CRITICAL_SECTIONS 5
static pthread_mutex_t global_lock[ MAX_LOCAL_CRITICAL_SECTIONS ];

/*
==================
Sys_EnterCriticalSection
==================
*/
void Sys_EnterCriticalSection(int index)
{
    assert(index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS);
    pthread_mutex_lock(&global_lock[index]);
}

/*
==================
Sys_LeaveCriticalSection
==================
*/
void Sys_LeaveCriticalSection(int index)
{
    assert(index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS);
    pthread_mutex_unlock(&global_lock[index]);
}

#define MAX_TRIGGER_EVENTS 2
enum {
    TRIGGER_EVENT_WINDOW_CREATED, // Android SurfaceView thread -> game/renderer thread: notify native window is set
    TRIGGER_EVENT_WINDOW_DESTROYED, // game thread/render thread -> Android SurfaceView thread: notify released OpenGL context
};

static pthread_cond_t	event_cond[ MAX_TRIGGER_EVENTS ];
static qboolean 			signaled[ MAX_TRIGGER_EVENTS ];
static qboolean			waiting[ MAX_TRIGGER_EVENTS ];

/*
==================
Sys_WaitForEvent
==================
*/
void Sys_WaitForEvent(int index)
{
    assert(index >= 0 && index < MAX_TRIGGER_EVENTS);
    Sys_EnterCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
    assert(!waiting[ index ]);	// WaitForEvent from multiple threads? that wouldn't be good

    if (signaled[ index ]) {
        // emulate windows behaviour: signal has been raised already. clear and keep going
        signaled[ index ] = false;
    } else {
        waiting[ index ] = true;
        pthread_cond_wait(&event_cond[ index ], &global_lock[ MAX_LOCAL_CRITICAL_SECTIONS - 1 ]);
        waiting[ index ] = false;
    }

    Sys_LeaveCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
}

/*
==================
Sys_TriggerEvent
==================
*/
void Sys_TriggerEvent(int index)
{
    assert(index >= 0 && index < MAX_TRIGGER_EVENTS);
    Sys_EnterCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);

    if (waiting[ index ]) {
        pthread_cond_signal(&event_cond[ index ]);
    } else {
        // emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
        signaled[ index ] = true;
    }

    Sys_LeaveCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
}

/*
==================
Posix_InitPThreads
==================
*/
void Sys_InitThreads()
{
    int i;
    pthread_mutexattr_t attr;

    // init critical sections
    for (i = 0; i < MAX_LOCAL_CRITICAL_SECTIONS; i++) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&global_lock[i], &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // init event sleep/triggers
    for (i = 0; i < MAX_TRIGGER_EVENTS; i++) {
        pthread_cond_init(&event_cond[ i ], NULL);
        signaled[i] = false;
        waiting[i] = false;
    }
}

void Android_GrabMouseCursor(qboolean grabIt)
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
    Com_Printf("Call JNI::tmpfile.\n");
    FILE *f = itmpfile ? itmpfile() : NULL;
    if (!f) {
        Com_Printf("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
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
    char *ptr = (char *)malloc(len + 1);
    strncpy(ptr, text, len);
    ptr[len] = '\0';
    free(text);
    return ptr;
}

void Sys_ForceResolution(void)
{
/*    cvarSystem->SetCVarBool("r_fullscreen",  true);
    cvarSystem->SetCVarInteger("r_mode", -1);

    cvarSystem->SetCVarInteger("r_customWidth", screen_width);
    cvarSystem->SetCVarInteger("r_customHeight", screen_height);

    float r = (float) screen_width / (float) screen_height;

    if (r > 1.7f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 1);    // 16:9
    } else if (r > 1.55f) {
        cvarSystem->SetCVarInteger("r_aspectRatio", 2);    // 16:10
    } else {
        cvarSystem->SetCVarInteger("r_aspectRatio", 0);    // 4:3
    }

    Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i)",
               -1, screen_width, screen_height);*/
}

void Q3E_PrintInitialContext(int argc, const char **argv)
{
    printf("[Harmattan]: DIII4A start\n\n");

    printf("DOOM3 initial context\n");
    printf("  OpenGL: \n");
    printf("    Format: 0x%X\n", gl_format);
    printf("    MSAA: %d\n", gl_msaa);
    printf("  Variables: \n");
    printf("    Native library directory: %s\n", native_library_dir);
    printf("    Redirect output to file: %d\n", redirect_output_to_file);
    printf("    No handle signals: %d\n", no_handle_signals);
    printf("    Continue when missing OpenGL context: %d\n", continue_when_no_gl_context);
    printf("\n");

    printf("DOOM3 callback\n");
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
    printf("  Other: \n");
    printf("    setState: %p\n", setState);
    printf("\n");

    Com_Printf("Quake2 command line arguments: %d\n", argc);
    for(int i = 0; i < argc; i++)
    {
        Com_Printf("  %d: %s\n", i, argv[i]);
    }
    printf("\n");
}

void Q3E_SetResolution(int width, int height)
{
    screen_width=width;
    screen_height=height;
}

extern void VID_Restart_f(void);
void Q3E_OGLRestart()
{
    VID_Restart_f();
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

    setState = ptr->set_state;
}

void (*Q3E_KeyEvent_p)(int state,int key,int character);
void (*Q3E_MotionEvent_p)(float dx, float dy);
void Q3E_KeyEvent(int state,int key,int character)
{
    (*Q3E_KeyEvent_p)(state,key,character);
}

void Q3E_MotionEvent(float dx, float dy)
{
    (*Q3E_MotionEvent_p)(dx,dy);
}

void Q3E_DrawFrame()
{
    main_frame();
}

void Q3E_Analog(int enable,float x,float y)
{
    analogenabled=enable;
    analogx=x;
    analogy=y;
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
    redirect_output_to_file = ptr->redirectOutputToFile ? true : false;
    no_handle_signals = ptr->noHandleSignals ? true : false;
    continue_when_no_gl_context = ptr->continueWhenNoGLContext ? true : false;
}

// View paused
void Q3E_OnPause(void)
{
    if(IsInitialized)
        paused = true;
}

// View resume
void Q3E_OnResume(void)
{
    if(IsInitialized)
        paused = false;
}

// because SurfaceView may be destroy or create new ANativeWindow in DOOM3 main thread
void Q3E_CheckNativeWindowChanged(void)
{
    if(window_changed)
    {
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
    Com_Printf("[Harmattan]: quake2 exit.\n");
}

// Setup OpenGL context variables in Android SurfaceView's thread
void Q3E_SetGLContext(ANativeWindow *w)
{
    // if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
    if(IsInitialized)
    {
        Com_Printf("[Harmattan]: ANativeWindow changed: %p\n", w);
        if(!w) // set window is null, and wait game main thread deactive OpenGL render context.
        {
            window = NULL;
            window_changed = true;
            while(window_changed)
                Sys_WaitForEvent(TRIGGER_EVENT_WINDOW_DESTROYED);
        }
        else // set new window, notify game main thread active OpenGL render context
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

extern int main(int argc, char **argv);
#pragma GCC visibility push(default)
void GetIDTechAPI(void *d3interface)
{
    Q3E_Interface_t *ptr = (Q3E_Interface_t *)d3interface;
    memset(ptr, 0, sizeof(*ptr));

    ptr->main = (int (*)(int, const char **))&main;
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

#ifndef _ANDROID_PACKAGE_NAME
//#define _ANDROID_PACKAGE_NAME "com.n0n3m4.DIII4A"
#define _ANDROID_PACKAGE_NAME "com.karin.idTech4Amm"
#endif
#define _ANDROID_DLL_PATH "/data/data/" _ANDROID_PACKAGE_NAME "/lib"
const char * Sys_DLLDefaultPath(void)
{
    return native_library_dir ? native_library_dir : _ANDROID_DLL_PATH;
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
