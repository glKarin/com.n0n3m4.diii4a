/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#include "SDL_stdinc.h"
#include "SDL_atomic.h"
#include "SDL_hints.h"
#include "SDL_main.h"
#include "SDL_timer.h"
#include "SDL_version.h"

#ifdef __ANDROID__

#include "SDL_system.h"
#include "SDL_android.h"

#include "../../events/SDL_events_c.h"
#include "../../video/android/SDL_androidkeyboard.h"
#include "../../video/android/SDL_androidmouse.h"
#include "../../video/android/SDL_androidtouch.h"
#include "../../video/android/SDL_androidvideo.h"
#include "../../video/android/SDL_androidwindow.h"
#include "../../joystick/android/SDL_sysjoystick_c.h"
#include "../../haptic/android/SDL_syshaptic_c.h"

#include <android/log.h>
#include <android/configuration.h>
#include <android/asset_manager_jni.h>
#include <sys/system_properties.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>

#define INTERFACE_METHOD(ret, name, args) ret sdl_##name args;
#include "SDL_android_interface.h"

#include "SDL_android_env.h"

static SDL_INLINE void * sdl_array_alloc(int size)
{
    if(size == 0)
        return NULL;
    ARRAY_ALLOC(res, char, size);
    return res;
}

static SDL_INLINE void * sdl_array_allocs(int size, int num)
{
    return sdl_array_alloc(size * num);
}

static SDL_INLINE int sdl_array_length(void *arr)
{
    if(!arr)
        return 0;
    return ARRAY_LENGTH(arr);
}

static SDL_INLINE void sdl_array_free(void *arr)
{
    if(!arr)
        return;
    return ARRAY_FREE(arr);
}

/* Audio encoding definitions */
#define ENCODING_PCM_8BIT  3
#define ENCODING_PCM_16BIT 2
#define ENCODING_PCM_FLOAT 4

/* Uncomment this to log messages entering and exiting methods in this file */
/* #define DEBUG_JNI */

static void checkJNIReady(void);

/*******************************************************************************
 This file links the Java side of Android with libsdl
*******************************************************************************/
#include <jni.h>

/*******************************************************************************
                               Globals
*******************************************************************************/
static pthread_key_t mThreadKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

/* Main activity */

/* method signatures */
#define CALLBACK_METHOD(ret, name, args) ret (* mid##name) args
#include "SDL_android_callback.h"

#define CALLM(name, ...) mid##name(__VA_ARGS__)
#define METHOD(name) mid##name

__attribute__((visibility("default"))) void SDL_Android_GetAPI(const SDL_Android_Callback_t *callbacks, SDL_Android_Interface_t *interfaces)
{
#define CALLBACK_METHOD(ret, name, args) mid##name = callbacks->name;
#include "SDL_android_callback.h"

#define INTERFACE_METHOD(ret, name, args) interfaces->name = sdl_##name;
#include "SDL_android_interface.h"
}

#define SDL_JAVA_INTERFACE(function) sdl_##function
#define SDL_JAVA_AUDIO_INTERFACE(function) sdl_audio_##function
#define SDL_JAVA_CONTROLLER_INTERFACE(function) sdl_controller_##function
#define SDL_JAVA_INTERFACE_INPUT_CONNECTION(function) sdl_connection_##function

/* audio manager */
/* method signatures */

/* controller manager */
/* method signatures */

/* Accelerometer data storage */
static SDL_DisplayOrientation displayOrientation;
static float fLastAccelerometer[3];
static SDL_bool bHasNewData;

static SDL_bool bHasEnvironmentVariables;

static SDL_atomic_t bPermissionRequestPending;
static SDL_bool bPermissionRequestResult;

/* Android AssetManager */
static AAssetManager *asset_manager = NULL;
static jobject javaAssetManagerRef = 0;

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

/* From http://developer.android.com/guide/practices/jni.html
 * All threads are Linux threads, scheduled by the kernel.
 * They're usually started from managed code (using Thread.start), but they can also be created elsewhere and then
 * attached to the JavaVM. For example, a thread started with pthread_create can be attached with the
 * JNI AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread is attached, it has no JNIEnv,
 * and cannot make JNI calls.
 * Attaching a natively-created thread causes a java.lang.Thread object to be constructed and added to the "main"
 * ThreadGroup, making it visible to the debugger. Calling AttachCurrentThread on an already-attached thread
 * is a no-op.
 * Note: You can call this function any number of times for the same thread, there's no harm in it
 */

/* From http://developer.android.com/guide/practices/jni.html
 * Threads attached through JNI must call DetachCurrentThread before they exit. If coding this directly is awkward,
 * in Android 2.0 (Eclair) and higher you can use pthread_key_create to define a destructor function that will be
 * called before the thread exits, and call DetachCurrentThread from there. (Use that key with pthread_setspecific
 * to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.)
 * Note: The destructor is not called unless the stored value is != NULL
 * Note: You can call this function any number of times for the same thread, there's no harm in it
 *       (except for some lost CPU cycles)
 */

/* Set local storage value */
static int Android_JNI_SetEnv(JNIEnv *env)
{
    int status = pthread_setspecific(mThreadKey, env);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "Failed pthread_setspecific() in Android_JNI_SetEnv() (err=%d)", status);
    }
    return status;
}

/* Get local storage value */
JNIEnv *Android_JNI_GetEnv(void)
{
    /* Get JNIEnv from the Thread local storage */
    JNIEnv *env = pthread_getspecific(mThreadKey);
    if (!env) {
        /* Attach the current thread to the JVM and get a JNIEnv.
         * It will be detached by pthread_create destructor 'Android_JNI_ThreadDestroyed' */
        CALLM(attachCurrentThread);

        /* Save JNIEnv into the Thread local storage */
        if (Android_JNI_SetEnv(env) < 0) {
            return NULL;
        }
    }

    return env;
}

/* Set up an external thread for using JNI with Android_JNI_GetEnv() */
int Android_JNI_SetupThread(void)
{
    CALLM(attachCurrentThread);

    return 1;
}

/* Destructor called for each thread where mThreadKey is not NULL */
static void Android_JNI_ThreadDestroyed(void *value)
{
    /* The thread is being destroyed, detach it from the Java VM and set the mThreadKey value to NULL as required */
    CALLM(detachCurrentThread);
}

/* Creation of local storage mThreadKey */
static void Android_JNI_CreateKey(void)
{
    int status = pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "Error initializing mThreadKey with pthread_key_create() (err=%d)", status);
    }
}

static void Android_JNI_CreateKey_once(void)
{
    int status = pthread_once(&key_once, Android_JNI_CreateKey);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "Error initializing mThreadKey with pthread_once() (err=%d)", status);
    }
}

static void register_methods(JNIEnv *env, const char *classname, JNINativeMethod *methods, int nb)
{
    jclass clazz = (*env)->FindClass(env, classname);
    if (!clazz || (*env)->RegisterNatives(env, clazz, methods, nb) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "Failed to register methods of %s", classname);
        return;
    }
}

/* Library init */
JNIEXPORT int JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    return 0;
}


void checkJNIReady(void)
{
#if 0
    if (!mActivityClass || !mAudioManagerClass || !mControllerManagerClass) {
        /* We aren't fully initialized, let's just return. */
        return;
    }
#endif

    SDL_SetMainReady();
}

