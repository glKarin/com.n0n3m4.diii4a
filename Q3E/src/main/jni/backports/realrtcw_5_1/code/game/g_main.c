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




#include "g_local.h"
#ifdef STEAM // !defined(__ANDROID__) //karin: without steam
#include "../../steam/steam.h"
#else // using stub
#include "../steam/steam.c"
#endif

level_locals_t level;

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
	qboolean trackChange;           // track this variable, and announce if changed
	qboolean teamShader;      // track and if changed, update shader state
} cvarTable_t;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

int g_scriptGlobalAccumBuffer[G_MAX_SCRIPT_GLOBAL_ACCUM_BUFFERS];

gentity_t       *g_camEnt = NULL;   //----(SA)	script camera

// Rafael gameskill
extern int bg_pmove_gameskill_integer;
// done

vmCvar_t g_gametype;

vmCvar_t g_newinventory;

// Rafael gameskill
vmCvar_t g_gameskill;
// done
vmCvar_t g_reloading;       //----(SA)	added

vmCvar_t g_dmflags;
vmCvar_t g_fraglimit;
vmCvar_t g_timelimit;
vmCvar_t g_capturelimit;
vmCvar_t g_friendlyFire;
vmCvar_t g_password;
vmCvar_t g_maxclients;
vmCvar_t g_maxGameClients;
vmCvar_t g_dedicated;
vmCvar_t g_speed;
vmCvar_t g_gravity;
vmCvar_t g_cheats;
vmCvar_t g_knockback;
vmCvar_t g_quadfactor;
vmCvar_t g_forcerespawn;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_debugBullets;
vmCvar_t g_debugAudibleEvents;      //----(SA)	added
vmCvar_t g_headshotMaxDist;     //----(SA)	added
vmCvar_t g_weaponRespawn;
vmCvar_t g_motd;
vmCvar_t g_synchronousClients;
vmCvar_t g_warmup;
vmCvar_t g_restarted;
vmCvar_t g_logfile;
vmCvar_t g_logfileSync;
vmCvar_t g_blood;
vmCvar_t g_podiumDist;
vmCvar_t g_podiumDrop;
vmCvar_t g_allowVote;

vmCvar_t g_needpass;
vmCvar_t g_weaponTeamRespawn;
vmCvar_t g_doWarmup;
vmCvar_t g_teamAutoJoin;
vmCvar_t g_teamForceBalance;
vmCvar_t g_listEntity;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;
vmCvar_t g_rankings;
vmCvar_t g_enableBreath;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

// Rafael
vmCvar_t g_autoactivate;
vmCvar_t g_testPain;

vmCvar_t g_missionStats;
vmCvar_t ai_scriptName;         // name of AI script file to run (instead of default for that map)
vmCvar_t g_scriptName;          // name of script file to run (instead of default for that map)

vmCvar_t g_developer;

vmCvar_t g_userAim;

vmCvar_t g_forceModel;

vmCvar_t g_mg42arc;

vmCvar_t g_totalPlayTime;
vmCvar_t g_attempts;

vmCvar_t g_footstepAudibleRange;
// JPW NERVE multiplayer reinforcement times
vmCvar_t g_redlimbotime;
vmCvar_t g_bluelimbotime;
// charge times for character class special weapons
vmCvar_t g_medicChargeTime;
vmCvar_t g_engineerChargeTime;
vmCvar_t g_LTChargeTime;
vmCvar_t g_soldierChargeTime;
// jpw

vmCvar_t g_playerStart;         // set when the player enters the game

vmCvar_t g_localTeamPref;

//RealRTCW new CVARs
vmCvar_t g_bodysink;
vmCvar_t g_weaponfalloff;
vmCvar_t g_jumptime;
vmCvar_t g_fireonthemove;
vmCvar_t g_spawndogs;
vmCvar_t g_spawnpriests;
vmCvar_t g_spawnxshepherds;
vmCvar_t g_aicanheadshot;
vmCvar_t g_ironchallenge;
vmCvar_t g_nohudchallenge;
vmCvar_t g_nopickupchallenge;
vmCvar_t g_decaychallenge;
vmCvar_t g_airespawn;
vmCvar_t g_reinforce;
vmCvar_t g_fullarsenal;
vmCvar_t g_endmapbonus;
vmCvar_t g_randomweapons;
vmCvar_t g_realism;
vmCvar_t g_regen;
vmCvar_t g_flushItems;	// items land depending on the slope thy're on
vmCvar_t g_midgame;
vmCvar_t g_vanilla_guns;
vmCvar_t g_dlc1;
vmCvar_t g_class;
vmCvar_t g_noobTube;

vmCvar_t g_playerSurvivalClass;

vmCvar_t g_mapname;

