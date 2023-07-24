// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/BuildVersion.h"
#include "../../framework/BuildDefines.h"
#include "../../framework/Licensee.h"
#include "../rules/GameRules.h"

#if defined( _DEBUG )
	#define	BUILD_DEBUG	"(debug)"
#else
	#define	BUILD_DEBUG ""
#endif

/*

All game cvars should be defined here.

*/

struct gameVersion_t {
	static const int strSize = 256;
	gameVersion_t( void ) {
		idStr::snPrintf( string, strSize, "%s %d.%d.%d.%d %s %s %s %s", GAME_NAME, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_SRC_REVISION, ENGINE_MEDIA_REVISION, BUILD_DEBUG, BUILD_STRING, __DATE__, __TIME__ );
		string[ strSize - 1 ] = '\0';
	}
	char	string[strSize];
} gameVersion;

idCVar g_version(					"g_version",				gameVersion.string,	CVAR_GAME | CVAR_ROM, "game version" );

// noset vars
idCVar gamename(					"gamename",					GAME_VERSION,	CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM, "" );
idCVar gamedate(					"gamedate",					__DATE__,		CVAR_GAME | CVAR_ROM, "" );

// server info
idCVar si_name(						"si_name",					GAME_NAME " Server",	CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "name of the server" );
idCVar si_maxPlayers(				"si_maxPlayers",			"24",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED, "max number of players allowed on the server", 1, static_cast< float >( MAX_CLIENTS ) );
idCVar si_privateClients(			"si_privateClients",		"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "max number of private players allowed on the server", 0, static_cast< float >( MAX_CLIENTS ) );
idCVar si_teamDamage(				"si_teamDamage",			"1",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED, "enable team damage" );
idCVar si_needPass(					"si_needPass",				"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED, "enable client password checking" );
idCVar si_pure(						"si_pure",					"1",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED, "server is pure and does not allow modified data" );
idCVar si_spectators(				"si_spectators",			"1",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL, "allow spectators or require all clients to play" );
idCVar si_rules(					"si_rules",					"sdGameRulesCampaign",	CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_RANKLOCKED, "ruleset for game", sdGameRules::ArgCompletion_RuleTypes );
idCVar si_timeLimit(				"si_timelimit",				"20",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED, "time limit (mins)" );
idCVar si_map(						"si_map",					"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM, "current active map" );
idCVar si_campaign(					"si_campaign",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM, "current active campaign" );
idCVar si_campaignInfo(				"si_campaignInfo",			"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM, "current campaign map info" );
idCVar si_teamForceBalance(			"si_teamForceBalance",		"1",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED,		"Stop players from unbalancing teams" );
idCVar si_website(					"si_website",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "website info" );
idCVar si_adminname(				"si_adminname",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "admin name(s)" );
idCVar si_email(					"si_email",					"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "contact email address" );
idCVar si_irc(						"si_irc",					"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "IRC channel" );
idCVar si_motd_1(					"si_motd_1",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "motd line 1" );
idCVar si_motd_2(					"si_motd_2",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "motd line 2" );
idCVar si_motd_3(					"si_motd_3",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "motd line 3" );
idCVar si_motd_4(					"si_motd_4",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "motd line 4" );
idCVar si_adminStart(				"si_adminStart",			"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_RANKLOCKED, "admin required to start the match" );
idCVar si_disableVoting(			"si_disableVoting",			"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "disable/enable all voting" );
idCVar si_readyPercent(				"si_readyPercent",			"51",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED, "percentage of players that need to ready up to start a match" );
idCVar si_minPlayers(				"si_minPlayers",			"6",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED, "minimum players before a game can be started" );
idCVar si_allowLateJoin(			"si_allowLateJoin",			"1",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL,		"Enable/disable players joining a match in progress" );
idCVar si_noProficiency(			"si_noProficiency",			"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED, "enable/disable XP" );
idCVar si_disableGlobalChat(		"si_disableGlobalChat",		"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED, "disable global text communication" );
idCVar si_gameReviewReadyWait(		"si_gameReviewReadyWait",	"0",					CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_BOOL,		"wait for players to ready up before going to the next map" );
idCVar si_serverURL(				"si_serverURL",				"",						CVAR_GAME | CVAR_SERVERINFO | CVAR_ARCHIVE, "server information page" );


idCVar* si_motd_cvars[] = {
	&si_motd_1, &si_motd_2, &si_motd_3, &si_motd_4,
};

// user info
idCVar ui_name(						"ui_name",						"Player",	CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE, "player name" );
idCVar ui_clanTag(					"ui_clanTag",					"",			CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE, "player clan tag" );
idCVar ui_clanTagPosition(			"ui_clanTagPosition",			"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_INTEGER, "positioning of player clan tag. 0 is before their name, 1 is after" );
idCVar ui_showGun(					"ui_showGun",					"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "show gun" );
idCVar ui_autoSwitchEmptyWeapons(	"ui_autoSwitchEmptyWeapons",	"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, will switch to the next usable weapon when the current weapon runs out of ammo" );
idCVar ui_ignoreExplosiveWeapons(	"ui_ignoreExplosiveWeapons",	"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, weapons marked as explosive will be ignored during auto-switches" );
idCVar ui_postArmFindBestWeapon(	"ui_postArmFindBestWeapon",		"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, after arming players' best weapon will be selected" );
idCVar ui_advancedFlightControls(	"ui_advancedFlightControls",	"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, advanced flight controls are activated" );
idCVar ui_rememberCameraMode(		"ui_rememberCameraMode",		"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "use same camera mode as was previously used when re-entering a vehicle" );
idCVar ui_drivingCameraFreelook(	"ui_drivingCameraFreelook",		"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, driving cameras where there is no weapon defaults to freelook" );
idCVar ui_voipReceiveGlobal(		"ui_voipReceiveGlobal",			"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, receive voice chat sent to the global channel" );
idCVar ui_voipReceiveTeam(			"ui_voipReceiveTeam",			"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, receive voice chat sent to the team channel" );
idCVar ui_voipReceiveFireTeam(		"ui_voipReceiveFireTeam",		"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, receive voice chat sent to the fireteam channel" );
idCVar ui_showComplaints(			"ui_showComplaints",			"1",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, receive complaints popups for team kills" );
idCVar ui_swapFlightYawAndRoll(		"ui_swapFlightYawAndRoll",		"0",		CVAR_GAME | CVAR_PROFILE | CVAR_USERINFO | CVAR_ARCHIVE | CVAR_BOOL, "if true, swaps the yaw & roll controls for flying vehicles - mouse becomes yaw and keys become roll" );

// change anytime vars
idCVar g_decals(					"g_decals",					"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_BOOL, "show decals such as bullet holes" );
idCVar g_knockback(					"g_knockback",				"1",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_gravity(					"g_gravity",		DEFAULT_GRAVITY_STRING, CVAR_GAME | CVAR_FLOAT, "" );

idCVar g_disasm(					"g_disasm",					"0",			CVAR_GAME | CVAR_BOOL, "disassemble script into base/script/disasm.txt on the local drive when script is compiled" );
idCVar g_debugBounds(				"g_debugBounds",			"0",			CVAR_GAME | CVAR_BOOL, "checks for models with bounds > 2048" );
idCVar g_debugAnim(					"g_debugAnim",				"-1",			CVAR_GAME | CVAR_INTEGER, "displays information on which animations are playing on the specified entity number.  set to -1 to disable." );
idCVar g_debugAnimStance(			"g_debugAnimStance",		"-1",			CVAR_GAME | CVAR_INTEGER, "displays information on which stances are set on the specified entity number.  set to -1 to disable." );
idCVar g_debugDamage(				"g_debugDamage",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugWeapon(				"g_debugWeapon",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugWeaponSpread(			"g_debugWeaponSpread",		"0",			CVAR_GAME | CVAR_BOOL, "displays the current spread value for the weapon" );
idCVar g_debugScript(				"g_debugScript",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugCinematic(			"g_debugCinematic",			"0",			CVAR_GAME | CVAR_BOOL, "" );

idCVar g_debugPlayerList(			"g_debugPlayerList",		"0",			CVAR_GAME | CVAR_INTEGER, "fills UI lists with fake players" );

idCVar g_showPVS(					"g_showPVS",				"0",			CVAR_GAME | CVAR_INTEGER, "", 0, 2 );
idCVar g_showTargets(				"g_showTargets",			"0",			CVAR_GAME | CVAR_BOOL, "draws entities and their targets.  hidden entities are drawn grey." );
idCVar g_showTriggers(				"g_showTriggers",			"0",			CVAR_GAME | CVAR_BOOL, "draws trigger entities (orange) and their targets (green).  disabled triggers are drawn grey." );
idCVar g_showCollisionWorld(		"g_showCollisionWorld",		"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_showVehiclePathNodes(		"g_showVehiclePathNodes",	"0",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_showCollisionModels(		"g_showCollisionModels",	"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showRenderModelBounds(		"g_showRenderModelBounds",	"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_collisionModelMask(		"g_collisionModelMask",		"-1",			CVAR_GAME | CVAR_INTEGER, "" );
idCVar g_showCollisionTraces(		"g_showCollisionTraces",	"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showClipSectors(			"g_showClipSectors",		"0",			CVAR_GAME | CVAR_BOOL,	"" );
idCVar g_showClipSectorFilter(		"g_showClipSectorFilter",	"0",			CVAR_GAME,				"" );
idCVar g_showAreaClipSectors(		"g_showAreaClipSectors",	"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_maxShowDistance(			"g_maxShowDistance",		"128",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_showEntityInfo(			"g_showEntityInfo",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showcamerainfo(			"g_showcamerainfo",			"0",			CVAR_GAME, "displays the current frame # for the camera when playing cinematics" );
idCVar g_showTestModelFrame(		"g_showTestModelFrame",		"0",			CVAR_GAME | CVAR_BOOL, "displays the current animation and frame # for testmodels" );
idCVar g_showActiveEntities(		"g_showActiveEntities",		"0",			CVAR_GAME | CVAR_BOOL, "draws boxes around thinking entities. " );
idCVar g_debugMask(					"g_debugMask",				"",				CVAR_GAME, "debugs a deployment mask" );
idCVar g_debugLocations(			"g_debugLocations",			"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_showActiveDeployZones(		"g_showActiveDeployZones",	"0",			CVAR_GAME | CVAR_BOOL, "" );

idCVar g_disableVehicleSpawns(		"g_disableVehicleSpawns",	"0",			CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_RANKLOCKED, "disables vehicles spawning from construction pads" );

idCVar g_frametime(					"g_frametime",				"0",			CVAR_GAME | CVAR_BOOL, "displays timing information for each game frame" );
//idCVar g_timeentities(				"g_timeEntities",			"0",			CVAR_GAME | CVAR_FLOAT, "when non-zero, shows entities whose think functions exceeded the # of milliseconds specified" );
//idCVar g_timetypeentities(			"g_timeTypeEntities",		"",				CVAR_GAME, "" );
	
idCVar ai_debugScript(				"ai_debugScript",			"-1",			CVAR_GAME | CVAR_INTEGER, "displays script calls for the specified monster entity number" );
idCVar ai_debugAnimState(			"ai_debugAnimState",		"-1",			CVAR_GAME | CVAR_INTEGER, "displays animState changes for the specified monster entity number" );
idCVar ai_debugMove(				"ai_debugMove",				"0",			CVAR_GAME | CVAR_BOOL, "draws movement information for monsters" );
idCVar ai_debugTrajectory(			"ai_debugTrajectory",		"0",			CVAR_GAME | CVAR_BOOL, "draws trajectory tests for monsters" );
	
idCVar g_kickTime(					"g_kickTime",				"1",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_kickAmplitude(				"g_kickAmplitude",			"0.0001",		CVAR_GAME | CVAR_FLOAT, "" );
//idCVar g_blobTime(					"g_blobTime",				"1",			CVAR_GAME | CVAR_FLOAT, "" );
//idCVar g_blobSize(					"g_blobSize",				"1",			CVAR_GAME | CVAR_FLOAT, "" );

idCVar g_editEntityMode(			"g_editEntityMode",			"0",			CVAR_GAME | CVAR_INTEGER,	"0 = off\n"
																											"1 = lights\n"
																											"2 = sounds\n"
																											"3 = articulated figures\n"
																											"4 = particle systems\n"
																											"5 = monsters\n"
																											"6 = entity names\n"
																											"7 = entity models", 0, 7, idCmdSystem::ArgCompletion_Integer<0,7> );
idCVar g_dragEntity(				"g_dragEntity",				"0",			CVAR_GAME | CVAR_BOOL, "allows dragging physics objects around by placing the crosshair over them and holding the fire button" );
idCVar g_dragDamping(				"g_dragDamping",			"0.5",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_dragMaxforce(				"g_dragMaxforce",			"5000000",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_dragShowSelection(			"g_dragShowSelection",		"0",			CVAR_GAME | CVAR_BOOL, "" );

idCVar g_vehicleVelocity(			"g_vehicleVelocity",		"1000",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleForce(				"g_vehicleForce",			"50000",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionUp(		"g_vehicleSuspensionUp",	"32",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionDown(		"g_vehicleSuspensionDown",	"20",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionKCompress("g_vehicleSuspensionKCompress","200",		CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleSuspensionDamping(	"g_vehicleSuspensionDamping","400",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_vehicleTireFriction(		"g_vehicleTireFriction",	"0.8",			CVAR_GAME | CVAR_FLOAT, "" );

idCVar g_commandMapZoomStep(		"g_commandMapZoomStep",		"0.125",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "percent to increase/decrease command map zoom by" );
idCVar g_commandMapZoom(			"g_commandMapZoom",			"0.25",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "command map zoom level", 0.125f, 0.75f );

idCVar g_showPlayerSpeed(			"g_showPlayerSpeed",		"0",			CVAR_GAME | CVAR_BOOL, "displays player movement speed" );

idCVar m_helicopterPitch(			"m_helicopterPitch",		"-0.022",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "helicopter mouse pitch scale" );
idCVar m_helicopterYaw(				"m_helicopterYaw",			"0.022",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "helicopter mouse yaw scale" );

idCVar m_helicopterPitchScale(		"m_helicopterPitchScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Anansi/Tormentor pitch" );
idCVar m_helicopterYawScale(		"m_helicopterYawScale",		"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Anansi/Tormentor yaw" );
idCVar m_bumblebeePitchScale(		"m_bumblebeePitchScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Bumblebee pitch" );
idCVar m_bumblebeeYawScale(			"m_bumblebeeYawScale",		"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Bumblebee yaw" );
idCVar m_lightVehiclePitchScale(	"m_lightVehiclePitchScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Husky/Armadillo/Hog/Trojan pitch" );
idCVar m_lightVehicleYawScale(		"m_lightVehicleYawScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Husky/Armadillo/Hog/Trojan yaw" );
idCVar m_heavyVehiclePitchScale(	"m_heavyVehiclePitchScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Titan/Cyclops pitch" );
idCVar m_heavyVehicleYawScale(		"m_heavyVehicleYawScale",	"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for Titan/Cyclops yaw" );
idCVar m_playerPitchScale(			"m_playerPitchScale",		"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for player pitch" );
idCVar m_playerYawScale(			"m_playerYawScale",			"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NOCHEAT, "sensitivity scale (over base) for player yaw" );

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

idCVar rb_showTimings(				"rb_showTimings",			"0",			CVAR_GAME | CVAR_INTEGER, "show rigid body cpu usage" );
idCVar rb_showBodies(				"rb_showBodies",			"0",			CVAR_GAME | CVAR_BOOL, "show rigid bodies" );
idCVar rb_showMass(					"rb_showMass",				"0",			CVAR_GAME | CVAR_BOOL, "show the mass of each rigid body" );
idCVar rb_showInertia(				"rb_showInertia",			"0",			CVAR_GAME | CVAR_BOOL, "show the inertia tensor of each rigid body" );
idCVar rb_showVelocity(				"rb_showVelocity",			"0",			CVAR_GAME | CVAR_BOOL, "show the velocity of each rigid body" );
idCVar rb_showActive(				"rb_showActive",			"0",			CVAR_GAME | CVAR_BOOL, "show rigid bodies that are not at rest" );
idCVar rb_showContacts(				"rb_showContacts",			"0",			CVAR_GAME | CVAR_BOOL, "show contact points on rigid bodies" );

idCVar pm_friction(					"pm_friction",				"4",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "friction applied to player on the ground" );
idCVar pm_jumpheight(				"pm_jumpheight",			"68",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "approximate height the player can jump" );
idCVar pm_stepsize(					"pm_stepsize",				"16",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "maximum height the player can step up without jumping" );
idCVar pm_pronespeed(				"pm_pronespeed",			"60",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move while prone" );
idCVar pm_crouchspeed(				"pm_crouchspeed",			"80",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move while crouched" );
idCVar pm_walkspeed(				"pm_walkspeed",				"128",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move while walking" );
idCVar pm_runspeedforward(			"pm_runspeedforward",		"256",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move forwards while running" );
idCVar pm_runspeedback(				"pm_runspeedback",			"160",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move backwards while running" );
idCVar pm_runspeedstrafe(			"pm_runspeedstrafe",		"212",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move sideways while running" );
idCVar pm_sprintspeedforward(		"pm_sprintspeedforward",	"352",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move forwards while sprinting" );
idCVar pm_sprintspeedstrafe(		"pm_sprintspeedstrafe",		"176",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "speed the player can move sideways while sprinting" );
idCVar pm_noclipspeed(				"pm_noclipspeed",			"480",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while in noclip" );
idCVar pm_noclipspeedsprint(		"pm_noclipspeedsprint",		"960",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while in noclip and sprinting" );
idCVar pm_noclipspeedwalk(			"pm_noclipspeedwalk",		"256",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while in noclip and walking" );
idCVar pm_democamspeed(				"pm_democamspeed",			"200",			CVAR_GAME | CVAR_FLOAT | CVAR_NOCHEAT, "speed the player can move while flying around in a demo" );
idCVar pm_spectatespeed(			"pm_spectatespeed",			"450",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while spectating" );
idCVar pm_spectatespeedsprint(		"pm_spectatespeedsprint",	"1024",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while spectating and sprinting" );
idCVar pm_spectatespeedwalk(		"pm_spectatespeedwalk",		"256",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "speed the player can move while spectating and walking" );
idCVar pm_spectatebbox(				"pm_spectatebbox",			"16",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "size of the spectator bounding box" );
idCVar pm_minviewpitch(				"pm_minviewpitch",			"-89",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "amount player's view can look up (negative values are up)" );
idCVar pm_maxviewpitch(				"pm_maxviewpitch",			"88",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "amount player's view can look down" );
idCVar pm_minproneviewpitch(		"pm_minproneviewpitch",		"-45",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "amount player's view can look up when prone(negative values are up)" );
idCVar pm_maxproneviewpitch(		"pm_maxproneviewpitch",		"89",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "amount player's view can look down when prone" );
idCVar pm_proneheight(				"pm_proneheight",			"20",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's bounding box while prone" );
idCVar pm_proneviewheight(			"pm_proneviewheight",		"16",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's view while prone" );
idCVar pm_proneviewdistance(		"pm_proneviewdistance",		"10",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "distance in front of the player's view while prone" );
idCVar pm_crouchheight(				"pm_crouchheight",			"56",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's bounding box while crouched" );
idCVar pm_crouchviewheight(			"pm_crouchviewheight",		"48",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's view while crouched" );
idCVar pm_normalheight(				"pm_normalheight",			"79",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's bounding box while standing" );
idCVar pm_normalviewheight(			"pm_normalviewheight",		"72",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's view while standing" );
idCVar pm_deadheight(				"pm_deadheight",			"20",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's bounding box while dead" );
idCVar pm_deadviewheight(			"pm_deadviewheight",		"10",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "height of player's view while dead" );
idCVar pm_crouchrate(				"pm_crouchrate",			"180",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "time it takes for player's view to change from standing to crouching" );
idCVar pm_bboxwidth(				"pm_bboxwidth",				"32",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT | CVAR_RANKLOCKED, "x/y size of player's bounding box" );
idCVar pm_crouchbob(				"pm_crouchbob",				"0.23",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "bob much faster when crouched" );
idCVar pm_walkbob(					"pm_walkbob",				"0.3",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "bob slowly when walking" );
idCVar pm_runbob(					"pm_runbob",				"0.4",			CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "bob faster when running" );
idCVar pm_runpitch(					"pm_runpitch",				"0.002",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "" );
idCVar pm_runroll(					"pm_runroll",				"0.005",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "" );
idCVar pm_bobup(					"pm_bobup",					"0.005",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "" );
idCVar pm_bobpitch(					"pm_bobpitch",				"0.002",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "" );
idCVar pm_bobroll(					"pm_bobroll",				"0.002",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "" );
idCVar pm_skipBob(					"pm_skipBob",				"0",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "Disable all bobbing" );
idCVar pm_thirdPersonRange(			"pm_thirdPersonRange",		"80",			CVAR_GAME | CVAR_FLOAT, "camera distance from player in 3rd person" );
idCVar pm_thirdPersonHeight(		"pm_thirdPersonHeight",		"0",			CVAR_GAME | CVAR_FLOAT, "height of camera from normal view height in 3rd person" );
idCVar pm_thirdPersonAngle(			"pm_thirdPersonAngle",		"0",			CVAR_GAME | CVAR_FLOAT, "direction of camera from player in 3rd person in degrees (0 = behind player, 180 = in front)" );
idCVar pm_thirdPersonOrbit(			"pm_thirdPersonOrbit",		"0",			CVAR_GAME | CVAR_FLOAT, "if set, will automatically increment pm_thirdPersonAngle every frame" );
idCVar pm_thirdPersonNoPitch(		"pm_thirdPersonNoPitch",	"0",			CVAR_GAME | CVAR_BOOL, "ignore camera pitch when in third person mode" );
idCVar pm_thirdPersonClip(			"pm_thirdPersonClip",		"1",			CVAR_GAME | CVAR_BOOL, "clip third person view into world space" );
idCVar pm_thirdPerson(				"pm_thirdPerson",			"0",			CVAR_GAME | CVAR_BOOL, "enables third person view" );
idCVar pm_pausePhysics(				"pm_pausePhysics",			"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL | CVAR_RANKLOCKED, "pauses physics" );
idCVar pm_deployThirdPersonRange(	"pm_deployThirdPersonRange","200",			CVAR_GAME | CVAR_FLOAT, "camera distance from player in 3rd person" );
idCVar pm_deployThirdPersonHeight(	"pm_deployThirdPersonHeight","100",			CVAR_GAME | CVAR_FLOAT, "height of camera from normal view height in 3rd person" );
idCVar pm_deployThirdPersonAngle(	"pm_deployThirdPersonAngle","0",			CVAR_GAME | CVAR_FLOAT, "direction of camera from player in 3rd person in degrees (0 = behind player, 180 = in front)" );

idCVar pm_deathThirdPersonRange(	"pm_deathThirdPersonRange",	"100",			CVAR_GAME | CVAR_FLOAT, "camera distance from player in 3rd person" );
idCVar pm_deathThirdPersonHeight(	"pm_deathThirdPersonHeight","20",			CVAR_GAME | CVAR_FLOAT, "height of camera from normal view height in 3rd person" );
idCVar pm_deathThirdPersonAngle(	"pm_deathThirdPersonAngle",	"0",			CVAR_GAME | CVAR_FLOAT, "direction of camera from player in 3rd person in degrees (0 = behind player, 180 = in front)" );


idCVar pm_slidevelocity(			"pm_slidevelocity",			"1",			CVAR_GAME | CVAR_BOOL | CVAR_NETWORKSYNC | CVAR_RANKLOCKED, "what to do with velocity when hitting a surface at an angle. 0: use horizontal speed, 1: keep some of the impact speed to push along the slide" );
idCVar pm_powerslide(				"pm_powerslide",			"0.09",			CVAR_GAME | CVAR_FLOAT | CVAR_NETWORKSYNC, "adjust the push when pm_slidevelocity == 1, set power < 1 -> more speed, > 1 -> closer to pm_slidevelocity 0", 0, 4 );

idCVar g_showPlayerArrows(			"g_showPlayerArrows",		"1",			CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC, "enable/disable arrows above the heads of players (0=off,1=all,2=friendly only)" );
idCVar g_showPlayerClassIcon(		"g_showPlayerClassIcon",	"0",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "Force drawing of player class icon above players in the fireteam." );

idCVar g_showPlayerShadow(			"g_showPlayerShadow",		"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_BOOL, "enables shadow of player model" );
idCVar g_showHud(					"g_showHud",				"1",			CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT, "draw the hud gui" );
idCVar g_advancedHud(				"g_advancedHud",			"0",			CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "Draw advanced HUD" );
idCVar g_skipPostProcess(			"g_skipPostProcess",		"0",			CVAR_GAME | CVAR_BOOL, "draw the post process gui" );
idCVar g_gun_x(						"g_gunX",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_gun_y(						"g_gunY",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_gun_z(						"g_gunZ",					"0",			CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_fov(						"g_fov",					"90",			CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT, "", 1.0f, 179.0f );
idCVar g_skipViewEffects(			"g_skipViewEffects",		"0",			CVAR_GAME | CVAR_BOOL, "skip damage and other view effects" );
idCVar g_forceClear(				"g_forceClear",				"1",			CVAR_GAME | CVAR_BOOL, "forces clearing of color buffer on main game draw (faster)" );

idCVar g_testParticle(				"g_testParticle",			"0",			CVAR_GAME | CVAR_INTEGER, "test particle visualization, set by the particle editor" );
idCVar g_testParticleName(			"g_testParticleName",		"",				CVAR_GAME, "name of the particle being tested by the particle editor" );
idCVar g_testModelRotate(			"g_testModelRotate",		"0",			CVAR_GAME, "test model rotation speed" );
idCVar g_testPostProcess(			"g_testPostProcess",		"",				CVAR_GAME, "name of material to draw over screen" );
idCVar g_testViewSkin(				"g_testViewSkin",			"",				CVAR_GAME, "name of skin to use for the view" );
idCVar g_testModelAnimate(			"g_testModelAnimate",		"0",			CVAR_GAME | CVAR_INTEGER, "test model animation,\n"
																							"0 = cycle anim with origin reset\n"
																							"1 = cycle anim with fixed origin\n"
																							"2 = cycle anim with continuous origin\n"
																							"3 = frame by frame with continuous origin\n"
																							"4 = play anim once", 0, 4, idCmdSystem::ArgCompletion_Integer<0,4> );

idCVar g_disableGlobalAudio(		"g_disableGlobalAudio",		"0",			CVAR_GAME | CVAR_NETWORKSYNC | CVAR_BOOL, "disable global VOIP communication" );
idCVar g_maxPlayerWarnings(			"g_maxPlayerWarnings",		"0",			CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED, "maximum warnings before player is kicked" );


idCVar g_testModelBlend(			"g_testModelBlend",			"0",			CVAR_GAME | CVAR_INTEGER, "number of frames to blend" );
idCVar g_exportMask(				"g_exportMask",				"",				CVAR_GAME, "" );

idCVar password(					"password",					"",				CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_RANKLOCKED, "client password used when connecting" );

idCVar net_useAOR(					"net_useAOR",				"1",			CVAR_GAME | CVAR_BOOL, "Enable/Disable Area of Relevance" );
idCVar net_aorPVSScale(				"net_aorPVSScale",			"4",			CVAR_GAME | CVAR_FLOAT, "AoR scale for outside of PVS" );

idCVar pm_vehicleSoundLerpScale(	"pm_vehicleSoundLerpScale",	"10",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT, "" );

idCVar pm_waterSpeed(				"pm_waterSpeed",			"400",			CVAR_GAME | CVAR_FLOAT | CVAR_RANKLOCKED, "speed player will be pushed up in water when totally under water" );

idCVar g_debugNetworkWrite(			"g_debugNetworkWrite",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_debugProficiency(			"g_debugProficiency",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_weaponSwitchTimeout(		"g_weaponSwitchTimeout",	"1.5",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_FLOAT, "" );

idCVar g_hitBeep(					"g_hitBeep",				"1",			CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_INTEGER, "play hit beep sound when you inflict damage.\n  0 = do nothing\n  1 = beep/flash cross-hair\n  2 = beep\n  3 = flash cross-hair" );

idCVar fs_debug(					"fs_debug",					"0",			CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );

#if !defined( _XENON ) && !defined( MONOLITHIC )
idCVar r_aspectRatio( 				"r_aspectRatio",			"0",			CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "aspect ratio. 0 is 4:3, 1 is 16:9, 2 is 16:10, 3 is 5:4. -1 uses r_customAspectRatioH and r_customAspectRatioV" );
idCVar r_customAspectRatioH( 		"r_customAspectRatioH",		"16",			CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "horizontal custom aspect ratio" );
idCVar r_customAspectRatioV( 		"r_customAspectRatioV",		"10",			CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "vertical custom aspect ratio" );
#endif

idCVar anim_showMissingAnims(		"anim_showMissingAnims",	"0",			CVAR_BOOL, "Show warnings for missing animations" );

const char *aas_types[] = {
	"aas_player",
	"aas_vehicle",
	NULL
};

idCVar aas_test(					"aas_test",						"0",		CVAR_GAME, "select which AAS to test", aas_types, idCmdSystem::ArgCompletion_String<aas_types> );
idCVar aas_showEdgeNums(			"aas_showEdgeNums",				"0",		CVAR_GAME | CVAR_BOOL, "show edge nums" );
idCVar aas_showAreas(				"aas_showAreas",				"0",		CVAR_GAME | CVAR_INTEGER, "show the areas in the selected aas", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar aas_showAreaNumber(			"aas_showAreaNumber",			"0",		CVAR_GAME | CVAR_INTEGER, "show the specific area number set" );
idCVar aas_showPath(				"aas_showPath",					"0",		CVAR_GAME, "show the path to the walk specified area" );
idCVar aas_showHopPath(				"aas_showHopPath",				"0",		CVAR_GAME, "show hop path to specified area" );
idCVar aas_showWallEdges(			"aas_showWallEdges",			"0",		CVAR_GAME | CVAR_INTEGER, "show the edges of walls, 2 = project all to same height, 3 = project onscreen", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar aas_showWallEdgeNums(		"aas_showWallEdgeNums",			"0",		CVAR_GAME | CVAR_BOOL, "show the number of the edges of walls" );
idCVar aas_showNearestCoverArea(	"aas_showNearestCoverArea",		"0",		CVAR_GAME | CVAR_INTEGER, "show the nearest area with cover from the selected area (aas_showHideArea 4 will show the nearest area in cover from area 4)" );
idCVar aas_showNearestInsideArea(	"aas_showNearestInsideArea",	"0",		CVAR_GAME | CVAR_BOOL, "show the nearest area that is inside" );
idCVar aas_showTravelTime(			"aas_showTravelTime",			"0",		CVAR_GAME | CVAR_INTEGER, "print the travel time to the specified goal area (only when aas_showAreas is set)" );
idCVar aas_showPushIntoArea(		"aas_showPushIntoArea",			"0",		CVAR_GAME | CVAR_BOOL, "show an arrow going to the closest area" );
idCVar aas_showFloorTrace(			"aas_showFloorTrace",			"0",		CVAR_GAME | CVAR_BOOL, "show floor trace" );
idCVar aas_showObstaclePVS(			"aas_showObstaclePVS",			"0",		CVAR_GAME | CVAR_INTEGER, "show obstacle PVS for the given area" );
idCVar aas_showManualReachabilities("aas_showManualReachabilities",	"0",		CVAR_GAME | CVAR_BOOL, "show manually placed reachabilities" );
idCVar aas_showFuncObstacles(		"aas_showFuncObstacles",		"0",		CVAR_GAME | CVAR_BOOL, "show the AAS func_obstacles on the map" );
idCVar aas_showBadAreas(			"aas_showBadAreas",				"0",		CVAR_GAME | CVAR_INTEGER, "show bad AAS areas", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar aas_locationMemory(			"aas_locationMemory",			"0",		CVAR_GAME, "used to remember a particular location, set to 'current' to store the current x,y,z location" );
idCVar aas_pullPlayer(				"aas_pullPlayer",				"0",		CVAR_GAME, "pull the player to the specified area" );
idCVar aas_randomPullPlayer(		"aas_randomPullPlayer",			"0",		CVAR_GAME | CVAR_INTEGER, "pull the player to a random area" );

idCVar bot_threading(				"bot_threading",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"enable running the bot AI in a separate thread" );
idCVar bot_threadMinFrameDelay(		"bot_threadMinFrameDelay",		"1",		CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT,	"minimum number of game frames the bot AI trails behind", 0, 4, idCmdSystem::ArgCompletion_Integer<0,4> );
idCVar bot_threadMaxFrameDelay(		"bot_threadMaxFrameDelay",		"4",		CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT,	"maximum number of game frames the bot AI can trail behind", 0, 4, idCmdSystem::ArgCompletion_Integer<0,4> );
idCVar bot_pause(					"bot_pause",					"0",		CVAR_GAME | CVAR_BOOL,						"Pause the bot's thinking - useful for screenshots/debugging/etc" );
idCVar bot_drawClientNumbers(		"bot_drawClientNumbers",		"0",		CVAR_GAME | CVAR_BOOL,						"Draw every clients number above their head" );
idCVar bot_drawActions(				"bot_drawActions",				"0",		CVAR_GAME | CVAR_BOOL,						"Draw the bot's actions." );
idCVar bot_drawBadIcarusActions(	"bot_drawBadIcarusActions",		"0",		CVAR_GAME | CVAR_BOOL,						"Draw actions with an icarus flag, that aren't in a valid vehicle AAS area." );
idCVar bot_drawIcarusActions(		"bot_drawIcarusActions",		"0",		CVAR_GAME | CVAR_BOOL,						"Draw actions with an icarus flag, that appear valid to the AAS." );
idCVar bot_drawActionRoutesOnly(	"bot_drawActionRoutesOnly",		"-1",		CVAR_GAME | CVAR_INTEGER,					"Draw only the bot actions that have the defined route." );
idCVar bot_drawNodes(				"bot_drawNodes",				"0",		CVAR_BOOL | CVAR_GAME,						"draw vehicle path nodes" );
idCVar bot_drawNodeNumber(			"bot_drawNodeNumber",			"-1",		CVAR_INTEGER | CVAR_GAME,					"draw a specific vehicle path node" );
idCVar bot_drawDefuseHints(			"bot_drawDefuseHints",			"0",		CVAR_BOOL | CVAR_GAME,						"draw the bot's defuse hints." );
idCVar bot_drawActionNumber(		"bot_drawActionNumber",			"-1",		CVAR_GAME | CVAR_INTEGER,					"Draw a specific bot action only. -1 = disable" );
idCVar bot_drawActionVehicleType(	"bot_drawActionVehicleType",	"-1",		CVAR_GAME | CVAR_INTEGER,					"Draw only the actions that have this vehicleType set. -1 = disable" );
idCVar bot_drawActiveActionsOnly(	"bot_drawActiveActionsOnly",	"0",		CVAR_GAME | CVAR_INTEGER,					"Draw only active bot actions. 1 = all active actions. 2 = only GDF active actions. 3 = only Strogg active actions. Combo actions, that have both GDF and strogg goals, will still show up." );
idCVar bot_drawActionWithClasses(	"bot_drawActionWithClasses",	"0",		CVAR_GAME | CVAR_BOOL,						"Draw only actions that have a validClass set to anything other then 0 ( 0 = any class )." );
idCVar bot_drawActionTypeOnly(		"bot_drawActionTypeOnly",		"-1",		CVAR_GAME | CVAR_INTEGER,					"Draw only actions that have a gdf/strogg goal number matching the cvar value. Check the bot manual for goal numbers. -1 = disabled." );
idCVar bot_drawRoutes(				"bot_drawRoutes",				"0",		CVAR_GAME | CVAR_BOOL,						"Draw the routes on the map." );
idCVar bot_drawActiveRoutesOnly(	"bot_drawActiveRoutesOnly",		"0",		CVAR_GAME | CVAR_BOOL,						"Only draw the active routes on the map." );
idCVar bot_drawRouteGroupOnly(		"bot_drawRouteGroupOnly",		"-1",		CVAR_GAME | CVAR_INTEGER,					"Only draw routes that have the groupID specified." );
idCVar bot_drawActionSize(			"bot_drawActionSize",			"0.2",		CVAR_GAME | CVAR_FLOAT,						"How big to draw the bot action info. Default is 0.2" );
idCVar bot_drawActionGroupNum(		"bot_drawActionGroupNum",		"-1",		CVAR_GAME | CVAR_INTEGER,					"Filter what action groups to draw with the bot_drawAction cmd. -1 = disabled." );
idCVar bot_drawActionDist(			"bot_drawActionDist",			"4092",		CVAR_GAME | CVAR_FLOAT,						"How far away to draw the bot action info. Default is 2048" );
idCVar bot_drawObstacles(			"bot_drawObstacles",			"0",		CVAR_GAME | CVAR_BOOL,						"Draw the bot's dynamic obstacles in the world" );
idCVar bot_drawRearSpawnLocations(	"bot_drawRearSpawnLocations",	"0",		CVAR_GAME | CVAR_BOOL,						"Draw the rear spawn locations for each team" );
idCVar bot_enable(					"bot_enable",					"1",		CVAR_GAME | CVAR_BOOL | CVAR_SERVERINFO,	"0 = bots will not be loaded in the game. 1 = bots are loaded." );
idCVar bot_doObjectives(			"bot_doObjectives",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_NETWORKSYNC | CVAR_NOCHEAT,		"0 = bots let the player play the hero, with the bots filling a supporting role, 1 = bots do all the major objectives along with the player" );
idCVar bot_ignoreGoals(				"bot_ignoreGoals",				"0",		CVAR_GAME | CVAR_INTEGER | CVAR_CHEAT,		"If set to 1, bots will ignore all map objectives. Useful for debugging bot behavior" );
idCVar bot_useVehicles(				"bot_useVehicles",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots dont use vehicles, 1 = bots do use vehicles" );
idCVar bot_stayInVehicles(			"bot_stayInVehicles",			"0",		CVAR_GAME | CVAR_BOOL,						"1 = bots will never leave their vehicle. Only useful for debugging. Default is 0" );
idCVar bot_useStrafeJump(			"bot_useStrafeJump",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots can't strafe jump, 1 = bots CAN strafe jump to goal locations that are far away" );
idCVar bot_useSuicideWhenStuck(		"bot_useSuicideWhenStuck",		"1",		CVAR_GAME | CVAR_BOOL,						"0 = bots never suicide when stuck. 1 = bots suicide if they detect that they're stuck" );
idCVar bot_useDeployables(			"bot_useDeployables",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots dont drop deployables of any kind, 1 = bots can drop all deployables" );
idCVar bot_useMines(				"bot_useMines",					"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots dont use mines, 1 = bots can use mines. Default = 1" );
idCVar bot_sillyWarmup(				"bot_sillyWarmup",				"0",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots play the game like normal, 1 = bots shoot each other and act silly during warmup" );
idCVar bot_useSpawnHosts(			"bot_useSpawnHosts",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = strogg bots can't use spawn host bodies, 1 = bots can use spawnhosts" );
idCVar bot_useUniforms(				"bot_useUniforms",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots won't steal uniforms, 1 = bots take uniforms" );
idCVar bot_noChat(					"bot_noChat",					"0",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots chat, 1 = bots never chat" );
idCVar bot_noTaunt(					"bot_noTaunt",					"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots taunt, 1 = bots never taunt" );
idCVar bot_aimSkill(				"bot_aimSkill",					"1",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"Sets the bot's default aiming skill. 0 = EASY, 1 = MEDIUM, 2 = EXPERT, 3 = MASTER", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar bot_skill(					"bot_skill",					"3",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"Sets the bot's default AI skill. 0 = EASY, 1 = NORMAL, 2 = EXPERT, 3 = TRAINING MODE - this mode is useful for learning about the game", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar bot_knifeOnly(				"bot_knifeOnly",				"0",		CVAR_GAME | CVAR_BOOL,						"goofy mode where the bots only use their knifes in combat." );
idCVar bot_ignoreEnemies(			"bot_ignoreEnemies",			"0",		CVAR_GAME | CVAR_INTEGER,					"If set to 1, bots will ignore all enemies. 2 = Ignore Strogg. 3 = Ignore GDF. Useful for debugging bot behavior" );
idCVar bot_debug(					"bot_debug",					"0",		CVAR_GAME | CVAR_BOOL,						"Debug various bot subsystems. Many bot debugging features are disabled if this is not set to 1" );
idCVar bot_debugActionGoalNumber(	"bot_debugActionGoalNumber",	"-1",		CVAR_GAME | CVAR_INTEGER,					"Set to any action number on the map to have the bot ALWAYS do that action, for debugging. -1 = disabled. NOTE: The bot will treat the goal as a camp goal. This is useful for path testing." );
idCVar bot_debugSpeed(				"bot_debugSpeed",				"-1",		CVAR_GAME | CVAR_INTEGER,					"Debug bot's move speed. -1 = disable" );
idCVar bot_debugGroundVehicles(		"bot_debugGroundVehicles",		"-1",		CVAR_GAME | CVAR_INTEGER,					"Debug bot ground vehicle usage. -1 = disable" );
idCVar bot_debugAirVehicles(		"bot_debugAirVehicles",			"-1",		CVAR_GAME | CVAR_INTEGER,					"Debug bot air vehicle usage. -1 = disable" );
idCVar bot_debugObstacles(			"bot_debugObstacles",			"0",		CVAR_GAME | CVAR_BOOL,						"Debug bot obstacles in the world" );
idCVar bot_spectateDebug(			"bot_spectateDebug",			"0",		CVAR_GAME | CVAR_BOOL,						"If enabled, automatically sets the debug hud to the bot being spectated" );
idCVar bot_followMe(				"bot_followMe",					"0",		CVAR_GAME | CVAR_INTEGER,					"Have the bots follow you in debug mode" );
idCVar bot_breakPoint(				"bot_breakPoint",				"0",		CVAR_GAME | CVAR_BOOL,						"Cause a program break to occur inside the bot's AI" );
idCVar bot_useShotguns(				"bot_useShotguns",				"0",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots wont use shotguns/nailguns. 1 = bots will use shotguns/nailguns." );
idCVar bot_useSniperWeapons(		"bot_useSniperWeapons",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots wont use sniper rifles. 1 = bots will use sniper rifles." );
idCVar bot_minClients(				"bot_minClients",				"-1",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"Keep a minimum number of clients on the server with bots and humans. -1 to disable", -1, MAX_CLIENTS );
idCVar bot_minClientsMax(			"bot_minClientsMax",			"16",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_INIT,	"Maximum allowed value of bot_minClients. Only affects the in-game UI.", -1, MAX_CLIENTS );
idCVar bot_debugPersonalVehicles(	"bot_debugPersonalVehicles",	"0",		CVAR_GAME | CVAR_BOOL,						"Only used for debugging the use of the husky/icarus." );
idCVar bot_debugWeapons(			"bot_debugWeapons",				"0",		CVAR_GAME | CVAR_BOOL,						"Only used for debugging bots weapons." );
idCVar bot_uiNumStrogg(				"bot_uiNumStrogg",				"-1",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"The number of strogg bots to add to the server. -1 to disable" );
idCVar bot_uiNumGDF(				"bot_uiNumGDF",					"-1",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"The number of gdf bots to add to the server. -1 to disable" );
idCVar bot_uiSkill(					"bot_uiSkill",					"5",		CVAR_GAME | CVAR_INTEGER | CVAR_NETWORKSYNC | CVAR_NOCHEAT,	"The overall skill the bots should play at in the game. 0 = EASY, 1 = MEDIUM, 2 = EXPERT, 3 = MASTER, 4 = CUSTOM, 5 = TRAINING" );
idCVar bot_showPath(				"bot_showPath",					"-1",		CVAR_GAME | CVAR_INTEGER,					"Show the path for the bot's client number. -1 = disable." );
idCVar bot_skipThinkClient(			"bot_skipThinkClient",			"-1",		CVAR_GAME | CVAR_INTEGER,					"A debug only cvar that skips thinking for a particular bot with the client number entered. -1 = disabled." );
idCVar bot_debugMapScript(			"bot_debugMapScript",			"0",		CVAR_GAME | CVAR_BOOL,						"Allows you to debug the bot script." );
idCVar bot_useTKRevive(				"bot_useTKRevive",				"1",		CVAR_GAME | CVAR_BOOL,						"Allows the bots to use the advanced tactic of TK reviving if their teammate is weak. 0 = disabled. Default is 1" );
idCVar bot_debugObstacleAvoidance(	"bot_debugObstacleAvoidance",	"0",		CVAR_GAME | CVAR_BOOL,						"Debug obstacle avoidance" );
idCVar bot_testObstacleQuery(		"bot_testObstacleQuery",		"",			CVAR_GAME,									"test a previously recorded obstacle avoidance query" );
idCVar bot_balanceCriticalClass(	"bot_balanceCriticalClass",		"1",		CVAR_GAME | CVAR_BOOL,						"Have the bots try to keep someone playing the critical class at all times. 0 = keep the class they spawn in as. Default = 1." );
idCVar bot_useAltRoutes(			"bot_useAltRoutes",				"1",		CVAR_GAME | CVAR_BOOL,						"Debug the bot's alternate path use." );
idCVar bot_godMode(					"bot_godMode",					"-1",		CVAR_GAME | CVAR_INTEGER,					"Set to the bot client you want to enter god mode. -1 = disable." );
idCVar bot_useRearSpawn(			"bot_useRearSpawn",				"1",		CVAR_BOOL | CVAR_GAME,						"debug bots using rear spawn points" );
idCVar bot_sleepWhenServerEmpty(	"bot_sleepWhenServerEmpty",		"1",		CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT,		"has the bots stop thinking when the server is idle and there are no humans playing, to conserve CPU." );
idCVar bot_allowObstacleDecay(		"bot_allowObstacleDecay",		"1",		CVAR_BOOL | CVAR_GAME,						"0 = dont allow obstacle decay. 1 = allow obstacle decay." );
idCVar bot_allowClassChanges(		"bot_allowClassChanges",		"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots won't ever change their class. 1 = Bots can change their class thru script/code." );
idCVar bot_testPathToBotAction(		"bot_testPathToBotAction",		"-1",		CVAR_GAME | CVAR_INTEGER,					"based on which aas type aas_test is set to, will test to see if a path is available from the players current origin, to the bot action in question. You need to join a team for this to work properly! -1 = disabled." );
idCVar bot_pauseInVehicleTime(		"bot_pauseInVehicleTime",		"7",		CVAR_GAME | CVAR_INTEGER,					"Time the bots will pause when first enter a vehicle ( in seconds ) to allow others to jump in. Default is 7 seconds." );
idCVar bot_doObjsInTrainingMode(	"bot_doObjsInTrainingMode",		"1",		CVAR_GAME | CVAR_BOOL,						"Controls whether or not bots will do objectives in training mode, if the human isn't the correct class to do the objective. 0 = bots won't do primary or secondary objecitives in training mode. 1 = bots will do objectives. Default = 1. " );
idCVar bot_doObjsDelayTimeInMins(	"bot_doObjsDelayTimeInMins",	"3",		CVAR_GAME | CVAR_INTEGER,					"How long of a delay in time the bots will have before they start considering objectives while in Training mode. Default is 3 minutes. " );
idCVar bot_useAirVehicles(			"bot_useAirVehicles",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,		"0 = bots dont use air vehicles, 1 = bots do use air vehicles. Useful for debugging ground vehicles only." );

idCVar g_showCrosshairInfo(			"g_showCrosshairInfo",			"1",		CVAR_INTEGER | CVAR_GAME,					"shows information about the entity under your crosshair" );

idCVar g_banner_1(					"g_banner_1",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 1" );
idCVar g_banner_2(					"g_banner_2",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 2" );
idCVar g_banner_3(					"g_banner_3",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 3" );
idCVar g_banner_4(					"g_banner_4",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 4" );
idCVar g_banner_5(					"g_banner_5",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 5" );
idCVar g_banner_6(					"g_banner_6",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 6" );
idCVar g_banner_7(					"g_banner_7",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 7" );
idCVar g_banner_8(					"g_banner_8",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 8" );
idCVar g_banner_9(					"g_banner_9",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 9" );
idCVar g_banner_10(					"g_banner_10",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 10" );
idCVar g_banner_11(					"g_banner_11",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 11" );
idCVar g_banner_12(					"g_banner_12",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 12" );
idCVar g_banner_13(					"g_banner_13",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 13" );
idCVar g_banner_14(					"g_banner_14",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 14" );
idCVar g_banner_15(					"g_banner_15",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 15" );
idCVar g_banner_16(					"g_banner_16",					"",			CVAR_GAME | CVAR_NOCHEAT,					"banner message 16" );
idCVar g_bannerDelay(				"g_banner_delay",				"1",		CVAR_GAME | CVAR_NOCHEAT | CVAR_INTEGER,	"delay in seconds between banner messages" );
idCVar g_bannerLoopDelay(			"g_banner_loopdelay",			"0",		CVAR_GAME | CVAR_NOCHEAT | CVAR_INTEGER,	"delay in seconds before banner messages repeat, 0 = off" );

idCVar* g_bannerCvars[ NUM_BANNER_MESSAGES ] = {
	&g_banner_1, &g_banner_2, &g_banner_3, &g_banner_4, &g_banner_5, &g_banner_6, &g_banner_7, &g_banner_8,
	&g_banner_9, &g_banner_10, &g_banner_11, &g_banner_12, &g_banner_13, &g_banner_14, &g_banner_15, &g_banner_16,
};

idCVar g_allowComplaintFiresupport( "g_allowComplaint_firesupport", "1",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Allow complaints for teamkills with fire support" );
idCVar g_allowComplaintCharge(		"g_allowComplaint_charge",		"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Allow complaints for teamkills with charges" );
idCVar g_allowComplaintExplosives(	"g_allowComplaint_explosives",	"1",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Allow complaints for teamkills with explosive weapons and items" );
idCVar g_allowComplaintVehicles(	"g_allowComplaint_vehicles",	"1",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Allow complaints for teamkills with vehicle" );

idCVar g_complaintLimit(			"g_complaintLimit",				"6",		CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED,	"Total complaints at which a player will be kicked" );
idCVar g_complaintGUIDLimit(		"g_complaintGUIDLimit",			"4",		CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED,	"Total unique complaints at which a player will be kicked" );

idCVar g_execMapConfigs(			"g_execMapConfigs",				"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Execute map cfg with same name" );

idCVar g_teamSwitchDelay(			"g_teamSwitchDelay",			"5",		CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RANKLOCKED,	"Delay (in seconds) before player can change teams again" );
idCVar g_warmupDamage(				"g_warmupDamage",				"1",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL,		"Enable/disable players taking damage during warmup" );
idCVar g_muteSpecs(					"g_muteSpecs",					"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED,		"Send all spectator global chat to team chat" );
idCVar g_warmup(					"g_warmup",						"0.5",		CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED,		"Length (in minutes) of warmup period" );
idCVar g_gameReviewPause(			"g_gameReviewPause",			"0.5",		CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED,		"Time (in minutes) for scores review time" );
idCVar g_password(					"g_password",					"",			CVAR_GAME | CVAR_ARCHIVE | CVAR_RANKLOCKED,	"game password" );
idCVar g_privatePassword(			"g_privatePassword",			"",			CVAR_GAME | CVAR_ARCHIVE, "game password for private slots" );
idCVar g_xpSave(					"g_xpSave",						"1",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL | CVAR_RANKLOCKED,		"stores xp for disconnected players which will be given back if they reconnect" );
idCVar g_kickBanLength(				"g_kickBanLength",				"2",		CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED,		"length of time a kicked player will be banned for" );
idCVar g_maxSpectateTime(			"g_maxSpectateTime",			"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED,		"maximum length of time a player may spectate for" );

idCVar g_unlock_updateAngles(		"g_unlock_updateAngles",		"1",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_BOOL,		"update view angles in fps unlock mode" );
idCVar g_unlock_updateViewpos(		"g_unlock_updateViewpos",		"1",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_BOOL,		"update view origin in fps unlock mode" );
idCVar g_unlock_interpolateMoving(	"g_unlock_interpolateMoving",	"1",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_BOOL,		"interpolate moving objects in fps unlock mode" );
idCVar g_unlock_viewStyle(			"g_unlock_viewStyle",			"1",		CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE | CVAR_INTEGER,		"0: extrapolate view origin, 1: interpolate view origin" );

idCVar g_voteWait(					"g_voteWait",					"2.5",		CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RANKLOCKED,		"Delay (in minutes) before player may perform a callvote again" );

idCVar g_useTraceCollection(		"g_useTraceCollection",			"1",		CVAR_GAME | CVAR_BOOL,						"Use optimized trace collections" );

idCVar g_removeStaticEntities(		"g_removeStaticEntities",		"1",		CVAR_GAME | CVAR_BOOL,						"Remove non-dynamic entities on map spawn when they aren't needed" );

idCVar g_maxVoiceChats(				"g_maxVoiceChats",				"4",		CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER,	"maximum number of voice chats a player may do in a period of time" );
idCVar g_maxVoiceChatsOver(			"g_maxVoiceChatsOver",			"30",		CVAR_GAME | CVAR_ARCHIVE | CVAR_INTEGER,	"time over which the maximum number of voice chat limit is applied" );

idCVar g_profileEntityThink(		"g_profileEntityThink",			"0",		CVAR_GAME | CVAR_BOOL,						"Enable entity think profiling" );
idCVar g_timeoutToSpec(				"g_timeoutToSpec",				"0",		CVAR_GAME | CVAR_FLOAT | CVAR_NOCHEAT,		"Timeout in minutes for players who are AFK to go into spectator mode (0=disabled)" );

idCVar g_autoReadyPercent(			"g_autoReadyPercent",			"50",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE,		"Percentage of a full server that must be in game for auto ready to start" );
idCVar g_autoReadyWait(				"g_autoReadyWait",				"1",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE,		"Length of time in minutes auto ready will wait before starting the countdown" );

idCVar g_useBotsInPlayerTotal(		"g_useBotsInPlayerTotal",		"1",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE,		"Should bots count towards the number of players required to start the game?" );

idCVar g_playTooltipSound(			"g_playTooltipSound",			"2",		CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE | CVAR_PROFILE, "0: no sound 1: play tooltip sound in single player only 2: Always play tooltip sound" );
idCVar g_tooltipTimeScale(			"g_tooltipTimeScale",			"1",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "Scale the amount of time that a tooltip is visible. 0 will disable all tooltips." );
idCVar g_tooltipVolumeScale(		"g_tooltipVolumeScale",			"-20",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "Change the game volume while playing a tooltip with VO." );
idCVar g_allowSpecPauseFreeFly(		"g_allowSpecPauseFreeFly",		"1",		CVAR_GAME | CVAR_BOOL | CVAR_NETWORKSYNC,	"Allow spectators to free fly when the game is paused" );
idCVar g_smartTeamBalance(			"g_smartTeamBalance",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_RANKLOCKED,	"Encourages players to balance teams themselves by giving rewards." );
idCVar g_smartTeamBalanceReward(	"g_smartTeamBalanceReward",		"10",		CVAR_GAME | CVAR_INTEGER | CVAR_RANKLOCKED,	"The amount of XP to give people who switch teams when asked to." );

idCVar g_keepFireTeamList(			"g_keepFireTeamList",			"0",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "Always show the fireteam list on the HUD." );

idCVar net_serverDownload(			"net_serverDownload",			"0",		CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "enable server download redirects. 0: off 1: client exits and opens si_serverURL in web browser 2: client downloads pak files from an URL and connects again. See net_serverDl* cvars for configuration" );
idCVar net_serverDlBaseURL(			"net_serverDlBaseURL",			"",			CVAR_GAME | CVAR_ARCHIVE, "base URL for the download redirection" );
idCVar net_serverDlTable(			"net_serverDlTable",			"",			CVAR_GAME | CVAR_ARCHIVE, "pak names for which download is provided, seperated by ; - use a * to mark all paks" );

idCVar g_drawMineIcons(				"g_drawMineIcons",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE,		"Draw icons on the HUD for mines." );
idCVar g_allowMineIcons(			"g_allowMineIcons",				"1",		CVAR_GAME | CVAR_BOOL | CVAR_NETWORKSYNC | CVAR_RANKLOCKED, "Allow clients to draw icons on the HUD for mines." );
idCVar g_mineIconSize(				"g_mineIconSize",				"10",		CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE,		"Size of the screen space mine icons. NOTE: Will only take effect for new mines, not those already existing.", 0, 20 );
idCVar g_mineIconAlphaScale(		"g_mineIconAlphaScale",			"1",		CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE,		"Alpha scale to apply to mine icons. NOTE: Will only take effect for new mines, not those already existing." );

idCVar g_drawVehicleIcons(			"g_drawVehicleIcons",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE,		"Draw icons on the HUD for vehicles (eg spawn invulnerability)." );

#ifdef SD_SUPPORT_REPEATER

idCVar ri_useViewerPass(			"ri_useViewerPass",				"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_REPEATERINFO | CVAR_BOOL, "use g_viewerPassword for viewers/repeaters" );
idCVar g_viewerPassword(			"g_viewerPassword",				"",			CVAR_GAME | CVAR_ARCHIVE, "password for viewers" );
idCVar g_repeaterPassword(			"g_repeaterPassword",			"",			CVAR_GAME | CVAR_ARCHIVE, "password for repeaters" );

idCVar ri_privateViewers(			"ri_privateViewers",			"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_REPEATERINFO | CVAR_INTEGER, "number of private viewer slots" );
idCVar g_privateViewerPassword(		"g_privateViewerPassword",		"",			CVAR_GAME | CVAR_ARCHIVE, "privatePassword for private viewer slots" );

idCVar ri_name(						"ri_name",						"",			CVAR_GAME | CVAR_ARCHIVE | CVAR_REPEATERINFO, "override the server's si_name with this for relays" );

idCVar g_noTVChat(					"g_noTVChat",					"0",		CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "Server enable/disable flag for viewer chat on ETQW:TV" );

#endif // SD_SUPPORT_REPEATER

idCVar g_drawHudMessages(			"g_drawHudMessages",			"1",		CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE | CVAR_PROFILE,		"Draw task, task success and objective text on HUD." );
idCVar g_mineTriggerWarning(		"g_mineTriggerWarning",			"1",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE,		"Show warning message on HUD when triggering a proximity mine." );
idCVar g_aptWarning(				"g_aptWarning",					"3",		CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE | CVAR_PROFILE,		"Show warning message on HUD when APT is locking on. 0: Off 1: Visual warning only 2: Beep only 3: Visual and beep" );

idCVar g_trainingMode(				"g_trainingMode",				"0",		CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT,						"whether the game is in training mode or not" );

idCVar g_objectiveDecayTime(		"g_objectiveDecayTime",			"5",		CVAR_GAME | CVAR_FLOAT | CVAR_RANKLOCKED,					"Length of time in seconds that it takes a construct/hack objective to decay once after the initial timeout is complete" );

idCVar g_noQuickChats(				"g_noQuickChats",				"0",		CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE,		"disables sound and text of quickchats" );

idCVar g_maxProficiency(			"g_maxProficiency",				"0",		CVAR_GAME | CVAR_BOOL | CVAR_RANKLOCKED, "" );