/* Get SDL version -- called before SDL_main() to verify JNI bindings */
JNIEXPORT const char * JNICALL SDL_JAVA_INTERFACE(nativeGetVersion)()
{
    static char version[128];

    SDL_snprintf(version, sizeof(version), "%d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);

    return version;
}

/* Activity initialization -- called before SDL_main() to initialize JNI bindings */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeSetupJNI)()
{
    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "nativeSetupJNI()");

    /*
     * Create mThreadKey so we can keep track of the JNIEnv assigned to each thread
     * Refer to http://developer.android.com/guide/practices/design/jni.html for the rationale behind this
     */
    Android_JNI_CreateKey_once();

    /* Save JNIEnv of SDLActivity */
    //Android_JNI_SetEnv(env);

    /* Use a mutex to prevent concurrency issues between Java Activity and Native thread code, when using 'Android_Window'.
     * (Eg. Java sending Touch events, while native code is destroying the main SDL_Window. )
     */
    if (!Android_ActivityMutex) {
        Android_ActivityMutex = SDL_CreateMutex(); /* Could this be created twice if onCreate() is called a second time ? */
    }

    if (!Android_ActivityMutex) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "failed to create Android_ActivityMutex mutex");
    }

    Android_PauseSem = SDL_CreateSemaphore(0);
    if (!Android_PauseSem) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "failed to create Android_PauseSem semaphore");
    }

    Android_ResumeSem = SDL_CreateSemaphore(0);
    if (!Android_ResumeSem) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "failed to create Android_ResumeSem semaphore");
    }

#if 0
    if (!midClipboardGetText ||
        !midClipboardHasText ||
        !midClipboardSetText ||
        !midCreateCustomCursor ||
        !midDestroyCustomCursor ||
        !midGetContext ||
        !midGetDisplayDPI ||
        !midGetManifestEnvironmentVariables ||
        !midGetNativeSurface ||
        !midInitTouch ||
        !midIsAndroidTV ||
        !midIsChromebook ||
        !midIsDeXMode ||
        !midIsScreenKeyboardShown ||
        !midIsTablet ||
        !midManualBackButton ||
        !midMinimizeWindow ||
        !midOpenURL ||
        !midRequestPermission ||
        !midShowToast ||
        !midSendMessage ||
        !midSetActivityTitle ||
        !midSetCustomCursor ||
        !midSetOrientation ||
        !midSetRelativeMouseEnabled ||
        !midSetSystemCursor ||
        !midSetWindowStyle ||
        !midShouldMinimizeOnFocusLoss ||
        !midShowTextInput ||
        !midSupportsRelativeMouse) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "Missing some Java callbacks, do you have the latest version of SDLActivity.java?");
    }
#endif

    checkJNIReady();
}

/* Audio initialization -- called before SDL_main() to initialize JNI bindings */
JNIEXPORT void JNICALL SDL_JAVA_AUDIO_INTERFACE(nativeSetupJNI)()
{
    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "AUDIO nativeSetupJNI()");

#if 0
    if (!midGetAudioOutputDevices || !midGetAudioInputDevices || !midAudioOpen ||
        !midAudioWriteByteBuffer || !midAudioWriteShortBuffer || !midAudioWriteFloatBuffer ||
        !midAudioClose ||
        !midCaptureOpen || !midCaptureReadByteBuffer || !midCaptureReadShortBuffer ||
        !midCaptureReadFloatBuffer || !midCaptureClose || !midAudioSetThreadPriority) {
        __android_log_print(ANDROID_LOG_WARN, "SDL",
                            "Missing some Java callbacks, do you have the latest version of SDLAudioManager.java?");
    }
#endif

    checkJNIReady();
}

/* Controller initialization -- called before SDL_main() to initialize JNI bindings */
JNIEXPORT void JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeSetupJNI)()
{
    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "CONTROLLER nativeSetupJNI()");

#if 0
    if (!midPollInputDevices || !midPollHapticDevices || !midHapticRun || !midHapticStop) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "Missing some Java callbacks, do you have the latest version of SDLControllerManager.java?");
    }
#endif

    checkJNIReady();
}

/* SDL main function prototype */
typedef int (*SDL_main_func)(int argc, char *argv[]);

/* Start up the SDL app */
JNIEXPORT int JNICALL SDL_JAVA_INTERFACE(nativeRunMain)( const char * library, const char * function, const char **array)
{
#if 0
    int status = -1;
    const char *library_file;
    void *library_handle;

    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "nativeRunMain()");

    /* Save JNIEnv of SDLThread */
    Android_JNI_SetEnv(env);

    library_file = (*env)->GetStringUTFChars(env, library, NULL);
    library_handle = dlopen(library_file, RTLD_GLOBAL);

    if (library_handle == NULL) {
        /* When deploying android app bundle format uncompressed native libs may not extract from apk to filesystem.
           In this case we should use lib name without path. https://bugzilla.libsdl.org/show_bug.cgi?id=4739 */
        const char *library_name = SDL_strrchr(library_file, '/');
        if (library_name && *library_name) {
            library_name += 1;
            library_handle = dlopen(library_name, RTLD_GLOBAL);
        }
    }

    if (library_handle) {
        const char *function_name;
        SDL_main_func SDL_main;

        function_name = (*env)->GetStringUTFChars(env, function, NULL);
        SDL_main = (SDL_main_func)dlsym(library_handle, function_name);
        if (SDL_main) {
            int i;
            int argc;
            int len;
            char **argv;
            SDL_bool isstack;

            /* Prepare the arguments. */
            len = (*env)->GetArrayLength(env, array);
            argv = SDL_small_alloc(char *, 1 + len + 1, &isstack); /* !!! FIXME: check for NULL */
            argc = 0;
            /* Use the name "app_process" so PHYSFS_platformCalcBaseDir() works.
               https://bitbucket.org/MartinFelis/love-android-sdl2/issue/23/release-build-crash-on-start
             */
            argv[argc++] = SDL_strdup("app_process");
            for (i = 0; i < len; ++i) {
                const char *utf;
                char *arg = NULL;
                const char * string = (*env)->GetObjectArrayElement(env, array, i);
                if (string) {
                    utf = (*env)->GetStringUTFChars(env, string, 0);
                    if (utf) {
                        arg = SDL_strdup(utf);
                        (*env)->ReleaseStringUTFChars(env, string, utf);
                    }
                    (*env)->DeleteLocalRef(env, string);
                }
                if (arg == NULL) {
                    arg = SDL_strdup("");
                }
                argv[argc++] = arg;
            }
            argv[argc] = NULL;

            /* Run the application. */
            status = SDL_main(argc, argv);

            /* Release the arguments. */
            for (i = 0; i < argc; ++i) {
                SDL_free(argv[i]);
            }
            SDL_small_free(argv, isstack);

        } else {
            __android_log_print(ANDROID_LOG_ERROR, "SDL", "nativeRunMain(): Couldn't find function %s in library %s", function_name, library_file);
        }
        (*env)->ReleaseStringUTFChars(env, function, function_name);

        dlclose(library_handle);

    } else {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "nativeRunMain(): Couldn't load library %s", library_file);
    }
    (*env)->ReleaseStringUTFChars(env, library, library_file);

    /* This is a Java thread, it doesn't need to be Detached from the JVM.
     * Set to mThreadKey value to NULL not to call pthread_create destructor 'Android_JNI_ThreadDestroyed' */
    Android_JNI_SetEnv(NULL);

    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    /* exit(status); */

    return status;
#endif
    return 0;
}

/* Drop file */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeDropFile)(const char * filename)
{
    SDL_SendDropFile(NULL, filename);
    SDL_SendDropComplete(NULL);
}

/* Lock / Unlock Mutex */
void Android_ActivityMutex_Lock(void)
{
    SDL_LockMutex(Android_ActivityMutex);
}

