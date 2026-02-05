/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "framework/Licensee.h"
#include "framework/BuildVersion.h"

#include "GameBase.h"
#include "MultiplayerGame.h"

#include "SysCvar.h"

#if defined( _DEBUG )
	#define	BUILD_DEBUG	"-debug"
#else
	#define	BUILD_DEBUG "-release"
#endif

/*

All game cvars should be defined here.

*/

#ifdef CTF
const char *si_gameTypeArgs[]		= { "singleplayer", "deathmatch", "Tourney", "Team DM", "Last Man", "CTF", NULL };
#else
const char *si_gameTypeArgs[]		= { "singleplayer", "deathmatch", "Tourney", "Team DM", "Last Man", NULL };
#endif

const char *si_readyArgs[]			= { "Not Ready", "Ready", NULL };
const char *si_spectateArgs[]		= { "Play", "Spectate", NULL };

#ifdef _D3XP
const char *ui_skinArgs[]			= { "skins/characters/player/marine_mp", "skins/characters/player/marine_mp_red", "skins/characters/player/marine_mp_blue", "skins/characters/player/marine_mp_green", "skins/characters/player/marine_mp_yellow", "skins/characters/player/marine_mp_purple", "skins/characters/player/marine_mp_grey", "skins/characters/player/marine_mp_orange", NULL };
#else
const char *ui_skinArgs[]			= { "skins/characters/player/marine_mp", "skins/characters/player/marine_mp_red", "skins/characters/player/marine_mp_blue", "skins/characters/player/marine_mp_green", "skins/characters/player/marine_mp_yellow", NULL };
#endif

const char *ui_teamArgs[]			= { "Red", "Blue", NULL };

struct gameVersion_s {
	gameVersion_s( void ) { sprintf( string, "%s.%d-%s %s-%s %s %s", ENGINE_VERSION, BUILD_NUMBER, CMAKE_INTDIR, BUILD_OS, BUILD_CPU, __DATE__, __TIME__ ); }
	char	string[256];
} gameVersion;

#define YEARINDEX 2
#define MONTHINDEX 0
#define DAYINDEX 1

#define HOURINDEX 0
#define MINUTEINDEX 1

// SW 10th March 2025:
// __DATE__ is formatted as Mmm dd yyyy but the dd has a leading space if the date number is less than 10
// As you can probably tell by the date on this comment, I have run afoul of this. Fixing now.
struct gameVersionShort_s {
	gameVersionShort_s()
	{
		idStr date = __DATE__;
		idStr time = __TIME__;
		bool leadingSpace = false;

		idList<idStr> dateSplits = date.Split(' ');
		if (dateSplits.Num() > 3) // SW 10th March 2025
		{
			leadingSpace = true;
		}

		idList<idStr> months;
		months.Append("");
		months.Append("Jan");
		months.Append("Feb");
		months.Append("Mar");
		months.Append("Apr");
		months.Append("May");
		months.Append("Jun");
		months.Append("Jul");
		months.Append("Aug");
		months.Append("Sep");
		months.Append("Oct");
		months.Append("Nov");
		months.Append("Dec");

		string = dateSplits[YEARINDEX + (leadingSpace ? 1 : 0)]; //Year.
		string += '.';
		for (int i = 0; i < months.Num(); i++)
		{
			if (months[i] == dateSplits[MONTHINDEX]) //month
			{				
				string += idStr::Format("%02d", i);
			}
		}
		string += '.';

		if (dateSplits[DAYINDEX + (leadingSpace ? 1 : 0)].Length() <= 1)
			string += idStr::Format("0%s", dateSplits[DAYINDEX + (leadingSpace ? 1 : 0)].c_str()); //day
		else
			string += dateSplits[DAYINDEX + (leadingSpace ? 1 : 0)]; //day

		string += '.';

		idList<idStr> timeSplits = time.Split(':');

		if (timeSplits[HOURINDEX].Length() <= 1)
			string += idStr::Format("0%s", timeSplits[HOURINDEX].c_str()); //hour
		else
			string += timeSplits[HOURINDEX]; //hour

		if (timeSplits[MINUTEINDEX].Length() <= 1)
			string += idStr::Format("0%s", timeSplits[MINUTEINDEX].c_str()); //minute
		else
			string += timeSplits[MINUTEINDEX]; //minute

	}
	idStr	string;
} gameVersionShort;

idCVar g_version(					"g_version",				gameVersion.string,	CVAR_GAME | CVAR_ROM, "game version" );
idCVar g_versionShort(				"g_versionShort",			gameVersionShort.string, CVAR_GAME | CVAR_ROM, "game version (short)");

// noset vars
idCVar gamename(					"gamename",					GAME_VERSION,	CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM, "" );
idCVar gamedate(					"gamedate",					__DATE__,		CVAR_GAME | CVAR_ROM, "" );

// server info
idCVar si_name(						"si_name",					"dhewm server",	CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "name of the server" );

#ifdef CTF
idCVar si_gameType(					"si_gameType",		si_gameTypeArgs[ 0 ],	CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "game type - singleplayer, deathmatch, Tourney, Team DM, Last Man, or CTF", si_gameTypeArgs, idCmdSystem::ArgCompletion_String<si_gameTypeArgs> );
#else
idCVar si_gameType(					"si_gameType",		si_gameTypeArgs[ 0 ],	CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "game type - singleplayer, deathmatch, Tourney, Team DM or Last Man", si_gameTypeArgs, idCmdSystem::ArgCompletion_String<si_gameTypeArgs> );
#endif


idCVar si_map(						"si_map",					"game/mp/d3dm1",CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "map to be played next on server", idCmdSystem::ArgCompletion_MapName );

idCVar si_maxPlayers(				"si_maxPlayers",			"32",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "max number of players allowed on the server", 1, 8 );
idCVar si_fragLimit(				"si_fragLimit",				"10",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "frag limit", 1, MP_PLAYER_MAXFRAGS );
idCVar si_timeLimit(				"si_timeLimit",				"10",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "time limit in minutes", 0, 60 );
idCVar si_teamDamage(				"si_teamDamage",			"0",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL, "enable team damage" );
idCVar si_warmup(					"si_warmup",				"0",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL, "do pre-game warmup" );
idCVar si_usePass(					"si_usePass",				"0",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL, "enable client password checking" );
idCVar si_pure(						"si_pure",					"1",			CVAR_GAME | CVAR_SERVERINFO | CVAR_BOOL, "server is pure and does not allow modified data" );
idCVar si_spectators(				"si_spectators",			"1",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL, "allow spectators or require all clients to play" );
idCVar si_serverURL(				"si_serverURL",				"",				CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "where to reach the server admins and get information about the server" );

#ifdef CTF
//idCVar si_pointLimit(				"si_pointlimit",			"8",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "team points limit to win in CTF" );
idCVar si_flagDropTimeLimit(		"si_flagDropTimeLimit",		"30",			CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "seconds before a dropped CTF flag is returned" );
idCVar si_midnight(                 "si_midnight",              "0",            CVAR_GAME | CVAR_INTEGER | CVAR_SERVERINFO, "Start the game up in midnight CTF (completely dark)" );
#endif


