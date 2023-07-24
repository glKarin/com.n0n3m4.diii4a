// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../sys/threading/SysLock.h"

/*
=============
sdLock::sdLock
=============
*/
sdLock::sdLock() {
	sdSysLock::Init( handle );
}

/*
=============
sdLock:~:sdLock
=============
*/
sdLock::~sdLock() {
	sdSysLock::Destroy( handle );
}

/*
=============
sdLock::Acquire
=============
*/
bool sdLock::Acquire( bool blocking ) {
	return sdSysLock::Acquire( handle, blocking );
}

/*
=============
sdLock::Release
=============
*/
void sdLock::Release() {
	sdSysLock::Release( handle );
}
