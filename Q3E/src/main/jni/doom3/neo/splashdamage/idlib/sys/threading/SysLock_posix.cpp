// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#include "SysLock.h"

/*
=============
sdSysLock::Create
=============
*/
void sdSysLock::Init( lockHandle_t& handle ) {
	pthread_mutexattr_t	attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle, &attr );
	pthread_mutexattr_destroy( &attr );	
}

/*
=============
sdSysLock::Destroy
=============
*/
void sdSysLock::Destroy( lockHandle_t& handle ) {
	pthread_mutex_destroy( &handle );
}

/*
=============
sdSysLock::Acquire
=============
*/
bool sdSysLock::Acquire( lockHandle_t& handle, bool blocking ) {
	if ( blocking ) {
		pthread_mutex_lock( &handle );
		return true;
	}
	if ( pthread_mutex_trylock( &handle ) == EBUSY ) {
		return false;
	}
	return true;
}

/*
=============
sdSysLock::Release
=============
*/
void sdSysLock::Release( lockHandle_t& handle ) {
	pthread_mutex_unlock( &handle );
}
