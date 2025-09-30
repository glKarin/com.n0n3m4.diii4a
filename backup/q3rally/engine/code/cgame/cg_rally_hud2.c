/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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

/*
=================
CG_DrawHUD_Times
=================
*/
void CG_DrawHUD_Times(float x, float y){
	centity_t	*cent;
	int			lapTime, lastTime, totalTime, teamTime;
	//int		bestTime;
	char		*time;
	int			i, count = 0;

	cent = &cg_entities[cg.snap->ps.clientNum];

	if (cent->finishRaceTime){
		lapTime = cent->finishRaceTime - cent->startLapTime;
		totalTime = cent->finishRaceTime - cent->startRaceTime;
	}
	else if (cent->startRaceTime){
		lapTime = cg.time - cent->startLapTime;
		totalTime = cg.time - cent->startRaceTime;
	}
	else {
		lapTime = 0;
		totalTime = 0;
	}

	if (cent->lastStartLapTime)
		lastTime = cent->startLapTime - cent->lastStartLapTime;
	else
		lastTime = 0;

	//bestTime = cent->bestLapTime;

	// draw heading
	CG_FillRect(x, y, 170, 18, bgColor);
	CG_DrawSmallDigitalStringColor(x + 12, y, "TIMES", colorWhite);

	y += 20;

	// draw lap time
	CG_DrawSmallDigitalStringColor(x + 12, y, "LAP:", colorWhite);
	time = getStringForTime( lapTime );
	CG_DrawSmallDigitalStringColor(x + 102, y, time, colorWhite);

	y += 20;

	// draw last lap time
	CG_DrawSmallDigitalStringColor(x + 12, y, "LAST:", colorWhite);
	if ( lastTime ){
		time = getStringForTime( lastTime );
		CG_DrawSmallDigitalStringColor(x + 102, y, time, colorWhite);
	}
	else {
		CG_DrawSmallDigitalStringColor(x + 102, y, "N/A", colorWhite);
	}

	y += 20;

	// draw total time
	CG_DrawSmallDigitalStringColor(x + 12, y, "TOTAL:", colorWhite);
	time = getStringForTime( totalTime );
	CG_DrawSmallDigitalStringColor(x + 102, y, time, colorWhite);

	y += 20;

	// draw team time
	if (cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM){
		// get new scores for accurate team dm times
		if (cg.scoresRequestTime + 2000 < cg.time){
			cg.scoresRequestTime = cg.time;
			trap_SendClientCommand( "score" );
		}

		// add up times for team members
		teamTime = 0;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( !cgs.clientinfo[i].infoValid )	continue;

			if ( cgs.clientinfo[i].team == cgs.clientinfo[cg.snap->ps.clientNum].team ) {
				count++;

				cent = &cg_entities[i];

				if (cent->finishRaceTime)
					teamTime += cent->finishRaceTime - cent->startLapTime;
				else if (cent->startRaceTime)
					teamTime += cg.time - cent->startRaceTime;
			}
		}

		// subtract team frag times
		if (cgs.gametype == GT_TEAM_RACING_DM){
			switch(cgs.clientinfo[cg.snap->ps.clientNum].team){
			case TEAM_RED:
				if (cgs.scores1 > 0)
					teamTime -= cgs.scores1 * TIME_BONUS_PER_FRAG;
				break;
			case TEAM_BLUE:
				if (cgs.scores2 > 0)
					teamTime -= cgs.scores2 * TIME_BONUS_PER_FRAG;
				break;
			case TEAM_GREEN:
				if (cgs.scores3 > 0)
					teamTime -= cgs.scores3 * TIME_BONUS_PER_FRAG;
				break;
			case TEAM_YELLOW:
				if (cgs.scores4 > 0)
					teamTime -= cgs.scores4 * TIME_BONUS_PER_FRAG;
				break;
			default:
				break;
			}
		}

		// average team time
		teamTime /= count;

		CG_DrawSmallDigitalStringColor(x + 12, y, "TEAM:", colorWhite);
		time = getStringForTime(teamTime);
		CG_DrawSmallDigitalStringColor(x + 102, y, time, colorWhite);
	}
}


/*
====================
CG_DrawHUD_Positions
====================
*/
void CG_DrawHUD_Positions(float x, float y){
	int		i, num_teams, team_rank;

	num_teams = 0;
	for ( i = 0 ; i < 4 ; i++ ) {
		if (!TeamCount(-1, TEAM_RED + i)) continue;

		num_teams++;
	}

	team_rank = 0;
	for ( i = 0 ; i < 4 ; i++ ) {
		if (GetTeamAtRank(i + 1) == cgs.clientinfo[cg.snap->ps.clientNum].team)
			team_rank = i+1;
	}

	// draw heading
	CG_FillRect(x, y, 170, 18, bgColor);
	CG_DrawSmallDigitalStringColor(x + 12, y, "POSITION", colorWhite);

	y += 20;

	// draw your position
	CG_DrawSmallDigitalStringColor(x + 12, y, "YOU:", colorWhite);
	CG_DrawSmallDigitalStringColor(x + 102, y, va("%i/%i", cg_entities[cg.snap->ps.clientNum].currentPosition, cgs.numRacers), colorWhite);

	y += 20;

	// draw team position
	if (cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM){
		CG_DrawSmallDigitalStringColor(x + 12, y, "TEAM:", colorWhite);
		CG_DrawSmallDigitalStringColor(x + 102, y, va("%i/%i", team_rank, num_teams), colorWhite);
	}
}