cvarTable_t gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, qfalse },

	// noset vars
	{ NULL, "gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", PRODUCT_DATE, CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse  },

	{ &g_newinventory, "g_newinventory", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// Rafael gameskill
	{ &g_gameskill, "g_gameskill", "2", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse  },   // (SA) new default '2' (was '1')
	// done
	{ &g_ironchallenge, "g_ironchallenge", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse }, 
	{ &g_nohudchallenge, "g_nohudchallenge", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_nopickupchallenge, "g_nopickupchallenge", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_decaychallenge, "g_decaychallenge", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  }, 

	{ &g_airespawn, "g_airespawn", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse},
    { &g_reinforce, "g_reinforce", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_fullarsenal, "g_fullarsenal", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_endmapbonus, "g_endmapbonus", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_randomweapons, "g_randomweapons", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_realism, "g_realism", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_regen, "g_regen", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_midgame, "g_midgame", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_vanilla_guns, "g_vanilla_guns", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_dlc1, "g_dlc1", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_class, "g_class", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_noobTube, "g_noobTube", "0", CVAR_ARCHIVE, 0, qfalse },

	{ &g_playerSurvivalClass, "g_playersurvivalclass", "0", CVAR_ARCHIVE | CVAR_LATCH , 0, qfalse },

	{ &g_reloading, "g_reloading", "0", CVAR_ROM },   //----(SA)	added

// JPW NERVE multiplayer stuffs
	{ &g_redlimbotime, "g_redlimbotime", "30000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_bluelimbotime, "g_bluelimbotime", "30000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_medicChargeTime, "g_medicChargeTime", "45000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_engineerChargeTime, "g_engineerChargeTime", "30000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_jumptime, "g_jumptime", "1", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_fireonthemove, "g_fireonthemove", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_spawndogs, "g_spawndogs", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_spawnpriests, "g_spawnpriests", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_spawnxshepherds, "g_spawnxshepherds", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_aicanheadshot, "g_aicanheadshot", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_LTChargeTime, "g_LTChargeTime", "35000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_soldierChargeTime, "g_soldierChargeTime", "20000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
// jpw

	{ &g_playerStart, "g_playerStart", "0", CVAR_ROM, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_friendlyFire, "g_friendlyFire", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },                            // NERVE - SMF - merge from team arena

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_logfile, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },
	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "320", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue },
	{ &g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_debugBullets, "g_debugBullets", "0", CVAR_CHEAT, 0, qfalse}, //----(SA)	added
	{ &g_debugAudibleEvents, "g_debugAudibleEvents", "0", CVAR_CHEAT, 0, qfalse}, //----(SA)	added

	{ &g_headshotMaxDist, "g_headshotMaxDist", "1024", CVAR_CHEAT, 0, qfalse},    //----(SA)	added


	{ &g_motd, "g_motd", "", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", 0, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

	{ &g_enableBreath, "g_enableBreath", "1", CVAR_SERVERINFO, 0, qtrue},

	{ &g_testPain, "g_testPain", "0", CVAR_CHEAT, 0, qfalse },

	{ &g_missionStats, "g_missionStats", "0", CVAR_ROM, 0, qfalse },

	{ &g_developer, "developer", "0", CVAR_TEMP, 0, qfalse },

	{ &g_rankings, "g_rankings", "0", 0, 0, qfalse},

	{ &g_userAim, "g_userAim", "1", CVAR_CHEAT, 0, qfalse },

	{ &g_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE, 0, qfalse},
	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},


	{&g_mg42arc, "g_mg42arc", "0", CVAR_TEMP, 0, qfalse},

	{&g_totalPlayTime, "g_totalPlayTime", "0", CVAR_ROM, 0, qfalse},
	{&g_attempts, "g_attempts", "0", CVAR_ROM, 0, qfalse},

	{&g_footstepAudibleRange, "g_footstepAudibleRange", "256", CVAR_CHEAT, 0, qfalse},

	{&g_scriptName, "g_scriptName", "", CVAR_ROM, 0, qfalse},
	{&ai_scriptName, "ai_scriptName", "", CVAR_ROM, 0, qfalse},

	{ &g_localTeamPref, "g_localTeamPref", "", 0, 0, qfalse },

	{ &g_bodysink, "g_bodysink", "0", CVAR_ARCHIVE },
	{ &g_weaponfalloff, "g_weaponfalloff", "0", CVAR_ARCHIVE },
	{ &g_flushItems,	"g_flushItems",		"1",	0 },
	{ &g_mapname, "mapname", "", CVAR_ARCHIVE }
};

static int gameCvarTableSize = ARRAY_LEN( gameCvarTable );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );

// Ridah, Cast AI
qboolean AICast_VisibleFromPos( vec3_t srcpos, int srcnum,
								vec3_t destpos, int destnum, qboolean updateVisPos );
qboolean AICast_CheckAttackAtPos( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld );
void AICast_Init( void );
// done.

void G_RetrieveMoveSpeedsFromClient( int entnum, char *text );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0 );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
		// Ridah, Cast AI
	case AICAST_VISIBLEFROMPOS:
		return AICast_VisibleFromPos( (float *)arg0, arg1, (float *)arg2, arg3, arg4 );
	case AICAST_CHECKATTACKATPOS:
		return AICast_CheckAttackAtPos( arg0, arg1, (float *)arg2, arg3, arg4 );
		// done.

	case GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT:
		G_RetrieveMoveSpeedsFromClient( arg0, (char *)arg1 );
		return 0;
	case GAME_GETMODELINFO:
		return G_GetModelInfo( arg0, (char *)arg1, (animModelInfo_t **)arg2 );
	}

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end( argptr );

	trap_Print( text );
}

