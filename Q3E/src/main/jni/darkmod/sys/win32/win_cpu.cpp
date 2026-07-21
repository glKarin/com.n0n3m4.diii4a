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

#include <float.h>
#include "win_local.h"

#include "intrin.h"
#include "immintrin.h"

/*
==============================================================

	Clock ticks

==============================================================
*/

/*
================
Sys_GetClockTicks
================
*/
uint64 Sys_GetClockTicks( void ) {
#if 0

	LARGE_INTEGER li;

	QueryPerformanceCounter( &li );
	return = ( double ) li.LowPart + ( double ) 0xFFFFFFFF * li.HighPart;

#else

	/*// stgatilov: serialize pipeline with cpuid instruction
	int values[4];
	__cpuid( values, 0 );*/
	// greebo: Use the intrinsic provided by the VC++ compiler in x64
	unsigned __int64 ticks = __rdtsc();
	return ticks;

#endif
}

/*
================
Sys_ClockTicksPerSecond
================
*/
uint64 Sys_ClockTicksPerSecond( void ) {
	static uint64 ticks = 0;
#if 0

	if ( !ticks ) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticks = li.QuadPart;
	}

#else

	if ( !ticks ) {
		HKEY hKey;
		uint64_t ProcSpeed;
		DWORD buflen, ret;

		if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey ) ) {
			ProcSpeed = 0;
			buflen = sizeof( ProcSpeed );
			ret = RegQueryValueEx( hKey, "~MHz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			// If we don't succeed, try some other spellings.
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~Mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			RegCloseKey( hKey );
			if ( ret == ERROR_SUCCESS ) {
				ticks = ( double )( ( unsigned long )ProcSpeed ) * 1000000;
			}
		}
	}

#endif
	return ticks;
}


/*
==============================================================

	CPU

==============================================================
*/

/*
========================
CountSetBits 
Helper function to count set bits in the processor mask.
========================
*/
DWORD CountSetBits( ULONG_PTR bitMask ) {
	DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    

	for ( DWORD i = 0; i <= LSHIFT; i++ ) {
		bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
		bitTest /= 2;
	}

	return bitSetCount;
}

typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD );

enum LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL {
	localRelationProcessorCore,
	localRelationNumaNode,
	localRelationCache,
	localRelationProcessorPackage
};

struct cpuInfo_t {
	int processorPackageCount;
	int processorCoreCount;
	int logicalProcessorCount;
	int numaNodeCount;
	struct cacheInfo_t {
		int count;
		int associativity;
		int lineSize;
		int size;
	} cacheLevel[3];
};

/*
========================
GetCPUInfo
========================
*/
bool GetCPUInfo( cpuInfo_t & cpuInfo ) {
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
	BOOL		done = FALSE;
	DWORD		returnLength = 0;
	DWORD		byteOffset = 0;

	memset( & cpuInfo, 0, sizeof( cpuInfo ) );

	glpi = (LPFN_GLPI)GetProcAddress( GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation" );
	if ( NULL == glpi ) {
		idLib::Printf( "\nGetLogicalProcessorInformation is not supported.\n" );
		return 0;
	}

	while ( !done ) {
		DWORD rc = glpi( buffer, &returnLength );

		if ( FALSE == rc ) {
			if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
				if ( buffer ) {
					free( buffer );
				}

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc( returnLength );
			} else {
				idLib::Printf( "Sys_CPUCount error: %d\n", GetLastError() );
				return false;
			}
		} else {
			done = TRUE;
		}
	}

	ptr = buffer;

	while ( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ) {
		switch ( (LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL) ptr->Relationship ) {
		case localRelationProcessorCore:
			cpuInfo.processorCoreCount++;

			// A hyperthreaded core supplies more than one logical processor.
			cpuInfo.logicalProcessorCount += CountSetBits( ptr->ProcessorMask );
			break;

		case localRelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			cpuInfo.numaNodeCount++;
			break;

		case localRelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			Cache = &ptr->Cache;
			if ( Cache->Level >= 1 && Cache->Level <= 3 ) {
				int level = Cache->Level - 1;
				if ( cpuInfo.cacheLevel[level].count > 0 ) {
					cpuInfo.cacheLevel[level].count++;
				} else {
					cpuInfo.cacheLevel[level].associativity = Cache->Associativity;
					cpuInfo.cacheLevel[level].lineSize = Cache->LineSize;
					cpuInfo.cacheLevel[level].size = Cache->Size;
				}
			}
			break;

		case localRelationProcessorPackage:
			// Logical processors share a physical package.
			cpuInfo.processorPackageCount++;
			break;

		default:
			idLib::Printf( "Error: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n" );
			break;
		}
		byteOffset += sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
		ptr++;
	}

	free( buffer );

	return true;
}

/*
========================
Sys_GetCPUCacheSize
========================
*/
void Sys_GetCPUCacheSize( int level, int & count, int & size, int & lineSize ) {
	assert( level >= 1 && level <= 3 );
	cpuInfo_t cpuInfo;

	GetCPUInfo( cpuInfo );

	count = cpuInfo.cacheLevel[level - 1].count;
	size = cpuInfo.cacheLevel[level - 1].size;
	lineSize = cpuInfo.cacheLevel[level - 1].lineSize;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int & numLogicalCPUCores, int & numPhysicalCPUCores, int & numCPUPackages ) {
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );

	numPhysicalCPUCores = cpuInfo.processorCoreCount;
	numLogicalCPUCores = cpuInfo.logicalProcessorCount;
	numCPUPackages = cpuInfo.processorPackageCount;
}
