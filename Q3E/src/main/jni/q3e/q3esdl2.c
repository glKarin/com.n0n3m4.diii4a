#include <jni.h>

#include "q3esdl2.h"

#include <dlfcn.h>

#include <android/log.h>
#include <android/keycodes.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "q3estd.h"
#include "q3eenv.h"

#include "../deplibs/SDL2/src/core/android/SDL_android_env.h"
#include "../deplibs/SDL2/include/SDL.h"

#define LOG_TAG "Q3E::SDL"

#define Q3E_SDL_FAKE_CUSTOM_CURSOR 1
#define SDL_HINT_EGL_Q3E_SPECIAL_CONFIG "SDL_HINT_EGL_Q3E_SPECIAL_CONFIG"

#define GET_JNI(env) JNIEnv *env = Q3E_GetEnv();

extern char * get_clipboard_text(void);
extern jobject q3eCallbackObj;
extern ANativeWindow *window;
extern void copy_to_clipboard(const char *text);
extern void *libdl;
extern void Android_AttachThread(void);
extern void Android_DetachThread(void);
extern void open_url(const char *url);
extern void show_toast(const char *text);
extern void grab_mouse(int grab);
extern int open_dialog(const char *title, const char *message, int num, const char *buttons[]);

SDL_Android_GetAPI_f sdl_api;
int USING_SDL = 0;
static char *sdl_clipboardText = NULL;
static SDL_bool (*SDL_SetHint_f)(const char *name, const char *value);

/** Standard activity result: operation canceled. */
#define RESULT_CANCELED     0
/** Standard activity result: operation succeeded. */
#define RESULT_OK            -1
/** Start of user-defined activity results. */
#define RESULT_FIRST_USER    1

#define INTERFACE_METHOD(ret, name, args) ret (* name) args;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

static jmethodID android_RequestPermission_method;

/* controller manager */
static jclass mControllerManagerClass;

/* method signatures */
static jmethodID midPollInputDevices;
//static jmethodID midPollHapticDevices;
//static jmethodID midHapticRun;
//static jmethodID midHapticStop;

void Q3E_InitSDLJava(JNIEnv *env, jclass q3eCallbackClass)
{
    android_RequestPermission_method = (*env)->GetMethodID(env, q3eCallbackClass, "RequestPermission", "(Ljava/lang/String;I)Z");

    jclass SDLControllerManager = (*env)->FindClass(env, "com/n0n3m4/q3e/sdl/SDLControllerManager");
    mControllerManagerClass = (jclass)((*env)->NewGlobalRef(env, SDLControllerManager));

    midPollInputDevices = (*env)->GetStaticMethodID(env, mControllerManagerClass,
                                                    "pollInputDevices", "()V");
//    midPollHapticDevices = (*env)->GetStaticMethodID(env, mControllerManagerClass,
//                                                     "pollHapticDevices", "()V");
//    midHapticRun = (*env)->GetStaticMethodID(env, mControllerManagerClass,
//                                             "hapticRun", "(IFI)V");
//    midHapticStop = (*env)->GetStaticMethodID(env, mControllerManagerClass,
//                                              "hapticStop", "(I)V");
}

// from Java
static int request_permission(const char *perm, int code)
{
    GET_JNI(env)

    if(!perm)
        return Q3E_FALSE;

    LOGI("Request permission: %s(%d)", perm, code);
    jstring str = (*env)->NewStringUTF(env, perm);
    jstring nstr = (*env)->NewWeakGlobalRef(env, str);
    (*env)->DeleteLocalRef(env, str);
    jboolean res = (*env)->CallBooleanMethod(env, q3eCallbackObj, android_RequestPermission_method, nstr, code);
    return res ? Q3E_TRUE : Q3E_FALSE;
}

// from SDL2
static void UnicodeToUtf8( int w , char *utf8buf)
{
    unsigned char *utf8s = (unsigned char *) utf8buf;

    if ( w < 0x0080 ) {
        utf8s[0] = ( unsigned char ) w;
        utf8s[1] = 0;
    }
    else if ( w < 0x0800 ) {
        utf8s[0] = 0xc0 | (( w ) >> 6 );
        utf8s[1] = 0x80 | (( w ) & 0x3f );
        utf8s[2] = 0;
    }
    else {
        utf8s[0] = 0xe0 | (( w ) >> 12 );
        utf8s[1] = 0x80 | (( ( w ) >> 6 ) & 0x3f );
        utf8s[2] = 0x80 | (( w ) & 0x3f );
        utf8s[3] = 0;
    }
}