// user info
idCVar ui_name(						"ui_name",					"Player",		CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE, "player name" );
idCVar ui_skin(						"ui_skin",				ui_skinArgs[ 0 ],	CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE, "player skin", ui_skinArgs, idCmdSystem::ArgCompletion_String<ui_skinArgs> );
idCVar ui_team(						"ui_team",				ui_teamArgs[ 0 ],	CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE, "player team", ui_teamArgs, idCmdSystem::ArgCompletion_String<ui_teamArgs> );
idCVar ui_autoSwitch(				"ui_autoSwitch",			"0",			CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "auto switch weapon" );
idCVar ui_autoReload(				"ui_autoReload",			"0",			CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "auto reload weapon" );
idCVar ui_showGun(					"ui_showGun",				"1",			CVAR_GAME | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "(DEPRECATED NOT IN USE) show gun" );
idCVar ui_ready(					"ui_ready",				si_readyArgs[ 0 ],	CVAR_GAME | CVAR_USERINFO, "player is ready to start playing", idCmdSystem::ArgCompletion_String<si_readyArgs> );
idCVar ui_spectate(					"ui_spectate",		si_spectateArgs[ 0 ],	CVAR_GAME | CVAR_USERINFO, "play or spectate", idCmdSystem::ArgCompletion_String<si_spectateArgs> );
idCVar ui_chat(						"ui_chat",					"0",			CVAR_GAME | CVAR_USERINFO | CVAR_BOOL | CVAR_ROM | CVAR_CHEAT, "player is chatting" );

// change anytime vars
idCVar developer(					"developer",				"0",			CVAR_GAME | CVAR_INTEGER, "" );

idCVar r_aspectRatio(				"r_aspectRatio",			"-1",			CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "aspect ratio of view:\n0 = 4:3\n1 = 16:9\n2 = 16:10\n-1 = auto (guess from resolution)", -1, 2 );

idCVar g_cinematic(					"g_cinematic",				"1",			CVAR_GAME | CVAR_BOOL, "skips updating entities that aren't marked 'cinematic' '1' during cinematics" );
idCVar g_cinematicMaxSkipTime(		"g_cinematicMaxSkipTime",	"600",			CVAR_GAME | CVAR_FLOAT, "# of seconds to allow game to run when skipping cinematic.  prevents lock-up when cinematic doesn't end.", 0, 3600 );