void QDECL G_DPrintf( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	if ( !g_developer.integer ) {
		return;
	}

	va_start( argptr, fmt );
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end( argptr );

	trap_Print( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end( argptr );

	trap_Error( text );
}



qboolean G_canStealthStab( int aiChar ) {
	switch ( aiChar ) {
	case AICHAR_SOLDIER:
	case AICHAR_AMERICAN:
	case AICHAR_ELITEGUARD:
	case AICHAR_BLACKGUARD:
	case AICHAR_PARTISAN:
	case AICHAR_RUSSIAN:
	case AICHAR_CIVILIAN:
		return qtrue;
	}
	return qfalse;
}

/*
==============
G_EndGame
==============
*/
void G_EndGame( void ) {
	trap_Endgame();
}


#define CH_KNIFE_DIST       48  // from g_weapon.c
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96
#define CH_ACTIVATE_DIST    96
#define CH_EXIT_DIST        256
#define CH_FRIENDLY_DIST    1024    // distance at which you can identify if someone is a friend

#define CH_MAX_DIST         1024    // use the largest value from above
#define CH_MAX_DIST_ZOOM    8192    // max dist for zooming hints
/*
==============
G_CheckForCursorHints
	non-AI's check for cursor hint contacts

	server-side because there's info we want to show that the client
	just doesn't know about.  (health or other info of an explosive,invisible_users,items,etc.)

	traceEnt is the ent hit by the trace, checkEnt is the ent that is being
	checked against (in case the traceent was an invisible_user or something)

==============
*/
static int nextCheckTime = 0;

//----(SA)	can't include ai_cast.h that has the 'real' defines for these
#define AITEAM_NAZI     0
#define AITEAM_ALLIES   1
#define AITEAM_MONSTER  2
#define AITEAM_NEUTRAL  7   // yes, '7'

void G_CheckForCursorHints( gentity_t *ent ) {
	vec3_t forward, right, up, offset, end;
	trace_t     *tr;
	float dist;
	gentity_t   *checkEnt, *traceEnt = 0;
	playerState_t *ps;
	int hintType, hintDist, hintVal;
	qboolean zooming, indirectHit;      // indirectHit means the checkent was not the ent hit by the trace (checkEnt!=traceEnt)
	int trace_contents;                 // DHM - Nerve

	if ( !ent->client ) {
		return;
	}

	ps = &ent->client->ps;

	// don't change anything if reloading.  just set the exit hint
	if ( g_reloading.integer == RELOAD_NEXTMAP_WAITING ) {
		ps->serverCursorHint = HINT_EXIT;
		ps->serverCursorHintVal = 0;
		return;
	}


	if ( ps->aiChar != AICHAR_NONE ) {
		return;
	}

	if ( nextCheckTime > level.time ) {
		return;
	}

	nextCheckTime = level.time + 100;   // wait a little before checking again	(10hz)

	indirectHit = qfalse;

	zooming = (qboolean)( ps->eFlags & EF_ZOOMING );

	AngleVectors( ps->viewangles, forward, right, up );

	//----(SA)	modified to use shared routine for finding start point
	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	if ( zooming ) {
		VectorMA( offset, CH_MAX_DIST_ZOOM, forward, end );
	} else {
		VectorMA( offset, CH_MAX_DIST, forward, end );
	}

	tr = &ps->serverCursorHintTrace;
	trace_contents = ( CONTENTS_TRIGGER | CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY | CONTENTS_CORPSE );   // SP fine checking corpses
	trap_Trace( tr, offset, NULL, NULL, end, ps->clientNum, trace_contents );

	traceEnt = &g_entities[tr->entityNum];

	if ( traceEnt->classname && Q_stricmp( traceEnt->classname, "trigger_hurt" ) == 0 ) {
		// ignore trigger_hurt so it's possible to pickup chalice (-1472 -3472 284) at the end of map crypt2
		trap_Trace( tr, tr->endpos, NULL, NULL, end, tr->entityNum, trace_contents );

		// muzzle and trigger_hurt are in player bbox?
		if ( tr->entityNum == ps->clientNum ) {
			tr->entityNum = ENTITYNUM_NONE;
			tr->fraction = 1;
		}
	}

	// reset all
	hintType    = ps->serverCursorHint      = HINT_NONE;
	hintVal     = ps->serverCursorHintVal   = 0;

	if ( zooming ) {
		dist        = tr->fraction * CH_MAX_DIST_ZOOM;
		hintDist    = CH_MAX_DIST_ZOOM;
	} else {
		dist        = tr->fraction * CH_MAX_DIST;
		hintDist    = CH_MAX_DIST;
	}

	if ( tr->fraction == 1 ) {
		return;
	}

	traceEnt = &g_entities[tr->entityNum];


	//
	// WORLD
	//
	if ( tr->entityNum == ENTITYNUM_WORLD ) {
		if ( tr->contents & CONTENTS_WATER ) {
			hintDist = CH_WATER_DIST;
			hintType = HINT_WATER;
		}
		// ladder
		else if ( ( tr->surfaceFlags & SURF_LADDER ) && !( ps->pm_flags & PMF_LADDER ) ) {
			hintDist = CH_LADDER_DIST;
			hintType = HINT_LADDER;
		}
	}
	//
	// PEOPLE
	//
	else if ( tr->entityNum < MAX_CLIENTS ) {

		if ( ent->s.weapon == WP_KNIFE ) {
			vec3_t pforward, eforward;
			qboolean canKnife = qfalse;

// only knife certain characters
			if ( G_canStealthStab( traceEnt->client->ps.aiChar ) ) {
				canKnife = qtrue;
			}

			if ( canKnife ) {
				AngleVectors( ent->client->ps.viewangles,       pforward, NULL, NULL );
				AngleVectors( traceEnt->client->ps.viewangles,  eforward, NULL, NULL );

				// (SA) TODO: neutralize pitch (so only yaw is considered)?
				if ( DotProduct( eforward, pforward ) > 0.9f ) {   // from behind
					hintType = HINT_KNIFE;
					hintDist = CH_KNIFE_DIST;
				}
			}
		}

		// check for friendly.
		if ( traceEnt->aiTeam == AITEAM_ALLIES || traceEnt->aiTeam == AITEAM_NEUTRAL ) {
		
			if (traceEnt->canSpeak == 1 ) {
				if ( dist > CH_ACTIVATE_DIST ) {
				   hintType = HINT_PLYR_FRIEND;
			       hintDist = CH_FRIENDLY_DIST; 
				} else {
			       hintType = HINT_PLYR_SPEAK;
			       hintDist = CH_ACTIVATE_DIST; 
				}
			} else {
   		        hintType = HINT_PLYR_FRIEND;
			    hintDist = CH_FRIENDLY_DIST; 
			}
		}

	}

	//
	// OTHER ENTITIES
	//
	else {
		checkEnt = traceEnt;

		// check invisible_users first since you don't want to draw a hint based
		// on that ent, but rather on what they are targeting.
		// so find the target and set checkEnt to that to show the proper hint.
		if ( traceEnt->s.eType == ET_GENERAL ) {

			// ignore trigger_aidoor.  can't just not trace for triggers, since I need invisible_users...
			// damn, I would like to ignore some of these triggers though.

			if ( !Q_stricmp( traceEnt->classname, "trigger_aidoor" ) ) {
				return;
			}

			if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) ) {
				indirectHit = qtrue;

				checkEnt = G_Find( NULL, FOFS( targetname ), traceEnt->target );
				if ( !checkEnt ) {     // no target found
				hintType = HINT_BAD_USER;
				hintDist = CH_MAX_DIST_ZOOM;    // show this one from super far for debugging
					}
			}
		}


		if ( checkEnt ) {
			if ( checkEnt->s.eType == ET_GENERAL ) {

				// this is effectively an 'exit' brush.  they should be created with:
				//
				// classname = 'ai_trigger'
				// ainame = 'player'
				// target = 'endmap'
				//
				if (!Q_stricmp(checkEnt->classname, "props_grammofon") && (!checkEnt->is_dead)) {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_BUTTON;
				}
				else if (!Q_stricmp(checkEnt->classname, "item_key_unlock")) {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_DOOR;
				}
				else if ( !Q_stricmp( traceEnt->classname, "ai_trigger" ) ) {
					if ( ( !Q_stricmp( traceEnt->aiName, "player" ) ) &&
						 ( !Q_stricmp( traceEnt->target, "endmap" ) ) ) {

						hintDist = CH_EXIT_DIST;

						if ( G_SendMissionStats() ) {  // update the 'time' in the exit stats
							if ( zooming ) {
								hintType = HINT_EXIT_FAR;
							} else {
								hintType = HINT_EXIT;
							}
						} else {
							if ( zooming ) {
								hintType = HINT_NOEXIT_FAR;
							} else {
								hintType = HINT_NOEXIT;
							}
						}


						// show distance in the cursorhint bar
						if ( dist <= 255 ) {
							hintVal = (int)dist;    // range for this hint is 256, so it happens to translate nicely
						} else {
							hintVal = 255;
						}
					}
				}
			} else if ( checkEnt->s.eType == ET_MG42 )      {
					if ( ent->s.weapon != WP_SNIPERRIFLE &&
						 ent->s.weapon != WP_SNOOPERSCOPE &&
						 ent->s.weapon != WP_FG42SCOPE &&
						 ent->s.weapon != WP_DELISLESCOPE &&
						 ent->s.weapon != WP_M1941SCOPE ) 
						{
						if ( traceEnt->takedamage ) {
							hintDist = CH_ACTIVATE_DIST;
							hintType = HINT_MG42;
						}
					}
			} else if ( checkEnt->s.eType == ET_EXPLOSIVE )      {
				if ( checkEnt->takedamage && checkEnt->health > 0 ) {              // 0 health explosives are not breakable
					hintDist    = CH_BREAKABLE_DIST;
					hintType    = HINT_BREAKABLE;
					hintVal     = checkEnt->health;     // also send health to client for visualization
				}
			} else if ( checkEnt->s.eType == ET_ALARMBOX )      {
				if ( checkEnt->health > 0 ) {
//					hintDist	= CH_BREAKABLE_DIST;
                    hintDist = CH_ACTIVATE_DIST;
					hintType    = HINT_ACTIVATE;
				}
			} else if ( checkEnt->s.eType == ET_ITEM )      {
				gitem_t *it;
				it = &bg_itemlist[checkEnt->item - bg_itemlist];

				hintDist = CH_ACTIVATE_DIST;

				switch ( it->giType ) {
				case IT_HEALTH:
					if ( !( checkEnt->s.density == ( 1 << 9 ) ) ) { // (10 bits of data transmission for density)
						hintType = HINT_HEALTH;
					}
					break;
				case IT_TREASURE:
					hintType = HINT_TREASURE;
					break;
				case IT_CLIPBOARD:
					hintType = HINT_CLIPBOARD;
					break;
				case IT_WEAPON:
					hintType = HINT_WEAPON;
					break;
				case IT_AMMO:
					hintType = HINT_AMMO;
					break;
				case IT_ARMOR:
					hintType = HINT_ARMOR;
					break;
				case IT_POWERUP:
					if ( !( checkEnt->s.density == ( 1 << 9 ) ) ) { // (10 bits of data transmission for density)
						hintType = HINT_POWERUP;
					}
					break;
				case IT_HOLDABLE:
					hintType = HINT_HOLDABLE;
					break;
				case IT_KEY:
					hintType = HINT_INVENTORY;
					break;
				case IT_TEAM:
				case IT_BAD:
				default:
					break;
				}
			} else if ( checkEnt->s.eType == ET_MOVER )     {
				if ( !Q_stricmp( checkEnt->classname, "func_door_rotating" ) ) {
					if ( checkEnt->moverState == MOVER_POS1ROTATE    ||      // stationary/closed
						 ( checkEnt->moverState == MOVER_POS2ROTATE && checkEnt->flags & FL_TOGGLE ) ) { // toggle door that's open

						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR_ROTATING;

						if ( checkEnt->key >= KEY_LOCKED_TARGET ) {    // locked
							//						hintType = HINT_DOOR_ROTATING_LOCKED;
						}
					}
				} else if ( !Q_stricmp( checkEnt->classname, "func_door" ) )         {
					if ( checkEnt->moverState == MOVER_POS1   || // stationary/closed
						 ( checkEnt->moverState == MOVER_POS2 && checkEnt->flags & FL_TOGGLE ) ) { // toggle door that's open

						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR;

						if ( checkEnt->key >= KEY_LOCKED_TARGET ) {    // locked
							//						hintType = HINT_DOOR_LOCKED;
						}
					}
				} else if ( !Q_stricmp( checkEnt->classname, "func_button" ) )         {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_BUTTON;
				} else if ( !Q_stricmp( checkEnt->classname, "props_flamebarrel" ) )         {
					hintDist = CH_BREAKABLE_DIST;
					hintType = HINT_BREAKABLE;
				} else if ( !Q_stricmp( checkEnt->classname, "props_statue" ) )         {
					hintDist = CH_BREAKABLE_DIST;
					hintType = HINT_BREAKABLE;
				} else if ( strstr( checkEnt->classname, "chair" ) )         {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_CHAIR;
				}
			}

			// hint icon specified in check entity (possibly an entity targeted by an invis_user) and appropriate contact was made, so hintType was set
			// first try the checkent...
			if ( checkEnt->s.dmgFlags && hintType ) {
				hintType = checkEnt->s.dmgFlags;
			}
		}

		// then the traceent
		if ( traceEnt->s.dmgFlags && ( hintType || indirectHit ) ) {

			hintType = traceEnt->s.dmgFlags;

			// set up any forced max distances for specified hints
			switch ( hintType ) {
			case HINT_KNIFE:
				if ( ent->s.weapon == WP_KNIFE ) {
					hintDist = CH_KNIFE_DIST;
				} else {
					hintType = 0;       // no knife, clear it
				}
				break;
			case HINT_BREAKABLE:
				hintDist = CH_BREAKABLE_DIST;
				break;
			case HINT_LADDER:
				hintDist = CH_LADDER_DIST;
				break;

			default:
				hintDist = CH_ACTIVATE_DIST;
				break;
			}
		}

	}


	if ( zooming ) {
		hintDist = CH_MAX_DIST_ZOOM;

		// zooming can eat a lot of potential hints
		switch ( hintType ) {

			// allow while zooming
		case HINT_MG42:
		case HINT_PLAYER:
		case HINT_TREASURE:
		case HINT_LADDER:
		case HINT_EXIT_FAR:
		case HINT_NOEXIT:
		case HINT_NOEXIT_FAR:
		case HINT_PLYR_FRIEND:
		case HINT_PLYR_SPEAK:
		case HINT_PLYR_NEUTRAL:
		case HINT_PLYR_ENEMY:
		case HINT_PLYR_UNKNOWN:
			break;

		default:
			return;
		}
	}


	if ( dist <= hintDist ) {
		ps->serverCursorHint = hintType;
		ps->serverCursorHintVal = hintVal;
	}