const char * clipboardGetText()
{
    if(sdl_clipboardText)
    {
        free(sdl_clipboardText);
        sdl_clipboardText = NULL;
    }
    sdl_clipboardText = get_clipboard_text();
    return sdl_clipboardText;
}

int clipboardHasText()
{
    char *text = get_clipboard_text();
    int res = text && text[0];
    if(text)
        free(text);
    return res;
}

void clipboardSetText(const char *text)
{
    copy_to_clipboard(text);
}

static int mLastCursorID = 0;
int createCustomCursor(int *pixels, int w, int h, int x, int y)
{
    LOGW("Q3E SDL createCustomCursor(%p, %d, %d, %d, %d) -> not supported!", pixels, w, h, x, y);
#ifdef Q3E_SDL_FAKE_CUSTOM_CURSOR
    return ++mLastCursorID;
#else
    return 0;
#endif
}

void destroyCustomCursor(int id)
{
    LOGW("Q3E SDL destroyCustomCursor(%d) -> not supported!", id);
}

void * getContext()
{
    LOGE("Q3E SDL getContext() -> not supported!");
    return NULL;
}

void * getDisplayDPI()
{
    LOGE("Q3E SDL getDisplayDPI() -> not supported!");
    return NULL;
}

void * getNativeSurface()
{
    return window;
}

void initTouch()
{
    LOGI("Q3E SDL initTouch() -> not supported!");
}

int isAndroidTV()
{
    return Q3E_FALSE;
}

int isChromebook()
{
    return Q3E_FALSE;
}

int isDeXMode()
{
    return Q3E_FALSE;
}

int isScreenKeyboardShown()
{
    return Q3E_FALSE;
}

int isTablet()
{
    return Q3E_FALSE;
}

void manualBackButton()
{
    LOGW("Q3E SDL manualBackButton() -> not supported!");
}

void minimizeWindow()
{
    LOGW("Q3E SDL minimizeWindow() -> not supported!");
}

int openURL(const char *url)
{
    open_url(url);
    return Q3E_TRUE;
}

void requestPermission(const char *permission, int requestCode)
{
    int res = request_permission(permission, requestCode);
    LOGI("Q3E SDL request permission '%s' with request code '%d', result = %d", permission, requestCode, res);
    CALL_SDL(nativePermissionResult, requestCode, res);
}

int showToast(const char *message, int duration, int gravity, int xOffset, int yOffset)
{
    show_toast(message);
    return Q3E_TRUE;
}

int sendMessage(int command, int param)
{
    return Q3E_FALSE;
}

int setActivityTitle(const char *title)
{
    LOGW("Q3E SDL setActivityTitle(\"%s\") -> not supported!", title);
    return Q3E_FALSE;
}

int setCustomCursor(int cursorID)
{
    LOGW("Q3E SDL setCustomCursor(%d) -> not supported!", cursorID);
#ifdef Q3E_SDL_FAKE_CUSTOM_CURSOR
    return Q3E_TRUE;
#else
    return Q3E_FALSE;
#endif
}

void setWindowStyle(int fullscreen)
{
    LOGW("Q3E SDL setWindowStyle(%d) -> not supported!", fullscreen);
}

int shouldMinimizeOnFocusLoss()
{
    return Q3E_FALSE;
}

int showTextInput(int x, int y, int w, int h)
{
    LOGW("Q3E SDL showTextInput(%d, %d, %d, %d) -> not supported!", x, y, w, h);
    return Q3E_FALSE;
}

int supportsRelativeMouse()
{
    return Q3E_TRUE;
}

int setSystemCursor(int cursorID)
{
    LOGW("Q3E SDL setSystemCursor(%d) -> not supported!", cursorID);
#ifdef Q3E_SDL_FAKE_CUSTOM_CURSOR
    return Q3E_TRUE;
#else
    return Q3E_FALSE;
#endif
}

void setOrientation(int w, int h, int resizable, const char *hint)
{
    LOGW("Q3E SDL setOrientation(%d, %d, %d, \"%s\") -> not supported!", w, h, resizable, hint);
}

int setRelativeMouseEnabled(int on)
{
    grab_mouse(on);
    return Q3E_TRUE;
}

int getManifestEnvironmentVariables()
{
    return Q3E_FALSE;
}

