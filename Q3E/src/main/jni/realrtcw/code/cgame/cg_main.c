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

/*
 * name:		cg_main.c
 *
 * desc:		initialization and primary entry point for cgame
 *
*/


#include "cg_local.h"
#include "../ui/ui_shared.h"

displayContextDef_t cgDC;

int forceModelModificationCount = -1;

void CG_Init( int serverMessageNum, int serverCommandSequence );
void CG_Shutdown( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
	switch ( command ) {
	case CG_GET_TAG:
		return CG_GetTag( arg0, (char *)arg1, (orientation_t *)arg2 );
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling( arg0 );
		return 0;
	case CG_INIT:
		CG_Init( arg0, arg1 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent( arg0, arg1 );
		return 0;
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		CG_MouseEvent( arg0, arg1 );
		return 0;
	default:
		CG_Error( "vmMain: unknown command %li", (long)command );
		break;
	}
	return -1;
}


cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t cg_items[MAX_ITEMS];

vmCvar_t cg_bobbing;
vmCvar_t cg_railTrailTime;
vmCvar_t cg_centertime;
vmCvar_t cg_buyprinttime;
vmCvar_t cg_egprinttime;
vmCvar_t cg_runpitch;
vmCvar_t cg_runroll;
vmCvar_t cg_bobup;
vmCvar_t cg_bobpitch;
vmCvar_t cg_bobroll;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_gibs;
vmCvar_t cg_drawTimer;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_draw3dIcons;
vmCvar_t cg_drawIcons;
vmCvar_t cg_youGotMail;         //----(SA)	added
vmCvar_t cg_drawAmmoWarning;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairBinoc;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawCrosshairPickups;
vmCvar_t cg_drawCrosshairReticle;
vmCvar_t cg_hudAlpha;
vmCvar_t cg_weaponCycleDelay;       //----(SA)	added
vmCvar_t cg_cycleAllWeaps;
vmCvar_t cg_useWeapsForZoom;
vmCvar_t cg_drawAllWeaps;
vmCvar_t cg_drawRewards;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairAlpha;     //----(SA)	added
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawSubtitles;
vmCvar_t cg_subtitleSize;
vmCvar_t cg_subtitleShadow;
vmCvar_t cg_drawFrags;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_drawSpreadScale;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_footsteps;
vmCvar_t cg_markTime;
vmCvar_t cg_brassTime;
vmCvar_t cg_viewsize;
vmCvar_t cg_letterbox;
vmCvar_t cg_drawGun;
vmCvar_t cg_drawFPGun;
vmCvar_t cg_drawGamemodels;
vmCvar_t cg_cursorHints;
vmCvar_t cg_hintFadeTime;       //----(SA)	added
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_tracerSpeed;
vmCvar_t cg_autoswitch;
vmCvar_t cg_ignore;
vmCvar_t cg_simpleItems;
vmCvar_t cg_fov;
vmCvar_t cg_fixedAspect;
vmCvar_t cg_fixedAspectFOV;
vmCvar_t cg_drawCheckpoint;
vmCvar_t cg_oldWolfUI;
vmCvar_t cg_drawStatusHead;
vmCvar_t cg_hudWeapIcon;
vmCvar_t cg_hudStamina;
vmCvar_t cg_hudAmmoClip;
vmCvar_t cg_hudStatus;
vmCvar_t cg_journalStyle;
vmCvar_t cg_zoomFov;
vmCvar_t cg_zoomStepBinoc;
vmCvar_t cg_zoomStepSniper;
vmCvar_t cg_zoomStepSnooper;
vmCvar_t cg_zoomStepFG;         //----(SA)	added
vmCvar_t cg_zoomSensitivity;
vmCvar_t cg_zoomSensitivityFovScaled;
vmCvar_t cg_zoomDefaultBinoc;
vmCvar_t cg_zoomDefaultSniper;
vmCvar_t cg_zoomDefaultSnooper;
vmCvar_t cg_zoomDefaultFG;      //----(SA)	added
vmCvar_t cg_reticles;
vmCvar_t cg_reticleBrightness;      //----(SA)	added
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_lagometer;
vmCvar_t cg_drawAttacker;
vmCvar_t cg_synchronousClients;
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_forceModel;
vmCvar_t cg_coronafardist;
vmCvar_t cg_coronas;
vmCvar_t cg_paused;
vmCvar_t cg_blood;
vmCvar_t cg_predictItems;
vmCvar_t cg_deferPlayers;
vmCvar_t cg_drawTeamOverlay;
vmCvar_t cg_enableBreath;
vmCvar_t cg_autoactivate;
vmCvar_t cg_useSuggestedWeapons;    //----(SA)	added
vmCvar_t cg_emptyswitch;
vmCvar_t cg_particleDist;
vmCvar_t cg_particleLOD;
vmCvar_t cg_blinktime;      //----(SA)	added

vmCvar_t cg_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

// Rafael - particle switch
vmCvar_t cg_wolfparticles;
// done

// Ridah
vmCvar_t cg_gameType;
vmCvar_t cg_newinventory;
vmCvar_t cg_bloodTime;
vmCvar_t cg_norender;
vmCvar_t cg_skybox;

// Rafael
vmCvar_t cg_gameSkill;
// done

vmCvar_t cg_hitSounds;

vmCvar_t cg_reloading;      //----(SA)	added

// JPW NERVE
vmCvar_t cg_medicChargeTime;
vmCvar_t cg_engineerChargeTime;
vmCvar_t cg_jumptime;
vmCvar_t cg_realism;

vmCvar_t cg_LTChargeTime;
vmCvar_t cg_soldierChargeTime;
vmCvar_t cg_redlimbotime;
vmCvar_t cg_bluelimbotime;
// jpw

vmCvar_t cg_hunkUsed;
vmCvar_t cg_soundAdjust;
vmCvar_t cg_expectedhunkusage;

vmCvar_t cg_showAIState;

vmCvar_t cg_notebook;
vmCvar_t cg_notebookpages;          // bitflags for the currently accessable pages.  if they wanna cheat, let 'em.  Most won't, or will wait 'til they actually play it.

vmCvar_t cg_currentSelectedPlayer;
vmCvar_t cg_currentSelectedPlayerName;
vmCvar_t cg_cameraMode;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_smallFont;
vmCvar_t cg_bigFont;
vmCvar_t cg_hudStyle;

vmCvar_t cg_animState;
vmCvar_t cg_missionStats;
vmCvar_t cg_waitForFire;

vmCvar_t cg_loadWeaponSelect;

// NERVE - SMF - Wolf multiplayer configuration cvars
vmCvar_t mp_playerType;
vmCvar_t mp_team;
vmCvar_t mp_weapon;
vmCvar_t mp_pistol;
vmCvar_t mp_item1;

vmCvar_t mp_item2;
vmCvar_t mp_mapDesc;
vmCvar_t mp_mapTitle;
vmCvar_t mp_itemDesc;

// RealRTCW new CVARs
vmCvar_t cg_solidCrosshair;
vmCvar_t cg_bloodBlend;
vmCvar_t cg_snipersCrosshair;
vmCvar_t cg_atmosphericEffects;
vmCvar_t cg_lowAtmosphericEffects;
vmCvar_t cg_forceAtmosphericEffects;
vmCvar_t cg_ironChallenge;
vmCvar_t cg_nohudChallenge;
vmCvar_t cg_nopickupChallenge;
vmCvar_t cg_decayChallenge;
vmCvar_t cg_vanilla_guns;
vmCvar_t cg_autoReload;
vmCvar_t cg_uinfo;
vmCvar_t int_cl_maxpackets;
vmCvar_t int_cl_timenudge;
vmCvar_t cg_bodysink;
vmCvar_t cg_gunPosLock;
vmCvar_t cg_weaponBounceSound;

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int cvarFlags;
} cvarTable_t;

cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 },  // used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "2", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_drawGamemodels, "cg_drawGamemodels", "1", CVAR_CHEAT },
	{ &cg_drawFPGun, "cg_drawFPGun", "1", CVAR_ARCHIVE },
	{ &cg_gun_frame, "cg_gun_frame", "0", CVAR_TEMP },
	{ &cg_cursorHints, "cg_cursorHints", "4", CVAR_ARCHIVE },
	{ &cg_hintFadeTime, "cg_hintFadeTime", "500", CVAR_ARCHIVE }, //----(SA)	added
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_zoomDefaultBinoc, "cg_zoomDefaultBinoc", "22.5", CVAR_ARCHIVE },
	{ &cg_zoomDefaultSniper, "cg_zoomDefaultSniper", "15", CVAR_ARCHIVE },
	{ &cg_zoomDefaultSnooper, "cg_zoomDefaultSnooper", "40", CVAR_ARCHIVE },
	{ &cg_zoomDefaultFG, "cg_zoomDefaultFG", "55", CVAR_ARCHIVE },                //----(SA)	added
	{ &cg_zoomSensitivityFovScaled, "cg_zoomSensitivityFovScaled", "1", CVAR_ARCHIVE },
	{ &cg_zoomSensitivity, "cg_zoomSensitivity", "1", CVAR_ARCHIVE },
	{ &cg_zoomStepBinoc, "cg_zoomStepBinoc", "3", CVAR_ARCHIVE },
	{ &cg_zoomStepSniper, "cg_zoomStepSniper", "2", CVAR_ARCHIVE },
	{ &cg_zoomStepSnooper, "cg_zoomStepSnooper", "5", CVAR_ARCHIVE },
	{ &cg_zoomStepFG, "cg_zoomStepFG", "10", CVAR_ARCHIVE },          //----(SA)	added
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },	// NOTE: there is already a dmflag (DF_FIXED_FOV) to allow server control of this cheat
	{ &cg_fixedAspect, "cg_fixedAspect", "2", CVAR_ARCHIVE | CVAR_LATCH }, // Essentially the same as setting DF_FIXED_FOV for widescreen aspects
	{ &cg_fixedAspectFOV, "cg_fixedAspectFOV", "1", CVAR_ARCHIVE },
	{ &cg_oldWolfUI, "cg_oldWolfUI", "0", CVAR_ARCHIVE },
	{ &cg_drawStatusHead, "cg_drawStatusHead", "0", CVAR_ARCHIVE },
	{ &cg_hudWeapIcon, "cg_hudWeapIcon", "1", CVAR_ARCHIVE },
	{ &cg_hudStamina, "cg_hudStamina", "1", CVAR_ARCHIVE },
	{ &cg_hudAmmoClip, "cg_hudAmmoClip", "1", CVAR_ARCHIVE },
	{ &cg_hudStatus, "cg_hudStatus", "4", CVAR_ARCHIVE },
	{ &cg_journalStyle, "cg_journalStyle", "1", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_letterbox, "cg_letterbox", "0", CVAR_TEMP },    //----(SA)	added
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawSubtitles, "cg_drawSubtitles", "0", CVAR_ARCHIVE},
	{ &cg_subtitleSize, "cg_subtitleSize", "7", CVAR_ARCHIVE},
	{ &cg_subtitleShadow, "cg_subtitleShadow", "1", CVAR_ARCHIVE},
	{ &cg_drawSpreadScale, "cg_drawSpreadScale", "1", CVAR_ARCHIVE },
	{ &cg_drawFrags, "cg_drawFrags", "1", CVAR_ARCHIVE },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairBinoc, "cg_drawCrosshairBinoc", "0", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairPickups, "cg_drawCrosshairPickups", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairReticle, "cg_drawCrosshairReticle", "0", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_hudAlpha, "cg_hudAlpha", "1.0", CVAR_ARCHIVE },
	{ &cg_useWeapsForZoom,  "cg_useWeapsForZoom", "1", CVAR_ARCHIVE },
	{ &cg_weaponCycleDelay, "cg_weaponCycleDelay", "150", CVAR_ARCHIVE }, //----(SA)	added
	{ &cg_cycleAllWeaps,    "cg_cycleAllWeaps", "1", CVAR_ARCHIVE },
	{ &cg_drawAllWeaps,     "cg_drawAllWeaps",   "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "48", CVAR_ARCHIVE },
	{ &cg_crosshairAlpha, "cg_crosshairAlpha", "1.0", CVAR_ARCHIVE }, //----(SA)	added
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE }, // was 1250
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_reticles, "cg_reticles", "1", CVAR_CHEAT },
	{ &cg_reticleBrightness, "cg_reticleBrightness", "0.7", CVAR_ARCHIVE },
	{ &cg_markTime, "cg_marktime", "30000", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT  },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT  },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT  },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_buyprinttime, "cg_buyprinttime", "1", CVAR_CHEAT },
	{ &cg_egprinttime, "cg_egprinttime", "7", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup, "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },

	{ &cg_bobbing, "cg_bobbing", "1", CVAR_ARCHIVE },

	{ &cg_drawCheckpoint, "cg_drawCheckpoint", "1", CVAR_ARCHIVE },

	

	// JOSEPH 10-27-99
	{ &cg_autoactivate, "cg_autoactivate", "1", CVAR_ARCHIVE },
	{ &cg_emptyswitch, "cg_emptyswitch", "0", CVAR_ARCHIVE },
	// END JOSEPH

//----(SA)	added
	{ &cg_particleDist, "cg_particleDist", "1024", CVAR_ARCHIVE },
	{ &cg_particleLOD, "cg_particleLOD", "0", CVAR_ARCHIVE },
	{ &cg_useSuggestedWeapons, "cg_useSuggestedWeapons", "1", CVAR_ARCHIVE }, //----(SA)	added
//----(SA)	end

	// Ridah, more fluid rotations
	{ &cg_swingSpeed, "cg_swingSpeed", "0.1", CVAR_CHEAT },   // was 0.3 for Q3
	{ &cg_bloodTime, "cg_bloodTime", "120", CVAR_ARCHIVE },
	{ &cg_hunkUsed, "com_hunkUsed", "0", 0 },
	{ &cg_soundAdjust, "hunk_soundadjust", "0", 0 },

	{ &cg_skybox, "cg_skybox", "1", CVAR_CHEAT },
	// done.

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
	{ &cg_tracerWidth, "cg_tracerwidth", "0.8", CVAR_CHEAT },
	{ &cg_tracerSpeed, "cg_tracerSpeed", "4500", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "160", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "50", CVAR_ARCHIVE },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_coronafardist, "cg_coronafardist", "1536", CVAR_ARCHIVE },
	{ &cg_coronas, "cg_coronas", "1", CVAR_ARCHIVE },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_blinktime, "cg_blinktime", "100", CVAR_ARCHIVE},         //----(SA)	added

	{ &cg_enableBreath, "g_enableBreath", "1", CVAR_SERVERINFO},
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
//	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &cg_hudStyle, "cg_hudStyle", "1", CVAR_ARCHIVE},

	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript, "com_buildScript", "0", 0 },   // force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },

	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},

	// Rafael - particle switch
	{ &cg_wolfparticles, "cg_wolfparticles", "1", CVAR_ARCHIVE },
	// done

	{ &cg_atmosphericEffects,     "cg_atmosphericEffects",     "1",           CVAR_ARCHIVE, }, // RealRTCW
	{ &cg_lowAtmosphericEffects, "cg_lowAtmosphericEffects", "0", CVAR_ARCHIVE },		/// added by Berserker
	{ &cg_forceAtmosphericEffects, "cg_forceAtmosphericEffects", "0", CVAR_LATCH },		/// added by Berserker

	{ &cg_autoReload, "cg_autoReload", "1", CVAR_ARCHIVE },
	{ &cg_uinfo, "cg_uinfo", "0", CVAR_ROM | CVAR_USERINFO },
	{ &int_cl_maxpackets, "cl_maxpackets", "30", CVAR_ARCHIVE },
	{ &int_cl_timenudge, "cl_timenudge", "0", CVAR_ARCHIVE },

	// Ridah
	{ &cg_gameType, "g_gametype", "0", 0 }, // communicated by systeminfo
	{ &cg_newinventory, "g_newinventory", "0", CVAR_ARCHIVE }, // communicated by systeminfo
	{ &cg_norender, "cg_norender", "0", 0 },  // only used during single player, to suppress rendering until the server is ready

	{ &cg_gameSkill, "g_gameskill", "2", 0 }, // communicated by systeminfo	// (SA) new default '2' (was '1')

	{ &cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE },

	{ &cg_ironChallenge, "g_ironchallenge", "0", CVAR_SERVERINFO | CVAR_ROM }, 
	{ &cg_nohudChallenge, "g_nohudchallenge", "0", CVAR_SERVERINFO | CVAR_ROM }, 
	{ &cg_nopickupChallenge, "g_nopickupchallenge", "0", CVAR_SERVERINFO | CVAR_ROM }, 
	{ &cg_decayChallenge, "g_decaychallenge", "0", CVAR_SERVERINFO | CVAR_ROM }, 

	{ &cg_vanilla_guns, "g_vanilla_guns", "0", CVAR_ARCHIVE }, 

	{ &cg_reloading, "g_reloading", "0", 0 }, //----(SA)	added

	{ &cg_jumptime, "g_jumptime", "0", 0 }, //----(SA)	added
	

	// JPW NERVE
	{ &cg_medicChargeTime,  "g_medicChargeTime", "20000", 0 }, // communicated by systeminfo
	{ &cg_LTChargeTime, "g_LTChargeTime", "35000", 0 }, // communicated by systeminfo
	{ &cg_engineerChargeTime,   "g_engineerChargeTime", "30000", 0 }, // communicated by systeminfo
	{ &cg_soldierChargeTime,    "g_soldierChargeTime", "20000", 0 }, // communicated by systeminfo
	{ &cg_bluelimbotime,        "g_bluelimbotime", "30000", 0 }, // communicated by systeminfo
	{ &cg_redlimbotime,         "g_redlimbotime", "30000", 0 }, // communicated by systeminfo
	// jpw

	{ &cg_notebook, "cl_notebook", "0", CVAR_ROM },
	{ &cg_notebookpages, "cg_notebookpages", "0", CVAR_ROM},
