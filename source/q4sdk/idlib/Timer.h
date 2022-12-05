
#ifndef __TIMER_H__
#define __TIMER_H__

/*
===============================================================================

	Clock tick counter. Should only be used for profiling.

===============================================================================
*/

class idTimer {
public:
					idTimer( void );
					idTimer( double clockTicks );
					~idTimer( void );

	idTimer			operator+( const idTimer &t ) const;
	idTimer			operator-( const idTimer &t ) const;
	idTimer &		operator+=( const idTimer &t );
	idTimer &		operator-=( const idTimer &t );

	void			Start( void );
	void			Stop( void );
	void			Clear( void );
	double			ClockTicks( void ) const;
	double			Milliseconds( void ) const;

private:
	static double	base;
	enum			{
						TS_STARTED,
						TS_STOPPED
					} state;
	double			start;
	double			clockTicks;

	void			InitBaseClockTicks( void ) const;
};

/*
=================
idTimer::idTimer
=================
*/
ID_INLINE idTimer::idTimer( void ) {
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
idTimer::idTimer
=================
*/
ID_INLINE idTimer::idTimer( double _clockTicks ) {
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
idTimer::~idTimer
=================
*/
ID_INLINE idTimer::~idTimer( void ) {
}

/*
=================
idTimer::operator+
=================
*/
ID_INLINE idTimer idTimer::operator+( const idTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return idTimer( clockTicks + t.clockTicks );
}

/*
=================
idTimer::operator-
=================
*/
ID_INLINE idTimer idTimer::operator-( const idTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return idTimer( clockTicks - t.clockTicks );
}

/*
=================
idTimer::operator+=
=================
*/
ID_INLINE idTimer &idTimer::operator+=( const idTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
idTimer::operator-=
=================
*/
ID_INLINE idTimer &idTimer::operator-=( const idTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
idTimer::Start
=================
*/
ID_INLINE void idTimer::Start( void ) {
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = idLib::sys->GetClockTicks();
}

/*
=================
idTimer::Stop
=================
*/
ID_INLINE void idTimer::Stop( void ) {
	assert( state == TS_STARTED );
	clockTicks += idLib::sys->GetClockTicks() - start;
	if ( base < 0.0 ) {
		InitBaseClockTicks();
	}
	if ( clockTicks > base ) {
		clockTicks -= base;
	}
	state = TS_STOPPED;
}

/*
=================
idTimer::Clear
=================
*/
ID_INLINE void idTimer::Clear( void ) {
	clockTicks = 0.0;
}

/*
=================
idTimer::ClockTicks
=================
*/
ID_INLINE double idTimer::ClockTicks( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
idTimer::Milliseconds
=================
*/
ID_INLINE double idTimer::Milliseconds( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks / ( idLib::sys->ClockTicksPerSecond() * 0.001 );
}


/*
===============================================================================

	Report of multiple named timers.

===============================================================================
*/

class idTimerReport {
public:
					idTimerReport( void );
					~idTimerReport( void );

	void			SetReportName( const char *name );
	int				AddReport( const char *name );
	void			Clear( void );
	void			Reset( void );
	void			PrintReport( void );
	void			AddTime( const char *name, idTimer *time );

private:
	idList<idTimer*>timers;
	idStrList		names;
	idStr			reportName;
};

#endif /* !__TIMER_H__ */