idCVar g_muzzleFlash(				"g_muzzleFlash",			"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show muzzle flashes" );
idCVar g_projectileLights(			"g_projectileLights",		"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show dynamic lights on projectiles" );
idCVar g_bloodEffects(				"g_bloodEffects",			"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show blood splats, sprays and gibs" );
idCVar g_doubleVision(				"g_doubleVision",			"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show double vision when taking damage" );
idCVar g_monsters(					"g_monsters",				"1",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_decals(					"g_decals",					"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show decals such as bullet holes" );
idCVar g_knockback(					"g_knockback",				"1000",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_skill(						"g_skill",					"1",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_nightmare(					"g_nightmare",				"0",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "if nightmare mode is allowed" );
idCVar g_gravity(					"g_gravity",		DEFAULT_GRAVITY_STRING, CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_skipFX(					"g_skipFX",					"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_skipParticles(				"g_skipParticles",			"0",			CVAR_GAME | CVAR_BOOL, "" );

idCVar g_disasm(					"g_disasm",					"0",			CVAR_GAME | CVAR_BOOL, "disassemble script into base/script/disasm.txt on the local drive when script is compiled" );
idCVar g_debugBounds(				"g_debugBounds",			"0",			CVAR_GAME | CVAR_BOOL, "checks for models with bounds > 2048" );
idCVar g_debugAnim(					"g_debugAnim",				"-1",			CVAR_GAME | CVAR_INTEGER, "displays information on which animations are playing on the specified entity number.  set to -1 to disable." );
idCVar g_debugMove(					"g_debugMove",				"0",			CVAR_GAME | CVAR_BOOL, "Debug player movement." );
idCVar g_debugDamage(				"g_debugDamage",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugWeapon(				"g_debugWeapon",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugScript(				"g_debugScript",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugMover(				"g_debugMover",				"0",			CVAR_GAME | CVAR_BOOL, "Debug idMover entities." );
idCVar g_debugTriggers(				"g_debugTriggers",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugCinematic(			"g_debugCinematic",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_stopTime(					"g_stopTime",				"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_damageScale(				"g_damageScale",			"1",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, "scale final damage on player by this factor" );
idCVar g_armorProtection(			"g_armorProtection",		"1",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, "armor takes this percentage of damage" );
idCVar g_armorProtectionMP(			"g_armorProtectionMP",		"0.6",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, "armor takes this percentage of damage in mp" );
idCVar g_useDynamicProtection(		"g_useDynamicProtection",	"1",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "scale damage and armor dynamically to keep the player alive more often" );
idCVar g_healthTakeTime(			"g_healthTakeTime",			"5",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "how often to take health in nightmare mode" );
idCVar g_healthTakeAmt(				"g_healthTakeAmt",			"5",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "how much health to take in nightmare mode" );
idCVar g_healthTakeLimit(			"g_healthTakeLimit",		"25",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "how low can health get taken in nightmare mode" );
idCVar g_locbox(					"g_locbox",					"1",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "Localization box. 0 = always off, 1 = automatic, 2 = always on");
idCVar g_locbox_minscreensize(		"g_locbox_minscreensize",	"1500",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, "Minimum area (in screen space coordinates) that a locbox must occupy before it is displayed."); // SW 12th March 2025
idCVar g_onehitkill(				"g_onehitkill",				"0",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "Toggle one-hit kill accessibility option.");


idCVar g_showPVS(					"g_showPVS",				"0",			CVAR_GAME | CVAR_INTEGER, "", 0, 2 );
idCVar g_showTargets(				"g_showTargets",			"0",			CVAR_GAME | CVAR_BOOL, "draws entities and thier targets.  hidden entities are drawn grey." );
idCVar g_showTriggers(				"g_showTriggers",			"0",			CVAR_GAME | CVAR_BOOL, "draws trigger entities (orange) and thier targets (green).  disabled triggers are drawn grey." );
idCVar g_showCollisionWorld(		"g_showCollisionWorld",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showCollisionModels(		"g_showCollisionModels",	"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showCollisionTraces(		"g_showCollisionTraces",	"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_maxShowDistance(			"g_maxShowDistance",		"128",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_showEntityInfo(			"g_showEntityInfo",			"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_showviewpos(				"g_showviewpos",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showcamerainfo(			"g_showcamerainfo",			"0",			CVAR_GAME | CVAR_ARCHIVE, "displays the current frame # for the camera when playing cinematics" );
idCVar g_showTestModelFrame(		"g_showTestModelFrame",		"0",			CVAR_GAME | CVAR_BOOL, "displays the current animation and frame # for testmodels" );
idCVar g_showActiveEntities(		"g_showActiveEntities",		"0",			CVAR_GAME | CVAR_BOOL, "draws boxes around thinking entities.  dormant entities (outside of pvs) are drawn yellow.  non-dormant are green." );
idCVar g_showEnemies(				"g_showEnemies",			"0",			CVAR_GAME | CVAR_BOOL, "draws boxes around monsters that have targeted the the player" );

idCVar g_frametime(					"g_frametime",				"0",			CVAR_GAME | CVAR_BOOL, "displays timing information for each game frame" );
idCVar g_timeentities(				"g_timeEntities",			"0",			CVAR_GAME | CVAR_FLOAT, "when non-zero, shows entities whose think functions exceeded the # of milliseconds specified" );

#ifdef _D3XP
idCVar g_testPistolFlashlight(		"g_testPistolFlashlight",	"1",			CVAR_GAME | CVAR_BOOL, "Test out having a flashlight out with the pistol" );
idCVar g_debugShockwave(			"g_debugShockwave",			"0",			CVAR_GAME | CVAR_BOOL, "Debug the shockwave" );

idCVar g_enableSlowmo(				"g_enableSlowmo",			"0",			CVAR_GAME | CVAR_BOOL, "for testing purposes only" );
idCVar g_slowmoStepRate(			"g_slowmoStepRate",			"0.1",			CVAR_GAME | CVAR_FLOAT, "Rate at which slowmo ramps on/off." );

idCVar g_enablePortalSky(			"g_enablePortalSky",		"1",			CVAR_GAME | CVAR_BOOL, "enables the portal sky" );
idCVar g_testFullscreenFX(			"g_testFullscreenFX",		"-1",			CVAR_GAME | CVAR_INTEGER, "index will activate specific fx, -2 is for all on, -1 is off" );
idCVar g_testHelltimeFX(			"g_testHelltimeFX",			"-1",			CVAR_GAME | CVAR_INTEGER, "set to 0, 1, 2 to test helltime, -1 is off" );
idCVar g_testMultiplayerFX(			"g_testMultiplayerFX",		"-1",			CVAR_GAME | CVAR_INTEGER, "set to 0, 1, 2 to test multiplayer, -1 is off" );
idCVar g_lowresFullscreenFX(		"g_lowresFullscreenFX",		"0",			CVAR_GAME | CVAR_BOOL, "enable lores mode for fx" );

idCVar g_moveableDamageScale(		"g_moveableDamageScale",	"0.1",			CVAR_GAME | CVAR_FLOAT, "scales damage wrt mass of object in multiplayer" );

idCVar g_testBloomSpeed(			"g_testBloomSpeed",			"1",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_testBloomIntensity(		"g_testBloomIntensity",		"-0.01",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_testBloomNumPasses(		"g_testBloomNumPasses",		"20",			CVAR_GAME | CVAR_INTEGER, "" );
#endif

idCVar ai_debugScript(				"ai_debugScript",			"-1",			CVAR_GAME | CVAR_INTEGER, "displays script calls for the specified monster entity number" );
idCVar ai_debugMove(				"ai_debugMove",				"0",			CVAR_GAME | CVAR_INTEGER, "draws movement information for monsters" );
idCVar ai_debugTrajectory(			"ai_debugTrajectory",		"0",			CVAR_GAME | CVAR_BOOL, "draws trajectory tests for monsters" );
idCVar ai_testPredictPath(			"ai_testPredictPath",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar ai_showCombatNodes(			"ai_showCombatNodes",		"0",			CVAR_GAME | CVAR_BOOL, "draws attack cones for monsters" );
idCVar ai_showPaths(				"ai_showPaths",				"0",			CVAR_GAME | CVAR_BOOL, "draws path_* entities" );
idCVar ai_showObstacleAvoidance(	"ai_showObstacleAvoidance",	"0",			CVAR_GAME | CVAR_INTEGER, "draws obstacle avoidance information for monsters.  if 2, draws obstacles for player, as well", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar ai_blockedFailSafe(			"ai_blockedFailSafe",		"1",			CVAR_GAME | CVAR_BOOL, "enable blocked fail safe handling" );

#ifdef _D3XP
idCVar ai_showHealth(				"ai_showHealth",			"0",			CVAR_GAME | CVAR_BOOL, "Draws the AI's health above its head" );
#endif



idCVar g_dvTime(					"g_dvTime",					"2",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_dvAmplitude(				"g_dvAmplitude",			"0.0005",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_dvFrequency(				"g_dvFrequency",			"0.5",			CVAR_GAME | CVAR_FLOAT, "" );

idCVar g_kickTime(					"g_kickTime",				"1",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_kickAmplitude(				"g_kickAmplitude",			"0.0001",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_blobTime(					"g_blobTime",				"1",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_blobSize(					"g_blobSize",				"1",			CVAR_GAME | CVAR_FLOAT, "" );

idCVar g_testHealthVision(			"g_testHealthVision",		"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_editEntityMode(			"g_editEntityMode",			"0",			CVAR_GAME | CVAR_INTEGER,	"0 = off\n"
																											"1 = lights\n"
																											"2 = sounds\n"
																											"3 = articulated figures\n"
																											"4 = particle systems\n"
																											"5 = monsters\n"
																											"6 = entity names\n"
																											"7 = entity models", 0, 7, idCmdSystem::ArgCompletion_Integer<0,7> );
idCVar g_dragEntity(				"g_dragEntity",				"0",			CVAR_GAME | CVAR_INTEGER, "allows dragging physics objects around by placing the crosshair over them and holding the fire button" );
idCVar g_dragDamping(				"g_dragDamping",			"0.5",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_dragShowSelection(			"g_dragShowSelection",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_dropItemRotation(			"g_dropItemRotation",		"",				CVAR_GAME, "" );

#ifdef CTF
// Note: These cvars do not necessarily need to be in the shipping game.
idCVar g_flagAttachJoint( "g_flagAttachJoint", "Chest", CVAR_GAME | CVAR_CHEAT, "player joint to attach CTF flag to" );
idCVar g_flagAttachOffsetX( "g_flagAttachOffsetX", "8", CVAR_GAME | CVAR_CHEAT, "X offset of CTF flag when carried" );
idCVar g_flagAttachOffsetY( "g_flagAttachOffsetY", "4", CVAR_GAME | CVAR_CHEAT, "Y offset of CTF flag when carried" );
idCVar g_flagAttachOffsetZ( "g_flagAttachOffsetZ", "-12", CVAR_GAME | CVAR_CHEAT, "Z offset of CTF flag when carried" );
idCVar g_flagAttachAngleX( "g_flagAttachAngleX", "90", CVAR_GAME | CVAR_CHEAT, "X angle of CTF flag when carried" );
idCVar g_flagAttachAngleY( "g_flagAttachAngleY", "25", CVAR_GAME | CVAR_CHEAT, "Y angle of CTF flag when carried" );
idCVar g_flagAttachAngleZ( "g_flagAttachAngleZ", "-90", CVAR_GAME | CVAR_CHEAT, "Z angle of CTF flag when carried" );
#endif


idCVar g_vehicleVelocity(			"g_vehicleVelocity",		"1000",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleForce(				"g_vehicleForce",			"50000",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionUp(		"g_vehicleSuspensionUp",	"32",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionDown(		"g_vehicleSuspensionDown",	"20",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionKCompress("g_vehicleSuspensionKCompress","200",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionDamping(	"g_vehicleSuspensionDamping","400",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleTireFriction(		"g_vehicleTireFriction",	"0.8",			CVAR_GAME | CVAR_FLOAT, "" );
#ifdef _D3XP
idCVar g_vehicleDebug(				"g_vehicleDebug",			"0",			CVAR_GAME | CVAR_BOOL, "" );
#endif

idCVar ik_enable(					"ik_enable",				"1",			CVAR_GAME | CVAR_BOOL, "enable IK" );
idCVar ik_debug(					"ik_debug",					"0",			CVAR_GAME | CVAR_BOOL, "show IK debug lines" );

idCVar af_useLinearTime(			"af_useLinearTime",			"1",			CVAR_GAME | CVAR_BOOL, "use linear time algorithm for tree-like structures" );
idCVar af_useImpulseFriction(		"af_useImpulseFriction",	"0",			CVAR_GAME | CVAR_BOOL, "use impulse based contact friction" );
idCVar af_useJointImpulseFriction(	"af_useJointImpulseFriction","0",			CVAR_GAME | CVAR_BOOL, "use impulse based joint friction" );
idCVar af_useSymmetry(				"af_useSymmetry",			"1",			CVAR_GAME | CVAR_BOOL, "use constraint matrix symmetry" );
idCVar af_skipSelfCollision(		"af_skipSelfCollision",		"0",			CVAR_GAME | CVAR_BOOL, "skip self collision detection" );
idCVar af_skipLimits(				"af_skipLimits",			"0",			CVAR_GAME | CVAR_BOOL, "skip joint limits" );
idCVar af_skipFriction(				"af_skipFriction",			"0",			CVAR_GAME | CVAR_BOOL, "skip friction" );
idCVar af_forceFriction(			"af_forceFriction",			"-1",			CVAR_GAME | CVAR_FLOAT, "force the given friction value" );
idCVar af_maxLinearVelocity(		"af_maxLinearVelocity",		"128",			CVAR_GAME | CVAR_FLOAT, "maximum linear velocity" );
idCVar af_maxAngularVelocity(		"af_maxAngularVelocity",	"1.57",			CVAR_GAME | CVAR_FLOAT, "maximum angular velocity" );
idCVar af_timeScale(				"af_timeScale",				"1",			CVAR_GAME | CVAR_FLOAT, "scales the time" );
idCVar af_jointFrictionScale(		"af_jointFrictionScale",	"0",			CVAR_GAME | CVAR_FLOAT, "scales the joint friction" );
idCVar af_contactFrictionScale(		"af_contactFrictionScale",	"0",			CVAR_GAME | CVAR_FLOAT, "scales the contact friction" );
idCVar af_highlightBody(			"af_highlightBody",			"",				CVAR_GAME, "name of the body to highlight" );
idCVar af_highlightConstraint(		"af_highlightConstraint",	"",				CVAR_GAME, "name of the constraint to highlight" );
idCVar af_showTimings(				"af_showTimings",			"0",			CVAR_GAME | CVAR_BOOL, "show articulated figure cpu usage" );
idCVar af_showConstraints(			"af_showConstraints",		"0",			CVAR_GAME | CVAR_BOOL, "show constraints" );
idCVar af_showConstraintNames(		"af_showConstraintNames",	"0",			CVAR_GAME | CVAR_BOOL, "show constraint names" );
idCVar af_showConstrainedBodies(	"af_showConstrainedBodies",	"0",			CVAR_GAME | CVAR_BOOL, "show the two bodies contrained by the highlighted constraint" );
idCVar af_showPrimaryOnly(			"af_showPrimaryOnly",		"0",			CVAR_GAME | CVAR_BOOL, "show primary constraints only" );
idCVar af_showTrees(				"af_showTrees",				"0",			CVAR_GAME | CVAR_BOOL, "show tree-like structures" );
idCVar af_showLimits(				"af_showLimits",			"0",			CVAR_GAME | CVAR_BOOL, "show joint limits" );
idCVar af_showBodies(				"af_showBodies",			"0",			CVAR_GAME | CVAR_BOOL, "show bodies" );
idCVar af_showBodyNames(			"af_showBodyNames",			"0",			CVAR_GAME | CVAR_BOOL, "show body names" );
idCVar af_showMass(					"af_showMass",				"0",			CVAR_GAME | CVAR_BOOL, "show the mass of each body" );
idCVar af_showTotalMass(			"af_showTotalMass",			"0",			CVAR_GAME | CVAR_BOOL, "show the total mass of each articulated figure" );
idCVar af_showInertia(				"af_showInertia",			"0",			CVAR_GAME | CVAR_BOOL, "show the inertia tensor of each body" );
idCVar af_showVelocity(				"af_showVelocity",			"0",			CVAR_GAME | CVAR_BOOL, "show the velocity of each body" );
idCVar af_showActive(				"af_showActive",			"0",			CVAR_GAME | CVAR_BOOL, "show tree-like structures of articulated figures not at rest" );
idCVar af_testSolid(				"af_testSolid",				"1",			CVAR_GAME | CVAR_BOOL, "test for bodies initially stuck in solid" );

idCVar rb_showTimings(				"rb_showTimings",			"0",			CVAR_GAME | CVAR_BOOL, "show rigid body cpu usage" );
idCVar rb_showBodies(				"rb_showBodies",			"0",			CVAR_GAME | CVAR_BOOL, "show rigid bodies" );
idCVar rb_showMass(					"rb_showMass",				"0",			CVAR_GAME | CVAR_BOOL, "show the mass of each rigid body" );
idCVar rb_showInertia(				"rb_showInertia",			"0",			CVAR_GAME | CVAR_BOOL, "show the inertia tensor of each rigid body" );
idCVar rb_showVelocity(				"rb_showVelocity",			"0",			CVAR_GAME | CVAR_BOOL, "show the velocity of each rigid body" );
idCVar rb_showActive(				"rb_showActive",			"0",			CVAR_GAME | CVAR_BOOL, "show rigid bodies that are not at rest" );

// The default values for player movement cvars are set in def/player.def
idCVar pm_jumpheight(				"pm_jumpheight",			"48",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "approximate hieght the player can jump" );
idCVar pm_stepsize(					"pm_stepsize",				"16",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "maximum height the player can step up without jumping" );
idCVar pm_crouchspeed(				"pm_crouchspeed",			"100",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while crouched" );
idCVar pm_walkspeed(				"pm_walkspeed",				"150",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while walking" );
idCVar pm_runspeed(					"pm_runspeed",				"1024",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while running" );
idCVar pm_noclipspeed(				"pm_noclipspeed",			"300",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while in noclip" );
idCVar pm_spectatespeed(			"pm_spectatespeed",			"300",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while spectating" );
idCVar pm_spectatebbox(				"pm_spectatebbox",			"32",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "size of the spectator bounding box" );
idCVar pm_usecylinder(				"pm_usecylinder",			"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "use a cylinder approximation instead of a bounding box for player collision detection" );
idCVar pm_minviewpitch(				"pm_minviewpitch",			"-89",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "amount player's view can look up (negative values are up)" );
idCVar pm_maxviewpitch(				"pm_maxviewpitch",			"89",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "amount player's view can look down" );
idCVar pm_stamina(					"pm_stamina",				"100",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "length of time player can run" );



idCVar pm_staminathreshold(			"pm_staminathreshold",		"45",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "when stamina drops below this value, player gradually slows to a walk" );
idCVar pm_staminarate(				"pm_staminarate",			"256",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "rate that player regains stamina. divide pm_stamina by this value to determine how long it takes to fully recharge." );
idCVar pm_crouchheight(				"pm_crouchheight",			"19",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's bounding box while crouched" );
idCVar pm_crouchviewheight(			"pm_crouchviewheight",		"16",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's view while crouched" );
idCVar pm_normalheight(				"pm_normalheight",			"74",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's bounding box while standing" );
idCVar pm_normalviewheight(			"pm_normalviewheight",		"68",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's view while standing" );
idCVar pm_deadheight(				"pm_deadheight",			"20",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's bounding box while dead" );
idCVar pm_deadviewheight(			"pm_deadviewheight",		"10",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of player's view while dead" );
idCVar pm_crouchrate(				"pm_crouchrate",			"0.15",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "fraction of the distance between crouching and standing to move per frame" );
idCVar pm_bboxwidth(				"pm_bboxwidth",				"32",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "x/y size of player's bounding box" );
idCVar pm_crouchbob(				"pm_crouchbob",				"0.3",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "bob much faster when crouched" );
idCVar pm_walkbob(					"pm_walkbob",				"0.3",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "bob slowly when walking" );
idCVar pm_runbob(					"pm_runbob",				"0.4",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "bob faster when running" );
idCVar pm_runpitch(					"pm_runpitch",				"0.002",		CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "" );
idCVar pm_runroll(					"pm_runroll",				"0.005",		CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "" );
idCVar pm_bobup(					"pm_bobup",					"0.005",		CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "" );
idCVar pm_bobpitch(					"pm_bobpitch",				"0.002",		CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "" );
idCVar pm_bobroll(					"pm_bobroll",				"0.002",		CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "" );
idCVar pm_bobstepup(				"pm_bobstepup",				"4",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "how far head cocks forward and up on stairs, etc");
idCVar pm_thirdPersonRange(			"pm_thirdPersonRange",		"60",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "camera distance from player in 3rd person" );
idCVar pm_thirdPersonHeight(		"pm_thirdPersonHeight",		"-5",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "height of camera from normal view height in 3rd person" );
idCVar pm_thirdPersonAngle(			"pm_thirdPersonAngle",		"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "direction of camera from player in 3rd person in degrees (0 = behind player, 180 = in front)" );
idCVar pm_thirdPersonClip(			"pm_thirdPersonClip",		"1",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "clip third person view into world space" );
idCVar pm_thirdPerson(				"pm_thirdPerson",			"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "enables third person view" );
idCVar pm_thirdPersonDeath(			"pm_thirdPersonDeath",		"1",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "enables third person view when player dies" );
idCVar pm_modelView(				"pm_modelView",				"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_INTEGER, "draws camera from POV of player model (1 = always, 2 = when dead)", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar pm_airTics(					"pm_air",					"5400",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_INTEGER, "how much air in air tank. Multiply this value by 16.6666666 in order to get total milliseconds." );
idCVar pm_airlessCrouch(			"pm_airlessCrouch",			"1",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "enables force crouching in zero-g" );



idCVar g_showPlayerShadow(			"g_showPlayerShadow",		"1",			CVAR_GAME | CVAR_BOOL, "enables shadow of player model" );
idCVar g_showHud(					"g_showHud",				"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "" );
idCVar g_showProjectilePct(			"g_showProjectilePct",		"0",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enables display of player hit percentage" );
idCVar g_showBrass(					"g_showBrass",				"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enables ejected shells from weapon" );
idCVar g_gun_x(						"g_gunX",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_gun_y(						"g_gunY",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_gun_z(						"g_gunZ",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_viewNodalX(				"g_viewNodalX",				"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_viewNodalZ(				"g_viewNodalZ",				"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_fov(						"g_fov",					"90",			CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT | CVAR_ARCHIVE, "" ); //BC 5-2-2025: fov is now saved between sessions.
idCVar g_skipViewEffects(			"g_skipViewEffects",		"0",			CVAR_GAME | CVAR_BOOL, "skip damage and other view effects" );
idCVar g_mpWeaponAngleScale(		"g_mpWeaponAngleScale",		"0",			CVAR_GAME | CVAR_FLOAT, "Control the weapon sway in MP" );

idCVar g_testParticle(				"g_testParticle",			"0",			CVAR_GAME | CVAR_INTEGER, "test particle visualation, set by the particle editor" );
idCVar g_testParticleName(			"g_testParticleName",		"",				CVAR_GAME, "name of the particle being tested by the particle editor" );
idCVar g_testModelRotate(			"g_testModelRotate",		"0",			CVAR_GAME, "test model rotation speed" );
idCVar g_testPostProcess(			"g_testPostProcess",		"",				CVAR_GAME, "name of material to draw over screen" );
idCVar g_testModelAnimate(			"g_testModelAnimate",		"0",			CVAR_GAME | CVAR_INTEGER, "test model animation,\n"
																							"0 = cycle anim with origin reset\n"
																							"1 = cycle anim with fixed origin\n"
																							"2 = cycle anim with continuous origin\n"
																							"3 = frame by frame with continuous origin\n"
																							"4 = play anim once", 0, 4, idCmdSystem::ArgCompletion_Integer<0,4> );
idCVar g_testModelBlend(			"g_testModelBlend",			"0",			CVAR_GAME | CVAR_INTEGER, "number of frames to blend" );
idCVar g_testDeath(					"g_testDeath",				"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_exportMask(				"g_exportMask",				"",				CVAR_GAME, "" );
idCVar g_flushSave(					"g_flushSave",				"0",			CVAR_GAME | CVAR_BOOL, "1 = don't buffer file writing for save games." );

idCVar aas_test(					"aas_test",					"0",			CVAR_GAME | CVAR_INTEGER, "Index of the AAS type to load for debugging.\nCheck the aas_types entity definition for a list.\nDefault should be 0 (aas24)." );
idCVar aas_showAreas(				"aas_showAreas",			"0",			CVAR_GAME | CVAR_INTEGER, "1 = show AAS areas and neighboring areas. 2 = show only single AAS area." );
idCVar aas_showPath(				"aas_showPath",				"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_showFlyPath(				"aas_showFlyPath",			"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_showWallEdges(			"aas_showWallEdges",		"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_showHideArea(			"aas_showHideArea",			"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_pullPlayer(				"aas_pullPlayer",			"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_randomPullPlayer(		"aas_randomPullPlayer",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar aas_goalArea(				"aas_goalArea",				"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar aas_showPushIntoArea(		"aas_showPushIntoArea",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar aas_showSingleArea(			"aas_showSingleArea",		"-1",			CVAR_GAME | CVAR_INTEGER, "Show a single aas cell, by its area number."); //bc

idCVar g_password(					"g_password",				"",				CVAR_GAME | CVAR_ARCHIVE, "game password" );
idCVar password(					"password",					"",				CVAR_GAME | CVAR_NOCHEAT, "client password used when connecting" );

idCVar g_countDown(					"g_countDown",				"5",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "pregame countdown in seconds", 4, 3600 );
idCVar g_gameReviewPause(			"g_gameReviewPause",		"10",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_INTEGER | CVAR_ARCHIVE, "scores review time in seconds (at end game)", 2, 3600 );
idCVar g_TDMArrows(					"g_TDMArrows",				"1",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "draw arrows over teammates in team deathmatch" );
idCVar g_balanceTDM(				"g_balanceTDM",				"1",			CVAR_GAME | CVAR_BOOL, "maintain even teams" );
#ifdef CTF
idCVar g_CTFArrows(					"g_CTFArrows",				"1",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "draw arrows over teammates in CTF" );
#endif

idCVar net_clientPredictGUI(		"net_clientPredictGUI",		"1",			CVAR_GAME | CVAR_BOOL, "test guis in networking without prediction" );

idCVar g_voteFlags(					"g_voteFlags",				"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_INTEGER | CVAR_ARCHIVE, "vote flags. bit mask of votes not allowed on this server\n"
																					"bit 0 (+1)   restart now\n"
																					"bit 1 (+2)   time limit\n"
																					"bit 2 (+4)   frag limit\n"
																					"bit 3 (+8)   game type\n"
																					"bit 4 (+16)  kick player\n"
																					"bit 5 (+32)  change map\n"
																					"bit 6 (+64)  spectators\n"
																					"bit 7 (+128) next map" );
idCVar g_mapCycle(					"g_mapCycle",				"mapcycle",		CVAR_GAME | CVAR_ARCHIVE, "map cycling script for multiplayer games - see mapcycle.scriptcfg" );

#ifdef _D3XP
idCVar mod_validSkins(				"mod_validSkins",			"skins/characters/player/marine_mp;skins/characters/player/marine_mp_green;skins/characters/player/marine_mp_blue;skins/characters/player/marine_mp_red;skins/characters/player/marine_mp_yellow;skins/characters/player/marine_mp_purple;skins/characters/player/marine_mp_grey;skins/characters/player/marine_mp_orange",		CVAR_GAME | CVAR_ARCHIVE, "valid skins for the game" );
#else
idCVar mod_validSkins(				"mod_validSkins",			"skins/characters/player/marine_mp;skins/characters/player/marine_mp_green;skins/characters/player/marine_mp_blue;skins/characters/player/marine_mp_red;skins/characters/player/marine_mp_yellow",		CVAR_GAME | CVAR_ARCHIVE, "valid skins for the game" );
#endif


#ifdef _D3XP
idCVar g_grabberHoldSeconds(		"g_grabberHoldSeconds",		"3",			CVAR_GAME | CVAR_FLOAT | CVAR_CHEAT, "number of seconds to hold object" );
idCVar g_grabberEnableShake(		"g_grabberEnableShake",		"1",			CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "enable the grabber shake" );
idCVar g_grabberRandomMotion(		"g_grabberRandomMotion",	"1",			CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "enable random motion on the grabbed object" );
idCVar g_grabberHardStop(			"g_grabberHardStop",		"1",			CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "hard stops object if too fast" );
idCVar g_grabberDamping(			"g_grabberDamping",			"0.5",			CVAR_GAME | CVAR_FLOAT | CVAR_CHEAT, "damping of grabber" );
#endif

#ifdef _D3XP
idCVar g_xp_bind_run_once( "g_xp_bind_run_once", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "Rebind all controls once for D3XP." );
#endif

idCVar net_serverDownload(			"net_serverDownload",		"0",			CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "enable server download redirects. 0: off 1: redirect to si_serverURL 2: use builtin download. see net_serverDl cvars for configuration" );
idCVar net_serverDlBaseURL(			"net_serverDlBaseURL",		"",				CVAR_GAME | CVAR_ARCHIVE, "base URL for the download redirection" );
idCVar net_serverDlTable(			"net_serverDlTable",		"",				CVAR_GAME | CVAR_ARCHIVE, "pak names for which download is provided, separated by ;" );




//BC

idCVar g_showmaterial(				"g_showmaterial",			"0",			CVAR_GAME | CVAR_BOOL, "Show material at the crosshair location.");
idCVar g_showmodel(					"g_showmodel",				"0",			CVAR_GAME | CVAR_BOOL, "Show model filepath at the crosshair location.");
idCVar g_showEntityHealth(			"g_showEntityHealth",		"0",			CVAR_GAME | CVAR_BOOL, "Show health of entities.");
idCVar g_screenshake(				"g_screenshake",			"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enable screenshake.");
idCVar g_headbob(					"g_headbob",				"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enable headbob.");
idCVar g_showPlayerBody(			"g_showPlayerBody",			"1",			CVAR_GAME | CVAR_BOOL, "enables first person player model");
idCVar g_hiteffects(				"g_hiteffects",				"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enable player fullscreen hit effects.");
idCVar g_clambermaxheight(			"g_clambermaxheight",		"144",			CVAR_GAME | CVAR_INTEGER, "Clamp maximum height the player can clamber.");
idCVar g_savingthrows(				"g_savingthrows",			"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "toggle saving throw system");
idCVar g_cryospawn(					"g_cryospawn",				"1",			CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "enables cryospawn random spawning system.");
idCVar g_showEntityNumber(			"g_showEntityNumber",		"-1",			CVAR_GAME | CVAR_INTEGER, "Show info only for a specific entity number. Helpful for tracking down location of entity. -1 = turn off.");
idCVar g_showEntityDetail(			"g_showEntityDetail",		"-1",			CVAR_GAME | CVAR_INTEGER, "Show info of entities looking at (values larger than 1 will be the view distance limit).");
idCVar g_ruler(						"g_ruler",					"0",			CVAR_GAME | CVAR_INTEGER, "Renders a ruler to measure distances. 1 = orthogonal, 2 = arbitrary");
idCVar g_repairenabled(				"g_repairEnabled",			"1",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_CHEAT, "Toggles the repairbot system.");
idCVar g_showParticleFX(			"g_showParticleFX",			"0",			CVAR_GAME | CVAR_BOOL, "Draws particle effect names.");
idCVar g_searchtime(				"g_searchtime",				"900",			CVAR_GAME | CVAR_INTEGER, "How long the AI search state is.");
idCVar g_oxygenscale(				"g_oxygenscale",			"1.0",			CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "Modify oxygen consumption rate. 0.0 = zero oxygen use. 1.0 = default.");
idCVar ai_showState(				"ai_showState",				"0",			CVAR_GAME | CVAR_BOOL, "Draws the AI logic state above its head");
idCVar ai_showAnimState(			"ai_showAnimState",			"0",			CVAR_GAME | CVAR_INTEGER, "Draws AI anim states above their heads (head, torso, legs), 2 = show cur anim name");
idCVar ai_showPlayerState(			"ai_showPlayerState",		"0",			CVAR_GAME | CVAR_INTEGER, "Draws the Player state changes in console, 2 = show anim name changes");
idCVar ai_debugRepairbot(			"ai_debugRepairbot",		"0",			CVAR_GAME | CVAR_BOOL, "Draws repairbot debug.");
idCVar ai_debugPerception(			"ai_debugPerception",		"0",			CVAR_GAME | CVAR_INTEGER, "Draws AI perception debug.");
idCVar ai_showInterestPoints(		"ai_showInterestPoints",	"0",			CVAR_GAME | CVAR_INTEGER, "Draws interestpoint debug. 1 = show all in world. 2 = show live interest reactions.");
idCVar ai_targetPredictTime(		"ai_targetPredictTime",		"0.016",		CVAR_GAME | CVAR_FLOAT, "How far ahead (in time) the enemies track the target. A higher number is easier to avoid.", 0.0f, 0.5f);

idCVar pm_staminachargedelay(		"pm_staminachargedelay",	"2000",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "length of time before stamina begins to recharge");
idCVar pm_spawnRagdollOnDeath(		"pm_spawnRagdollOnDeath",	"1",			CVAR_GAME | CVAR_BOOL, "Whether to spawn a separate ragdoll when the player dies");

idCVar dmap_noFloodEntities(		"dmap_noFloodEntities",		"",				CVAR_GAME, "Entity types which can't occupy an area.");
idCVar r_skipspaceparticles(		"r_skipspaceparticles",		"0",			CVAR_GAME | CVAR_BOOL, "Whether to spawn the space particles");

idCVar g_hideHudInternal(			"g_hideHudInternal",		"0",			CVAR_GUI | CVAR_ROM | CVAR_BOOL, "internal use"); // blendo eric: used to prevent linking problems when shutting off hud in other systems

idCVar g_useBlendoImageCache(		"g_useblendoimagecache", 	"1",			CVAR_GUI | CVAR_BOOL, "internal use"); // blendo eric: used in light detection

idCVar g_debugEmailScript(			"g_debugEmailScript",		"0",			CVAR_GAME | CVAR_BOOL, "Show debug info for email scripting.");

// SW: Cvars for controlling the 'light gem' luminance detection system
idCVar g_luminance_enabled(				"g_luminance_enabled",				"1",	CVAR_RENDERER | CVAR_BOOL, "Enables 'light gem'-style luminance detection on the player.");
idCVar g_luminance_occlusionTestType(	"g_luminance_occlusionTestType",	"1",	CVAR_RENDERER | CVAR_INTEGER, "Changes how we test for occlusion.\n0 = Progressively shrunken bbox trace from head, 1 = Point trace from feet.", 0, 1);
idCVar g_luminance_updateRate(			"g_luminance_updateRate",			"50",	CVAR_RENDERER | CVAR_INTEGER, "Minimum amount of time (in ms) that needs to pass between luminance checks.\nSince the code is fairly trace-heavy, it seems prudent to allow us to tune this value.", 0, 10000);
idCVar g_luminance_greyscaleConversion(	"g_luminance_greyscaleConversion",	"0",	CVAR_RENDERER | CVAR_INTEGER, "Determines method for converting RGB luminance values to greyscale.\n0 = perceptual luminance-preserving, 1 = unweighted.", 0, 1);
idCVar g_luminance_showvalue(			"g_luminance_showvalue", "0",				CVAR_BOOL, "Show float value for player luminance.");

idCVar g_meta_candidateDistanceMetric(	"g_meta_candidateDistanceMetric",	"1",	CVAR_GAME | CVAR_INTEGER, "Selects the method for measuring candidate distance when assigning AIs to an interestpoint.\n0 = Straight-line distance (not smart, but fast)\n1 = Straight-line weighted by heuristics\n2 = Full pathfinding (might be expensive)");

idCVar g_eventLog_logToFile("g_eventLog_logToFile", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "Enables logging of ongoing events to file.");

idCVar g_skyOverride("g_skyOverride", "-1", CVAR_RENDERER | CVAR_INTEGER, "Override the index of the dynamic sky to use. Mostly just for testing.");

idCVar g_spawnfilter("g_spawnfilter", "", CVAR_GAME, "name of spawnfilter you want to use");
idCVar g_spawnfilter_usemapsetting("g_spawnfilter_usemapsetting", "1", CVAR_GAME | CVAR_BOOL, "whether to use the map's custom spawnfilter setting.");

// SM: For testing blur
idCVar g_blur_testEnable( "g_blur_testEnable", "0", CVAR_SYSTEM | CVAR_BOOL, "Set to true to test the blur effect" );
idCVar g_blur_numIterations( "g_blur_numIterations", "8", CVAR_RENDERER | CVAR_INTEGER, "Number of blur iterations from 0-8 (more iterations is blurrier)", 0, 8 );
idCVar g_blur_intensity( "g_blur_intensity", "6.0", CVAR_RENDERER | CVAR_FLOAT, "Intensity of blur effect from 0-64", 0.0f, 64.0f );

idCVar g_motionBlur_testEnable( "g_motionBlur_testEnable", "0", CVAR_SYSTEM | CVAR_BOOL, "Set to true to test the motion blur effect" );
idCVar g_motionBlur_intensity( "g_motionBlur_intensity", "0.25", CVAR_RENDERER | CVAR_FLOAT, "Intensity of blur effect from 0-8.", 0.0f, 8.0f );

idCVar in_toggleCrouch( "in_toggleCrouch", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _movedown button toggles player crouching/standing" );
idCVar m_leanSensitivity( "m_leanSensitivity", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse sensitivity for lean", 1.0f, 5.0f );

idCVar g_showUnassignedLocations( "g_showUnassignedLocations", "0", CVAR_SYSTEM | CVAR_BOOL, "Set to true to draw debug arrows (to the edges) of unassigned locations" );

idCVar g_inspectDebug("g_inspectDebug", "", CVAR_GAME, "test item inspection offset. Usage: give it X Y Z float values (forward, right, up)");
idCVar g_carryDebug("g_carryDebug", "", CVAR_GAME, "test carry position offset. Usage: give it X Y Z float values (forward, right, up)");
idCVar g_zoominspectDebug("g_zoominspectDebug", "", CVAR_GAME, "test zoom inspect position offset. Usage: give it X Y Z float values (forward, right, up)");
idCVar g_zoominspectShowCameraPosition("g_zoominspectShowCameraPosition", "", CVAR_GAME | CVAR_BOOL, "display zoominspect camera position.");

//SW
idCVar g_teleFocusError("g_telefocuserror", "0.3", CVAR_GAME |  CVAR_FLOAT, "Sets the margin of error for telescope focus as a fraction of the FOV.", 0.0f, 1.0f);
idCVar g_teleSeenTime("g_teleseentime", "500", CVAR_GAME |  CVAR_INTEGER, "Number of ms that something must be in view for the player to have 'seen' it.\n");
idCVar g_teleFocusTime("g_telefocustime", "500", CVAR_GAME |  CVAR_INTEGER, "Number of ms the player must be 'focused' on something for it to count.\n");
idCVar g_autorunVignettes("g_autorunVignettes", "1", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "Tells certain map scripts to run immediately instead of waiting for 'start' button.");

//BC
idCVar pm_spacespeed("pm_spacespeed", "300", CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while in zero-g");
idCVar g_showPlayerSpeed("g_showPlayerSpeed", "0", CVAR_GAME | CVAR_BOOL, "prints current player speed");
idCVar g_perceptionScale("g_perceptionScale", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, "scale how quickly enemies get suspicious.", 0.0f, 128.0f);
idCVar g_safescreen("g_safescreen", "0", CVAR_GAME | CVAR_BOOL , "draw safescreen");
idCVar g_showInterestPoints("g_showInterestPoints", "0", CVAR_GAME | CVAR_BOOL, "Set to true to draw debug for interestpoints");

// blendo 
idCVar rb_angularDampen("rb_angularDampen", "1", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "blendo rigid body angular dampening on contact");
idCVar g_showClipBodies("g_showClipBodies", "0", CVAR_SYSTEM | CVAR_BOOL, "blendo actor/physics base physics clip model debug");
idCVar g_showAimAssistTrace("g_showAimAssistTrace", "0", CVAR_SYSTEM | CVAR_BOOL, "blendo aim trace point debug");
 
idCVar gui_subtitleCutoffIntensity("gui_subtitleCutoffIntensity","0.05", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "minimum sound intensity (0.0-1.0) value for subtitles to display", 0.0f, 1.0f);
idCVar gui_inGameLeftClick("gui_inGameLeftClick", "0", CVAR_SYSTEM | CVAR_BOOL, "Whether to allow in-game guis to be left clicked instead of frobbed");

idCVar pm_clamberEdgeDetection( "pm_clamberedgedetection", "1", CVAR_SYSTEM | CVAR_BOOL, "extra edge detection, so the player is placed forward when hovering on the very edge" );
idCVar pm_clamberReachDist( "pm_clamberReachDist", "64", CVAR_CHEAT | CVAR_SYSTEM | CVAR_INTEGER, "max horizontal reach distance for clamber" );
idCVar pm_clamberCubbyExitDistFront( "pm_clamberCubbyExitDistFront", "32", CVAR_GAME | CVAR_FLOAT, "distance check in front of player when trying to stand and exit a cubby");
idCVar pm_clamberCubbyExitDistBack( "pm_clamberCubbyExitDistBack", "8", CVAR_GAME | CVAR_FLOAT, "distance check behind player when trying to stand and exit a cubby");

idCVar g_newSecurityCamLights("g_newSecurityCamLights", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "toggle use of new spotlight generation code on security cameras");

idCVar g_showModelNames("g_showmodelnames", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_CHEAT, "show model filenames");

idCVar g_eventhighlighter("g_eventhighlighter", "1", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "enables event highlighter.");

idCVar g_showIntro("g_showIntro", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "show intro.");

//BC 2-20-2025: impact slowmo toggle.
idCVar g_impactslowmo("g_impactslowmo", "1", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "enables impact slowmo.");

idCVar g_froboutline("g_froboutline", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "enables frob outline.");

idCVar gamepad_rumble_intensity("gamepad_rumble_intensity", "0.5", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE, "intensity of controller/gamepad rumble/vibrate/haptic, 0 to disable, 1 high, >1 overdrive", 0.0f, 1000.0f);
idCVar gamepad_light_intensity("gamepad_light_intensity", "1.0", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE, "intensity of controller/gamepad light, 0 to disable, 1 high, >1 overdrive", 0.0f, 1.0f);
idCVar gamepad_screenshake_rumble("gamepad_screenshake_rumble", "0", CVAR_SYSTEM | CVAR_FLOAT, "amount scereenshake affects rumble. (0.0 - 1.0, >1 overdrive)", 0.0f, 1000.0f);
idCVar gamepad_screenshake_light("gamepad_screenshake_light", "0", CVAR_SYSTEM | CVAR_FLOAT, "amount scereenshake affects led light. (0.0 - 1.0, >1 overdrive)", 0.0f, 1000.0f);
idCVar gamepad_fx_background("gamepad_fx_background", "0", CVAR_SYSTEM | CVAR_BOOL| CVAR_ARCHIVE, "play gamepad fx when window is out of focus");
idCVar gamepad_fx_debug("gamepad_fx_debug", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_CHEAT, "print gamepad fx debug info");
idCVar gamepad_rumble_enable("gamepad_rumble_enable", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "toggle gamepad rumble (0/1)");

idCVar s_aiMuffleDoor( "s_aiMuffleDoor", "1.0", CVAR_CHEAT | CVAR_SOUND | CVAR_FLOAT, "reduce distance AI hears past a door with this percent [0.0f,1.0f] (0.0 = no reduction, 0.5f = half distance, 1.0 = silent)", 0.0f, 1.0f);
idCVar s_aiMuffleCorner( "s_aiMuffleCorner", "0.1", CVAR_CHEAT | CVAR_SOUND | CVAR_FLOAT, "reduce distance AI hears around a corner with this percent [0.0f,1.0f] (0.0 = no reduction, 0.5f = half distance, 1.0 = silent)", 0.0f, 1.0f);

idCVar g_pauseOnFocusLost("g_pauseOnFocusLost", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "If true, game pauses when focus is lost.");

//BC 3-14-2025 g_showtimer
idCVar g_showtimer("g_showtimer", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "Show mission timer.");