//	{ &cg_youGotMail, "cg_youGotMail", "0", CVAR_ROM},	// used to display notebook new-info icon
	{ &cg_youGotMail, "cg_youGotMail", "0", 0},   // used to display notebook new-info icon

	{ &cg_animState, "cg_animState", "0", CVAR_CHEAT},
	{ &cg_missionStats, "g_missionStats", "0", CVAR_ROM},
	{ &cg_waitForFire, "cl_waitForFire", "0", CVAR_ROM},

	{ &cg_loadWeaponSelect, "cg_loadWeaponSelect", "0", CVAR_ROM},

	{ &cg_expectedhunkusage, "com_expectedhunkusage", "0", CVAR_ROM},

	// NERVE - SMF
	{ &mp_playerType, "mp_playerType", "0", 0 },
	{ &mp_team, "mp_team", "0", 0 },
	{ &mp_weapon, "mp_weapon", "0", 0 },
	{ &mp_pistol, "mp_pistol", "0", 0 },
	{ &mp_item1, "mp_item1", "0", 0 },

	{ &mp_item2, "mp_item2", "0", 0 },
	{ &mp_mapDesc, "mp_mapDesc", "", 0 },
	{ &mp_mapTitle, "mp_mapTitle", "", 0 },
	{ &mp_itemDesc, "mp_itemDesc", "", 0 },
	// -NERVE - SMF

	{ &cg_showAIState, "cg_showAIState", "0", CVAR_CHEAT},

	{ &cg_solidCrosshair, "cg_solidCrosshair", "0", CVAR_ARCHIVE },
	{ &cg_bloodBlend, "cg_bloodBlend", "1", CVAR_ARCHIVE },
	{ &cg_snipersCrosshair, "cg_snipersCrosshair", "1", CVAR_ARCHIVE },

	{ &cg_bodysink, "g_bodysink", "0", CVAR_ARCHIVE },
	
	{ &cg_gunPosLock, "cg_gunposlock", "1", CVAR_ARCHIVE},

	{ &cg_realism, "g_realism", "0", CVAR_ARCHIVE},

	{ &cg_weaponBounceSound, "cg_weaponBounceSound", "1",CVAR_ARCHIVE	},
};
int cvarTableSize = ARRAY_LEN( cvarTable );
void CG_setClientFlags( void );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int i;
	cvarTable_t *cv;
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_Set( "cg_letterbox", "0" ); // force this for people who might have it in their

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
							cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register( NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "head", DEFAULT_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );


}

/*
===================
CG_ForceModelChange
===================
*/
/*
static void CG_ForceModelChange( void ) {
	int	i;

	for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
		const char      *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}
*/

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int i;
	qboolean fSetFlags = qfalse;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
		if ( cv->vmCvar == &cg_autoReload || cv->vmCvar == &int_cl_timenudge || cv->vmCvar == &int_cl_maxpackets ) {
		fSetFlags = qtrue;
	}

/* RF, disabled this, not needed anymore
	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
*/

}
	// Send any relevent updates
	if ( fSetFlags ) {
		CG_setClientFlags();
	}
}

 void CG_setClientFlags( void ) {
	if ( cg.demoPlayback ) {
		return;
	}

 	cg.pmext.bAutoReload = ( cg_autoReload.integer > 0 );
	trap_Cvar_Set( "cg_uinfo", va( "%d %d %d",
								   // Client Flags
								   (
									   ( ( cg_autoReload.integer > 0 ) ? CGF_AUTORELOAD : 0 ) 
									   // Add more in here, as needed
								   ),

 								   // Timenudge
								   int_cl_timenudge.integer,
								   // MaxPackets
								   int_cl_maxpackets.integer
								   ) );
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

void QDECL CG_Printf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================
void CG_SetupDlightstyles( void ) {
	int i, j;
	char        *str;
	char        *token;
	int entnum;
	centity_t   *cent;

	cg.lightstylesInited = qtrue;

	for ( i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; i++ )
	{
		str = (char *) CG_ConfigString( CS_DLIGHTS + i );
		if ( !strlen( str ) ) {
			break;
		}

		token = COM_Parse( &str );   // ent num
		entnum = atoi( token );
		cent = &cg_entities[entnum];

		token = COM_Parse( &str );   // stylestring
		Q_strncpyz( cent->dl_stylestring, token, strlen( token ) );

		token = COM_Parse( &str );   // offset
		cent->dl_frame      = atoi( token );
		cent->dl_oldframe   = cent->dl_frame - 1;
		if ( cent->dl_oldframe < 0 ) {
			cent->dl_oldframe = strlen( cent->dl_stylestring );
		}

		token = COM_Parse( &str );   // sound id
		cent->dl_sound = atoi( token );

		token = COM_Parse( &str );   // attenuation
		cent->dl_atten = atoi( token );

		for ( j = 0; j < strlen( cent->dl_stylestring ); j++ ) {

			cent->dl_stylestring[j] += cent->dl_atten;  // adjust character for attenuation/amplification

			// clamp result
			if ( cent->dl_stylestring[j] < 'a' ) {
				cent->dl_stylestring[j] = 'a';
			}
			if ( cent->dl_stylestring[j] > 'z' ) {
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp   = 0.0;
		cent->dl_time       = cg.time;
	}

}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t         *item;
	char data[MAX_QPATH];
	char            *s, *start;
	int len;

	item = &bg_itemlist[ itemNum ];

	if ( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if ( !s || !s[0] ) {
		return;
	}

	while ( *s ) {
		start = s;
		while ( *s && *s != ' ' ) {
			s++;
		}

		len = s - start;
		if ( len >= MAX_QPATH || len < 5 ) {
			CG_Error( "PrecacheItem: %s has bad precache string",
					  item->classname );
			return;
		}
		memcpy( data, start, len );
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp( data + len - 3, "wav" ) ) {
			trap_S_RegisterSound( data );
		}
	}
}


//----(SA)	added

// this is the only thing that sets a cap on # items.  would like it to be adaptable.
// (rather than 256 max items with pickup name fixed at 32 chars)

/*
==============
CG_LoadPickupNames
==============
*/
#define MAX_BUFFER          20000
static void CG_LoadPickupNames( void ) {
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i;
	char *token;

	Com_sprintf( filename, MAX_QPATH, "text/pickupnames.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_RED "WARNING: pickup name file (pickupnames.txt not found in main/text)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}

	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;

	for ( i = 0; i < bg_numItems; i++ ) {
		token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
		if ( !Q_stricmp( token, "---" ) ) {   // no name.  use hardcoded value
			if ( bg_itemlist[i].pickup_name && strlen( bg_itemlist[i].pickup_name ) ) {
				Com_sprintf( cgs.itemPrintNames[i], MAX_QPATH, "%s", bg_itemlist[ i ].pickup_name );
			} else {
				cgs.itemPrintNames[i][0] = 0;
			}
		} else {
			Com_sprintf( cgs.itemPrintNames[i], MAX_QPATH, "%s", token );
		}
	}
}

// a straight dupe right now so I don't mess anything up while adding this
static void CG_LoadTranslationStrings( void ) {
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i, numStrings;
	char *token;

	Com_sprintf( filename, MAX_QPATH, "text/strings.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_RED "WARNING: string translation file (strings.txt not found in main/text)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}

	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;

	numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
#ifdef Q3_VM // new IORTCW syscall (works for qvms and dlls), but have dlls use vanilla rtcw compatible code
		translateStrings[i].localname = (char *)trap_Alloc( strlen( token ) + 1 );
#else
		translateStrings[i].localname = (char *)malloc( strlen( token ) + 1 );
#endif
		strcpy( translateStrings[i].localname, token );
	}
}

// a straight dupe right now so I don't mess anything up while adding this
static void CG_LoadbonusStrings( void ) {
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i, numStrings;
	char *token;

	Com_sprintf( filename, MAX_QPATH, "text/bonus_strings.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_RED "WARNING: string translation file (bonus_strings.txt not found in main/text)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}

	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;

	numStrings = sizeof( bonusStrings ) / sizeof( bonusStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
#ifdef Q3_VM // new IORTCW syscall (works for qvms and dlls), but have dlls use vanilla rtcw compatible code
		bonusStrings[i].localname = (char *)trap_Alloc( strlen( token ) + 1 );
#else
		bonusStrings[i].localname = (char *)malloc( strlen( token ) + 1 );
#endif
		strcpy( bonusStrings[i].localname, token );
	}
}

static void CG_LoadTranslationTextStrings(const char *file) {
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i;
	char *token;

	Com_sprintf(filename, MAX_QPATH, "%s", file); //karin: error: format string is not a string literal (potentially insecure) [-Werror,-Wformat-security]
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0) {
		CG_Printf(S_COLOR_RED "WARNING: string translation file (main/%s)\n", filename);
		return;
	}
	if (len > MAX_BUFFER) {
		CG_Printf("%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER);
	}

	// load the file into memory
	trap_FS_Read(buffer, len, f);
	buffer[len] = 0;
	trap_FS_FCloseFile(f);
	// parse the list
	text = buffer;
	token = COM_ParseExt(&text, qtrue);
	if (token[0] != '{') {
		CG_Printf("^1WARNING: expecting '{', found '%s' instead in translation file \"text/translate.txt\"\n", token);
		return;
	}
	i = 0;
	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (!token[0]) {
			CG_Printf("^1WARNING: no concluding '}' in translation file \"text/translate.txt\"\n");
			break;
		}
		// end of shader definition
		if (token[0] == '}') {
			break;
		}
		translateTextStrings[i].stringname = malloc(strlen(token) + 1);
		strcpy(translateTextStrings[i].stringname, token);
		token = COM_ParseExt(&text, qfalse);
		translateTextStrings[i].stringtext = malloc(strlen(token) + 1);
		strcpy(translateTextStrings[i].stringtext, token);
		i++;
	}
}

