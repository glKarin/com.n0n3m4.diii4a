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

You should have received a copy of the GNU General Public Licenseа
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );


int realVidWidth;
int realVidHeight;		// leilei - global video hack


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
        CG_FairCvars();
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[WEAPONS_NUM+1];
itemInfo_t			cg_items[MAX_ITEMS];

vmCvar_t	g_gametype;

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

vmCvar_t	cg_gibjump;
vmCvar_t	cg_gibvelocity;
vmCvar_t	cg_gibmodifier;

vmCvar_t	cg_zoomtime;
vmCvar_t	cg_itemscaletime;
vmCvar_t	cg_weaponselecttime;

//Noire Set
vmCvar_t	toolgun_mod1;		//modifier
vmCvar_t	toolgun_mod2;		//modifier
vmCvar_t	toolgun_mod3;		//modifier
vmCvar_t	toolgun_mod4;		//modifier
vmCvar_t	toolgun_mod5;		//modifier
vmCvar_t	toolgun_mod6;		//modifier
vmCvar_t	toolgun_mod7;		//modifier
vmCvar_t	toolgun_mod8;		//modifier
vmCvar_t	toolgun_mod9;		//modifier
vmCvar_t	toolgun_mod10;		//modifier
vmCvar_t	toolgun_mod11;		//modifier
vmCvar_t	toolgun_mod12;		//modifier
vmCvar_t	toolgun_mod13;		//modifier
vmCvar_t	toolgun_mod14;		//modifier
vmCvar_t	toolgun_mod15;		//modifier
vmCvar_t	toolgun_mod16;		//modifier
vmCvar_t	toolgun_mod17;		//modifier
vmCvar_t	toolgun_mod18;		//modifier
vmCvar_t	toolgun_mod19;		//modifier
vmCvar_t	toolgun_tool;		//tool id
vmCvar_t	toolgun_toolcmd1;	//command
vmCvar_t	toolgun_toolcmd2;	//command
vmCvar_t	toolgun_toolcmd3;	//command
vmCvar_t	toolgun_toolcmd4;	//command
vmCvar_t	toolgun_tooltext;	//info
vmCvar_t	toolgun_tooltip1;	//info
vmCvar_t	toolgun_tooltip2;	//info
vmCvar_t	toolgun_tooltip3;	//info
vmCvar_t	toolgun_tooltip4;	//info
vmCvar_t	toolgun_toolmode1;	//mode
vmCvar_t	toolgun_toolmode2;	//mode
vmCvar_t	toolgun_toolmode3;	//mode
vmCvar_t	toolgun_toolmode4;	//mode
vmCvar_t	toolgun_modelst;	//preview model
vmCvar_t	sb_classnum_view;	//preview class
vmCvar_t	sb_texture_view;	//preview material
vmCvar_t	sb_texturename;		//preview texture
vmCvar_t	cg_hide255;			//invisible model

vmCvar_t	ns_haveerror;		//Noire.Script error

vmCvar_t	cg_postprocess;
vmCvar_t	cg_toolguninfo;
vmCvar_t	cl_language;
vmCvar_t	con_notifytime;
vmCvar_t 	cg_leiChibi;
vmCvar_t    cg_cameraeyes;
vmCvar_t    cg_helightred;
vmCvar_t    cg_helightgreen;
vmCvar_t    cg_helightblue;
vmCvar_t    cg_tolightred;
vmCvar_t    cg_tolightgreen;
vmCvar_t    cg_tolightblue;
vmCvar_t    cg_plightred;
vmCvar_t    cg_plightgreen;
vmCvar_t    cg_plightblue;
vmCvar_t    cl_screenoffset;
vmCvar_t    ui_backcolors;
vmCvar_t	legsskin;
vmCvar_t	team_legsskin;
vmCvar_t	cg_oldscoreboard;
vmCvar_t	cg_itemstyle;
vmCvar_t	cg_gibtime;
vmCvar_t	cg_paintballMode;
vmCvar_t	cg_disableLevelStartFade;
vmCvar_t	cg_bigheadMode;
vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_drawsubtitles;
vmCvar_t	cg_drawSyncMessage;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_crosshairScale;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPersonOffset;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawSpeed;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_teamChatScaleX;
vmCvar_t 	cg_teamChatScaleY;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t 	cg_scorePlum;
vmCvar_t	cg_newFont;
vmCvar_t	cg_newConsole;
vmCvar_t	cg_chatTime;
vmCvar_t 	cg_teamChatTime;
vmCvar_t	cg_consoleTime;

vmCvar_t 	cg_teamChatY;
vmCvar_t 	cg_chatY;
vmCvar_t	cg_fontScale;
vmCvar_t	cg_fontShadow;

vmCvar_t	cg_consoleSizeX;
vmCvar_t	cg_consoleSizeY;
vmCvar_t	cg_chatSizeX;
vmCvar_t	cg_chatSizeY;
vmCvar_t	cg_teamChatSizeX;
vmCvar_t	cg_teamChatSizeY;

vmCvar_t	cg_consoleLines;
vmCvar_t	cg_commonConsoleLines;
vmCvar_t	cg_chatLines;
vmCvar_t	cg_teamChatLines;

vmCvar_t	cg_commonConsole;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t    pmove_float;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraEyes_Fwd;
vmCvar_t	cg_cameraEyes_Up;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_leiEnhancement;
vmCvar_t	cg_leiBrassNoise;
vmCvar_t	cg_leiGoreNoise;
vmCvar_t	cg_trueLightning;
vmCvar_t    cg_music;
vmCvar_t    cg_weaponOrder;

vmCvar_t    cg_cameramode;
vmCvar_t    cg_cameraEyes;

vmCvar_t	cg_obeliskRespawnDelay;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;

//unlagged - client options
vmCvar_t	cg_delag;
vmCvar_t	cg_cmdTimeNudge;
vmCvar_t	sv_fps;
vmCvar_t	cg_projectileNudge;
vmCvar_t	cg_optimizePrediction;
vmCvar_t	cl_timeNudge;
//unlagged - client options

//elimination addition
vmCvar_t	cg_alwaysWeaponBar;
vmCvar_t	cg_hitsound;
vmCvar_t    cg_voteflags;

vmCvar_t    cg_autovertex;
// custom variable used in modified atmospheric effects from q3f
vmCvar_t	cg_atmosphericLevel;

vmCvar_t	cg_crosshairPulse;