/*
===============
CG_DrawHUD_Laps
===============
*/
void CG_DrawHUD_Laps(float x, float y){

	// draw heading
	CG_FillRect(x, y, 170, 18, bgColor);
	CG_DrawSmallDigitalStringColor(x + 12, y, "LAP:", colorWhite);
	CG_DrawSmallDigitalStringColor(x + 102, y, va("%i/%i", cg_entities[cg.snap->ps.clientNum].currentLap, cgs.laplimit), colorWhite);
}

/*
=======================
CG_DrawHUD_OpponentList
=======================
*/
void CG_DrawHUD_OpponentList(float x, float y){
	centity_t	*cent, *other;
	char		player[64];
	int			i, j, num;
	float		width, height;
	int			startPos, endPos;
	char		s[64];
	vec4_t		color;

	//ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	startPos = cent->currentPosition - 4 < 1 ? 1 : cent->currentPosition - 4;
	endPos = startPos + 8 > cgs.numRacers ? cgs.numRacers : startPos + 8;
	startPos = endPos - 8 < 1 ? 1 : endPos - 8;

	width = 198;
	height = 18;

	// draw your position
	CG_FillRect(x, y, width, height, bgColor);
	CG_DrawSmallDigitalStringColor(x, y, "POS:", colorWhite);
	CG_DrawSmallDigitalStringColor(x + 82, y, va("%i/%i", cent->currentPosition, cgs.numRacers), colorWhite);

	y += 20;

	for (i = startPos; i <= endPos; i++){
		num = -1;
		for (j = 0; j < cgs.maxclients; j++){
			other = &cg_entities[j];
			if ( !other ) continue;
//			if ( isRaceObserver(other->currentState.clientNum) ) continue;
			if ( cgs.clientinfo[other->currentState.clientNum].team == TEAM_SPECTATOR ) continue;

			if ( cgs.clientinfo[other->currentState.clientNum].position == i ) {
				num = other->currentState.clientNum;
				break;
			}
		}

		if (num < 0 || num >= cgs.maxclients)
		{
#if 0
			if( (cg.time / 1000)%2 == 0 && (cg.time / 100)%2 == 0 ) {
				Com_Printf( "DEBUG: Could not find player for position %i. Num %i.\n", i, num );
			}
#endif
			continue;
		}

		if (cgs.clientinfo[num].team == TEAM_RED){
			Vector4Copy(colorRed, color);
			color[3] = 0.5f;
		}
		else if (cgs.clientinfo[num].team == TEAM_BLUE){
			Vector4Copy(colorBlue, color);
			color[3] = 0.5f;
		}
		else if (cgs.clientinfo[num].team == TEAM_GREEN){
			Vector4Copy(colorGreen, color);
			color[3] = 0.5f;
		}
		else if (cgs.clientinfo[num].team == TEAM_YELLOW){
			Vector4Copy(colorYellow, color);
			color[3] = 0.5f;
		}
		else {
			Vector4Copy(bgColor, color);
		}

		CG_FillRect(x, y, width, height, color);

		Q_strncpyz(player, cgs.clientinfo[num].name, 16 );
		Com_sprintf(s, sizeof(s), " %i: %s", cgs.clientinfo[num].position, player);
		CG_DrawSmallDigitalStringColor( x, y, s, colorWhite);

		y += 20;
	}
}

