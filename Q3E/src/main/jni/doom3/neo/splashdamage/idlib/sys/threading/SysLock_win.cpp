// Copyright (C) 2007 Id Software, Inc.
//

 #include "../../precompiled.h"
#pragma hdrstop

#include "SysLock.h"

/*
=============
sdSysLock::Create
=============
*/
void sdSysLock::Init( lockHandle_t& handle ) {
	::InitializeCriticalSection( &handle );
}

/*
=============
sdSysLock::Destroy
=============
*/
void sdSysLock::Destroy( lockHandle_t& handle ) {
	::DeleteCriticalSection( &handle );
}

/*
=============
sdSysLock::Acquire
=============
*/
bool sdSysLock::Acquire( lockHandle_t& handle, bool blocking ) {
	if ( ::TryEnterCriticalSection( &handle ) == 0 ) {
		if ( !blocking ) {
			return false;
		}
		::EnterCriticalSection( &handle );
	}
	return true;
}

/*
=============
sdSysLock::Release
=============
*/
void sdSysLock::Release( lockHandle_t& handle ) {
	::LeaveCriticalSection( &handle );
}
