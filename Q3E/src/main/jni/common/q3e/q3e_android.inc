/* Android functions */

#ifdef Q3E_ANDROID_INC
#error "q3e_android.inc included many times!"
#endif
#define Q3E_ANDROID_INC

#if !defined(Q3E_GAME_NAME)
#error "Q3E_GAME_NAME must defined for as game name!"
#endif

#if !defined(Q3E_IS_INITIALIZED)
#error "Q3E_IS_INITIALIZED must defined for using check engine is initialized!"
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

#if !defined(Q3E_SHUTDOWN_GAME)
#warning "Q3E_SHUTDOWN_GAME defined for using shutdown game!"
#define Q3E_SHUTDOWN_GAME
#endif

#if !defined(Q3E_AT_EXIT)
#warning "Q3E_AT_EXIT defined for using atexit!"
#define Q3E_AT_EXIT
#endif

#if !defined(Q3E_PRE_START)
#warning "Q3E_PRE_START defined for before main start!"
#define Q3E_PRE_START
#endif

#if !defined(Q3E_POST_START)
#warning "Q3E_POST_START defined for after main start!"
#define Q3E_POST_START
#endif

#if !defined(Q3E_PRE_END)
#warning "Q3E_PRE_END defined for before main end!"
#define Q3E_PRE_END
#endif

#if !defined(Q3E_POST_END)
#warning "Q3E_POST_END defined for after main end!"
#define Q3E_POST_END
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
#define _ANDROID_DLL_PATH ""
#define _ANDROID_GAME_DATA_PATH "/sdcard/diii4a"
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



/* Global context variables */
// Surface window size
int screen_width = 640;
int screen_height = 480;

// Screen refresh rate
int refresh_rate = 60;

// max console height frac
float console_max_height_frac = -1.0f;

// Smooth joystick
Q3Ebool smooth_joystick = Q3E_FALSE;

// Using mouse
Q3Ebool mouse_available = Q3E_FALSE;

// Do not catch signal
Q3Ebool no_handle_signals = Q3E_FALSE;

// APK's native library path on Android.
char *native_library_dir = NULL;

// Game data directory.
char *game_data_dir = NULL;

// Application home directory.
char *app_home_dir = NULL;

// Enable redirect stdout/stderr to file
static Q3Ebool redirect_output_to_file = Q3E_TRUE;

// Using external libraries
Q3Ebool using_external_libs = Q3E_FALSE;

// Continue when missing OpenGL context
volatile Q3Ebool continue_when_no_gl_context = Q3E_FALSE;

// Surface window
volatile ANativeWindow *native_window = NULL;
static volatile Q3Ebool window_changed = Q3E_FALSE;

// App paused
volatile Q3Ebool q3e_paused = Q3E_FALSE;

// App exit
volatile Q3Ebool q3e_running = Q3E_FALSE;

// OpenGL color format
int gl_format = 0x8888;
// OpenGL depth bits
int gl_depth_bits = 24;
// OpenGL multisamples
int gl_msaa = 0;

// Analog parms
float analogx = 0.0f;
float analogy = 0.0f;
int analogenabled = 0;

// Game main thread
static pthread_t main_thread;



/* Include macro defines */
#include "q3e_thread.inc"
#if !defined(INIT_Q3E_THREADS)
#warning "INIT_Q3E_THREADS defined for using initialize threads!"
#define INIT_Q3E_THREADS
#endif

#if !defined(WAIT_CONTEXT_DESTROYED)
#error "WAIT_CONTEXT_DESTROYED must defined for using wait window destroyed!"
#endif

#if !defined(TRIGGER_CONTEXT_DESTROYED)
#error "TRIGGER_CONTEXT_DESTROYED must defined for using trigger window destroyed!"
#endif

#if !defined(WAIT_WINDOW_CREATED)
#error "WAIT_WINDOW_CREATED must defined for using wait window created!"
#endif

#if !defined(TRIGGER_WINDOW_CREATED)
#error "TRIGGER_WINDOW_CREATED must defined for using trigger window created!"
#endif

#if !defined(ENTER_WINDOW_CHANGED_CRITICAL_SECTION)
#error "ENTER_WINDOW_CHANGED_CRITICAL_SECTION must defined for using wait window created!"
#endif

#if !defined(LEAVE_WINDOW_CHANGED_CRITICAL_SECTION)
#error "LEAVE_WINDOW_CHANGED_CRITICAL_SECTION must defined for using trigger window created!"
#endif

#define NOTIFY_EXIT Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_MAIN);

