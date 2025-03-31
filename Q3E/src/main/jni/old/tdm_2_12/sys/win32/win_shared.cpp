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

#include "precompiled.h"
#pragma hdrstop

#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>

#pragma comment (lib, "wbemuuid.lib")

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds( void ) {
	int sys_curtime;
	static int sys_timeBase;
	static bool	initialized = false;

	if ( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized = true;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_GetTimeMicroseconds
================
*/
uint64_t Sys_GetTimeMicroseconds( void ) {
	FILETIME ft = {0};
	//note: returns number of 100-nanosec intervals since January 1, 1601 (UTC)
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER num;
	num.HighPart = ft.dwHighDateTime;
	num.LowPart = ft.dwLowDateTime;

	//difference between 1970-Jan-01 & 1601-Jan-01 in 100-nanosecond intervals
	const uint64_t shift = 116444736000000000ULL; // (27111902 << 32) + 3577643008
	uint64_t res = (num.QuadPart - shift) / 10;
	return res;	//in microsecs now
}

/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
int Sys_GetDriveFreeSpace( const char *path ) {
	DWORDLONG lpFreeBytesAvailable;
	DWORDLONG lpTotalNumberOfBytes;
	DWORDLONG lpTotalNumberOfFreeBytes;
	int ret = 26;
	//FIXME: see why this is failing on some machines
	//CANNOTFIX: uses reduntdant code needs total rewrite for modern windows archs, hack used to return positive numbers on win7 but breaks completely on any arch above that.
	if ( ::GetDiskFreeSpaceEx( path, (PULARGE_INTEGER)&lpFreeBytesAvailable, (PULARGE_INTEGER)&lpTotalNumberOfBytes, (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes ) ) {
		ret = ( double )( lpFreeBytesAvailable ) / ( 1024.0 * 1024.0 );
	}
	// force it to output positive numbers
	if (ret < 0)	{
		ret = -ret;
	}
	return abs(ret);
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes ) {
	::SetProcessWorkingSetSize( GetCurrentProcess(), minBytes, maxBytes );
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void *ptr, int bytes ) {
	return ( VirtualLock( ptr, (SIZE_T)bytes ) != FALSE );
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void *ptr, int bytes ) {
	return ( VirtualUnlock( ptr, (SIZE_T)bytes ) != FALSE );
}
