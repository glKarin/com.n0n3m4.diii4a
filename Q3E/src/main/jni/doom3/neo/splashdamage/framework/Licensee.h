// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LICENSEE_H__
#define __LICENSEE_H__

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME						"ETQW"		// appears on window titles and errors
#define GAME_ICON						"etqw.bmp"

#define ENGINE_VERSION_MAJOR			1
#define ENGINE_VERSION_MINOR			5

// paths
#define	CD_BASEDIR						"etqw"
#define	BASE_GAMEDIR					"base"

#define PREGENERATED_BASEDIR			"generated"

// filenames
#define	CD_EXE							"etqw.exe"
#if defined( ID_DEDICATED )
	#define BINDING_FILE					"etqwdedbinds.cfg"
	#define CONFIG_FILE						"etqwdedconfig.cfg"
#else
	#define BINDING_FILE					"etqwbinds.cfg"
	#define CONFIG_FILE						"etqwconfig.cfg"
#endif
#define PID_FILE						"etqw.pid"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"quack"

// default scripts
#define SCRIPT_DEFAULTDEFS				"script/defs.script"
#define SCRIPT_DEFAULT					"script/main.script"
#define SCRIPT_DEFAULTFUNC				"game_main"

// update server
#ifndef UPDATE_HOST
	#define UPDATE_HOST					"etqwupdate.idsoftware.com"
	// the IP is used as a fallback if the DNS resolution fails
	#define UPDATE_HOST_IP				"192.246.40.96"
	// same as default server port so we're less likely to get firewalled out
	#define UPDATE_PORT					27733
	// printed out as the default fallback URL when the update server doesn't reply
	#define UPDATE_FALLBACK_URL			"http://www.enemyterritory.com/"
#endif

// default network server port
#ifndef PORT_SERVER
	#define	PORT_SERVER					27733
#endif

#ifndef PORT_HTTP
	#define	PORT_HTTP					27733
#endif

// default network repeater port
#ifndef PORT_REPEATER
	#define	PORT_REPEATER				27833
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
const int NUM_SERVER_PORTS				= 4;

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
//
//	1: DOOM 3
//	2: Quake 4
//	10: ET:QW
//	11: ET:QW - private beta
//	12: ET:QW - demo
//	13: ET:QW - public beta
//	14: ET:QW - dev
//	22: Quake 4 Xbox 360
//	??: Prey
#if defined ( SD_PUBLIC_BETA_BUILD )
	#define ASYNC_PROTOCOL_MAJOR			13
#elif defined( SD_PRIVATE_BETA_BUILD )
	#define ASYNC_PROTOCOL_MAJOR			11
#elif defined ( SD_DEMO_BUILD )
	#define ASYNC_PROTOCOL_MAJOR			12
#elif defined ( SD_PUBLIC_BUILD )
	#define ASYNC_PROTOCOL_MAJOR			10
#else
	#define ASYNC_PROTOCOL_MAJOR			14
#endif

#if defined( SD_PUBLIC_BETA_BUILD )
	#define SD_BUILD_TAG L"Public Beta"
#elif defined( SD_PRIVATE_BETA_BUILD )
	#define SD_BUILD_TAG L"Private Beta"
#endif


// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// NOTE: a seperate core savegame version and game savegame version could be useful
//#define SAVEGAME_VERSION				16

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2: 2
#define RENDERDEMO_VERSION				2

#if defined( _WIN32 )
	// win32 info
	#define WIN32_CONSOLE_CLASS				"ETQW WinConsole"
	#define	WIN32_WINDOW_CLASS_NAME			"ETQW"
	#define	WIN32_FAKE_WINDOW_CLASS_NAME	"ETQW_WGL_FAKE"

	#if defined ( SD_PUBLIC_BETA_BUILD )
		#define DEFAULT_BASE_DIR				"id Software\\Enemy Territory - QUAKE Wars Public Beta"
	#elif defined ( SD_PRIVATE_BETA_BUILD )
		#define DEFAULT_BASE_DIR				"id Software\\Enemy Territory - QUAKE Wars Private Beta"
	#elif defined( SD_DEMO_BUILD )
		#define DEFAULT_BASE_DIR				"id Software\\Enemy Territory - QUAKE Wars Demo"
	#else
		#define DEFAULT_BASE_DIR				"id Software\\Enemy Territory - QUAKE Wars"
	#endif
#elif defined( __linux__ )
	// Linux info
	#define DEFAULT_BASE_PATH				"/usr/local/games/etqw"
#elif defined( MACOS_X )
	#define DEFAULT_BASE_PATH				"/Applications/Enemy Territory: QUAKE Wars"
#endif

#define CONFIG_SPEC						"etqwconfig.spec"

#endif // __LICENSEE_H__