//	Com_Printf("hint: %d\n", ps->serverCursorHint);
}


void G_SetTargetName( gentity_t* ent, char* targetname ) {
	if ( targetname && *targetname ) {
		ent->targetname = targetname;
		ent->targetnamehash = BG_StringHashValue( targetname );
	} else {
		ent->targetnamehash = -1;
	}
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t   *e, *e2;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for ( i = MAX_CLIENTS, e = g_entities + i ; i < level.num_entities ; i++,e++ ) {
		if ( !e->inuse ) {
			continue;
		}

		if ( !e->team ) {
			continue;
		}

		if ( e->flags & FL_TEAMSLAVE ) {
			continue;
		}

//----(SA)	this was never setting a master except for tramcar's...
		if ( !Q_stricmp( e->classname, "func_tramcar" ) ) {
			if ( e->spawnflags & 8 ) { // leader
				e->teammaster = e;
			} else {
				continue;
			}
		} else {
			e->teammaster = e;
		}
//----(SA)	end

		c++;
		c2++;
		for ( j = i + 1, e2 = e + 1 ; j < level.num_entities ; j++,e2++ )
		{
			if ( !e2->inuse ) {
				continue;
			}
			if ( !e2->team ) {
				continue;
			}
			if ( e2->flags & FL_TEAMSLAVE ) {
				continue;
			}
			if ( !strcmp( e->team, e2->team ) ) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
//				e2->key = e->key;	// (SA) I can't set the key here since the master door hasn't finished spawning yet and therefore has a key of -1
				e2->flags |= FL_TEAMSLAVE;

				if ( !Q_stricmp( e2->classname, "func_tramcar" ) ) {
					trap_UnlinkEntity( e2 );
				}

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;

					// Rafael
					// note to self: added this because of problems
					// pertaining to keys and double doors
					if ( Q_stricmp( e2->classname, "func_door_rotating" ) ) {
						e2->targetname = NULL;
					}
				}
			}
		}
	}
}


