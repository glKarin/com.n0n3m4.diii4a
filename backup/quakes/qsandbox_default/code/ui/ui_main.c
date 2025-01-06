// Copyright (C) 1999-2000 Id Software, Inc.
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
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
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
		return qfalse;  // bk010117 - change this to qfalse for mods!
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

vmCvar_t	cl_propsmallsizescale;
vmCvar_t 	cl_propheight;
vmCvar_t 	cl_propspacewidth;
vmCvar_t 	cl_propgapwidth;
vmCvar_t 	cl_smallcharwidth;
vmCvar_t 	cl_smallcharheight;
vmCvar_t 	cl_bigcharwidth;
vmCvar_t 	cl_bigcharheight;
vmCvar_t 	cl_giantcharwidth;
vmCvar_t 	cl_giantcharheight;

//QSandbox Sandbox
vmCvar_t	sb_private;
vmCvar_t	sb_texture;
vmCvar_t	sb_texturename;
vmCvar_t	sb_grid;
vmCvar_t	sb_modelnum;
vmCvar_t	sb_classnum;
vmCvar_t	sb_texturenum;
vmCvar_t	sb_tab;
vmCvar_t	spawn_preset;
vmCvar_t	tool_spawnpreset;
vmCvar_t	tool_modifypreset;
vmCvar_t	tool_modifypreset2;
vmCvar_t	tool_modifypreset3;
vmCvar_t	tool_modifypreset4;

vmCvar_t	sb_ctab_1;
vmCvar_t	sb_ctab_2;
vmCvar_t	sb_ctab_3;
vmCvar_t	sb_ctab_4;
vmCvar_t	sb_ctab_5;
vmCvar_t	sb_ctab_6;
vmCvar_t	sb_ctab_7;
vmCvar_t	sb_ctab_8;
vmCvar_t	sb_ctab_9;
vmCvar_t	sb_ctab_10;

vmCvar_t	toolgun_toolset1;
vmCvar_t	toolgun_toolset2;
vmCvar_t	toolgun_toolset3;
vmCvar_t	toolgun_toolset4;
vmCvar_t	toolgun_toolset5;
vmCvar_t	toolgun_toolset6;
vmCvar_t	toolgun_toolset7;
vmCvar_t	toolgun_toolset8;
vmCvar_t	toolgun_toolset9;
vmCvar_t	toolgun_toolset10;
vmCvar_t	toolgun_toolset11;
vmCvar_t	toolgun_toolset12;
vmCvar_t	toolgun_toolset13;
vmCvar_t	toolgun_toolset14;
vmCvar_t	toolgun_toolset15;
vmCvar_t	toolgun_toolset16;
vmCvar_t	toolgun_toolset17;
vmCvar_t	toolgun_toolset18;

vmCvar_t	toolgun_disabledarg1;
vmCvar_t	toolgun_disabledarg2;
vmCvar_t	toolgun_disabledarg3;
vmCvar_t	toolgun_disabledarg4;

vmCvar_t	cl_sprun;

vmCvar_t	sbt_color0_0;
vmCvar_t	sbt_color0_1;
vmCvar_t	sbt_color0_2;
vmCvar_t	sbt_color0_3;
vmCvar_t	sbt_color1_0;
vmCvar_t	sbt_color1_1;
vmCvar_t	sbt_color1_2;
vmCvar_t	sbt_color1_3;
vmCvar_t	sbt_color2_0;
vmCvar_t	sbt_color2_1;
vmCvar_t	sbt_color2_2;
vmCvar_t	sbt_color2_3;
vmCvar_t	sbt_color3_0;
vmCvar_t	sbt_color3_1;
vmCvar_t	sbt_color3_2;
vmCvar_t	sbt_color3_3;
vmCvar_t	sbt_wallpaper;

vmCvar_t	ui_3dmap;