vmCvar_t	cg_crosshairColorRed;
vmCvar_t	cg_crosshairColorGreen;
vmCvar_t	cg_crosshairColorBlue;

vmCvar_t	cg_weaponBarStyle;
vmCvar_t	cg_chatBeep;
vmCvar_t	cg_teamChatBeep;

int	mod_mgspread;
int	mod_sgspread;
int mod_sgcount;
int	mod_lgrange;
int	mod_cgspread;
int mod_jumpheight;
int	mod_gdelay;
int	mod_mgdelay;
int	mod_sgdelay;
int	mod_gldelay;
int	mod_rldelay;
int	mod_lgdelay;
int	mod_pgdelay;
int	mod_rgdelay;
int	mod_bfgdelay;
int	mod_ngdelay;
int	mod_pldelay;
int	mod_cgdelay;
int	mod_ftdelay;
int	mod_amdelay;
float mod_hastefirespeed;
float mod_ammoregenfirespeed;
float mod_scoutfirespeed;
float	mod_guardfirespeed;
float	mod_doublerfirespeed;
int mod_noplayerclip;
int	mod_ammolimit;
int mod_invulmove;
float mod_teamred_firespeed;
float mod_teamblue_firespeed;
int mod_medkitlimit;
int mod_medkitinf;
int mod_teleporterinf;
int mod_portalinf;
int mod_kamikazeinf;
int mod_invulinf;
int mod_accelerate;
int mod_slickmove;
int mod_overlay;
int mod_gravity;
int mod_fogModel;
int mod_fogShader;
int mod_fogDistance;
int mod_fogInterval;
int mod_fogColorR;
int mod_fogColorG;
int mod_fogColorB;
int mod_fogColorA;
int mod_skyShader;
int mod_skyColorR;
int mod_skyColorG;
int mod_skyColorB;
int mod_skyColorA;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;



