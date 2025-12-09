/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define SCOREBOARD_WIDTH    ( 31 * BIGCHAR_WIDTH )

/*
=================
WM_DrawClientScore
=================
*/
static int INFO_PLAYER_WIDTH    = 300;
static int INFO_SCORE_WIDTH     = 50;
static int INFO_LATENCY_WIDTH   = 80;

/*
=================
WM_DrawObjectives
=================
*/
int WM_DrawObjectives( int x, int y, int width, float fade ) {
	const char *s, *buf, *str;
	char teamstr[32];
	int i, num, strwidth, status;

	y += 32;

	// determine character's team
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		strcpy( teamstr, "axis_desc" );
	} else {
		strcpy( teamstr, "allied_desc" );
	}

	s = CG_ConfigString( CS_MULTI_INFO );
	buf = Info_ValueForKey( s, "numobjectives" );

	if ( buf && atoi( buf ) ) {
		num = atoi( buf );

		for ( i = 0; i < num; i++ ) {
			s = CG_ConfigString( CS_MULTI_OBJECTIVE1 + i );
			buf = Info_ValueForKey( s, teamstr );

			// draw text
			str = va( "%s", buf );
			strwidth = CG_DrawStrlen( str ) * SMALLCHAR_WIDTH;
			CG_DrawSmallString( x + width / 2 - strwidth / 2 - 12, y, str, fade );

			// draw status flags
			status = atoi( Info_ValueForKey( s, "status" ) );

			if ( status == 0 ) {
				CG_DrawPic( x + width / 2 - strwidth / 2 - 16 - 24, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/ger_flag.tga" ) );
				CG_DrawPic( x + width / 2 + strwidth / 2 - 12 + 4, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/ger_flag.tga" ) );
			} else if ( status == 1 )   {
				CG_DrawPic( x + width / 2 - strwidth / 2 - 16 - 24, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/usa_flag.tga" ) );
				CG_DrawPic( x + width / 2 + strwidth / 2 - 12 + 4, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/usa_flag.tga" ) );
			}

			y += 16;
		}
	}

	return y;
}

/*
=================
WM_ScoreboardOverlay
=================
*/
int WM_ScoreboardOverlay( int x, int y, float fade ) {
	vec4_t hcolor;
	int width;
	char        *s; // JPW NERVE
	int msec, mins, seconds, tens; // JPW NERVE

	width = INFO_PLAYER_WIDTH + INFO_LATENCY_WIDTH + INFO_SCORE_WIDTH + 25;

	VectorSet( hcolor, 0, 0, 0 );
	hcolor[3] = 0.7 * fade;

	// draw background
	CG_FillRect( x - 12, y, width, 400, hcolor );

	// draw title frame
	VectorSet( hcolor, 0.0039, 0.0039, 0.2461 );
	hcolor[3] = 1 * fade;
	CG_FillRect( x - 12, y, width, 30, hcolor );
	CG_DrawRect( x - 12, y, width, 400, 2, hcolor );

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		const char *s, *buf;

		s = CG_ConfigString( CS_MULTI_INFO );
		buf = Info_ValueForKey( s, "winner" );

		if ( atoi( buf ) ) {
			CG_DrawSmallString( x - 12 + 5, y, "ALLIES WIN!", fade );
		} else {
			CG_DrawSmallString( x - 12 + 5, y, "AXIS WIN!", fade );
		}
	}
// JPW NERVE -- mission time & reinforce time
	else {
		msec = ( cgs.timelimit * 60.f * 1000.f ) - ( cg.time - cgs.levelStartTime );

		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;

		s = va( "Mission time:   %2.0f:%i%i", (float)mins, tens, seconds ); // float cast to line up with reinforce time
		CG_DrawSmallString( x - 7,y,s,fade );

		if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) {
			msec = cg_redlimbotime.integer - ( cg.time % cg_redlimbotime.integer );
		} else if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE )     {
			msec = cg_bluelimbotime.integer - ( cg.time % cg_bluelimbotime.integer );
		} else { // no team (spectator mode)
			msec = 0;
		}

		if ( msec ) {
			seconds = msec / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			tens = seconds / 10;
			seconds -= tens * 10;

			s = va( "Reinforce time: %2.0f:%i%i", (float)mins, tens, seconds );
			CG_DrawSmallString( x - 7,y + 16,s,fade );
		}
	}
// jpw
//	CG_DrawSmallString( x - 12 + 5, y, "Wolfenstein Multiplayer", fade ); // old one

	y = WM_DrawObjectives( x, y, width, fade );
	y += 5;

	// draw field names
	CG_DrawSmallString( x, y, "Players", fade );
	x += INFO_PLAYER_WIDTH;

	CG_DrawSmallString( x, y, "Score", fade );
	x += INFO_SCORE_WIDTH;

	CG_DrawSmallString( x, y, "Latency", fade );

	y += 20;

	return y;
}
// -NERVE - SMF

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard( void ) {
	int x = 0, y = 0, w;
	float fade;
	float   *fadeColor;
	char    *s;

	// don't draw anything if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// still need to see 'mission failed' message in SP
	if ( cg.predictedPlayerState.pm_type == PM_DEAD ) {
		return qfalse;
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va( "Killed by %s", cg.killerName );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
	}


	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 1 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float	x;
	vec4_t	color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawTourneyScoreboard( void ) {
	const char      *s;
	vec4_t color;
	int min, tens, ones;
	clientInfo_t    *ci;
	int y;
	int i;

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	// draw the dialog background
	if ( cg_fixedAspect.integer ) {
		color[0] = color[1] = color[2] = 0;
	 	color[3] = 1;
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		color[0] = color[1] = color[2] = 0;
		color[3] = 1;
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va( "%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			s = va( "%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen( s ), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			y += 64;
		}
}

