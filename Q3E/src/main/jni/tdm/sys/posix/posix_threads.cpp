/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <pthread.h>

#include "../../idlib/precompiled.h"
#include "posix_public.h"

#if defined(_DEBUG)
// #define ID_VERBOSE_PTHREADS 
#endif

/*
======================================================
locks
======================================================
*/

// we use an extra lock for the local stuff
const int MAX_LOCAL_CRITICAL_SECTIONS = MAX_CRITICAL_SECTIONS + 1;
static pthread_mutex_t global_lock[ MAX_LOCAL_CRITICAL_SECTIONS ];

/*
==================
Sys_EnterCriticalSection
==================
*/
void Sys_EnterCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS );
#ifdef ID_VERBOSE_PTHREADS	
	if ( pthread_mutex_trylock( &global_lock[index] ) == EBUSY ) {
		Sys_Printf( "busy lock %d in thread '%s'\n", index, Sys_GetThreadName() );
		if ( pthread_mutex_lock( &global_lock[index] ) == EDEADLK ) {
			Sys_Printf( "FATAL: DEADLOCK %d, in thread '%s'\n", index, Sys_GetThreadName() );
		}
	}	
#else
	pthread_mutex_lock( &global_lock[index] );
#endif
}

/*
==================
Sys_LeaveCriticalSection
==================
*/
void Sys_LeaveCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS );
#ifdef ID_VERBOSE_PTHREADS
	if ( pthread_mutex_unlock( &global_lock[index] ) == EPERM ) {
		Sys_Printf( "FATAL: NOT LOCKED %d, in thread '%s'\n", index, Sys_GetThreadName() );
	}
#else
	pthread_mutex_unlock( &global_lock[index] );
#endif
}

/*
======================================================
wait and trigger events
we use a single lock to manipulate the conditions, MAX_LOCAL_CRITICAL_SECTIONS-1

the semantics match the win32 version. signals raised while no one is waiting stay raised until a wait happens (which then does a simple pass-through)

NOTE: we use the same mutex for all the events. I don't think this would become much of a problem
cond_wait unlocks atomically with setting the wait condition, and locks it back before exiting the function
the potential for time wasting lock waits is very low
======================================================
*/

pthread_cond_t	event_cond[ MAX_TRIGGER_EVENTS ];
bool			signaled[ MAX_TRIGGER_EVENTS ];
bool			waiting[ MAX_TRIGGER_EVENTS ];

/*
==================
Sys_WaitForEvent
==================
*/
void Sys_WaitForEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	Sys_EnterCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
	assert( !waiting[ index ] );	// WaitForEvent from multiple threads? that wouldn't be good
	if ( signaled[ index ] ) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		signaled[ index ] = false;
	} else {
		waiting[ index ] = true;
		pthread_cond_wait( &event_cond[ index ], &global_lock[ MAX_LOCAL_CRITICAL_SECTIONS - 1 ] );
		waiting[ index ] = false;
	}
	Sys_LeaveCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
}

/*
==================
Sys_TriggerEvent
==================
*/
void Sys_TriggerEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	Sys_EnterCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
	if ( waiting[ index ] ) {		
		pthread_cond_signal( &event_cond[ index ] );
	} else {
		// emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
		signaled[ index ] = true;
	}
	Sys_LeaveCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
}

/*
======================================================
thread create and destroy
======================================================
*/

typedef void *(*pthread_function_t) (void *);

/*
==================
Sys_GetThreadName
find the name of the calling thread
==================
*/
static int Sys_GetThreadName( pthread_t handle, char* namebuf, size_t buflen )
{
	int ret = 0;
#ifdef __ANDROID__ //karin: pthread_getname_np on Android
    #if __ANDROID_API__ >= 26
	ret = pthread_getname_np( handle, namebuf, buflen );
    #else
    ret = 0;
    #endif
	if( ret != 0 )
		idLib::common->Printf( "Getting threadname failed, reason: %s (%i)\n", strerror( errno ), errno );
#elif defined(__linux__)
	ret = pthread_getname_np( handle, namebuf, buflen );
	if( ret != 0 )
		idLib::common->Printf( "Getting threadname failed, reason: %s (%i)\n", strerror( errno ), errno );
#elif defined(__FreeBSD__)
	// seems like there is no pthread_getname_np equivalent on FreeBSD
	idStr::snPrintf( namebuf, buflen, "Can't read threadname on this platform!" );
#endif
	/* TODO: OSX:
		// int pthread_getname_np(pthread_t, char*, size_t);
	*/
	
	return ret;
}

/*
=========================================================
Async Thread
=========================================================
*/

uintptr_t asyncThread;
volatile bool asyncThreadShutdown;

/*
=================
Posix_StartAsyncThread
=================
*/
void Posix_StartAsyncThread() {
	if ( asyncThread == 0 ) {
		asyncThread = Sys_CreateThread( (xthread_t) Sys_AsyncThread, NULL, THREAD_NORMAL, "Async" );
	} else {
		common->Printf( "Async thread already running\n" );
	}
	common->Printf( "Async thread started\n" );
}

/*
==================
Posix_InitPThreads
==================
*/
void Posix_InitPThreads( ) {
	int i;
	pthread_mutexattr_t attr;

	// init critical sections
	for ( i = 0; i < MAX_LOCAL_CRITICAL_SECTIONS; i++ ) {
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
		pthread_mutex_init( &global_lock[i], &attr );
		pthread_mutexattr_destroy( &attr );
	}

	// init event sleep/triggers
	for ( i = 0; i < MAX_TRIGGER_EVENTS; i++ ) {
		pthread_cond_init( &event_cond[ i ], NULL );
		signaled[i] = false;
		waiting[i] = false;
	}

	/* stgatilov: this was removed by duzenko
	// init threads table
	for ( i = 0; i < MAX_THREADS; i++ ) {
		g_threads[ i ] = NULL;
	}*/
}