static void CG_LoadIgnoredTranslationTextStrings() {

	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i;
	char *token;

	Com_sprintf( filename, MAX_QPATH, "text/ignoredstitles.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_RED "WARNING: ignored name file (ignoredstitles.txt not found in main/text)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}

	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;

	for ( i = 0; i < 255; i++ ) {
		token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
		//CG_Printf("ignored text: %s\n", token);
		Com_sprintf( cgs.ignoredSubtitles[i], MAX_QPATH, "%s", token );
	}
}

static void CG_LoadTranslateStrings( void ) {
	const char  *info;
	char    *mapname;

	info = CG_ConfigString( CS_SERVERINFO );
	mapname = Info_ValueForKey( info, "mapname" );

	CG_LoadPickupNames();
	CG_LoadTranslationStrings();    // right now just centerprint
	CG_LoadbonusStrings();
	CG_LoadTranslationTextStrings(va("text/EnglishUSA/maps/%s.txt", mapname));
	CG_LoadIgnoredTranslationTextStrings();
}

////////
/// Added by Eugeny Panikarowsky
////////
const char *CG_translateTextString(const char *str) {
	int i, numStrings;

    numStrings = sizeof(cgs.ignoredSubtitles) / sizeof(cgs.ignoredSubtitles[0]) - 1;
    for (i = 0; i < numStrings; i++) {
        if (!strcmp(str, cgs.ignoredSubtitles[i])) {
            // Return a special string to indicate an ignored subtitle
            return "IGNORED_SUBTITLE";
        }
    }
	numStrings = sizeof(translateTextStrings) / sizeof(translateTextStrings[0]) - 1;
	i = 0;
	
	for (i = 0; i < numStrings; i++) {
		if (!translateTextStrings[i].stringname || !strlen(translateTextStrings[i].stringname)) {
			return str;
		}
		if (!strcmp(str, translateTextStrings[i].stringname)) {
			if (translateTextStrings[i].stringtext && strlen(translateTextStrings[i].stringtext)) {
				return translateTextStrings[i].stringtext;
			}
			break;
		}
	}
	return str;
}

