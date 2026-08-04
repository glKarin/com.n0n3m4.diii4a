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
#ifndef __SYS_THREADING_H__
#define __SYS_THREADING_H__

#ifndef __TYPEINFOGEN__

/*
================================================================================================

	Platform specific mutex, signal, atomic integer and memory barrier.

================================================================================================
*/

// RB begin
#if defined(_WIN32)
typedef CRITICAL_SECTION		mutexHandle_t;
typedef HANDLE					signalHandle_t;
typedef LONG					interlockedInt_t;
#else

#include <pthread.h>

struct signalHandle_t
{
	// DG: all this stuff is needed to emulate Window's Event API
	//     (CreateEvent(), SetEvent(), WaitForSingleObject(), ...)
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int waiting; // number of threads waiting for a signal
	bool manualReset;
	bool signaled; // is it signaled right now?
};

typedef pthread_mutex_t			mutexHandle_t;
typedef int						interlockedInt_t;
#endif
// RB end

// _ReadWriteBarrier() does not translate to any instructions but keeps the compiler
// from reordering read and write instructions across the barrier.
// MemoryBarrier() inserts and CPU instruction that keeps the CPU from reordering reads and writes.
#if defined(_MSC_VER)
#pragma intrinsic(_ReadWriteBarrier)
#define SYS_MEMORYBARRIER		_ReadWriteBarrier(); MemoryBarrier()
#elif defined(__GNUC__) // FIXME: what about clang?
// according to http://en.wikipedia.org/wiki/Memory_ordering the following should be equivalent to the stuff above..
//#ifdef __sync_syncronize
#define SYS_MEMORYBARRIER		asm volatile("" ::: "memory");__sync_synchronize()
#endif




/*
================================================================================================

	Platform specific thread local storage.
	Can be used to store either a pointer or an integer.

================================================================================================
*/

// RB: added POSIX implementation
#if defined(_WIN32)
class idSysThreadLocalStorage
{
public:
	idSysThreadLocalStorage()
	{
		tlsIndex = TlsAlloc();
	}

	idSysThreadLocalStorage( const ptrdiff_t& val )
	{
		tlsIndex = TlsAlloc();
		TlsSetValue( tlsIndex, (LPVOID)val );
	}

	~idSysThreadLocalStorage()
	{
		TlsFree( tlsIndex );
	}

	operator ptrdiff_t()
	{
		return (ptrdiff_t)TlsGetValue( tlsIndex );
	}

	const ptrdiff_t& operator = ( const ptrdiff_t& val )
	{
		TlsSetValue( tlsIndex, (LPVOID)val );
		return val;
	}

	DWORD	tlsIndex;
};
#else
class idSysThreadLocalStorage
{
public:
	idSysThreadLocalStorage()
	{
		pthread_key_create( &key, NULL );
	}

	idSysThreadLocalStorage( const ptrdiff_t& val )
	{
		pthread_key_create( &key, NULL );
		pthread_setspecific( key, (const void*)val );
	}

	~idSysThreadLocalStorage()
	{
		pthread_key_delete( key );
	}

	operator ptrdiff_t()
	{
		return (ptrdiff_t)pthread_getspecific( key );
	}

	const ptrdiff_t& operator = ( const ptrdiff_t& val )
	{
		pthread_setspecific( key, (const void*)val );
		return val;
	}

	pthread_key_t	key;
};
#endif
// RB end

#define ID_TLS idSysThreadLocalStorage


#endif // __TYPEINFOGEN__

/*
================================================================================================

	Platform independent threading functions.

================================================================================================
*/

enum core_t
{
	CORE_ANY = -1,
	CORE_0A,
	CORE_0B,
	CORE_1A,
	CORE_1B,
	CORE_2A,
	CORE_2B
};

typedef unsigned int ( *xthread_t )( void* );

enum xthreadPriority
{
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
};

#define DEFAULT_THREAD_STACK_SIZE		( 256 * 1024 )

// on win32, the threadID is NOT the same as the threadHandle
uintptr_t			Sys_GetCurrentThreadID();

// returns a threadHandle
uintptr_t			Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority,
	const char* name, core_t core = CORE_ANY, int stackSize = DEFAULT_THREAD_STACK_SIZE,
	bool suspended = false );

// RB begin
// removed unused Sys_WaitForThread
void				Sys_DestroyThread( uintptr_t threadHandle );
void				Sys_SetCurrentThreadName( const char* name );

void				Sys_SignalCreate( signalHandle_t& handle, bool manualReset );
void				Sys_SignalDestroy( signalHandle_t& handle );
void				Sys_SignalRaise( signalHandle_t& handle );
void				Sys_SignalClear( signalHandle_t& handle );
bool				Sys_SignalWait( signalHandle_t& handle, int timeout );

void				Sys_MutexCreate( mutexHandle_t& handle );
void				Sys_MutexDestroy( mutexHandle_t& handle );
bool				Sys_MutexLock( mutexHandle_t& handle, bool blocking );
void				Sys_MutexUnlock( mutexHandle_t& handle );

interlockedInt_t	Sys_InterlockedIncrement( interlockedInt_t& value );
interlockedInt_t	Sys_InterlockedDecrement( interlockedInt_t& value );

interlockedInt_t	Sys_InterlockedAdd( interlockedInt_t& value, interlockedInt_t i );
interlockedInt_t	Sys_InterlockedSub( interlockedInt_t& value, interlockedInt_t i );

interlockedInt_t	Sys_InterlockedExchange( interlockedInt_t& value, interlockedInt_t exchange );
interlockedInt_t	Sys_InterlockedCompareExchange( interlockedInt_t& value, interlockedInt_t comparand, interlockedInt_t exchange );

void* Sys_InterlockedExchangePointer( void*& ptr, void* exchange );
void* Sys_InterlockedCompareExchangePointer( void*& ptr, void* comparand, void* exchange );

void				Sys_Yield();

const int MAX_CRITICAL_SECTIONS = 4;

enum criticalSection_t
{
	CRITICAL_SECTION_ZERO = 0,	// general purpose
	CRITICAL_SECTION_ONE,		// sound
	CRITICAL_SECTION_TWO,		// front end parallel jobs
	CRITICAL_SECTION_THREE,		// cinematic log
};

#endif	// !__SYS_THREADING_H__
