/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"


void CG_NewLapTime( int lap, int time ) {
	centity_t	*cent;
	char		*t;

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		t = getStringForTime( cent->bestLapTime );

		Com_Printf("You got a personal record lap time of %s!\n", t);
	}

	cent->currentLap = lap;
	cent->lastStartLapTime = cent->startLapTime;
	cent->startLapTime = time;
}

void CG_FinishedRace( int client, int time ) {
	centity_t	*cent;
	char		*t;

	cent = &cg_entities[client];

	if ( client == cg.snap->ps.clientNum
		&& ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0) ){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		t = getStringForTime( cent->bestLapTime );

		Com_Printf("You got a personal record lap time of %s!\n", t);
	}

	cent->finishRaceTime = time;
}

void CG_StartRace( int time ) {
	int			i;
	centity_t	*player;

	for (i = 0; i < MAX_CLIENTS; i++){
		player = &cg_entities[i];
		if (!player) continue;

		if (!player->startRaceTime){
			player->startRaceTime = time;
			player->finishRaceTime = 0;
			player->startLapTime = time;
			player->currentLap = 1;
			player->bestLapTime = 0;
			player->lastStartLapTime = 0;
		}
	}
}

void CG_DrawRaceCountDown( void ){
	float	f, scale;
	int		x, y, w, h;
	vec4_t	color;

	if (cg.countDownEnd + 1000 < cg.time || cg.countDownPrint[0] == 0)
		return;

	f = cg.countDownEnd < cg.time ? 0.0f : (cg.countDownEnd - cg.time) / 3000.0f;

	color[0] = 1.0f * f;
	color[1] = 1.0f * (1-f);
	color[2] = 0;
	color[3] = 1.0f;

	scale = cg.countDownEnd < cg.time ? 0.8f : ((cg.countDownEnd - cg.time) % 1000) / 1000.0f;
	w = 3*GIANTCHAR_WIDTH * scale;
	h = 3*GIANTCHAR_HEIGHT * scale;
	x = 320 - (strlen(cg.countDownPrint) * w) / 2;
	y = 240 - h/2;
	CG_DrawStringExt( x, y, cg.countDownPrint, color, qfalse, qtrue, w, h, 0 );
}

void CG_RaceCountDown( const char *str, int secondsLeft ){
	cg.centerPrintTime = 0;
	cg.countDownEnd = cg.time + secondsLeft * 1000;
	Q_strncpyz( cg.countDownPrint, str, sizeof(cg.countDownPrint) );
}
