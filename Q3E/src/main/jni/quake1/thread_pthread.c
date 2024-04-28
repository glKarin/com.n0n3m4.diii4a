#include "quakedef.h"
#include "thread.h"
#ifdef THREADRECURSIVE
#define __USE_UNIX98
#include <pthread.h>
#endif
#include <stdint.h>


int Thread_Init(void)
{
	return 0;
}

void Thread_Shutdown(void)
{
}

qbool Thread_HasThreads(void)
{
	return true;
}

void *_Thread_CreateMutex(const char *filename, int fileline)
{
#ifdef THREADRECURSIVE
	pthread_mutexattr_t    attr;
#endif
	pthread_mutex_t *mutexp = (pthread_mutex_t *) Z_Malloc(sizeof(pthread_mutex_t));
#ifdef THREADDEBUG
	Sys_Printf("%p mutex create %s:%i\n" , mutexp, filename, fileline);
#endif
#ifdef THREADRECURSIVE
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(mutexp, &attr);
	pthread_mutexattr_destroy(&attr);
#else
	pthread_mutex_init(mutexp, NULL);
#endif
	return mutexp;
}

void _Thread_DestroyMutex(void *mutex, const char *filename, int fileline)
{
	pthread_mutex_t *mutexp = (pthread_mutex_t *) mutex;
#ifdef THREADDEBUG
	Sys_Printf("%p mutex destroy %s:%i\n", mutex, filename, fileline);
#endif
	pthread_mutex_destroy(mutexp);
	Z_Free(mutexp);
}

int _Thread_LockMutex(void *mutex, const char *filename, int fileline)
{
	pthread_mutex_t *mutexp = (pthread_mutex_t *) mutex;
#ifdef THREADDEBUG
	Sys_Printf("%p mutex lock %s:%i\n"   , mutex, filename, fileline);
#endif
	return pthread_mutex_lock(mutexp);
}

int _Thread_UnlockMutex(void *mutex, const char *filename, int fileline)
{
	pthread_mutex_t *mutexp = (pthread_mutex_t *) mutex;
#ifdef THREADDEBUG
	Sys_Printf("%p mutex unlock %s:%i\n" , mutex, filename, fileline);
#endif
	return pthread_mutex_unlock(mutexp);
}

void *_Thread_CreateCond(const char *filename, int fileline)
{
	pthread_cond_t *condp = (pthread_cond_t *) Z_Malloc(sizeof(pthread_cond_t));
	pthread_cond_init(condp, NULL);
#ifdef THREADDEBUG
	Sys_Printf("%p cond create %s:%i\n"   , condp, filename, fileline);
#endif
	return condp;
}

void _Thread_DestroyCond(void *cond, const char *filename, int fileline)
{
	pthread_cond_t *condp = (pthread_cond_t *) cond;
#ifdef THREADDEBUG
	Sys_Printf("%p cond destroy %s:%i\n"   , cond, filename, fileline);
#endif
	pthread_cond_destroy(condp);
	Z_Free(condp);
}

int _Thread_CondSignal(void *cond, const char *filename, int fileline)
{
	pthread_cond_t *condp = (pthread_cond_t *) cond;
#ifdef THREADDEBUG
	Sys_Printf("%p cond signal %s:%i\n"   , cond, filename, fileline);
#endif
	return pthread_cond_signal(condp);
}

int _Thread_CondBroadcast(void *cond, const char *filename, int fileline)
{
	pthread_cond_t *condp = (pthread_cond_t *) cond;
#ifdef THREADDEBUG
	Sys_Printf("%p cond broadcast %s:%i\n"   , cond, filename, fileline);
#endif
	return pthread_cond_broadcast(condp);
}

int _Thread_CondWait(void *cond, void *mutex, const char *filename, int fileline)
{
	pthread_cond_t *condp = (pthread_cond_t *) cond;
	pthread_mutex_t *mutexp = (pthread_mutex_t *) mutex;
#ifdef THREADDEBUG
	Sys_Printf("%p cond wait %s:%i\n"   , cond, filename, fileline);
#endif
	return pthread_cond_wait(condp, mutexp);
}

void *_Thread_CreateThread(int (*fn)(void *), void *data, const char *filename, int fileline)
{
	pthread_t *threadp = (pthread_t *) Z_Malloc(sizeof(pthread_t));
#ifdef THREADDEBUG
	Sys_Printf("%p thread create %s:%i\n"   , threadp, filename, fileline);
#endif
	int r = pthread_create(threadp, NULL, (void * (*) (void *)) fn, data);
	if(r)
	{
		Z_Free(threadp);
		return NULL;
	}
	return threadp;
}

int _Thread_WaitThread(void *thread, int retval, const char *filename, int fileline)
{
	pthread_t *threadp = (pthread_t *) thread;
	void *status = (void *) (intptr_t) retval;
#ifdef THREADDEBUG
	Sys_Printf("%p thread wait %s:%i\n"   , thread, filename, fileline);
#endif
	pthread_join(*threadp, &status);
	Z_Free(threadp);
	return (int) (intptr_t) status;
}

#ifdef PTHREAD_BARRIER_SERIAL_THREAD
void *_Thread_CreateBarrier(unsigned int count, const char *filename, int fileline)
{
	pthread_barrier_t *b = (pthread_barrier_t *) Z_Malloc(sizeof(pthread_barrier_t));
#ifdef THREADDEBUG
	Sys_Printf("%p barrier create(%d) %s:%i\n", b, count, filename, fileline);
#endif
	pthread_barrier_init(b, NULL, count);
	return (void *) b;
}

void _Thread_DestroyBarrier(void *barrier, const char *filename, int fileline)
{
	pthread_barrier_t *b = (pthread_barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_Printf("%p barrier destroy %s:%i\n", b, filename, fileline);
#endif
	pthread_barrier_destroy(b);
}

void _Thread_WaitBarrier(void *barrier, const char *filename, int fileline)
{
	pthread_barrier_t *b = (pthread_barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_Printf("%p barrier wait %s:%i\n", b, filename, fileline);
#endif
	pthread_barrier_wait(b);
}
#else
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
#endif

void _Thread_AtomicLock(Thread_SpinLock *lock, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_Printf("%p atomic lock %s:%i\n", lock, filename, fileline);
#endif
	//SDL_AtomicLock(lock);
	atomic_flag_test_and_set(lock);
}

void _Thread_AtomicUnlock(Thread_SpinLock *lock, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_Printf("%p atomic unlock %s:%i\n", lock, filename, fileline);
#endif
	//SDL_AtomicUnlock(lock);
	atomic_flag_clear(lock);
}