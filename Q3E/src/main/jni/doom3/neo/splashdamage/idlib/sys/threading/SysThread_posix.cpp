// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#include "../../../framework/CVarSystem.h"
#include "SysThread.h"

#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

threadProc_t	sdSysThread::createProc;
void			*sdSysThread::createParms;
int				sdSysThread::priority;

/*
=============
sdSysThread::Create

pthreads don't support a suspended create
so don't do anything in Create except storing the proc, and wait for Start call

FIXME: consider the priority stuff
=============
*/
bool sdSysThread::Create( threadProc_t proc, void* parms, threadHandle_t& handle, threadPriority_e _priority, unsigned int stackSize ) {
	priority = _priority;
	createProc = proc;
	createParms = parms;
	return true;
}

/*
===============
sdSysThread::setPriorityProc
wrap the thread start to allow setting up priority
===============
*/
void* sdSysThread::setPriorityProc( void *parms ) {

	errno = 0;
	int cur_prio = getpriority( PRIO_PROCESS, 0 );
	if ( errno != 0 ) {
		// Error called from a thread. this is likely to crash. Hopefully the error message has a change to print out
		common->Error( "sdSysThread::setPriorityProc getpriority failed %d %s", errno, strerror( errno ) );
	}
	// THREAD_NORMAL -> 0, THREAD_BELOW_NORMAL -> 1, THREAD_LOWEST -> 2
	cur_prio += ( THREAD_NORMAL - priority );
	int ret = setpriority( PRIO_PROCESS, 0, cur_prio );
	// we're in a thread, don't call into common->
	if ( ret != 0 ) {
		printf( "sdSysThread::setPriorityProc setpriority failed %d %s\n", errno, strerror( errno ) );
	} else {
		printf( "thread priority set to %d\n", cur_prio );
	}

	return createProc( parms );
}

/*
=============
sdSysThread::Start
=============
*/
bool sdSysThread::Start( threadHandle_t& handle ) {
	pthread_attr_t	attr;
	threadProc_t	localCreateProc = createProc;

	// assuming threads are always going to be created from the main thread
	// as a security, make sure the toplevel priority doesn't get modified
	static int		main_priority = -1;
	bool			setPriority = !idLib::cvarSystem->GetCVarBool( "sys_skipThreadPrio" );

	if ( setPriority ) {

		assert( priority <= THREAD_NORMAL );

		errno = 0;
		int current_prio = getpriority( PRIO_PROCESS, 0 );
		if ( errno != 0 ) {
			common->Error( "sdSysThread::Start getpriority failed %d %s", errno, strerror( errno ) );
		}
		if ( main_priority != current_prio ) {
			if ( main_priority != -1 ) {
				common->Warning( "sdSysThread::Start: process priority has changed: %d -> %d\n", main_priority, current_prio );
			}
			main_priority = current_prio;
		}
		
		if ( priority != THREAD_NORMAL ) {
			localCreateProc = setPriorityProc;
		}
	}

	pthread_attr_init( &attr );

	// the pthread API to set priorities doesn't work on Linux, but may work on BSD
	// keeping this in case I do a native BSD server
#if 0
	int ret;
	sched_param param;
	// THREAD_NORMAL -> 0, THREAD_BELOW_NORMAL -> 1, THREAD_LOWEST -> 2
	param.sched_priority = ( THREAD_NORMAL - priority );
	ret = pthread_attr_setschedparam( &attr, &param );
	if ( ret != 0 ) {
		// is not giving an error, but does not work either, the getschedparam later on still says zero
		common->Printf( "attr_setschedparam failed %d %s\n", ret, strerror( ret ) );
	}
#endif

	// some systems bail out with a pthread_create error - I assume something is leaking out threads
	int create_ret = pthread_create( &handle, &attr, localCreateProc, createParms );
	if ( create_ret != 0 ) {
		common->Error( "sdSysThread::Create ERROR: pthread_create failed - %d %s", create_ret, strerror( create_ret ) );
	}
	pthread_attr_destroy( &attr );

#if 0
	assert( priority <= THREAD_NORMAL );

	//	sched_param param;
	int policy;
	ret = pthread_getschedparam( handle, &policy, &param );
	if ( ret != 0 ) {
		common->Printf( "sdSysThread::Create ERROR: pthread_getschedparam failed - %d\n", ret );
	}
	printf( "thread priority before: %d, policy: %d SCHED_OTHER: %d\n", param.sched_priority, policy, SCHED_OTHER );

	param.sched_priority += ( THREAD_NORMAL - priority );
	printf( "set thread prio %d (etqw prio: %d)\n", param.sched_priority, priority );

	policy = SCHED_OTHER;
	ret = pthread_setschedparam( handle, policy, &param );
	if ( ret != 0 ) {
		common->Printf( "sdSysThread::Create ERROR: pthread_setschedparam failed - %d %s\n", ret, strerror( ret ) );
	}

	ret = pthread_getschedparam( handle, &policy, &param );
	if ( ret != 0 ) {
		common->Printf( "sdSysThread::Create ERROR: pthread_getschedparam failed - %d\n", ret );
	}
	printf( "thread priority after: %d\n", param.sched_priority );
#endif

	return true;
}

/*
=============
sdSysThread::Exit
=============
*/
unsigned int sdSysThread::Exit( unsigned int retVal ) {
	pthread_exit( &retVal );
	// won't actually return, pthread_exit is a termination, but it's cleaner that way
	return -1;
}

/*
=============
sdSysThread::Join
=============
*/
void sdSysThread::Join( threadHandle_t& handle ) {	
	int ret = pthread_join( handle, NULL );
	if ( ret != 0 ) {
		common->Printf( "sdSysThread::Join pthread_join failed: %d %s\n", ret, strerror( ret ) );
	}
}

/*
=============
sdSysThread::Destroy
=============
*/
void sdSysThread::Destroy( threadHandle_t& handle ) {
	// nothing to do here for pthread. this is called from main thread, but we have nothing extra to free
}

/*
=============
sdSysThread::SetPriority
=============
*/
void sdSysThread::SetPriority( threadHandle_t& handle, const threadPriority_e priority ) {
}

/*
=============
sdSysThread::SetName
=============
*/
void sdSysThread::SetName( threadHandle_t& handle, const char* name ) {
}

/*
=============
sdSysThread::SetProcessor
=============
*/
void sdSysThread::SetProcessor( threadHandle_t& handle, const unsigned int processor ) {
}
