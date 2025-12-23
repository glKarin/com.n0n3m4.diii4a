#ifndef _Q3E_THREAD_H
#define _Q3E_THREAD_H

#include <pthread.h>

#define Q3E_THREAD_CANCEL_SIG SIGUSR1

#define PTHREAD_ID_WRAP(x) (uintptr_t)(x)

#ifdef __cplusplus
extern "C" {
#endif

int q3e_pthread_cancel(pthread_t pthread_id);
int q3e_pthread_cancelable(void);

int Q3E_CreateThread(pthread_t *threadid, void * (*mainf)(void *), void *data);
int Q3E_QuitThread(pthread_t *threadid, void **data, int cancel);

void Q3E_InitPThreads(void);
void Q3E_ShutdownPThreads(void);
void Q3E_WaitForEvent(void);
void Q3E_TriggerEvent(void);
void Q3E_LeaveCriticalSection(void);
void Q3E_EnterCriticalSection(void);

#ifdef __cplusplus
};
#endif

#endif // _Q3E_THREAD_H