/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#ifdef _RAVEN // quake4

// rjohnson: this is the name of the game we are making
#define GAME_NAME						"Quake4"			// appears on window titles and errors
#define GAME_ICON						"q4icon.bmp"

#include "../raven/sys/AutoVersion.h"
#define ENGINE_VERSION					"Quake4 " VERSION_STRING_DOTTED // "DOOM 1.3.1"	// printed in console

// jnewquist: build type
#if defined(_DEBUG)
#define GAME_BUILD_TYPE					"Debug"
#elif defined(_MPBETA)
#define GAME_BUILD_TYPE					"MPBeta"
#elif defined(_FINAL)
#define GAME_BUILD_TYPE					""
#elif defined(_RELEASE)
#define	GAME_BUILD_TYPE					""
#else
#define	GAME_BUILD_TYPE					""
#endif

// paths
#define	CD_BASEDIR						"Quake4"
#ifdef ID_DEMO_BUILD
#define BASE_GAMEDIR					"demo"
#else
#define	BASE_GAMEDIR					"q4base"
#endif
#define	DEMO_GAMEDIR					"demo"
#define	BASE_MPGAMEDIR					"q4mp"

// filenames
#define	CD_EXE							"Quake4.exe"
#define CONFIG_FILE						"Quake4Config.cfg"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"neo"
//#define SOURCE_CODE_BASE_FOLDER			"code"

#define DEVELOPER_DOMAIN				"ravensoft.com"

#ifndef MOTD_HOST
	#define MOTD_HOST					"q4m-test.ravensoft.com"
#endif

#ifndef MOTD_PORT
	#define MOTD_PORT					"27700"
#endif

// Q4TV default network repeater port
#ifndef PORT_REPEATER
#define PORT_REPEATER					28104
#endif

#ifndef PORT_HTTP
#define PORT_HTTP					28004
#endif

// RAVEN BEGIN
// rjohnson: changed the host to our temp address
// default idnet host address
#ifndef IDNET_HOST
#define IDNET_HOST					"q4master.idsoftware.com"
#endif
// RAVEN END

// default idnet master port
#ifndef IDNET_MASTER_PORT
#define IDNET_MASTER_PORT			"27650"
#endif

// default network server port
#ifndef PORT_SERVER
//RAVEN BEGIN
#define	PORT_SERVER					28004
//RAVEN END
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
// RAVEN BEGIN
// ddynerman: rev ASYNC_PROTOCOL_MAJOR to 2 for Quake 4
#define ASYNC_PROTOCOL_MAJOR			2
// RAVEN END

// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// For testing, we're using the build number to ensure no one ever tries to load a stale savegame
//#define SAVEGAME_VERSION				VERSION_BUILD_NUMBER
#define SAVEGAME_VERSION				17

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2: 2
#define RENDERDEMO_VERSION				2

// editor info
#define EDITOR_DEFAULT_PROJECT			"doom.qe4"
#define EDITOR_REGISTRY_KEY				"DOOMRadiant"
#define EDITOR_WINDOWTEXT				"QuakeEdit"

// win32 info
#define WIN32_CONSOLE_CLASS				"Quake 4 WinConsole"
#define WIN32_SPLASH_CLASS				"Quake 4 Splash"
#define	WIN32_WINDOW_CLASS_NAME			"Quake4"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"QUAKE4_WGL_FAKE"

// Linux info
#ifdef ID_DEMO_BUILD
#define DEFAULT_BASE_PATH				"/usr/local/games/quake4-demo"
#else
#define DEFAULT_BASE_PATH				"/usr/local/games/quake4"
#endif

// CD Key file info
#define CDKEY_FILE						"quake4key"
#define CDKEY_TEXT						"\n// Do not give this file to ANYONE.\n" \
										"// id Software, Raven Software or Activision will NOT ask you to send this file to them.\n"

