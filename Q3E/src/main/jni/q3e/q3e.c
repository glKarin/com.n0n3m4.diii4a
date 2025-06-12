/*
	Copyright (C) 2012 n0n3m4	
	This file contains some code from kwaak3:
	Copyright (C) 2010 Roderick Colenbrander

    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "q3e.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <pthread.h>

#include "q3estd.h"
#include "q3eeventqueue.h"
#include "q3ebt.h"
#include "q3ethread.h"
#include "q3eutility.h"
#include "q3emisc.h"
#ifdef _Q3E_SDL
#include "q3esdl2.h"
#else
#define CALL_SDL(...)
#define EXEC_SDL(...)
#define INIT_SDL()
#endif

#include "doom3/neo/sys/android/sys_android.h"

#define LOG_TAG "Q3E::JNI"

#define JNI_Version JNI_VERSION_1_4
#define Q3E_MAX_ARGS 512 // 255

//#define AUDIOTRACK_BYTEBUFFER 1

// call DOOM3
static void (*setResolution)(int width, int height);
static void (*Q3E_SetInitialContext)(const void *context);
static void (*setCallbacks)(const void *func);
static void (*set_gl_context)(ANativeWindow *window);
static void (*request_thread_quit)(void);

static int  (*qmain)(int argc, char **argv);
static void (*onKeyEvent)(int state, int key,int chr);
static void (*onAnalog)(int enable, float x, float y);
static void (*onMotionEvent)(float x, float y);
static void (*on_pause)(void);
static void (*on_resume)(void);
static void (*qexit)(void);

// Android function
static int pull_input_event(int num);
static void grab_mouse(int grab);
static FILE * android_tmpfile(void);
static void copy_to_clipboard(const char *text);
static char * get_clipboard_text(void);
static void show_toast(const char *text);
static void open_keyboard(void);
static void close_keyboard(void);
static void setup_smooth_joystick(int enable);
static void open_url(const char *url);
static int open_dialog(const char *title, const char *message, int num, const char *buttons[]);
static void finish(void);

// data
static char *game_data_dir = NULL;
static char *arg_str = NULL;

static void *libdl;
static ANativeWindow *window = NULL;
static int usingNativeEventQueue = 1;
static int usingNativeThread = 1;

static int resultCode = -1;

static int backtrace_exit = 0;

// Java object ref
static JavaVM *jVM;
static jobject audioBuffer=0;
static jobject q3eCallbackObj=0;
static const jbyte *audio_track_buffer = NULL;
static int mouse_available = 0;

// game main thread
static pthread_t				main_thread;

// Java method
static jmethodID android_PullEvent_method;
static jmethodID android_GrabMouse_method;
static jmethodID android_SetupSmoothJoystick_method;
static jmethodID android_CopyToClipboard_method;
static jmethodID android_GetClipboardText_method;

static jmethodID android_initAudio;
static jmethodID android_writeAudio;
static jmethodID android_setState;
static jmethodID android_writeAudio_array;

static jmethodID android_ShowToast_method;
static jmethodID android_OpenVKB_method;
static jmethodID android_CloseVKB_method;
static jmethodID android_OpenURL_method;
static jmethodID android_OpenDialog_method;
static jmethodID android_Finish_method;
static jmethodID android_Backtrace_method;

#define ATTACH_JNI(env) \
	JNIEnv *env = 0; \
	if ( ((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4)) < 0 ) \
	{ \
		(*jVM)->AttachCurrentThread(jVM, &env, NULL); \
	}

static void Android_AttachThread(void)
{
	ATTACH_JNI(env)
}

static void Android_DetachThread(void)
{
	JNIEnv *env = 0;
	if ( ((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4)) >= 0 ) {
		(*jVM)->DetachCurrentThread(jVM);
	}
}

static int backtrace_after_caught_signal(int signnum)
{
	ATTACH_JNI(env)

	LOGI("idTech4A++ caught signal: %d, application exiting......", signnum);
	Q3E_BT_Shutdown();
	if(!backtrace_exit)
	{
		if(q3eCallbackObj && android_Finish_method)
			finish();
		else
			exit(0);
	}
	return 0;
}

static void backtrace_signal_caughted(int num, int pid, int tid, int mask, const char *cfi, const char *fp, const char *eh)
{
	ATTACH_JNI(env)

	LOGI("Backtrace dialog: %d", num);

	jstring str;
	jobjectArray stackArray = NULL;
	jclass stringClazz = (*env)->FindClass(env, "java/lang/String");
	stackArray = (*env)->NewObjectArray(env, 3, stringClazz, NULL);

	if(mask > 0)
	{
		if(mask & SAMPLE_SOLUTION_CFI)
		{
			if(cfi)
			{
				str = (*env)->NewStringUTF(env, cfi);
				jstring nStr = (*env)->NewWeakGlobalRef(env, str);
				(*env)->DeleteLocalRef(env, str);
				(*env)->SetObjectArrayElement(env, stackArray, 0, nStr);
			}
		}
		if(mask & SAMPLE_SOLUTION_FP)
		{
			if(fp)
			{
				str = (*env)->NewStringUTF(env, fp);
				jstring nStr = (*env)->NewWeakGlobalRef(env, str);
				(*env)->DeleteLocalRef(env, str);
				(*env)->SetObjectArrayElement(env, stackArray, 1, nStr);
			}
		}
		if(mask & SAMPLE_SOLUTION_EH)
		{
			if(eh)
			{
				str = (*env)->NewStringUTF(env, eh);
				jstring nStr = (*env)->NewWeakGlobalRef(env, str);
				(*env)->DeleteLocalRef(env, str);
				(*env)->SetObjectArrayElement(env, stackArray, 2, nStr);
			}
		}

		jobjectArray nArr = (*env)->NewWeakGlobalRef(env, stackArray);
		(*env)->DeleteLocalRef(env, stackArray);
		stackArray = nArr;
	}
	jboolean res = (*env)->CallIntMethod(env, q3eCallbackObj, android_Backtrace_method, num, pid, tid, mask, stackArray);
	LOGI("Backtrace dialog: result -> %d", res);
	backtrace_exit = res ? 1 : 0;
}

static void setup_backtrace(void)
{
	Q3E_BT_Init();
	Q3E_BT_SetupSolution(0xFF);
	Q3E_BT_AfterCaught(backtrace_after_caught_signal);
	Q3E_BT_SignalCaughted(backtrace_signal_caughted);
}

static int loadLib(const char* libpath)
{
	LOGI("Load library: %s......", libpath);
    libdl = dlopen(libpath, RTLD_NOW | RTLD_GLOBAL);
    if(!libdl)
    {
        LOGE("Unable to load library '%s': %s", libpath, dlerror());

		char text[1024];
		snprintf(text, sizeof(text), "Unable to load library '%s': %s", libpath, dlerror());
		copy_to_clipboard(text);
		show_toast(text);

        return 1;
    }
	void (*GetIDTechAPI)(void *);

	GetIDTechAPI = dlsym(libdl, "GetIDTechAPI");
	Q3E_Interface_t d3interface;
	GetIDTechAPI(&d3interface);

	Q3E_PrintInterface(&d3interface);

	qmain = d3interface.main;
	setCallbacks = d3interface.setCallbacks;
	Q3E_SetInitialContext = d3interface.setInitialContext;

	on_pause = d3interface.pause;
	on_resume = d3interface.resume;
	qexit = d3interface.exit;

	set_gl_context = d3interface.setGLContext;
	request_thread_quit = d3interface.requestThreadQuit;

	onKeyEvent = d3interface.keyEvent;
	onAnalog = d3interface.analogEvent;
	onMotionEvent = d3interface.motionEvent;

	return 0;
}

void initAudio(void *buffer, int size)
{
	jobject tmp;

	ATTACH_JNI(env)

	LOGI("Q3E AudioTrack init");
#ifdef AUDIOTRACK_BYTEBUFFER
    tmp = (*env)->NewDirectByteBuffer(env, buffer, size);
#else
	audio_track_buffer = buffer;
	tmp = (*env)->NewByteArray(env, size);
#endif
	audioBuffer = (jobject)(*env)->NewGlobalRef(env, tmp);
	return (*env)->CallVoidMethod(env, q3eCallbackObj, android_initAudio, size);
}

//k NEW: 
// if offset >= 0 and length > 0, only write.
// if offset >= 0 and length < 0, length = -length, then write and flush.
// If offset < 0 and length == 0, only flush.
int writeAudio(int offset, int length)
{
#ifdef AUDIOTRACK_BYTEBUFFER
	if (audioBuffer==0) return 0;
#else
	if (!audio_track_buffer || !audioBuffer)
		return 0;
#endif

	ATTACH_JNI(env)

#ifdef AUDIOTRACK_BYTEBUFFER
    return (*env)->CallIntMethod(env, q3eCallbackObj, android_writeAudio, audioBuffer, offset, length);
#else
	if(offset >= 0 && length != 0)
	{
		int len = abs(length);
#if 0
		jbyte *buf_mem = (*env)->GetByteArrayElements(env, audioBuffer, NULL);
		memcpy(buf_mem, audio_track_buffer, len);
		(*env)->ReleaseByteArrayElements(env, audioBuffer, buf_mem, 0);
#else
		(*env)->SetByteArrayRegion(env, audioBuffer, offset, len, audio_track_buffer);
#endif
	}
	return (*env)->CallIntMethod(env, q3eCallbackObj, android_writeAudio_array, audioBuffer, offset, length);
#endif
}

void closeAudio()
{
	if (!audioBuffer)
		return;

	ATTACH_JNI(env)

	LOGI("Q3E AudioTrack shutdown");
	jobject ab = audioBuffer;
	audioBuffer = 0;
	(*env)->DeleteGlobalRef(env, ab);
}

void setState(int state)
{
	ATTACH_JNI(env)

    (*env)->CallVoidMethod(env, q3eCallbackObj, android_setState, state);
}

static void q3e_exit(void)
{
	LOGI("Q3E JNI exit");
    if(main_thread)
        Q3E_QuitThread(&main_thread, NULL, 1);

	Q3E_FreeArgs();
	Q3E_BT_Shutdown();
	if(Q3E_EventManagerIsInitialized())
    	Q3E_ShutdownEventManager();
	if(game_data_dir)
	{
		free(game_data_dir);
		game_data_dir = NULL;
	}
	if(arg_str)
	{
		free(arg_str);
		arg_str = NULL;
	}
	if(libdl)
	{
		dlclose(libdl);
		libdl = NULL;
	    LOGI("Unload game library");
	}
	Q3E_CloseRedirectOutput();
}

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    jVM = vm;
    if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        LOGE("JNI fatal error");
        return -1;
    }

	atexit(q3e_exit);
	LOGI("JNI loaded %d", JNI_Version);

    return JNI_Version;
}

void JNI_OnUnload(JavaVM *vm, void *reserved)
{
	LOGI("JNI unload");
	JNIEnv *env;
	if((*vm)->GetEnv(vm, (void**) &env, JNI_Version) != JNI_OK)
	{
		LOGI("JNI unload error");
	}
	else
	{
		if(q3eCallbackObj)
		{
			(*env)->DeleteGlobalRef(env, q3eCallbackObj);
			q3eCallbackObj = NULL;
		}
		if(audioBuffer)
		{
			(*env)->DeleteGlobalRef(env, audioBuffer);
			audioBuffer = NULL;
		}
		LOGI("JNI unload done");
	}
	jVM = NULL;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_setCallbackObject(JNIEnv *env, jclass c, jobject obj)
{
    q3eCallbackObj = obj;
    jclass q3eCallbackClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    q3eCallbackObj = (jobject)(*env)->NewGlobalRef(env, obj);
    q3eCallbackClass = (*env)->GetObjectClass(env, q3eCallbackObj);
    
    android_initAudio = (*env)->GetMethodID(env,q3eCallbackClass,"init","(I)V");
    android_writeAudio = (*env)->GetMethodID(env,q3eCallbackClass,"writeAudio","(Ljava/nio/ByteBuffer;II)I");
	android_setState = (*env)->GetMethodID(env,q3eCallbackClass,"setState","(I)V");
	android_writeAudio_array = (*env)->GetMethodID(env,q3eCallbackClass, "writeAudio_array", "([BII)I");
	
	//k
	android_PullEvent_method = (*env)->GetMethodID(env, q3eCallbackClass, "PullEvent", "(I)I");
	android_GrabMouse_method = (*env)->GetMethodID(env, q3eCallbackClass, "GrabMouse", "(Z)V");
	android_SetupSmoothJoystick_method = (*env)->GetMethodID(env, q3eCallbackClass, "SetupSmoothJoystick", "(Z)V");
	android_CopyToClipboard_method = (*env)->GetMethodID(env, q3eCallbackClass, "CopyToClipboard", "(Ljava/lang/String;)V");
	android_GetClipboardText_method = (*env)->GetMethodID(env, q3eCallbackClass, "GetClipboardText", "()Ljava/lang/String;");
	android_ShowToast_method = (*env)->GetMethodID(env, q3eCallbackClass, "ShowToast", "(Ljava/lang/String;)V");
	android_OpenVKB_method = (*env)->GetMethodID(env, q3eCallbackClass, "OpenVKB", "()V");
	android_CloseVKB_method = (*env)->GetMethodID(env, q3eCallbackClass, "CloseVKB", "()V");
	android_OpenURL_method = (*env)->GetMethodID(env, q3eCallbackClass, "OpenURL", "(Ljava/lang/String;)V");
	android_OpenDialog_method = (*env)->GetMethodID(env, q3eCallbackClass, "OpenDialog", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I");
	android_Finish_method = (*env)->GetMethodID(env, q3eCallbackClass, "Finish", "()V");
	android_Backtrace_method = (*env)->GetMethodID(env, q3eCallbackClass, "Backtrace", "(IIII[Ljava/lang/String;)Z");
}

static void setup_Q3E_callback(void)
{
	Q3E_Callback_t callback;
	memset(&callback, 0, sizeof(callback));

	callback.AudioTrack_init = &initAudio;
	callback.AudioTrack_write = &writeAudio;
	callback.AudioTrack_shutdown = &closeAudio;

	callback.Input_grabMouse = &grab_mouse;
	callback.Input_pullEvent = &pull_input_event;
	callback.Input_setupSmoothJoystick = &setup_smooth_joystick;

	callback.Sys_attachThread = &Android_AttachThread;

	callback.set_state = &setState;
	callback.Sys_tmpfile = &android_tmpfile;
	callback.Sys_copyToClipboard = &copy_to_clipboard;
	callback.Sys_getClipboardText = &get_clipboard_text;
	callback.Sys_openKeyboard = &open_keyboard;
	callback.Sys_closeKeyboard = &close_keyboard;
	callback.Sys_openURL = &open_url;
	callback.Sys_exitFinish = &finish;

	callback.Gui_ShowToast = &show_toast;
	callback.Gui_openDialog = &open_dialog;

	Q3E_PrintCallbacks(&callback);

	setCallbacks(&callback);
}

JNIEXPORT jboolean JNICALL Java_com_n0n3m4_q3e_Q3EJNI_init(JNIEnv *env, jclass c, jstring LibPath, jstring nativeLibPath, jint width, jint height, jstring GameDir, jstring gameSubDir, jstring Cmdline, jobject view, jint format, jint depthBits, jint msaa, jint glVersion, jboolean redirectOutputToFile, jint signalsHandler, jboolean bMultithread, jboolean mouseAvailable, jint refreshRate, jstring appHome, jboolean smoothJoystick, jint consoleMaxHeightFrac, jboolean usingExternalLibs, jboolean bContinueNoGLContext)
{
    char **argv;
    int argc;
	jboolean iscopy;

	if(signalsHandler == 2) // using backtrace
	{
		setup_backtrace();
	}

	const char *engineLibPath = (*env)->GetStringUTFChars(env, LibPath, &iscopy);
	LOGI("idTech4a++ engine native library file: %s", engineLibPath);
	int res = loadLib(engineLibPath);
	if(res != 0)
	{
		(*env)->ReleaseStringUTFChars(env, LibPath, engineLibPath);
		return JNI_FALSE; // init fail
	}

    INIT_SDL();
	EXEC_SDL(Q3E_SDL_SetWindowSize, width, height);

	const char *dir = (*env)->GetStringUTFChars(env, GameDir, &iscopy);
    game_data_dir = strdup(dir);
	(*env)->ReleaseStringUTFChars(env, GameDir, dir);

	if(gameSubDir)
	{
		const char *game_type = (*env)->GetStringUTFChars(env, gameSubDir, &iscopy);
		const int Len = strlen(game_data_dir) + 1 + strlen(game_type);
		char *game_path = malloc(Len + 1);
		sprintf(game_path, "%s/%s", game_data_dir, game_type);
		game_path[Len] = '\0';
		free(game_data_dir);
		game_data_dir = game_path;
		(*env)->ReleaseStringUTFChars(env, gameSubDir, game_type);
	}
	chdir(game_data_dir);

	if(redirectOutputToFile)
		Q3E_RedirectOutput();

	LOGI("Load library: %s", engineLibPath);
	(*env)->ReleaseStringUTFChars(env, LibPath, engineLibPath);
	
	LOGI("idTech4A++ game data directory: %s", game_data_dir);

	LOGI("idTech4A++ using %s event queue.", usingNativeEventQueue ? "native" : "Java");
	LOGI("idTech4A++ using %s thread.", usingNativeThread ? "pthread" : "Java");

	const char *arg = (*env)->GetStringUTFChars(env, Cmdline, &iscopy);
	LOGI("idTech4A++ game command: %s", arg);
	argv = malloc(sizeof(char*) * Q3E_MAX_ARGS);
    arg_str = strdup(arg);
	argc = ParseCommandLine(arg_str, argv);
	(*env)->ReleaseStringUTFChars(env, Cmdline, arg);

	setup_Q3E_callback();

	// setResolution(width, height);
	char *doom3_path = NULL;

	const char *native_lib_path = (*env)->GetStringUTFChars(env, nativeLibPath, &iscopy);
	doom3_path = strdup(native_lib_path);
	(*env)->ReleaseStringUTFChars(env, nativeLibPath, native_lib_path);

	const char *app_home = (*env)->GetStringUTFChars(env, appHome, &iscopy);
	char *app_home_dir = strdup(app_home);
	(*env)->ReleaseStringUTFChars(env, appHome, app_home);

	Q3E_DumpArgs(argc, argv);

	mouse_available = mouseAvailable;

	Q3E_InitialContext_t context;
	memset(&context, 0, sizeof(context));

	context.openGL_format = format;
	context.openGL_depth = depthBits;
	context.openGL_msaa = msaa;
	context.openGL_version = glVersion;

	context.nativeLibraryDir = doom3_path;
	context.appHomeDir = app_home_dir;
	context.redirectOutputToFile = redirectOutputToFile ? 1 : 0;
	context.noHandleSignals = signalsHandler ? 1 : 0;
	context.multithread = bMultithread ? 1 : 0;
	context.mouseAvailable = mouseAvailable ? 1 : 0;
	context.continueWhenNoGLContext = bContinueNoGLContext ? 1 : 0;
	context.gameDataDir = game_data_dir;
	context.refreshRate = refreshRate;
	context.smoothJoystick = smoothJoystick ? 1 : 0;
	context.consoleMaxHeightFrac = consoleMaxHeightFrac;
	context.usingExternalLibs = usingExternalLibs;

	window = ANativeWindow_fromSurface(env, view);
	// set_gl_context(window);
	CALL_SDL(onNativeSurfaceCreated);
	CALL_SDL(nativeSetScreenResolution, width, height, width, height, refreshRate);
	CALL_SDL(onNativeResize);

	context.window = window;
	context.width = width;
	context.height = height;

	Q3E_PrintInitialContext(&context);

	Q3E_SetInitialContext(&context);

	if(usingNativeEventQueue)
    	Q3E_InitEventManager(onKeyEvent, onMotionEvent, onAnalog);

    // qmain(argc, argv);

	LOGI("idTech4A++(arm%d) game data directory: %s", sizeof(void *) == 8 ? 64 : 32, game_data_dir);

	free(argv);
    free(doom3_path);
	free(app_home_dir);

	return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendKeyEvent(JNIEnv *env, jclass c, jint state, jint key, jint chr)
{
    onKeyEvent(state,key,chr);
	EXEC_SDL(Q3E_SDL_KeyEvent, key, state);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendAnalog(JNIEnv *env, jclass c, jint enable, jfloat x, jfloat y)
{
    onAnalog(enable,x,y);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendMotionEvent(JNIEnv *env, jclass c, jfloat x, jfloat y)
{
    onMotionEvent(x, y);
	EXEC_SDL(Q3E_SDL_MotionEvent, x, y);
}

JNIEXPORT jboolean JNICALL Java_com_n0n3m4_q3e_Q3EJNI_Is64(JNIEnv *env, jclass c)
{    
    return sizeof(void *) == 8 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_OnPause(JNIEnv *env, jclass clazz)
{
	if(on_pause)
		on_pause();
	CALL_SDL(nativePause);
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_OnResume(JNIEnv *env, jclass clazz)
{
	if(on_resume)
    	on_resume();
	CALL_SDL(nativeResume);
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_SetSurface(JNIEnv *env, jclass clazz, jobject view) {
	if(window)
	{
		window = NULL;
	}
	if(view)
	{
		window = ANativeWindow_fromSurface(env, view);
	}
	set_gl_context(window);
	if(window)
	{
		CALL_SDL(onNativeSurfaceCreated);
		CALL_SDL(onNativeSurfaceChanged);
	}
	else
	{
		CALL_SDL(onNativeSurfaceDestroyed);
	}
}

void finish(void)
{
	ATTACH_JNI(env)

	LOGI("Finish");
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_Finish_method);
}

static void * Q3E_MainLoop(void *data)
{
	LOGI("Enter native game main thread.");
	Android_AttachThread();
	q3e_pthread_cancelable();
	resultCode = qmain(q3e_argc, q3e_argv);
	main_thread = 0;
    /*Q3E_exit();*/
	LOGI("Leave native game main thread.");
	finish();
	return (void *)(intptr_t)resultCode;
}