void Android_ActivityMutex_Unlock(void)
{
    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Lock the Mutex when the Activity is in its 'Running' state */
void Android_ActivityMutex_Lock_Running(void)
{
    int pauseSignaled = 0;
    int resumeSignaled = 0;

retry:

    SDL_LockMutex(Android_ActivityMutex);

    pauseSignaled = SDL_SemValue(Android_PauseSem);
    resumeSignaled = SDL_SemValue(Android_ResumeSem);

    if (pauseSignaled > resumeSignaled) {
        SDL_UnlockMutex(Android_ActivityMutex);
        SDL_Delay(50);
        goto retry;
    }
}

/* Set screen resolution */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeSetScreenResolution)(
    int surfaceWidth, int surfaceHeight,
    int deviceWidth, int deviceHeight, float rate)
{
    SDL_LockMutex(Android_ActivityMutex);

    Android_SetScreenResolution(surfaceWidth, surfaceHeight, deviceWidth, deviceHeight, rate);

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Resize */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeResize)()
{
    SDL_LockMutex(Android_ActivityMutex);

    if (Android_Window) {
        Android_SendResize(Android_Window);
    }

    SDL_UnlockMutex(Android_ActivityMutex);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeOrientationChanged)(int orientation)
{
    SDL_LockMutex(Android_ActivityMutex);

    displayOrientation = (SDL_DisplayOrientation)orientation;

    if (Android_Window) {
        SDL_VideoDisplay *display = SDL_GetDisplay(0);
        SDL_SendDisplayEvent(display, SDL_DISPLAYEVENT_ORIENTATION, orientation);
    }

    SDL_UnlockMutex(Android_ActivityMutex);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeAddTouch)(int touchId, const char * name)
{
    SDL_AddTouch((SDL_TouchID)touchId, SDL_TOUCH_DEVICE_DIRECT, name);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativePermissionResult)(int requestCode, int result)
{
    bPermissionRequestResult = result;
    SDL_AtomicSet(&bPermissionRequestPending, SDL_FALSE);
}

extern void SDL_AddAudioDevice(const SDL_bool iscapture, const char *name, SDL_AudioSpec *spec, void *handle);
extern void SDL_RemoveAudioDevice(const SDL_bool iscapture, void *handle);

JNIEXPORT void JNICALL
SDL_JAVA_AUDIO_INTERFACE(addAudioDevice)(int is_capture,
                                         int device_id)
{
    if (SDL_GetCurrentAudioDriver() != NULL) {
        char device_name[64];
        SDL_snprintf(device_name, sizeof(device_name), "%d", device_id);
        SDL_Log("Adding device with name %s, capture %d", device_name, is_capture);
        SDL_AddAudioDevice(is_capture, SDL_strdup(device_name), NULL, (void *)((size_t)device_id + 1));
    }
}

JNIEXPORT void JNICALL
SDL_JAVA_AUDIO_INTERFACE(removeAudioDevice)(int is_capture,
                                            int device_id)
{
    if (SDL_GetCurrentAudioDriver() != NULL) {
        SDL_Log("Removing device with handle %d, capture %d", device_id + 1, is_capture);
        SDL_RemoveAudioDevice(is_capture, (void *)((size_t)device_id + 1));
    }
}

/* Paddown */
JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativePadDown)(int device_id, int keycode) {
    return Android_OnPadDown(device_id, keycode);
}

/* Padup */
JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativePadUp)(int device_id, int keycode)
{
    return Android_OnPadUp(device_id, keycode);
}

/* Joy */
JNIEXPORT void JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativeJoy)(int device_id, int axis, float value)
{
    Android_OnJoy(device_id, axis, value);
}

/* POV Hat */
JNIEXPORT void JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativeHat)(int device_id, int hat_id, int x, int y)
{
    Android_OnHat(device_id, hat_id, x, y);
}

JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeAddJoystick)(
    int device_id, const char * device_name, const char * device_desc,
    int vendor_id, int product_id, int is_accelerometer,
    int button_mask, int naxes, int axis_mask, int nhats, int nballs)
{
    int retval;

    retval = Android_AddJoystick(device_id, device_name, device_desc, vendor_id, product_id, is_accelerometer ? SDL_TRUE : SDL_FALSE, button_mask, naxes, axis_mask, nhats, nballs);

    return retval;
}

JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveJoystick)(int device_id)
{
    return Android_RemoveJoystick(device_id);
}

JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeAddHaptic)(int device_id, const char * device_name)
{
    int retval;

    retval = Android_AddHaptic(device_id, device_name);

    return retval;
}

JNIEXPORT int JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveHaptic)(int device_id)
{
    return Android_RemoveHaptic(device_id);
}

/* Called from surfaceCreated() */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeSurfaceCreated)()
{
    SDL_LockMutex(Android_ActivityMutex);

    if (Android_Window) {
        SDL_WindowData *data = (SDL_WindowData *)Android_Window->driverdata;

        data->native_window = Android_JNI_GetNativeWindow();
        if (data->native_window == NULL) {
            SDL_SetError("Could not fetch native window from UI thread");
        }
    }

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Called from surfaceChanged() */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeSurfaceChanged)()
{
    SDL_LockMutex(Android_ActivityMutex);

#ifdef SDL_VIDEO_OPENGL_EGL
    if (Android_Window) {
        SDL_VideoDevice *_this = SDL_GetVideoDevice();
        SDL_WindowData *data = (SDL_WindowData *)Android_Window->driverdata;

        /* If the surface has been previously destroyed by onNativeSurfaceDestroyed, recreate it here */
        if (data->egl_surface == EGL_NO_SURFACE) {
            data->egl_surface = SDL_EGL_CreateSurface(_this, (NativeWindowType)data->native_window);
        }

        /* GL Context handling is done in the event loop because this function is run from the Java thread */
    }
#endif

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Called from surfaceDestroyed() */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeSurfaceDestroyed)()
{
    int nb_attempt = 50;

retry:

    SDL_LockMutex(Android_ActivityMutex);

    if (Android_Window) {
        SDL_VideoDevice *_this = SDL_GetVideoDevice();
        SDL_WindowData *data = (SDL_WindowData *)Android_Window->driverdata;

        /* Wait for Main thread being paused and context un-activated to release 'egl_surface' */
        if (!data->backup_done) {
            nb_attempt -= 1;
            if (nb_attempt == 0) {
                SDL_SetError("Try to release egl_surface with context probably still active");
            } else {
                SDL_UnlockMutex(Android_ActivityMutex);
                SDL_Delay(10);
                goto retry;
            }
        }

#ifdef SDL_VIDEO_OPENGL_EGL
        if (data->egl_surface != EGL_NO_SURFACE) {
            SDL_EGL_DestroySurface(_this, data->egl_surface);
            data->egl_surface = EGL_NO_SURFACE;
        }
#endif

        if (data->native_window) {
            ANativeWindow_release(data->native_window);
            data->native_window = NULL;
        }

        /* GL Context handling is done in the event loop because this function is run from the Java thread */
    }

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Keydown */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeKeyDown)(int keycode)
{
    Android_OnKeyDown(keycode);
}

/* Keyup */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeKeyUp)(int keycode)
{
    Android_OnKeyUp(keycode);
}

/* Virtual keyboard return key might stop text input */
JNIEXPORT int JNICALL SDL_JAVA_INTERFACE(onNativeSoftReturnKey)()
{
    if (SDL_GetHintBoolean(SDL_HINT_RETURN_KEY_HIDES_IME, SDL_FALSE)) {
        SDL_StopTextInput();
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

/* Keyboard Focus Lost */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeKeyboardFocusLost)()
{
    /* Calling SDL_StopTextInput will take care of hiding the keyboard and cleaning up the DummyText widget */
    SDL_StopTextInput();
}

/* Touch */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeTouch)(

    int touch_device_id_in, int pointer_finger_id_in,
    int action, float x, float y, float p)
{
    SDL_LockMutex(Android_ActivityMutex);

    Android_OnTouch(Android_Window, touch_device_id_in, pointer_finger_id_in, action, x, y, p);

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Mouse */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeMouse)(

    int button, int action, float x, float y, int relative)
{
    SDL_LockMutex(Android_ActivityMutex);

    Android_OnMouse(Android_Window, button, action, x, y, relative);

    SDL_UnlockMutex(Android_ActivityMutex);
}

/* Accelerometer */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeAccel)(float x, float y, float z)
{
    fLastAccelerometer[0] = x;
    fLastAccelerometer[1] = y;
    fLastAccelerometer[2] = z;
    bHasNewData = SDL_TRUE;
}

/* Clipboard */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeClipboardChanged)()
{
    SDL_SendClipboardUpdate();
}