/*
==============
G_RemapTeamShaders
==============
*/
void G_RemapTeamShaders( void ) {
#ifdef MISSIONPACK
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof( string ), "team_icon/%s_red", g_redteam.string );
	AddRemap( "textures/ctf2/redteam01", string, f );
	AddRemap( "textures/ctf2/redteam02", string, f );
	Com_sprintf( string, sizeof( string ), "team_icon/%s_blue", g_blueteam.string );
	AddRemap( "textures/ctf2/blueteam01", string, f );
	AddRemap( "textures/ctf2/blueteam02", string, f );
	trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
#endif
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int i;
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
							cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar ) {
			cv->modificationCount = cv->vmCvar->modificationCount;
		}

		if ( cv->teamShader ) {
			remapped = qtrue;
		}
	}

	if ( remapped ) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update( &g_gametype );
	}

	// Rafael gameskill
	if ( g_gameskill.integer < GSKILL_EASY || g_gameskill.integer > GSKILL_SURVIVAL ) {
		G_Printf( "g_gameskill %i is out of range, default to medium\n", g_gameskill.integer );
		trap_Cvar_Set( "g_gameskill", va( "%d", GSKILL_MEDIUM ) ); // default to medium
	}

	bg_pmove_gameskill_integer = g_gameskill.integer;
	// done

	level.warmupModificationCount = g_warmup.modificationCount;
}

extern void AICast_CastScriptThink( void ) ;

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int i;
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"",
													cv->cvarName, cv->vmCvar->string ) );
				}

				if ( cv->teamShader ) {
					remapped = qtrue;
				}

				// check for changed values for particular cvars
				if ( !Q_stricmp( cv->cvarName, "g_playerStart" ) ) {
					gentity_t *player;
					player = AICast_FindEntityForName( "player" );
					if ( player && cv->vmCvar->integer ) {
						char filename[MAX_QPATH];
						char mapname[MAX_QPATH];

						// RF, we need to save the game after the "playestart" is called, but before the
						// camera is actually started (since the camera needs to be triggered on loading the game)
						// So we should call the script, but not let it actually run. Therefore upon loading, the
						// script should run, and the camera start.

						saveGamePending = qtrue;    // set this temporarily so we dont actually run the script just yet
						AICast_ScriptEvent( AICast_GetCastState( player->s.number ), "playerstart", "" );
						saveGamePending = qfalse;   // set it back

						// save the "autosave\\<mapname>" savegame, which is taken before any cameras have been played
						trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
						Q_strncpyz( filename, "autosave\\", sizeof( filename ) );
						Q_strcat( filename, sizeof( filename ), mapname );
						G_SaveGame( filename );

						// now let it think
						AICast_CastScriptThink();

						// if we are not watching a cutscene, save the game
						if ( !g_entities[0].client->cameraPortal ) {
							G_SaveGame( NULL );
							G_SaveGame( "lastcheckpoint" );
						}

						trap_Cvar_Set( "cg_norender", "0" );  // camera has started, render 'on'
						trap_Cvar_Set( "g_playerstart", "0" ); // reset calling of "playerstart" from script
					}
				}
			}
		}
	}

	if ( remapped ) {
		G_RemapTeamShaders();
	}
}



/*
==============
G_SpawnScriptCamera
	create the game entity that's used for camera<->script communication and portal location for camera view
==============
*/
void G_SpawnScriptCamera( void ) {
	if ( g_camEnt ) {
		G_FreeEntity( g_camEnt );
	}

	g_camEnt = G_Spawn();

	g_camEnt->scriptName = "scriptcamera";

	g_camEnt->s.eType = ET_CAMERA;
	g_camEnt->s.apos.trType = TR_STATIONARY;
	g_camEnt->s.apos.trTime = 0;
	g_camEnt->s.apos.trDuration = 0;
	VectorCopy( g_camEnt->s.angles, g_camEnt->s.apos.trBase );
	VectorClear( g_camEnt->s.apos.trDelta );

	g_camEnt->s.frame = 0;

	g_camEnt->r.svFlags |= SVF_NOCLIENT;        // only broadcast when in use

	if ( g_camEnt->s.number >= MAX_CLIENTS && g_camEnt->scriptName ) {
		G_Script_ScriptParse( g_camEnt );
		G_Script_ScriptEvent( g_camEnt, "spawn", "" );
	}

}


/*
==============
G_SendMissionStats
	for updating the g_missionStats string to the client
==============
*/
int G_SendMissionStats( void ) {
	char cmd[MAX_QPATH];
	gentity_t   *player;
	int i, attempts = 0, playtime = 0, minutes, objs = 0, sec = 0, treas = 0;
	int canExit = 0;

	player = AICast_FindEntityForName( "player" );
	if ( player ) {
		attempts = AICast_NumAttempts( player->s.number ) + 1;    // attempts tracks '0' as attempt 1
		AICast_AgePlayTime( player->s.number );
		playtime = AICast_PlayTime( player->s.number );

		for ( i = 0; i < 8; i++ ) {  // max objectives is '8'.  FIXME: use #define somewhere
			if ( player->missionObjectives & ( 1 << i ) ) {
				objs++;
			}
		}
		sec = player->numSecretsFound;
		treas = player->numTreasureFound;
	}

	memset( cmd, 0, sizeof( cmd ) );
	Q_strcat( cmd, sizeof( cmd ), "s=" );

	// 'kills' no longer tracked

	if ( playtime < 3600000 ) {
		minutes = ( playtime / 1000 ) / 60;
	} else {
		minutes = ( ( playtime % 3600000 ) / 1000 ) / 60; // handle hours in a map

	}
	Q_strcat( cmd, sizeof( cmd ), va( ",%i,%i,%i", ( ( playtime / 1000 ) / 60 ) / 60, minutes, ( playtime / 1000 ) % 60 ) );

	// objectives
	Q_strcat( cmd, sizeof( cmd ), va( ",%i,%i", objs, level.numObjectives ) );
	if ( objs >= level.numObjectives ) { // you're ready to go!
		canExit = 1;
	}

	// secrets
	Q_strcat( cmd, sizeof( cmd ), va( ",%i,%i", sec, level.numSecrets ) );

	// treasure
	Q_strcat( cmd, sizeof( cmd ), va( ",%i,%i", treas, level.numTreasure ) );

	// attempts
	Q_strcat( cmd, sizeof( cmd ), va( ",%i", attempts ) );

//	trap_Cvar_Set( "g_missionStats", cmd );
	// changing to a configstring (should help w/ savegame, no?)
	trap_SetConfigstring( CS_MISSIONSTATS, cmd );

	return canExit;
}