int * getAudioOutputDevices()
{
    LOGW("Q3E SDL getAudioOutputDevices() -> not supported!");
    return NULL;
}

int * getAudioInputDevices()
{
    LOGW("Q3E SDL getAudioInputDevices() -> not supported!");
    return NULL;
}

int * audioOpen(int freq, int audioformat, int channels, int samples, int device_id)
{
    LOGW("Q3E SDL audioOpen(%d, %d, %d, %d, %d) -> not supported!", freq, audioformat, channels, samples, device_id);
    return NULL;
}

void audioWriteByteBuffer(char *buf)
{
    
}

void audioWriteShortBuffer(short *buf)
{
    
}

void audioWriteFloatBuffer(float *buf)
{
    
}

void audioClose()
{
    LOGW("Q3E SDL audioClose() -> not supported!");
}

int * captureOpen(int freq, int audioformat, int channels, int samples, int device_id)
{
    LOGW("Q3E SDL captureOpen(%d, %d, %d, %d, %d) -> not supported!", freq, audioformat, channels, samples, device_id);
    return NULL;
}

int captureReadByteBuffer(char *buf, int b)
{
    return 0;
}

int captureReadShortBuffer(short *buf, int b)
{
    return 0;
}

int captureReadFloatBuffer(float *buf, int b)
{
    return 0;
}

void captureClose()
{
    LOGW("Q3E SDL captureClose() -> not supported!");
}

void audioSetThreadPriority(int iscapture, int device_id)
{
    //LOGW("Q3E SDL audioSetThreadPriority(%d, %d) -> not supported!", iscapture, device_id);

    GET_JNI(env)
    int res = 0;

    jclass Thread = (*env)->FindClass(env, "java/lang/Thread");
    jmethodID currentThread = (*env)->GetStaticMethodID(env, Thread, "currentThread", "()Ljava/lang/Thread;");
    jmethodID setName = (*env)->GetMethodID(env, Thread, "setName", "(Ljava/lang/String;)V");
    jmethodID getId = (*env)->GetMethodID(env, Thread, "getId", "()J");
    jobject thread;
    jclass Process = (*env)->FindClass(env, "android/os/Process");
    jmethodID setThreadPriority = (*env)->GetStaticMethodID(env, Process, "setThreadPriority", "(I)V");
    int THREAD_PRIORITY_AUDIO = -16;
    jfieldID THREAD_PRIORITY_AUDIO_field = (*env)->GetStaticFieldID(env, Process, "THREAD_PRIORITY_AUDIO", "I");
    if(THREAD_PRIORITY_AUDIO_field)
    {
        THREAD_PRIORITY_AUDIO = (*env)->GetStaticIntField(env, Process, THREAD_PRIORITY_AUDIO_field);
    }
    jlong thread_id;

    //try {

        thread = (*env)->CallStaticObjectMethod(env, Thread, currentThread);
        char name[9 + 32 + 1] = { 0 };
        /* Set thread name */
        if (iscapture) {
            //Thread.currentThread().setName("SDLAudioC" + device_id);
            snprintf(name, sizeof(name), "SDLAudioC%d", device_id);
        } else {
            //Thread.currentThread().setName("SDLAudioP" + device_id);
            snprintf(name, sizeof(name), "SDLAudioP%d", device_id);
        }
        jstring str = (*env)->NewStringUTF(env, name);
        (*env)->CallVoidMethod(env, thread, setName, str);
        (*env)->DeleteLocalRef(env, str);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("modify thread properties failed Thread.setThread");
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            goto _Exit;
        }

        /* Set thread priority */
        //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
        (*env)->CallStaticVoidMethod(env, Process, setThreadPriority, THREAD_PRIORITY_AUDIO);

   // } catch (Exception e) {
        if ((*env)->ExceptionCheck(env)) {
            LOGE("modify thread properties failed ");
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            goto _Exit;
        }
    //}

    res = 1;

    //LOGW("Q3E SDL audioSetThreadPriority(%d, %d) -> not supported!", iscapture, device_id);

_Exit:
    thread_id = (*env)->CallLongMethod(env, thread, getId);
    LOGW("Q3E SDL audioSetThreadPriority(%d, %d): thread name=%s(%lld), priority=%d -> %d", iscapture, device_id, name, (long long)thread_id, THREAD_PRIORITY_AUDIO, res);
}