/* Low memory */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeLowMemory)()
{
    SDL_SendAppEvent(SDL_APP_LOWMEMORY);
}

/* Locale
 * requires android:configChanges="layoutDirection|locale" in AndroidManifest.xml */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeLocaleChanged)()
{
    SDL_SendAppEvent(SDL_LOCALECHANGED);
}

/* Send Quit event to "SDLThread" thread */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeSendQuit)()
{
    /* Discard previous events. The user should have handled state storage
     * in SDL_APP_WILLENTERBACKGROUND. After nativeSendQuit() is called, no
     * events other than SDL_QUIT and SDL_APP_TERMINATING should fire */
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    /* Inject a SDL_QUIT event */
    SDL_SendQuit();
    SDL_SendAppEvent(SDL_APP_TERMINATING);
    /* Robustness: clear any pending Pause */
    while (SDL_SemTryWait(Android_PauseSem) == 0) {
        /* empty */
    }
    /* Resume the event loop so that the app can catch SDL_QUIT which
     * should now be the top event in the event queue. */
    SDL_SemPost(Android_ResumeSem);
}

/* Activity ends */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeQuit)()
{
    const char *str;

    if (Android_ActivityMutex) {
        SDL_DestroyMutex(Android_ActivityMutex);
        Android_ActivityMutex = NULL;
    }

    if (Android_PauseSem) {
        SDL_DestroySemaphore(Android_PauseSem);
        Android_PauseSem = NULL;
    }

    if (Android_ResumeSem) {
        SDL_DestroySemaphore(Android_ResumeSem);
        Android_ResumeSem = NULL;
    }

    str = SDL_GetError();
    if (str && str[0]) {
        __android_log_print(ANDROID_LOG_ERROR, "SDL", "SDLActivity thread ends (error=%s)", str);
    } else {
        __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDLActivity thread ends");
    }
}

/* Pause */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativePause)()
{
    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "nativePause()");

    /* Signal the pause semaphore so the event loop knows to pause and (optionally) block itself.
     * Sometimes 2 pauses can be queued (eg pause/resume/pause), so it's always increased. */
    SDL_SemPost(Android_PauseSem);
}

/* Resume */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeResume)()
{
    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "nativeResume()");

    /* Signal the resume semaphore so the event loop knows to resume and restore the GL Context
     * We can't restore the GL Context here because it needs to be done on the SDL main thread
     * and this function will be called from the Java thread instead.
     */
    SDL_SemPost(Android_ResumeSem);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeFocusChanged)(int hasFocus)
{
    SDL_LockMutex(Android_ActivityMutex);

    if (Android_Window) {
        __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "nativeFocusChanged()");
        SDL_SendWindowEvent(Android_Window, (hasFocus ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST), 0, 0);
    }

    SDL_UnlockMutex(Android_ActivityMutex);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeCommitText)(const char *text, int newCursorPosition)
{
    SDL_SendKeyboardText(text);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeGenerateScancodeForUnichar)(

    jchar chUnicode)
{
    SDL_SendKeyboardUnicodeKey(chUnicode);
}

JNIEXPORT const char * JNICALL SDL_JAVA_INTERFACE(nativeGetHint)(const char * name)
{
    static char result[1024];

    const char *hint = SDL_GetHint(name);
    if(!hint)
        return NULL;

    SDL_snprintf(result, sizeof(result) - 1, "%s", hint);

    return result;
}

JNIEXPORT int JNICALL SDL_JAVA_INTERFACE(nativeGetHintBoolean)(const char * name, int default_value)
{
    int result;

    result = SDL_GetHintBoolean(name, default_value);

    return result;
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeSetenv)( const char * name, const char * value)
{
    SDL_setenv(name, value, 1);
}

/*******************************************************************************
             Functions called by SDL into Java
*******************************************************************************/

static SDL_atomic_t s_active;
struct LocalReferenceHolder
{
    JNIEnv *m_env;
    const char *m_func;
};

static struct LocalReferenceHolder LocalReferenceHolder_Setup(const char *func)
{
    struct LocalReferenceHolder refholder;
    refholder.m_env = NULL;
    refholder.m_func = func;
#ifdef DEBUG_JNI
    SDL_Log("Entering function %s", func);
#endif
    return refholder;
}

static SDL_bool LocalReferenceHolder_Init(struct LocalReferenceHolder *refholder, JNIEnv *env)
{
    const int capacity = 16;
    if ((*env)->PushLocalFrame(env, capacity) < 0) {
        SDL_SetError("Failed to allocate enough JVM local references");
        return SDL_FALSE;
    }
    SDL_AtomicIncRef(&s_active);
    refholder->m_env = env;
    return SDL_TRUE;
}

static void LocalReferenceHolder_Cleanup(struct LocalReferenceHolder *refholder)
{
#ifdef DEBUG_JNI
    SDL_Log("Leaving function %s", refholder->m_func);
#endif
    if (refholder->m_env) {
        JNIEnv *env = refholder->m_env;
        (*env)->PopLocalFrame(env, NULL);
        SDL_AtomicDecRef(&s_active);
    }
}

ANativeWindow *Android_JNI_GetNativeWindow(void)
{
    ANativeWindow *anw = NULL;

    anw = CALLM(getNativeSurface);

    return anw;
}

void Android_JNI_SetActivityTitle(const char *title)
{
    CALLM(setActivityTitle, title);
}

void Android_JNI_SetWindowStyle(SDL_bool fullscreen)
{
    CALLM(setWindowStyle, fullscreen ? 1 : 0);
}

void Android_JNI_SetOrientation(int w, int h, int resizable, const char *hint)
{
    CALLM(setOrientation, w, h, (resizable ? 1 : 0), hint);
}

void Android_JNI_MinizeWindow(void)
{
    CALLM(minimizeWindow);
}

SDL_bool Android_JNI_ShouldMinimizeOnFocusLoss(void)
{
    return CALLM(shouldMinimizeOnFocusLoss);
}

SDL_bool Android_JNI_GetAccelerometerValues(float values[3])
{
    int i;
    SDL_bool retval = SDL_FALSE;

    if (bHasNewData) {
        for (i = 0; i < 3; ++i) {
            values[i] = fLastAccelerometer[i];
        }
        bHasNewData = SDL_FALSE;
        retval = SDL_TRUE;
    }

    return retval;
}

/*
 * Audio support
 */
static int audioBufferFormat = 0;
static char *audioBuffer = NULL;
static void *audioBufferPinned = NULL;
static int captureBufferFormat = 0;
static char *captureBuffer = NULL;

static void Android_JNI_GetAudioDevices(int *devices, int *length, int max_len, int is_input)
{
    int *result;

    if (is_input) {
        result = CALLM(getAudioInputDevices);
    } else {
        result = CALLM(getAudioOutputDevices);
    }

    *length = result ? ARRAY_LENGTH(result) : 0;

    *length = SDL_min(*length, max_len);

    memcpy(devices, result, sizeof(int) * *length);
}

void Android_DetectDevices(void)
{
    int inputs[100];
    int outputs[100];
    int inputs_length = 0;
    int outputs_length = 0;

    SDL_zeroa(inputs);

    Android_JNI_GetAudioDevices(inputs, &inputs_length, 100, 1 /* input devices */);

    for (int i = 0; i < inputs_length; ++i) {
        int device_id = inputs[i];
        char device_name[64];
        SDL_snprintf(device_name, sizeof(device_name), "%d", device_id);
        SDL_Log("Adding input device with name %s", device_name);
        SDL_AddAudioDevice(SDL_TRUE, SDL_strdup(device_name), NULL, (void *)((size_t)device_id + 1));
    }

    SDL_zeroa(outputs);

    Android_JNI_GetAudioDevices(outputs, &outputs_length, 100, 0 /* output devices */);

    for (int i = 0; i < outputs_length; ++i) {
        int device_id = outputs[i];
        char device_name[64];
        SDL_snprintf(device_name, sizeof(device_name), "%d", device_id);
        SDL_Log("Adding output device with name %s", device_name);
        SDL_AddAudioDevice(SDL_FALSE, SDL_strdup(device_name), NULL, (void *)((size_t)device_id + 1));
    }
}

