// Copyright (C) 2007 Id Software, Inc.
//

/*
================================================================================

idAutoTimer 

idAutoTimer is a simple derivation of idTimer that can start a timer upon 
instantiation and stop the timer on destruction, outputting the results.

The TIMER_ macros simplify the use of the timers.  To time the execution of an
entire function, just use TIMER_FUNC(); at the top of the function.

Timers are normally active in debug builds.  To disable timers within a file
in a debug build, define DISABLE_AUTO_TIMERS before including AutoTimer.h.

NOTE: classes derived from idAutoTimer must call idAutoTimer::Shutdown in their
destructor to enable auto timer stopping and output.

================================================================================
*/

#ifndef	__TIMERS_H__
#define	__TIMERS_H__

#if defined( DEBUG ) && !defined( DISABLE_AUTO_TIMERS )
	#define	ENABLE_AUTO_TIMERS
#endif

#if defined( ENABLE_AUTO_TIMERS )
	#if ( _MSC_VER >= 1300 )
		#define	TIMER_FUNC()	idTimerConsole funcTimer( __FUNCTION__, true )
	#else
		#define	TIMER_FUNC()
	#endif

	#define TIMER_START( _name_ ) idTimerConsole timer##_name_( #_name_, true )
	#define TIMER_START_EX( _name_, _string_ ) idTimerConsole timer##_name_( #_name_, _string_, true )
	#define	TIMER_STOP( _name_ ) timer##_name_.Stop();
	#define TIMER_MS( _name_ ) timer##_name_.Milliseconds()
	#define TIMER_TICKS( _name_ ) timer##_name_.ClockTicks()
	#define	TIMER_OUT( _name_ ) timer##_name_.Stop(); timer##_name_.idAutoTimer::Output();
#else
	#define TIMER_FUNC()
	#define TIMER_START( _name_ )
	#define TIMER_START_EX( _name_ )
	#define	TIMER_STOP( _name_ )
	#define	TIMER_MS( _name_ )	0
	#define TIMER_TICKS( _name_ )	0
	#define TIMER_OUT( _name_ )
#endif


/*
================================================================================

idAutoTimer 

idTimer that will automatically stop the timer and results when it goes out of 
scope.
================================================================================
*/
class idAutoTimer : public idTimer {
public:
	const char*			name;
	const char *		extraData;

public:
	idAutoTimer( const char *name, const char *extraData, bool start = false ) 
	:	name( name ),
		extraData( extraData ) {
		if ( start ) {
			Clear();
			Start();
		} 
	}
	idAutoTimer( const char *name, bool start = false ) 
	:	name( name ),
		extraData( NULL ) {
		if ( start ) {
			Clear();
			Start();
		} 
	}
	~idAutoTimer( void ) {
		// don't call shutdown here... virtual function table will be hosed already.
		// call it in the derived class's destructor.
	}

	// derived classes should call from destructor to output timer info
	void	Shutdown( void ) {
		// if timer was started and hasn't been stopped, stop on destruction and output
		if ( State() != idTimer::TS_STOPPED ) {
			Stop();
			Output();
		}
		name = NULL;
		extraData = NULL;
	}

	void	Output( void ) const {
		assert( State() == idTimer::TS_STOPPED );
		if ( extraData != NULL ) {
			Output( name, extraData );
		} else {
			Output( name );
		}
	}

	void	OutputMsg( const char *message ) const {
		assert( State() == idTimer::TS_STOPPED );
		if ( extraData != NULL ) {
			OutputMsg( message, name, extraData );
		} else {
			OutputMsg( message, name );
		}
	}

protected:
	// output a standard message
	virtual	void	Output( const char *name ) const = 0;
	// output a standard message with some extra data
	virtual	void	Output( const char *name, const char *extraData ) const = 0;
	
	// output a message formatted with %s (name) and %f (time)
	virtual	void	OutputMsg( const char *message, const char *name ) const = 0;

	// output a message formatted with %s (name), %s (extraData) and %f (time)
	virtual void	OutputMsg( const char *message, const char *name, const char *extraData ) const = 0;
};

/*
================================================================================

idTimerConsole 

Timer that outputs results to the console
================================================================================
*/
class idTimerConsole : public idAutoTimer {
public:
	idTimerConsole( const char *name, bool start = false ) 
		:	idAutoTimer( name, start ) {
	}
	idTimerConsole( const char *name, const char *extraData, bool start = false ) 
		:	idAutoTimer( name, extraData, start ) {
	}

	virtual ~idTimerConsole() {
		idAutoTimer::Shutdown();
	}

protected:
	virtual	void	Output( const char *name ) const {
		common->Printf( "Timer \"%s\" took %.2f ms.\n", name, Milliseconds() );
	}

	virtual	void	Output( const char *name, const char *extraData ) const {
		common->Printf( "Timer \"%s\" ( %s ) took %.2f ms.\n", name, extraData, Milliseconds() );
	}

	virtual	void	OutputMsg( const char *message, const char *name ) const {
		common->Printf( message, name, Milliseconds() );
	}

	virtual void	OutputMsg( const char *message, const char *name, const char *extraData ) const {
		common->Printf( message, name, extraData, Milliseconds() );
	}
};

#endif	/* __TIMERS_H__ */