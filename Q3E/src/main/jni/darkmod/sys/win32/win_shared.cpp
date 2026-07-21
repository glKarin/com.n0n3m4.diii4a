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

//#include "precompiled.h"
//#pragma hdrstop

// stgatilov: can't use PCH here due to MFC headers removing necessary bits of windows.h!
#include "windows.h"

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
	static HMODULE hKernel32Dll = LoadLibrary("Kernel32.dll");
	static auto GetSystemTimePreciseAsFileTime = (void(WINAPI *)(LPFILETIME))GetProcAddress(hKernel32Dll, "GetSystemTimePreciseAsFileTime");

	FILETIME ft = {0};
	// note: both functions return number of 100-nanosec intervals since January 1, 1601 (UTC)
	if (GetSystemTimePreciseAsFileTime) {
		// < 1 us precision, but requires Windows 8+
		GetSystemTimePreciseAsFileTime(&ft);
	} else {
		// seems to provide 1 ms precision only
		GetSystemTimeAsFileTime(&ft);
	}
	ULARGE_INTEGER num;
	num.HighPart = ft.dwHighDateTime;
	num.LowPart = ft.dwLowDateTime;

	// difference between 1970-Jan-01 & 1601-Jan-01 in 100-nanosecond intervals
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
		ret = int( lpFreeBytesAvailable / ( 1024.0 * 1024.0 ) );
	}
	// force it to output positive numbers
	if (ret < 0)	{
		ret = -ret;
	}
	return abs(ret);
}

/*
================
Sys_IsFileOnHdd

Checks whether the disk containing the file incurs seeking penalty.
Taken from Raymond Chen blog: https://devblogs.microsoft.com/oldnewthing/20201023-00/?p=104395
================
*/
HANDLE GetVolumeHandleForFile(const char *filePath) {
	char volumePath[MAX_PATH];
	if (!GetVolumePathName(filePath, volumePath, ARRAYSIZE(volumePath))) {
		return INVALID_HANDLE_VALUE;
	}

	char volumeName[MAX_PATH];
	if (!GetVolumeNameForVolumeMountPoint(volumePath, volumeName, ARRAYSIZE(volumeName))) {
		return INVALID_HANDLE_VALUE;
	}

	size_t length = strlen(volumeName);
	if (length && volumeName[length - 1] == '\\') {
		volumeName[length - 1] = '\0';
	}

	return CreateFile(
		volumeName, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr
	);
}
bool Sys_IsFileOnHdd(const char *filePath) {
	HANDLE volume = GetVolumeHandleForFile(filePath);
	if (volume == INVALID_HANDLE_VALUE) {
		return false;	// supposedly happens for files on network
	}

	// note: if you get compile error here, make sure windows.h is included without WIN32_LEAN_AND_MEAN
	// Note that MFC headers define and require this define, so we can't use PCH in this cpp file
	STORAGE_PROPERTY_QUERY query{};
	query.PropertyId = StorageDeviceSeekPenaltyProperty;
	query.QueryType = PropertyStandardQuery;
	DWORD bytesWritten;
	DEVICE_SEEK_PENALTY_DESCRIPTOR result{};

	BOOL ok = DeviceIoControl(
		volume, IOCTL_STORAGE_QUERY_PROPERTY,
		&query, sizeof(query),
		&result, sizeof(result),
		&bytesWritten, nullptr
	);
	CloseHandle(volume);

	if (ok) {
		return result.IncursSeekPenalty;
	}
	return true;	// supposedly happens for multi-disk volumes
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