int Android_JNI_OpenAudioDevice(int iscapture, int device_id, SDL_AudioSpec *spec)
{
    int audioformat;
    char *jbufobj = NULL;
    int *result;
    int *resultElements;

    switch (spec->format) {
    case AUDIO_U8:
        audioformat = ENCODING_PCM_8BIT;
        break;
    case AUDIO_S16:
        audioformat = ENCODING_PCM_16BIT;
        break;
    case AUDIO_F32:
        audioformat = ENCODING_PCM_FLOAT;
        break;
    default:
        return SDL_SetError("Unsupported audio format: 0x%x", spec->format);
    }

    if (iscapture) {
        __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device for capture");
        result = CALLM(captureOpen, spec->freq, audioformat, spec->channels, spec->samples, device_id);
    } else {
        __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device for output");
        result = CALLM(audioOpen, spec->freq, audioformat, spec->channels, spec->samples, device_id);
    }
    if (!result) {
        /* Error during audio initialization, error printed from Java */
        return SDL_SetError("Java-side initialization failed");
    }

    if (ARRAY_LENGTH(result) != 4) {
        return SDL_SetError("Unexpected results from Java, expected 4, got %d", ARRAY_LENGTH(result));
    }
    resultElements = result;
    spec->freq = resultElements[0];
    audioformat = resultElements[1];
    switch (audioformat) {
    case ENCODING_PCM_8BIT:
        spec->format = AUDIO_U8;
        break;
    case ENCODING_PCM_16BIT:
        spec->format = AUDIO_S16;
        break;
    case ENCODING_PCM_FLOAT:
        spec->format = AUDIO_F32;
        break;
    default:
        return SDL_SetError("Unexpected audio format from Java: %d\n", audioformat);
    }
    spec->channels = resultElements[2];
    spec->samples = resultElements[3];

    /* Allocating the audio buffer from the Java side and passing it as the return value for audioInit no longer works on
     * Android >= 4.2 due to a "stale global reference" error. So now we allocate this buffer directly from this side. */
    switch (audioformat) {
    case ENCODING_PCM_8BIT:
    {
        jbufobj = sdl_array_alloc(spec->samples * spec->channels);
    } break;
    case ENCODING_PCM_16BIT:
    {
        jbufobj = sdl_array_allocs(2, spec->samples * spec->channels);
    } break;
    case ENCODING_PCM_FLOAT:
    {
        jbufobj = sdl_array_allocs(4, spec->samples * spec->channels);
    } break;
    default:
        return SDL_SetError("Unexpected audio format from Java: %d\n", audioformat);
    }

    if (!jbufobj) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: could not allocate an audio buffer");
        return SDL_OutOfMemory();
    }

    if (iscapture) {
        captureBufferFormat = audioformat;
        captureBuffer = jbufobj;
    } else {
        audioBufferFormat = audioformat;
        audioBuffer = jbufobj;
    }

    if (!iscapture) {
        switch (audioformat) {
        case ENCODING_PCM_8BIT:
            audioBufferPinned = audioBuffer;
            break;
        case ENCODING_PCM_16BIT:
            audioBufferPinned = audioBuffer;
            break;
        case ENCODING_PCM_FLOAT:
            audioBufferPinned = audioBuffer;
            break;
        default:
            return SDL_SetError("Unexpected audio format from Java: %d\n", audioformat);
        }
    }
    return 0;
}

SDL_DisplayOrientation Android_JNI_GetDisplayOrientation(void)
{
    return displayOrientation;
}

int Android_JNI_GetDisplayDPI(float *ddpi, float *xdpi, float *ydpi)
{
#if 0
    JNIEnv *env = Android_JNI_GetEnv();

    jobject jDisplayObj = (*env)->CallStaticObjectMethod(env, mActivityClass, midGetDisplayDPI);
    jclass jDisplayClass = (*env)->GetObjectClass(env, jDisplayObj);

    jfieldID fidXdpi = (*env)->GetFieldID(env, jDisplayClass, "xdpi", "F");
    jfieldID fidYdpi = (*env)->GetFieldID(env, jDisplayClass, "ydpi", "F");
    jfieldID fidDdpi = (*env)->GetFieldID(env, jDisplayClass, "densityDpi", "I");

    float nativeXdpi = (*env)->GetFloatField(env, jDisplayObj, fidXdpi);
    float nativeYdpi = (*env)->GetFloatField(env, jDisplayObj, fidYdpi);
    int nativeDdpi = (*env)->GetIntField(env, jDisplayObj, fidDdpi);

    (*env)->DeleteLocalRef(env, jDisplayObj);
    (*env)->DeleteLocalRef(env, jDisplayClass);

    if (ddpi) {
        *ddpi = (float)nativeDdpi;
    }
    if (xdpi) {
        *xdpi = nativeXdpi;
    }
    if (ydpi) {
        *ydpi = nativeYdpi;
    }
#endif
    return 0;
}

void *Android_JNI_GetAudioBuffer(void)
{
    return audioBufferPinned;
}

void Android_JNI_WriteAudioBuffer(void)
{
    switch (audioBufferFormat) {
    case ENCODING_PCM_8BIT:
        CALLM(audioWriteByteBuffer, (char *)audioBuffer);
        break;
    case ENCODING_PCM_16BIT:
        CALLM(audioWriteShortBuffer, (short *)audioBuffer);
        break;
    case ENCODING_PCM_FLOAT:
        CALLM(audioWriteFloatBuffer, (float *)audioBuffer);
        break;
    default:
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: unhandled audio buffer format");
        break;
    }

    /* JNI_COMMIT means the changes are committed to the VM but the buffer remains pinned */
}

int Android_JNI_CaptureAudioBuffer(void *buffer, int buflen)
{
    int br = -1;

    switch (captureBufferFormat) {
    case ENCODING_PCM_8BIT:
        SDL_assert(sdl_array_length(captureBuffer) == buflen);
        br = CALLM(captureReadByteBuffer, (char *)captureBuffer, JNI_TRUE);
        if (br > 0) {
            jbyte *ptr = (char *)captureBuffer;
            SDL_memcpy(buffer, ptr, br);
        }
        break;
    case ENCODING_PCM_16BIT:
        SDL_assert(sdl_array_length(captureBuffer) == (buflen / sizeof(Sint16)));
        br = CALLM(captureReadShortBuffer, (short *)captureBuffer, JNI_TRUE);
        if (br > 0) {
            jshort *ptr = (short *)captureBuffer;
            br *= sizeof(Sint16);
            SDL_memcpy(buffer, ptr, br);
        }
        break;
    case ENCODING_PCM_FLOAT:
        SDL_assert(sdl_array_length(captureBuffer) == (buflen / sizeof(float)));
        br = CALLM(captureReadFloatBuffer, (float *)captureBuffer, JNI_TRUE);
        if (br > 0) {
            float *ptr = (float *)captureBuffer;
            br *= sizeof(float);
            SDL_memcpy(buffer, ptr, br);
        }
        break;
    default:
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: unhandled capture buffer format");
        break;
    }
    return br;
}