void pollInputDevices()
{
    GET_JNI(env)
    (*env)->CallStaticVoidMethod(env, mControllerManagerClass, midPollInputDevices);
}

void pollHapticDevices()
{

}

void hapticRun(int device_id, float intensity, int length)
{
    
}

void hapticStop(int device_id)
{
    
}

int messageboxShowMessageBox(int flags, const char *title, const char *message, int *button_flags, int *button_ids,  const char *button_texts[], int *colors)
{
    int num = ARRAY_ELEMENT_LENGTH(button_texts, const char *);
    int res = open_dialog(title, message, num, button_texts);
    switch(res)
    {
        case 1:
            if(num > 0)
                return button_ids[0];
        case 2:
            if(num > 1)
                return button_ids[1];
        case 3:
            if(num > 2)
                return button_ids[2];
        default: // -1 0
            return res;
    }
    return -2; // unknown
}

void attachCurrentThread()
{
    Android_AttachThread();
}

void detachCurrentThread()
{
    Android_DetachThread();
}

void Q3E_ShutdownSDL(void)
{
    if(sdl_clipboardText)
    {
        free(sdl_clipboardText);
        sdl_clipboardText = NULL;
    }
    SDL_SetHint_f = NULL;
    sdl_api = NULL;
    LOGI("Q3E SDL2 shutdown");
}

void Q3E_SDL_SetHint(const char *name, const char *value)
{
    if(!SDL_SetHint_f)
    {
        LOGW("Q3E SDL2 not initialized: SDL_SetHint missing");
        return;
    }

    LOGI("Q3E SDL_SetHint('%s', '%s')", name, value);
    SDL_SetHint_f(name, value);
}

void Q3E_SDL_SetAudioDriver(int driver)
{
    if(!SDL_SetHint_f)
    {
        LOGW("Q3E SDL2 not initialized: SDL_SetHint missing");
        return;
    }
    switch (driver) {
        case Q3E_SDL_AUDIO_DRIVER_OPENSLES:
            LOGI("Q3E SDL2 using 'opensles' as audio driver");
            Q3E_SDL_SetHint(SDL_HINT_AUDIODRIVER, "opensles");
            break;
        case Q3E_SDL_AUDIO_DRIVER_AAUDIO:
            LOGI("Q3E SDL2 using 'aaudio' as audio driver");
            Q3E_SDL_SetHint(SDL_HINT_AUDIODRIVER, "aaudio");
            break;
        default:
            LOGI("Q3E SDL2 using default audio driver");
            break;
    }
}

void Q3E_InitSDL(void)
{
    if(!libdl)
    {
        LOGW("Please load game library first");
        return;
    }

    if(sdl_api)
    {
        LOGW("Q3E SDL2 has initialized");
        return;
    }

    void *ptr = dlsym(libdl, "SDL_Android_GetAPI");
    if(!ptr)
    {
        LOGI("Game library not use SDL2");
        return;
    }

    LOGI("SDL2 API found!");

    sdl_api = (SDL_Android_GetAPI_f)ptr;
    SDL_Android_Callback_t callbacks;
#define CALLBACK_METHOD(ret, name, args) callbacks.name = name;
#include "../deplibs/SDL2/src/core/android/SDL_android_callback.h"
            
    SDL_Android_Interface_t interfaces;

    sdl_api(&callbacks, &interfaces);

#define INTERFACE_METHOD(ret, name, args) name = interfaces.name;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

    CALL_SDL(nativeSetupJNI);
    CALL_SDL(audio_nativeSetupJNI);
    CALL_SDL(controller_nativeSetupJNI);

    GET_JNI(env)
    jclass Q3ESDL = (*env)->FindClass(env, "com/n0n3m4/q3e/sdl/Q3ESDL");
    jmethodID InitSDL = (*env)->GetStaticMethodID(env, Q3ESDL, "InitSDL", "()V");
    (*env)->CallStaticObjectMethod(env, Q3ESDL, InitSDL);

    atexit(Q3E_ShutdownSDL);

    void *SDL_SetHint = dlsym(libdl, "SDL_SetHint");
    SDL_SetHint_f = (SDL_bool (*)(const char *name, const char *value))SDL_SetHint;

    //Q3E_SDL_SetHint(SDL_HINT_EGL_Q3E_SPECIAL_CONFIG, "1");

    USING_SDL = 1;
    LOGI("Game library SDL2 initialized!");
}

#include "q3esdljni.c"
