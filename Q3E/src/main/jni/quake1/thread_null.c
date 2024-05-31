#include "quakedef.h"
#include "thread.h"

int Thread_Init(void)
{
	return 0;
}

void Thread_Shutdown(void)
{
}

qbool Thread_HasThreads(void)
{
	return false;
}

void *_Thread_CreateMutex(const char *filename, int fileline)
{
	return NULL;
}

void _Thread_DestroyMutex(void *mutex, const char *filename, int fileline)
{
}

int _Thread_LockMutex(void *mutex, const char *filename, int fileline)
{
	return -1;
}

int _Thread_UnlockMutex(void *mutex, const char *filename, int fileline)
{
	return -1;
}

void *_Thread_CreateCond(const char *filename, int fileline)
{
	return NULL;
}

void _Thread_DestroyCond(void *cond, const char *filename, int fileline)
{
}

int _Thread_CondSignal(void *cond, const char *filename, int fileline)
{
	return -1;
}

int _Thread_CondBroadcast(void *cond, const char *filename, int fileline)
{
	return -1;
}

int _Thread_CondWait(void *cond, void *mutex, const char *filename, int fileline)
{
	return -1;
}

void *_Thread_CreateThread(int (*fn)(void *), void *data, const char *filename, int fileline)
{
	return NULL;
}

int _Thread_WaitThread(void *thread, int retval, const char *filename, int fileline)
{
	return retval;
}

void *_Thread_CreateBarrier(unsigned int count, const char *filename, int fileline)
{
	return NULL;
}

void _Thread_DestroyBarrier(void *barrier, const char *filename, int fileline)
{
}

void _Thread_WaitBarrier(void *barrier, const char *filename, int fileline)
{
}

int _Thread_AtomicGet(Thread_Atomic *a, const char *filename, int fileline)
{
	return a->value;
}

int _Thread_AtomicSet(Thread_Atomic *a, int v, const char *filename, int fileline)
{
	int value = a->value;
	a->value = v;
	return value;
}

int _Thread_AtomicAdd(Thread_Atomic *a, int v, const char *filename, int fileline)
{
	int value = a->value;
	a->value += v;
	return value;
}

void _Thread_AtomicIncRef(Thread_Atomic *a, const char *filename, int fileline)
{
	a->value++;
}

qbool _Thread_AtomicDecRef(Thread_Atomic *a, const char *filename, int fileline)
{
	return a->value++ == 1;
}

qbool _Thread_AtomicTryLock(Thread_SpinLock *lock, const char *filename, int fileline)
{
	return true;
}

void _Thread_AtomicLock(Thread_SpinLock *lock, const char *filename, int fileline)
{
}

void _Thread_AtomicUnlock(Thread_SpinLock *lock, const char *filename, int fileline)
{
}
