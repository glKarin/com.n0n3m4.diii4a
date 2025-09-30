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
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/


#include "ui_local.h"


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		UI_Init();
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent( arg0, arg1 );
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
		return UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;
	case UI_HASUNIQUECDKEY:				// mod authors need to observe this
// STONELANCE
//		return qtrue;  // change this to qfalse for mods!
		return qfalse;
// END
	}

	return -1;
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

// STONELANCE
/*
vmCvar_t	ui_ffa_fraglimit;
vmCvar_t	ui_ffa_timelimit;

vmCvar_t	ui_tourney_fraglimit;
vmCvar_t	ui_tourney_timelimit;
*/

vmCvar_t	ui_racing_laplimit;
vmCvar_t	ui_racing_timelimit;

vmCvar_t	ui_team_racing_laplimit;
vmCvar_t	ui_team_racing_timelimit;
vmCvar_t	ui_team_racing_friendly;

vmCvar_t	ui_derby_timelimit;

vmCvar_t	ui_lcs_timelimit;

vmCvar_t	ui_dm_fraglimit;
vmCvar_t	ui_dm_timelimit;

vmCvar_t	ui_racing_tracklength;
vmCvar_t	ui_racing_trackreversed;
// END

vmCvar_t	ui_team_fraglimit;
vmCvar_t	ui_team_timelimit;
vmCvar_t	ui_team_friendly;

vmCvar_t	ui_ctf_capturelimit;
vmCvar_t	ui_ctf_timelimit;
vmCvar_t	ui_ctf_friendly;

// Q3Rally Code Start
vmCvar_t	ui_dom_capturelimit;
vmCvar_t	ui_dom_timelimit;
vmCvar_t	ui_dom_friendly;
vmCvar_t	ui_sigilLocator;
// Q3Rally Code END

vmCvar_t	ui_arenasFile;
vmCvar_t	ui_botsFile;
vmCvar_t	ui_spScores1;
vmCvar_t	ui_spScores2;
vmCvar_t	ui_spScores3;
vmCvar_t	ui_spScores4;
vmCvar_t	ui_spScores5;
vmCvar_t	ui_spAwards;
vmCvar_t	ui_spVideos;
vmCvar_t	ui_spSkill;

vmCvar_t	ui_spSelection;

vmCvar_t	ui_browserMaster;
vmCvar_t	ui_browserGameType;
vmCvar_t	ui_browserSortKey;
vmCvar_t	ui_browserShowFull;
vmCvar_t	ui_browserShowEmpty;
vmCvar_t	ui_browserOnlyHumans;

vmCvar_t	ui_brassTime;
vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;

vmCvar_t	ui_server1;
vmCvar_t	ui_server2;
vmCvar_t	ui_server3;
vmCvar_t	ui_server4;
vmCvar_t	ui_server5;
vmCvar_t	ui_server6;
vmCvar_t	ui_server7;
vmCvar_t	ui_server8;
vmCvar_t	ui_server9;
vmCvar_t	ui_server10;
vmCvar_t	ui_server11;
vmCvar_t	ui_server12;
vmCvar_t	ui_server13;
vmCvar_t	ui_server14;
vmCvar_t	ui_server15;
vmCvar_t	ui_server16;

// STONELANCE
vmCvar_t	ui_favoritecar1;
vmCvar_t	ui_favoritecar2;
vmCvar_t	ui_favoritecar3;
vmCvar_t	ui_favoritecar4;

vmCvar_t	ui_trackReversed;


vmCvar_t	ui_metricUnits;
vmCvar_t	ui_controlMode;
vmCvar_t	ui_manualShift;
vmCvar_t	ui_minSkidLength;
vmCvar_t	ui_drawRearView;
vmCvar_t	ui_checkpointArrowMode;
vmCvar_t	ui_atmosphericLevel;
vmCvar_t	ui_drawPositionSprites;
vmCvar_t	ui_engineSounds;
vmCvar_t    ui_drawMinimap;
vmCvar_t	ui_tightCamTracking;
vmCvar_t	ui_rearViewRenderLevel;
vmCvar_t	ui_mainViewRenderLevel;

vmCvar_t	ui_model;
vmCvar_t	ui_head;
vmCvar_t	ui_rim;
vmCvar_t	ui_plate;
// END