void Android_JNI_FlushCapturedAudio(void)
{
#if 0 /* !!! FIXME: this needs API 23, or it'll do blocking reads and never end. */
    switch (captureBufferFormat) {
    case ENCODING_PCM_8BIT:
        {
            const int len = (*env)->GetArrayLength(env, (jbyteArray)captureBuffer);
            while ((*env)->CallStaticIntMethod(env, mActivityClass, midCaptureReadByteBuffer, (jbyteArray)captureBuffer, JNI_FALSE) == len) { /* spin */ }
        }
        break;
    case ENCODING_PCM_16BIT:
        {
            const int len = (*env)->GetArrayLength(env, (jshortArray)captureBuffer);
            while ((*env)->CallStaticIntMethod(env, mActivityClass, midCaptureReadShortBuffer, (jshortArray)captureBuffer, JNI_FALSE) == len) { /* spin */ }
        }
        break;
    case ENCODING_PCM_FLOAT:
        {
            const int len = (*env)->GetArrayLength(env, (floatArray)captureBuffer);
            while ((*env)->CallStaticIntMethod(env, mActivityClass, midCaptureReadFloatBuffer, (floatArray)captureBuffer, JNI_FALSE) == len) { /* spin */ }
        }
        break;
    default:
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: flushing unhandled capture buffer format");
        break;
    }
#else
    switch (captureBufferFormat) {
    case ENCODING_PCM_8BIT:
        CALLM(captureReadByteBuffer, (char *)captureBuffer, JNI_FALSE);
        break;
    case ENCODING_PCM_16BIT:
        CALLM(captureReadShortBuffer, (short *)captureBuffer, JNI_FALSE);
        break;
    case ENCODING_PCM_FLOAT:
        CALLM(captureReadFloatBuffer, (float *)captureBuffer, JNI_FALSE);
        break;
    default:
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: flushing unhandled capture buffer format");
        break;
    }
#endif
}

void Android_JNI_CloseAudioDevice(const int iscapture)
{
    if (iscapture) {
        CALLM(captureClose);
        if (captureBuffer) {
            sdl_array_free(captureBuffer);
            captureBuffer = NULL;
        }
    } else {
        CALLM(audioClose);
        if (audioBuffer) {
            sdl_array_free(audioBuffer);
            audioBuffer = NULL;
            audioBufferPinned = NULL;
        }
    }
}

void Android_JNI_AudioSetThreadPriority(int iscapture, int device_id)
{
    CALLM(audioSetThreadPriority, iscapture, device_id);
}

/* Test for an exception and call SDL_SetError with its detail if one occurs */
/* If the parameter silent is truthy then SDL_SetError() will not be called. */
int Android_JNI_FileOpen(SDL_RWops *ctx,
                         const char *fileName, const char *mode)
{
    return SDL_SetError("Couldn't open asset '%s'", fileName);
}

size_t Android_JNI_FileRead(SDL_RWops *ctx, void *buffer,
                            size_t size, size_t maxnum)
{
    return 0;
}

size_t Android_JNI_FileWrite(SDL_RWops *ctx, const void *buffer,
                             size_t size, size_t num)
{
    SDL_SetError("Cannot write to Android package filesystem");
    return 0;
}

Sint64 Android_JNI_FileSize(SDL_RWops *ctx)
{
    return 0;
}

Sint64 Android_JNI_FileSeek(SDL_RWops *ctx, Sint64 offset, int whence)
{
    return 0;
}

int Android_JNI_FileClose(SDL_RWops *ctx)
{
    return 0;
}

int Android_JNI_SetClipboardText(const char *text)
{
    CALLM(clipboardSetText, text);
    return 0;
}

char *Android_JNI_GetClipboardText(void)
{
    char *text = NULL;
    const char *string;

    string = CALLM(clipboardGetText);
    if (string) {
        text = SDL_strdup(string);
    }

    return (!text) ? SDL_strdup("") : text;
}

SDL_bool Android_JNI_HasClipboardText(void)
{
    int retval = CALLM(clipboardHasText);
    return (retval) ? SDL_TRUE : SDL_FALSE;
}

/* returns 0 on success or -1 on error (others undefined then)
 * returns truthy or falsy value in plugged, charged and battery
 * returns the value in seconds and percent or -1 if not available
 */
int Android_JNI_GetPowerInfo(int *plugged, int *charged, int *battery, int *seconds, int *percent)
{
#if 0
    struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
    JNIEnv *env = Android_JNI_GetEnv();
    jmethodID mid;
    jobject context;
    const char * action;
    jclass cls;
    jobject filter;
    jobject intent;
    const char * iname;
    jmethodID imid;
    const char * bname;
    jmethodID bmid;
    if (!LocalReferenceHolder_Init(&refs, env)) {
        LocalReferenceHolder_Cleanup(&refs);
        return -1;
    }

    /* context = SDLActivity.getContext(); */
    context = (*env)->CallStaticObjectMethod(env, mActivityClass, midGetContext);

    action = (*env)->NewStringUTF(env, "android.intent.action.BATTERY_CHANGED");

    cls = (*env)->FindClass(env, "android/content/IntentFilter");

    mid = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;)V");
    filter = (*env)->NewObject(env, cls, mid, action);

    (*env)->DeleteLocalRef(env, action);

    mid = (*env)->GetMethodID(env, mActivityClass, "registerReceiver", "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
    intent = (*env)->CallObjectMethod(env, context, mid, NULL, filter);

    (*env)->DeleteLocalRef(env, filter);

    cls = (*env)->GetObjectClass(env, intent);

    imid = (*env)->GetMethodID(env, cls, "getIntExtra", "(Ljava/lang/String;I)I");

    /* Watch out for C89 scoping rules because of the macro */
#define GET_INT_EXTRA(var, key)                                  \
    int var;                                                     \
    iname = (*env)->NewStringUTF(env, key);                      \
    (var) = (*env)->CallIntMethod(env, intent, imid, iname, -1); \
    (*env)->DeleteLocalRef(env, iname);

    bmid = (*env)->GetMethodID(env, cls, "getBooleanExtra", "(Ljava/lang/String;Z)Z");

    /* Watch out for C89 scoping rules because of the macro */
#define GET_BOOL_EXTRA(var, key)                                            \
    int var;                                                                \
    bname = (*env)->NewStringUTF(env, key);                                 \
    (var) = (*env)->CallBooleanMethod(env, intent, bmid, bname, JNI_FALSE); \
    (*env)->DeleteLocalRef(env, bname);

    if (plugged) {
        /* Watch out for C89 scoping rules because of the macro */
        GET_INT_EXTRA(plug, "plugged") /* == BatteryManager.EXTRA_PLUGGED (API 5) */
        if (plug == -1) {
            LocalReferenceHolder_Cleanup(&refs);
            return -1;
        }
        /* 1 == BatteryManager.BATTERY_PLUGGED_AC */
        /* 2 == BatteryManager.BATTERY_PLUGGED_USB */
        *plugged = (0 < plug) ? 1 : 0;
    }

    if (charged) {
        /* Watch out for C89 scoping rules because of the macro */
        GET_INT_EXTRA(status, "status") /* == BatteryManager.EXTRA_STATUS (API 5) */
        if (status == -1) {
            LocalReferenceHolder_Cleanup(&refs);
            return -1;
        }
        /* 5 == BatteryManager.BATTERY_STATUS_FULL */
        *charged = (status == 5) ? 1 : 0;
    }

    if (battery) {
        GET_BOOL_EXTRA(present, "present") /* == BatteryManager.EXTRA_PRESENT (API 5) */
        *battery = present ? 1 : 0;
    }

    if (seconds) {
        *seconds = -1; /* not possible */
    }

    if (percent) {
        int level;
        int scale;

        /* Watch out for C89 scoping rules because of the macro */
        {
            GET_INT_EXTRA(level_temp, "level") /* == BatteryManager.EXTRA_LEVEL (API 5) */
            level = level_temp;
        }
        /* Watch out for C89 scoping rules because of the macro */
        {
            GET_INT_EXTRA(scale_temp, "scale") /* == BatteryManager.EXTRA_SCALE (API 5) */
            scale = scale_temp;
        }

        if ((level == -1) || (scale == -1)) {
            LocalReferenceHolder_Cleanup(&refs);
            return -1;
        }
        *percent = level * 100 / scale;
    }

    (*env)->DeleteLocalRef(env, intent);

    LocalReferenceHolder_Cleanup(&refs);
    return 0;
#endif
    return -1;
}

