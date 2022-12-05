
#include "precompiled.h"
#pragma hdrstop

double idTimer::base = -1.0;


/*
=================
idTimer::InitBaseClockTicks
=================
*/
void idTimer::InitBaseClockTicks( void ) const {
	idTimer timer;
	double ct, b;
	int i;

	base = 0.0;
	b = -1.0;
	for ( i = 0; i < 1000; i++ ) {
		timer.Clear();
		timer.Start();
		timer.Stop();
		ct = timer.ClockTicks();
		if ( b < 0.0 || ct < b ) {
			b = ct;
		}
	}
	base = b;
}


/*
=================
idTimerReport::idTimerReport
=================
*/
idTimerReport::idTimerReport() {
}

/*
=================
idTimerReport::SetReportName
=================
*/
void idTimerReport::SetReportName( const char *name ) {
	reportName = ( name ) ? name : "Timer Report";
}

/*
=================
idTimerReport::~idTimerReport
=================
*/
idTimerReport::~idTimerReport() {
	Clear();
}

/*
=================
idTimerReport::AddReport
=================
*/
int idTimerReport::AddReport( const char *name ) {
	if ( name && *name ) {
		names.Append( name );
		return timers.Append( new idTimer() );
	}
	return -1;
}

/*
=================
idTimerReport::Clear
=================
*/
void idTimerReport::Clear() {
	timers.DeleteContents( true );
	names.Clear();
	reportName.Clear();
}

/*
=================
idTimerReport::Reset
=================
*/
void idTimerReport::Reset() {
	assert ( timers.Num() == names.Num() );
	for ( int i = 0; i < timers.Num(); i++ ) {
		timers[i]->Clear();
	}
}

/*
=================
idTimerReport::AddTime
=================
*/
void idTimerReport::AddTime( const char *name, idTimer *time ) {
	assert ( timers.Num() == names.Num() );
	int i;
	for ( i = 0; i < names.Num(); i++ ) {
		if ( names[i].Icmp( name ) == 0 ) {
			*timers[i] += *time;
			break;
		}
	}
	if ( i == names.Num() ) {
		int index = AddReport( name );
		if ( index >= 0 ) {
			timers[index]->Clear();
			*timers[index] += *time;
		}
	}
}

/*
=================
idTimerReport::PrintReport
=================
*/
void idTimerReport::PrintReport() {
	assert( timers.Num() == names.Num() );
	idLib::common->Printf( "Timing Report for %s\n", reportName.c_str() );
	idLib::common->Printf( "-------------------------------\n" );
	float total = 0.0f;
	for ( int i = 0; i < names.Num(); i++ ) {
		idLib::common->Printf( "%s consumed %5.2f seconds\n", names[i].c_str(), timers[i]->Milliseconds() * 0.001f );
		total += timers[i]->Milliseconds();
	}
	idLib::common->Printf( "Total time for report %s was %5.2f\n\n", reportName.c_str(), total * 0.001f );
}
