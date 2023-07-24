// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../sys/threading/SysSignal.h"

/*
=============
sdSignal::sdSignal
=============
*/
sdSignal::sdSignal() {
	sdSysSignal::Create( handle );
}

/*
=============
sdSignal:~:sdSignal
=============
*/
sdSignal::~sdSignal() {
	sdSysSignal::Destroy( handle );
}

/*
=============
sdSignal::Set
=============
*/
void sdSignal::Set() {
	sdSysSignal::Set( handle );
}

/*
=============
sdSignal::Clear
=============
*/
void sdSignal::Clear() {
	sdSysSignal::Clear( handle );
}

/*
=============
sdSignal::Wait
=============
*/
bool sdSignal::Wait( int timeout ) {
	return sdSysSignal::Wait( handle, timeout );
}

/*
=============
sdSignal::Wait
=============
*/
bool sdSignal::SignalAndWait( sdSignal &signal, int timeout ) {
	return sdSysSignal::SignalAndWait( signal.handle, handle, timeout );
}