// start game main thread from Android Surface thread(call by Surface view thread)
static void Q3E_StartGameMainThread(void)
{
	if(main_thread)
		return;

	int res = Q3E_CreateThread(&main_thread, Q3E_MainLoop, NULL);
	if(main_thread < 0)
	{
	    exit(res);
	}
}

// shutdown game main thread(call by Java main thread)
static void Q3E_ShutdownGameMainThread(void)
{
	if(!main_thread)
		return;

	LOGI("Stop native game main thread......");
	//request_thread_quit();
	Q3E_QuitThread(&main_thread, NULL, 0);
}

// call game main thread(call by Java game thread or Surface view thread)
JNIEXPORT jint JNICALL Java_com_n0n3m4_q3e_Q3EJNI_main(JNIEnv *env, jclass clazz)
{
	LOGI("Enter java game main thread.");
	resultCode = qmain(q3e_argc, q3e_argv);
	LOGI("Leave java game main thread: %d.", resultCode);
	return resultCode;
}

JNIEXPORT jlong JNICALL Java_com_n0n3m4_q3e_Q3EJNI_StartThread(JNIEnv *env, jclass clazz)
{
    Q3E_StartGameMainThread();
	return (jlong)main_thread;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_StopThread(JNIEnv *env, jclass clazz)
{
	Q3E_ShutdownGameMainThread();
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_NotifyExit(JNIEnv *env, jclass clazz)
{
	request_thread_quit();
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_shutdown(JNIEnv *env, jclass c)
{
    LOGI("Shutdown result code: %d", resultCode);
	if(resultCode < 0)
	{
		request_thread_quit();
		if(qexit)
		{
			qexit();
		}
	}
}

int pull_input_event(int num)
{
	ATTACH_JNI(env)

    jint jres = (*env)->CallIntMethod(env, q3eCallbackObj, android_PullEvent_method, (jint)num);
	if(usingNativeEventQueue)
		return Q3E_PullEvent(num);
	else
		return jres;
}

void grab_mouse(int grab)
{
	ATTACH_JNI(env)

	if(mouse_available)
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_GrabMouse_method, (jboolean)grab);
	EXEC_SDL(Q3E_SDL_SetRelativeMouseMode, grab);
}

void copy_to_clipboard(const char *text)
{
	ATTACH_JNI(env)

	if(!text)
	{
		(*env)->CallVoidMethod(env, q3eCallbackObj, android_CopyToClipboard_method, NULL);
		return;
	}

	LOGI("Copy clipboard: %s", text);
	jstring str = (*env)->NewStringUTF(env, text);
	jstring nstr = (*env)->NewWeakGlobalRef(env, str);
	(*env)->DeleteLocalRef(env, str);
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_CopyToClipboard_method, nstr);
}

char * get_clipboard_text(void)
{
	ATTACH_JNI(env)

	jstring str = (*env)->CallObjectMethod(env, q3eCallbackObj, android_GetClipboardText_method);
	if(!str)
		return NULL;

	LOGI("Read clipboard");
	const char *nstr = (*env)->GetStringUTFChars(env, str, NULL);
	char *res = NULL;
	if(nstr)
		res = strdup(nstr);
	(*env)->ReleaseStringUTFChars(env, str, nstr);
	return res;
}

void show_toast(const char *text)
{
	if(!text)
		return;

	ATTACH_JNI(env)

	LOGI("Toast: %s", text);
	jstring str = (*env)->NewStringUTF(env, text);
	jstring nstr = (*env)->NewWeakGlobalRef(env, str);
	(*env)->DeleteLocalRef(env, str);
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_ShowToast_method, nstr);
}