vmCvar_t	ui_singlemode;
vmCvar_t	legsskin;
vmCvar_t	team_legsskin;
vmCvar_t	cl_selectedmod;
vmCvar_t	cl_language;
vmCvar_t	cl_screenoffset;
vmCvar_t	ui_loaded;
vmCvar_t	ui_backcolors;
vmCvar_t	sensitivitymenu;

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
vmCvar_t	ui_server17;
vmCvar_t	ui_server18;
vmCvar_t	ui_server19;
vmCvar_t	ui_server20;
vmCvar_t	ui_server21;
vmCvar_t	ui_server22;
vmCvar_t	ui_server23;
vmCvar_t	ui_server24;
vmCvar_t	ui_server25;
vmCvar_t	ui_server26;
vmCvar_t	ui_server27;
vmCvar_t	ui_server28;
vmCvar_t	ui_server29;
vmCvar_t	ui_server30;
vmCvar_t	ui_server31;
vmCvar_t	ui_server32;

// UIE conventional cvars
vmCvar_t	uie_animsfx;
vmCvar_t	uie_mapicons;
vmCvar_t	uie_autoclosebotmenu;
vmCvar_t	uie_ingame_dynamicmenu;
vmCvar_t	uie_map_multisel;
vmCvar_t	uie_map_list;
vmCvar_t	uie_bot_multisel;
vmCvar_t	uie_bot_list;