static inline const char* G_GameSkillIntToStr( int skill ) {
	switch ( skill ) {
		case GSKILL_EASY:		return "easy";
		case GSKILL_MEDIUM:		return "medium";
		case GSKILL_HARD:		return "hard";
		case GSKILL_MAX:		return "max";
		case GSKILL_REALISM:	return "realism";
		case GSKILL_SURVIVAL:	return "survival";
		default:				return "";
	}
}

/*
============
G_InitGame

============
*/
extern void trap_Cvar_Reset( const char *var_name );

void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int i;

	steamInit();

	srand( randomSeed );

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.numSecrets = 0;   //----(SA)	added

	// don't need it.
//	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	level.bulletRicochetSound = G_SoundIndex( "bulletRicochet" );

	level.snipersound = G_SoundIndex( "sound/weapons/mauser/mauserf1.wav" );

	//----(SA)	added sound caching
	level.knifeSound[0] = G_SoundIndex( "sound/weapons/knife/knife_hitwall1.wav" );
	//----(SA)	end

	// RF, init the anim scripting
	level.animScriptData.soundIndex = G_SoundIndex;
	level.animScriptData.playSound = G_AnimScriptSound;

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof( g_entities[0] ) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof( g_clients[0] ) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for ( i=0 ; i<MAX_CLIENTS ; i++ ) {
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
						 &level.clients[0].ps, sizeof( level.clients[0] ) );

	// Ridah
		char s[10];
		// Ridah, initialize cast AI system
		// DHM - Nerve :: Moved this down so that it only happens in SinglePlayer games

		// Load survival parameters
		if (g_gametype.integer == GT_SURVIVAL)
		{
			AI_LoadSurvivalTable(g_mapname.string);
		}

		AICast_Init();
		// done.

		AICast_ScriptLoad();

		trap_Cvar_VariableStringBuffer( "g_missionStats", s, sizeof( s ) );
		if ( strlen( s ) < 1 ) {
			// g_missionStats is used to get the player to press a key to begin
			trap_Cvar_Set( "g_missionStats", "xx" );
		}

		for ( i = 0; i < 8; i++ )     {  // max objective cvars: 8 (FIXME: use #define somewhere)
			trap_Cvar_Set( va( "g_objective%i", i + 1 ), "0" );   // clear the objective ROM cvars
		}
		trap_Cvar_Set( "cg_yougotMail", "0" );

	G_Script_ScriptLoad();
	// done.

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	G_ResetRemappedShaders();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// create the camera entity that will communicate with the scripts
	G_SpawnScriptCamera();

	// general initialization
	G_FindTeams();

	SaveRegisteredItems();


	G_ModelIndex( SP_PODIUM_MODEL );

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
//		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	// Load ammo parameters for all weapons
	for ( weapon_t weaponNum = WP_KNIFE; weaponNum < WP_NUM_WEAPONS; weaponNum++ ) {
		G_LoadAmmoTable( weaponNum );
		BG_SetWeaponForSkill( weaponNum, g_gameskill.integer );
	}

	// Load behavior parameters for all AICharacters
	for ( AICharacters_t characterNum = AICHAR_SOLDIER; characterNum < NUM_CHARACTERS; characterNum++ ) {
		AI_LoadBehaviorTable( characterNum );
	}

	trap_SetConfigstring( CS_INTERMISSION, "" );

	// fretn
	G_LoadArenas();

	steamSetRichPresence( "Mapname", g_mapname.string );
	steamSetRichPresence( "Skill", G_GameSkillIntToStr( g_gameskill.integer ) ) ;
	steamSetRichPresence( "steam_display", "#status_map" );
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {

	if ( level.logFile ) {
		G_LogPrintf( "ShutdownGame:\n" );
		G_LogPrintf( "------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
		level.logFile = 0;
	}

	// RF, update the playtime
	AICast_AgePlayTime( 0 );
	

	// Ridah, shutdown the Botlib, so weapons and things get reset upon doing a "map xxx" command
	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		int i;

		// Ridah, kill AI cast's
		for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
			if ( g_entities[i].r.svFlags & SVF_CASTAI ) {
				trap_DropClient( i, "Drop Cast AI" );
			}
		}
		// done.
	}
	// done.

	// write all the client session data so we can get it back
	G_WriteSessionData();


	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	if ( !restart ) {
		steamSetRichPresence( "steam_display", "#status_mainmenu" );
	}
}



