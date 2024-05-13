#ifdef Q3E_REQUIRE_THREAD

#define ENTER_CRITICAL_SECTION (Sys_EnterCriticalSectionQ3E(MAX_LOCAL_CRITICAL_SECTIONS - 1));
#define LEAVE_CRITICAL_SECTION (Sys_LeaveCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1));

// we use an extra lock for the local stuff
const int MAX_CRITICAL_SECTIONS		= 4;
#define MAX_LOCAL_CRITICAL_SECTIONS 5
static pthread_mutex_t global_lock[ MAX_LOCAL_CRITICAL_SECTIONS ];

#define MAX_TRIGGER_EVENTS 2
enum {
    TRIGGER_EVENT_WINDOW_CREATED, // Android SurfaceView thread -> game/renderer thread: notify native window is set
    TRIGGER_EVENT_WINDOW_DESTROYED, // game thread/render thread -> Android SurfaceView thread: notify released OpenGL context
};

/*
==================
Q3E_Sys_EnterCriticalSection
==================
*/
void Q3E_Sys_EnterCriticalSection(int index)
{
    assert(index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS);
    pthread_mutex_lock(&global_lock[index]);
}

/*
==================
Q3E_Sys_LeaveCriticalSection
==================
*/
void Q3E_Sys_LeaveCriticalSection(int index)
{
    assert(index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS);
    pthread_mutex_unlock(&global_lock[index]);
}

static pthread_cond_t	event_cond[ MAX_TRIGGER_EVENTS ];
static Q3Ebool 			signaled[ MAX_TRIGGER_EVENTS ];
static Q3Ebool			waiting[ MAX_TRIGGER_EVENTS ];

/*
==================
Q3E_Sys_WaitForEvent
==================
*/
void Q3E_Sys_WaitForEvent(int index)
{
    assert(index >= 0 && index < MAX_TRIGGER_EVENTS);
    Q3E_Sys_EnterCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
    assert(!waiting[ index ]);	// WaitForEvent from multiple threads? that wouldn't be good

    if (signaled[ index ]) {
        // emulate windows behaviour: signal has been raised already. clear and keep going
        signaled[ index ] = Q3E_FALSE;
    } else {
        waiting[ index ] = Q3E_TRUE;
        pthread_cond_wait(&event_cond[ index ], &global_lock[ MAX_LOCAL_CRITICAL_SECTIONS - 1 ]);
        waiting[ index ] = Q3E_FALSE;
    }

    Q3E_Sys_LeaveCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
}

/*
==================
Q3E_Sys_TriggerEvent
==================
*/
void Q3E_Sys_TriggerEvent(int index)
{
    assert(index >= 0 && index < MAX_TRIGGER_EVENTS);
    Q3E_Sys_EnterCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);

    if (waiting[ index ]) {
        pthread_cond_signal(&event_cond[ index ]);
    } else {
        // emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
        signaled[ index ] = Q3E_TRUE;
    }

    Q3E_Sys_LeaveCriticalSection(MAX_LOCAL_CRITICAL_SECTIONS - 1);
}

/*
==================
Q3E_Sys_InitPThreads
==================
*/
void Q3E_Sys_InitThreads()
{
    int i;
    pthread_mutexattr_t attr;

    // init critical sections
    for (i = 0; i < MAX_LOCAL_CRITICAL_SECTIONS; i++) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&global_lock[i], &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // init event sleep/triggers
    for (i = 0; i < MAX_TRIGGER_EVENTS; i++) {
        pthread_cond_init(&event_cond[ i ], NULL);
        signaled[i] = Q3E_FALSE;
        waiting[i] = Q3E_FALSE;
    }
}

#define WAIT_FOR_EVENT(index) Q3E_Sys_WaitForEvent(index)
#define TRIGGER_EVENT(index) Q3E_Sys_TriggerEvent(index)

#define INIT_Q3E_THREADS Q3E_Sys_InitThreads()

#define WAIT_WINDOW_DESTROYED WAIT_FOR_EVENT(TRIGGER_EVENT_WINDOW_DESTROYED)
#define TRIGGER_WINDOW_DESTROYED TRIGGER_EVENT(TRIGGER_EVENT_WINDOW_DESTROYED)

#define WAIT_WINDOW_CREATED WAIT_FOR_EVENT(TRIGGER_EVENT_WINDOW_CREATED)
#define TRIGGER_WINDOW_CREATED TRIGGER_EVENT(TRIGGER_EVENT_WINDOW_CREATED)
#endif