static cvarTable_t cvarTable[] = { // bk001129

	{ &g_gametype, "g_gametype", "0", 0},

	{ &cl_propsmallsizescale, "cl_propsmallsizescale", "0.60", CVAR_ARCHIVE},
	{ &cl_propheight, "cl_propheight", "21", CVAR_ARCHIVE  },
	{ &cl_propspacewidth, "cl_propspacewidth", "8", CVAR_ARCHIVE  },
	{ &cl_propgapwidth, "cl_propgapwidth", "3", CVAR_ARCHIVE  },
	{ &cl_smallcharwidth, "cl_smallcharwidth", "8", CVAR_ARCHIVE  },
	{ &cl_smallcharheight, "cl_smallcharheight", "12", CVAR_ARCHIVE  },
	{ &cl_bigcharwidth, "cl_bigcharwidth", "12", CVAR_ARCHIVE  },
	{ &cl_bigcharheight, "cl_bigcharheight", "12", CVAR_ARCHIVE  },
	{ &cl_giantcharwidth, "cl_giantcharwidth", "20", CVAR_ARCHIVE  },
	{ &cl_giantcharheight, "cl_giantcharheight", "32", CVAR_ARCHIVE  },
	
	{ &cg_itemscaletime, "cg_itemscaletime", "5000", CVAR_ARCHIVE },
	{ &cg_weaponselecttime, "cg_weaponselecttime", "5000", CVAR_ARCHIVE },
	{ &cg_zoomtime, "cg_zoomtime", "300", CVAR_ARCHIVE },

	//ArenaSandBox Set
	{ &toolgun_mod1, "toolgun_mod1", "0", 0},
	{ &toolgun_mod2, "toolgun_mod2", "0", 0},
	{ &toolgun_mod3, "toolgun_mod3", "0", 0},
	{ &toolgun_mod4, "toolgun_mod4", "0", 0},
	{ &toolgun_mod5, "toolgun_mod5", "0", 0},
	{ &toolgun_mod6, "toolgun_mod6", "0", 0},
	{ &toolgun_mod7, "toolgun_mod7", "0", 0},
	{ &toolgun_mod8, "toolgun_mod8", "0", 0},
	{ &toolgun_mod9, "toolgun_mod9", "0", 0},
	{ &toolgun_mod10, "toolgun_mod10", "0", 0},
	{ &toolgun_mod11, "toolgun_mod11", "0", 0},
	{ &toolgun_mod12, "toolgun_mod12", "0", 0},
	{ &toolgun_mod13, "toolgun_mod13", "0", 0},
	{ &toolgun_mod14, "toolgun_mod14", "0", 0},
	{ &toolgun_mod15, "toolgun_mod15", "0", 0},
	{ &toolgun_mod16, "toolgun_mod16", "0", 0},
	{ &toolgun_mod17, "toolgun_mod17", "0", 0},
	{ &toolgun_mod18, "toolgun_mod18", "0", 0},
	{ &toolgun_mod19, "toolgun_mod19", "0", 0},
	{ &toolgun_tool, "toolgun_tool", "0", CVAR_USERINFO},
	{ &toolgun_toolcmd1, "toolgun_toolcmd1", "", 0},
	{ &toolgun_toolcmd2, "toolgun_toolcmd2", "", 0},
	{ &toolgun_toolcmd3, "toolgun_toolcmd3", "", 0},
	{ &toolgun_toolcmd4, "toolgun_toolcmd4", "", 0},
	{ &toolgun_tooltext, "toolgun_tooltext", "", 0},
	{ &toolgun_tooltip1, "toolgun_tooltip1", "", 0},
	{ &toolgun_tooltip2, "toolgun_tooltip2", "", 0},
	{ &toolgun_tooltip3, "toolgun_tooltip3", "", 0},
	{ &toolgun_tooltip4, "toolgun_tooltip4", "", 0},
	{ &toolgun_toolmode1, "toolgun_toolmode1", "", 0},
	{ &toolgun_toolmode2, "toolgun_toolmode2", "", 0},
	{ &toolgun_toolmode3, "toolgun_toolmode3", "", 0},
	{ &toolgun_toolmode4, "toolgun_toolmode4", "", 0},
	{ &toolgun_modelst, "toolgun_modelst", "0", 0},
	{ &sb_classnum_view, "sb_classnum_view", "0", 0},
	{ &sb_texture_view, "sb_texture_view", "0", 0},
	{ &sb_texturename, "sb_texturename", "0", 0},
	{ &cg_hide255, "cg_hide255", "0", 0},

	{ &ns_haveerror, "ns_haveerror", "0", 0},

	{ &cg_postprocess, "cg_postprocess", "", 0 },
	{ &cl_language, "cl_language", "0", CVAR_ARCHIVE },
	{ &con_notifytime, "con_notifytime", "3", CVAR_ARCHIVE },
    { &ui_backcolors, "ui_backcolors", "1", CVAR_ARCHIVE },
	{ &cg_leiChibi, "cg_leiChibi", "0", CVAR_ARCHIVE}, // LEILEI
    { &cg_cameraeyes, "cg_cameraeyes", "0", CVAR_ARCHIVE },
    { &cg_helightred, "cg_helightred", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_helightgreen, "cg_helightgreen", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_helightblue, "cg_helightblue", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_tolightred, "cg_tolightred", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_tolightgreen, "cg_tolightgreen", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_tolightblue, "cg_tolightblue", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_plightred, "cg_plightred", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_plightgreen, "cg_plightgreen", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cg_plightblue, "cg_plightblue", "100", CVAR_USERINFO | CVAR_ARCHIVE },
    { &cl_screenoffset, "cl_screenoffset", "107", CVAR_ARCHIVE },
	{ &cg_itemstyle, "cg_itemstyle", "2", CVAR_ARCHIVE },
	{ &legsskin, "legsskin", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE },
	{ &team_legsskin, "team_legsskin", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_oldscoreboard, "cg_oldscoreboard", "0", CVAR_ARCHIVE },
	{ &cg_gibtime, "cg_gibtime", "30", CVAR_ARCHIVE },
	{ &cg_gibjump, "cg_gibjump", "350", CVAR_ARCHIVE },
	{ &cg_gibvelocity, "cg_gibvelocity", "350", CVAR_ARCHIVE },
	{ &cg_gibmodifier, "cg_gibmodifier", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "110", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", 0 },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_crosshairScale, "cg_crosshairScale", "24", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE },
	{ &cg_paintballMode, "cg_paintballMode", "0", CVAR_ARCHIVE },
	{ &cg_disableLevelStartFade , "cg_disableLevelStartFade", "0", CVAR_ARCHIVE | CVAR_CHEAT},
	{ &cg_bigheadMode, "cg_bigheadMode", "0", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "5", CVAR_ARCHIVE },
	{ &cg_gun_y, "cg_gunY", "-1", CVAR_ARCHIVE },
	{ &cg_gun_z, "cg_gunZ", "-1", CVAR_ARCHIVE },
	{ &cg_centertime, "cg_centertime", "6", CVAR_ARCHIVE },
	{ &cg_drawsubtitles, "cg_drawsubtitles", "1", CVAR_ARCHIVE },
	{ &cg_drawSyncMessage, "cg_drawsyncmessage", "1", CVAR_ARCHIVE },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_ARCHIVE },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "65", CVAR_ARCHIVE },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_ARCHIVE },
	{ &cg_thirdPersonOffset, "cg_thirdPersonOffset", "25", CVAR_ARCHIVE },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE},
	{ &cg_atmosphericLevel, "cg_atmosphericLevel", "2", CVAR_ARCHIVE },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE  },
	{ &cg_teamChatScaleX, "cg_teamChatScaleX", "0.7", CVAR_ARCHIVE  },
	{ &cg_teamChatScaleY, "cg_teamChatScaleY", "1", CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_alwaysWeaponBar, "cg_alwaysWeaponBar", "0", CVAR_ARCHIVE},	//Elimination
    { &cg_hitsound, "cg_hitsound", "0", CVAR_ARCHIVE},
    { &cg_voteflags, "cg_voteflags", "*", CVAR_ROM},
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo

    { &cg_autovertex, "cg_autovertex", "0", CVAR_ARCHIVE },
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO},

	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_ARCHIVE},
	{ &cg_chatTime ,    "cg_chatTime", "10000", CVAR_ARCHIVE},
	{ &cg_consoleTime , "cg_consoleTime", "10000", CVAR_ARCHIVE},
	{ &cg_teamChatTime, "cg_teamChatTime", "10000", CVAR_ARCHIVE  },
	
	{ &cg_toolguninfo, "cg_toolguninfo", "40", CVAR_ARCHIVE  },

	{ &cg_teamChatY, "cg_teamChatY", "180", CVAR_ARCHIVE  },
	{ &cg_chatY, "cg_chatY", "-230", CVAR_ARCHIVE  },

	{ &cg_newFont ,     "cg_newFont", "1", CVAR_ARCHIVE},

	{ &cg_newConsole ,  "cg_newConsole", "1", CVAR_ARCHIVE},

	{ &cg_consoleSizeX , "cg_consoleSizeX", "6", CVAR_ARCHIVE},
	{ &cg_consoleSizeY , "cg_consoleSizeY", "9", CVAR_ARCHIVE},
	{ &cg_chatSizeX , "cg_chatSizeX", "6", CVAR_ARCHIVE},
	{ &cg_chatSizeY , "cg_chatSizeY", "10", CVAR_ARCHIVE},
	{ &cg_teamChatSizeX , "cg_teamChatSizeX", "6", CVAR_ARCHIVE},
	{ &cg_teamChatSizeY , "cg_teamChatSizeY", "10", CVAR_ARCHIVE},

	{ &cg_commonConsole , "cg_commonConsole", "0", CVAR_ARCHIVE},

	{ &cg_consoleLines , "cg_consoleLines", "4", CVAR_ARCHIVE},
	{ &cg_commonConsoleLines , "cg_commonConsoleLines", "6", CVAR_ARCHIVE},
	{ &cg_chatLines , "cg_chatLines", "6", CVAR_ARCHIVE},
	{ &cg_teamChatLines , "cg_teamChatLines", "6", CVAR_ARCHIVE},

	{ &cg_fontScale , "cg_fontScale", "1.5", CVAR_ARCHIVE},
	{ &cg_fontShadow , "cg_fontShadow", "1", CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{ &pmove_msec, "pmove_msec", "11", CVAR_SYSTEMINFO},
        { &pmove_float, "pmove_float", "1", CVAR_SYSTEMINFO},
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{ &cg_leiEnhancement, "cg_leiEnhancement", "1", CVAR_ARCHIVE},				// LEILEI default off (in case of whiner)
	{ &cg_leiGoreNoise, "cg_leiGoreNoise", "1", CVAR_ARCHIVE},					// LEILEI
	{ &cg_leiBrassNoise, "cg_leiBrassNoise", "1", CVAR_ARCHIVE},				// LEILEI
	{ &cg_cameramode, "cg_cameramode", "0", CVAR_ARCHIVE},				// LEILEI
	{ &cg_cameraEyes, "cg_cameraEyes", "0", CVAR_ARCHIVE},				// LEILEI
	{ &cg_cameraEyes_Fwd, "cg_cameraEyes_Fwd", "3", CVAR_ARCHIVE},				// LEILEI
	{ &cg_cameraEyes_Up, "cg_cameraEyes_Up", "3", CVAR_ARCHIVE},				// LEILEI
	//unlagged - client options
	{ &cg_delag, "cg_delag", "1", CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_cmdTimeNudge, "cg_cmdTimeNudge", "0", CVAR_ARCHIVE | CVAR_USERINFO },
	// this will be automagically copied from the server
	{ &sv_fps, "sv_fps", "60", CVAR_SYSTEMINFO },
	{ &cg_projectileNudge, "cg_projectileNudge", "0", CVAR_ARCHIVE },
	{ &cg_optimizePrediction, "cg_optimizePrediction", "1", CVAR_ARCHIVE },
	{ &cl_timeNudge, "cl_timeNudge", "0", CVAR_ARCHIVE },
	//unlagged - client options
	{ &cg_trueLightning, "cg_trueLightning", "0.9", CVAR_ARCHIVE},
    { &cg_music, "cg_music", "", CVAR_ARCHIVE},

	{ &cg_crosshairPulse, "cg_crosshairPulse", "1", CVAR_ARCHIVE},

	{ &cg_crosshairColorRed, "cg_crosshairColorRed", "0.5", CVAR_ARCHIVE | CVAR_USERINFO},
    { &cg_crosshairColorGreen, "cg_crosshairColorGreen", "0.75", CVAR_ARCHIVE | CVAR_USERINFO},
    { &cg_crosshairColorBlue, "cg_crosshairColorBlue", "1.0", CVAR_ARCHIVE | CVAR_USERINFO},

	{ &cg_weaponBarStyle, "cg_weaponBarStyle", "0", CVAR_ARCHIVE},
    { &cg_weaponOrder,"cg_weaponOrder", "/1/2/4/3/6/7/8/9/5/", CVAR_ARCHIVE},
    { &cg_chatBeep, "cg_chatBeep", "1", CVAR_ARCHIVE },
    { &cg_teamChatBeep, "cg_teamChatBeep", "1", CVAR_ARCHIVE }
};

