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

#include "q3e.h"

#include "doom3/neo/sys/android/sys_android.h"

#define LOG_TAG "Q3E_JNI"

#define JNI_Version JNI_VERSION_1_4

#define LOGD(fmt, args...) { printf("[" LOG_TAG " debug]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args); }
#define LOGI(fmt, args...) { printf("[" LOG_TAG " info]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); }
#define LOGE(fmt, args...) { printf("[" LOG_TAG " error]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args); }

//#define AUDIOTRACK_BYTEBUFFER 1

// call DOOM3
static void (*setResolution)(int width, int height);
static void (*Q3E_SetInitialContext)(const void *context);
static void (*setCallbacks)(const void *func);
static void (*set_gl_context)(ANativeWindow *window);

static int  (*qmain)(int argc, char **argv);
static void (*onFrame)();
static void (*onKeyEvent)(int state, int key,int chr);
static void (*onAnalog)(int enable, float x, float y);
static void (*onMotionEvent)(float x, float y);
static void (*vidRestart)();
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

// data
static char *game_data_dir = NULL;
static char *arg_str = NULL;

static void *libdl;
static ANativeWindow *window = NULL;

// Java object ref
static JavaVM *jVM;
static jobject audioBuffer=0;
static jobject q3eCallbackObj=0;
static const jbyte *audio_track_buffer = NULL;

// Java method
static jmethodID android_PullEvent_method;
static jmethodID android_GrabMouse_method;
static jmethodID android_CopyToClipboard_method;
static jmethodID android_GetClipboardText_method;

static jmethodID android_initAudio;
static jmethodID android_writeAudio;
static jmethodID android_setState;
static jmethodID android_writeAudio_array;

static jmethodID android_ShowToast_method;
static jmethodID android_OpenVKB_method;
static jmethodID android_CloseVKB_method;

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

static void print_interface(void)
{
	LOGI("idTech4A++ interface ---------> ");

	LOGI("Main function: %p", qmain);
	LOGI("Setup callbacks: %p", setCallbacks);
	LOGI("Setup initial context: %p", Q3E_SetInitialContext);
	LOGI("Setup resolution: %p", setResolution);
	LOGI("On pause: %p", on_pause);
	LOGI("On resume: %p", on_resume);
	LOGI("Exit function: %p", qexit);

	LOGI("Setup OpenGL context: %p", set_gl_context);

	LOGI("On frame: %p", onFrame);
	LOGI("Restart OpenGL: %p", vidRestart);

	LOGI("Key event: %p", onKeyEvent);
	LOGI("Analog event: %p", onAnalog);
	LOGI("Motion event: %p", onMotionEvent);

	LOGI("<---------");
}

static int loadLib(const char* libpath)
{
	LOGI("Load library: %s......\n", libpath);
    libdl = dlopen(libpath, RTLD_NOW | RTLD_GLOBAL);
    if(!libdl)
    {
        LOGE("Unable to load library '%s': %s\n", libpath, dlerror());

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

	qmain = d3interface.main;
	setCallbacks = d3interface.setCallbacks;
	Q3E_SetInitialContext = d3interface.setInitialContext;
	setResolution = d3interface.setResolution;

	on_pause = d3interface.pause;
	on_resume = d3interface.resume;
	qexit = d3interface.exit;

	set_gl_context = d3interface.setGLContext;

	onFrame = d3interface.frame;
	vidRestart = d3interface.vidRestart;

	onKeyEvent = d3interface.keyEvent;
	onAnalog = d3interface.analogEvent;
	onMotionEvent = d3interface.motionEvent;

	print_interface();

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
	}
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
	android_CopyToClipboard_method = (*env)->GetMethodID(env, q3eCallbackClass, "CopyToClipboard", "(Ljava/lang/String;)V");
	android_GetClipboardText_method = (*env)->GetMethodID(env, q3eCallbackClass, "GetClipboardText", "()Ljava/lang/String;");
	android_ShowToast_method = (*env)->GetMethodID(env, q3eCallbackClass, "ShowToast", "(Ljava/lang/String;)V");
	android_OpenVKB_method = (*env)->GetMethodID(env, q3eCallbackClass, "OpenVKB", "()V");
	android_CloseVKB_method = (*env)->GetMethodID(env, q3eCallbackClass, "CloseVKB", "()V");
}