// window changed
static Q3Ebool Q3E_WindowChanged(ANativeWindow *w, int fromNative)
{
	ENTER_WINDOW_CHANGED_CRITICAL_SECTION;

	Q3Ebool res = Q3E_FALSE;
	if(fromNative == WINDOW_CHANGE_THREAD_GAME) // native
	{
		if(window_changed)
		{
			Q3E_PRINTF("[Native]: ANativeWindow changed: %p.\n", native_window);
			if(native_window)
			{
				Q3E_CHANGE_WINDOW(native_window);
				Q3E_PRINTF("[Native]: Notify GL context created.\n");
				window_changed = Q3E_FALSE;
				TRIGGER_CONTEXT_CREATED;
			}
			else
			{
			    if(q3e_running)
			    {
                    Q3E_QUIT_WINDOW();
                    Q3E_PRINTF("[Native]: Notify GL context destroyed.\n");
                    window_changed = Q3E_FALSE;
                    TRIGGER_CONTEXT_DESTROYED;
                    res = Q3E_TRUE;
			    }
			    else
			    {
                    Q3E_PRINTF("[Native]: GL context has shutdown.\n");
                    window_changed = Q3E_FALSE;
			    }
			}
		}
	}
	else if(fromNative == WINDOW_CHANGE_THREAD_SURFACE_VIEW) // jni/java surface view
	{
		native_window = w;
		window_changed = Q3E_TRUE;
		if(native_window)
		{
			Q3E_PRINTF("[Java/JNI]: Notify ANativeWindow created.\n");
            TRIGGER_WINDOW_CREATED;
		}
		res = Q3E_TRUE;
	}
	else // jni/java main
	{
		Q3E_PRINTF("[Java/JNI]: Notify application destroyed.\n");
		native_window = NULL;
		window_changed = Q3E_TRUE;
		TRIGGER_WINDOW_CREATED;
		res = Q3E_TRUE;
	}

	LEAVE_WINDOW_CHANGED_CRITICAL_SECTION;

	return res;
}

// because SurfaceView may be destroy or create new ANativeWindow in DOOM3 main thread
Q3Ebool Q3E_CheckNativeWindowChanged(void)
{
	if(window_changed)
	{
		if(Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_GAME))
		{
#if 0
            if(continue_when_no_gl_context)
            {
                return Q3E_TRUE;
            }
#endif
            Q3E_PRINTF("[Native]: Waiting ANativeWindow create......\n");
            // wait new ANativeWindow created
            while(!window_changed)
            {
                WAIT_WINDOW_CREATED;
            }
            Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_GAME);
        }
        return native_window != NULL ? Q3E_TRUE : Q3E_FALSE;
	}
	else
        return Q3E_TRUE;
}

// Setup OpenGL context variables in Android SurfaceView's thread
void Q3E_SetGLContext(ANativeWindow *w)
{
    // if engine has started, w is null, means Surfece destroyed, w not null, means Surface has changed.
    if(Q3E_IS_INITIALIZED)
    {
        Q3E_PRINTF("[Harmattan]: ANativeWindow changed: %p\n", w);
        if(w) // set window is null, and wait game main thread deactive OpenGL render context.
        {
			Q3E_WindowChanged(w, WINDOW_CHANGE_THREAD_SURFACE_VIEW);
			Q3E_PRINTF("[Java/JNI]: Waiting GL context create......\n");
            while(window_changed)
            {
				WAIT_CONTEXT_CREATED;
            }
			Q3E_PRINTF("[Java/JNI]: GL context created.\n");
        }
        else // set new window, notify game main thread active OpenGL render context
        {
			Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_SURFACE_VIEW);
			Q3E_PRINTF("[Java/JNI]: Waiting GL context destroy......\n");
            while(window_changed)
            {
				WAIT_CONTEXT_DESTROYED;
            }
			Q3E_PRINTF("[Java/JNI]: GL context destroyed.\n");
        }
    }
    else
        native_window = w;
}

// JNI request thread quit
void Q3E_RequestThreadQuit(void)
{
    // if idTech4 main thread is waiting new window
    q3e_running = Q3E_FALSE;
	Q3E_WindowChanged(NULL, WINDOW_CHANGE_THREAD_MAIN);
}

volatile ANativeWindow * Android_GetWindow(void)
{
    return native_window;
}

// start/end in main function
static void game_exit(void)
{
    Q3E_AT_EXIT;
	Q3E_PRINTF("[Harmattan]: " Q3E_GAME_NAME " exit.\n");
}

void Q3E_Start(void)
{
    Q3E_PRINTF("[Harmattan]: Enter " Q3E_GAME_NAME " main thread.\n");

    INIT_Q3E_THREADS;
	atexit(game_exit);

    Q3E_PRE_START;
    main_thread = pthread_self();
    q3e_running = Q3E_TRUE;

    Q3E_INIT_WINDOW(native_window);

    Q3E_POST_START;
}

void Q3E_End(void)
{
    q3e_running = Q3E_FALSE;

    Q3E_PRE_END;

    Q3E_QUIT_WINDOW();
    native_window = NULL;
    main_thread = 0;
    
    Q3E_POST_END;

    Q3E_PRINTF("[Harmattan]: Leave " Q3E_GAME_NAME " main thread.\n");
}



/* Functions of wrap JNI callback */
void Android_GrabMouseCursor(Q3Ebool grabIt)
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

void Android_EnableSmoothJoystick(Q3Ebool enable)
{
    //if(smooth_joystick != enable)
    {
        setup_smooth_joystick(enable);
        smooth_joystick = enable;
    }
}