void open_keyboard(void)
{
	ATTACH_JNI(env)

	LOGI("Open keyboard");
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_OpenVKB_method);
}

void close_keyboard(void)
{
	ATTACH_JNI(env)

	LOGI("Close keyboard");
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_CloseVKB_method);
}

void open_url(const char *url)
{
	ATTACH_JNI(env)

	if(!url)
	{
		return;
	}

	LOGI("Open URL: %s", url);
	jstring str = (*env)->NewStringUTF(env, url);
	jstring nstr = (*env)->NewWeakGlobalRef(env, str);
	(*env)->DeleteLocalRef(env, str);
	(*env)->CallVoidMethod(env, q3eCallbackObj, android_OpenURL_method, nstr);
}

int open_dialog(const char *title, const char *message, int num, const char *buttons[])
{
	ATTACH_JNI(env)

	if(!title || !message)
	{
		return -1;
	}

	LOGI("Open Dialog: %s\n%s", title, message);
	jstring str = (*env)->NewStringUTF(env, title);
	jstring titleStr = (*env)->NewWeakGlobalRef(env, str);
	(*env)->DeleteLocalRef(env, str);
	str = (*env)->NewStringUTF(env, message);
	jstring messageStr = (*env)->NewWeakGlobalRef(env, str);
	(*env)->DeleteLocalRef(env, str);
	jobjectArray buttonArray = NULL;
	jclass stringClazz = (*env)->FindClass(env, "java/lang/String");
	LOGI("Open Dialog: num buttons -> %d", num);
	if(num > 0)
	{
		buttonArray = (*env)->NewObjectArray(env, num, stringClazz, NULL);
		int i;

		for(i = 0; i < num; i++)
		{
			LOGI("Open Dialog: button %d -> %s", i, buttons[i] ? buttons[i] : "");
			if(!buttons[i])
				continue;
			str = (*env)->NewStringUTF(env, buttons[i]);
			jstring nStr = (*env)->NewWeakGlobalRef(env, str);
			(*env)->DeleteLocalRef(env, str);
			(*env)->SetObjectArrayElement(env, buttonArray, i, nStr);
		}

		jobjectArray nArr = (*env)->NewWeakGlobalRef(env, buttonArray);
		(*env)->DeleteLocalRef(env, buttonArray);
		buttonArray = nArr;
	}
	jint res = (*env)->CallIntMethod(env, q3eCallbackObj, android_OpenDialog_method, titleStr, messageStr, buttonArray);
	LOGI("Open Dialog: result -> %d", res);

	return res;
}