static void UnEscapeQuotes( char *arg )
{
	char *last = NULL;
	while( *arg ) {
		if( *arg == '"' && *last == '\\' ) {
			char *c_curr = arg;
			char *c_last = last;
			while( *c_curr ) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;
	argc = last_argc = 0;
	for ( bufp = cmdline; *bufp; ) {		
		while ( isspace(*bufp) ) {
			++bufp;
		}		
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}			
			while ( *bufp && ( *bufp != '"' || *lastp == '\\' ) ) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}			
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
		if( argv && last_argc != argc ) {
			UnEscapeQuotes( argv[last_argc] );	
		}
		last_argc = argc;	
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
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

	callback.Sys_attachThread = &Android_AttachThread;

	callback.set_state = &setState;
	callback.Sys_tmpfile = &android_tmpfile;
	callback.Sys_copyToClipboard = &copy_to_clipboard;
	callback.Sys_getClipboardText = &get_clipboard_text;
	callback.Sys_openKeyboard = &open_keyboard;
	callback.Sys_closeKeyboard = &close_keyboard;

	callback.Gui_ShowToast = &show_toast;

	setCallbacks(&callback);
}

static void print_initial_context(const Q3E_InitialContext_t *context)
{
	LOGI("idTech4A++ initial context ---------> ");

	LOGI("Native library directory: %s", context->nativeLibraryDir);
	LOGI("Redirect output to file: %d", context->redirectOutputToFile);
	LOGI("Don't handle signals: %d", context->noHandleSignals);
	LOGI("Enable multi-thread: %d", context->multithread);
	LOGI("OpenGL format: 0x%X", context->openGL_format);
	LOGI("OpenGL MSAA: %d", context->openGL_msaa);
	LOGI("OpenGL Version: %x", context->openGL_version);
	LOGI("Using mouse: %x", context->mouseAvailable);
	LOGI("Game data directory: %s", context->gameDataDir);
	LOGI("Refresh rate: %d", context->refreshRate);
    LOGI("Continue when missing OpenGL context: %d", context->continueWhenNoGLContext);

	LOGI("<---------");
}

JNIEXPORT jboolean JNICALL Java_com_n0n3m4_q3e_Q3EJNI_init(JNIEnv *env, jclass c, jstring LibPath, jstring nativeLibPath, jint width, jint height, jstring GameDir, jstring gameSubDir, jstring Cmdline, jobject view, jint format, jint msaa, jint glVersion, jboolean redirectOutputToFile, jboolean noHandleSignals, jboolean bMultithread, jboolean mouseAvailable, jint refreshRate, jboolean bContinueNoGLContext)
{
    char **argv;
    int argc;
	jboolean iscopy;

	const char *engineLibPath = (*env)->GetStringUTFChars(env, LibPath, &iscopy);
	LOGI("idTech4a++ engine native library file: %s", engineLibPath);
	int res = loadLib(engineLibPath);
	(*env)->ReleaseStringUTFChars(env, LibPath, engineLibPath);
	if(res != 0)
	{
		return JNI_FALSE; // init fail
	}

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
	LOGI("idTech4A++ game data directory: %s\n", game_data_dir);
	chdir(game_data_dir);

	const char *arg = (*env)->GetStringUTFChars(env, Cmdline, &iscopy);
	LOGI("idTech4A++ game command: %s\n", arg);
	argv = malloc(sizeof(char*) * 255);
    arg_str = strdup(arg);
	argc = ParseCommandLine(arg_str, argv);
	(*env)->ReleaseStringUTFChars(env, Cmdline, arg);

	setup_Q3E_callback();

	setResolution(width, height);
	char *doom3_path = NULL;

	const char *native_lib_path = (*env)->GetStringUTFChars(env, nativeLibPath, &iscopy);
	doom3_path = strdup(native_lib_path);
	(*env)->ReleaseStringUTFChars(env, nativeLibPath, native_lib_path);
	Q3E_InitialContext_t context;
	memset(&context, 0, sizeof(context));

	context.openGL_format = format;
	context.openGL_msaa = msaa;
	context.openGL_version = glVersion;

	context.nativeLibraryDir = doom3_path;
	context.redirectOutputToFile = redirectOutputToFile ? 1 : 0;
	context.noHandleSignals = noHandleSignals ? 1 : 0;
	context.multithread = bMultithread ? 1 : 0;
	context.mouseAvailable = mouseAvailable ? 1 : 0;
	context.continueWhenNoGLContext = bContinueNoGLContext ? 1 : 0;
	context.gameDataDir = game_data_dir;
	context.refreshRate = refreshRate;

	print_initial_context(&context);

	Q3E_SetInitialContext(&context);

	window = ANativeWindow_fromSurface(env, view);
	set_gl_context(window);
    
    qmain(argc, argv);

	LOGI("idTech4A++ game data directory: %s\n", game_data_dir);

	free(argv);
    free(doom3_path);

	return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_drawFrame(JNIEnv *env, jclass c)
{
    onFrame();
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendKeyEvent(JNIEnv *env, jclass c, jint state, jint key, jint chr)
{
    onKeyEvent(state,key,chr);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendAnalog(JNIEnv *env, jclass c, jint enable, jfloat x, jfloat y)
{
    onAnalog(enable,x,y);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_sendMotionEvent(JNIEnv *env, jclass c, jfloat x, jfloat y)
{
    onMotionEvent(x, y);
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_vidRestart(JNIEnv *env, jclass c)
{    
	vidRestart();
}


JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_shutdown(JNIEnv *env, jclass c)
{
    if(qexit)  
        qexit();
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
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_OnResume(JNIEnv *env, jclass clazz)
{
	if(on_resume)
    	on_resume();
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
}

int pull_input_event(int num)
{
	ATTACH_JNI(env)

    return (*env)->CallIntMethod(env, q3eCallbackObj, android_PullEvent_method, (jint)num);
}

void grab_mouse(int grab)
{
	ATTACH_JNI(env)

	(*env)->CallVoidMethod(env, q3eCallbackObj, android_GrabMouse_method, (jboolean)grab);
}

void copy_to_clipboard(const char *text)
{
	ATTACH_JNI(env)

	if(!text)
	{
		(*env)->CallVoidMethod(env, q3eCallbackObj, android_CopyToClipboard_method, NULL);
		return;
	}

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