// FIXME: Update to Doom
// Product ID. Stored in "productid.txt".
//										This file is copyright 1999 Id Software, and may not be duplicated except during a licensed installation of the full commercial version of Quake 3:Arena
#undef PRODUCT_ID
#define PRODUCT_ID						220, 129, 255, 108, 244, 163, 171, 55, 133, 65, 199, 36, 140, 222, 53, 99, 65, 171, 175, 232, 236, 193, 210, 250, 169, 104, 231, 231, 21, 201, 170, 208, 135, 175, 130, 136, 85, 215, 71, 23, 96, 32, 96, 83, 44, 240, 219, 138, 184, 215, 73, 27, 196, 247, 55, 139, 148, 68, 78, 203, 213, 238, 139, 23, 45, 205, 118, 186, 236, 230, 231, 107, 212, 1, 10, 98, 30, 20, 116, 180, 216, 248, 166, 35, 45, 22, 215, 229, 35, 116, 250, 167, 117, 3, 57, 55, 201, 229, 218, 222, 128, 12, 141, 149, 32, 110, 168, 215, 184, 53, 31, 147, 62, 12, 138, 67, 132, 54, 125, 6, 221, 148, 140, 4, 21, 44, 198, 3, 126, 12, 100, 236, 61, 42, 44, 251, 15, 135, 14, 134, 89, 92, 177, 246, 152, 106, 124, 78, 118, 80, 28, 42
#undef PRODUCT_ID_LENGTH
#define PRODUCT_ID_LENGTH				152

#define CONFIG_SPEC						"config.spec"

// compat
#define XPKEY_FILE						"xpkey"

#elif defined(_HUMANHEAD)
#define GAME_NAME						"Prey"			// appears on window titles and errors

#define	ENGINE_VERSION					"Prey 1.2"		// printed in console

// paths
#define	CD_BASEDIR						"Prey"
//HUMANHEAD jsh PCF 5/26/06: prey demo uses "base" so that save games all go in same folder
//#ifdef ID_DEMO_BUILD
//	#define BASE_GAMEDIR					"demo"
//#else
#define	BASE_GAMEDIR					"preybase" // base
//#endif

// filenames
#define	CD_EXE							"prey.exe"
#define CONFIG_FILE						"preyconfig.cfg"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"."


// default idnet host address
#ifndef IDNET_HOST
	//Q4MASTER
	//#define IDNET_HOST				"q4master.idsoftware.com"
#define IDNET_HOST					"preymaster.humanhead.com"
#endif

// default idnet master port
#ifndef IDNET_MASTER_PORT
#ifdef ID_DEMO_BUILD
#define IDNET_MASTER_PORT			"27655"
#else
#define IDNET_MASTER_PORT			"27650"
#endif
#endif

// default network server port
#ifndef PORT_SERVER
#define	PORT_SERVER					27719	// ID used 27666
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
//Q4MASTER
//use 2
#ifdef ID_DEMO_BUILD
#define ASYNC_PROTOCOL_MAJOR			4	// Doom3=1, Quake4=2, Prey=3, Prey Demo=4
#else
#define ASYNC_PROTOCOL_MAJOR			3	// Doom3=1, Quake4=2, Prey=3
#endif

// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// NOTE: a seperate core savegame version and game savegame version could be useful
#define SAVEGAME_VERSION				114

#define RENDERDEMO_VERSION				100

// editor info
#define EDITOR_DEFAULT_PROJECT			"prey.qe4"
#define EDITOR_REGISTRY_KEY				"PreyRadiant"
#define EDITOR_WINDOWTEXT				"PREDITOR"

// win32 info
#define WIN32_CONSOLE_CLASS				"Prey WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"PREY"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"PREY_WGL_FAKE"

// Linux info
#ifdef ID_DEMO_BUILD
#define LINUX_DEFAULT_PATH			"/usr/local/games/prey-demo"
#else
#define LINUX_DEFAULT_PATH			"/usr/local/games/prey"
#endif

// CD Key file info
// goes into BASE_GAMEDIR whatever the fs_game is set to
// two distinct files for easier win32 installer job
#define CDKEY_FILE						"preykey"
#ifdef HUMANHEAD_XP // HUMANHEAD mdl
#define XPKEY_FILE						"xpkey"
#endif // HUMANHEAD END
// HUMANHEAD PCF mdl 05-08-06 - Changed CDKEY_TEXT
#define CDKEY_TEXT						"\n// Do not give this file to ANYONE.\n" \
										"// Human Head Studios and 2K Games will NOT ask you to send this file to them.\n"

