// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../decllib/DeclSurfaceType.h"

#include "Script_Thread.h"
#include "Script_Helper.h"
#include "Script_ScriptObject.h"

CLASS_DECLARATION( sdSysCallThread, idThread )
	EVENT( EV_Remove,				idThread::Event_Remove )
END_CLASS

idThread*						idThread::currentThread = NULL;
int								idThread::threadIndex = 0;
idLinkList< idThread >			idThread::threadList;
idList< int >					idThread::threadNumList;
idBlockAlloc< idThread, 16 >	idThread::threadAllocator;

/*
================
idThread::CurrentThread
================
*/
idThread *idThread::CurrentThread( void ) {
	return currentThread;
}

/*
================
idThread::CurrentThreadNum
================
*/
int idThread::CurrentThreadNum( void ) {
	if ( currentThread ) {
		return currentThread->GetThreadNum();
	} else {
		return 0;
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread() {
	threadNode.SetOwner( this );

	interpreter.Init( reinterpret_cast< idProgram* >( gameLocal.program ) );
	interpreter.SetThread( this );

	SetName( "unnamedthread" );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::Init
================
*/
void idThread::Init( idInterpreter *source, const sdProgram::sdFunction* func, int args, bool guiThread ) {
	Init();
	this->guiThread = guiThread;
	interpreter.ThreadCall( source, reinterpret_cast< const function_t* >( func ), args );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::Finalize
================
*/
void idThread::Finalize( void ) {
	idEvent::CancelEvents( this );

	threadNode.Remove();
	GetAutoNode().Remove();

	interpreter.Reset();

	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: end thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}

	threadName.Clear();

	if ( currentThread == this ) {
		currentThread = NULL;
	}

	threadNumList.Alloc() = threadNum;

#ifdef _DEBUG
	SetName( "__dead__" );
#endif // _DEBUG
}

/*
================
idThread::~idThread
================
*/
idThread::~idThread( void ) {
	Finalize();
}

/*
================
idThread::ManualDelete
================
*/
void idThread::ManualDelete( void ) {
	interpreter.terminateOnExit = false;
}

/*
================
idThread::AutoDelete
================
*/
void idThread::AutoDelete( void ) {
	interpreter.terminateOnExit = true;
}

/*
================
idThread::GetFreeThreadNum
================
*/
int idThread::GetFreeThreadNum( void ) {
	if ( !threadNumList.Num() ) {
		return ++threadIndex;
	}

	int index = threadNumList.Num() - 1;
	int value = threadNumList[ index ];
	threadNumList.RemoveIndex( index );
	return value;
}

/*
================
idThread::Init
================
*/
void idThread::Init( void ) {
	threadNum = GetFreeThreadNum();
	threadNode.AddToEnd( threadList );
	
	creationTime = gameLocal.time;
	lastExecuteTime = 0;
	manualControl = false;
	guiThread = false;

	ClearWaitFor();
}

/*
================
idThread::GetThread
================
*/
idThread *idThread::GetThread( int num ) {
	for ( idThread* thread = threadList.Next(); thread; thread = thread->threadNode.Next() ) {
		if ( thread->GetThreadNum() == num ) {
			return thread;
		}
	}

	return NULL;
}

/*
================
idThread::DisplayInfo
================
*/
void idThread::DisplayInfo( void ) {
	gameLocal.Printf( 
		"%12i: '%s'\n"
		"        File: %s(%d)\n"
		"     Created: %d (%d ms ago)\n"
		"      Status: ", 
		threadNum, threadName.c_str(), 
		interpreter.CurrentFile(), interpreter.CurrentLine(), 
		creationTime, gameLocal.time - creationTime );

	if ( interpreter.threadDying ) {
		gameLocal.Printf( "Dying\n" );
	} else if ( interpreter.doneProcessing ) {
		gameLocal.Printf( 
			"Paused since %d (%d ms)\n"
			"      Reason: ",  lastExecuteTime, gameLocal.time - lastExecuteTime );
		if ( waitingUntil ) {
			gameLocal.Printf( "Waiting until %d (%d ms total wait time)\n", waitingUntil, waitingUntil - lastExecuteTime );
		} else {
			gameLocal.Printf( "None\n" );
		}
	} else {
		gameLocal.Printf( "Processing\n" );
	}

	interpreter.DisplayInfo();

	gameLocal.Printf( "\n" );
}

/*
================
idThread::ListThreads
================
*/
void idThread::ListThreads( void ) {
	int n = 0;
	int totalStack = 0;
	for ( idThread* thread = threadList.Next(); thread; thread = thread->threadNode.Next(), n++ ) {
		gameLocal.Printf( "%3i: %-20s : %s(%d)\n", thread->threadNum, thread->threadName.c_str(), thread->interpreter.CurrentFile(), thread->interpreter.CurrentLine() );
		totalStack += thread->interpreter.GetStackSize();
	}
	gameLocal.Printf( "%d active threads\n", n );
	gameLocal.Printf( "%d stack highpoint\n", idInterpreter::s_stackHigh );
}

/*
================
idThread::PruneThreads
================
*/
void idThread::PruneThreads( void ) {
	idThread* next = NULL;
	for ( idThread* thread = threadList.Next(); thread; thread = next ) {
		next = thread->threadNode.Next();

		if ( thread->interpreter.terminateOnExit ) {
			FreeThread( thread );
			continue;
		}	
	}
}


/*
================
idThread::Restart
================
*/
void idThread::Restart( void ) {
	// reset the threadIndex
	threadIndex = 0;

	currentThread = NULL;

	threadAllocator.Shutdown();

	while ( !threadList.IsListEmpty() ) {
		FreeThread( threadList.Next() );
	}
	threadNumList.Clear();

	memset( &trace, 0, sizeof( trace ) );
	trace.c.entityNum = ENTITYNUM_NONE;
}

/*
================
idThread::DelayedStart
================
*/
void idThread::DelayedStart( int delay ) {
	CancelEvents( &EV_Thread_Execute );
	if ( gameLocal.time <= 0 ) {
		delay++;
	}
	if ( guiThread ) {
		PostGUIEventMS( &EV_Thread_Execute, delay );
	} else {
		PostEventMS( &EV_Thread_Execute, delay );
	}
}

/*
================
idThread::SetName
================
*/
void idThread::SetName( const char *name ) {
	threadName = name;
}

/*
================
idThread::End
================
*/
void idThread::End( void ) {
	assert( threadName.Icmp( "__dead__" ) != 0 );

	// Tell thread to die.  It will exit on its own.
	Pause();
	interpreter.threadDying = true;
}

/*
================
idThread::EndThread
================
*/
void idThread::EndThread( void ) {
	assert( threadName.Icmp( "__dead__" ) != 0 );

	interpreter.threadDying = true;
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( const char *name ) {
	for ( idThread* thread = threadList.Next(); thread; thread = thread->threadNode.Next() ) {
		if ( !idStr::Cmp( thread->GetThreadName(), name ) ) {
			thread->End();
		}
	}
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( int num ) {
	idThread* thread = GetThread( num );
	if ( thread != NULL ) {
		// Tell thread to die.  It will delete itself on it's own.
		thread->End();
	} else {
		gameLocal.Warning( "Couldn't Find Thread '%d'", num );
	}
}

/*
================
idThread::Execute
================
*/
bool idThread::Execute( void ) {
	idThread	*oldThread;
	bool		done;

	int now = gameLocal.time;
	if ( guiThread ) {
		now = gameLocal.ToGuiTime( now );
	}

	if ( manualControl && ( waitingUntil > now ) ) {
		return false;
	}

	oldThread = currentThread;
	currentThread = this;

	lastExecuteTime = now;
	ClearWaitFor();
	done = interpreter.Execute();
	if ( done ) {
		End();
		if ( interpreter.terminateOnExit ) {
			PostEventMS( &EV_Remove, 0 );
		}
	} else if ( !manualControl ) {
		if ( waitingUntil > lastExecuteTime ) {
			if ( guiThread ) {
				PostGUIEventMS( &EV_Thread_Execute, waitingUntil - lastExecuteTime );
			} else {
				PostEventMS( &EV_Thread_Execute, waitingUntil - lastExecuteTime );
			}
		} else if ( waitFrame ) {
			waitFrame = false;
			if ( guiThread ) {
				PostGUIEventMS( &EV_Thread_Execute, NEXT_FRAME_EVENT_TIME );
			} else {
				PostEventMS( &EV_Thread_Execute, NEXT_FRAME_EVENT_TIME );
			}
		}
	}

	currentThread = oldThread;

	return done;
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( const sdProgram::sdFunction* func ) {
	ClearWaitFor();
	interpreter.EnterFunction( reinterpret_cast< const function_t* >( func ), true );
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( idScriptObject* object, const sdProgram::sdFunction* func ) {
	assert( object );
	ClearWaitFor();
	interpreter.EnterObjectFunction( object, reinterpret_cast< const function_t* >( func ), true );
}

/*
================
idThread::ClearWaitFor
================
*/
void idThread::ClearWaitFor( void ) {
	waitingUntil		= 0;
	waitFrame			= false;
}

/*
================
idThread::Error
================
*/
void idThread::Error( const char* text ) const {
	interpreter.Error( "%s", text );
}

/*
================
idThread::Warning
================
*/
void idThread::Warning( const char *fmt, ... ) const {
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	interpreter.Warning( "%s", text );
}

/*
================
idThread::CurrentFile
================
*/
const char* idThread::CurrentFile( void ) const {
	return interpreter.CurrentFile();
}

/*
================
idThread::CurrentLine
================
*/
int idThread::CurrentLine( void ) const {
	return interpreter.CurrentLine();
}

/*
================
idThread::StackTrace
================
*/
void idThread::StackTrace( void ) const {
	interpreter.StackTrace();
}

/*
================
idThread::Pause
================
*/
void idThread::Pause( void ) {
	ClearWaitFor();
	interpreter.doneProcessing = true;
}

/*
================
idThread::WaitMS
================
*/
void idThread::WaitMS( int time ) {
	if ( time <= 0 ) {
		WaitFrame();
		return;
	}

	Pause();
	int now = gameLocal.time;
	if ( guiThread ) {
		now = gameLocal.ToGuiTime( now );
	}
	waitingUntil = now + time;
}

/*
================
idThread::Wait
================
*/
void idThread::Wait( float time ) {
	WaitMS( SEC2MS( time ) );
}

/*
================
idThread::WaitFrame
================
*/
void idThread::WaitFrame( void ) {
	Pause();

	// manual control threads don't set waitingUntil so that they can be run again
	// that frame if necessary.
	if ( !manualControl ) {
		waitFrame = true;
	}
}

/*
================
idThread::Assert
================
*/
void idThread::Assert( void ) {
#ifdef _DEBUG
	AssertFailed( interpreter.CurrentFile(), interpreter.CurrentLine(), "Script assertion" );
#endif // _DEBUG
}

/*
================
idThread::Event_Remove
================
*/
void idThread::Event_Remove( void ) {
	OnEventRemove();
	FreeThread( this );
}

/*
================
idThread::AllocThread
================
*/
idThread* idThread::AllocThread( void ) {
	idThread* thread = threadAllocator.Alloc();
	if ( thread->IsDying() ) {
		assert( false );
		thread->Finalize();
	}
	thread->Init();
	thread->AutoDelete();

	return thread;
}

/*
================
idThread::FreeThread
================
*/
void idThread::FreeThread( idThread* thread ) {
	thread->Finalize();
	threadAllocator.Free( thread );
}