FILE * Sys_tmpfile(void)
{
    FILE *f = /*itmpfile ? */itmpfile()/* : NULL*/;
    if (!f) {
        Q3E_PRINTF("JNI::tmpfile failed: %s", strerror(errno));
    }
    return f;
}

void Android_SetClipboardData(const char *text)
{
    if(text)
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

float Android_GetConsoleMaxHeightFrac(float frac)
{
    return console_max_height_frac > 0.0f && console_max_height_frac < frac ? console_max_height_frac : frac;
}

int Android_GetConsoleMaxHeight(int height, int maxHeight)
{
    if(console_max_height_frac <= 0.0f)
        return height;
    int h = console_max_height_frac * (float)maxHeight;
    return h < height ? h : height;
}

const char * Sys_DLLInternalPath(void)
{
    return native_library_dir ? native_library_dir : _ANDROID_DLL_PATH;
}

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

char * Sys_MakeDLLPath(const char *libname, char path[], int max_length)
{
#undef snprintf
#define HARM_MAX_OSPATH 1024
	char dllName[HARM_MAX_OSPATH];
	memset(dllName, 0, HARM_MAX_OSPATH);
	size_t libnameLength = strlen(libname);
	
	if(libnameLength >= 3 && (libname[0] != 'l' || libname[1] != 'i' || libname[2] != 'b'))
		snprintf(dllName, HARM_MAX_OSPATH - 1, "lib");
	snprintf(dllName + strlen(dllName), HARM_MAX_OSPATH - 1 - strlen(dllName), "%s", libname);

	if(libnameLength >= 3 && (libname[libnameLength - 3] != '.' || libname[libnameLength - 2] != 's' || libname[libnameLength - 1] != 'o'))
		snprintf(dllName + strlen(dllName), HARM_MAX_OSPATH - 1 - strlen(dllName), ".so");
	
	memset(path, 0, max_length);
	const char * dllDefaultPath = Sys_DLLDefaultPath();
	if(dllDefaultPath && dllDefaultPath[0])
		snprintf(path, max_length - 1, "%s/%s", dllDefaultPath, dllName);
	else
		snprintf(path, max_length - 1, "%s", dllName);
	Q3E_PRINTF("[Harmattan]: Make library path -> %s\n", path);
	return path;
#undef HARM_MAX_OSPATH
}



/* Functions of JNI call */
void Q3E_Analog(int enable, float x, float y)
{
    analogenabled = enable;
    analogx = x;
    analogy = y;
}

// View paused
void Q3E_OnPause(void)
{
    if(Q3E_IS_INITIALIZED)
        q3e_paused = Q3E_TRUE;
}

// View resume
void Q3E_OnResume(void)
{
    if(Q3E_IS_INITIALIZED)
        q3e_paused = Q3E_FALSE;
}

// Request exit
void Q3E_exit(void)
{
    q3e_running = Q3E_FALSE;
    Q3E_SHUTDOWN_GAME;
    if(native_window)
        native_window = NULL;
    Q3E_QUIT_WINDOW();
    main_thread = 0;
    Q3E_PRINTF("[Harmattan]: " Q3E_GAME_NAME " exit.\n");
    // Android_ExitFinish();
}

int Q3E_Main(int argc, char **argv)
{
    Q3E_Start();

    int res = main(argc, argv);

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

    native_library_dir = strdup(ptr->nativeLibraryDir ? ptr->nativeLibraryDir : "");
    game_data_dir = strdup(ptr->gameDataDir ? ptr->gameDataDir : "");
    app_home_dir = strdup(ptr->appHomeDir ? ptr->appHomeDir : "");
    redirect_output_to_file = ptr->redirectOutputToFile ? Q3E_TRUE : Q3E_FALSE;
    no_handle_signals = ptr->noHandleSignals ? Q3E_TRUE : Q3E_FALSE;
    continue_when_no_gl_context = ptr->continueWhenNoGLContext ? Q3E_TRUE : Q3E_FALSE;
    mouse_available = ptr->mouseAvailable ? Q3E_TRUE : Q3E_FALSE;
    refresh_rate = ptr->refreshRate <= 0 ? 60 : ptr->refreshRate;
    smooth_joystick = ptr->smoothJoystick ? Q3E_TRUE : Q3E_FALSE;
    console_max_height_frac = ptr->consoleMaxHeightFrac > 0 && ptr->consoleMaxHeightFrac < 100 ? (float)ptr->consoleMaxHeightFrac / 100.0f : -1.0f;
    using_external_libs = ptr->usingExternalLibs ? Q3E_TRUE : Q3E_FALSE;

    native_window = ptr->window;
    screen_width = ptr->width;
    screen_height = ptr->height;
}

#ifdef __cplusplus
extern "C" {
#endif
#pragma GCC visibility push(default)
void GetIDTechAPI(void *d3interface) {
    Q3E_Interface_t *ptr = (Q3E_Interface_t *) d3interface;
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
#ifdef __cplusplus
}
#endif