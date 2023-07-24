// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __THREAD_H__
#define __THREAD_H__

class sdThread;
class sdThreadProcess;

struct sdThreadParms {
						sdThreadParms() :
							thread( NULL ),
							process( NULL ),
							parm( NULL ) {
						}

	sdThread*			thread;
	sdThreadProcess*	process;
	void*				parm;
};

class sdThread {
public:
						sdThread( sdThreadProcess* process, threadPriority_e priority = THREAD_NORMAL, unsigned int stackSize = 0 );

	void				Destroy();

	bool				Start( const void *parm = NULL, size_t size = 0 );
	bool				StartWorker( const void *parm = NULL, size_t size = 0 );
	void				SignalWork();
	void				Stop();
	void				Join();

	void				SetPriority( const threadPriority_e priority );
	void				SetProcessor( const unsigned int processor );

	bool				IsRunning() const;

	void				SetName( const char* name );
	const char*			GetName() const;

protected:
						~sdThread();

#if defined( _WIN32 )
	static unsigned int	ThreadProc( sdThreadParms* parms );
#else
	static void*		ThreadProc( void* parms );
#endif

protected:
	sdThreadParms		parms;
	threadPriority_e	priority;
	threadHandle_t		handle;
	bool				isWorker;
	bool				isRunning;
	bool				isStopping;
	idStr				name;

#ifdef _WIN32
	HANDLE				hEventWorkerDone;
	HANDLE				hEventMoreWorkToDo;
#endif
};

ID_INLINE bool sdThread::IsRunning() const {
	return isRunning;
}

ID_INLINE const char* sdThread::GetName() const {
	return name.c_str();
}

#endif /* !__THREAD_H__ */