// FIXME: Update to Doom
// Product ID. Stored in "productid.txt".
//										This file is copyright 1999 Id Software, and may not be duplicated except during a licensed installation of the full commercial version of Quake 3:Arena
#undef PRODUCT_ID
#define PRODUCT_ID						220, 129, 255, 108, 244, 163, 171, 55, 133, 65, 199, 36, 140, 222, 53, 99, 65, 171, 175, 232, 236, 193, 210, 250, 169, 104, 231, 231, 21, 201, 170, 208, 135, 175, 130, 136, 85, 215, 71, 23, 96, 32, 96, 83, 44, 240, 219, 138, 184, 215, 73, 27, 196, 247, 55, 139, 148, 68, 78, 203, 213, 238, 139, 23, 45, 205, 118, 186, 236, 230, 231, 107, 212, 1, 10, 98, 30, 20, 116, 180, 216, 248, 166, 35, 45, 22, 215, 229, 35, 116, 250, 167, 117, 3, 57, 55, 201, 229, 218, 222, 128, 12, 141, 149, 32, 110, 168, 215, 184, 53, 31, 147, 62, 12, 138, 67, 132, 54, 125, 6, 221, 148, 140, 4, 21, 44, 198, 3, 126, 12, 100, 236, 61, 42, 44, 251, 15, 135, 14, 134, 89, 92, 177, 246, 152, 106, 124, 78, 118, 80, 28, 42
#undef PRODUCT_ID_LENGTH
#define PRODUCT_ID_LENGTH				152

#define CONFIG_SPEC						"config.spec"

// HUMANHEAD pdm: Our additions go here
#define	SINGLE_MAP_BUILD				1		// For single map external builds
#define PARTICLE_BOUNDS					1		// rdr - New type of particle bounds calc
#define SOUND_TOOLS_BUILD				1		// Turn on for making builds for Ed to change reverbs, should be 0 in gold build
#define GUIS_IN_DEMOS					1		// Include guis in demo streams
#define	MUSICAL_LEVELLOADS				1		// Allow music playback during level loads
#define GAMEPORTAL_PVS					0		// Allow PVS to flow through game portals
#define GAMEPORTAL_SOUND				0		// Allow sound to flow through game portals (requires GAMEPORTAL_PVS)
#define DEATHWALK_AUTOLOAD				1		// Append deathwalk map to all maps loaded
#define	GAME_PLAYERDEFNAME				"player_tommy"
#define GAME_PLAYERDEFNAME_MP			"player_tommy_mp"
#define AUTOMAP							0
#define EDITOR_HELP_LOCATOR				"http://www.3drealms.com/prey/wiki"
#define PREY_SITE_LOCATOR				"http://www.prey.com"
#define CREATIVE_DRIVER_LOCATOR			"http://www.creative.com/language.asp?sDestUrl=/support/downloads"
#define TRITON_LOCATOR					"http://www.playtriton.com/prey"
#define NVIDIA_DRIVER_LOCATOR			"www.nvidia.com"
#define ATI_DRIVER_LOCATOR				"www.ati.com"
#define _HH_RENDERDEMO_HACKS			1		//rww - if 1 enables hacks to make renderdemos work through whatever nefarious means necessary
#define _HH_CLIP_FASTSECTORS			1		//rww - much faster method for clip sector checking
#define NEW_MESH_TRANSFORM				0		//bjk - SSE new vert transform
#define SIMD_SHADOW						0		//bjk - simd shadow calculations
#define MULTICORE						0		// Multicore optimizations
#define DEBUG_SOUND_LOG					0		// Write out a debug log, remove from final build
#ifdef _USE_SECUROM_ // mdl: Only enable securom for certain builds
#define _HH_SECUROM						1		//rww - enables securom api hooks
#else
#define _HH_SECUROM						0
#endif
#define _HH_INLINED_PROC_CLIPMODELS		0		//rww - enables crazy last-minute proc geometry clipmodel support

#ifdef ID_DEDICATED
#define _HH_MYGAMES_SAVES				0
#else
#define _HH_MYGAMES_SAVES				1		//HUMANHEAD PCF rww 05/10/06 - use My Games for saves
#endif

// Map to start with 'New Game' button
#ifdef ID_DEMO_BUILD
#define STARTING_LEVEL					"roadhouse"
#else
#if SINGLE_MAP_BUILD
#define STARTING_LEVEL				"feedingtowerc"
#else
#define STARTING_LEVEL				"feedingtowera"
#endif
#endif

#ifdef ID_DEMO_BUILD
#define DEMO_END_LEVEL					"game/feedingtowerd"
#endif

// Flip these for final final
//HH rww SDK - gold must be enabled, to match the state of the release exe.
#if 1
#define GOLD							1		// Used for disabling development code
#define CONSOLE_IDENTITY				0		// Print build identity on console
#define REMOTE_DMAP						0		// Remote dmap support (development only)
#else
#define GOLD							0       // Off for development
#define CONSOLE_IDENTITY				1		// Print build identity on console
#define REMOTE_DMAP						1		// Remote dmap support (development only)
#endif