const char *CG_translateTextString2(const char *str) {
	int i, numStrings;

	numStrings = sizeof(cgs.ignoredSubtitles) / sizeof(cgs.ignoredSubtitles[0]) - 1;
	for (i = 0; i < numStrings; i++) {
		if (!strcmp(str, cgs.ignoredSubtitles[i])) {
			return "";
		}
	}
	numStrings = sizeof(translateTextStrings) / sizeof(translateTextStrings[0]) - 1;
	i = 0;
	
	for (i = 0; i < numStrings; i++) {
		if (!translateTextStrings[i].stringname || !strlen(translateTextStrings[i].stringname)) {
			return str;
		}
		if (!strcmp(str, translateTextStrings[i].stringname)) {
			if (translateTextStrings[i].stringtext && strlen(translateTextStrings[i].stringtext)) {
				return translateTextStrings[i].stringtext;
			}
			break;
		}
	}
	return str;
}
//----(SA)	end


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int i;
	char items[MAX_ITEMS + 1];
	char name[MAX_QPATH];
	const char  *soundName;

	// Ridah, init sound scripts
	CG_SoundInit();
	// done.

	cgs.media.n_health = trap_S_RegisterSound( "sound/items/n_health.wav" );
	cgs.media.noFireUnderwater = trap_S_RegisterSound( "sound/weapons/underwaterfire.wav" ); 

	cgs.media.snipersound = trap_S_RegisterSound( "sound/weapons/mauser/mauserf1.wav" );
	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav" );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav" );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav" );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav" );
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav" );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav" );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav" );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav" );

	cgs.media.headShot = trap_S_RegisterSound( "sound/hitsounds/hithead.wav" );
	cgs.media.bodyShot = trap_S_RegisterSound( "sound/hitsounds/hit.wav" );
	cgs.media.teamShot = trap_S_RegisterSound( "sound/hitsounds/hitteam.wav" );

	cgs.media.grenadebounce[GRENBOUNCE_DEFAULT][0]  = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb1a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_DEFAULT][1]  = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb2a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_DIRT][0]     = trap_S_RegisterSound( "sound/weapons/grenade/hg_dirt1a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_DIRT][1]     = trap_S_RegisterSound( "sound/weapons/grenade/hg_dirt2a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_WOOD][0]     = trap_S_RegisterSound( "sound/weapons/grenade/hg_wood1a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_WOOD][1]     = trap_S_RegisterSound( "sound/weapons/grenade/hg_wood2a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_METAL][0]    = trap_S_RegisterSound( "sound/weapons/grenade/hg_metal1a.wav" );
	cgs.media.grenadebounce[GRENBOUNCE_METAL][1]    = trap_S_RegisterSound( "sound/weapons/grenade/hg_metal2a.wav" );

	cgs.media.dynamitebounce1 = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_bounce.wav" );

	cgs.media.fbarrelexp1 = trap_S_RegisterSound( "sound/weapons/flamebarrel/fbarrela.wav" );
	cgs.media.fbarrelexp2 = trap_S_RegisterSound( "sound/weapons/flamebarrel/fbarrelb.wav" );

	cgs.media.fkickwall = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav" );
	cgs.media.fkickflesh = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav" );
	cgs.media.fkickmiss = trap_S_RegisterSound( "sound/weapons/melee/fstmiss.wav" );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav" );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav" );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav" );

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav" );
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav" );
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav" );

	cgs.media.underWaterSound = trap_S_RegisterSound( "sound/world/underwater03.wav" );

	cgs.media.poisonGasCough = trap_S_RegisterSound( "sound/weapons/gasgrenade/cough.wav");
	cgs.media.knifeThrow = trap_S_RegisterSound( "sound/weapons/knife/knife_throw.wav");

	for ( i = 0 ; i < 4 ; i++ ) {
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/boot%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/mech%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/energy%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound( name );


		// (SA) Wolf footstep sound registration
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/wood%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/grass%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/gravel%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/roof%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_ROOF][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/snow%i.wav", i + 1 );
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/carpet%i.wav", i + 1 );    //----(SA)
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound( name );
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS + i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;   // custom sound
		}

		// Ridah, register sound scripts seperately
		if ( !strstr( soundName, ".wav" ) ) {
			cgs.gameSounds[i] = CG_SoundScriptPrecache( soundName );    //----(SA)	shouldn't this be okay?  The cs index is reserved anyway, so it can't hurt, right?
			cgs.gameSoundTypes[i] = 2;
		} else {
			cgs.gameSounds[i] = trap_S_RegisterSound( soundName );
			cgs.gameSoundTypes[i] = 1;
		}
	}

	cgs.media.grenadePulseSound4 = trap_S_RegisterSound( "sound/weapons/grenade/grenpulse4.wav" );
	cgs.media.grenadePulseSound3 = trap_S_RegisterSound( "sound/weapons/grenade/grenpulse3.wav" );
	cgs.media.grenadePulseSound2 = trap_S_RegisterSound( "sound/weapons/grenade/grenpulse2.wav" );
	cgs.media.grenadePulseSound1 = trap_S_RegisterSound( "sound/weapons/grenade/grenpulse1.wav" );

	cgs.media.quickgrenSound = trap_S_RegisterSound( "sound/weapons/grenade/grenlf1a.wav" );

	cgs.media.debBounce1Sound = trap_S_RegisterSound( "sound/world/block.wav" );
	cgs.media.debBounce2Sound = trap_S_RegisterSound( "sound/world/brick.wav" );
	cgs.media.debBounce3Sound = trap_S_RegisterSound( "sound/world/brick2.wav" );

	cgs.media.flameSound = trap_S_RegisterSound( "sound/weapons/flamethrower/fl_fire.wav" );
	cgs.media.flameBlowSound = trap_S_RegisterSound( "sound/weapons/flamethrower/fl_blow.wav" );
	cgs.media.flameStartSound = trap_S_RegisterSound( "sound/weapons/flamethrower/fl_start.wav" );
	cgs.media.flameStreamSound = trap_S_RegisterSound( "sound/weapons/flamethrower/fl_stream.wav" );
	cgs.media.flameCrackSound = trap_S_RegisterSound( "sound/world/firecrack1.wav" );
	cgs.media.boneBounceSound = trap_S_RegisterSound( "sound/world/bonebounce.wav" );    // TODO: need a real sound for this

	cgs.media.lightningSounds[0] = trap_S_RegisterSound( "sound/world/electzap1.wav" );
	cgs.media.lightningSounds[1] = trap_S_RegisterSound( "sound/world/electzap2.wav" );
	cgs.media.lightningSounds[2] = trap_S_RegisterSound( "sound/world/electzap3.wav" );
	cgs.media.lightningZap = trap_S_RegisterSound( "sound/world/electrocute.wav" );

	// precache sound scripts that get called from the cgame
	cgs.media.bulletHitFleshScript = CG_SoundScriptPrecache( "bulletHitFlesh" );
	cgs.media.bulletHitFleshMetalScript = CG_SoundScriptPrecache( "bulletHitFleshMetal" );

	cgs.media.teslaZapScript = CG_SoundScriptPrecache( "teslaZap" );
	cgs.media.crossZapScript = CG_SoundScriptPrecache( "crossZap" );
	cgs.media.teslaLoopSound = trap_S_RegisterSound( "sound/weapons/tesla/loop.wav" );

	cgs.media.batsFlyingLoopSound = trap_S_RegisterSound( "sound/world/bats_flying.wav" );

	cgs.media.elecSound = trap_S_RegisterSound( "sound/items/use_elec.wav" );
	cgs.media.fireSound = trap_S_RegisterSound( "sound/items/use_fire.wav" );
	cgs.media.waterSound = trap_S_RegisterSound( "sound/items/use_water.wav" );
	cgs.media.wineSound = trap_S_RegisterSound( "sound/pickup/holdable/use_wine.wav" );       
	cgs.media.bookSound = trap_S_RegisterSound( "sound/pickup/holdable/use_book.wav" );       
	cgs.media.adrenalineSound = trap_S_RegisterSound( "sound/pickup/holdable/use_adrenaline.wav" ); 
	cgs.media.bandagesSound = trap_S_RegisterSound( "sound/pickup/holdable/use_bandages.wav" ); 
	cgs.media.quadSound = trap_S_RegisterSound( "sound/items/damage3.wav" );
	cgs.media.sfx_ric1 = trap_S_RegisterSound( "sound/weapons/machinegun/ric1.wav" );
	cgs.media.sfx_ric2 = trap_S_RegisterSound( "sound/weapons/machinegun/ric2.wav" );
	cgs.media.sfx_ric3 = trap_S_RegisterSound( "sound/weapons/machinegun/ric3.wav" );

	// Explosion sounds
	// Rocket
	cgs.media.sfx_rockexp = trap_S_RegisterSound( "sound/weapons/rocket/rocklx1a.wav" );
	cgs.media.sfx_rockexpDist = trap_S_RegisterSound( "sound/weapons/rocket/rocket_explode_far.wav" );
    // Grenades
	cgs.media.sfx_grenexp = trap_S_RegisterSound( "sound/weapons/grenade/grenade_explode.wav" );
    cgs.media.sfx_grenexpDist = trap_S_RegisterSound( "sound/weapons/grenade/grenade_explode_far.wav" );
	cgs.media.sfx_grenexpWater =    trap_S_RegisterSound( "sound/weapons/grenade/grenade_explode_water.wav");
	// Dynamite
	cgs.media.sfx_dynamiteexp = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_exp.wav" );
	cgs.media.sfx_dynamiteexpDist = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_exp_dist.wav" );   
	// Barrel
	cgs.media.sfx_barrelexp = trap_S_RegisterSound( "sound/world/barrel_exp.wav" );


	cgs.media.sfx_knifehit[0] = trap_S_RegisterSound( "sound/weapons/knife/knife_hit1.wav" ); 
	cgs.media.sfx_knifehit[1] = trap_S_RegisterSound( "sound/weapons/knife/knife_hit2.wav" );
	cgs.media.sfx_knifehit[2] = trap_S_RegisterSound( "sound/weapons/knife/knife_hit3.wav" );
	cgs.media.sfx_knifehit[3] = trap_S_RegisterSound( "sound/weapons/knife/knife_hit4.wav" );

	cgs.media.sfx_knifehit[4] = trap_S_RegisterSound( "sound/weapons/knife/knife_hitwall1.wav" ); 

	cgs.media.sfx_bullet_metalhit[0] = trap_S_RegisterSound( "sound/weapons/bullethit_metal1.wav" );
	cgs.media.sfx_bullet_metalhit[1] = trap_S_RegisterSound( "sound/weapons/bullethit_metal2.wav" );
	cgs.media.sfx_bullet_metalhit[2] = trap_S_RegisterSound( "sound/weapons/bullethit_metal3.wav" );

	cgs.media.sfx_bullet_woodhit[0] = trap_S_RegisterSound( "sound/weapons/bullethit_wood1.wav" );
	cgs.media.sfx_bullet_woodhit[1] = trap_S_RegisterSound( "sound/weapons/bullethit_wood2.wav" );
	cgs.media.sfx_bullet_woodhit[2] = trap_S_RegisterSound( "sound/weapons/bullethit_wood3.wav" );

	cgs.media.sfx_bullet_roofhit[0] = trap_S_RegisterSound( "sound/weapons/bullethit_roof1.wav" );
	cgs.media.sfx_bullet_roofhit[1] = trap_S_RegisterSound( "sound/weapons/bullethit_roof2.wav" );
	cgs.media.sfx_bullet_roofhit[2] = trap_S_RegisterSound( "sound/weapons/bullethit_roof3.wav" );

	cgs.media.sfx_bullet_ceramichit[0] = trap_S_RegisterSound( "sound/weapons/bullethit_ceramic1.wav" );
	cgs.media.sfx_bullet_ceramichit[1] = trap_S_RegisterSound( "sound/weapons/bullethit_ceramic2.wav" );
	cgs.media.sfx_bullet_ceramichit[2] = trap_S_RegisterSound( "sound/weapons/bullethit_ceramic3.wav" );

	cgs.media.sfx_bullet_glasshit[0] = trap_S_RegisterSound( "sound/weapons/bullethit_glass1.wav" );
	cgs.media.sfx_bullet_glasshit[1] = trap_S_RegisterSound( "sound/weapons/bullethit_glass2.wav" );
	cgs.media.sfx_bullet_glasshit[2] = trap_S_RegisterSound( "sound/weapons/bullethit_glass3.wav" );

	// RealRTCW brass sound
	cgs.media.sfx_brassSound_metal[0] = trap_S_RegisterSound("sound/weapons/misc/shell_metal1.wav" );
	cgs.media.sfx_brassSound_metal[1] = trap_S_RegisterSound("sound/weapons/misc/shell_metal2.wav" );
	cgs.media.sfx_brassSound_metal[2] = trap_S_RegisterSound("sound/weapons/misc/shell_metal3.wav" );

	cgs.media.sfx_brassSound_soft[0] = trap_S_RegisterSound("sound/weapons/misc/shell_soft1.wav" );
	cgs.media.sfx_brassSound_soft[1] = trap_S_RegisterSound("sound/weapons/misc/shell_soft2.wav" );
	cgs.media.sfx_brassSound_soft[2] = trap_S_RegisterSound("sound/weapons/misc/shell_soft3.wav" );

	cgs.media.sfx_brassSound_stone[0] = trap_S_RegisterSound("sound/weapons/misc/shell_stone1.wav" );
	cgs.media.sfx_brassSound_stone[1] = trap_S_RegisterSound("sound/weapons/misc/shell_stone2.wav" );
	cgs.media.sfx_brassSound_stone[2] = trap_S_RegisterSound("sound/weapons/misc/shell_stone3.wav" );

	cgs.media.sfx_brassSound_wood[0] = trap_S_RegisterSound("sound/weapons/misc/shell_wood1.wav" );
	cgs.media.sfx_brassSound_wood[1] = trap_S_RegisterSound("sound/weapons/misc/shell_wood2.wav" );
	cgs.media.sfx_brassSound_wood[2] = trap_S_RegisterSound("sound/weapons/misc/shell_wood3.wav" );
	
	cgs.media.sfx_rubbleBounce[i]                    = trap_S_RegisterSound("sound/world/debris%i.wav" );

	cgs.media.sparkSounds[0] = trap_S_RegisterSound( "sound/world/saarc2.wav" );
	cgs.media.sparkSounds[1] = trap_S_RegisterSound( "sound/world/arc2.wav" );

	trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav" );
	trap_S_RegisterSound( "sound/weapons/melee/fstmiss.wav" );

	trap_S_RegisterSound( "sound/Loogie/spit.wav" );
	trap_S_RegisterSound( "sound/Loogie/sizzle.wav" );
}


//===================================================================================



