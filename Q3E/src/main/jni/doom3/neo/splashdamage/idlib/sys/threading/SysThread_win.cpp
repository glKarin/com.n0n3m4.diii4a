// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "SysThread.h"
#include <process.h>

/*
=============
sdSysThread::Create
=============
*/
bool sdSysThread::Create( threadProc_t proc, void* parms, threadHandle_t& handle, threadPriority_e priority, unsigned int stackSize ) {
	handle.handle = reinterpret_cast< HANDLE >( ::_beginthreadex( NULL, stackSize, proc, parms, CREATE_SUSPENDED, &handle.id ) );
	if ( NULL == handle.handle ) {
		return false;
	}

	SetPriority( handle, priority );

	return true;
}

/*
=============
sdSysThread::Start
=============
*/
bool sdSysThread::Start( threadHandle_t& handle ) {
	if ( -1 == ::ResumeThread( handle.handle ) ) {
		return false;
	}
	return true;
}

/*
=============
sdSysThread::Suspend
=============
*/
bool sdSysThread::Suspend( threadHandle_t& handle ) {
	if ( -1 == ::SuspendThread( handle.handle ) ) {
		return false;
	}
	return true;
}

/*
=============
sdSysThread::Resume
=============
*/
bool sdSysThread::Resume( threadHandle_t& handle ) {
	if ( -1 == ::ResumeThread( handle.handle ) ) {
		return false;
	}
	return true;
}

/*
=============
sdSysThread::Exit
=============
*/
unsigned int sdSysThread::Exit( unsigned int retVal ) {
	::_endthreadex( retVal );
	return retVal;
}

/*
=============
sdSysThread::Join
=============
*/
void sdSysThread::Join( threadHandle_t& handle ) {
	::WaitForSingleObject( handle.handle, INFINITE );
}

/*
=============
sdSysThread::Destroy
=============
*/
void sdSysThread::Destroy( threadHandle_t& handle ) {
	::CloseHandle( handle.handle );
}

/*
=============
sdSysThread::SetPriority
=============
*/
void sdSysThread::SetPriority( threadHandle_t& handle, const threadPriority_e priority ) {
	switch( priority ) {
		case THREAD_LOWEST:
			::SetThreadPriority( handle.handle, THREAD_PRIORITY_LOWEST );
			break;
		case THREAD_BELOW_NORMAL:
			::SetThreadPriority( handle.handle, THREAD_PRIORITY_BELOW_NORMAL );
			break;
		default:
		case THREAD_NORMAL:
			::SetThreadPriority( handle.handle, THREAD_PRIORITY_NORMAL );
			break;
		case THREAD_ABOVE_NORMAL:
			::SetThreadPriority( handle.handle, THREAD_PRIORITY_ABOVE_NORMAL );
			break;
		case THREAD_HIGHEST:
			::SetThreadPriority( handle.handle, THREAD_PRIORITY_HIGHEST );
			break;
	}
}

/*
=============
sdSysThread::SetProcessor
=============
*/
void sdSysThread::SetProcessor( threadHandle_t& handle, const unsigned int processor ) {
#if defined( _XENON )
	::XSetThreadProcessor( handle.handle, processor );
#else
	::SetThreadAffinityMask( handle.handle, 1 << processor );
#endif	
}

/*
=============
sdSysThread::SetName
=============
*/
void sdSysThread::SetName( threadHandle_t& handle, const char* name ) {
//#if defined( _WIN32_WINNT ) && ( _WIN32_WINNT >=0x0501 )
	struct THREADNAME_INFO {
		DWORD	dwType;		// must be 0x1000
		LPCSTR	szName;		// pointer to name (in user addr space)
		DWORD	dwThreadID;	// thread ID (-1=caller thread)
		DWORD	dwFlags;	// reserved for future use, must be zero
	};

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	//info.dwThreadID = ::GetThreadId( handle );
	info.dwThreadID = handle.id;
	info.dwFlags = 0;

	if ( info.dwThreadID != 0 ) {
		__try {
			::RaiseException( 0x406D1388, 0, sizeof( info ) / sizeof( DWORD ), (ULONG_PTR *)&info );
		}
		__except( EXCEPTION_CONTINUE_EXECUTION ) {
		}
	}
//#endif
}