static cvarTable_t		cvarTable[] = {

	{ &cl_propsmallsizescale, "cl_propsmallsizescale", "0.60", CVAR_ARCHIVE },
	{ &cl_propheight, "cl_propheight", "21", CVAR_ARCHIVE  },
	{ &cl_propspacewidth, "cl_propspacewidth", "8", CVAR_ARCHIVE  },
	{ &cl_propgapwidth, "cl_propgapwidth", "3", CVAR_ARCHIVE  },
	{ &cl_smallcharwidth, "cl_smallcharwidth", "8", CVAR_ARCHIVE  },
	{ &cl_smallcharheight, "cl_smallcharheight", "12", CVAR_ARCHIVE  },
	{ &cl_bigcharwidth, "cl_bigcharwidth", "12", CVAR_ARCHIVE  },
	{ &cl_bigcharheight, "cl_bigcharheight", "12", CVAR_ARCHIVE  },
	{ &cl_giantcharwidth, "cl_giantcharwidth", "20", CVAR_ARCHIVE  },
	{ &cl_giantcharheight, "cl_giantcharheight", "32", CVAR_ARCHIVE  },

//QSandbox Sandbox
	{ &sb_private, "sb_private", "0", 0 },
	{ &sb_grid, "sb_grid", "25", 0 },
	{ &sb_texture, "sb_texture", "0", 0 },
	{ &sb_texturename, "sb_texturename", "0", 0 },
	{ &sb_modelnum, "sb_modelnum", "0", CVAR_ARCHIVE },
	{ &sb_classnum, "sb_classnum", "0", CVAR_ARCHIVE },
	{ &sb_texturenum, "sb_texturenum", "0", CVAR_ARCHIVE },
	{ &sb_tab, "sb_tab", "1", 0 },
	{ &spawn_preset, "spawn_preset", "set toolcmd_spawn sl prop %s %s %i 25 %s 0 %s 1 0 \"none\" -1 0 0 0 0 1 1 1 0 0 0.5", 0 },
	{ &tool_spawnpreset, "tool_spawnpreset", "set toolcmd_spawn sl prop %s %s %i 25 %s 0 %s 1 0 \"none\" -1 0 0 0 0 1 1 1 0 0 0.5", 0 },
	{ &tool_modifypreset, "tool_modifypreset", "set toolcmd_modify tm %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s 0", 0 },
	{ &tool_modifypreset2, "tool_modifypreset2", "set toolcmd_modify2 tm %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s 1", 0 },
	{ &tool_modifypreset3, "tool_modifypreset3", "set toolcmd_modify3 tm %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s 2", 0 },
	{ &tool_modifypreset4, "tool_modifypreset4", "set toolcmd_modify4 tm %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s 3", 0 },

	{ &sb_ctab_1, "sb_ctab_1", "", CVAR_ARCHIVE },
	{ &sb_ctab_2, "sb_ctab_2", "", CVAR_ARCHIVE },
	{ &sb_ctab_3, "sb_ctab_3", "", CVAR_ARCHIVE },
	{ &sb_ctab_4, "sb_ctab_4", "", CVAR_ARCHIVE },
	{ &sb_ctab_5, "sb_ctab_5", "", CVAR_ARCHIVE },
	{ &sb_ctab_6, "sb_ctab_6", "", CVAR_ARCHIVE },
	{ &sb_ctab_7, "sb_ctab_7", "", CVAR_ARCHIVE },
	{ &sb_ctab_8, "sb_ctab_8", "", CVAR_ARCHIVE },
	{ &sb_ctab_9, "sb_ctab_9", "", CVAR_ARCHIVE },
	{ &sb_ctab_10, "sb_ctab_10", "", CVAR_ARCHIVE },
	
	{ &toolgun_toolset1, "toolgun_toolset1", "--------:", 0 },
	{ &toolgun_toolset2, "toolgun_toolset2", "--------:", 0 },
	{ &toolgun_toolset3, "toolgun_toolset3", "--------:", 0 },
	{ &toolgun_toolset4, "toolgun_toolset4", "--------:", 0 },
	{ &toolgun_toolset5, "toolgun_toolset5", "--------:", 0 },
	{ &toolgun_toolset6, "toolgun_toolset6", "--------:", 0 },
	{ &toolgun_toolset7, "toolgun_toolset7", "--------:", 0 },
	{ &toolgun_toolset8, "toolgun_toolset8", "--------:", 0 },
	{ &toolgun_toolset9, "toolgun_toolset9", "--------:", 0 },
	{ &toolgun_toolset10, "toolgun_toolset10", "--------:", 0 },
	{ &toolgun_toolset11, "toolgun_toolset11", "--------:", 0 },
	{ &toolgun_toolset12, "toolgun_toolset12", "--------:", 0 },
	{ &toolgun_toolset13, "toolgun_toolset13", "--------:", 0 },
	{ &toolgun_toolset14, "toolgun_toolset14", "--------:", 0 },
	{ &toolgun_toolset15, "toolgun_toolset15", "--------:", 0 },
	{ &toolgun_toolset16, "toolgun_toolset16", "--------:", 0 },
	{ &toolgun_toolset17, "toolgun_toolset17", "--------:", 0 },
	{ &toolgun_toolset18, "toolgun_toolset18", "--------:", 0 },
	
	{ &toolgun_disabledarg1, "toolgun_disabledarg1", "0", 0 },
	{ &toolgun_disabledarg2, "toolgun_disabledarg2", "0", 0 },
	{ &toolgun_disabledarg3, "toolgun_disabledarg3", "0", 0 },
	{ &toolgun_disabledarg4, "toolgun_disabledarg4", "0", 0 },

	{ &cl_sprun, "cl_sprun", "0", 0 },
	
	{ &sbt_color0_0,  "sbt_color0_0", "1", 	 CVAR_ARCHIVE },
	{ &sbt_color0_1,  "sbt_color0_1", "1", 	 CVAR_ARCHIVE },
	{ &sbt_color0_2,  "sbt_color0_2", "1", 	 CVAR_ARCHIVE },
	{ &sbt_color0_3,  "sbt_color0_3", "0.80", CVAR_ARCHIVE },
	{ &sbt_color1_0,  "sbt_color1_0", "0.50", CVAR_ARCHIVE },
	{ &sbt_color1_1,  "sbt_color1_1", "0.50", CVAR_ARCHIVE },
	{ &sbt_color1_2,  "sbt_color1_2", "0.50", CVAR_ARCHIVE },
	{ &sbt_color1_3,  "sbt_color1_3", "0.90", CVAR_ARCHIVE },
	{ &sbt_color2_0,  "sbt_color2_0", "0.30", CVAR_ARCHIVE },
	{ &sbt_color2_1,  "sbt_color2_1", "0.30", CVAR_ARCHIVE },
	{ &sbt_color2_2,  "sbt_color2_2", "0.95", CVAR_ARCHIVE },
	{ &sbt_color2_3,  "sbt_color2_3", "0.90", CVAR_ARCHIVE },
	{ &sbt_color3_0,  "sbt_color3_0", "0", 	 CVAR_ARCHIVE },
	{ &sbt_color3_1,  "sbt_color3_1", "0", 	 CVAR_ARCHIVE },
	{ &sbt_color3_2,  "sbt_color3_2", "0", 	 CVAR_ARCHIVE },
	{ &sbt_color3_3,  "sbt_color3_3", "1", 	 CVAR_ARCHIVE },
	{ &sbt_wallpaper, "sbt_wallpaper", "trans", 	 CVAR_ARCHIVE },

	{ &ui_3dmap, "ui_3dmap", "", CVAR_ARCHIVE },

	{ &ui_singlemode, "ui_singlemode", "0", CVAR_ARCHIVE },
	{ &ui_loaded, "ui_loaded", "0", 0 },
	{ &legsskin, "legsskin", "beret/default", CVAR_ARCHIVE },
	{ &team_legsskin, "team_legsskin", "beret/default", CVAR_ARCHIVE },
	{ &cl_selectedmod, "cl_selectedmod", "default", CVAR_ARCHIVE },
	{ &cl_language, "cl_language", "0", CVAR_ARCHIVE },
	{ &cl_screenoffset, "cl_screenoffset", "107", CVAR_ARCHIVE },
	{ &ui_backcolors, "ui_backcolors", "1", CVAR_ARCHIVE },
	{ &sensitivitymenu, "sensitivitymenu", "1", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spSkill, "g_spSkill", "1", 0 },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },

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
	{ &ui_server17, "server17", "", CVAR_ARCHIVE },
	{ &ui_server18, "server18", "", CVAR_ARCHIVE },
	{ &ui_server19, "server19", "", CVAR_ARCHIVE },
	{ &ui_server20, "server20", "", CVAR_ARCHIVE },
	{ &ui_server21, "server21", "", CVAR_ARCHIVE },
	{ &ui_server22, "server22", "", CVAR_ARCHIVE },
	{ &ui_server23, "server23", "", CVAR_ARCHIVE },
	{ &ui_server24, "server24", "", CVAR_ARCHIVE },
	{ &ui_server25, "server25", "", CVAR_ARCHIVE },
	{ &ui_server26, "server26", "", CVAR_ARCHIVE },
	{ &ui_server27, "server27", "", CVAR_ARCHIVE },
	{ &ui_server28, "server28", "", CVAR_ARCHIVE },
	{ &ui_server29, "server29", "", CVAR_ARCHIVE },
	{ &ui_server30, "server30", "", CVAR_ARCHIVE },
	{ &ui_server31, "server31", "", CVAR_ARCHIVE },
	{ &ui_server32, "server32", "", CVAR_ARCHIVE },

	{ &uie_map_multisel, "uie_map_multisel", "0", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_map_list, "uie_map_list", "0", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_bot_multisel, "uie_bot_multisel", "0", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_bot_list, "uie_bot_list", "0", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_ingame_dynamicmenu, "uie_ingame_dynamicmenu", "1", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_animsfx, "uie_s_animsfx", "1", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_mapicons, "uie_mapicons", "0", CVAR_ROM|CVAR_ARCHIVE },
	{ &uie_autoclosebotmenu, "uie_autoclosebotmenu", "0", CVAR_ROM|CVAR_ARCHIVE },
};

static int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);
int 	mod_ammolimit = 200;
int 	mod_gravity = 800;


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

	// we also set default values for the disable_* Cvars here
	// since first usage defines their default value
	UIE_StartServer_RegisterDisableCvars(qtrue);
	trap_Cvar_Set( "cl_sprun", "0" );
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec uiautoexec.cfg\n");
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



/*
=================
UI_IsValidCvar
=================
*/
qboolean UI_IsValidCvar(const char* cvar)
{
	int i;

	for (i = 0; i < cvarTableSize; i++)
	{
		if (!Q_stricmp(cvar, cvarTable[i].cvarName))
			return qtrue;
	}
	return qfalse;
}

/*
==================
 * UI_SetDefaultCvar
 * If the cvar is blank it will be set to value
 * This is only good for cvars that cannot naturally be blank
==================
 */
void UI_SetDefaultCvar(const char* cvar, const char* value) {
    if(strlen(UI_Cvar_VariableString(cvar)) == 0)
        trap_Cvar_Set(cvar,value);
}