/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	char name[1024];

	int i;
	char items[MAX_ITEMS + 1];
	static char     *sb_nums[11] = {
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

	// precache status bar pics
	CG_LoadingString( "game media" );

	CG_LoadingString( " - textures" );

	for ( i = 0 ; i < 11 ; i++ ) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );

	// Rafael - blood pool
	//cgs.media.bloodPool = trap_R_RegisterShader ("bloodPool");

	// RF, blood cloud
	cgs.media.bloodCloudShader = trap_R_RegisterShader( "bloodCloud" );

	// Rafael - cannon
	cgs.media.smokePuffShaderdirty = trap_R_RegisterShader( "smokePuffdirty" );
	cgs.media.smokePuffShaderb1 = trap_R_RegisterShader( "smokePuffblack1" );
	cgs.media.smokePuffShaderb2 = trap_R_RegisterShader( "smokePuffblack2" );
	cgs.media.smokePuffShaderb3 = trap_R_RegisterShader( "smokePuffblack3" );
	cgs.media.smokePuffShaderb4 = trap_R_RegisterShader( "smokePuffblack4" );
	cgs.media.smokePuffShaderb5 = trap_R_RegisterShader( "smokePuffblack5" );
	// done

	// Rafael - bleedanim
	for ( i = 0; i < 5; i++ ) {
		cgs.media.viewBloodAni[i] = trap_R_RegisterShader( va( "viewBloodBlend%i", i + 1 ) );
	}
	cgs.media.viewFlashBlood = trap_R_RegisterShader( "viewFlashBlood" );
	for ( i = 0; i < 16; i++ ) {
		cgs.media.viewFlashFire[i] = trap_R_RegisterShader( va( "viewFlashFire%i", i + 1 ) );
	}
	// done

	// Rafael bats
	for ( i = 0; i < 10; i++ ) {
		cgs.media.bats[i] = trap_R_RegisterShader( va( "bats%i",i + 1 ) );
	}
	// done

	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );

	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = trap_R_RegisterShader( "lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );

//	cgs.media.reticleShaderSimple = trap_R_RegisterShader( "gfx/misc/reticlesimple" );		// TODO: remove
	cgs.media.reticleShaderSimpleQ = trap_R_RegisterShader( "gfx/misc/reticlesimple_quarter" );
	cgs.media.snooperShaderSimple = trap_R_RegisterShader( "gfx/misc/snoopersimple" );

//	cgs.media.binocShaderSimple = trap_R_RegisterShader( "gfx/misc/binocsimple" );			// TODO: remove
	cgs.media.binocShaderSimpleQ = trap_R_RegisterShader( "gfx/misc/binocsimple_quarter" );  //----(SA)	added

	// Rafael
	// cgs.media.snowShader = trap_R_RegisterShader ( "snowPuff" );
	cgs.media.snowShader = trap_R_RegisterShader( "snow_tri" );

	cgs.media.oilParticle = trap_R_RegisterShader( "oilParticle" );
	cgs.media.oilSlick = trap_R_RegisterShader( "oilSlick" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	cgs.media.hintShaders[HINT_NOEXIT]              = cgs.media.hintShaders[HINT_EXIT];
	cgs.media.hintShaders[HINT_EXIT_FAR]            = cgs.media.hintShaders[HINT_EXIT];
	cgs.media.hintShaders[HINT_NOEXIT_FAR]          = cgs.media.hintShaders[HINT_EXIT];  

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShaderNoMip( va( "gfx/2d/crosshair%c", 'a' + i ) );
	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/enviro" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
//	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );
	cgs.media.redQuadShader = trap_R_RegisterShader("powerups/vampire" );

	CG_LoadingString( " - models" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons/shells/m_shell.md3" );
	cgs.media.panzerfaustBrassModel = trap_R_RegisterModel( "models/weapons/shells/pf_shell.md3" );
	cgs.media.smallgunBrassModel = trap_R_RegisterModel( "models/weapons/shells/sm_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons/shells/sh_shell.md3" );

	//----(SA) wolf debris
	cgs.media.debBlock[0] = trap_R_RegisterModel( "models/mapobjects/debris/brick1.md3" );
	cgs.media.debBlock[1] = trap_R_RegisterModel( "models/mapobjects/debris/brick2.md3" );
	cgs.media.debBlock[2] = trap_R_RegisterModel( "models/mapobjects/debris/brick3.md3" );
	cgs.media.debBlock[3] = trap_R_RegisterModel( "models/mapobjects/debris/brick4.md3" );
	cgs.media.debBlock[4] = trap_R_RegisterModel( "models/mapobjects/debris/brick5.md3" );
	cgs.media.debBlock[5] = trap_R_RegisterModel( "models/mapobjects/debris/brick6.md3" );

	cgs.media.debRock[0] = trap_R_RegisterModel( "models/mapobjects/debris/rubble1.md3" );
	cgs.media.debRock[1] = trap_R_RegisterModel( "models/mapobjects/debris/rubble2.md3" );
	cgs.media.debRock[2] = trap_R_RegisterModel( "models/mapobjects/debris/rubble3.md3" );


	cgs.media.debWood[0] = trap_R_RegisterModel( "models/gibs/wood/wood1.md3" );
	cgs.media.debWood[1] = trap_R_RegisterModel( "models/gibs/wood/wood2.md3" );
	cgs.media.debWood[2] = trap_R_RegisterModel( "models/gibs/wood/wood3.md3" );
	cgs.media.debWood[3] = trap_R_RegisterModel( "models/gibs/wood/wood4.md3" );
	cgs.media.debWood[4] = trap_R_RegisterModel( "models/gibs/wood/wood5.md3" );
	cgs.media.debWood[5] = trap_R_RegisterModel( "models/gibs/wood/wood6.md3" );

	cgs.media.debFabric[0] = trap_R_RegisterModel( "models/shards/fabric1.md3" );
	cgs.media.debFabric[1] = trap_R_RegisterModel( "models/shards/fabric2.md3" );
	cgs.media.debFabric[2] = trap_R_RegisterModel( "models/shards/fabric3.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	for ( i = 0; i < MAX_AISTATES; i++ ) {
		cgs.media.aiStateShaders[i] = trap_R_RegisterShader( va( "sprites/aistate%i", i + 1 ) );
	}

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.sparkParticleShader = trap_R_RegisterShader( "sparkParticle" );
	cgs.media.smokeTrailShader = trap_R_RegisterShader( "smokeTrail" );
	cgs.media.lightningBoltShader = trap_R_RegisterShader( "lightningBolt" );
	cgs.media.flamethrowerFireStream = trap_R_RegisterShader( "flamethrowerFireStream" );
	cgs.media.flamethrowerBlueStream = trap_R_RegisterShader( "flamethrowerBlueStream" );
	cgs.media.onFireShader2 = trap_R_RegisterShader( "entityOnFire1" );
	cgs.media.onFireShader = trap_R_RegisterShader( "entityOnFire2" );
	cgs.media.viewFadeBlack = trap_R_RegisterShader( "viewFadeBlack" );
	cgs.media.sparkFlareShader = trap_R_RegisterShader( "sparkFlareParticle" );

	// spotlight
	// shaders
	cgs.media.spotLightShader = trap_R_RegisterShader( "spotLight" );
	cgs.media.spotLightBeamShader = trap_R_RegisterShader( "lightBeam" );

	// models
	cgs.media.spotLightBaseModel = trap_R_RegisterModel( "models/mapobjects/light/searchlight1_b.md3" );
	cgs.media.spotLightLightModel = trap_R_RegisterModel( "models/mapobjects/light/searchlight1_l.md3" );
	cgs.media.spotLightLightModelBroke = trap_R_RegisterModel( "models/mapobjects/light/searchlight_l_broke.md3" );
	// end spotlight

	cgs.media.lightningHitWallShader = trap_R_RegisterShader( "lightningHitWall" );
	cgs.media.lightningWaveShader = trap_R_RegisterShader( "lightningWave" );
	cgs.media.bulletParticleTrailShader = trap_R_RegisterShader( "bulletParticleTrail" );
	cgs.media.smokeParticleShader = trap_R_RegisterShader( "smokeParticle" );

	// DHM - Nerve :: bullet hitting dirt
	cgs.media.dirtParticle1Shader = trap_R_RegisterShader( "dirt_splash" );
	cgs.media.dirtParticle2Shader = trap_R_RegisterShader( "water_splash" );
	//cgs.media.dirtParticle3Shader = trap_R_RegisterShader( "dirtParticle3" );

	cgs.media.teslaDamageEffectShader = trap_R_RegisterShader( "teslaDamageEffect" );
	cgs.media.teslaAltDamageEffectShader = trap_R_RegisterShader( "teslaAltDamageEffect" );
	cgs.media.viewTeslaDamageEffectShader = trap_R_RegisterShader( "viewTeslaDamageEffect" );
	cgs.media.viewTeslaAltDamageEffectShader = trap_R_RegisterShader( "viewTeslaAltDamageEffect" );
	// done.

	cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );  // (SA) for debugging server traces

	cgs.media.thirdPersonBinocModel = trap_R_RegisterModel( "models/powerups/holdable/binocs_thirdperson.md3" ); //----(SA)	added
	cgs.media.cigModel = trap_R_RegisterModel( "models/players/infantryss/acc/cig.md3" );    //----(SA)	added

	// RF, not used anymore
	//cgs.media.targetEffectExplosionShader	= trap_R_RegisterShader( "targetEffectExplode" );
	//cgs.media.rocketExplosionShader			= trap_R_RegisterShader( "rocketExplosion" );
	//cgs.media.grenadeExplosionShader		= trap_R_RegisterShader( "grenadeExplosion" );

	// zombie shot
	//cgs.media.zombieLoogie = trap_R_RegisterModel( "models/mapobjects/bodyparts/zom_loog.md3" );
	cgs.media.flamebarrel = trap_R_RegisterModel( "models/furniture/barrel/barrel_a.md3" );
	//----(SA) end

	cgs.media.mg42muzzleflash = trap_R_RegisterModel( "models/weapons2/machinegun/mg42_flash.md3" );
	// cgs.media.mg42muzzleflashgg = trap_R_RegisterModel ("models/weapons2/machinegun/mg42_flash_gg.md3" );

	cgs.media.planemuzzleflash = trap_R_RegisterModel( "models/mapobjects/vehicles/gunflare.md3" );

	// Rafael shards
	cgs.media.shardGlass1 = trap_R_RegisterModel( "models/shards/glass1.md3" );
	cgs.media.shardGlass2 = trap_R_RegisterModel( "models/shards/glass2.md3" );
	cgs.media.shardWood1 = trap_R_RegisterModel( "models/shards/wood1.md3" );
	cgs.media.shardWood2 = trap_R_RegisterModel( "models/shards/wood2.md3" );
	cgs.media.shardMetal1 = trap_R_RegisterModel( "models/shards/metal1.md3" );
	cgs.media.shardMetal2 = trap_R_RegisterModel( "models/shards/metal2.md3" );
	cgs.media.shardCeramic1 = trap_R_RegisterModel( "models/shards/ceramic1.md3" );
	cgs.media.shardCeramic2 = trap_R_RegisterModel( "models/shards/ceramic2.md3" );
	// done

	cgs.media.shardRubble1 = trap_R_RegisterModel( "models/mapobjects/debris/brick000.md3" );
	cgs.media.shardRubble2 = trap_R_RegisterModel( "models/mapobjects/debris/brick001.md3" );
	cgs.media.shardRubble3 = trap_R_RegisterModel( "models/mapobjects/debris/brick002.md3" );

	for ( i = 0; i < MAX_LOCKER_DEBRIS; i++ )
	{
		Com_sprintf( name, sizeof( name ), "models/mapobjects/debris/personal%i.md3", i + 1 );
		cgs.media.shardJunk[i] = trap_R_RegisterModel( name );
	}

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	CG_LoadTranslateStrings();  //----(SA)	added.  for localization, read on-screen print names from text file

	// TODO: FIXME:  REMOVE REGISTRATION OF EACH MODEL FOR EVERY LEVEL LOAD


	//----(SA)	okay, new stuff to intialize rather than doing it at level load time (or "give all" time)
	//			(I'm certainly not against being efficient here, but I'm tired of the rocket launcher effect only registering
	//			sometimes and want it to work for sure for this demo)


// code is almost complete for doing this correctly.  will remove when that is complete.
	CG_LoadingString( " - weapons" );
	for ( i = WP_KNIFE; i < WP_DUMMY_MG42; i++ ) {
			CG_RegisterWeapon( i, qfalse );
	}

// END


	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	CG_LoadingString( " - items" );
	for ( i = 1 ; i < bg_numItems ; i++ ) {
		CG_RegisterItemVisuals( i );
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.shadowFootShader = trap_R_RegisterShader( "markShadowFoot" );
	cgs.media.shadowTorsoShader = trap_R_RegisterShader( "markShadowTorso" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.wakeMarkShaderAnim = trap_R_RegisterShader( "wakeAnim" ); // (SA)

	//----(SA)	added
	cgs.media.bulletMarkShaderMetal = trap_R_RegisterShader( "gfx/damage/metal_mrk" );
	cgs.media.bulletMarkShaderWood = trap_R_RegisterShader( "gfx/damage/wood_mrk" );
	cgs.media.bulletMarkShaderCeramic = trap_R_RegisterShader( "gfx/damage/ceramic_mrk" );
	cgs.media.bulletMarkShaderGlass = trap_R_RegisterShader( "gfx/damage/glass_mrk" );

	for ( i = 0 ; i < 5 ; i++ ) {
		char name[32];
		//Com_sprintf( name, sizeof(name), "textures/decals/blood%i", i+1 );
		//cgs.media.bloodMarkShaders[i] = trap_R_RegisterShader( name );
		Com_sprintf( name, sizeof( name ), "blood_dot%i", i + 1 );
		cgs.media.bloodDotShaders[i] = trap_R_RegisterShader( name );
	}

	CG_LoadingString( " - inline models" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf( name, sizeof( name ), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	CG_LoadingString( " - server models" );

	// register all the server specified models
	for ( i = 1 ; i < MAX_MODELS ; i++ ) {
		const char      *modelName;

		modelName = CG_ConfigString( CS_MODELS + i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

	CG_LoadingString( " - particles" );
	CG_ClearParticles();

    InitSmokeSprites();

	for ( i = 1; i < MAX_PARTICLES_AREAS; i++ )
	{
		{
			int rval;

			rval = CG_NewParticleArea( CS_PARTICLES + i );
			if ( !rval ) {
				break;
			}
		}
	}

//	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	CG_LoadingString( " - game media done" );

}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients( void ) {
	int i;

	for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
		const char      *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
}

//===========================================================================

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

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char    *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	if ( strlen( parm1 ) ) {
		trap_S_StartBackgroundTrack( parm1, parm2, 0 );
	}
}

//----(SA)	added
/*
==============
CG_QueueMusic
==============
*/
void CG_QueueMusic( void ) {
	char    *s;
	char parm[MAX_QPATH];

	// prepare the next background track
	s = (char *)CG_ConfigString( CS_MUSIC_QUEUE );
	Q_strncpyz( parm, COM_Parse( &s ), sizeof( parm ) );

	// even if no strlen(parm).  we want to be able to clear the queue

	// TODO: \/		the values stored in here will be made accessable so
	//				it doesn't have to go through startbackgroundtrack() (which is stupid)
	trap_S_StartBackgroundTrack( parm, "", -2 );  // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
}
//----(SA)	end

char *CG_GetMenuBuffer( const char *filename ) {
	int len;
	fileHandle_t f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse( int handle ) {
	pc_token_t token;
	const char *tempStr;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( Q_stricmp( token.string, "{" ) != 0 ) {
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 ) {
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) ) {
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.textFont );
			continue;
		}

		// smallFont
		if ( Q_stricmp( token.string, "smallFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) ) {
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.smallFont );
			continue;
		}

		// font
		if ( Q_stricmp( token.string, "bigfont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) ) {
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.bigFont );
			continue;
		}

		// handwriting
		if ( Q_stricmp( token.string, "handwritingFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) ) {
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.handwritingFont );
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "ladderHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_LADDER] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "friendHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_PLYR_FRIEND] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "speakHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_PLYR_SPEAK] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "usableHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_ACTIVATE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "notUsableHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_NOACTIVATE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "doorHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_DOOR] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "doorRotateHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_DOOR_ROTATING] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "doorLockHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_DOOR_LOCKED] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "doorRotateLockHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_DOOR_ROTATING_LOCKED] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "mg42Hint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_MG42] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "breakableHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_BREAKABLE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "chairHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_CHAIR] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "alarmHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_ALARM] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "healthHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_HEALTH] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "treasureHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_TREASURE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "knifeHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_KNIFE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "buttonHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_BUTTON] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "waterHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_WATER] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "cautionHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_CAUTION] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "dangerHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_DANGER] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "secretHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_SECRET] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "questionHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_QUESTION] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "exclamationHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_EXCLAMATION] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "clipboardHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_CLIPBOARD] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "weaponHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_WEAPON] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "ammoHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_AMMO] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "armorHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_ARMOR] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "powerupHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_POWERUP] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "holdableHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_HOLDABLE] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "inventoryHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_INVENTORY] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "exitHint" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.hintShaders[HINT_EXIT] = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "yougotmail" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.youGotMailShader = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "yougotobjective" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.youGotObjectiveShader = trap_R_RegisterShader( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "friendlycross" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgs.media.crosshairFriendly = trap_R_RegisterShader( tempStr );
			continue;
		}


		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 ) {
			if ( !PC_String_Parse( handle, &cgDC.Assets.cursorStr ) ) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeClamp ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 ) {
			if ( !PC_Int_Parse( handle, &cgDC.Assets.fadeCycle ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeAmount ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowX ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowY ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 ) {
			if ( !PC_Color_Parse( handle, &cgDC.Assets.shadowColor ) ) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void CG_ParseMenu( const char *menuFile ) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource( menuFile );
	if ( !handle ) {
		handle = trap_PC_LoadSource( "ui/testhud.menu" );
	}
	if ( !handle ) {
		return;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 ) {
			if ( CG_Asset_Parse( handle ) ) {
				continue;
			} else {
				break;
			}
		}


		if ( Q_stricmp( token.string, "menudef" ) == 0 ) {
			// start a new menu
			Menu_New( handle );
		}
	}
	trap_PC_FreeSource( handle );
}

qboolean CG_Load_Menu( char **p ) {
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 ) {
			return qtrue;
		}

		if ( !token[0] ) {
			return qfalse;
		}

		CG_ParseMenu( token );
	}
	return qfalse;
}