//===================================================================

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, error );
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end( argptr );

	trap_Print( text );
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int i;
	gclient_t   *client;
	gclient_t   *nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			 client->sess.spectatorClient < 0  ) {
			continue;
		}

		if(!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum)
			nextInLine = client;
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue(gclient_t *client)
{
	int index;
	gclient_t *curclient;
	
	for(index = 0; index < level.maxclients; index++)
	{
		curclient = &level.clients[index];
		
		if(curclient->pers.connected != CON_DISCONNECTED)
		{
			if(curclient == client)
				curclient->sess.spectatorNum = 0;
			else if(curclient->sess.sessionTeam == TEAM_SPECTATOR)
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}


/*
=======================
AdjustTournamentScores

=======================
*/
void AdjustTournamentScores( void ) {
	int clientNum;

	clientNum = level.sortedClients[0];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged( clientNum );
	}

	clientNum = level.sortedClients[1];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged( clientNum );
	}

}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t   *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		 > cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		 < cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int i;
	int rank;
	int score;
	int newScore;
	gclient_t   *cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;     // don't count bots

	for (i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++)
		level.numteamVotingClients[i] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;

				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !( g_entities[i].r.svFlags & SVF_BOT ) ) {
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED ) {
							level.numteamVotingClients[0]++;
						} else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE ) {
							level.numteamVotingClients[1]++;
						}
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients,
		   sizeof( level.sortedClients[0] ), SortRanks );

		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i - 1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va( "%i", SCORE_NOT_PRESENT ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va( "%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va( "%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}
	

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy( level.intermission_angle, ent->client->ps.viewangles );
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof( ent->client->ps.powerups ) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t   *ent, *target;
	vec3_t dir;

	// find the intermission spot
	ent = G_Find( NULL, FOFS( classname ), "info_player_intermission" );
	if ( !ent ) {   // the map creator forgot to put in an intermission point...
		SelectSpawnPoint( vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy( ent->s.origin, level.intermission_origin );
		VectorCopy( ent->s.angles, level.intermission_angle );
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int i;
	gentity_t   *client;

	if ( level.intermissiontime ) {
		return;     // already active
	}

	level.intermissiontime = level.time;

	// move all clients to the intermission point
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = g_entities + i;
		if ( !client->inuse ) {
			continue;
		}
		// respawn if dead
		if ( client->health <= 0 ) {
			ClientRespawn( client );
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel( void ) {
	int i;
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	trap_Cvar_VariableStringBuffer( "d1", d1, sizeof(d1) );

	if( !Q_stricmp( nextmap, "map_restart 0" ) && Q_stricmp( d1, "" ) ) {
		trap_Cvar_Set( "nextmap", "vstr d2" );
		trap_SendConsoleCommand( EXEC_APPEND, "vstr d1\n" );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}

	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {

		// Ridah, kill AI cast's
		if ( g_entities[i].r.svFlags & SVF_CASTAI ) {
			trap_DropClient( i, "Drop Cast AI" );
			continue;
		}
		// done.

		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list argptr;
	char string[1024];
	int min, tens, sec;

	sec = ( level.time - level.startTime ) / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int i, numSorted;
	gclient_t       *cl;



	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	for ( i = 0 ; i < numSorted ; i++ ) {
		int ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n",
					 cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],
					 cl->pers.netname );
	}
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {

return;
	
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
	int i;
	gclient_t   *cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit();
		return;
	}

	if ( level.intermissionQueued ) {
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	if ( g_timelimit.integer < 0 || g_timelimit.integer > INT_MAX / 60000 ) {
		G_Printf( "timelimit %i is out of range, defaulting to 0\n", g_timelimit.integer );
		trap_Cvar_Set( "timelimit", "0" );
		trap_Cvar_Update( &g_timelimit );
	}

	if ( g_timelimit.integer && !level.warmupTime ) {
		if ( level.time - level.startTime >= g_timelimit.integer * 60000 ) {
			// check for sudden death
			if ( ScoreIsTied() ) {
				// score is tied, so don't end the game
				return;
			}
			trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"" );
			LogExit( "Timelimit hit." );
			return;
		}
	}

	if ( g_fraglimit.integer < 0 ) {
		G_Printf( "fraglimit %i is out of range, defaulting to 0\n", g_fraglimit.integer );
		trap_Cvar_Set( "fraglimit", "0" );
		trap_Cvar_Update( &g_fraglimit );
	}

	if ( g_fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}

		for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			/*if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Fraglimit hit." );
				trap_SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
												cl->pers.netname ) );
				return;
			}*/
		}
	}

	if ( g_capturelimit.integer < 0 ) {
		G_Printf( "capturelimit %i is out of range, defaulting to 8\n", g_capturelimit.integer );
		trap_Cvar_Set( "capturelimit", "8" );
		trap_Cvar_Update( &g_capturelimit );
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=============
CheckTournement

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournement( void ) {

return;

}


/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
	} else {
		if ( level.voteYes > level.numVotingClients / 2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );
			level.voteExecuteTime = level.time + 3000;
		} else if ( level.voteNo >= level.numVotingClients / 2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}

/*
=============
CheckReloadStatus
=============
*/
void CheckReloadStatus( void ) {
	// if we are waiting for a reload, check the delay time
	if ( g_reloading.integer ) {
		if ( level.reloadDelayTime ) {
			if ( level.reloadDelayTime < level.time ) {

				if ( g_reloading.integer == RELOAD_NEXTMAP_WAITING ) {
					trap_Cvar_Set( "g_reloading", va( "%d", RELOAD_NEXTMAP ) ); // set so sv_map_f will know it's okay to start a map
				  if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
					if ( g_cheats.integer ) {
						trap_SendConsoleCommand( EXEC_APPEND, va( "spdevmap %s\n", level.nextMap ) );
					} else {
						trap_SendConsoleCommand( EXEC_APPEND, va( "spmap %s\n", level.nextMap ) );
					} 
				  } else if ( g_gametype.integer == GT_GOTHIC ) {
			        if ( g_cheats.integer ) {
						trap_SendConsoleCommand( EXEC_APPEND, va( "gtdevmap %s\n", level.nextMap ) );
					} else {
						trap_SendConsoleCommand( EXEC_APPEND, va( "gtmap %s\n", level.nextMap ) );
					} 
				  } else if ( g_gametype.integer == GT_SURVIVAL ) {
			        if ( g_cheats.integer ) {
						trap_SendConsoleCommand( EXEC_APPEND, va( "svdevmap %s\n", level.nextMap ) );
					} else {
						trap_SendConsoleCommand( EXEC_APPEND, va( "svmap %s\n", level.nextMap ) );
					} 
				  }
				} else if ( g_reloading.integer == RELOAD_ENDGAME ) {
					G_EndGame();    // kick out to the menu and start the "endgame" menu (credits, etc)

				} else {
					// set the loadgame flag, and restart the server
					trap_Cvar_Set( "savegame_loading", "2" ); // 2 means it's a restart, so stop rendering until we are loaded
					trap_SendConsoleCommand( EXEC_INSERT, "map_restart\n" );
				}

				level.reloadDelayTime = 0;
			}
		} else if ( level.reloadPauseTime ) {
			if ( level.reloadPauseTime < level.time ) {
				trap_Cvar_Set( "g_reloading", "0" );
				level.reloadPauseTime = 0;
			}
		}
	}
}

/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t *ent ) {
	float thinktime;

	// RF, run scripting
	if ( ent->s.number >= MAX_CLIENTS ) {
		ent->scriptStatusCurrent = ent->scriptStatus;
		G_Script_ScriptRun( ent );
	}

	thinktime = ent->nextthink;
	if ( thinktime <= 0 ) {
		return;
	}
	if ( thinktime > level.time ) {
		return;
	}

	ent->nextthink = 0;
	if ( !ent->think ) {
		G_Error( "NULL ent->think" );
	}
	ent->think( ent );
}

