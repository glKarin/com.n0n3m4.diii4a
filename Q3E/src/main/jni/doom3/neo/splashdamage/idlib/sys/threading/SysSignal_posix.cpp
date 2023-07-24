// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#include "SysSignal.h"

#include <sys/time.h>
#include <time.h>

/*
=============
sdSysSignal::Create
=============
*/
void sdSysSignal::Create( signalHandle_t &handle ) {	
	pthread_mutexattr_t	attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle.mutex, &attr );
	pthread_mutexattr_destroy( &attr );	

	pthread_cond_init( &handle.cond, NULL );

	handle.signaled = false;
	handle.waiting = false;
}

/*
=============
sdSysSignal::Destroy
=============
*/
void sdSysSignal::Destroy( signalHandle_t& handle ) {
	pthread_mutex_destroy( &handle.mutex );
	pthread_cond_destroy( &handle.cond );
}

/*
=============
sdSysSignal::Set
=============
*/
void sdSysSignal::Set( signalHandle_t& handle ) {
	pthread_mutex_lock( &handle.mutex );
	if ( handle.waiting ) {
		pthread_cond_signal( &handle.cond );
	} else {
		// emulate windows behaviour: if no thread is waiting, leave the signal on so next Wait() keeps going
		handle.signaled = true;
	}
	pthread_mutex_unlock( &handle.mutex );
}

/*
=============
sdSysSignal::Clear
=============
*/
void sdSysSignal::Clear( signalHandle_t& handle ) {
	assert( false );	// unused
	handle.signaled = false;
}

/*
=============
sdSysSignal::Wait
=============
*/
bool sdSysSignal::Wait( signalHandle_t& handle, int timeout ) {
	int rc;
	struct timespec ts;
	struct timeval tp;
	pthread_mutex_lock( &handle.mutex );
	assert( !handle.waiting );	// Wait() from multiple threads?
	if ( handle.signaled ) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		handle.signaled = false;
	} else {
		handle.waiting = true;
		if ( timeout == sdSignal::WAIT_INFINITE ) {
			pthread_cond_wait( &handle.cond, &handle.mutex );
		} else {
			rc =  gettimeofday( &tp, NULL );
			assert( rc == 0 );
			ts.tv_sec = tp.tv_sec + timeout / 1000;
			ts.tv_nsec = tp.tv_usec * 1000 + ( timeout % 1000 ) * 1000000;
			pthread_cond_timedwait( &handle.cond, &handle.mutex, &ts );
		}
		handle.waiting = false;
	}
	pthread_mutex_unlock( &handle.mutex );
	return true;
}

/*
=============
sdSysSignal::SignalAndWait

the windows implementation uses SignalObjectAndWait to obtain an atomic set and wait operation

a race condition is possible if implemented with just a Set followed by a Wait

for instance in the initial use case of this code, for synchronization of the bot thread and the game thread
signal == botSignal, handle = gameSignal
the bot signal gets raised, the game thread runs, triggers the game thread signal and starts waiting on the bot signal again
all before the bot thread starts waiting on the game signal. when it does, both threads are deadlocked

we use the Wait() implementation, and only fire up botSignal once we've acquired the gameSignal mutex
the game thread always triggers gameSignal before (possibly) waiting on botSignal
so if it tries to do that in between the botSignal firing and the gameSignal wait,
it will have to wait on the gameSignal mutex until pthread_cond_wait releases it upon waiting on gameSignal condition
in that case that will just trigger the condition right away, with no deadlock possibility

see pthread_cond_wait documentation for precise details
(in particular the "releasing mutex upon waiting" part)
=============
*/
bool sdSysSignal::SignalAndWait( signalHandle_t& signal, signalHandle_t& handle, int timeout ) {
	int rc;
	struct timespec ts;
	struct timeval tp;

	pthread_mutex_lock( &handle.mutex );
	assert( !handle.waiting );	// Wait() from multiple threads?

	Set( signal );

	if ( handle.signaled ) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		handle.signaled = false;
	} else {
		handle.waiting = true;
		if ( timeout == sdSignal::WAIT_INFINITE ) {
			pthread_cond_wait( &handle.cond, &handle.mutex );
		} else {
			rc =  gettimeofday( &tp, NULL );
			assert( rc == 0 );
			ts.tv_sec = tp.tv_sec + timeout / 1000;
			ts.tv_nsec = tp.tv_usec * 1000 + ( timeout % 1000 ) * 1000000;
			pthread_cond_timedwait( &handle.cond, &handle.mutex, &ts );
		}
		handle.waiting = false;
	}
	pthread_mutex_unlock( &handle.mutex );
	return true;
}
