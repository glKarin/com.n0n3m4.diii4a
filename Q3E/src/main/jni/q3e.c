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

int  (*qmain)(int argc, char **argv);
void (*onFrame)();
void (*onKeyEvent)(int state, int key,int chr);
void (*onAnalog)(int enable, float x, float y);
void (*onMotionEvent)(float x, float y);
void (*onAudio)();
void (*setCallbacks)(void *func, void *func2, void *func3);
void (*setResolution)(int width, int height);
void (*vidRestart)();
void (*on_pause)(void);
void (*on_resume)(void);
void (*set_gl_context)(ANativeWindow *window, int size, ...);

intptr_t (*Q3E_Call)(int protocol, int size, ...);
static void pull_input_event(int execCmd);
static void grab_mouse(int grab);
static FILE * android_tmpfile(void);
static char *game_data_dir = NULL;
static int redirect_output_to_file = 0;
static int no_handle_signals = 0;
static int multithread = 0;
void (*qexit)(void);
intptr_t (*(*set_Android_Call)(intptr_t (*func)(int, int, ...)))(int, int, ...);

static jmethodID android_PullEvent_method;
jmethodID android_GrabMouse_method;

jmethodID android_initAudio;
jmethodID android_writeAudio;
jmethodID android_setState;

static JavaVM *jVM;
static jobject audioBuffer=0;
static jobject q3eCallbackObj=0;
static void *libdl;
static ANativeWindow *window = NULL;

#define ANDROID_CALL_PROTOCOL_TMPFILE 0x10001
#define ANDROID_CALL_PROTOCOL_PULL_INPUT_EVENT 0x10002
#define ANDROID_CALL_PROTOCOL_ATTACH_THREAD 0x10003
#define ANDROID_CALL_PROTOCOL_GRAB_MOUSE 0x10005

#define ANDROID_CALL_PROTOCOL_NATIVE_LIBRARY_DIR 0x20001
#define ANDROID_CALL_PROTOCOL_REDIRECT_OUTPUT_TO_FILE 0x20002
#define ANDROID_CALL_PROTOCOL_NO_HANDLE_SIGNALS 0x20003
#define ANDROID_CALL_PROTOCOL_MULTITHREAD 0x20005
#define ANDROID_CALL_PROTOCOL_SYS_PULL_INPUT_EVENT 0x20006

intptr_t Android_Call(int protocol, int size, ...)
{
	intptr_t res = 0;
	va_list va;
	JNIEnv *env = 0;

	va_start(va, size);
	switch(protocol)
	{
		case ANDROID_CALL_PROTOCOL_TMPFILE:
			res = (intptr_t)android_tmpfile();
			break;
		case ANDROID_CALL_PROTOCOL_PULL_INPUT_EVENT:
			pull_input_event(va_arg(va, int));
			res = 1;
			break;
		case ANDROID_CALL_PROTOCOL_ATTACH_THREAD:
			if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4)) < 0)
			{
				(*jVM)->AttachCurrentThread(jVM, &env, NULL);
			}
			res = 1;
			break;
		case ANDROID_CALL_PROTOCOL_GRAB_MOUSE:
			grab_mouse(va_arg(va, int));
			res = 1;
			break;
		default:
			break;
	}
	va_end(va);

	return res;
}

static void loadLib(char* libpath)
{
    libdl = dlopen(libpath, RTLD_NOW | RTLD_GLOBAL);
    if(!libdl)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Q3E_JNI", "Unable to load library: %s\n", dlerror());
        return;
    }
    qmain = dlsym(libdl, "main");
    onFrame = dlsym(libdl, "Q3E_DrawFrame");
    onKeyEvent = dlsym(libdl, "Q3E_KeyEvent");
    onAnalog = dlsym(libdl, "Q3E_Analog");
    onMotionEvent = dlsym(libdl, "Q3E_MotionEvent");
    onAudio = dlsym(libdl, "Q3E_GetAudio");
    setCallbacks = dlsym(libdl, "Q3E_SetCallbacks");
    setResolution = dlsym(libdl, "Q3E_SetResolution");
	vidRestart = dlsym(libdl, "Q3E_OGLRestart");

    on_pause = dlsym(libdl, "Q3E_OnPause");
    on_resume = dlsym(libdl, "Q3E_OnResume");
    Q3E_Call = dlsym(libdl, "Q3E_Call");
    qexit = dlsym(libdl, "Q3E_exit");
	set_Android_Call = dlsym(libdl, "set_Android_Call");
	set_gl_context = dlsym(libdl, "Android_SetGLContext");
}

void initAudio(void *buffer, int size)
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }
    tmp = (*env)->NewDirectByteBuffer(env, buffer, size);
    audioBuffer = (jobject)(*env)->NewGlobalRef(env, tmp);
    return (*env)->CallVoidMethod(env, q3eCallbackObj, android_initAudio, size);
}

//k NEW: 
// if offset >= 0 and length > 0, only write.
// if offset >= 0 and length < 0, length = -length, then write and flush.
// If length == 0 and offset < 0, only flush.
int writeAudio(int offset, int length)
{
	if (audioBuffer==0) return 0;
    JNIEnv *env;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
    	(*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }
    return (*env)->CallIntMethod(env, q3eCallbackObj, android_writeAudio, audioBuffer, offset, length);
}