void G_RunEntity( gentity_t* ent, int msec );

void G_RunEntity( gentity_t* ent, int msec ) {

	if ( ent->runthisframe ) {
		return;
	}

	ent->runthisframe = qtrue;

	if ( !ent->inuse ) {
		return;
	}

	// check EF_NODRAW status for non-clients
	if ( ent - g_entities > level.maxclients ) {
		if ( ent->flags & FL_NODRAW ) {
			ent->s.eFlags |= EF_NODRAW;
		} else {
			ent->s.eFlags &= ~EF_NODRAW;
		}
	}


	// clear events that are too old
	if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
		if ( ent->s.event ) {
			ent->s.event = 0;
		}
		if ( ent->freeAfterEvent ) {
			// tempEntities or dropped items completely go away after their event
			G_FreeEntity( ent );
			return;
		} else if ( ent->unlinkAfterEvent ) {
			// items that will respawn will hide themselves after their pickup event
			ent->unlinkAfterEvent = qfalse;
			trap_UnlinkEntity( ent );
		}
	}

	// temporary entities don't think
	if ( ent->freeAfterEvent ) {
		return;
	}

	if ( !ent->r.linked && ent->neverFree ) {
		return;
	}

	if ( ent->s.eType == ET_MISSILE
		 || ent->s.eType == ET_FLAMEBARREL
		 || ent->s.eType == ET_FP_PARTS
		 || ent->s.eType == ET_FIRE_COLUMN
		 || ent->s.eType == ET_FIRE_COLUMN_SMOKE
		 || ent->s.eType == ET_EXPLO_PART
		 || ent->s.eType == ET_RAMJET ) {
		return;
	}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
		G_RunItem( ent );

		// ydnar: hack for instantaneous velocity
		VectorSubtract( ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity );
		VectorScale( ent->instantVelocity, 1000.0f / msec, ent->instantVelocity );

		return;
	}

	if ( ent->s.eType == ET_MOVER || ent->s.eType == ET_PROP ) {
		G_RunMover( ent );

		// ydnar: hack for instantaneous velocity
		VectorSubtract( ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity );
		VectorScale( ent->instantVelocity, 1000.0f / msec, ent->instantVelocity );

		return;
	}

	if ( ent - g_entities < MAX_CLIENTS ) {
		G_RunClient( ent );

		// ydnar: hack for instantaneous velocity
		VectorSubtract( ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity );
		VectorScale( ent->instantVelocity, 1000.0f / msec, ent->instantVelocity );

		return;
	}

	if ( ( ent->s.eType == ET_HEALER || ent->s.eType == ET_SUPPLIER ) && ent->target_ent ) {
		ent->target_ent->s.onFireStart =    ent->health;
		ent->target_ent->s.onFireEnd =      ent->count;
	}

	G_RunThink( ent );

	// ydnar: hack for instantaneous velocity
	VectorSubtract( ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity );
	VectorScale( ent->instantVelocity, 1000.0f / msec, ent->instantVelocity );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int i, msec;
	gentity_t   *ent;

	if (steamAlive())
	{
		steamRun();
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	msec = level.time - level.previousTime;

	// Ridah, check for loading a save game
		extern void AICast_CheckLoadGame( void );
		AICast_CheckLoadGame();
	// done.

	// get any cvar changes
	G_UpdateCvars();

	for ( i = 0; i < level.num_entities; i++ ) {
		g_entities[i].runthisframe = qfalse;
	}

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for ( i = 0 ; i < level.num_entities ; i++, ent++ ) {
		G_RunEntity( &g_entities[ i ], msec );
		if ( !ent->inuse ) {
			continue;
		}

		// check EF_NODRAW status for non-clients
		if ( i > level.maxclients ) {
			if ( ent->flags & FL_NODRAW ) {
				ent->s.eFlags |= EF_NODRAW;
			} else {
				ent->s.eFlags &= ~EF_NODRAW;
			}
		}

		// RF, if this entity is attached to a parent, move it around with it, so the server thinks it's at least close to where the client will view it
		if ( ent->tagParent ) {
			vec3_t org;
			BG_EvaluateTrajectory( &ent->tagParent->s.pos, level.time, org );
			G_SetOrigin( ent, org );
			VectorCopy( org, ent->s.origin );
			if ( ent->r.linked ) {    // update position
				trap_LinkEntity( ent );
			}
		}

		// clear events that are too old
		if ( ent->eventTime && level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;   // &= EV_EVENT_BITS;
			}
			// RF, clear all listed events (fixes hearing lots of sounds and events after vid_restart)
			memset( ent->s.events, 0, sizeof( ent->s.events ) );
			memset( ent->s.eventParms, 0, sizeof( ent->s.eventParms ) );
			ent->s.eventSequence = 0;
			if ( ent->client ) {
				memset( ent->client->ps.events, 0, sizeof( ent->client->ps.events ) );
				memset( ent->client->ps.eventParms, 0, sizeof( ent->client->ps.eventParms ) );
				ent->client->ps.eventSequence = 0;
				ent->client->ps.oldEventSequence = 0;
				ent->client->ps.entityEventSequence = 0;
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
			ent->eventTime = 0;
		}

		// MrE: let the server know about bbox or capsule collision
		if ( ent->s.eFlags & EF_CAPSULE ) {
			ent->r.svFlags |= SVF_CAPSULE;
		} else {
			ent->r.svFlags &= ~SVF_CAPSULE;
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE
			 || ent->s.eType == ET_FLAMEBARREL
			 || ent->s.eType == ET_FP_PARTS
			 || ent->s.eType == ET_FIRE_COLUMN
			 || ent->s.eType == ET_FIRE_COLUMN_SMOKE
			 || ent->s.eType == ET_EXPLO_PART
			 || ent->s.eType == ET_RAMJET ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ZOMBIESPIT ) {
			G_RunSpit( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_ALARMBOX ) {
			if ( ent->flags & FL_TEAMSLAVE ) {
				continue;
			}
			G_RunThink( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER || ent->s.eType == ET_PROP ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}

	// Ridah, move the AI
	AICast_StartServerFrame( level.time );

	// perform final fixups on the players
	ent = &g_entities[0];
	for ( i = 0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
//	CheckTeamVote( TEAM_RED );
//	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if ( g_listEntity.integer ) {
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			G_Printf( "%4i: %s\n", i, g_entities[i].classname );
		}
		trap_Cvar_Set( "g_listEntity", "0" );
	}

	// Ridah, check if we are reloading, and times have expired
	CheckReloadStatus();

}
