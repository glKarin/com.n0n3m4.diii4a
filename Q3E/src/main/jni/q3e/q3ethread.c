#include "q3ethread.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <android/log.h>

#include "q3estd.h"

#define LOG_TAG "Q3E::Thread"

#include <dlfcn.h>
typedef int (*PROC_pthread_getattr_np)(pthread_t __pthread, pthread_attr_t* __attr);
typedef int (*PROC_pthread_setname_np)(pthread_t __pthread, const char* __name);
// __ANDROID_API__ >= 26
typedef int (*PROC_pthread_getname_np)(pthread_t __pthread, char* __buf, size_t __n);
static PROC_pthread_getattr_np pthread_getattr_np_f;
static PROC_pthread_setname_np pthread_setname_np_f;
static PROC_pthread_getname_np pthread_getname_np_f;
static _Bool proc_init = 0;

static pthread_mutex_t global_lock;
static pthread_mutex_t cond_lock;
static pthread_cond_t event_cond;
static _Bool waiting = 0;
static _Bool signaled = 0;

int q3e_pthread_cancel(pthread_t pthread_id)
{
    int status;
    if ( (status = pthread_kill(pthread_id, Q3E_THREAD_CANCEL_SIG) ) != 0)
        LOGE("Error cancelling thread %zu, error = %d", PTHREAD_ID_WRAP(pthread_id), status)
    else
        LOGI("pthread_cancel: thread=%zu", PTHREAD_ID_WRAP(pthread_id))
    return status;
}

static void thread_exit_handler(int sig)
{
    pthread_t tid = pthread_self();
    LOGI("pthread_exit: thread=%zu, signal=%d", PTHREAD_ID_WRAP(tid), sig);
    pthread_exit(NULL);
    LOGI("Thread exit: %zu", PTHREAD_ID_WRAP(tid));
}

int q3e_pthread_cancelable(void)
{
#if 0
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(Q3E_THREAD_CANCEL_SIG, &actions, NULL);
#else
	signal(Q3E_THREAD_CANCEL_SIG, thread_exit_handler);
#endif
    return 0;
}