void setState(int state)
{
    JNIEnv *env;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }
    //(*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    (*env)->CallVoidMethod(env, q3eCallbackObj, android_setState, state);
}

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    jVM = vm;
    if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Q3E_JNI", "JNI fatal error");
        return -1;
    }

    return JNI_VERSION_1_4;
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
	
	//k
	android_PullEvent_method = (*env)->GetMethodID(env, q3eCallbackClass, "PullEvent", "(Z)V");
	android_GrabMouse_method = (*env)->GetMethodID(env, q3eCallbackClass, "GrabMouse", "(Z)V");
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


JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_init(JNIEnv *env, jclass c, jstring LibPath, jint width, jint height, jstring GameDir, jstring Cmdline, jobject view, jint format, jint msaa)
{
    char **argv;
    int argc=0;
	jboolean iscopy;
	const char *dir = (*env)->GetStringUTFChars(
                env, GameDir, &iscopy);
    const char *arg = (*env)->GetStringUTFChars(
                env, Cmdline, &iscopy);	
    game_data_dir = strdup(dir);
	chdir(game_data_dir);
	(*env)->ReleaseStringUTFChars(env, GameDir, dir);
	argv = malloc(sizeof(char*) * 255);
    char *arg_str = strdup(arg);
	argc = ParseCommandLine(arg_str, argv);	
    //free(arg_str); //TODO: not free
	(*env)->ReleaseStringUTFChars(env, Cmdline, arg);    
	
	const char *libpath = (*env)->GetStringUTFChars(
                env, LibPath, &iscopy);	
    char *doom3_path = strdup(libpath);
	loadLib(doom3_path);
	(*env)->ReleaseStringUTFChars(env, LibPath, libpath);    

    setCallbacks(&initAudio,&writeAudio,&setState);    
    setResolution(width, height);
    
	set_Android_Call(Android_Call);
    __android_log_print(ANDROID_LOG_INFO, "Q3E_JNI", "DOOM3 native library file: %s", doom3_path);
    if(Q3E_Call)
    {
        char *ptr = strrchr(doom3_path, '/');
        if(ptr) *ptr = '\0';
        __android_log_print(ANDROID_LOG_INFO, "Q3E_JNI", "DOOM3 native library dir: %s", doom3_path);
        (void)Q3E_Call(ANDROID_CALL_PROTOCOL_NATIVE_LIBRARY_DIR, 1, doom3_path);
        __android_log_print(ANDROID_LOG_INFO, "Q3E_JNI", "DOOM3 redirect output to file: %d", redirect_output_to_file);
        (void)Q3E_Call(ANDROID_CALL_PROTOCOL_REDIRECT_OUTPUT_TO_FILE, 1, redirect_output_to_file);
        __android_log_print(ANDROID_LOG_INFO, "Q3E_JNI", "DOOM3 no handle signals: %d", no_handle_signals);
        (void)Q3E_Call(ANDROID_CALL_PROTOCOL_NO_HANDLE_SIGNALS, 1, no_handle_signals);
		__android_log_print(ANDROID_LOG_INFO, "Q3E_JNI", "DOOM3 multi-thread: %d", multithread);
		(void)Q3E_Call(ANDROID_CALL_PROTOCOL_MULTITHREAD, 1, multithread);

		(void)Q3E_Call(ANDROID_CALL_PROTOCOL_SYS_PULL_INPUT_EVENT, 1, pull_input_event);
    }
	window = ANativeWindow_fromSurface(env, view);
	set_gl_context(window, 2, format, msaa);
    
    qmain(argc, argv);
	free(argv);
    free(doom3_path);
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

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_requestAudioData(JNIEnv *env, jclass c)
{
    onAudio();
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

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_SetRedirectOutputToFile(JNIEnv *env, jclass c, jboolean enabled)
{
	redirect_output_to_file = enabled ? 1 : 0;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_SetNoHandleSignals(JNIEnv *env, jclass c, jboolean enabled)
{
    no_handle_signals = enabled ? 1 : 0;
}

JNIEXPORT void JNICALL Java_com_n0n3m4_q3e_Q3EJNI_SetMultiThread(JNIEnv *env, jclass c, jboolean enabled)
{
	multithread = enabled ? 1 : 0;
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_OnPause(JNIEnv *env, jclass clazz)
{
    on_pause();
}

JNIEXPORT void JNICALL
Java_com_n0n3m4_q3e_Q3EJNI_OnResume(JNIEnv *env, jclass clazz)
{
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
	set_gl_context(window, 0);
}

void pull_input_event(int execCmd)
{
    JNIEnv *env = 0;

    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    (*env)->CallVoidMethod(env, q3eCallbackObj, android_PullEvent_method, (jboolean)execCmd);
}

void grab_mouse(int grab)
{
	JNIEnv *env = 0;

	if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
	{
		(*jVM)->AttachCurrentThread(jVM,&env, NULL);
	}

	(*env)->CallVoidMethod(env, q3eCallbackObj, android_GrabMouse_method, (jboolean)grab);
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
		__android_log_print(ANDROID_LOG_ERROR, "Q3E_JNI", "Call mkstemp(%s) error: %s", tmp_file, strerror(errno));
		free(tmp_file);
		return NULL;
	}

	FILE *res = fdopen(fd, "w+b");
	if(!res)
	{
		__android_log_print(ANDROID_LOG_ERROR, "Q3E_JNI", "Call fdopen(%d) error: %s", fd, strerror(errno));
		close(fd);
		free(tmp_file);
		return NULL;
	}
	unlink(tmp_file);
    __android_log_print(ANDROID_LOG_DEBUG, "Q3E_JNI", "itmpfile create: %s", tmp_file);
	free(tmp_file);
	return res;
}
