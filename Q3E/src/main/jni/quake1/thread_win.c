#include "quakedef.h"
#include "thread.h"
#include <process.h>

int Thread_Init(void)
{
#ifdef THREADDISABLE
	Con_Printf("Threading disabled in this build\n");
#endif
	return 0;
}

void Thread_Shutdown(void)
{
}

qbool Thread_HasThreads(void)
{
#ifdef THREADDISABLE
	return false;
#else
	return true;
#endif
}

void *_Thread_CreateMutex(const char *filename, int fileline)
{
	void *mutex = (void *)CreateMutex(NULL, FALSE, NULL);
#ifdef THREADDEBUG
	Sys_Printf("%p mutex create %s:%i\n" , mutex, filename, fileline);
#endif
	return mutex;
}

void _Thread_DestroyMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_Printf("%p mutex destroy %s:%i\n", mutex, filename, fileline);
#endif
	CloseHandle(mutex);
}

int _Thread_LockMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_Printf("%p mutex lock %s:%i\n"   , mutex, filename, fileline);
#endif
	return (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED) ? -1 : 0;
}

int _Thread_UnlockMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_Printf("%p mutex unlock %s:%i\n" , mutex, filename, fileline);
#endif
	return (ReleaseMutex(mutex) == false) ? -1 : 0;
}

typedef struct thread_semaphore_s
{
	HANDLE semaphore;
	volatile LONG value;
}
thread_semaphore_t;

static thread_semaphore_t *Thread_CreateSemaphore(unsigned int v)
{
	thread_semaphore_t *s = (thread_semaphore_t *)calloc(sizeof(*s), 1);
	s->semaphore = CreateSemaphore(NULL, v, 32768, NULL);
	s->value = v;
	return s;
}

static void Thread_DestroySemaphore(thread_semaphore_t *s)
{
	CloseHandle(s->semaphore);
	free(s);
}

static int Thread_WaitSemaphore(thread_semaphore_t *s, unsigned int msec)
{
	int r = WaitForSingleObject(s->semaphore, msec);
	if (r == WAIT_OBJECT_0)
	{
		InterlockedDecrement(&s->value);
		return 0;
	}
	if (r == WAIT_TIMEOUT)
		return 1;
	return -1;
}

static int Thread_PostSemaphore(thread_semaphore_t *s)
{
	InterlockedIncrement(&s->value);
	if (ReleaseSemaphore(s->semaphore, 1, NULL))
		return 0;
	InterlockedDecrement(&s->value);
	return -1;
}

typedef struct thread_cond_s
{
	HANDLE mutex;
	int waiting;
	int signals;
	thread_semaphore_t *sem;
	thread_semaphore_t *done;
}
thread_cond_t;

void *_Thread_CreateCond(const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)calloc(sizeof(*c), 1);
	c->mutex = CreateMutex(NULL, FALSE, NULL);
	c->sem = Thread_CreateSemaphore(0);
	c->done = Thread_CreateSemaphore(0);
	c->waiting = 0;
	c->signals = 0;
#ifdef THREADDEBUG
	Sys_Printf("%p cond create %s:%i\n"   , c, filename, fileline);
#endif
	return c;
}

void _Thread_DestroyCond(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
#ifdef THREADDEBUG
	Sys_Printf("%p cond destroy %s:%i\n"   , cond, filename, fileline);
#endif
	Thread_DestroySemaphore(c->sem);
	Thread_DestroySemaphore(c->done);
	CloseHandle(c->mutex);
}

int _Thread_CondSignal(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int n;
#ifdef THREADDEBUG
	Sys_Printf("%p cond signal %s:%i\n"   , cond, filename, fileline);
#endif
	WaitForSingleObject(c->mutex, INFINITE);
	n = c->waiting - c->signals;
	if (n > 0)
	{
		c->signals++;
		Thread_PostSemaphore(c->sem);
	}
	ReleaseMutex(c->mutex);
	if (n > 0)
		Thread_WaitSemaphore(c->done, INFINITE);
	return 0;
}