/* Add all touch devices */
void Android_JNI_InitTouch(void)
{
    CALLM(initTouch);
}

void Android_JNI_PollInputDevices(void)
{
    CALLM(pollInputDevices);
}

void Android_JNI_PollHapticDevices(void)
{
    CALLM(pollHapticDevices);
}

void Android_JNI_HapticRun(int device_id, float intensity, int length)
{
    CALLM(hapticRun, device_id, intensity, length);
}

void Android_JNI_HapticStop(int device_id)
{
    CALLM(hapticStop, device_id);
}

/* See SDLActivity.java for constants. */
#define COMMAND_SET_KEEP_SCREEN_ON 5

int SDL_AndroidSendMessage(Uint32 command, int param)
{
    if (command >= 0x8000) {
        return Android_JNI_SendMessage(command, param);
    }
    return -1;
}

/* sends message to be handled on the UI event dispatch thread */
int Android_JNI_SendMessage(int command, int param)
{
    int success;
    success = CALLM(sendMessage, command, param);
    return success ? 0 : -1;
}

void Android_JNI_SuspendScreenSaver(SDL_bool suspend)
{
    Android_JNI_SendMessage(COMMAND_SET_KEEP_SCREEN_ON, (suspend == SDL_FALSE) ? 0 : 1);
}

void Android_JNI_ShowScreenKeyboard(SDL_Rect *inputRect)
{
    CALLM(showTextInput,
                                    inputRect->x,
                                    inputRect->y,
                                    inputRect->w,
                                    inputRect->h);
}

void Android_JNI_HideScreenKeyboard(void)
{
    /* has to match Activity constant */
    const int COMMAND_TEXTEDIT_HIDE = 3;
    Android_JNI_SendMessage(COMMAND_TEXTEDIT_HIDE, 0);
}

SDL_bool Android_JNI_IsScreenKeyboardShown(void)
{
    int is_shown = 0;
    is_shown = CALLM(isScreenKeyboardShown);
    return is_shown;
}

int Android_JNI_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid)
{
#if 0
    JNIEnv *env;
    jclass clazz;
    jmethodID mid;
    jobject context;
    const char * title;
    const char * message;
    intArray button_flags;
    intArray button_ids;
    jobjectArray button_texts;
    intArray colors;
    jobject text;
    int temp;
    int i;

    env = Android_JNI_GetEnv();

    /* convert parameters */

    clazz = (*env)->FindClass(env, "java/lang/String");

    title = (*env)->NewStringUTF(env, messageboxdata->title);
    message = (*env)->NewStringUTF(env, messageboxdata->message);

    button_flags = (*env)->NewIntArray(env, messageboxdata->numbuttons);
    button_ids = (*env)->NewIntArray(env, messageboxdata->numbuttons);
    button_texts = (*env)->NewObjectArray(env, messageboxdata->numbuttons,
                                          clazz, NULL);
    for (i = 0; i < messageboxdata->numbuttons; ++i) {
        const SDL_MessageBoxButtonData *sdlButton;

        if (messageboxdata->flags & SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT) {
            sdlButton = &messageboxdata->buttons[messageboxdata->numbuttons - 1 - i];
        } else {
            sdlButton = &messageboxdata->buttons[i];
        }

        temp = sdlButton->flags;
        (*env)->SetIntArrayRegion(env, button_flags, i, 1, &temp);
        temp = sdlButton->buttonid;
        (*env)->SetIntArrayRegion(env, button_ids, i, 1, &temp);
        text = (*env)->NewStringUTF(env, sdlButton->text);
        (*env)->SetObjectArrayElement(env, button_texts, i, text);
        (*env)->DeleteLocalRef(env, text);
    }

    if (messageboxdata->colorScheme) {
        colors = (*env)->NewIntArray(env, SDL_MESSAGEBOX_COLOR_MAX);
        for (i = 0; i < SDL_MESSAGEBOX_COLOR_MAX; ++i) {
            temp = (0xFFU << 24) |
                   (messageboxdata->colorScheme->colors[i].r << 16) |
                   (messageboxdata->colorScheme->colors[i].g << 8) |
                   (messageboxdata->colorScheme->colors[i].b << 0);
            (*env)->SetIntArrayRegion(env, colors, i, 1, &temp);
        }
    } else {
        colors = NULL;
    }

    (*env)->DeleteLocalRef(env, clazz);

    /* context = SDLActivity.getContext(); */
    context = (*env)->CallStaticObjectMethod(env, mActivityClass, midGetContext);

    clazz = (*env)->GetObjectClass(env, context);

    mid = (*env)->GetMethodID(env, clazz,
                              "messageboxShowMessageBox", "(ILjava/lang/String;Ljava/lang/String;[I[I[Ljava/lang/String;[I)I");
    *buttonid = (*env)->CallIntMethod(env, context, mid,
                                      messageboxdata->flags,
                                      title,
                                      message,
                                      button_flags,
                                      button_ids,
                                      button_texts,
                                      colors);

    (*env)->DeleteLocalRef(env, context);
    (*env)->DeleteLocalRef(env, clazz);

    /* delete parameters */

    (*env)->DeleteLocalRef(env, title);
    (*env)->DeleteLocalRef(env, message);
    (*env)->DeleteLocalRef(env, button_flags);
    (*env)->DeleteLocalRef(env, button_ids);
    (*env)->DeleteLocalRef(env, button_texts);
    (*env)->DeleteLocalRef(env, colors);

#endif
    const char * title;
    const char * message;
    int *button_flags;
    int *button_ids;
    const char **button_texts;
    int *colors = NULL;
    int temp;
    int i;

    title = messageboxdata->title;
    message = messageboxdata->message;

    button_flags = sdl_array_allocs( sizeof(int), messageboxdata->numbuttons);
    button_ids = sdl_array_allocs(sizeof(int), messageboxdata->numbuttons);
    button_texts = sdl_array_allocs(sizeof(const char *), messageboxdata->numbuttons);
    for (i = 0; i < messageboxdata->numbuttons; ++i) {
        const SDL_MessageBoxButtonData *sdlButton;

        if (messageboxdata->flags & SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT) {
            sdlButton = &messageboxdata->buttons[messageboxdata->numbuttons - 1 - i];
        } else {
            sdlButton = &messageboxdata->buttons[i];
        }

        temp = sdlButton->flags;
        button_flags[i] = temp;
        temp = sdlButton->buttonid;
        button_ids[i] = temp;
        button_texts[i] = sdlButton->text;
    }

    if (messageboxdata->colorScheme) {
        colors = sdl_array_allocs(sizeof(int), SDL_MESSAGEBOX_COLOR_MAX);
        for (i = 0; i < SDL_MESSAGEBOX_COLOR_MAX; ++i) {
            temp = (0xFFU << 24) |
                   (messageboxdata->colorScheme->colors[i].r << 16) |
                   (messageboxdata->colorScheme->colors[i].g << 8) |
                   (messageboxdata->colorScheme->colors[i].b << 0);
            colors[i] = temp;
        }
    } else {
        colors = NULL;
    }

    int res = CALLM(messageboxShowMessageBox, messageboxdata->flags, title, message, button_flags, button_ids, button_texts, colors);

    sdl_array_free(button_flags);
    sdl_array_free(button_ids);
    sdl_array_free(button_texts);
    sdl_array_free(colors);

    return res;
}

/*
//////////////////////////////////////////////////////////////////////////////
//
// Functions exposed to SDL applications in SDL_system.h
//////////////////////////////////////////////////////////////////////////////
*/

void *SDL_AndroidGetJNIEnv(void)
{
    return Android_JNI_GetEnv();
}

void *SDL_AndroidGetActivity(void)
{
    /* See SDL_system.h for caveats on using this function. */

    /* return SDLActivity.getContext(); */
    return CALLM(getContext);
}