/*
=================
CG_DrawHUD_Scores
=================
*/
void CG_DrawHUD_Scores(float x, float y){
	int		score, w;
	char	*s;

	// frags
	// FIXME: cgs.clientinfo[cg.snap->ps.clientNum].score doesnt seem to update?
	//        always zero even when player has frags

	s = va("%i", cgs.clientinfo[cg.snap->ps.clientNum].score);
	w = CG_DrawStrlen( s ) * GIANTCHAR_WIDTH;
	CG_FillRect(x, y, 96, 72, bgColor);
	CG_DrawSmallDigitalStringColor( x + 20, y + 6, "FRAGS:", colorWhite);
	CG_DrawGiantDigitalStringColor( x + 48 - w/2, y + 26, s, colorWhite);

	y += 72;

	// team frags
	// UPDATE: fix this so it displays the proper score
	if (cgs.gametype >= GT_TEAM){
		switch( cg.snap->ps.persistant[PERS_TEAM] ){
		default:
		case TEAM_RED:
			score = cgs.scores1;
			break;
		case TEAM_BLUE:
			score = cgs.scores2;
			break;
		case TEAM_GREEN:
			score = cgs.scores3;
			break;
		case TEAM_YELLOW:
			score = cgs.scores4;
			break;
		}

		s = va( "%i", score );
		w = CG_DrawStrlen( s ) * GIANTCHAR_WIDTH;
		CG_FillRect(x, y, 96, 78, bgColor);
		CG_DrawSmallDigitalStringColor( x + 24, y + 6, "TEAM:", colorWhite);
		CG_DrawGiantDigitalStringColor( x + 48 - w/2, y + 26, s, colorWhite);

		y += 78;
	}

	// draw scores from cg_draw
	CG_DrawScores(x + 96, y);
}

/*
====================
CG_DrawHUD_DerbyList
====================
*/
void CG_DrawHUD_DerbyList(float x, float y){
	int			i;
	vec4_t		color;
	centity_t	*cent;
	char		*time;
	float		playTime;

	// draw heading
    x = 636 - 120;
	CG_FillRect(x, y, 120, 18, bgColor);

	// name
	CG_DrawTinyDigitalStringColor( x + 16, y, "P:", colorWhite);

	// time
//	CG_DrawTinyStringColor( x + 70, y, "TIME:", colorWhite);

	// dmg dealt
	CG_DrawTinyDigitalStringColor( x + 70, y, "DD:", colorWhite);

	// dmg taken
	CG_DrawTinyDigitalStringColor( x + 100, y, "DT:", colorWhite);

	y += 20;

	// draw top 8 players
	for (i = 0; i < 8; i++){
		if (cg.scores[i].scoreFlags < 0) continue; // score is not valid so skip it

		cent = &cg_entities[cg.scores[i].client];
		if (!cent) continue;

		CG_FillRect(x, y, 120, 18, bgColor);

		Vector4Copy(colorWhite, color);
		if (cg.scores[i].client == cg.snap->ps.clientNum){
			if (cg.snap->ps.stats[STAT_HEALTH] <= 0 || cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR)
				Vector4Copy(colorMdGrey, color);
		}
		else if (cent->currentState.eFlags & EF_DEAD || cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR){
			Vector4Copy(colorMdGrey, color);
		}

		playTime = 0;
		if (cent->finishRaceTime){
			playTime = cent->finishRaceTime - cent->startLapTime;
		}
		else if (cent->startRaceTime){
			playTime = cg.time - cent->startLapTime;
		}
		time = getStringForTime(playTime);

		// num
		CG_DrawTinyDigitalStringColor( x + 2, y, va("%i", (i+1)), color);

		// name
		CG_DrawTinyDigitalStringColor( x + 16, y, cgs.clientinfo[cg.scores[i].client].name, color);

		// time
//		CG_DrawTinyStringColor( x + 70, y, time, color);

		// dmg dealt
		CG_DrawTinyDigitalStringColor( x + 75, y, va("%i", cg.scores[i].damageDealt), color);

		// dmg taken
		CG_DrawTinyDigitalStringColor( x + 105, y, va("%i", cg.scores[i].damageTaken), color);

		y += 20;
	}
}


/*
================================
CG_DrawHUD - Draws the extra HUD
================================
*/
qboolean CG_DrawHUD( void ) {
	// don't draw anything if the menu or console is up
	if ( cg_paused.integer ) {
		return qfalse;
	}

	if ( !cg.showHUD ) {
		return qfalse;
	}

	// get new scores for accurate team dm times
	if ( cg.scoresRequestTime + 2000 < cg.time ){
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	switch(cgs.gametype){
	default:
	case GT_RACING:
	case GT_TEAM_RACING:
		CG_DrawHUD_Times(0, 112);
		CG_DrawHUD_Positions(0, 228);
		CG_DrawHUD_Laps(0, 304);
		CG_DrawHUD_OpponentList(440, 130);

		break;

	case GT_RACING_DM:
	case GT_TEAM_RACING_DM:
		CG_DrawHUD_Times(0, 112);
		CG_DrawHUD_Positions(0, 228);
		CG_DrawHUD_Laps(0, 304);
		CG_DrawHUD_OpponentList(440, 130);
		CG_DrawHUD_Scores(264, 130);

		break;

	case GT_DEATHMATCH:
	case GT_TEAM:
	case GT_CTF:
    case GT_DOMINATION:
		CG_DrawHUD_Scores(264, 130);

		break;

	case GT_DERBY:
	case GT_LCS:
//		CG_DrawHUD_DerbyList(44, 130);

		break;
	}

	return qtrue;
}
