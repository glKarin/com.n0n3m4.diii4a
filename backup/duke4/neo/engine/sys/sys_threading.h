/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __SYS_THREADING_H__
#define __SYS_THREADING_H__

#ifndef __TYPEINFOGEN__

/*
================================================================================================

	Platform specific mutex, signal, atomic integer and memory barrier.

================================================================================================
*/

#ifdef __ANDROID__
#include <pthread.h>

    typedef pthread_mutex_t		mutexHandle_t;
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
    typedef int					interlockedInt_t;
    #define SYS_MEMORYBARRIER
#else
	typedef CRITICAL_SECTION		mutexHandle_t;
	typedef HANDLE					signalHandle_t;
	typedef LONG					interlockedInt_t;

	// _ReadWriteBarrier() does not translate to any instructions but keeps the compiler
	// from reordering read and write instructions across the barrier.
	// MemoryBarrier() inserts and CPU instruction that keeps the CPU from reordering reads and writes.
	#pragma intrinsic(_ReadWriteBarrier)
	#define SYS_MEMORYBARRIER		_ReadWriteBarrier(); MemoryBarrier()
#endif





/*
================================================================================================

	Platform specific thread local storage.
	Can be used to store either a pointer or an integer.

================================================================================================
*/


	class idSysThreadLocalStorage {
	public:
#ifdef __ANDROID__
        static void close_thread(void* value) {
            /*多是为了释放value指针指向的资源*/
        }
        idSysThreadLocalStorage() {
            pthread_key_create(&tlsIndex, idSysThreadLocalStorage::close_thread);
        }
        idSysThreadLocalStorage( const ptrdiff_t &val )
        : idSysThreadLocalStorage()
        {
            pthread_setspecific( tlsIndex, (const void *)val );
        }
        ~idSysThreadLocalStorage() {
            pthread_key_delete( tlsIndex );
        }
        operator ptrdiff_t() {
            return (ptrdiff_t)pthread_getspecific( tlsIndex );
        }
        const ptrdiff_t & operator = ( const ptrdiff_t &val ) {
            pthread_setspecific( tlsIndex, (const void *)val );
            return val;
        }
        pthread_key_t	tlsIndex;
#else
		idSysThreadLocalStorage() { 
			tlsIndex = TlsAlloc();
		}
		idSysThreadLocalStorage( const ptrdiff_t &val ) {
			tlsIndex = TlsAlloc();
			TlsSetValue( tlsIndex, (LPVOID)val );
		}
		~idSysThreadLocalStorage() {
			TlsFree( tlsIndex );
		}
		operator ptrdiff_t() {
			return (ptrdiff_t)TlsGetValue( tlsIndex );
		}
		const ptrdiff_t & operator = ( const ptrdiff_t &val ) {
			TlsSetValue( tlsIndex, (LPVOID)val );
			return val;
		}	
		DWORD	tlsIndex;
#endif
	};

#define ID_TLS idSysThreadLocalStorage


#endif // __TYPEINFOGEN__

/*
================================================================================================

	Platform independent threading functions.

================================================================================================
*/

enum core_t {
	CORE_ANY = -1,
	CORE_0A,
	CORE_0B,
	CORE_1A,
	CORE_1B,
	CORE_2A,
	CORE_2B
};

//k typedef unsigned int (*xthread_t)( void * );
typedef void *(*xthread_t)(void *);

enum xthreadPriority {
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
uintptr_t			Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, 
									  const char *name, core_t core, int stackSize = DEFAULT_THREAD_STACK_SIZE, 
									  bool suspended = false );

void				Sys_WaitForThread( uintptr_t threadHandle );
void				Sys_DestroyThread( uintptr_t threadHandle );
void				Sys_SetCurrentThreadName( const char *name );

void				Sys_SignalCreate( signalHandle_t & handle, bool manualReset );
void				Sys_SignalDestroy( signalHandle_t & handle );
void				Sys_SignalRaise( signalHandle_t & handle );
void				Sys_SignalClear( signalHandle_t & handle );
bool				Sys_SignalWait( signalHandle_t & handle, int timeout );

void				Sys_MutexCreate( mutexHandle_t & handle );
void				Sys_MutexDestroy( mutexHandle_t & handle );
bool				Sys_MutexLock( mutexHandle_t & handle, bool blocking );
void				Sys_MutexUnlock( mutexHandle_t & handle );

interlockedInt_t	Sys_InterlockedIncrement( interlockedInt_t & value );
interlockedInt_t	Sys_InterlockedDecrement( interlockedInt_t & value );

interlockedInt_t	Sys_InterlockedAdd( interlockedInt_t & value, interlockedInt_t i );
interlockedInt_t	Sys_InterlockedSub( interlockedInt_t & value, interlockedInt_t i );

interlockedInt_t	Sys_InterlockedExchange( interlockedInt_t & value, interlockedInt_t exchange );
interlockedInt_t	Sys_InterlockedCompareExchange( interlockedInt_t & value, interlockedInt_t comparand, interlockedInt_t exchange );

void *				Sys_InterlockedExchangePointer( void * & ptr, void * exchange );
void *				Sys_InterlockedCompareExchangePointer( void * & ptr, void * comparand, void * exchange );

void				Sys_Yield();

const int MAX_CRITICAL_SECTIONS		= 4;

enum {
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE
};


/*
================================================================================================

	Mutex

================================================================================================
*/

/*
========================
Sys_MutexCreate
========================
*/
ID_INLINE void Sys_MutexCreate(mutexHandle_t& handle) {
#ifdef __ANDROID__
	pthread_mutexattr_t attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle, &attr );

	pthread_mutexattr_destroy( &attr );
#else
	InitializeCriticalSection(&handle);
#endif
}

/*
========================
Sys_MutexDestroy
========================
*/
ID_INLINE void Sys_MutexDestroy(mutexHandle_t& handle) {
#ifdef __ANDROID__
	pthread_mutex_destroy( &handle );
#else
    DeleteCriticalSection(&handle);
#endif
}

/*
========================
Sys_MutexLock
========================
*/
ID_INLINE bool Sys_MutexLock(mutexHandle_t& handle, bool blocking) {
#ifdef __ANDROID__
	if( pthread_mutex_trylock( &handle ) != 0 )
	{
		if( !blocking )
		{
			return false;
		}
		pthread_mutex_lock( &handle );
	}
	return true;
#else
	if (TryEnterCriticalSection(&handle) == 0) {
		if (!blocking) {
			return false;
		}
		EnterCriticalSection(&handle);
	}
	return true;
#endif
}

/*
========================
Sys_MutexUnlock
========================
*/
ID_INLINE void Sys_MutexUnlock(mutexHandle_t& handle) {
#ifdef __ANDROID__
	pthread_mutex_unlock( & handle );
#else
	LeaveCriticalSection(&handle);
#endif
}


#endif	// !__SYS_THREADING_H__
