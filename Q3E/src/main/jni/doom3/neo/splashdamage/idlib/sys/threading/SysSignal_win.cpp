// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "SysSignal.h"

/*
=============
sdSysSignal::Create
=============
*/
void sdSysSignal::Create( signalHandle_t &handle ) {
	handle = ::CreateEvent( NULL, FALSE, FALSE, NULL );
}

/*
=============
sdSysSignal::Destroy
=============
*/
void sdSysSignal::Destroy( signalHandle_t& handle ) {
	::CloseHandle( handle );
}

/*
=============
sdSysSignal::Set
=============
*/
void sdSysSignal::Set( signalHandle_t& handle ) {
	::SetEvent( handle );
}

/*
=============
sdSysSignal::Clear
=============
*/
void sdSysSignal::Clear( signalHandle_t& handle ) {
	// events are created as auto-reset so this should never be needed
	assert( false );
	::ResetEvent( handle );
}

/*
=============
sdSysSignal::Wait
=============
*/
bool sdSysSignal::Wait( signalHandle_t& handle, int timeout ) {

	return ( ::WaitForSingleObject( handle, timeout == sdSignal::WAIT_INFINITE ? INFINITE : timeout ) != WAIT_FAILED );
}

/*
=============
sdSysSignal::SignalAndWait
=============
*/
bool sdSysSignal::SignalAndWait( signalHandle_t& signal, signalHandle_t& handle, int timeout ) {
	return ( ::SignalObjectAndWait( signal, handle, timeout == sdSignal::WAIT_INFINITE ? INFINITE : timeout, FALSE ) != WAIT_FAILED );
}
