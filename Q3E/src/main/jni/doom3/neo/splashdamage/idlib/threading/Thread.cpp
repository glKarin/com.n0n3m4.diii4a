// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../sys/threading/SysThread.h"

/*
=============
sdThread::sdThread
=============
*/
sdThread::sdThread( sdThreadProcess* process, threadPriority_e priority, unsigned int stackSize ) :
		priority( priority ),
		isWorker( false ),
		isRunning( false ),
		isStopping( false ) {

	parms.process = process;
	parms.thread = this;

	sdSysThread::Create( (threadProc_t)ThreadProc, &parms, handle, priority, stackSize );
}

/*
=============
sdThread::~sdThread
=============
*/
sdThread::~sdThread() {
	Mem_Free( parms.parm );
	parms.parm = NULL;

	sdSysThread::Destroy( handle );
}

/*
=============
sdThread::Destroy
=============
*/
void sdThread::Destroy() {
	delete this;
}

/*
=============
sdThread::Start
=============
*/
bool sdThread::Start( const void *parm, size_t size ) {
	if ( NULL == parms.process || isRunning ) {
		return false;
	}

	if ( NULL != parm ) {
		parms.parm = Mem_Alloc( size );
		::memcpy( parms.parm, parm, size );
	} else {
		parms.parm = NULL;
	}
	parms.process->Start();
	
	if ( !sdSysThread::Start( handle ) ) {
		return false;
	}

	isRunning = true;
	return true;
}

/*
=============
sdThread::StartWorker
=============
*/
bool sdThread::StartWorker( const void *parm, size_t size ) {
	isWorker = true;
#ifdef _WIN32
	hEventWorkerDone = CreateEvent( NULL, FALSE, FALSE, NULL );
	hEventMoreWorkToDo = CreateEvent( NULL, FALSE, FALSE, NULL );
#endif

	bool result = Start( parm, size );

#ifdef _WIN32
	DWORD dwRet = WaitForSingleObject( hEventWorkerDone, INFINITE );
	dwRet;	// shut compiler up
#endif

	return result;
}

/*
=============
sdThread::SignalWork
=============
*/
void sdThread::SignalWork() {
#ifdef _WIN32
	SetEvent( hEventMoreWorkToDo );
#endif
}

/*
=============
sdThread::Stop
=============
*/
void sdThread::Stop() {
	isStopping = true;
	parms.process->Stop();
}

/*
=============
sdThread::Join
=============
*/
void sdThread::Join() {
	if ( isWorker ) {
#ifdef _WIN32
		DWORD dwRet = WaitForSingleObject( hEventWorkerDone, INFINITE );
		dwRet;	// shut compiler up
#endif
	} else {
		sdSysThread::Join( handle );
		Mem_Free( parms.parm );
		parms.parm = NULL;
	}
}

/*
=============
sdThread::SetPriority
=============
*/
void sdThread::SetPriority( const threadPriority_e priority ) {
	sdSysThread::SetPriority( handle, priority );
}

/*
=============
sdThread::SetProcessor
=============
*/
void sdThread::SetProcessor( const unsigned int processor ) {
	sdSysThread::SetProcessor( handle, processor );
}

/*
=============
sdThread::SetName
=============
*/
void sdThread::SetName( const char* name ) {
	this->name = name;
	sdSysThread::SetName( handle, name );
}

#if defined( _WIN32 )

/*
=============
sdThread::ThreadProc
=============
*/
unsigned int sdThread::ThreadProc( sdThreadParms* parms ) {

	// This doesn't appear to work for suspended threads - so we call it once more on thread start
	sdSysThread::SetName( parms->thread->handle, parms->thread->name.c_str() );

	unsigned int retVal;
	if ( parms->thread->isWorker ) {
		do {
#ifdef _WIN32
			DWORD dwRet = SignalObjectAndWait( parms->thread->hEventWorkerDone, parms->thread->hEventMoreWorkToDo, INFINITE, FALSE );
			dwRet;	// shut compiler up
#endif
			retVal = parms->process->Run( parms->parm );

		} while( !parms->thread->isStopping );
	} else {
		retVal = parms->process->Run( parms->parm );
	}

	parms->thread->isRunning = false;

	return sdSysThread::Exit( retVal );
}

#else

/*
=============
sdThread::ThreadProc
=============
*/
void* sdThread::ThreadProc( void* p ) {
	sdThreadParms *parms = static_cast< sdThreadParms* >( p );
	unsigned int retVal = parms->process->Run( parms->parm );
	return sdSysThread::Exit( retVal );
}

#endif
