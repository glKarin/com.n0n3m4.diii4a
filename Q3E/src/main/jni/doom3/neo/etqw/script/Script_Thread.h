// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_THREAD_H__
#define __SCRIPT_THREAD_H__

#include "Script_Interpreter.h"
#include "Script_SysCall.h"

class idThread : public sdSysCallThread {
private:
	int								waitingUntil;
	bool							waitFrame;
	idInterpreter					interpreter;
									
	int 							threadNum;
	idStr 							threadName;

	idLinkList< idThread >			threadNode;

	int								lastExecuteTime;
	int								creationTime;

	bool							manualControl;
	bool							guiThread;

	static idThread*				currentThread;
	static int						threadIndex;
	static idLinkList< idThread >	threadList;
	static idList< int >			threadNumList;

	static idBlockAlloc< idThread, 16 > threadAllocator;

	void						Init( void );
	void						Pause( void );
	void						Finalize( void );

public:							
								CLASS_PROTOTYPE( idThread );
								
								idThread();

	void						Init( idInterpreter *source, const sdProgram::sdFunction* func, int args, bool guiThread );

	virtual						~idThread();

								// tells the thread manager not to delete this thread when it ends
	void						ManualDelete( void );
	void						AutoDelete( void );

	void						WaitMS( int time );
	void						WaitFrame( void );
								
	virtual void				Wait( float time );
	virtual void				Assert( void );

	virtual void				CallFunction( const sdProgram::sdFunction* func );
	virtual void				CallFunction( idScriptObject* object, const sdProgram::sdFunction* func );
	virtual void				SetName( const char* name );
	virtual bool				IsWaiting( void ) const { return waitingUntil > gameLocal.time; }
	virtual bool				IsDying( void ) const { return interpreter.threadDying; };

	idInterpreter&				GetInterpreter( void ) { return interpreter; }
	void						DisplayInfo();
	static idThread*			GetThread( int num );
	static void					ListThreads( void );
	static void					PruneThreads( void );
	static void					Restart( void );
	static idThread*			AllocThread( void );
	static void					FreeThread( idThread* thread );

	static idLinkList<idThread>*GetThreads ( void );
									
	bool						IsDoneProcessing ( void );
								
	void						End( void );
	static void					KillThread( const char *name );
	static void					KillThread( int num );
	bool						Execute( void );
	void						ManualControl( void ) { manualControl = true; CancelEvents( &EV_Thread_Execute ); };
	void						DoneProcessing( void ) { interpreter.doneProcessing = true; };
	void						ContinueProcessing( void ) { interpreter.doneProcessing = false; };
	void						EndThread( void );
	void						ClearWaitFor( void );
	void						DelayedStart( int delay );
	void						SetThreadNum( int num );
	int 						GetThreadNum( void );
	const char					*GetThreadName( void );

	void						Error( const char* text ) const;
	void						Warning( const char *fmt, ... ) const;
								
	static int					GetFreeThreadNum( void );
	static idThread*			CurrentThread( void );
	static int					CurrentThreadNum( void );

	virtual const char*			CurrentFile( void ) const;
	virtual int					CurrentLine( void ) const;
	virtual void				StackTrace( void ) const;

	void						Event_Remove( void );
};

/*
================
idThread::SetThreadNum
================
*/
ID_INLINE void idThread::SetThreadNum( int num ) {
	threadNum = num;
}

/*
================
idThread::GetThreadNum
================
*/
ID_INLINE int idThread::GetThreadNum( void ) {
	return threadNum;
}

/*
================
idThread::GetThreadName
================
*/
ID_INLINE const char *idThread::GetThreadName( void ) {
	return threadName.c_str();
}

/*
================
idThread::GetThreads
================
*/
ID_INLINE idLinkList<idThread>* idThread::GetThreads ( void ) {
	return &threadList;
}	

/*
================
idThread::IsDoneProcessing
================
*/
ID_INLINE bool idThread::IsDoneProcessing ( void ) {
	return interpreter.doneProcessing;
}

#endif /* !__SCRIPT_THREAD_H__ */