static void Q3E_InitPthreadProc(void)
{
	if(proc_init)
		return;
#define PTHREAD_GETPROC(x) x##_f = (PROC_##x)dlsym(RTLD_DEFAULT, #x); \
	if(x##_f) { LOGI("pthread proc " #x ": %p", x##_f); } \
	else { LOGI("pthread proc " #x ": not found"); }

	PTHREAD_GETPROC(pthread_getattr_np)
	PTHREAD_GETPROC(pthread_setname_np)
	PTHREAD_GETPROC(pthread_getname_np)

	proc_init = 1;
#undef PTHREAD_GETPROC
}

inline static int qpthread_getattr_np(pthread_t pid, pthread_attr_t *attr)
{
    Q3E_InitPthreadProc();

    if(pthread_getattr_np_f)
		return pthread_getattr_np_f(pid, attr);
	else
	{
        LOGW("pthread_getattr_np not support");
		return -1;
	}
}

inline static int qpthread_setname_np(pthread_t pid, const char *name)
{
    Q3E_InitPthreadProc();

    if(pthread_setname_np_f)
		return pthread_setname_np_f(pid, name);
	else
	{
        LOGW("pthread_setname_np not support");
		return -1;
	}
}

inline static int qpthread_getname_np(pthread_t pid, char *name, size_t n)
{
    Q3E_InitPthreadProc();

    if(pthread_getname_np_f)
		return pthread_getname_np_f(pid, name, n);
	else
	{
		LOGW("pthread_getname_np not support");
		return -1;
	}
}

int Q3E_AlignedStackSize(size_t stackSize)
{
	if (stackSize == 0)
		return 0;

	size_t bytes = stackSize * 1024;
	long pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize <= 0) {
		LOGI("Get page size error %ld, using 4096 bytes.", pagesize);
		pagesize = 4096;
	}
	else {
		LOGI("Get page size %ld bytes.", pagesize);
	}
	long stackmin = sysconf(_SC_THREAD_STACK_MIN);
	if (stackmin <= 0) {
#ifdef PTHREAD_STACK_MIN
		LOGI("Get thread stack min error %ld, using PTHREAD_STACK_MIN macro %d. bytes.", stackmin, PTHREAD_STACK_MIN);
		stackmin = PTHREAD_STACK_MIN;
#else
		LOGI("Get thread stack min error %ld, using %d bytes.", pagesize, 16384);
        stackmin = 16384;
#endif
	} else {
		LOGI("Get thread stack min %ld bytes.", stackmin);
	}
	if (bytes < stackmin)
		bytes = stackmin;
	size_t alignedsize = (bytes + pagesize - 1) / pagesize * pagesize;
	LOGI("Require stack size %zu bytes, aligned size %zu bytes.", bytes, alignedsize);

	return alignedsize;
}

static int Q3E_SetupThreadAttrStackSize(pthread_attr_t *attr, size_t stackSize)
{
    if (stackSize == 0)
        return 0;

    size_t alignedsize = Q3E_AlignedStackSize(stackSize);

    size_t size = 0;
    pthread_attr_getstacksize(attr, &size);
    LOGI("Default stack size %zu bytes.", size);
    if (pthread_attr_setstacksize(attr, alignedsize) == 0) {
        pthread_attr_getstacksize(attr, &size);
        LOGI("Setup stack size %zu bytes", size);
        return alignedsize;
    } else {
        LOGW("Setup stack size %zu bytes fail!", alignedsize);
        return -1;
    }
}

int Q3E_CreateThread(pthread_t *threadid, void * (*mainf)(void *), void *data, const char *name, size_t stackSize)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	int res;

	if ( (res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0 ) {
		LOGE("ERROR: pthread_attr_setdetachstate native thread failed: %d", res);
		return -1;
	}

    // setup thread stack size
    res = Q3E_SetupThreadAttrStackSize(&attr, stackSize);

#if 0
	if(data)
	{
        void (* before_f)(void *ptrs[], int size);
        before_f = dlsym(data, "q3e_test");
		LOGI("Find q3e_test entry point on library: %p", before_f);
		if(before_f)
		{
			void *ptrs[] = {
					&attr,
					NULL,
			};
			LOGI("Call q3e_test entry point on library: %p", &attr);
			before_f(ptrs, sizeof(ptrs) / sizeof(ptrs[0]) - 1);
		}
	}
#endif

	if ( (res = pthread_create((pthread_t *)threadid, &attr, mainf, data)) != 0 ) {
		LOGE("ERROR: pthread_create native thread failed: %d", res);
		return -2;
	}

	pthread_attr_destroy(&attr);

	qpthread_setname_np(*threadid, name);

	LOGI("Native thread '%s' created: %zu.", name, PTHREAD_ID_WRAP(*threadid));

	return 0;
}

int Q3E_GetThreadName(const pthread_t *pid, char *name, int len)
{
	pthread_t id = pid ? *pid : pthread_self();
    int res;
	if((res = qpthread_getname_np(id, name, len)) == 0)
	{
		return strlen(name);
	}
	else
	{
		LOGW("Get thread name fail: %d.", res);
		return 0;
	}
}

int Q3E_GetCurrentThreadName(char *name)
{
    if(prctl(PR_GET_NAME, (long)name, 0, 0, 0) == 0)
    {
        return strlen(name);
    }
    else
    {
        LOGW("Get current thread name fail.");
        return 0;
    }
}

int Q3E_GetStackSize(const pthread_t *pid)
{
	pthread_t id = pid ? *pid : pthread_self();
	pthread_attr_t attr;
    int res;

	if((res = qpthread_getattr_np(id, &attr)) == 0)
	{
		size_t size = 0;
		pthread_attr_getstacksize(&attr, &size);
		pthread_attr_destroy(&attr);
		return size;
	}
	else
	{
		LOGW("Get thread stack size fail: %d.", res);
		return 0;
	}
}

int Q3E_QuitThread(pthread_t *threadid, void **data, int cancel)
{
    int res;
	if(cancel)
		q3e_pthread_cancel(*threadid);

	if ( (res = pthread_join(*threadid, data)) != 0 ) {
		LOGE("ERROR: pthread_join main thread failed: %d", res);
		if(!cancel)
		    q3e_pthread_cancel(*threadid);
		return -1;
	}

	LOGI("Native thread quit: %zu.", PTHREAD_ID_WRAP(*threadid));

	*threadid = 0;

	return 0;
}

void Q3E_EnterCriticalSection(void)
{
#ifdef ID_VERBOSE_PTHREADS
    pthread_t threadid = pthread_self();
	if (pthread_mutex_trylock(&global_lock) == EBUSY) {
		LOGE("busy lock in thread '%zu'", index, PTHREAD_ID_WRAP(threadid));

		if (pthread_mutex_lock(&global_lock) == EDEADLK) {
			LOGE("FATAL: DEADLOCK, in thread '%zu'", index, PTHREAD_ID_WRAP(threadid));
		}
	}

#else
	pthread_mutex_lock(&global_lock);
#endif
}

void Q3E_LeaveCriticalSection(void)
{
#ifdef ID_VERBOSE_PTHREADS

	if (pthread_mutex_unlock(&global_lock) == EPERM) {
		LOGE("FATAL: NOT LOCKED, in thread '%zu'", PTHREAD_ID_WRAP(pthread_self()));
	}

#else
	pthread_mutex_unlock(&global_lock);
#endif
}

void Q3E_WaitForEvent(void)
{
	pthread_mutex_lock(&cond_lock);
	ASSERT(!waiting);	// WaitForEvent from multiple threads? that wouldn't be good

	if (signaled) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		signaled = 0;
	} else {
		waiting = 1;
		pthread_cond_wait(&event_cond, &cond_lock);
		waiting = 0;
	}

	pthread_mutex_unlock(&cond_lock);
}

void Q3E_TriggerEvent(void)
{
	pthread_mutex_lock(&cond_lock);

	if (waiting) {
		pthread_cond_signal(&event_cond);
	} else {
		// emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
		signaled = 1;
	}

	pthread_mutex_unlock(&cond_lock);
}

void Q3E_InitPThreads(void)
{
	int i;
	pthread_mutexattr_t attr;
	pthread_mutex_t *mp[] = { &global_lock, &cond_lock };

	// init critical sections
	for (i = 0; i < sizeof(mp) / sizeof(mp[0]); i++) {
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(mp[i], &attr);
		pthread_mutexattr_destroy(&attr);
	}

	// init event sleep/triggers
    pthread_cond_init(&event_cond, NULL);
    signaled = 0;
    waiting = 0;
}

void Q3E_ShutdownPThreads(void)
{
	pthread_mutex_destroy(&global_lock);
	pthread_mutex_destroy(&cond_lock);
    pthread_cond_destroy(&event_cond);
    signaled = 0;
    waiting = 0;
}