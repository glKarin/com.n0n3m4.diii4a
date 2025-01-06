/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define	SCOREBOARD_X		(0-cl_screenoffset.value)

#define SB_HEADER			18
#define SB_TOP				(SB_HEADER+12)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		430

#define SB_INTER_HEIGHT		16 // interleaved height

// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+4)
#define SB_HEAD_X			(SCOREBOARD_X+20)

#define SB_SCORELINE_X		28-cl_screenoffset.value

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH)
#define SB_SCORE_X			(SB_SCORELINE_X + 2 * BIGCHAR_WIDTH)
#define SB_PING_X			(SB_SCORELINE_X + 8 * BIGCHAR_WIDTH)
#define SB_TIME_X			(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH)
#define SB_NAME_X			(SB_SCORELINE_X + 18 * BIGCHAR_WIDTH)

static qboolean localClient; // true if local client has been displayed
/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, score_t *score, float *color, float fade ) {
	char	string[1024];
	clientInfo_t	*ci;
	int iconx, headx;

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		CG_DrawFlagModel( iconx, y, 16, 16, TEAM_RED, qfalse );
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_DrawFlagModel( iconx, y, 16, 16, TEAM_BLUE, qfalse );
	} else {
		// draw the wins / losses
		if ( cgs.gametype == GT_TOURNAMENT ) {
			Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
			CG_DrawSmallStringColor( iconx - 35, y + SMALLCHAR_HEIGHT/2, string, color );
		}
	}

	CG_DrawHead( headx, y, 16, 16, score->client );
	
	// draw the score line
	if ( score->ping == -1 ) {
		Com_sprintf(string, sizeof(string),
			" connecting    %s", ci->name);
	} else if ( ci->team == TEAM_SPECTATOR ) {
		Com_sprintf(string, sizeof(string),
			" SPECT %3i %4i %s", score->ping, score->time, ci->name);
	} else {
		Com_sprintf(string, sizeof(string),
			"%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) ||
			( ( cgs.gametype >= GT_TEAM ) &&
			( cgs.ffa_gt != 1 ) ) ) {
			// Sago: I think this means that it doesn't matter if two players are tied in team game - only team score counts
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		hcolor[0] = cg_crosshairColorRed.value;
		hcolor[1] = cg_crosshairColorGreen.value;
		hcolor[2] = cg_crosshairColorBlue.value;
		hcolor[3] = fade * 0.5;

		CG_FillRect( 90 - cl_screenoffset.value, y, 550 + (cl_screenoffset.value*2), BIGCHAR_HEIGHT+1, hcolor );
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
		CG_DrawBigStringColor( iconx, y, "READY", color );
	} else
        if(cgs.gametype == GT_LMS) {
            CG_DrawBigStringColor( iconx-50, y, va("*%i*",ci->isDead), color );
        } else
        if(ci->isDead) {
            CG_DrawBigStringColor( iconx-60, y, "D", color );
        }
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight ) {
	int		i;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		if ( score->ping == 0 ){ continue; }	// No draw with 0 ping

		CG_DrawClientScore( y + lineHeight * count, score, color, fade );

		count++;
	}

	return count;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	vec4_t         colorblk;

	if (cg.teamoverlay){
		return qfalse; //karin: missing return
	}

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, 0 );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	colorblk[0]=0.0f;
	colorblk[1]=0.0f;
	colorblk[2]=0.0f;
	colorblk[3]=0.75f;
	
	//CG_DrawRoundedRect(0 - cl_screenoffset.value, 0, 640 + (cl_screenoffset.value*2), 480, 0, colorblk);
	CG_DrawPic( -1 - cl_screenoffset.value, 0, 642+(cl_screenoffset.value*2), 480, trap_R_RegisterShaderNoMip( "menu/art/blacktrans" ) );

	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 2;
		CG_DrawBigString( x, y, s, fade );
	}

	// scoreboard
	y = SB_HEADER;

	if(cl_language.integer == 0){
	CG_DrawSmallString( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, "Score", fade );
	CG_DrawSmallString( SB_PING_X + (SB_RATING_WIDTH / 2), y, "Ping", fade );
	CG_DrawSmallString( SB_TIME_X + (SB_RATING_WIDTH / 2), y, "Time", fade );
	CG_DrawSmallString( SB_NAME_X + (SB_RATING_WIDTH / 2), y, "Name", fade );
	}
	if(cl_language.integer == 1){
	CG_DrawSmallString( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, "Счет", fade );
	CG_DrawSmallString( SB_PING_X + (SB_RATING_WIDTH / 2), y, "Пинг", fade );
	CG_DrawSmallString( SB_TIME_X + (SB_RATING_WIDTH / 2), y, "Время", fade );
	CG_DrawSmallString( SB_NAME_X + (SB_RATING_WIDTH / 2), y, "Имя", fade );
	}

	y = SB_TOP;

	maxClients = 24;
	lineHeight = SB_INTER_HEIGHT;
	topBorderSize = 8;
	bottomBorderSize = 16;

	localClient = qfalse;

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade );
				break;
			}
		}
	}

	return qtrue;
}