int SDL_GetAndroidSDKVersion(void)
{
    static int sdk_version;
    if (!sdk_version) {
        char sdk[PROP_VALUE_MAX] = { 0 };
        if (__system_property_get("ro.build.version.sdk", sdk) != 0) {
            sdk_version = SDL_atoi(sdk);
        }
    }
    return sdk_version;
}

SDL_bool SDL_IsAndroidTablet(void)
{
    return CALLM(isTablet);
}

SDL_bool SDL_IsAndroidTV(void)
{
    return CALLM(isAndroidTV);
}

SDL_bool SDL_IsChromebook(void)
{
    return CALLM(isChromebook);
}

SDL_bool SDL_IsDeXMode(void)
{
    return CALLM(isDeXMode);
}

void SDL_AndroidBackButton(void)
{
    CALLM(manualBackButton);
}

const char *SDL_AndroidGetInternalStoragePath(void)
{
    return "/sdcard";
}

int SDL_AndroidGetExternalStorageState(void)
{
#if 0
    struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
    jmethodID mid;
    jclass cls;
    const char * stateString;
    const char *state;
    int stateFlags;

    JNIEnv *env = Android_JNI_GetEnv();
    if (!LocalReferenceHolder_Init(&refs, env)) {
        LocalReferenceHolder_Cleanup(&refs);
        return 0;
    }

    cls = (*env)->FindClass(env, "android/os/Environment");
    mid = (*env)->GetStaticMethodID(env, cls,
                                    "getExternalStorageState", "()Ljava/lang/String;");
    stateString = (const char *)(*env)->CallStaticObjectMethod(env, cls, mid);

    state = (*env)->GetStringUTFChars(env, stateString, NULL);

    /* Print an info message so people debugging know the storage state */
    __android_log_print(ANDROID_LOG_INFO, "SDL", "external storage state: %s", state);

    if (SDL_strcmp(state, "mounted") == 0) {
        stateFlags = SDL_ANDROID_EXTERNAL_STORAGE_READ |
                     SDL_ANDROID_EXTERNAL_STORAGE_WRITE;
    } else if (SDL_strcmp(state, "mounted_ro") == 0) {
        stateFlags = SDL_ANDROID_EXTERNAL_STORAGE_READ;
    } else {
        stateFlags = 0;
    }
    (*env)->ReleaseStringUTFChars(env, stateString, state);

    LocalReferenceHolder_Cleanup(&refs);
    return stateFlags;
#endif
    return 0;
}

const char *SDL_AndroidGetExternalStoragePath(void)
{
#if 0
    static char *s_AndroidExternalFilesPath = NULL;

    if (!s_AndroidExternalFilesPath) {
        struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
        jmethodID mid;
        jobject context;
        jobject fileObject;
        const char * pathString;
        const char *path;

        JNIEnv *env = Android_JNI_GetEnv();
        if (!LocalReferenceHolder_Init(&refs, env)) {
            LocalReferenceHolder_Cleanup(&refs);
            return NULL;
        }

        /* context = SDLActivity.getContext(); */
        context = (*env)->CallStaticObjectMethod(env, mActivityClass, midGetContext);

        /* fileObj = context.getExternalFilesDir(); */
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
                                  "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
        fileObject = (*env)->CallObjectMethod(env, context, mid, NULL);
        if (!fileObject) {
            SDL_SetError("Couldn't get external directory");
            LocalReferenceHolder_Cleanup(&refs);
            return NULL;
        }

        /* path = fileObject.getAbsolutePath(); */
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject),
                                  "getAbsolutePath", "()Ljava/lang/String;");
        pathString = (const char *)(*env)->CallObjectMethod(env, fileObject, mid);

        path = (*env)->GetStringUTFChars(env, pathString, NULL);
        s_AndroidExternalFilesPath = SDL_strdup(path);
        (*env)->ReleaseStringUTFChars(env, pathString, path);

        LocalReferenceHolder_Cleanup(&refs);
    }
    return s_AndroidExternalFilesPath;
#endif
    return "/sdcard";
}

SDL_bool SDL_AndroidRequestPermission(const char *permission)
{
    return Android_JNI_RequestPermission(permission);
}

int SDL_AndroidShowToast(const char *message, int duration, int gravity, int xOffset, int yOffset)
{
    return Android_JNI_ShowToast(message, duration, gravity, xOffset, yOffset);
}

void Android_JNI_GetManifestEnvironmentVariables(void)
{
    if (!METHOD(getManifestEnvironmentVariables)) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "Request to get environment variables before JNI is ready");
        return;
    }

    if (!bHasEnvironmentVariables) {
        SDL_bool ret = CALLM(getManifestEnvironmentVariables);
        if (ret) {
            bHasEnvironmentVariables = SDL_TRUE;
        }
    }
}

int Android_JNI_CreateCustomCursor(SDL_Surface *surface, int hot_x, int hot_y)
{
    int custom_cursor = 0;
    int *pixels = (int *)surface->pixels;
    if (pixels) {
        custom_cursor = CALLM(createCustomCursor, pixels, surface->w, surface->h, hot_x, hot_y);
    } else {
        SDL_OutOfMemory();
    }
    return custom_cursor;
}

void Android_JNI_DestroyCustomCursor(int cursorID)
{
    CALLM(destroyCustomCursor, cursorID);
}

SDL_bool Android_JNI_SetCustomCursor(int cursorID)
{
    return CALLM(setCustomCursor, cursorID);
}

SDL_bool Android_JNI_SetSystemCursor(int cursorID)
{
    return CALLM(setSystemCursor, cursorID);
}

SDL_bool Android_JNI_SupportsRelativeMouse(void)
{
    return CALLM(supportsRelativeMouse);
}

SDL_bool Android_JNI_SetRelativeMouseEnabled(SDL_bool enabled)
{
    return CALLM(setRelativeMouseEnabled, (enabled == 1));
}

SDL_bool Android_JNI_RequestPermission(const char *permission)
{
    const int requestCode = 1;

    /* Wait for any pending request on another thread */
    while (SDL_AtomicGet(&bPermissionRequestPending) == SDL_TRUE) {
        SDL_Delay(10);
    }
    SDL_AtomicSet(&bPermissionRequestPending, SDL_TRUE);

    CALLM(requestPermission, permission, requestCode);

    /* Wait for the request to complete */
    while (SDL_AtomicGet(&bPermissionRequestPending) == SDL_TRUE) {
        SDL_Delay(10);
    }
    return bPermissionRequestResult;
}

/* Show toast notification */
int Android_JNI_ShowToast(const char *message, int duration, int gravity, int xOffset, int yOffset)
{
    int result = 0;
    result = CALLM(showToast, message, duration, gravity, xOffset, yOffset);
    return result;
}

int Android_JNI_GetLocale(char *buf, size_t buflen)
{
    AConfiguration *cfg;

    SDL_assert(buflen > 6);

    /* Need to re-create the asset manager if locale has changed (SDL_LOCALECHANGED) */

    if (!asset_manager) {
        return -1;
    }

    cfg = AConfiguration_new();
    if (!cfg) {
        return -1;
    }

    {
        char language[2] = {};
        char country[2] = {};
        size_t id = 0;

        AConfiguration_fromAssetManager(cfg, asset_manager);
        AConfiguration_getLanguage(cfg, language);
        AConfiguration_getCountry(cfg, country);

        /* copy language (not null terminated) */
        if (language[0]) {
            buf[id++] = language[0];
            if (language[1]) {
                buf[id++] = language[1];
            }
        }

        buf[id++] = '_';

        /* copy country (not null terminated) */
        if (country[0]) {
            buf[id++] = country[0];
            if (country[1]) {
                buf[id++] = country[1];
            }
        }

        buf[id++] = '\0';
        SDL_assert(id <= buflen);
    }

    AConfiguration_delete(cfg);

    return 0;
}

int Android_JNI_OpenURL(const char *url)
{
    const int ret = CALLM(openURL, url);
    return ret;
}

#endif /* __ANDROID__ */

/* vi: set ts=4 sw=4 expandtab: */
