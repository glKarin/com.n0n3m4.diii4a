#include <jni.h>

#include "q3eenv.h"

#include <stdio.h>

#include <pthread.h>
#include <android/log.h>

#include "q3estd.h"

#define LOG_TAG "Q3E::Env"

extern JavaVM *jVM;

static pthread_key_t env_thread_key;

/* Set local storage value */
static void Q3E_SetEnv(JNIEnv *env)
{
    /*int status = */pthread_setspecific(env_thread_key, env);
/*    if (status < 0) {
        LOGE("[SDL]: Failed pthread_setspecific() in Q3E_SetEnv() (err=%d)", status);
    }
    return status;*/
    //LOGD("[SDL]: Set JNIEnv thread key %zu: %p -> %d", (uintptr_t)env_thread_key, env, status);
}

/* Get local storage value */
JNIEnv * Q3E_GetEnv(void)
{
    /* Get JNIEnv from the Thread local storage */
    JNIEnv *env = pthread_getspecific(env_thread_key);
    if (!env) {
        /* If it fails, try to attach ! (e.g the thread isn't created with SDL_CreateThread() */
        //int status;

        /* There should be a JVM */
/*        if (!jVM) {
            LOGE("[SDL]: Failed, there is no JavaVM");
            return NULL;
        }*/

        /* Attach the current thread to the JVM and get a JNIEnv.
         * It will be detached by pthread_create destructor 'Android_JNI_ThreadDestroyed' */
        if ( ((*jVM)->GetEnv(jVM, (void**) &env, JNI_Version)) < 0 )
        {
            /*status = */(*jVM)->AttachCurrentThread(jVM, &env, NULL);
        }
//        if (env/*status < 0*/) {
//            LOGE("[SDL]: Failed to attach current thread (err=%d)", status);
//            return NULL;
//        }

        /* Save JNIEnv into the Thread local storage */
        Q3E_SetEnv(env);
    }
    //else LOGD("[SDL]: Get JNIEnv thread key %zu: %p", (uintptr_t)env_thread_key, env);

    return env;
}

/* Destructor called for each thread where env_thread_key is not NULL */
static void Q3E_ThreadDestroyed(void *value)
{
    /* The thread is being destroyed, detach it from the Java VM and set the env_thread_key value to NULL as required */
    JNIEnv *env = (JNIEnv *)value;
    if (env) {
        (*jVM)->DetachCurrentThread(jVM);
        Q3E_SetEnv(NULL);
        //LOGD("[SDL]: Destroy JNIEnv thread key %zu: %p", (uintptr_t)env_thread_key, env);
    }
}

/* Creation of local storage env_thread_key */
static void Q3E_CreateEnvKey(void)
{
    int status = pthread_key_create(&env_thread_key, Q3E_ThreadDestroyed);
    if (status < 0) {
        LOGE("[SDL]: Error initializing JNIEnv thread key with pthread_key_create() (err=%d)", status);
    }
    //else LOGD("[SDL]: Initializing JNIEnv thread key with pthread_key_create");
}

void Q3E_InitEnvKey(void)
{
    static pthread_once_t key_once = PTHREAD_ONCE_INIT;
    int status = pthread_once(&key_once, Q3E_CreateEnvKey);
    if (status < 0) {
        LOGE("[SDL]: Error initializing JNIEnv thread key with pthread_once() (err=%d)", status);
    }
    //else LOGD("[SDL]: Initializing JNIEnv thread key with pthread_once()");
}

void Q3E_DestroyEnvKey(void)
{
    int status = pthread_key_delete(env_thread_key);
    if (status < 0) {
        LOGE("[SDL]: Error destroy JNIEnv thread key with pthread_key_delete() (err=%d)", status);
    }
    //else LOGD("[SDL]: Destroy JNIEnv thread key with pthread_key_delete()");
}