void setup_smooth_joystick(int enable)
{
	ATTACH_JNI(env)

	(*env)->CallVoidMethod(env, q3eCallbackObj, android_SetupSmoothJoystick_method, (jboolean)enable);
}

#define TMPFILE_NAME "idtech4amm_harmattan_tmpfile_XXXXXX"
FILE * android_tmpfile(void)
{
    const int Len = strlen(game_data_dir) + 1 + strlen(TMPFILE_NAME) + 1;
	char *tmp_file = malloc(Len);
    memset(tmp_file, 0, Len);
    sprintf(tmp_file, "%s/%s", game_data_dir, TMPFILE_NAME);
	int fd = mkstemp(tmp_file);
	if(fd == -1)
	{
		LOGE("Call mkstemp(%s) error: %s", tmp_file, strerror(errno));
		free(tmp_file);
		return NULL;
	}

	FILE *res = fdopen(fd, "w+b");
	if(!res)
	{
		LOGE("Call fdopen(%d) error: %s", fd, strerror(errno));
		close(fd);
		free(tmp_file);
		return NULL;
	}
	unlink(tmp_file);
	LOGD("android_tmpfile create: %s", tmp_file);
	free(tmp_file);
	return res;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_PushKeyEvent(JNIEnv *env, jclass clazz, jint down, jint keycode, jint charcode)
{
    Q3E_PushKeyEvent(down, keycode, charcode);
	EXEC_SDL(Q3E_SDL_KeyEvent, keycode, down);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_PushMotionEvent(JNIEnv *env, jclass clazz, jfloat deltax, jfloat deltay)
{
    Q3E_PushMotionEvent(deltax, deltay);
	EXEC_SDL(Q3E_SDL_MotionEvent, deltax, deltay);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_PushAnalogEvent(JNIEnv *env, jclass c, jint enable, jfloat x, jfloat y)
{
    Q3E_PushAnalogEvent(enable, x, y);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_PreInit(JNIEnv *env, jclass clazz, jint eventQueueType, jint gameThreadType)
{
	usingNativeEventQueue = eventQueueType != EVENT_QUEUE_TYPE_JAVA;
	usingNativeThread = gameThreadType != GAME_THREAD_TYPE_JAVA;
}

#ifdef _Q3E_SDL
#include "q3esdl2.c"
#endif