// profiler/debugger
#ifdef ID_DEMO_BUILD
#define INGAME_DEBUGGER_ENABLED			0
#define INGAME_PROFILER_ENABLED			0
#else
#define INGAME_DEBUGGER_ENABLED			0
#define INGAME_PROFILER_ENABLED			0
#endif

#ifndef _NODONGLE_
#define __HH_DONGLE__					0		// always require a dongle somewhere on the net
#endif
#ifdef _GERMAN_BUILD_
#	define GERMAN_VERSION					1
#else
#	define GERMAN_VERSION					0		// Set to 1 to disable gore
#endif

// Venom
#define XENON							0		// Turn on at venom: anything exclusive to the xenon port
#define VENOM							0		// Turn on at venom
#define GAMEPAD_SUPPORT					0		// windows gamepad controller support (doesn't work on win2000)

#if !GOLD //rww - if not gold enable network entity stat tracking and float protection
//HUMANHEAD PCF rww 04/26/06 - i'm done with these internally for now as well.
//#define _HH_NET_DEBUGGING
//#define _HH_FLOAT_PROTECTION
#endif


// mdl:  Moved here from BuildDefines.h because we need GOLD and _SYSTEM_BUILD_ defined
#ifndef ID_ENFORCE_KEY
#	if GOLD && !defined( _INTERNAL_BUILD_ ) && !defined( _SYSTEM_BUILD_ ) && !defined( ID_DEDICATED ) && !defined( ID_DEMO_BUILD ) && !defined( _HH_DEMO_BUILD_ ) // HUMANHEAD mdl:  Added GOLD, _SYSTEM_BUILD_, and _HH_DEMO_BUILD_
#		define ID_ENFORCE_KEY 1
#	else
#		define ID_ENFORCE_KEY 0
#	endif
#endif

// HUMANHEAD mdl: save debugging
//#define HUMANHEAD_SAVEDEBUG
//#define HUMANHEAD_TESTSAVEGAME

// HUMANHEAD END

// compat
#define XPKEY_FILE						"xpkey"

#else // else is DOOM3

#define GAME_NAME						"DOOM 3"		// appears on window titles and errors

#define ENGINE_VERSION					"DOOM 1.3.1"	// printed in console

// paths
#define	CD_BASEDIR						"Doom"
#ifdef ID_DEMO_BUILD
#define BASE_GAMEDIR					"demo"
#else
#define	BASE_GAMEDIR					"base"
#endif

// filenames
#define	CD_EXE							"doom.exe"
#define CONFIG_FILE						"DoomConfig.cfg"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"neo"


// default idnet host address
#ifndef IDNET_HOST
#define IDNET_HOST					"idnet.ua-corp.com"
#endif

// default idnet master port
#ifndef IDNET_MASTER_PORT
#define IDNET_MASTER_PORT			"27650"
#endif

// default network server port
#ifndef PORT_SERVER
#define	PORT_SERVER					27666
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR			1

// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// NOTE: a seperate core savegame version and game savegame version could be useful
// 16: Doom v1.1
// 17: Doom v1.2 / D3XP. Can still read old v16 with defaults for new data
#define SAVEGAME_VERSION				17

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2: 2
#define RENDERDEMO_VERSION				2

// editor info
#define EDITOR_DEFAULT_PROJECT			"doom.qe4"
#define EDITOR_REGISTRY_KEY				"DOOMRadiant"
#define EDITOR_WINDOWTEXT				"DOOMEdit"

// win32 info
#define WIN32_CONSOLE_CLASS				"DOOM 3 WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"DOOM3"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"DOOM3_WGL_FAKE"

// Linux info
#ifdef ID_DEMO_BUILD
#define LINUX_DEFAULT_PATH			"/usr/local/games/doom3-demo"
#else
#define LINUX_DEFAULT_PATH			"/usr/local/games/doom3"
#endif

// CD Key file info
// goes into BASE_GAMEDIR whatever the fs_game is set to
// two distinct files for easier win32 installer job
#define CDKEY_FILE						"doomkey"
#define XPKEY_FILE						"xpkey"
#define CDKEY_TEXT						"\n// Do not give this file to ANYONE.\n" \
										"// id Software or Zenimax will NEVER ask you to send this file to them.\n"

#define CONFIG_SPEC						"config.spec"

#endif

#define ANDROID_DEFAULT_PATH			"/sdcard/Android/data/com.karin.idTech4Amm"