static int  cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );
/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	trap_Cvar_Register(NULL, "model", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_headmodel", "sarge/default", CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		//unlagged - client options
		if ( cv->vmCvar == &cg_cmdTimeNudge ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 999 );
		}
		else if ( cv->vmCvar == &cl_timeNudge ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, -50, 50 );
		}
		//unlagged - client options
        else if ( cv->vmCvar == &cg_errorDecay ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 250 );
		}
		else if ( cv->vmCvar == &con_notifytime ) {
			if (cg_newConsole.integer ) {
				if (cv->vmCvar->integer != -1) {
					Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "-1");
					trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				}
			} else if (cv->vmCvar->integer <= 0) {
				Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "%s", cv->defaultString);
				trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
			}
		}
		trap_Cvar_Update( cv->vmCvar );
	}

	trap_Cvar_Set( "teamoverlay", "1" );
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_PrintfChat( qboolean team, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		if (team) {
			CG_AddToGenericConsole(text, &cgs.teamChat);
		} else {
			CG_AddToGenericConsole(text, &cgs.chat);
		}
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		CG_AddToGenericConsole(text, &cgs.console);
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound_SourceTech( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string",
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound_SourceTech( data, qfalse );
		}
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	cgs.media.oneMinuteSound = trap_S_RegisterSound_SourceTech( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound_SourceTech( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound_SourceTech( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound_SourceTech( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound_SourceTech( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound_SourceTech( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound_SourceTech( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound_SourceTech( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound_SourceTech( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound_SourceTech( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound_SourceTech( "sound/feedback/prepare.wav", qtrue );

	// N_G: Another condition that makes no sense to me, see for
	// yourself if you really meant this
	// Sago: Makes perfect sense: Load team game stuff if the gametype is a teamgame and not an exception (like GT_LMS)
	if ( ( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) ||
		cg_buildScript.integer ) {

		cgs.media.captureAwardSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.redLeadsSound = trap_S_RegisterSound_SourceTech( "sound/feedback/redleads.wav", qtrue );
		cgs.media.blueLeadsSound = trap_S_RegisterSound_SourceTech( "sound/feedback/blueleads.wav", qtrue );
		cgs.media.teamsTiedSound = trap_S_RegisterSound_SourceTech( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound_SourceTech( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_red_scores.wav", qtrue );
		cgs.media.blueScoredSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_blue_scores.wav", qtrue );

		cgs.media.captureYourTeamSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagtaken_opponent.wav", qtrue );

		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION|| cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_blue_returned.wav", qtrue );
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_team_flag.wav", qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ||cg_buildScript.integer ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_you_flag.wav", qtrue );
			cgs.media.holyShitSound = trap_S_RegisterSound_SourceTech("sound/feedback/voc_holyshit.wav", qtrue);
		}

                if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound_SourceTech( "sound/teamplay/voc_base_attack.wav", qtrue );
		}
	}

	cgs.media.tracerSound = trap_S_RegisterSound_SourceTech( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound_SourceTech( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound_SourceTech( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound_SourceTech( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = trap_S_RegisterSound_SourceTech( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound_SourceTech( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound_SourceTech( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound_SourceTech( "sound/player/gibimp3.wav", qfalse );

	cgs.media.lspl1Sound = trap_S_RegisterSound_SourceTech( "sound/le/splat1.wav", qfalse );
	cgs.media.lspl2Sound = trap_S_RegisterSound_SourceTech( "sound/le/splat2.wav", qfalse );
	cgs.media.lspl3Sound = trap_S_RegisterSound_SourceTech( "sound/le/splat3.wav", qfalse );

	cgs.media.lbul1Sound = trap_S_RegisterSound_SourceTech( "sound/le/bullet1.wav", qfalse );
	cgs.media.lbul2Sound = trap_S_RegisterSound_SourceTech( "sound/le/bullet2.wav", qfalse );
	cgs.media.lbul3Sound = trap_S_RegisterSound_SourceTech( "sound/le/bullet3.wav", qfalse );

	cgs.media.lshl1Sound = trap_S_RegisterSound_SourceTech( "sound/le/shell1.wav", qfalse );
	cgs.media.lshl2Sound = trap_S_RegisterSound_SourceTech( "sound/le/shell2.wav", qfalse );
	cgs.media.lshl3Sound = trap_S_RegisterSound_SourceTech( "sound/le/shell3.wav", qfalse );

	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound_SourceTech( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound_SourceTech( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound_SourceTech( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound_SourceTech( "sound/items/invul_impact_03.wav", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound_SourceTech( "sound/items/invul_juiced.wav", qfalse );

	cgs.media.ammoregenSound = trap_S_RegisterSound_SourceTech("sound/items/cl_ammoregen.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound_SourceTech("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound_SourceTech("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound_SourceTech("sound/items/cl_scout.wav", qfalse);
        cgs.media.obeliskHitSound1 = trap_S_RegisterSound_SourceTech( "sound/items/obelisk_hit_01.wav", qfalse );
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound_SourceTech( "sound/items/obelisk_hit_02.wav", qfalse );
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound_SourceTech( "sound/items/obelisk_hit_03.wav", qfalse );
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound_SourceTech( "sound/items/obelisk_respawn.wav", qfalse );

	cgs.media.teleInSound = trap_S_RegisterSound_SourceTech( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound_SourceTech( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound_SourceTech( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound_SourceTech( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound_SourceTech( "sound/player/talk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound_SourceTech( "sound/player/land1.wav", qfalse);

	cgs.media.notifySound = trap_S_RegisterSound_SourceTech( "sound/notify.wav", qfalse );

    switch(cg_hitsound.integer) {
        case 0:
        default:
        cgs.media.hitSound = trap_S_RegisterSound_SourceTech( "sound/feedback/hitde.wav", qfalse );
    };

	cgs.media.hitSoundHighArmor = trap_S_RegisterSound_SourceTech( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound_SourceTech( "sound/feedback/hitlo.wav", qfalse );

	cgs.media.impressiveSound = trap_S_RegisterSound_SourceTech( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound_SourceTech( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound_SourceTech( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound_SourceTech( "sound/feedback/humiliation.wav", qtrue );
	cgs.media.assistSound = trap_S_RegisterSound_SourceTech( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = trap_S_RegisterSound_SourceTech( "sound/feedback/defense.wav", qtrue );

	cgs.media.takenLeadSound = trap_S_RegisterSound_SourceTech( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound_SourceTech( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound_SourceTech( "sound/feedback/lostlead.wav", qtrue);

	cgs.media.voteNow = trap_S_RegisterSound_SourceTech( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound_SourceTech( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound_SourceTech( "sound/feedback/vote_failed.wav", qtrue);

	cgs.media.watrInSound = trap_S_RegisterSound_SourceTech( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound_SourceTech( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound_SourceTech( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound_SourceTech ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound_SourceTech (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound_SourceTech (name, qfalse);
	}
	
	for (i=0 ; i<=10 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/vehicle/engine%i.ogg", i);
		cgs.media.carengine[i] = trap_S_RegisterSound_SourceTech (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		CG_RegisterItemSounds( i );
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound_SourceTech( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound_SourceTech( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound_SourceTech ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound_SourceTech("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound_SourceTech ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound_SourceTech ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound_SourceTech ("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_railg = trap_S_RegisterSound_SourceTech ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound_SourceTech ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound_SourceTech ("sound/weapons/plasma/plasmx1a.wav", qfalse);
	cgs.media.sfx_proxexp = trap_S_RegisterSound_SourceTech( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound_SourceTech( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound_SourceTech( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound_SourceTech( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	cgs.media.sfx_chghit = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
	cgs.media.weaponHoverSound = trap_S_RegisterSound_SourceTech( "sound/weapons/weapon_hover.wav", qfalse );
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound_SourceTech( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound_SourceTech( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound_SourceTech( "sound/items/kam_explode_far.wav", qfalse );
	cgs.media.winnerSound = trap_S_RegisterSound_SourceTech( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = trap_S_RegisterSound_SourceTech( "sound/feedback/voc_youlose.wav", qfalse );
	cgs.media.youSuckSound = trap_S_RegisterSound_SourceTech( "sound/misc/yousuck.wav", qfalse );

	cgs.media.wstbimplSound = trap_S_RegisterSound_SourceTech("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound_SourceTech("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound_SourceTech("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound_SourceTech("sound/weapons/proxmine/wstbactv.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound_SourceTech("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound_SourceTech("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound_SourceTech("sound/items/n_health.wav", qfalse );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound_SourceTech("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound_SourceTech("sound/weapons/grenade/hgrenb2a.wav", qfalse);
}

/*
=================
CG_RegisterOverlay

Registers the graphic for the target_effect overlay.
=================
*/
void CG_RegisterOverlay( void ) {
	const char *overlay;

	overlay = CG_ConfigString( CS_OVERLAY );
	if ( strlen(overlay) ) {
		cgs.media.effectOverlay = trap_R_RegisterShaderNoMip( overlay );
	} else {
		cgs.media.effectOverlay = 0;
	}
}

void CG_SetDefaultWeaponProperties(void) {
	mod_sgspread = 700;
	mod_sgcount = 11;
	mod_lgrange = 768;
	mod_mgspread = 200;
	mod_cgspread = 600;
	mod_jumpheight = 270;
	mod_gdelay = 400;
	mod_mgdelay = 100;
	mod_sgdelay = 1000;
	mod_gldelay = 800;
	mod_rldelay = 800;
	mod_lgdelay = 50;
	mod_pgdelay = 100;
	mod_rgdelay = 1500;
	mod_bfgdelay = 200;
	mod_ngdelay = 1000;
	mod_pldelay = 800;
	mod_cgdelay = 30;
	mod_ftdelay = 40;
	mod_amdelay = 40;
	mod_scoutfirespeed = 1.5;
	mod_doublerfirespeed = 1;
	mod_guardfirespeed = 1;
	mod_hastefirespeed = 1.3;
	mod_ammoregenfirespeed = 1.3;
	mod_noplayerclip = 0;
	mod_ammolimit = 200;
	mod_invulmove = 0;
	mod_teamred_firespeed = 1;
	mod_teamblue_firespeed = 1;
	mod_medkitlimit = 200;
	mod_medkitinf = 0;
	mod_teleporterinf = 0;
	mod_portalinf = 0;
	mod_kamikazeinf = 0;
	mod_invulinf = 0;
	mod_accelerate = 1;
	mod_slickmove = 0;
	mod_overlay = 0;
	mod_gravity = 800;
    mod_fogModel = 0;
    mod_fogShader = 0;
    mod_fogDistance = 0;
    mod_fogInterval = 0;
    mod_fogColorR = 0;
    mod_fogColorG = 0;
    mod_fogColorB = 0;
    mod_fogColorA = 0;
    mod_skyShader = 0;
    mod_skyColorR = 0;
    mod_skyColorG = 0;
    mod_skyColorB = 0;
    mod_skyColorA = 0;
}

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	gitem_t		*backpack;
	int			i;
	char		items[MAX_ITEMS+1];
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );
	
	CG_SetDefaultWeaponProperties();

	// precache status bar pics
	if(cl_language.integer == 0){
	CG_LoadingString( "game media" );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( "игровые ресурсы" );
	}

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );
	cgs.media.botSkillShaders[5] = trap_R_RegisterShader( "menu/art/skill6.tga" );
	cgs.media.botSkillShaders[6] = trap_R_RegisterShader( "menu/art/skill7.tga" );
	cgs.media.botSkillShaders[7] = trap_R_RegisterShader( "menu/art/skill8.tga" );
	cgs.media.botSkillShaders[8] = trap_R_RegisterShader( "menu/art/skill9.tga" );

	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.objectivesOverlay = trap_R_RegisterShaderNoMip( CG_ConfigString(CS_OBJECTIVESOVERLAY) );
	cgs.media.objectivesUpdated = trap_R_RegisterShaderNoMip( "menu/objectives/updated.tga" );
	cgs.media.objectivesUpdatedSound = trap_S_RegisterSound_SourceTech( "sound/misc/objective_update_01.wav", qfalse );

	cgs.media.deathImage = trap_R_RegisterShaderNoMip( "menu/art/level_complete5" );

	cgs.media.scoreShow = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rocklx1a.wav", qfalse );
	cgs.media.finalScoreShow = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rocklx1a.wav", qfalse );

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
	cgs.media.blueProxMine = trap_R_RegisterModel_SourceTech( "models/weaphits/proxmineb.md3" );
	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	cgs.media.flameBallShader = trap_R_RegisterShader( "sprites/flameball" );
	cgs.media.antimatterBallShader = trap_R_RegisterShader( "sprites/antimatter" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for (i = 0; i < NUM_CROSSHAIRS; i++ ) {
		if (i < 10){
			cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
		    cgs.media.crosshairSh3d[i] = trap_R_RegisterShader( va("gfx/3d/crosshair%c", 'a'+i) );
		} else {
			cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%02d", i - 10) );
			cgs.media.crosshairSh3d[i] = trap_R_RegisterShader( va("gfx/3d/crosshair%02d", i - 10) );
		}
 	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );
	
	cgs.media.ptexShader[0]	= trap_R_RegisterShader( "trans" );
	cgs.media.ptexShader[1]	= trap_R_RegisterShader( "powerups/quad" );

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION|| cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.redCubeModel = trap_R_RegisterModel_SourceTech( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel_SourceTech( "models/powerups/orb/b_orb.md3" );
		cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	}

        if( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) {
                cgs.media.redOverlay = trap_R_RegisterShader( "overlay/red");
                cgs.media.blueOverlay = trap_R_RegisterShader( "overlay/blue");
        } else {
                cgs.media.neutralOverlay = trap_R_RegisterShader( "overlay/neutral");
        }

//For Double Domination:
	if ( cgs.gametype == GT_DOUBLE_D ) {
		cgs.media.ddPointSkinA[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/icona_red" );
                cgs.media.ddPointSkinA[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/icona_blue" );
                cgs.media.ddPointSkinA[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/icona_white" );
                cgs.media.ddPointSkinA[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );

                cgs.media.ddPointSkinB[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/iconb_red" );
                cgs.media.ddPointSkinB[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/iconb_blue" );
                cgs.media.ddPointSkinB[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconb_white" );
                cgs.media.ddPointSkinB[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );
	}

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.redFlagModel = trap_R_RegisterModel_SourceTech( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel_SourceTech( "models/flags/b_flag.md3" );
                cgs.media.neutralFlagModel = trap_R_RegisterModel_SourceTech( "models/flags/n_flag.md3" );
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );
		cgs.media.flagPoleModel = trap_R_RegisterModel_SourceTech( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel_SourceTech( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );

		cgs.media.redFlagBaseModel = trap_R_RegisterModel_SourceTech( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel_SourceTech( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel_SourceTech( "models/mapobjects/flagbase/ntrl_base.md3" );
	}

	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
		cgs.media.neutralFlagModel = trap_R_RegisterModel_SourceTech( "models/flags/n_flag.md3" );
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel_SourceTech( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel_SourceTech( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel_SourceTech( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel_SourceTech( "models/powerups/overload_energy.md3" );
	}

	if ( cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel_SourceTech( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel_SourceTech( "models/powerups/obelisk/obelisk.md3" );
	}

	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	if ( ( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) ||
		cg_buildScript.integer ) {

		cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe3" );
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		//cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" ); - moved outside, used by accuracy
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
	}
	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );

	cgs.media.armorModel = trap_R_RegisterModel_SourceTech( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel_SourceTech( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel_SourceTech( "models/weapons2/shells/s_shell.md3" );

	cgs.media.gibAbdomen = trap_R_RegisterModel_SourceTech( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel_SourceTech( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel_SourceTech( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel_SourceTech( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel_SourceTech( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel_SourceTech( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel_SourceTech( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel_SourceTech( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel_SourceTech( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel_SourceTech( "models/gibs/brain.md3" );

	cgs.media.debrislight1 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b1.md3" );
	cgs.media.debrislight2 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b2.md3" );
	cgs.media.debrislight3 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b3.md3" );
	cgs.media.debrislight4 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b4.md3" );
	cgs.media.debrislight5 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b5.md3" );
	cgs.media.debrislight6 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b6.md3" );
	cgs.media.debrislight7 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b7.md3" );
	cgs.media.debrislight8 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b8.md3" );

	cgs.media.debrisdark1 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d1.md3" );
	cgs.media.debrisdark2 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d2.md3" );
	cgs.media.debrisdark3 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d3.md3" );
	cgs.media.debrisdark4 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d4.md3" );
	cgs.media.debrisdark5 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d5.md3" );
	cgs.media.debrisdark6 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d6.md3" );
	cgs.media.debrisdark7 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d7.md3" );
	cgs.media.debrisdark8 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_d8.md3" );

	cgs.media.debrislightlarge1 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b1_large.md3" );
	cgs.media.debrislightlarge2 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b2_large.md3" );
	cgs.media.debrislightlarge3 = trap_R_RegisterModel_SourceTech( "models/debris/concrete_b3_large.md3" );

	cgs.media.debrisdarklarge1 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b1.md3" );
	cgs.media.debrisdarklarge2 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b1.md3" );
	cgs.media.debrisdarklarge3 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b1.md3" );

	cgs.media.debriswood1 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b1.md3" );
	cgs.media.debriswood2 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b2.md3" );
	cgs.media.debriswood3 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b3.md3" );
	cgs.media.debriswood4 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b4.md3" );
	cgs.media.debriswood5 = trap_R_RegisterModel_SourceTech( "models/debris/wood_b5.md3" );

	cgs.media.debrisglass1 = trap_R_RegisterModel_SourceTech( "models/debris/glass_1.md3" );
	cgs.media.debrisglass2 = trap_R_RegisterModel_SourceTech( "models/debris/glass_2.md3" );
	cgs.media.debrisglass3 = trap_R_RegisterModel_SourceTech( "models/debris/glass_3.md3" );
	cgs.media.debrisglass4 = trap_R_RegisterModel_SourceTech( "models/debris/glass_4.md3" );
	cgs.media.debrisglass5 = trap_R_RegisterModel_SourceTech( "models/debris/glass_5.md3" );

	cgs.media.debrisglasslarge1 = trap_R_RegisterModel_SourceTech( "models/debris/glass_1_large.md3" );
	cgs.media.debrisglasslarge2 = trap_R_RegisterModel_SourceTech( "models/debris/glass_2_large.md3" );
	cgs.media.debrisglasslarge3 = trap_R_RegisterModel_SourceTech( "models/debris/glass_3_large.md3" );
	cgs.media.debrisglasslarge4 = trap_R_RegisterModel_SourceTech( "models/debris/glass_4_large.md3" );
	cgs.media.debrisglasslarge5 = trap_R_RegisterModel_SourceTech( "models/debris/glass_5_large.md3" );
	
	cgs.media.debrisstone1 = trap_R_RegisterModel_SourceTech( "models/debris/stone_1.md3" );
	cgs.media.debrisstone2 = trap_R_RegisterModel_SourceTech( "models/debris/stone_2.md3" );
	cgs.media.debrisstone3 = trap_R_RegisterModel_SourceTech( "models/debris/stone_3.md3" );
	cgs.media.debrisstone4 = trap_R_RegisterModel_SourceTech( "models/debris/stone_4.md3" );
	cgs.media.debrisstone5 = trap_R_RegisterModel_SourceTech( "models/debris/stone_5.md3" );

	cgs.media.sparkShader = trap_R_RegisterShaderNoMip("spark");
	
	cgs.media.smoke2 = trap_R_RegisterModel_SourceTech( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.bulletFlashModel = trap_R_RegisterModel_SourceTech("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel_SourceTech("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel_SourceTech("models/weaphits/boom01.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel_SourceTech( "models/powerups/pop.md3" );
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel_SourceTech( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel_SourceTech( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel_SourceTech( "models/powerups/kamikazi.md3" );
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel_SourceTech( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel_SourceTech( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel_SourceTech( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel_SourceTech( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel_SourceTech( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel_SourceTech( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel_SourceTech( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel = trap_R_RegisterModel_SourceTech( "models/powerups/regen.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );


	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel_SourceTech( "models/powerups/shield/shield.md3" );
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );

	// LEILEI SHADERS
	cgs.media.lsmkShader1 = trap_R_RegisterShader("leismoke1" );
	cgs.media.lsmkShader2 = trap_R_RegisterShader("leismoke2" );
	cgs.media.lsmkShader3 = trap_R_RegisterShader("leismoke3" );
	cgs.media.lsmkShader4 = trap_R_RegisterShader("leismoke4" );

	cgs.media.lsplShader = trap_R_RegisterShader("leisplash" );
	cgs.media.lspkShader1 = trap_R_RegisterShader("leispark" );
	cgs.media.lspkShader2 = trap_R_RegisterShader("leispark2" );
	cgs.media.lbumShader1 = trap_R_RegisterShader("leiboom1" );
	cgs.media.lfblShader1 = trap_R_RegisterShader("leifball" );

	cgs.media.lbldShader1 = trap_R_RegisterShader("leiblood1" );
	cgs.media.lbldShader2 = trap_R_RegisterShader("leiblood2" );	// this is a mark, by the way

	// New Bullet Marks
	cgs.media.lmarkmetal1 = trap_R_RegisterShader("leimetalmark1" );
	cgs.media.lmarkmetal2 = trap_R_RegisterShader("leimetalmark2" );
	cgs.media.lmarkmetal3 = trap_R_RegisterShader("leimetalmark3" );
	cgs.media.lmarkmetal4 = trap_R_RegisterShader("leimetalmark4" );
	cgs.media.lmarkbullet1 = trap_R_RegisterShader("leibulletmark1" );
	cgs.media.lmarkbullet2 = trap_R_RegisterShader("leibulletmark2" );
	cgs.media.lmarkbullet3 = trap_R_RegisterShader("leibulletmark3" );
	cgs.media.lmarkbullet4 = trap_R_RegisterShader("leibulletmark4" );


	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || 1 ) {
			//CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );

	// paintball mode marks
	cgs.media.bulletMarkPaintShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk_paint" );
	cgs.media.burnMarkPaintShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk_paint" );
	cgs.media.holeMarkPaintShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk_paint" );
	cgs.media.energyMarkPaintShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk_paint" );

	//explosion effect
	cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel_SourceTech( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel_SourceTech( modelName );
	}

	cgs.media.railCoreShader = trap_R_RegisterShader("railCore");

	CG_ClearParticles ();
}

/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

/*
======================
CG_StartMusic
======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	if ( *cg_music.string && Q_stricmp( cg_music.string, "none" ) ) {
		s = (char *)cg_music.string;
	} else {
		s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
        }
}

void CG_StartScoreboardMusic( void ) {
	char	var[MAX_TOKEN_CHARS];
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	//if music volume is 0, don't start the scoreboard music
	trap_Cvar_VariableStringBuffer( "s_musicvolume", var, sizeof( var ) );
	if ( !strcmp(var, "0") )
		return;

	// start the background music
	s = (char *)CG_ConfigString( CS_SCOREBOARDMUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

void CG_StartDeathMusic( void ) {
	char	var[MAX_TOKEN_CHARS];
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	//if music volume is 0, don't start the scoreboard music
	trap_Cvar_VariableStringBuffer( "s_musicvolume", var, sizeof( var ) );
	if ( !strcmp(var, "0") )
		return;

	// start the background music
	s = (char *)CG_ConfigString( CS_DEATHMUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
	cg.deathmusicStarted = qtrue;
}

void CG_StopDeathMusic( void ) {
	trap_S_StopBackgroundTrack();
	cg.deathmusicStarted = qfalse;
	CG_StartMusic();
}

int wideAdjustX; // leilei - dirty widescreen hack

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.defaultFont[0]		= trap_R_RegisterShader( "gfx/2d/default_font" ); //32x32
	cgs.media.defaultFont[1]		= trap_R_RegisterShader( "gfx/2d/default_font1" ); //64x64
	cgs.media.defaultFont[2]		= trap_R_RegisterShader( "gfx/2d/default_font2" ); //128x128
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.corner          	= trap_R_RegisterShader( "corner" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	trap_Cvar_Set("ns_haveerror", "0");
	NS_OpenScript("nscript/cgame/init.ns", NULL, 0);		//Noire.Script Init in cgame.qvm

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	realVidWidth = cgs.glconfig.vidWidth;
	realVidHeight = cgs.glconfig.vidHeight;
	{
		float resbias, resbiasy;
		float rex, rey, rias;
		int newresx, newresy;
		float adjustx, adjusty;

		rex = 640.0f / realVidWidth;
		rey = 480.0f / realVidHeight;

		newresx = 640.0f * (rex);
		newresy = 480.0f * (rey);

		newresx = realVidWidth * rey;
		newresy = realVidHeight * rey;

		resbias  = 0.5 * ( newresx -  ( newresy * (640.0/480.0) ) );
		resbiasy = 0.5 * ( newresy -  ( newresx * (640.0/480.0) ) );


		wideAdjustX = resbias;
	}
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		// wide screen
		cgs.screenXBias = 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * (640.0/480.0) ) );
		cgs.screenXScale = cgs.screenYScale;
	}
	else {
		// no wide screen
		cgs.screenXBias = 0;
	}



	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	if(cl_language.integer == 0){
	CG_LoadingString( "collision map" );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( "карта столкновений" );
	}

	trap_CM_LoadMap( cgs.mapname );

	cg.loading = qtrue;		// force players to load instead of defer

	if(cl_language.integer == 0){
	CG_LoadingString( "sounds" );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( "звуки" );
	}

	CG_RegisterSounds();

	if(cl_language.integer == 0){
	CG_LoadingString( "graphics" );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( "графика" );
	}

	CG_RegisterGraphics();

	if(cl_language.integer == 0){
	CG_LoadingString( "clients" );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( "игроки" );
	}

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
	
	trap_SendConsoleCommand("ns_openscript_ui tools/create.ns\n");
	trap_SendConsoleCommand( va("weapon %i\n", WP_TOOLGUN) );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*
==================
CG_IsTeamGame
returns true if we're currently in a team gametype
==================
*/
qboolean CG_IsTeamGame() {
	return qfalse;
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int type) {
}

void CG_KeyEvent(int key, qboolean down) {
}

void CG_MouseEvent(int x, int y) {
}

//unlagged - attack prediction #3
// moved from g_weapon.c
/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}
//unlagged - attack prediction #3

static qboolean do_vid_restart = qfalse;

void CG_FairCvars() {
    qboolean vid_restart_required = qfalse;
    char rendererinfos[128];

    if(cgs.videoflags & VF_LOCK_CVARS_EXTENDED) {
        //Lock extended cvars.
        trap_Cvar_VariableStringBuffer("r_subdivisions",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 20 ) {
            trap_Cvar_Set("r_subdivisions","20");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_picmip",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 3 ) {
            trap_Cvar_Set("r_picmip","3");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_picmip","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_intensity",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_intensity","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_intensity","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_mapoverbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_mapoverbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_mapoverbrightbits","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_overbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_overbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_overbrightbits","0");
            vid_restart_required = qtrue;
        }
    }

    if(cgs.videoflags & VF_LOCK_VERTEX) {
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) != 0 ) {
            trap_Cvar_Set("r_vertexlight","0");
            vid_restart_required = qtrue;
        }
    } else if(cg_autovertex.integer){
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) == 0 ) {
            trap_Cvar_Set("r_vertexlight","1");
            vid_restart_required = qtrue;
        }
    }

    if(vid_restart_required && do_vid_restart)
        trap_SendConsoleCommand("vid_restart");

    do_vid_restart = qtrue;

}

qhandle_t trap_R_RegisterModel_SourceTech( const char *name ) {
    char cvarname[1024];
    char itemname[1024];
    
	//Construct a replace cvar:
	Com_sprintf(cvarname, sizeof(cvarname), "replacemodel_%s", name);
    //Look an alternative model:
    trap_Cvar_VariableStringBuffer(cvarname,itemname,sizeof(itemname));
    if(itemname[0]==0) //If nothing found use original
        Com_sprintf(itemname, sizeof(itemname), "%s", name);
    else
        CG_Printf ("%s replaced by %s\n", name, itemname);
	
	return trap_R_RegisterModel( itemname );
}

sfxHandle_t	trap_S_RegisterSound_SourceTech( const char *sample, qboolean compressed ) {
    char cvarname[1024];
    char itemname[1024];
    
	//Construct a replace cvar:
	Com_sprintf(cvarname, sizeof(cvarname), "replacesound_%s", sample);
    //Look an alternative model:
    trap_Cvar_VariableStringBuffer(cvarname,itemname,sizeof(itemname));
    if(itemname[0]==0) //If nothing found use original
        Com_sprintf(itemname, sizeof(itemname), "%s", sample);
    else
        CG_Printf ("%s replaced by %s\n", sample, itemname);
	
	return trap_S_RegisterSound( itemname, compressed );
}


