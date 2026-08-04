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
#include <stdio.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int: 
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
		 or is it 48 days? the specs say int, but maybe it's casted from unsigned int?
*/
int Sys_Milliseconds(void)
{
	int curtime;
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	curtime = (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;

	return curtime;
}

#define STAT_BUF 100

int main(int argc, void *argv[]) {
	int start = 30; // start waiting with 30 ms 
	int dec = 2; // decrement by 2 ms
	int min = 4; // min wait test
	int i, j, now, next;
	int stats[STAT_BUF];

	struct sched_param parm;
	
	Sys_Milliseconds(); // init

	// set schedule policy to see if that affects usleep
	// (root rights required for that)
	parm.sched_priority = 99;
	if ( sched_setscheduler(0, SCHED_RR, &parm) != 0 ) {
		printf("sched_setscheduler SCHED_RR failed: %s\n", strerror(errno) );
	} else {
		printf("sched_setscheduler SCHED_RR ok\n");
	}
	
	// now run the test
	for( i = start ; i >= min ; i -= dec ) {
		printf( "sleep %d ms", i );
		for( j = 0 ; j < STAT_BUF ; j++ ) {
			now = Sys_Milliseconds();
			usleep(i*1000);			
			stats[j] = Sys_Milliseconds() - now;
		}
		for( j = 0; j < STAT_BUF; j++) {
			if ( ! (j & 0xf) ) {
				printf("\n");
			}
			printf( "%d ", stats[j] );
		}
		printf("\n");
	}
	return 0;
}