/*
=================
CG_GetSkill

Gets the current g_spskill value
=================
*/
int CG_GetSkill( void ) {
	char skill[64];
	
	trap_Cvar_VariableStringBuffer( "g_spskill", skill, sizeof(skill) );
	return atoi(skill);
}

/*
=================
CG_GetAccuracy

Gets the current g_spskill value
=================
*/
int CG_GetAccuracy( void ) {
	int i;
	for ( i = 0 ; i < cg.numScores ; i++ ) {
		if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
			return cg.scores[i].accuracy;
		}
	}

	return 0;
}

/*
=================
CG_DrawSinglePlayerIntermission

Draw the single player intermission screen
=================
*/
void CG_DrawSinglePlayerIntermission( void ) {
	vec4_t color;
	int i, y;
	int index;
	playerscore_t scores;
	
	scores = COM_CalculatePlayerScore( cg.snap->ps.persistant, CG_GetAccuracy(), CG_GetSkill() );

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	//carnage score
	y = 64;
	index = 1;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index))
		CG_DrawStringExt( 64, y, "       Carnage :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.scoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}
		CG_DrawStringExt( 64, y, va("       Carnage : %i", scores.carnageScore), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	}

	//accuracy bonus
	y += BIGCHAR_HEIGHT;
	index++;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index))
		CG_DrawStringExt( 64, y, "Accuracy bonus :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.scoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}

		CG_DrawStringExt( 64, y, va("Accuracy bonus : %i (%i%%)", scores.accuracyScore, scores.accuracy), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	}

	//skill bonus
	y += BIGCHAR_HEIGHT;
	index++;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index))
		CG_DrawStringExt( 64, y, "   Skill bonus :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.scoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}

		CG_DrawStringExt( 64, y, va("   Skill bonus : %i (%1.0f%%)", scores.skillScore, scores.skillModifier * 100), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	}

	//secrets score
	y += BIGCHAR_HEIGHT;
	index++;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index))
		CG_DrawStringExt( 64, y, "       Secrets :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.scoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}
		CG_DrawStringExt( 64, y, va("       Secrets : %i (%i/%i)", scores.secretsScore, scores.secretsFound, scores.secretsCount), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );		
	}

	//death penalty
	y += BIGCHAR_HEIGHT;
	index++;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index))
		CG_DrawStringExt( 64, y, "        Deaths :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.scoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}
		CG_DrawStringExt( 64, y, va("        Deaths : %i (%ix)", scores.deathsScore, scores.deaths), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	}

	//total score
	y += BIGCHAR_HEIGHT;
	index++;
	if (cg.time < cg.intermissionTime + (SCOREB_TIME * index) + SCOREB_TIME_LAST)	//wait slightly longer before showing final score
		CG_DrawStringExt( 64, y, "         TOTAL :", color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	else {
		if (cg.scoreSoundsPlayed == index - 1) {
			trap_S_StartLocalSound( cgs.media.finalScoreShow, CHAN_LOCAL_SOUND );
			cg.scoreSoundsPlayed++;
		}

		CG_DrawStringExt( 64, y, va("         TOTAL : %i", scores.totalScore), color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, 0 );
	}
}

/*
=================
CG_DrawSinglePlayerObjectives

Draw the single player objectives overlay
=================
*/
qboolean CG_DrawSinglePlayerObjectives( void ) {
	const char *p;
	const char *s;
	vec4_t color;
	vec4_t color_black;
	int i;
	int objlen;
	char lines[5][60];
	int spaceIndex, prevSpaceIndex = 0;
	int currentLine = 0;
	int lineIndex = 0;
	char c[2];
	qboolean tooLong = qfalse;	
	playerscore_t scores;
	float scoreboardX;
	float scoreboardY;
	int skill;


	if ( !cg.showScores )
		return qfalse;


	cg.objectivesTime = 0;	//stop objectives notification from showing
	
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 0.75;

	color_black[0] = 0;
	color_black[1] = 0;
	color_black[2] = 0;
	color_black[3] = 0.6;

	p = CG_ConfigString( CS_PRIMARYOBJECTIVE );
	s = CG_ConfigString( CS_SECONDARYOBJECTIVE );

	//draw objectives overlay
	scoreboardX = (SCREEN_WIDTH - 512) / 2;
	scoreboardY = (SCREEN_HEIGHT - 256) / 2;
	CG_DrawPic( scoreboardX, scoreboardY, 512, 256, cgs.media.objectivesOverlay );
	
	//draw primary objective
	objlen = strlen(p);

	
	for ( i = 0; i < objlen; i++) {
		
		c[0] = p[i];
		c[1] = '\0';

		if ( c[0] == ' ' ) {
			spaceIndex = i;
		}

		if (lineIndex == 60) {
			if (spaceIndex - prevSpaceIndex <= 0) {
				strcat(lines[currentLine-1], &p[prevSpaceIndex-1]);
				break;
			} else {
				Q_strncpyz(lines[currentLine], &p[prevSpaceIndex], (spaceIndex - prevSpaceIndex) + 1);
				CG_DrawSmallStringColor( 82, 146 + (SMALLCHAR_HEIGHT * currentLine), lines[currentLine], color_black);
				CG_DrawSmallStringColor( 80, 144 + (SMALLCHAR_HEIGHT * currentLine), lines[currentLine], color);
			}
			
			prevSpaceIndex = spaceIndex;
			prevSpaceIndex++;
			i = spaceIndex;
			lineIndex = -1;
			currentLine++;
			if (currentLine == 5)
			{
				tooLong = qtrue;
				break;
			}
			
		}
		lineIndex++;
		
	}

	if ( !tooLong ) {
		CG_DrawSmallStringColor( 82, 146 + (SMALLCHAR_HEIGHT * currentLine), va("%s", &p[prevSpaceIndex]), color_black);
		CG_DrawSmallStringColor( 80, 144 + (SMALLCHAR_HEIGHT * currentLine), va("%s", &p[prevSpaceIndex]), color);
	}


	//draw secondary objective
	spaceIndex = 0;
	prevSpaceIndex = 0;
	lineIndex = 0;
	currentLine = 0;
	tooLong = qfalse;

	objlen = strlen(s);

	for ( i = 0; i < objlen; i++) {
		c[0] = s[i];
		c[1] = '\0';

		if ( c[0] == ' ' ) {
			spaceIndex = i;
		}

		if (lineIndex == 60) {
			if (spaceIndex - prevSpaceIndex <= 0) {
				strcat(lines[currentLine-1], &s[prevSpaceIndex-1]);
				break;
			} else {
				Q_strncpyz(lines[currentLine], &s[prevSpaceIndex], (spaceIndex - prevSpaceIndex) + 1);
				CG_DrawSmallStringColor( 82, 266 + (SMALLCHAR_HEIGHT * currentLine), lines[currentLine], color_black);
				CG_DrawSmallStringColor( 80, 264 + (SMALLCHAR_HEIGHT * currentLine), lines[currentLine], color);
			}
			prevSpaceIndex = spaceIndex;
			prevSpaceIndex++;
			i = spaceIndex;
			lineIndex = -1;
			currentLine++;
			if (currentLine == 4) {
				tooLong = qtrue;
				break;
			}
		}
		lineIndex++;
	}

	if ( !tooLong ) {
		CG_DrawSmallStringColor( 82, 266 + (SMALLCHAR_HEIGHT * currentLine), va("%s", &s[prevSpaceIndex]), color_black);
		CG_DrawSmallStringColor( 80, 264 + (SMALLCHAR_HEIGHT * currentLine), va("%s", &s[prevSpaceIndex]), color);
	}

	//draw deaths counter
	color[0] = 1;
	color[1] = 0;
	color[2] = 0;

	i = strlen(va("%i", cg.snap->ps.persistant[PERS_KILLED]));
	CG_DrawBigStringColor( 208 - (i * BIGCHAR_WIDTH), 343, va("%i", cg.snap->ps.persistant[PERS_KILLED]), color );

	//draw level score
	scores = COM_CalculatePlayerScore( cg.snap->ps.persistant, CG_GetAccuracy(), CG_GetSkill() );
	i = strlen(va("%i", scores.totalScore));
	CG_DrawBigStringColor( 496 - (i * BIGCHAR_WIDTH), 343, va("%i", scores.totalScore), color);	

	//draw skill level
	skill = CG_GetSkill();
	CG_DrawPic(scoreboardX + (512 - 36), scoreboardY + (256 - 30), 24, 24, cgs.media.botSkillShaders[skill - 1]);

	return qtrue;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboardObj( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		CG_DrawSinglePlayerIntermission();
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}
	else 
	{
		return CG_DrawSinglePlayerObjectives();	//draw objectives screen instead of scores in SP.
	}
}

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float		x;
	vec4_t		color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawTourneyScoreboard( void ) {
	const char		*s;
	vec4_t			color;
	int				min, tens, ones;
	clientInfo_t	*ci;
	int				y;
	int				i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

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
	s = va("%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
		s = va("%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );

		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
		s = va("%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
	} else {
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

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
			s = va("%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, 0 );
			y += 64;
		}
	}


}