int _Thread_CondBroadcast(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int i = 0;
	int n = 0;
#ifdef THREADDEBUG
	Sys_Printf("%p cond broadcast %s:%i\n"   , cond, filename, fileline);
#endif
	WaitForSingleObject(c->mutex, INFINITE);
	n = c->waiting - c->signals;
	if (n > 0)
	{
		c->signals += n;
		for (i = 0;i < n;i++)
			Thread_PostSemaphore(c->sem);
	}
	ReleaseMutex(c->mutex);
	for (i = 0;i < n;i++)
		Thread_WaitSemaphore(c->done, INFINITE);
	return 0;
}

int _Thread_CondWait(void *cond, void *mutex, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int waitresult;
#ifdef THREADDEBUG
	Sys_Printf("%p cond wait %s:%i\n"   , cond, filename, fileline);
#endif

	WaitForSingleObject(c->mutex, INFINITE);
	c->waiting++;
	ReleaseMutex(c->mutex);

	ReleaseMutex(mutex);

	waitresult = Thread_WaitSemaphore(c->sem, INFINITE);
	WaitForSingleObject(c->mutex, INFINITE);
	if (c->signals > 0)
	{
		if (waitresult > 0)
			Thread_WaitSemaphore(c->sem, INFINITE);
		Thread_PostSemaphore(c->done);
		c->signals--;
	}
	c->waiting--;
	ReleaseMutex(c->mutex);

	WaitForSingleObject(mutex, INFINITE);
	return waitresult;
}

typedef struct threadwrapper_s
{
	HANDLE handle;
	unsigned int threadid;
	int result;
	int (*fn)(void *);
	void *data;
}
threadwrapper_t;

unsigned int __stdcall Thread_WrapperFunc(void *d)
{
	threadwrapper_t *w = (threadwrapper_t *)d;
	w->result = w->fn(w->data);
	_endthreadex(w->result);
	return w->result;
}

void *_Thread_CreateThread(int (*fn)(void *), void *data, const char *filename, int fileline)
{
	threadwrapper_t *w = (threadwrapper_t *)calloc(sizeof(*w), 1);
#ifdef THREADDEBUG
	Sys_Printf("%p thread create %s:%i\n"   , w, filename, fileline);
#endif
	w->fn = fn;
	w->data = data;
	w->threadid = 0;
	w->result = 0;
	w->handle = (HANDLE)_beginthreadex(NULL, 0, Thread_WrapperFunc, (void *)w, 0, &w->threadid);
	return (void *)w;
}

int _Thread_WaitThread(void *d, int retval, const char *filename, int fileline)
{
	threadwrapper_t *w = (threadwrapper_t *)d;
#ifdef THREADDEBUG
	Sys_Printf("%p thread wait %s:%i\n"   , w, filename, fileline);
#endif
	WaitForSingleObject(w->handle, INFINITE);
	CloseHandle(w->handle);
	retval = w->result;
	free(w);
	return retval;
}

// standard barrier implementation using conds and mutexes
// see: http://www.howforge.com/implementing-barrier-in-pthreads
typedef struct {
	unsigned int needed;
	unsigned int called;
	void *mutex;
	void *cond;
} barrier_t;

void *_Thread_CreateBarrier(unsigned int count, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) Z_Malloc(sizeof(barrier_t));
#ifdef THREADDEBUG
	Sys_Printf("%p barrier create(%d) %s:%i\n", b, count, filename, fileline);
#endif
	b->needed = count;
	b->called = 0;
	b->mutex = Thread_CreateMutex();
	b->cond = Thread_CreateCond();
	return (void *) b;
}

void _Thread_DestroyBarrier(void *barrier, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_Printf("%p barrier destroy %s:%i\n", b, filename, fileline);
#endif
	Thread_DestroyMutex(b->mutex);
	Thread_DestroyCond(b->cond);
}

void _Thread_WaitBarrier(void *barrier, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_Printf("%p barrier wait %s:%i\n", b, filename, fileline);
#endif
	Thread_LockMutex(b->mutex);
	b->called++;
	if (b->called == b->needed) {
		b->called = 0;
		Thread_CondBroadcast(b->cond);
	} else {
		do {
			Thread_CondWait(b->cond, b->mutex);
		} while(b->called);
	}
	Thread_UnlockMutex(b->mutex);
}