vmCvar_t	ui_cdkeychecked;
vmCvar_t	ui_ioq3;

static cvarTable_t		cvarTable[] = {

	{ &ui_racing_laplimit, "ui_racing_laplimit", "3", CVAR_ARCHIVE },
	{ &ui_racing_timelimit, "ui_racing_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_team_racing_laplimit, "ui_team_racing_laplimit", "3", CVAR_ARCHIVE },
	{ &ui_team_racing_timelimit, "ui_team_racing_timelimit", "0", CVAR_ARCHIVE },
	{ &ui_team_racing_friendly, "ui_team_racing_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_derby_timelimit, "ui_derby_timelimit", "10", CVAR_ARCHIVE },

	{ &ui_dm_fraglimit, "ui_dm_fraglimit", "15", CVAR_ARCHIVE },
	{ &ui_dm_timelimit, "ui_dm_timelimit", "10", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "15", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "10", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_dom_capturelimit, "ui_dom_capturelimit", "300", CVAR_ARCHIVE },
	{ &ui_dom_timelimit, "ui_dom_timelimit", "15", CVAR_ARCHIVE },
	{ &ui_dom_friendly, "ui_dom_friendly", "1", CVAR_ARCHIVE },
	{ &ui_sigilLocator, "cg_sigilLocator", "1", CVAR_ARCHIVE },

	{ &ui_racing_tracklength, "ui_racing_tracklength", "1", CVAR_ARCHIVE },
	{ &ui_racing_trackreversed, "ui_racing_trackreversed",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },
	{ &ui_browserOnlyHumans, "ui_browserOnlyHumans", "0", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },

// STONELANCE
	{ &ui_favoritecar1, "favoritecar1", "", CVAR_ARCHIVE },
	{ &ui_favoritecar2, "favoritecar2", "", CVAR_ARCHIVE },
	{ &ui_favoritecar3, "favoritecar3", "", CVAR_ARCHIVE },
	{ &ui_favoritecar4, "favoritecar4", "", CVAR_ARCHIVE },

	{ &ui_trackReversed, "g_trackReversed", "0", CVAR_LATCH },

	{ &ui_metricUnits, "cg_metricUnits", "0", CVAR_ARCHIVE },
	{ &ui_controlMode, "cg_controlMode", "0", CVAR_ARCHIVE },
	{ &ui_manualShift, "cg_manualShift", "0", CVAR_ARCHIVE },
	{ &ui_minSkidLength, "cg_minSkidLength", "12", CVAR_ARCHIVE },
	{ &ui_drawRearView, "cg_drawRearView", "0", CVAR_ARCHIVE },
	{ &ui_checkpointArrowMode, "cg_checkpointArrowMode", "2", CVAR_ARCHIVE },
	{ &ui_atmosphericLevel, "cg_atmosphericLevel", "2", CVAR_ARCHIVE },
	{ &ui_drawPositionSprites, "cg_drawPositionSprites", "1", CVAR_ARCHIVE },
	{ &ui_engineSounds, "cg_engineSounds", "1", CVAR_ARCHIVE },
    { &ui_drawMinimap, "cg_drawMMap", "0", CVAR_ARCHIVE },
	{ &ui_tightCamTracking, "cg_tightCamTracking", "1", CVAR_ARCHIVE },
	{ &ui_rearViewRenderLevel, "cg_rearViewRenderLevel", "31", CVAR_ARCHIVE },
	{ &ui_mainViewRenderLevel, "cg_mainViewRenderLevel", "31", CVAR_ARCHIVE },

	{ &ui_model, "model", "alpine/red", CVAR_USERINFO|CVAR_ARCHIVE },
	{ &ui_head, "head", "doom", CVAR_USERINFO|CVAR_ARCHIVE },
	{ &ui_rim, "rim", "svt_cobra", CVAR_USERINFO|CVAR_ARCHIVE },
	{ &ui_plate, "plate", "usa_california", CVAR_USERINFO|CVAR_ARCHIVE },
// END

	{ &ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM },
	{ &ui_ioq3, "ui_ioq3", "1", CVAR_ROM }
};

static int cvarTableSize = ARRAY_LEN( cvarTable );


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}