void CG_LoadMenus( const char *menuFile ) {
	char    *token;
	char *p;
	int len, start;
	fileHandle_t f;
	static char buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f ) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if ( !f ) {
			CG_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		trap_FS_FCloseFile( f );
		CG_Error( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress( buf );

	Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] || token[0] == '}' ) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if ( Q_stricmp( token, "loadmenu" ) == 0 ) {
			if ( CG_Load_Menu( &p ) ) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );

}



static qboolean CG_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key ) {
	return qfalse;
}


static int CG_FeederCount( float feederID ) {
	int i, count;
	count = 0;
	if ( feederID == FEEDER_REDTEAM_LIST ) {
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == TEAM_RED ) {
				count++;
			}
		}
	} else if ( feederID == FEEDER_BLUETEAM_LIST ) {
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == TEAM_BLUE ) {
				count++;
			}
		}
	} else if ( feederID == FEEDER_SCOREBOARD ) {
		return cg.numScores;
	}
	return count;
}




///////////////////////////
///////////////////////////

static clientInfo_t * CG_InfoFromScoreIndex( int index, int team, int *scoreIndex ) {
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText( float feederID, int index, int column, qhandle_t *handle ) {
#ifdef MISSIONPACK
	gitem_t *item;
#endif  // #ifdef MISSIONPACK
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if ( feederID == FEEDER_REDTEAM_LIST ) {
		team = TEAM_RED;
	} else if ( feederID == FEEDER_BLUETEAM_LIST ) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
	sp = &cg.scores[scoreIndex];

	if ( info && info->infoValid ) {
		switch ( column ) {
		case 0:
			break;
		case 3:
			return info->name;
			break;
		case 4:
			return va( "%i", info->score );
			break;
		case 5:
			return va( "%4i", sp->time );
			break;
		case 6:
			if ( sp->ping == -1 ) {
				return "connecting";
			}
			return va( "%4i", sp->ping );
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage( float feederID, int index ) {
	return 0;
}

static void CG_FeederSelection( float feederID, int index ) {
	cg.selectedScore = index;
}

static float CG_Cvar_Get( const char *cvar ) {
	char buff[128];
	memset( buff, 0, sizeof( buff ) );
	trap_Cvar_VariableStringBuffer( cvar, buff, sizeof( buff ) );
	return atof( buff );
}

void CG_Text_PaintWithCursor( float x, float y, int font, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style ) {
	CG_Text_Paint( x, y, font, scale, color, text, 0, limit, style );
}

static int CG_OwnerDrawWidth( int ownerDraw, int font, float scale ) {
	switch ( ownerDraw ) {
	case CG_GAME_TYPE:
		return CG_Text_Width( CG_GameTypeString(), font, scale, 0 );
	case CG_GAME_STATUS:
		return CG_Text_Width( CG_GetGameStatusText(), font, scale, 0 );
		break;
	case CG_KILLER:
		return CG_Text_Width( CG_GetKillerText(), font, scale, 0 );
		break;
#ifdef MISSIONPACK
	case CG_RED_NAME:
		return CG_Text_Width( cg_redTeamName.string, font, scale, 0 );
		break;
	case CG_BLUE_NAME:
		return CG_Text_Width( cg_blueTeamName.string, font, scale, 0 );
		break;
#endif

	}
	return 0;
}

static int CG_PlayCinematic( const char *name, float x, float y, float w, float h ) {
	return trap_CIN_PlayCinematic( name, x, y, w, h, CIN_loop );
}

static void CG_StopCinematic( int handle ) {
	trap_CIN_StopCinematic( handle );
}

static void CG_DrawCinematic( int handle, float x, float y, float w, float h ) {
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void CG_RunCinematicFrame( int handle ) {
	trap_CIN_RunCinematic( handle );
}



/*
==============
CG_translateString
	presumably if this gets used more extensively, it'll be modified to a hash table
==============
*/
const char *CG_translateString( const char *str ) {
	int i, numStrings;

	numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		if ( !translateStrings[i].name || !strlen( translateStrings[i].name ) ) {
			return str;
		}

		if ( !strcmp( str, translateStrings[i].name ) ) {
			if ( translateStrings[i].localname && strlen( translateStrings[i].localname ) ) {
				return translateStrings[i].localname;
			}
			break;
		}
	}

	return str;
}

/*
==============
CG_bonusString
	presumably if this gets used more extensively, it'll be modified to a hash table
==============
*/
const char *CG_bonusString( const char *str ) {
	int i, numStrings;

	numStrings = sizeof( bonusStrings ) / sizeof( bonusStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		if ( !bonusStrings[i].name || !strlen( bonusStrings[i].name ) ) {
			return str;
		}

		if ( !strcmp( str, bonusStrings[i].name ) ) {
			if ( bonusStrings[i].localname && strlen( bonusStrings[i].localname ) ) {
				return bonusStrings[i].localname;
			}
			break;
		}
	}

	return str;
}

/*
=================
CG_LoadHudMenu

=================
*/
void CG_LoadHudMenu( void ) {

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	cgDC.getTranslatedString = &CG_translateString;     //----(SA)	added
	cgDC.getbonusString = &CG_bonusString;     //----(SA)	added
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error;
	cgDC.Print = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;

	Init_Display( &cgDC );

	Menu_Reset();

    if (cg_hudStyle.integer == 1) {
	CG_LoadMenus( "ui/hud/wolf09.txt");
	} else if (cg_hudStyle.integer == 2) {
	CG_LoadMenus( "ui/hud/ps2.txt");
	} else if (cg_hudStyle.integer == 3) {
	CG_LoadMenus( "ui/hud/xbox.txt");
	} else if (cg_hudStyle.integer == 4) {
	CG_LoadMenus( "ui/hud/et.txt");
	} else if (cg_hudStyle.integer == 5) {
	CG_LoadMenus( "ui/hud/vanilla.txt");
	} else if (cg_hudStyle.integer == 6) {
	CG_LoadMenus( "ui/hud/custom_hud1.txt");
	} else if (cg_hudStyle.integer == 7) {
	CG_LoadMenus( "ui/hud/custom_hud2.txt");
	} else if (cg_hudStyle.integer == 8) {
	CG_LoadMenus( "ui/hud/custom_hud3.txt");
	} else if (cg_hudStyle.integer == 9) {
	CG_LoadMenus( "ui/hud/custom_hud4.txt");
	} else if (cg_hudStyle.integer == 10) {
	CG_LoadMenus( "ui/hud/custom_hud5.txt");
	}
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}


/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence ) {
	const char  *s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof( cg_entities ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_items, 0, sizeof( cg_items ) );

	cg.refdef_current = &cg.refdef;

	// RF, init the anim scripting
	cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
	cgs.animScriptData.playSound = CG_SoundPlayIndexedScript;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	// (SA) using Nerve's text since they have foreign characters
	cgs.media.charsetShader     = trap_R_RegisterShader( "gfx/2d/hudchars" ); //trap_R_RegisterShader( "gfx/2d/bigchars" );
	// JOSEPH 4-17-00
	cgs.media.menucharsetShader = trap_R_RegisterShader( "gfx/2d/hudchars" );
	// END JOSEPH
	cgs.media.whiteShader       = trap_R_RegisterShader( "white" );
	cgs.media.charsetProp       = trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow   = trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB      = trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

//	cg.weaponSelect = WP_MP40;

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	if ( cg_fixedAspect.integer ) {
		cgs.screenXScaleStretch = cgs.glconfig.vidWidth * (1.0/640.0);
		cgs.screenYScaleStretch = cgs.glconfig.vidHeight * (1.0/480.0);
		if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
			cgs.screenXScale = cgs.glconfig.vidWidth * (1.0/640.0);
			cgs.screenYScale = cgs.glconfig.vidHeight * (1.0/480.0);
			// wide screen
			cgs.screenXBias = 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * (640.0/480.0) ) );
			cgs.screenXScale = cgs.screenYScale;
			// no narrow screen
			cgs.screenYBias = 0;
		} else {
			cgs.screenXScale = cgs.glconfig.vidWidth * (1.0/640.0);
			cgs.screenYScale = cgs.glconfig.vidHeight * (1.0/480.0);
			// narrow screen
			cgs.screenYBias = 0.5 * ( cgs.glconfig.vidHeight - ( cgs.glconfig.vidWidth * (480.0/640.0) ) );
			cgs.screenYScale = cgs.screenXScale;
			// no wide screen
			cgs.screenXBias = 0;
		}
	} else {
		cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
		cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;
	}

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	cg.refdef_current = &cg.refdef;

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

	String_Init();

	cg.loading = qtrue;     // force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "flamechunks" );

	CG_InitFlameChunks();       // RF, register and clear all flamethrower resources

	CG_LoadingString( "clients" );

	CG_RegisterClients();       // if low on memory, some clients will be deferred

	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff

	cg.loading = qfalse;    // future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// RF, init ZombieFX
	trap_RB_ZombieFXAddNewHit( -1, NULL, NULL );

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_SetupCabinets();

	cg.lightstylesInited = qfalse;

	CG_LoadingString( "" );

	CG_ShaderStateChanged();

	// RF, clear all sounds, so we dont hear anything after level load
	trap_S_ClearLoopingSounds( 2 );

	// start level load music
	// too late...
//	trap_S_StartBackgroundTrack( "sound/music/fla_mp03.wav", "sound/music/fla_mp03.wav", 1 );

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

void CG_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume ) {
	trap_S_AddLoopingSound( entityNum, origin, velocity, 1250, sfx, volume );	// volume was previously removed from CG_S_ADDLOOPINGSOUND. I added 'range'
}

void CG_S_AddRangedLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range ) {
	trap_S_AddLoopingSound( entityNum, origin, velocity, range, sfx, 255 );		// RF, assume full volume, since thats how it worked before
}

void CG_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	//trap_S_AddRealLoopingSound( entityNum, origin, velocity, 1250, sfx, 255 );	//----(SA) modified
}
